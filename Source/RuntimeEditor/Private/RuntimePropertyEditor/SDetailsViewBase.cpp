// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SDetailsViewBase.h"

#include "Classes/SodaStyleSettings.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"
//#include "Settings/EditorExperimentalSettings.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorEditInline.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SSearchBox.h"

#include "RuntimePropertyEditor/CategoryPropertyNode.h"
#include "RuntimePropertyEditor/DetailCategoryBuilderImpl.h"
#include "RuntimePropertyEditor/DetailLayoutBuilderImpl.h"
#include "RuntimePropertyEditor/DetailLayoutHelpers.h"
#include "RuntimePropertyEditor/DetailPropertyRow.h"
//#include "EditConditionParser.h"
//#include "Editor.h"
//#include "EditorConfigSubsystem.h"
#include "RuntimePropertyEditor/IDetailCustomization.h"
#include "ObjectEditorUtils.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
//#include "ScopedTransaction.h"
#include "RuntimePropertyEditor/SDetailNameArea.h"
#include "RuntimePropertyEditor/SDetailsView.h"
#include "RuntimePropertyEditor/PropertyEditorPermissionList.h"
#include "RuntimePropertyEditor/DetailsViewConfig2.h"

//#include "ThumbnailRendering/ThumbnailManager.h"

namespace soda
{

SDetailsViewBase::SDetailsViewBase() 
	: bHasActiveFilter(false)
	, bIsLocked(false)
	, bHasOpenColorPicker(false)
	, bDisableCustomDetailLayouts( false )
	, NumVisibleTopLevelObjectNodes(0)
	, bPendingCleanupTimerSet(false)
{
	UDetailsConfig2* DetailsConfig = GetMutableDefault<UDetailsConfig2>();
	//DetailsConfig->LoadEditorConfig();

	const FDetailsViewConfig2* ViewConfig = GetConstViewConfig();
	if (ViewConfig != nullptr)
	{
		CurrentFilter.bShowAllAdvanced = ViewConfig->bShowAllAdvanced;
		CurrentFilter.bShowAllChildrenIfCategoryMatches = ViewConfig->bShowAllChildrenIfCategoryMatches;
		CurrentFilter.bShowOnlyAnimated = ViewConfig->bShowOnlyAnimated;
		CurrentFilter.bShowOnlyKeyable = ViewConfig->bShowOnlyKeyable;
		CurrentFilter.bShowOnlyModified = ViewConfig->bShowOnlyModified;
	}

	PropertyPermissionListChangedDelegate = FPropertyEditorPermissionList::Get().PermissionListUpdatedDelegate.AddLambda([this](TSoftObjectPtr<UStruct> Struct, FName Owner) { ForceRefresh(); });
	PropertyPermissionListEnabledDelegate = FPropertyEditorPermissionList::Get().PermissionListEnabledDelegate.AddRaw(this, &SDetailsViewBase::ForceRefresh);
}

SDetailsViewBase::~SDetailsViewBase()
{
	FPropertyEditorPermissionList::Get().PermissionListUpdatedDelegate.Remove(PropertyPermissionListChangedDelegate);
	FPropertyEditorPermissionList::Get().PermissionListEnabledDelegate.Remove(PropertyPermissionListEnabledDelegate);
}

void SDetailsViewBase::OnGetChildrenForDetailTree(TSharedRef<FDetailTreeNode> InTreeNode, TArray< TSharedRef<FDetailTreeNode> >& OutChildren)
{
	InTreeNode->GetChildren(OutChildren);
}

TSharedRef<ITableRow> SDetailsViewBase::OnGenerateRowForDetailTree(TSharedRef<FDetailTreeNode> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return InTreeNode->GenerateWidgetForTableView(OwnerTable, DetailsViewArgs.bAllowFavoriteSystem);
}

void SDetailsViewBase::SetRootExpansionStates(const bool bExpand, const bool bRecurse)
{
	if(ContainsMultipleTopLevelObjects())
	{
		FDetailNodeList Children;
		for(auto Iter = RootTreeNodes.CreateIterator(); Iter; ++Iter)
		{
			Children.Reset();
			(*Iter)->GetChildren(Children);

			for(TSharedRef<class FDetailTreeNode>& Child : Children)
			{
				SetNodeExpansionState(Child, bExpand, bRecurse);
			}
		}
	}
	else
	{
		for(auto Iter = RootTreeNodes.CreateIterator(); Iter; ++Iter)
		{
			SetNodeExpansionState(*Iter, bExpand, bRecurse);
		}
	}
}

void SDetailsViewBase::SetNodeExpansionState(TSharedRef<FDetailTreeNode> InTreeNode, bool bIsItemExpanded, bool bRecursive)
{
	TArray< TSharedRef<FDetailTreeNode> > Children;
	InTreeNode->GetChildren(Children);

	if (Children.Num())
	{
		RequestItemExpanded(InTreeNode, bIsItemExpanded);
		const bool bShouldSaveState = true;
		InTreeNode->OnItemExpansionChanged(bIsItemExpanded, bShouldSaveState);

		if (bRecursive)
		{
			for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
			{
				TSharedRef<FDetailTreeNode> Child = Children[ChildIndex];

				SetNodeExpansionState(Child, bIsItemExpanded, bRecursive);
			}
		}
	}
}

void SDetailsViewBase::SetNodeExpansionStateRecursive(TSharedRef<FDetailTreeNode> InTreeNode, bool bIsItemExpanded)
{
	SetNodeExpansionState(InTreeNode, bIsItemExpanded, true);
}

void SDetailsViewBase::OnItemExpansionChanged(TSharedRef<FDetailTreeNode> InTreeNode, bool bIsItemExpanded)
{
	SetNodeExpansionState(InTreeNode, bIsItemExpanded, false);
}

FReply SDetailsViewBase::OnLockButtonClicked()
{
	bIsLocked = !bIsLocked;
	return FReply::Handled();
}

void SDetailsViewBase::HideFilterArea(bool bHide)
{
	DetailsViewArgs.bAllowSearch = !bHide;
}

static void GetPropertiesInOrderDisplayedRecursive(const TArray< TSharedRef<FDetailTreeNode> >& TreeNodes, TArray< FPropertyPath > &OutLeaves)
{
	for( const TSharedRef<FDetailTreeNode>& TreeNode : TreeNodes )
	{
		const bool bIsPropertyRoot = TreeNode->IsLeaf() ||
			(	TreeNode->GetPropertyNode() && 
				TreeNode->GetPropertyNode()->GetProperty() );
		if( bIsPropertyRoot )
		{
			FPropertyPath Path = TreeNode->GetPropertyPath();
			// Some leaf nodes are not associated with properties, specifically the collision presets.
			// @todo doc: investigate what we can do about this, result is that for these fields
			// we can't highlight the property in the diff tool.
			if( Path.GetNumProperties() != 0 )
			{
				OutLeaves.Push(Path);
			}
		}
		else
		{
			TArray< TSharedRef<FDetailTreeNode> > Children;
			TreeNode->GetChildren(Children);
			GetPropertiesInOrderDisplayedRecursive(Children, OutLeaves);
		}
	}
}
TArray< FPropertyPath > SDetailsViewBase::GetPropertiesInOrderDisplayed() const
{
	TArray< FPropertyPath > Ret;
	GetPropertiesInOrderDisplayedRecursive(RootTreeNodes, Ret);
	return Ret;
}

// @return populates OutNodes with the leaf node corresponding to property as the first entry in the list (e.g. [leaf, parent, grandparent]):
static void FindTreeNodeFromPropertyRecursive( const TArray< TSharedRef<FDetailTreeNode> >& Nodes, const FPropertyPath& Property, TArray< TSharedPtr< FDetailTreeNode > >& OutNodes )
{
	if (Property == FPropertyPath())
	{
		return;
	}

	for (const TSharedRef<FDetailTreeNode>& TreeNode : Nodes)
	{
		if (TreeNode->IsLeaf())
		{
			FPropertyPath tmp = TreeNode->GetPropertyPath();
			if( Property == tmp )
			{
				OutNodes.Push(TreeNode);
				return;
			}
		}

		// Need to check children even if we're a leaf, because all DetailItemNodes are leaves, even if they may have sub-children
		TArray< TSharedRef<FDetailTreeNode> > Children;
		TreeNode->GetChildren(Children);
		FindTreeNodeFromPropertyRecursive(Children, Property, OutNodes);
		if (OutNodes.Num() > 0)
		{
			OutNodes.Push(TreeNode);
			return;
		}
	}
}

void SDetailsViewBase::HighlightProperty(const FPropertyPath& Property)
{
	TSharedPtr<FDetailTreeNode> PrevHighlightedNodePtr = CurrentlyHighlightedNode.Pin();
	if (PrevHighlightedNodePtr.IsValid())
	{
		PrevHighlightedNodePtr->SetIsHighlighted(false);
	}

	TSharedPtr< FDetailTreeNode > FinalNodePtr = nullptr;
	TArray< TSharedPtr< FDetailTreeNode > > TreeNodeChain;
	FindTreeNodeFromPropertyRecursive(RootTreeNodes, Property, TreeNodeChain);
	if (TreeNodeChain.Num() > 0)
	{
		FinalNodePtr = TreeNodeChain[0];
		check(FinalNodePtr.IsValid());
		FinalNodePtr->SetIsHighlighted(true);

		for (int ParentIndex = 1; ParentIndex < TreeNodeChain.Num(); ++ParentIndex)
		{
			TSharedPtr< FDetailTreeNode > CurrentParent = TreeNodeChain[ParentIndex];
			check(CurrentParent.IsValid());
			DetailTree->SetItemExpansion(CurrentParent.ToSharedRef(), true);
		}

		DetailTree->RequestScrollIntoView(FinalNodePtr.ToSharedRef());
	}
	CurrentlyHighlightedNode = FinalNodePtr;
}

void SDetailsViewBase::ShowAllAdvancedProperties()
{
	CurrentFilter.bShowAllAdvanced = true;
}

void SDetailsViewBase::SetOnDisplayedPropertiesChanged(FOnDisplayedPropertiesChanged InOnDisplayedPropertiesChangedDelegate)
{
	OnDisplayedPropertiesChangedDelegate = InOnDisplayedPropertiesChangedDelegate;
}

void SDetailsViewBase::RerunCurrentFilter()
{
	UpdateFilteredDetails();
}

EVisibility SDetailsViewBase::GetTreeVisibility() const
{
	for(const FDetailLayoutData& Data : DetailLayouts)
	{
		if(Data.DetailLayout.IsValid() && Data.DetailLayout->HasDetails())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility SDetailsViewBase::GetScrollBarVisibility() const
{
	const bool bHasAnythingToShow = RootTreeNodes.Num() > 0;
	const bool bShowScrollBar = DetailsViewArgs.bShowScrollBar && bHasAnythingToShow;
	return bShowScrollBar ? EVisibility::Visible : EVisibility::Collapsed;
}

/** Returns the image used for the icon on the filter button */
const FSlateBrush* SDetailsViewBase::OnGetFilterButtonImageResource() const
{
	if (HasActiveSearch())
	{
		return FSodaStyle::GetBrush(TEXT("PropertyWindow.FilterCancel"));
	}
	else
	{
		return FSodaStyle::GetBrush(TEXT("PropertyWindow.FilterSearch"));
	}
}

void SDetailsViewBase::EnqueueDeferredAction(FSimpleDelegate& DeferredAction)
{
	DeferredActions.Add(DeferredAction);
}

/**
* Creates the color picker window for this property view.
*
* @param Node				The slate property node to edit.
* @param bUseAlpha			Whether or not alpha is supported
*/
void SDetailsViewBase::CreateColorPickerWindow(const TSharedRef< FPropertyEditor >& PropertyEditor, bool bUseAlpha)
{
	const FProperty* Property = PropertyEditor->GetProperty();
	check(Property);

	FReadAddressList ReadAddresses;
	PropertyEditor->GetPropertyNode()->GetReadAddress(false, ReadAddresses, false);

	TOptional<FLinearColor> DefaultColor;
	bool bClampValue = false;
	if (ReadAddresses.Num())
	{
		for (int32 ColorIndex = 0; ColorIndex < ReadAddresses.Num(); ++ColorIndex)
		{
			const uint8* Addr = ReadAddresses.GetAddress(ColorIndex);
			if (Addr)
			{
				if (CastField<FStructProperty>(Property)->Struct->GetFName() == NAME_Color)
				{
					DefaultColor = *reinterpret_cast<const FColor*>(Addr);
					bClampValue = true;
				}
				else
				{
					check(CastField<FStructProperty>(Property)->Struct->GetFName() == NAME_LinearColor);
					DefaultColor = *reinterpret_cast<const FLinearColor*>(Addr);
				}
			}
		}
	}

	if (DefaultColor.IsSet())
	{
		bHasOpenColorPicker = true;
		ColorPropertyNode = PropertyEditor->GetPropertyNode();

		TWeakPtr<IPropertyHandle> WeakPropertyHandle = PropertyEditor->GetPropertyHandle();
		FColorPickerArgs PickerArgs = FColorPickerArgs(DefaultColor.GetValue(), FOnLinearColorValueChanged::CreateSP(this, &SDetailsViewBase::SetColorPropertyFromColorPicker));
		PickerArgs.ParentWidget = AsShared();
		PickerArgs.bUseAlpha = bUseAlpha;
		PickerArgs.bClampValue = bClampValue;
		PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
		PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP(this, &SDetailsViewBase::OnColorPickerWindowClosed);
		PickerArgs.OptionalOwningDetailsView = AsShared();
		OpenColorPicker(PickerArgs);
	}
}

void SDetailsViewBase::SetColorPropertyFromColorPicker(FLinearColor NewColor)
{
	const TSharedPtr< FPropertyNode > PinnedColorPropertyNode = ColorPropertyNode.Pin();
	if (ensure(PinnedColorPropertyNode.IsValid()))
	{
		FProperty* Property = PinnedColorPropertyNode->GetProperty();
		check(Property);

		FObjectPropertyNode* ObjectNode = PinnedColorPropertyNode->FindObjectItemParent();

		if (ObjectNode && ObjectNode->GetNumObjects())
		{
			//FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "SetColorProperty", "Set Color Property"));

			PinnedColorPropertyNode->NotifyPreChange(Property, GetNotifyHook());

			FPropertyChangedEvent ChangeEvent(Property, EPropertyChangeType::ValueSet);
			PinnedColorPropertyNode->NotifyPostChange(ChangeEvent, GetNotifyHook());
		}
	}
}


void SDetailsViewBase::UpdatePropertyMaps()
{
	RootTreeNodes.Empty();

	for(FDetailLayoutData& LayoutData : DetailLayouts)
	{
		// Check uniqueness.  It is critical that detail layouts can be destroyed
		// We need to be able to create a new detail layout and properly clean up the old one in the process
		check(!LayoutData.DetailLayout.IsValid() || LayoutData.DetailLayout.IsUnique());

		// Allow customizations to perform cleanup as the delete occurs later on
		for (TSharedPtr<IDetailCustomization>& DetailCustomization : LayoutData.CustomizationClassInstances)
		{
			if (DetailCustomization.IsValid())
			{
				DetailCustomization->PendingDelete();
			}
		}

		// All the current customization instances need to be deleted when it is safe
		CustomizationClassInstancesPendingDelete.Append(LayoutData.CustomizationClassInstances);

		// All the current detail layouts need to be deleted when it is safe
		DetailLayoutsPendingDelete.Add(LayoutData.DetailLayout);
	}

	if (DetailLayouts.Num() > 0)
	{
		// Set timer to free memory even when widget is not visible or ticking
		SetPendingCleanupTimer();
	}

	FRootPropertyNodeList& RootPropertyNodes = GetRootNodes();
	
	DetailLayouts.Empty(RootPropertyNodes.Num());

	// There should be one detail layout for each root node
	DetailLayouts.AddDefaulted(RootPropertyNodes.Num());

	for(int32 RootNodeIndex = 0; RootNodeIndex < RootPropertyNodes.Num(); ++RootNodeIndex)
	{
		FDetailLayoutData& LayoutData = DetailLayouts[RootNodeIndex];
		UpdateSinglePropertyMap(RootPropertyNodes[RootNodeIndex], LayoutData, false);
	}
}

void SDetailsViewBase::UpdateSinglePropertyMap(TSharedPtr<FComplexPropertyNode> InRootPropertyNode, FDetailLayoutData& LayoutData, bool bIsExternal)
{
	// Reset everything
	LayoutData.ClassToPropertyMap.Empty();

	TSharedPtr<FDetailLayoutBuilderImpl> DetailLayout = MakeShareable(new FDetailLayoutBuilderImpl(InRootPropertyNode, LayoutData.ClassToPropertyMap, PropertyUtilities.ToSharedRef(), PropertyGenerationUtilities.ToSharedRef(), SharedThis(this), bIsExternal));
	LayoutData.DetailLayout = DetailLayout;

	TSharedPtr<FComplexPropertyNode> RootPropertyNode = InRootPropertyNode;
	check(RootPropertyNode.IsValid());

	const bool bEnableFavoriteSystem = IsEngineExitRequested() ? false : DetailsViewArgs.bAllowFavoriteSystem;

	DetailLayoutHelpers::FUpdatePropertyMapArgs Args;

	Args.LayoutData = &LayoutData;
	Args.InstancedPropertyTypeToDetailLayoutMap = &InstancedTypeToLayoutMap;
	Args.IsPropertyReadOnly = [this](const FPropertyAndParent& PropertyAndParent) { return IsPropertyReadOnly(PropertyAndParent); };
	Args.IsPropertyVisible = [this](const FPropertyAndParent& PropertyAndParent) { return IsPropertyVisible(PropertyAndParent); };
	Args.bEnableFavoriteSystem = bEnableFavoriteSystem;
	Args.bUpdateFavoriteSystemOnly = false;
	DetailLayoutHelpers::UpdateSinglePropertyMapRecursive(*RootPropertyNode, NAME_None, RootPropertyNode.Get(), Args);

	CustomUpdatePropertyMap(LayoutData.DetailLayout);

	// Ask for custom detail layouts, unless disabled. One reason for disabling custom layouts is that the custom layouts
	// inhibit our ability to find a single property's tree node. This is problematic for the diff and merge tools, that need
	// to display and highlight each changed property for the user. We could allow 'known good' customizations here if 
	// we can make them work with the diff/merge tools.
	if (!bDisableCustomDetailLayouts)
	{
		DetailLayoutHelpers::QueryCustomDetailLayout(LayoutData, InstancedClassToDetailLayoutMap, GenericLayoutDelegate);
	}

	if (bEnableFavoriteSystem && InRootPropertyNode->GetInstancesNum() > 0)
	{
		static const FName FavoritesCategoryName("Favorites");
		FDetailCategoryImpl& FavoritesCategory = LayoutData.DetailLayout->DefaultCategory(FavoritesCategoryName);
		FavoritesCategory.SetSortOrder(0);
		FavoritesCategory.SetCategoryAsSpecialFavorite();

		if (FavoritesCategory.GetNumCustomizations() == 0)
		{
			FavoritesCategory.AddCustomRow(NSLOCTEXT("DetailLayoutHelpers", "Favorites", "Favorites"))
				.WholeRowContent()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DetailLayoutHelpers", "AddToFavoritesDescription", "Right-click on a property to add it to your Favorites."))
					.TextStyle(FAppStyle::Get(), "HintText")
				];
		}
	}

	LayoutData.DetailLayout->GenerateDetailLayout();
}

void SDetailsViewBase::OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window)
{
	const TSharedPtr< FPropertyNode > PinnedColorPropertyNode = ColorPropertyNode.Pin();
	if (ensure(PinnedColorPropertyNode.IsValid()))
	{
		FProperty* Property = PinnedColorPropertyNode->GetProperty();
		if (Property && PropertyUtilities.IsValid())
		{
			FPropertyChangedEvent ChangeEvent(Property, EPropertyChangeType::ArrayAdd);
			PinnedColorPropertyNode->FixPropertiesInEvent(ChangeEvent);
			PropertyUtilities->NotifyFinishedChangingProperties(ChangeEvent);
		}
	}

	// A color picker window is no longer open
	bHasOpenColorPicker = false;
	ColorPropertyNode.Reset();
}

void SDetailsViewBase::SetIsPropertyVisibleDelegate(FIsPropertyVisible InIsPropertyVisible)
{
	IsPropertyVisibleDelegate = InIsPropertyVisible;
}

void SDetailsViewBase::SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly InIsPropertyReadOnly)
{
	IsPropertyReadOnlyDelegate = InIsPropertyReadOnly;
}

void SDetailsViewBase::SetIsCustomRowVisibleDelegate(FIsCustomRowVisible InIsCustomRowVisible)
{
	IsCustomRowVisibleDelegate = InIsCustomRowVisible;
}

void SDetailsViewBase::SetIsCustomRowReadOnlyDelegate(FIsCustomRowReadOnly InIsCustomRowReadOnly)
{
	IsCustomRowReadOnlyDelegate = InIsCustomRowReadOnly;
}

void SDetailsViewBase::SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled IsPropertyEditingEnabled)
{
	IsPropertyEditingEnabledDelegate = IsPropertyEditingEnabled;
}

bool SDetailsViewBase::IsPropertyEditingEnabled() const
{
	// If the delegate is not bound assume property editing is enabled, otherwise ask the delegate
	return !IsPropertyEditingEnabledDelegate.IsBound() || IsPropertyEditingEnabledDelegate.Execute();
}

void SDetailsViewBase::SetKeyframeHandler( TSharedPtr<class IDetailKeyframeHandler> InKeyframeHandler )
{
	// if we don't have a keyframe handler and a valid handler is set, add width to the right column
	// if we do have a keyframe handler and an invalid handler is set, remove width
	float ExtraWidth = 0;
	if (!KeyframeHandler.IsValid() && InKeyframeHandler.IsValid())
	{
		ExtraWidth = 22;
	}
	else if (KeyframeHandler.IsValid() && !InKeyframeHandler.IsValid())
	{
		ExtraWidth = -22;
	}
	
	const float NewWidth = ColumnSizeData.GetRightColumnMinWidth() + ExtraWidth;
	ColumnSizeData.SetRightColumnMinWidth(NewWidth);

	KeyframeHandler = InKeyframeHandler;
	RefreshTree();
}

void SDetailsViewBase::SetExtensionHandler(TSharedPtr<class IDetailPropertyExtensionHandler> InExtensionHandler)
{
	ExtensionHandler = InExtensionHandler;
}

void SDetailsViewBase::SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance OnGetGenericDetails)
{
	GenericLayoutDelegate = OnGetGenericDetails;
}

void SDetailsViewBase::RefreshRootObjectVisibility()
{
	RerunCurrentFilter();
}
/*
TSharedPtr<FAssetThumbnailPool> SDetailsViewBase::GetThumbnailPool() const
{
	return UThumbnailManager::Get().GetSharedThumbnailPool();
}
*/

const TArray<TSharedRef<IClassViewerFilter>>& SDetailsViewBase::GetClassViewerFilters() const
{
	return ClassViewerFilters;
}

void SDetailsViewBase::NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	OnFinishedChangingPropertiesDelegate.Broadcast(PropertyChangedEvent);
}

void SDetailsViewBase::RequestItemExpanded(TSharedRef<FDetailTreeNode> TreeNode, bool bExpand)
{
	FilteredNodesRequestingExpansionState.Add(TreeNode, bExpand);
}

void SDetailsViewBase::RefreshTree()
{
	if (OnDisplayedPropertiesChangedDelegate.IsBound())
	{
		OnDisplayedPropertiesChangedDelegate.Execute();
	}

	DetailTree->RequestTreeRefresh();
}

void SDetailsViewBase::SaveCustomExpansionState(const FString& NodePath, bool bIsExpanded)
{
	if (bIsExpanded)
	{
		ExpandedDetailNodes.Add(NodePath);
	}
	else
	{
		ExpandedDetailNodes.Remove(NodePath);
	}
}

bool SDetailsViewBase::GetCustomSavedExpansionState(const FString& NodePath) const
{
	return ExpandedDetailNodes.Contains(NodePath);
}

bool SDetailsViewBase::IsPropertyVisible( const FPropertyAndParent& PropertyAndParent ) const
{
	return IsPropertyVisibleDelegate.IsBound() ? IsPropertyVisibleDelegate.Execute(PropertyAndParent) : true;
}

bool SDetailsViewBase::IsPropertyReadOnly( const FPropertyAndParent& PropertyAndParent ) const
{
	return IsPropertyReadOnlyDelegate.IsBound() ? IsPropertyReadOnlyDelegate.Execute(PropertyAndParent) : false;
}

bool SDetailsViewBase::IsCustomRowVisible(FName InRowName, FName InParentName) const
{
	return IsCustomRowVisibleDelegate.IsBound() ? IsCustomRowVisibleDelegate.Execute(InRowName, InParentName) : true;
}

bool SDetailsViewBase::IsCustomRowReadOnly(FName InRowName, FName InParentName) const
{
	return IsCustomRowReadOnlyDelegate.IsBound() ? IsCustomRowReadOnlyDelegate.Execute(InRowName, InParentName) : false;
}

TSharedPtr<IPropertyUtilities> SDetailsViewBase::GetPropertyUtilities()
{
	return PropertyUtilities;
}

const FDetailsViewConfig2* SDetailsViewBase::GetConstViewConfig() const 
{
	if (DetailsViewArgs.ViewIdentifier.IsNone())
	{
		return nullptr;
	}

	const UDetailsConfig2* DetailsConfig = GetDefault<UDetailsConfig2>();
	return DetailsConfig->Views.Find(DetailsViewArgs.ViewIdentifier);
}

FDetailsViewConfig2* SDetailsViewBase::GetMutableViewConfig()
{ 
	if (DetailsViewArgs.ViewIdentifier.IsNone())
	{
		return nullptr;
	}

	UDetailsConfig2* DetailsConfig = GetMutableDefault<UDetailsConfig2>();
	return &DetailsConfig->Views.FindOrAdd(DetailsViewArgs.ViewIdentifier);
}

void SDetailsViewBase::SaveViewConfig()
{
	//GetMutableDefault<UDetailsConfig2>()->SaveEditorConfig();
}

void SDetailsViewBase::OnShowOnlyModifiedClicked()
{
	CurrentFilter.bShowOnlyModified = !CurrentFilter.bShowOnlyModified;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowOnlyModified = CurrentFilter.bShowOnlyModified;
		SaveViewConfig();
	}

	UpdateFilteredDetails();
}

void SDetailsViewBase::OnCustomFilterClicked()
{
	if (CustomFilterDelegate.IsBound())
	{
		CustomFilterDelegate.Execute();
		bCustomFilterActive = !bCustomFilterActive;
	}
}

void SDetailsViewBase::OnShowAllAdvancedClicked()
{
	CurrentFilter.bShowAllAdvanced = !CurrentFilter.bShowAllAdvanced;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowAllAdvanced = CurrentFilter.bShowAllAdvanced;
		SaveViewConfig();
	}

	UpdateFilteredDetails();
}

void SDetailsViewBase::OnShowOnlyAllowedClicked()
{
	CurrentFilter.bShowOnlyAllowed = !CurrentFilter.bShowOnlyAllowed;

	UpdateFilteredDetails();
}

void SDetailsViewBase::OnShowAllChildrenIfCategoryMatchesClicked()
{
	CurrentFilter.bShowAllChildrenIfCategoryMatches = !CurrentFilter.bShowAllChildrenIfCategoryMatches;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowAllChildrenIfCategoryMatches = CurrentFilter.bShowAllChildrenIfCategoryMatches;
		SaveViewConfig();
	}

	UpdateFilteredDetails();
}

void SDetailsViewBase::OnShowKeyableClicked()
{
	CurrentFilter.bShowOnlyKeyable = !CurrentFilter.bShowOnlyKeyable;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowOnlyKeyable = CurrentFilter.bShowOnlyKeyable;
		SaveViewConfig();
	}
	UpdateFilteredDetails();
}

void SDetailsViewBase::OnShowAnimatedClicked()
{
	CurrentFilter.bShowOnlyAnimated = !CurrentFilter.bShowOnlyAnimated;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowOnlyAnimated = CurrentFilter.bShowOnlyAnimated;
		SaveViewConfig();
	}

	UpdateFilteredDetails();
}

/** Called when the filter text changes.  This filters specific property nodes out of view */
void SDetailsViewBase::OnFilterTextChanged(const FText& InFilterText)
{
	FilterView(InFilterText.ToString());
}

void SDetailsViewBase::OnFilterTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnCleared)
	{
		SearchBox->SetText(FText::GetEmpty());
		OnFilterTextChanged(FText::GetEmpty());
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
	}
}

TSharedPtr<SWidget> SDetailsViewBase::GetNameAreaWidget()
{
	return DetailsViewArgs.bCustomNameAreaLocation ? NameArea : nullptr;
}

void SDetailsViewBase::SetNameAreaCustomContent( TSharedRef<SWidget>& InCustomContent )
{
	NameArea->SetCustomContent(InCustomContent);	
}

TSharedPtr<SWidget> SDetailsViewBase::GetFilterAreaWidget()
{
	return DetailsViewArgs.bCustomFilterAreaLocation ? FilterRow : nullptr;
}

TSharedPtr<FUICommandList> SDetailsViewBase::GetHostCommandList() const
{
	return DetailsViewArgs.HostCommandList;
}

TSharedPtr<FTabManager> SDetailsViewBase::GetHostTabManager() const
{
	return DetailsViewArgs.HostTabManager;
}

void SDetailsViewBase::SetHostTabManager(TSharedPtr<FTabManager> InTabManager)
{
	DetailsViewArgs.HostTabManager = InTabManager;
}

/** 
 * Hides or shows properties based on the passed in filter text
 * 
 * @param InFilter The filter text
 */
void SDetailsViewBase::FilterView(const FString& InFilter)
{
	bool bHadActiveFilter = CurrentFilter.FilterStrings.Num() > 0;

	TArray<FString> CurrentFilterStrings;

	FString ParseString = InFilter;
	// Remove whitespace from the front and back of the string
	ParseString.TrimStartAndEndInline();
	ParseString.ParseIntoArray(CurrentFilterStrings, TEXT(" "), true);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	bHasActiveFilter = CurrentFilterStrings.Num() > 0;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
	CurrentFilter.FilterStrings = CurrentFilterStrings;

	if (!bHadActiveFilter && CurrentFilter.FilterStrings.Num() > 0)
	{
		SavePreSearchExpandedItems();
	}

	UpdateFilteredDetails();
	
	if (bHadActiveFilter && CurrentFilter.FilterStrings.Num() == 0)
	{
		RestorePreSearchExpandedItems();
	}
}

EVisibility SDetailsViewBase::GetFilterBoxVisibility() const
{
	EVisibility Result = EVisibility::Collapsed;
	// Visible if we allow search and we have anything to search otherwise collapsed so it doesn't take up room	
	if (DetailsViewArgs.bAllowSearch && IsConnected())
	{
		if (RootTreeNodes.Num() > 0 || HasActiveSearch() || !CurrentFilter.IsEmptyFilter())
		{
			Result = EVisibility::Visible;
		}
	}

	return Result;
}

bool SDetailsViewBase::SupportsKeyboardFocus() const
{
	return DetailsViewArgs.bSearchInitialKeyFocus && SearchBox->SupportsKeyboardFocus() && GetFilterBoxVisibility() == EVisibility::Visible;
}

FReply SDetailsViewBase::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	FReply Reply = FReply::Handled();

	if (InFocusEvent.GetCause() != EFocusCause::Cleared)
	{
		Reply.SetUserFocus(SearchBox.ToSharedRef(), InFocusEvent.GetCause());
	}

	return Reply;
}

void SDetailsViewBase::SetPendingCleanupTimer()
{
	/*
	if (!bPendingCleanupTimerSet)
	{
		if (GEditor && GEditor->IsTimerManagerValid())
		{
			bPendingCleanupTimerSet = true;
			GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateSP(this, &SDetailsViewBase::HandlePendingCleanupTimer));
		}
	}
	*/
}

void SDetailsViewBase::HandlePendingCleanupTimer()
{
	bPendingCleanupTimerSet = false;

	HandlePendingCleanup();

	for (auto It = FilteredNodesRequestingExpansionState.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void SDetailsViewBase::HandlePendingCleanup()
{
	for (int32 i = 0; i < CustomizationClassInstancesPendingDelete.Num(); ++i)
	{
		ensure(CustomizationClassInstancesPendingDelete[i].IsUnique());
	}

	// Release any pending kill nodes.
	for ( TSharedPtr<FComplexPropertyNode>& PendingKillNode : RootNodesPendingKill )
	{
		if ( PendingKillNode.IsValid() )
		{
			PendingKillNode->Disconnect();
			PendingKillNode.Reset();
		}
	}
	RootNodesPendingKill.Empty();

	// Empty all the customization instances that need to be deleted
	CustomizationClassInstancesPendingDelete.Empty();

	// Empty all the detail layouts that need to be deleted
	DetailLayoutsPendingDelete.Empty();
}

/** Ticks the property view.  This function performs a data consistency check */
void SDetailsViewBase::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	HandlePendingCleanup();

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig && ViewConfig->ValueColumnWidth != ColumnSizeData.ValueColumnWidth.Get(0))
	{
		ViewConfig->ValueColumnWidth = ColumnSizeData.ValueColumnWidth.Get(0);
		SaveViewConfig();
	}

	FRootPropertyNodeList& RootPropertyNodes = GetRootNodes();

	bool bHadDeferredActions = DeferredActions.Num() > 0;
	auto PreProcessRootNode = [bHadDeferredActions, this](TSharedPtr<FComplexPropertyNode> RootPropertyNode) 
	{
		check(RootPropertyNode.IsValid());

		// Purge any objects that are marked pending kill from the object list
		if (FObjectPropertyNode* ObjectRoot = RootPropertyNode->AsObjectNode())
		{
			ObjectRoot->PurgeKilledObjects();
		}

		if (bHadDeferredActions)
		{		
			// Any deferred actions are likely to cause the node tree to be at least partially rebuilt
			// Save the expansion state of existing nodes so we can expand them later
			SaveExpandedItems(RootPropertyNode.ToSharedRef());
		}
	};

	for (TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		PreProcessRootNode(RootPropertyNode);
	}

	for (FDetailLayoutData& LayoutData : DetailLayouts)
	{
		FRootPropertyNodeList& ExternalRootPropertyNodes = LayoutData.DetailLayout->GetExternalRootPropertyNodes();
		for (TSharedPtr<FComplexPropertyNode>& ExternalRootPropertyNode : ExternalRootPropertyNodes)
		{
			PreProcessRootNode(ExternalRootPropertyNode);
		}
	}

	if (bHadDeferredActions)
	{
		TArray<FSimpleDelegate> DeferredActionsCopy;
		
		do
		{
			// Execute any deferred actions
			DeferredActionsCopy = MoveTemp(DeferredActions);
			DeferredActions.Reset();

			// Execute any deferred actions
			for (const FSimpleDelegate& DeferredAction : DeferredActionsCopy)
			{
				DeferredAction.ExecuteIfBound();
			}
		} while (DeferredActions.Num() > 0);

		RestoreAllExpandedItems();
	}

	TSharedPtr<FComplexPropertyNode> LastRootPendingKill;
	if (RootNodesPendingKill.Num() > 0 )
	{
		LastRootPendingKill = RootNodesPendingKill.Last();
	}

	bool bValidateExternalNodes = true;

	int32 FoundIndex = RootPropertyNodes.Find(LastRootPendingKill);
	bool bUpdateFilteredDetails = false;
	if (FoundIndex != INDEX_NONE)
	{ 
		// Reacquire the root property nodes.  It may have been changed by the deferred actions if something like a blueprint editor forcefully resets a details panel during a posteditchange
		ForceRefresh();

		// All objects are being reset, no need to validate external nodes
		bValidateExternalNodes = false;
	}
	else
	{
		if (CustomValidatePropertyNodesFunction.IsBound())
		{
			bool bIsValid = CustomValidatePropertyNodesFunction.Execute(RootPropertyNodes);
		}
		else // standard validation behavior
		{
			for (TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
			{
				EPropertyDataValidationResult Result = RootPropertyNode->EnsureDataIsValid();
				if (Result == EPropertyDataValidationResult::PropertiesChanged || Result == EPropertyDataValidationResult::EditInlineNewValueChanged)
				{
					UpdatePropertyMaps();
					bUpdateFilteredDetails = true;
				}
				else if (Result == EPropertyDataValidationResult::ArraySizeChanged || Result == EPropertyDataValidationResult::ChildrenRebuilt)
				{
					bUpdateFilteredDetails = true;
				}
				else if (Result == EPropertyDataValidationResult::ObjectInvalid)
				{
					bValidateExternalNodes = false;

					ForceRefresh();
					break;
				}
			}
		}
	}

	if (bValidateExternalNodes)
	{
		for (FDetailLayoutData& LayoutData : DetailLayouts)
		{
			FRootPropertyNodeList& ExternalRootPropertyNodes = LayoutData.DetailLayout->GetExternalRootPropertyNodes();

			for (int32 NodeIndex = 0; NodeIndex < ExternalRootPropertyNodes.Num(); ++NodeIndex)
			{
				TSharedPtr<FPropertyNode> PropertyNode = ExternalRootPropertyNodes[NodeIndex];
				{
					EPropertyDataValidationResult Result = PropertyNode->EnsureDataIsValid();
					if (Result == EPropertyDataValidationResult::PropertiesChanged || Result == EPropertyDataValidationResult::EditInlineNewValueChanged)
					{
						// Note this will invalidate all the external root nodes so there is no need to continue
						ExternalRootPropertyNodes.Empty();

						UpdatePropertyMaps();
						bUpdateFilteredDetails = true;

						break;
					}
					else if (Result == EPropertyDataValidationResult::ArraySizeChanged || Result == EPropertyDataValidationResult::ChildrenRebuilt)
					{
						bUpdateFilteredDetails = true;
					}
				}
			}
		}
	}

	if (bUpdateFilteredDetails)
	{
		UpdateFilteredDetails();

		RestoreAllExpandedItems();
	}

	for(FDetailLayoutData& LayoutData : DetailLayouts)
	{
		if(LayoutData.DetailLayout.IsValid())
		{
			LayoutData.DetailLayout->Tick(InDeltaTime);
		}
	}

	if (!ColorPropertyNode.IsValid() && bHasOpenColorPicker)
	{
		// Destroy the color picker window if the color property node has become invalid
		//DestroyColorPicker();
		bHasOpenColorPicker = false;
	}

	if (FilteredNodesRequestingExpansionState.Num() > 0)
	{
		// change expansion state on the nodes that request it
		for (TMap<TWeakPtr<FDetailTreeNode>, bool>::TConstIterator It(FilteredNodesRequestingExpansionState); It; ++It)
		{
			TSharedPtr<FDetailTreeNode> DetailTreeNode = It.Key().Pin();
			if (DetailTreeNode.IsValid())
			{
				DetailTree->SetItemExpansion(DetailTreeNode.ToSharedRef(), It.Value());
			}
		}

		FilteredNodesRequestingExpansionState.Empty();
	}
}

/**
* Recursively gets expanded items for a node
*
* @param InPropertyNode			The node to get expanded items from
* @param OutExpandedItems	List of expanded items that were found
*/
static void GetExpandedItems(TSharedPtr<FPropertyNode> InPropertyNode, TSet<FString>& OutExpandedItems)
{
	if (InPropertyNode->HasNodeFlags(EPropertyNodeFlags::Expanded))
	{
		const bool bWithArrayIndex = true;
		FString Path;
		Path.Empty(128);
		InPropertyNode->GetQualifiedName(Path, bWithArrayIndex);

		OutExpandedItems.Add(Path);
	}

	for (int32 ChildIndex = 0; ChildIndex < InPropertyNode->GetNumChildNodes(); ++ChildIndex)
	{
		GetExpandedItems(InPropertyNode->GetChildNode(ChildIndex), OutExpandedItems);
	}
}

/**
* Recursively sets expanded items for a node
*
* @param InNode			The node to set expanded items on
* @param OutExpandedItems	List of expanded items to set
*/
static void SetExpandedItems(TSharedPtr<FPropertyNode> InPropertyNode, const TSet<FString>& InExpandedItems, bool bCollapseRest)
{
	const bool bWithArrayIndex = true;
	FString Path;
	Path.Empty(128);
	InPropertyNode->GetQualifiedName(Path, bWithArrayIndex);

	if (InExpandedItems.Contains(Path))
	{
		InPropertyNode->SetNodeFlags(EPropertyNodeFlags::Expanded, true);
	}
	else if (bCollapseRest)
	{
		InPropertyNode->SetNodeFlags(EPropertyNodeFlags::Expanded, false);
	}

	for (int32 NodeIndex = 0; NodeIndex < InPropertyNode->GetNumChildNodes(); ++NodeIndex)
	{
		SetExpandedItems(InPropertyNode->GetChildNode(NodeIndex), InExpandedItems, bCollapseRest);
	}
}

void SDetailsViewBase::SavePreSearchExpandedItems()
{
	PreSearchExpandedItems.Reset();

	FRootPropertyNodeList& RootPropertyNodes = GetRootNodes();

	for (TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		GetExpandedItems(RootPropertyNode.ToSharedRef(), PreSearchExpandedItems);
	}

	for (FDetailLayoutData& LayoutData : DetailLayouts)
	{
		FRootPropertyNodeList& ExternalRootPropertyNodes = LayoutData.DetailLayout->GetExternalRootPropertyNodes();
		for (TSharedPtr<FComplexPropertyNode>& ExternalRootPropertyNode : ExternalRootPropertyNodes)
		{
			GetExpandedItems(ExternalRootPropertyNode.ToSharedRef(), PreSearchExpandedItems);
		}
	}

	PreSearchExpandedCategories.Reset();

	for (const TSharedRef<FDetailTreeNode>& RootNode : RootTreeNodes)
	{
		if (RootNode->GetNodeType() == EDetailNodeType::Category)
		{
			FDetailCategoryImpl& Category = (FDetailCategoryImpl&) RootNode.Get();
			if (Category.ShouldBeExpanded())
			{
				PreSearchExpandedCategories.Add(Category.GetCategoryPathName());
			}
		}
	}
}

void SDetailsViewBase::RestorePreSearchExpandedItems()
{
	FRootPropertyNodeList& RootPropertyNodes = GetRootNodes();

	for (TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		SetExpandedItems(RootPropertyNode, PreSearchExpandedItems, true);
	}

	for (FDetailLayoutData& LayoutData : DetailLayouts)
	{
		FRootPropertyNodeList& ExternalRootPropertyNodes = LayoutData.DetailLayout->GetExternalRootPropertyNodes();
		for (TSharedPtr<FComplexPropertyNode>& ExternalRootPropertyNode : ExternalRootPropertyNodes)
		{
			SetExpandedItems(ExternalRootPropertyNode, PreSearchExpandedItems, true);
		}
	}

	PreSearchExpandedItems.Reset();

	for (const TSharedRef<FDetailTreeNode>& RootNode : RootTreeNodes)
	{
		if (RootNode->GetNodeType() == EDetailNodeType::Category)
		{
			FDetailCategoryImpl& Category = (FDetailCategoryImpl&) RootNode.Get();
			
			bool bShouldBeExpanded = PreSearchExpandedCategories.Contains(Category.GetCategoryPathName());
			RequestItemExpanded(RootNode, bShouldBeExpanded);
		}
	}

	PreSearchExpandedCategories.Reset();
}

void SDetailsViewBase::SaveExpandedItems(TSharedRef<FPropertyNode> StartNode)
{
	UStruct* BestBaseStruct = StartNode->FindComplexParent()->GetBaseStructure();

	TSet<FString> ExpandedPropertyItemSet;
	GetExpandedItems(StartNode, ExpandedPropertyItemSet);

	TArray<FString> ExpandedPropertyItems = ExpandedPropertyItemSet.Array();

	// Handle spaces in expanded node names by wrapping them in quotes
	for (FString& String : ExpandedPropertyItems)
	{
		String.InsertAt(0, '"');
		String.AppendChar('"');
	}

	TArray<FString> ExpandedCustomItems = ExpandedDetailNodes.Array();

	// Expanded custom items may have spaces but SetSingleLineArray doesn't support spaces (treats it as another element in the array)
	// Append a '|' after each element instead
	FString ExpandedCustomItemsString;
	for (const FString& DetailNode : ExpandedDetailNodes)
	{
		ExpandedCustomItemsString += DetailNode;
		ExpandedCustomItemsString += TEXT(",");
	}

	//while a valid class, and we're either the same as the base class (for multiple actors being selected and base class is AActor) OR we're not down to AActor yet)
	for (UStruct* Struct = BestBaseStruct; Struct && ((BestBaseStruct == Struct) || (Struct != AActor::StaticClass())); Struct = Struct->GetSuperStruct())
	{
		if (StartNode->GetNumChildNodes() > 0)
		{
			bool bShouldSave = ExpandedPropertyItems.Num() > 0;
			if (!bShouldSave)
			{
				TArray<FString> DummyExpandedPropertyItems;
				GConfig->GetSingleLineArray(TEXT("DetailPropertyExpansion"), *Struct->GetName(), DummyExpandedPropertyItems, GEditorPerProjectIni);
				bShouldSave = DummyExpandedPropertyItems.Num() > 0;
			}

			if (bShouldSave)
			{
				GConfig->SetSingleLineArray(TEXT("DetailPropertyExpansion"), *Struct->GetName(), ExpandedPropertyItems, GEditorPerProjectIni);
			}
		}
	}

	if (DetailLayouts.Num() > 0 && BestBaseStruct)
	{
		bool bShouldSave = !ExpandedCustomItemsString.IsEmpty();
		if (!bShouldSave)
		{
			FString DummyExpandedCustomItemsString;
			GConfig->GetString(TEXT("DetailCustomWidgetExpansion"), *BestBaseStruct->GetName(), DummyExpandedCustomItemsString, GEditorPerProjectIni);
			bShouldSave = !DummyExpandedCustomItemsString.IsEmpty();
		}
		if (bShouldSave)
		{
			GConfig->SetString(TEXT("DetailCustomWidgetExpansion"), *BestBaseStruct->GetName(), *ExpandedCustomItemsString, GEditorPerProjectIni);
		}
	}
}

void SDetailsViewBase::RestoreAllExpandedItems()
{
	for (TSharedPtr<FComplexPropertyNode>& RootPropertyNode : GetRootNodes())
	{
		check(RootPropertyNode.IsValid());
		RestoreExpandedItems(RootPropertyNode.ToSharedRef());
	}

	for (FDetailLayoutData& LayoutData : DetailLayouts)
	{
		FRootPropertyNodeList& ExternalRootPropertyNodes = LayoutData.DetailLayout->GetExternalRootPropertyNodes();
		for (TSharedPtr<FComplexPropertyNode>& ExternalRootPropertyNode : ExternalRootPropertyNodes)
		{
			check(ExternalRootPropertyNode.IsValid());
			RestoreExpandedItems(ExternalRootPropertyNode.ToSharedRef());
		}
	}
}

void SDetailsViewBase::RestoreExpandedItems(TSharedRef<FPropertyNode> StartNode)
{
	FString ExpandedCustomItems;

	UStruct* BestBaseStruct = StartNode->FindComplexParent()->GetBaseStructure();

	//while a valid class, and we're either the same as the base class (for multiple actors being selected and base class is AActor) OR we're not down to AActor yet)
	TArray<FString> DetailPropertyExpansionStrings;
	for (UStruct* Struct = BestBaseStruct; Struct && ((BestBaseStruct == Struct) || (Struct != AActor::StaticClass())); Struct = Struct->GetSuperStruct())
	{
		GConfig->GetSingleLineArray(TEXT("DetailPropertyExpansion"), *Struct->GetName(), DetailPropertyExpansionStrings, GEditorPerProjectIni);
	}

	TSet<FString> ExpandedPropertyItems;
	ExpandedPropertyItems.Append(DetailPropertyExpansionStrings);
	SetExpandedItems(StartNode, ExpandedPropertyItems, false);

	if (BestBaseStruct)
	{
		GConfig->GetString(TEXT("DetailCustomWidgetExpansion"), *BestBaseStruct->GetName(), ExpandedCustomItems, GEditorPerProjectIni);
		TArray<FString> ExpandedCustomItemsArray;
		ExpandedCustomItems.ParseIntoArray(ExpandedCustomItemsArray, TEXT(","), true);

		ExpandedDetailNodes.Append(ExpandedCustomItemsArray);
	}
}

void SDetailsViewBase::MarkNodeAnimating(TSharedPtr<FPropertyNode> InNode, float InAnimationDuration)
{
	if (InNode.IsValid() && InAnimationDuration > 0.0f)
	{
		CurrentlyAnimatingNodePath = FPropertyNode::CreatePropertyPath(InNode.ToSharedRef());
		//GEditor->GetTimerManager()->ClearTimer(AnimateNodeTimer);
		//GEditor->GetTimerManager()->SetTimer(AnimateNodeTimer, FTimerDelegate::CreateSP(this, &SDetailsViewBase::HandleNodeAnimationComplete), InAnimationDuration, false);
	}
}

bool SDetailsViewBase::IsNodeAnimating(TSharedPtr<FPropertyNode> InNode)
{
	if (CurrentlyAnimatingNodePath.IsValid() && InNode.IsValid())
	{
		TSharedRef<FPropertyPath> InNodePath = FPropertyNode::CreatePropertyPath(InNode.ToSharedRef());
		return FPropertyPath::AreEqual(CurrentlyAnimatingNodePath.ToSharedRef(), InNodePath);
	}

	return false;
}

void SDetailsViewBase::HandleNodeAnimationComplete()
{
	CurrentlyAnimatingNodePath = nullptr;
}

void SDetailsViewBase::UpdateFilteredDetails()
{
	RootTreeNodes.Reset();

	FDetailNodeList InitialRootNodeList;
	
	NumVisibleTopLevelObjectNodes = 0;
	
	FRootPropertyNodeList& RootPropertyNodes = GetRootNodes();
	for(int32 RootNodeIndex = 0; RootNodeIndex < RootPropertyNodes.Num(); ++RootNodeIndex)
	{
		const TSharedPtr<FComplexPropertyNode>& RootPropertyNode = RootPropertyNodes[RootNodeIndex];
		if(RootPropertyNode.IsValid())
		{
			RootPropertyNode->FilterNodes(CurrentFilter.FilterStrings);
			RootPropertyNode->ProcessSeenFlags(true);

			RestoreExpandedItems(RootPropertyNode.ToSharedRef());

			const TSharedPtr<FDetailLayoutBuilderImpl>& DetailLayout = DetailLayouts[RootNodeIndex].DetailLayout;
			if(DetailLayout.IsValid())
			{
				FRootPropertyNodeList& ExternalRootPropertyNodes = DetailLayout->GetExternalRootPropertyNodes();
				for (const TSharedPtr<FComplexPropertyNode>& ExternalRootNode : ExternalRootPropertyNodes)
				{
					if (ExternalRootNode.IsValid())
					{
						ExternalRootNode->FilterNodes(CurrentFilter.FilterStrings);
						ExternalRootNode->ProcessSeenFlags(true);

						RestoreExpandedItems(ExternalRootNode.ToSharedRef());
					}
				}

				DetailLayout->FilterDetailLayout(CurrentFilter);

				FDetailNodeList& LayoutRoots = DetailLayout->GetFilteredRootTreeNodes();
				if (LayoutRoots.Num() > 0)
				{
					// A top level object nodes has a non-filtered away root so add one to the total number we have
					++NumVisibleTopLevelObjectNodes;

					InitialRootNodeList.Append(LayoutRoots);
				}
			}
		}
	}

	// for multiple top level object we need to do a secondary pass on top level object nodes after we have determined if there is any nodes visible at all.  If there are then we ask the details panel if it wants to show childen
	for(TSharedRef<class FDetailTreeNode> RootNode : InitialRootNodeList)
	{
		if(RootNode->ShouldShowOnlyChildren())
		{
			RootNode->GetChildren(RootTreeNodes);
		}
		else
		{
			RootTreeNodes.Add(RootNode);
		}
	}

	RefreshTree();
}

void SDetailsViewBase::RegisterInstancedCustomPropertyLayout(UStruct* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
	check( Class );

	FDetailLayoutCallback Callback;
	Callback.DetailLayoutDelegate = DetailLayoutDelegate;
	// @todo: DetailsView: Fix me: this specifies the order in which detail layouts should be queried
	Callback.Order = InstancedClassToDetailLayoutMap.Num();

	InstancedClassToDetailLayoutMap.Add( Class, Callback );	
}

void SDetailsViewBase::RegisterInstancedCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier /*= nullptr*/)
{
	FPropertyTypeLayoutCallback Callback;
	Callback.PropertyTypeLayoutDelegate = PropertyTypeLayoutDelegate;
	Callback.PropertyTypeIdentifier = Identifier;

	FPropertyTypeLayoutCallbackList* LayoutCallbacks = InstancedTypeToLayoutMap.Find(PropertyTypeName);
	if (LayoutCallbacks)
	{
		LayoutCallbacks->Add(Callback);
	}
	else
	{
		FPropertyTypeLayoutCallbackList NewLayoutCallbacks;
		NewLayoutCallbacks.Add(Callback);
		InstancedTypeToLayoutMap.Add(PropertyTypeName, NewLayoutCallbacks);
	}
}

void SDetailsViewBase::UnregisterInstancedCustomPropertyLayout(UStruct* Class)
{
	check( Class );

	InstancedClassToDetailLayoutMap.Remove( Class );	
}

void SDetailsViewBase::UnregisterInstancedCustomPropertyTypeLayout(FName PropertyTypeName, TSharedPtr<IPropertyTypeIdentifier> Identifier /*= nullptr*/)
{
	FPropertyTypeLayoutCallbackList* LayoutCallbacks = InstancedTypeToLayoutMap.Find(PropertyTypeName);

	if (LayoutCallbacks)
	{
		LayoutCallbacks->Remove(Identifier);
	}
}


} // namespace soda