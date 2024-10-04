// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SDetailsView.h"

#include "RuntimePropertyEditor/CategoryPropertyNode.h"
#include "Classes/SodaStyleSettings.h"
#include "RuntimePropertyEditor/DetailCategoryBuilderImpl.h"
#include "RuntimePropertyEditor/DetailLayoutBuilderImpl.h"
#include "RuntimePropertyEditor/DetailsViewGenericObjectFilter.h"
#include "RuntimePropertyEditor/DetailsViewPropertyGenerationUtilities.h"
//#include "Editor.h"
//#include "EditorMetadataOverrides.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
#include "RuntimePropertyEditor/PropertyEditorHelpers.h"
#include "RuntimePropertyEditor/SDetailNameArea.h"
#include "Styling/StyleColors.h"
#include "RuntimePropertyEditor/UserInterface/PropertyDetails/PropertyDetailsUtilities.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Classes/SodaStyleSettings.h"

#include "RuntimeMetaData.h"

#define LOCTEXT_NAMESPACE "SDetailsView"

namespace soda
{

SDetailsView::~SDetailsView()
{
	const FRootPropertyNodeList& RootNodes = GetRootNodes();
	for(const TSharedPtr<FComplexPropertyNode>& RootNode : RootNodes)
	{
		SaveExpandedItems(RootNode.ToSharedRef());
	}

	//FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoDelegateHandle);
};

/**
 * Constructs the widget
 *
 * @param InArgs   Declaration from which to construct this widget.              
 */
void SDetailsView::Construct(const FArguments& InArgs, const FDetailsViewArgs& InDetailsViewArgs)
{
	DetailsViewArgs = InDetailsViewArgs;

	const FDetailsViewConfig2* ViewConfig = GetConstViewConfig();
	if (ViewConfig != nullptr)
	{
		if (ViewConfig->ValueColumnWidth != 0 && ViewConfig->ValueColumnWidth != DetailsViewArgs.ColumnWidth)
		{
			DetailsViewArgs.ColumnWidth = ViewConfig->ValueColumnWidth;
		}

		if (DetailsViewArgs.bShowSectionSelector)
		{
			DetailsViewArgs.bShowSectionSelector = ViewConfig->bShowSections;
		}

		CurrentFilter.bShowFavoritesCategory = ViewConfig->bShowFavoritesCategory;
	}

	ColumnSizeData.SetValueColumnWidth(DetailsViewArgs.ColumnWidth);
	ColumnSizeData.SetRightColumnMinWidth(DetailsViewArgs.RightColumnMinWidth);

	SetObjectFilter(InDetailsViewArgs.ObjectFilter);
	SetClassViewerFilters(InDetailsViewArgs.ClassViewerFilters);

	bViewingClassDefaultObject = false;

	PropertyUtilities = MakeShareable( new FPropertyDetailsUtilities( *this ) );
	PropertyGenerationUtilities = MakeShareable( new FDetailsViewPropertyGenerationUtilities(*this) );

	//PostUndoRedoDelegateHandle = FEditorDelegates::PostUndoRedo.AddSP(this, &SDetailsView::OnPostUndoRedo);

	// We want the scrollbar to always be visible when objects are selected, but not when there is no selection - however:
	//  - We can't use AlwaysShowScrollbar for this, as this will also show the scrollbar when nothing is selected
	//  - We can't use the Visibility construction parameter, as it gets translated into user visibility and can hide the scrollbar even when objects are selected
	// We instead have to explicitly set the visibility after the scrollbar has been constructed to get the exact behavior we want
	TSharedRef<SScrollBar> ExternalScrollbar = SNew(SScrollBar);
	ExternalScrollbar->SetVisibility( TAttribute<EVisibility>( this, &SDetailsView::GetScrollBarVisibility ) );

	FMenuBuilder DetailViewOptions( true, nullptr);

	if (DetailsViewArgs.bShowModifiedPropertiesOption)
	{
		DetailViewOptions.AddMenuEntry( 
			LOCTEXT("ShowOnlyModified", "Show Only Modified Properties"),
			LOCTEXT("ShowOnlyModified_ToolTip", "Displays only properties which have been changed from their default"),
			FSlateIcon(),
			FUIAction( 
				FExecuteAction::CreateSP( this, &SDetailsView::OnShowOnlyModifiedClicked ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SDetailsView::IsShowOnlyModifiedChecked )
			),
			NAME_None,
			EUserInterfaceActionType::Check 
		);
	}

	if (DetailsViewArgs.bShowCustomFilterOption)
	{
		TAttribute<FText> CustomFilterLabelDelegate;
		CustomFilterLabelDelegate.BindRaw(this, &SDetailsView::GetCustomFilterLabel);
		DetailViewOptions.AddMenuEntry(
			CustomFilterLabelDelegate,
			FText(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDetailsView::OnCustomFilterClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDetailsView::IsCustomFilterChecked)
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}

	if (DetailsViewArgs.bShowDifferingPropertiesOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowOnlyDiffering", "Show Only Differing Properties"),
			LOCTEXT("ShowOnlyDiffering_ToolTip", "Displays only properties in this instance which have been changed or added from the instance being compared"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDetailsView::OnShowOnlyAllowedClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDetailsView::IsShowOnlyAllowedChecked)
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}

	if (DetailsViewArgs.bShowKeyablePropertiesOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowOnlyKeyable", "Show Only Keyable Properties"),
			LOCTEXT("ShowOnlyKeyable_ToolTip", "Displays only properties which are keyable"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDetailsView::OnShowKeyableClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDetailsView::IsShowKeyableChecked)
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}

	if (DetailsViewArgs.bShowAnimatedPropertiesOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowAnimated", "Show Only Animated Properties"),
			LOCTEXT("ShowAnimated_ToolTip", "Displays only properties which are animated (have tracks)"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDetailsView::OnShowAnimatedClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDetailsView::IsShowAnimatedChecked)
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ShowAllAdvanced", "Show All Advanced Details"),
		LOCTEXT("ShowAllAdvanced_ToolTip", "Shows all advanced detail sections in each category"),
		FSlateIcon(),
		FUIAction( 
		FExecuteAction::CreateSP( this, &SDetailsView::OnShowAllAdvancedClicked ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SDetailsView::IsShowAllAdvancedChecked )
			),
		NAME_None,
		EUserInterfaceActionType::Check
	);

	if (DetailsViewArgs.bShowHiddenPropertiesWhilePlayingOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowHiddenPropertiesWhilePlaying", "Show Hidden Properties while Playing"),
			LOCTEXT("ShowHiddenPropertiesWhilePlaying_ToolTip", "When Playing or Simulating, shows all properties (even non-visible and non-editable properties), if the object belongs to a simulating world.  This is useful for debugging."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDetailsView::OnShowHiddenPropertiesWhilePlayingClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDetailsView::IsShowHiddenPropertiesWhilePlayingChecked)
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ShowAllChildrenIfCategoryMatches", "Show Child On Category Match"),
		LOCTEXT("ShowAllChildrenIfCategoryMatches_ToolTip", "Shows children if their category matches the search criteria"),
		FSlateIcon(),
		FUIAction( 
			FExecuteAction::CreateSP( this, &SDetailsView::OnShowAllChildrenIfCategoryMatchesClicked ),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP( this, &SDetailsView::IsShowAllChildrenIfCategoryMatchesChecked )
		),
		NAME_None,
		EUserInterfaceActionType::Check
	);

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ShowSections", "Show Sections"),
		LOCTEXT("ShowSections_ToolTip", "Shows the sections list."),
		FSlateIcon(),
		FUIAction( 
			FExecuteAction::CreateSP(this, &SDetailsView::OnShowSectionsClicked),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SDetailsView::IsShowSectionsChecked)
		),
		NAME_None,
		EUserInterfaceActionType::Check
	);		

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("CollapseAll", "Collapse All Categories"),
		LOCTEXT("CollapseAll_ToolTip", "Collapses all root level categories"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SDetailsView::SetRootExpansionStates, /*bExpanded=*/false, /*bRecurse=*/false )));

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ExpandAll", "Expand All Categories"),
		LOCTEXT("ExpandAll_ToolTip", "Expands all root level categories"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SDetailsView::SetRootExpansionStates, /*bExpanded=*/true, /*bRecurse=*/false )));

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
		
	// Create the name area which does not change when selection changes
	SAssignNew(NameArea, SDetailNameArea, &SelectedObjects)
		// the name area is only for actors
		.Visibility(this, &SDetailsView::GetActorNameAreaVisibility)
		.OnLockButtonClicked(this, &SDetailsView::OnLockButtonClicked)
		.IsLocked(this, &SDetailsView::IsLocked)
		.ShowLockButton(DetailsViewArgs.bLockable)
		.ShowObjectLabel(DetailsViewArgs.bShowObjectLabel)
		// only show the selection tip if we're not selecting objects
		.SelectionTip(!DetailsViewArgs.bHideSelectionTip);

	if( !DetailsViewArgs.bCustomNameAreaLocation )
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			NameArea.ToSharedRef()
		];
	}

	VerticalBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 0.0f)
	[
		SAssignNew(CustomWidgetsBox, SWrapBox)
		.UseAllottedSize(true)
	];

	TSharedRef<SHorizontalBox> FilterRowHBox = 
		SNew( SHorizontalBox )
		+SHorizontalBox::Slot()
		.Padding(6.f)
		.FillWidth(1)
		[
			// Create the search box
			SAssignNew(SearchBox, SSearchBox)
			.HintText(LOCTEXT("SearchDetailsHint", "Search"))
			.OnTextChanged(this, &SDetailsView::OnFilterTextChanged)
			.OnTextCommitted(this, &SDetailsView::OnFilterTextCommitted)
			.AddMetaData<FTagMetaData>(TEXT("Details.Search"))
		];

	if (DetailsViewArgs.bShowPropertyMatrixButton)
	{
		FilterRowHBox->AddSlot()
			.Padding(0)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				// Create the property matrix button
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "SimpleButton" )
				.OnClicked(this, &SDetailsView::OnOpenRawPropertyEditorClicked)
				.IsEnabled(this, &SDetailsView::CanOpenRawPropertyEditor)
				.ToolTipText(LOCTEXT("RawPropertyEditorButtonLabel", "Open Selection in Property Matrix"))
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FSodaStyle::Get().GetBrush("DetailsView.EditRawProperties"))
				]
			];
	} 

	if (DetailsViewArgs.bAllowFavoriteSystem)
	{
		FilterRowHBox->AddSlot()
			.Padding(0)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				// Create the toggle favorites button
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "SimpleButton" )
				.OnClicked(this, &SDetailsView::OnToggleFavoritesClicked)
				.ToolTipText(LOCTEXT("ToggleFavorites", "Toggle Favorites"))
				[
					SNew(SImage)
					.ColorAndOpacity(this, &SDetailsView::GetToggleFavoritesColor)
					.Image(FSodaStyle::Get().GetBrush("Icons.Star"))
				]
			];
	}

	if (DetailsViewArgs.bShowOptions)
	{
		FilterRowHBox->AddSlot()
			.Padding(0)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew( SComboButton )
				.HasDownArrow(false)
				.ContentPadding(0)
				.ForegroundColor( FSlateColor::UseForeground() )
				.ButtonStyle( FAppStyle::Get(), "SimpleButton" )
				.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ViewOptions")))
				.MenuContent()
				[
					DetailViewOptions.MakeWidget()
				]
				.ButtonContent()
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FSodaStyle::Get().GetBrush("DetailsView.ViewOptions"))
				]
			];
	}

	TSharedRef<SVerticalBox> FilterRowVBox = SNew(SVerticalBox)
		.Visibility( this, &SDetailsView::GetFilterBoxVisibility )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			FilterRowHBox
		];

	FilterRowVBox->AddSlot()
		.Padding(8, 2, 8, 7)
		.AutoHeight()
		[
			SAssignNew(SectionSelectorBox, SWrapBox)
			.UseAllottedSize(true)
			.InnerSlotPadding(FVector2D(4,4))
		];

	RebuildSectionSelector();

	FilterRow = FilterRowVBox;

	if( !DetailsViewArgs.bCustomFilterAreaLocation )
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			[
				FilterRow.ToSharedRef()
			]
		];
	}

	VerticalBox->AddSlot()
	.FillHeight(1)
	.Padding(0)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			ConstructTreeView(ExternalScrollbar)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		[
			SNew(SBox)
			.WidthOverride(16.0f)
			[
				ExternalScrollbar
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.Padding(2.0f, 24.0f, 2.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AllItemsFiltered", "All results have been filtered. Try changing your active filters above."))
			.AutoWrapText(true)
			.Visibility_Lambda([this]() { return ((this->GetFilterBoxVisibility() == EVisibility::Visible) && !this->CurrentFilter.IsEmptyFilter() && RootTreeNodes.Num() == 0) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		]
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FSodaStyle::GetBrush("Searching.SearchActiveBorder"))
			.Visibility_Lambda([this]() { return (this->GetFilterBoxVisibility() == EVisibility::Visible) && this->HasActiveSearch() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		]
	];

	ChildSlot
	[
		VerticalBox
	];
}

TSharedRef<SDetailTree> SDetailsView::ConstructTreeView( TSharedRef<SScrollBar>& ScrollBar )
{
	check( !DetailTree.IsValid() || DetailTree.IsUnique() );

	return
		SAssignNew(DetailTree, SDetailTree)
		.Visibility(this, &SDetailsView::GetTreeVisibility)
		.TreeItemsSource(&RootTreeNodes)
		.OnGetChildren(this, &SDetailsView::OnGetChildrenForDetailTree)
		.OnSetExpansionRecursive(this, &SDetailsView::SetNodeExpansionStateRecursive)
		.OnGenerateRow(this, &SDetailsView::OnGenerateRowForDetailTree)
		.OnExpansionChanged(this, &SDetailsView::OnItemExpansionChanged)
		.SelectionMode(ESelectionMode::None)
		.HandleDirectionalNavigation(false)
		.AllowOverscroll(DetailsViewArgs.bShowScrollBar ? EAllowOverscroll::Yes : EAllowOverscroll::No)
		.ExternalScrollbar(ScrollBar);
}

bool SDetailsView::CanOpenRawPropertyEditor() const
{
	return SelectedObjects.Num() > 0 && IsPropertyEditingEnabled();
}

FReply SDetailsView::OnOpenRawPropertyEditorClicked()
{
	//FRuntimeEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>( "RuntimeEditor" );
	//PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), SelectedObjects );

	return FReply::Handled();
}

EVisibility SDetailsView::GetActorNameAreaVisibility() const
{
	const bool bVisible = DetailsViewArgs.NameAreaSettings != FDetailsViewArgs::HideNameArea && !bViewingClassDefaultObject;
	return bVisible ? EVisibility::Visible : EVisibility::Collapsed; 
}

void SDetailsView::ForceRefresh()
{
	TArray<UObject*> NewObjectList;
	NewObjectList.Reserve(UnfilteredSelectedObjects.Num());
	TArray<TWeakObjectPtr<UObject>> ValidSelectedObjects;
	ValidSelectedObjects.Reserve(UnfilteredSelectedObjects.Num());

	for (const TWeakObjectPtr<UObject>& Object : UnfilteredSelectedObjects)
	{
		if (Object.IsValid())
		{
			ValidSelectedObjects.Add(Object);
			NewObjectList.Add(Object.Get());
		}
	}

	UnfilteredSelectedObjects = MoveTemp(ValidSelectedObjects);

	SetObjectArrayPrivate(NewObjectList);
}

void SDetailsView::MoveScrollOffset(int32 DeltaOffset)
{
	DetailTree->AddScrollOffset((float)DeltaOffset);
}

void SDetailsView::SetObjects(const TArray<UObject*>& InObjects, bool bForceRefresh/* = false*/, bool bOverrideLock/* = false*/)
{
	if (!IsLocked() || bOverrideLock)
	{
		if( bForceRefresh || ShouldSetNewObjects(InObjects) )
		{
			// Keep source object list around to reapply the object filter when it changes or force refresh.
			UnfilteredSelectedObjects.Empty(InObjects.Num());
			for (UObject* InObject : InObjects)
			{
				if (InObject)
				{
					UnfilteredSelectedObjects.Add(InObject);
				}
			}

			SetObjectArrayPrivate(InObjects);
		}
	}
}

void SDetailsView::SetObjects(const TArray<TWeakObjectPtr<UObject>>& InObjects, bool bForceRefresh/* = false*/, bool bOverrideLock/* = false*/)
{
	TArray<UObject*> SourceObjects;
	SourceObjects.Reserve(InObjects.Num());
	for (const TWeakObjectPtr<UObject>& Object : InObjects)
	{
		if (Object.IsValid())
		{
			SourceObjects.Add(Object.Get());
		}
	}

	SetObjects(SourceObjects, bForceRefresh, bOverrideLock);
}

void SDetailsView::SetObject(UObject* InObject, bool bForceRefresh)
{
	TArray<UObject*> SourceObjects;
	SourceObjects.Reserve(1);

	SourceObjects.Add(InObject);

	SetObjects(SourceObjects, bForceRefresh);
}

void SDetailsView::RemoveInvalidObjects()
{
	ForceRefresh();
}

void SDetailsView::SetObjectPackageOverrides(const TMap<TWeakObjectPtr<UObject>, TWeakObjectPtr<UPackage>>& InMapping)
{
	for(TSharedPtr<FComplexPropertyNode>& ComplexRootNode : RootPropertyNodes)
	{
		FObjectPropertyNode* RootNode = ComplexRootNode->AsObjectNode();
		if(RootNode)
		{
			RootNode->SetObjectPackageOverrides(InMapping);
		}
	}
}

void SDetailsView::SetRootObjectCustomizationInstance(TSharedPtr<IDetailRootObjectCustomization> InRootObjectCustomization)
{
	RootObjectCustomization = InRootObjectCustomization;
	RerunCurrentFilter();
}

void SDetailsView::ClearSearch()
{
	CurrentFilter.FilterStrings.Empty();
	SearchBox->SetText(FText::GetEmpty());
	RerunCurrentFilter();
}

void SDetailsView::SetObjectFilter(TSharedPtr<FDetailsViewObjectFilter> InFilter)
{
	ObjectFilter = InFilter;

	if (!ObjectFilter.IsValid())
	{
		ObjectFilter = MakeShared<FDetailsViewDefaultObjectFilter>(!!DetailsViewArgs.bAllowMultipleTopLevelObjects);
	}
}

void SDetailsView::SetClassViewerFilters(const TArray<TSharedRef<IClassViewerFilter>>& InFilters)
{
	ClassViewerFilters = InFilters;
}

bool SDetailsView::ShouldSetNewObjects(const TArray<UObject*>& InObjects) const
{
	bool bShouldSetObjects = false;

	const bool bHadBSPBrushSelected = false;//SelectedActorInfo.bHaveBSPBrush;
	if( bHadBSPBrushSelected == true )
	{
		// If a BSP brush was selected we need to refresh because surface could have been selected and the object set not updated
		bShouldSetObjects = true;
	}
	else if( InObjects.Num() != GetNumObjects() )
	{
		// If the object arrays differ in size then at least one object is different so we must reset
		bShouldSetObjects = true;
	}
	else if(InObjects.Num() == 0)
	{
		// User is likely resetting details panel
		bShouldSetObjects = true;
	}
	else
	{
		// Check to see if the objects passed in are different. If not we do not need to set anything
		TSet<UObject*> NewObjects;
		NewObjects.Append(InObjects);

		if(RootPropertyNodes.Num() > 1)
		{
			// For multiple top level node support, if the single object in each node is not found in the new object set
			// then we need to refresh
			for(int32 RootNodeIndex = 0; RootNodeIndex < RootPropertyNodes.Num(); ++RootNodeIndex)
			{
				FObjectPropertyNode* RootPropertyNode = RootPropertyNodes[RootNodeIndex]->AsObjectNode();
				
				if(RootPropertyNode && RootPropertyNode->GetNumObjects() > 0)
				{
					if(!NewObjects.Contains(RootPropertyNode->GetUObject(0)))
					{
						bShouldSetObjects = true;
						break;
					}
				}
				else
				{
					bShouldSetObjects = true;
					break;
				}
			}
		}
		else
		{
			FObjectPropertyNode* RootPropertyNode = RootPropertyNodes[0]->AsObjectNode();
			if( RootPropertyNode )
			{
				for(TPropObjectIterator Itor(RootPropertyNode->ObjectIterator()); Itor; ++Itor)
				{
					TWeakObjectPtr<UObject> Object = *Itor;
					if(Object.IsValid() && !NewObjects.Contains(Object.Get()))
					{
						// An existing object is not in the list of new objects to set
						bShouldSetObjects = true;
						break;
					}
					else if(!Object.IsValid())
					{
						// An existing object is invalid
						bShouldSetObjects = true;
						break;
					}
				}
			}
			else
			{
				bShouldSetObjects = true;
			}
		}
	}
	
	if (!bShouldSetObjects /* && AssetSelectionUtils::IsAnySurfaceSelected(nullptr)*/)
	{
		bShouldSetObjects = true;
	}

	return bShouldSetObjects;
}

int32 SDetailsView::GetNumObjects() const
{
	if(RootPropertyNodes.Num() > 1)
	{
		return RootPropertyNodes.Num();
	}
	else if( RootPropertyNodes.Num() > 0 && RootPropertyNodes[0]->AsObjectNode())
	{
		return RootPropertyNodes[0]->AsObjectNode()->GetNumObjects();
	}

	return 0;
}

void SDetailsView::SetObjectArrayPrivate(const TArray<UObject*>& InObjects)
{
	double StartTime = FPlatformTime::Seconds();

	const TArray<FDetailsViewObjectRoot> Roots = ObjectFilter->FilterObjects(InObjects);

	PreSetObject(Roots.Num());

	// Selected actors for building SelectedActorInfo
	TArray<AActor*> SelectedRawActors;

	bViewingClassDefaultObject = InObjects.Num() > 0 ? true : false;
	bool bOwnedByLockedLevel = false;

	check(RootPropertyNodes.Num() == Roots.Num());

	for(int32 RootIndex = 0; RootIndex < Roots.Num(); ++RootIndex)
	{
		const FDetailsViewObjectRoot& Root = Roots[RootIndex];

		FObjectPropertyNode* RootNode = RootPropertyNodes[RootIndex]->AsObjectNode();

		for(const TWeakObjectPtr<UObject> Object : Root.Objects)
		{
			if (Object.IsValid())
			{
				bViewingClassDefaultObject &= Object->HasAnyFlags(RF_ClassDefaultObject);

				RootNode->AddObject(Object.Get());

				SelectedObjects.Add(Object);
				if (AActor* Actor = Cast<AActor>(Object.Get()))
				{
					SelectedActors.Add(Actor);
					SelectedRawActors.Add(Actor);
				}

				// TODO: Try to invoke AddGeneratedMetadataToGlobalScope() once when object loaded
				if (Object->GetClass()->ImplementsInterface(UEditableObject::StaticClass()))
				{
					FRuntimeMetaData::AddGeneratedMetadataToGlobalScope(Object->GetClass());
				}
			}
		}
	}

	if(SelectedObjects.Num() == 0)
	{
		// Unlock the view automatically if we are viewing nothing
		bIsLocked = false;
	}

	// Selection changed, refresh the detail area
	NameArea->Refresh(SelectedObjects, DetailsViewArgs.NameAreaSettings);
	
	// When selection changes rebuild information about the selection
	//SelectedActorInfo = AssetSelectionUtils::BuildSelectedActorInfo(SelectedRawActors);

	PostSetObject(Roots);

	// Set the title of the window based on the objects we are viewing
	// Or call the delegate for handling when the title changed
	FString Title;

	if(SelectedObjects.Num() == 0 )
	{
		Title = NSLOCTEXT("PropertyView", "NothingSelectedTitle", "Nothing selected").ToString();
	}
	else if(Roots.Num() == 1 && RootPropertyNodes[0]->AsObjectNode()->GetNumObjects() == 1)
	{
		// if the object is the default metaobject for a UClass, use the UClass's name instead
		UObject* Object = RootPropertyNodes[0]->AsObjectNode()->GetUObject(0);

		FString ObjectName;
		if ( Object && Object->GetClass()->GetDefaultObject() == Object )
		{
			ObjectName = Object->GetClass()->GetName();
		}
		else if( Object )
		{
			ObjectName = Object->GetName();
			/*
			// Is this an actor?  If so, it might have a friendly name to display
			const AActor* Actor = Cast<const  AActor >( Object );
			if( Actor != nullptr)
			{
				// Use the friendly label for this actor
				ObjectName =  Actor->GetActorLabel();
			}
			*/
		}

		Title = ObjectName;
	}
	else if(Roots.Num() > 1)
	{
		Title = FText::Format(NSLOCTEXT("PropertyView", "MultipleToLevelObjectsSelectedFmt", "{0} selected"), Roots.Num()).ToString();
	}
	else
	{
		FObjectPropertyNode* RootPropertyNode = RootPropertyNodes[0]->AsObjectNode();
		Title = FText::Format( NSLOCTEXT("PropertyView", "MultipleSelected", "{0} ({1} selected)"), FText::FromString(RootPropertyNode->GetObjectBaseClass()->GetName()), RootPropertyNode->GetNumObjects() ).ToString();
	}

	OnObjectArrayChanged.ExecuteIfBound(Title, InObjects);

	RebuildSectionSelector();

	CustomWidgetsBox->ClearChildren();
	for (auto& Object : SelectedObjects)
	{
		IEditableObject* EditableObject = Cast<IEditableObject>(Object);
		if (EditableObject)
		{
			if (TSharedPtr<SWidget> ToolBar = EditableObject->GenerateToolBar())
			{
				CustomWidgetsBox->AddSlot()
				.FillEmptySpace(true)
				[
					ToolBar.ToSharedRef()
				];
			}
		}
	}

	double ElapsedTime = FPlatformTime::Seconds() - StartTime;
}

void SDetailsView::ReplaceObjects(const TMap<UObject*, UObject*>& OldToNewObjectMap)
{
	TArray<UObject*> NewObjectList;
	NewObjectList.Reserve(UnfilteredSelectedObjects.Num());
	TArray<TWeakObjectPtr<UObject>> NewUnfilteredSelectedObjects;
	NewUnfilteredSelectedObjects.Reserve(UnfilteredSelectedObjects.Num());

	bool bNeedRefresh = false;
	for (const TWeakObjectPtr<UObject>& Object : UnfilteredSelectedObjects)
	{
		// We could be replacing an object that has already been garbage collected, so look up the object using the raw pointer.
		UObject* Replacement = OldToNewObjectMap.FindRef(Object.GetEvenIfUnreachable());
		if (Replacement)
		{
			NewObjectList.Add(Replacement);
			NewUnfilteredSelectedObjects.Add(Replacement);
			bNeedRefresh = true;
		}
		else if (Object.IsValid())
		{
			NewObjectList.Add(Object.Get());
			NewUnfilteredSelectedObjects.Add(Object);
		}
		else
		{
			bNeedRefresh = true;
		}
	}

	if (bNeedRefresh)
	{
		UnfilteredSelectedObjects = MoveTemp(NewUnfilteredSelectedObjects);
		SetObjectArrayPrivate(NewObjectList);
	}
}

void SDetailsView::RemoveDeletedObjects(const TArray<UObject*>& DeletedObjects)
{
	TArray<UObject*> NewObjectList;
	NewObjectList.Reserve(UnfilteredSelectedObjects.Num());
	TArray<TWeakObjectPtr<UObject>> NewUnfilteredSelectedObjects;
	NewUnfilteredSelectedObjects.Reserve(UnfilteredSelectedObjects.Num());

	for (const TWeakObjectPtr<UObject>& Object : UnfilteredSelectedObjects)
	{
		if (Object.IsValid() && !DeletedObjects.Contains(Object.Get()))
		{
			NewUnfilteredSelectedObjects.Add(Object);
			NewObjectList.Add(Object.Get());
		}
	}

	if (NewUnfilteredSelectedObjects.Num() != UnfilteredSelectedObjects.Num())
	{
		UnfilteredSelectedObjects = MoveTemp(NewUnfilteredSelectedObjects);
		SetObjectArrayPrivate(NewObjectList);
	}
}

/** Called before during SetObjectArray before we change the objects being observed */
void SDetailsView::PreSetObject(int32 InNewNumObjects)
{
	TSharedPtr<SColorPicker> ExistingColorPicker = GetColorPicker();
	if (ExistingColorPicker.IsValid()
		&& ExistingColorPicker->GetOptionalOwningDetailsView().IsValid()
		&& ExistingColorPicker->GetOptionalOwningDetailsView().Get() != this)
	{
		DestroyColorPicker();
	}

	// Save existing expanded items first
	for(TSharedPtr<FComplexPropertyNode>& RootNode : RootPropertyNodes)
	{
		SaveExpandedItems(RootNode.ToSharedRef());

		RootNodesPendingKill.Add(RootNode);
		FObjectPropertyNode* RootObjectNode = RootNode->AsObjectNode();
		RootObjectNode->RemoveAllObjects();
		RootObjectNode->ClearCachedReadAddresses(true);
		RootObjectNode->ClearObjectPackageOverrides();
	}

	for(const FDetailLayoutData& Layout : DetailLayouts)
	{
		FRootPropertyNodeList& ExternalRootPropertyNodes = Layout.DetailLayout->GetExternalRootPropertyNodes();
		for (TSharedPtr<FComplexPropertyNode>& ExternalRootNode : ExternalRootPropertyNodes)
		{
			if (ExternalRootNode.IsValid())
			{
				SaveExpandedItems(ExternalRootNode.ToSharedRef());

				ExternalRootNode->Disconnect();
			}
		}
	}

	RootPropertyNodes.Empty(InNewNumObjects);
	ExpandedDetailNodes.Empty();

	for (int32 NewRootIndex = 0; NewRootIndex < InNewNumObjects; ++NewRootIndex)
	{
		RootPropertyNodes.Add(MakeShareable(new FObjectPropertyNode));
	}

	SelectedActors.Empty();
	SelectedObjects.Empty();
}


/** Called at the end of SetObjectArray after we change the objects being observed */
void SDetailsView::PostSetObject(const TArray<FDetailsViewObjectRoot>& Roots)
{
	TSharedPtr<SColorPicker> ExistingColorPicker = GetColorPicker();
	if (ExistingColorPicker.IsValid()
		&& (!ExistingColorPicker->GetOptionalOwningDetailsView().IsValid()
			|| ExistingColorPicker->GetOptionalOwningDetailsView().Get() == this))
	{
		DestroyColorPicker();
	}

	ColorPropertyNode = nullptr;

	// Are we editing PIE objects?  If the bShowHiddenPropertiesWhilePlaying setting is enabled, we may want to
	// show all of the properties that would normally be hidden for objects that are part of the PIE world.
	bool bAnyPIEObjects = false;
	{
		for( int32 RootNodeIndex = 0; RootNodeIndex < RootPropertyNodes.Num(); ++RootNodeIndex )
		{
			FObjectPropertyNode* RootPropertyNode = RootPropertyNodes[ RootNodeIndex ]->AsObjectNode();
			if( RootPropertyNode != nullptr )
			{
				const int32 ObjectCount = RootPropertyNode->GetNumObjects();
				for( int32 ObjectIndex = 0; ObjectIndex < ObjectCount; ++ObjectIndex )
				{
					UObject* Object = RootPropertyNode->GetUObject( ObjectIndex );
					if( Object->GetOutermost()->HasAnyPackageFlags( PKG_PlayInEditor ) )
					{
						bAnyPIEObjects = true;
						break;
					}
				}
			}
		}
	}

	FPropertyNodeInitParams InitParams;
	InitParams.ParentNode = nullptr;
	InitParams.Property = nullptr;
	InitParams.ArrayOffset = 0;
	InitParams.ArrayIndex = INDEX_NONE;
	InitParams.bAllowChildren = true;
	InitParams.bForceHiddenPropertyVisibility = 
		FPropertySettings::Get().ShowHiddenProperties() || 
		( GetDefault<USodaStyleSettings>()->bShowHiddenPropertiesWhilePlaying && bAnyPIEObjects ) ||
		DetailsViewArgs.bForceHiddenPropertyVisibility;
	InitParams.bGameModeOnlyVisible = DetailsViewArgs.bGameModeOnlyVisible;

	switch ( DetailsViewArgs.DefaultsOnlyVisibility )
	{
	case EEditDefaultsOnlyNodeVisibility::Hide:
		InitParams.bCreateDisableEditOnInstanceNodes = false;
		break;
	case EEditDefaultsOnlyNodeVisibility::Show:
		InitParams.bCreateDisableEditOnInstanceNodes = true;
		break;
	case EEditDefaultsOnlyNodeVisibility::Automatic:
		InitParams.bCreateDisableEditOnInstanceNodes = HasClassDefaultObject();
		break;
	default:
		check(false);
	}

	for( TSharedPtr<FComplexPropertyNode>& ComplexRootNode : RootPropertyNodes )
	{
		FObjectPropertyNode* RootPropertyNode = ComplexRootNode->AsObjectNode();

		RootPropertyNode->InitNode( InitParams );
	}

	RestoreAllExpandedItems();

	UpdatePropertyMaps();
	UpdateFilteredDetails();
}

void SDetailsView::SetOnObjectArrayChanged(FOnObjectArrayChanged OnObjectArrayChangedDelegate)
{
	OnObjectArrayChanged = OnObjectArrayChangedDelegate;
}

void SDetailsView::OnPostUndoRedo()
{
	InvalidateCachedState();
}

void SDetailsView::InvalidateCachedState()
{
	for (const TSharedPtr<FComplexPropertyNode>& RootNode : RootPropertyNodes)
	{
		RootNode->InvalidateCachedState();
	}
}

bool SDetailsView::IsConnected() const
{
	return GetNumObjects() > 0;
}

const FSlateBrush* SDetailsView::OnGetLockButtonImageResource() const
{
	if (bIsLocked)
	{
		return FSodaStyle::GetBrush(TEXT("PropertyWindow.Locked"));
	}
	else
	{
		return FSodaStyle::GetBrush(TEXT("PropertyWindow.Unlocked"));
	}
}

bool SDetailsView::IsShowHiddenPropertiesWhilePlayingChecked() const
{
	const FDetailsViewConfig2* ViewConfig = GetConstViewConfig();
	if (ViewConfig != nullptr)
	{
		return ViewConfig->bShowHiddenPropertiesWhilePlaying;
	}

	return false;
}

void SDetailsView::OnShowHiddenPropertiesWhilePlayingClicked()
{
	const bool bNewValue = !IsShowHiddenPropertiesWhilePlayingChecked();

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowHiddenPropertiesWhilePlaying = bNewValue;
		SaveViewConfig();
	}

	GConfig->SetBool(TEXT("/Script/SodaStyle.SodaStyleSettings"), TEXT("bShowHiddenPropertiesWhilePlaying"), bNewValue, GEditorPerProjectIni);

	// Force a refresh of the whole details panel, as the entire set of visible properties may be different
	ForceRefresh();
}

void SDetailsView::OnShowSectionsClicked()
{
	DetailsViewArgs.bShowSectionSelector = !DetailsViewArgs.bShowSectionSelector;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowSections = DetailsViewArgs.bShowSectionSelector;
		SaveViewConfig();
	}

	if (!DetailsViewArgs.bShowSectionSelector)
	{
		CurrentFilter.VisibleSections.Reset();
	}

	RebuildSectionSelector();
	UpdateFilteredDetails();
}

FSlateColor SDetailsView::GetToggleFavoritesColor() const
{
	if (DetailsViewArgs.bAllowFavoriteSystem && CurrentFilter.bShowFavoritesCategory)
	{
		return FSlateColor(EStyleColor::AccentBlue);
	}
	else
	{
		return FSlateColor(EStyleColor::Foreground);
	}
}

FReply SDetailsView::OnToggleFavoritesClicked()
{
	CurrentFilter.bShowFavoritesCategory = !CurrentFilter.bShowFavoritesCategory;

	FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
	if (ViewConfig != nullptr)
	{
		ViewConfig->bShowFavoritesCategory = CurrentFilter.bShowFavoritesCategory;
		SaveViewConfig();
	}

	RerunCurrentFilter();

	return FReply::Handled();
}

/*
static bool IsInEditorMetadataList(FName ListKey, FStringView Value, const TArray<TSharedPtr<FComplexPropertyNode>>& RootPropertyNodes)
{
	if (Value.IsEmpty())
	{
		return false;
	}

	UEditorMetadataOverrides* EditorMetadata = GEditor->GetEditorSubsystem<UEditorMetadataOverrides>();
	if (!EditorMetadata)
	{
		return false;
	}

	const FString ValueString(Value);

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* BaseStruct = RootPropertyNode->GetBaseStructure();
		if (BaseStruct != nullptr)
		{
			TArray<FString> ValueList;
			if (EditorMetadata->GetArrayMetadata(BaseStruct, ListKey, ValueList))
			{
				if (ValueList.Contains(ValueString))
				{
					return true;
				}
			}
		}
	}

	return false;
}

static void AddToEditorMetadataList(FName ListKey, FStringView Value, const TArray<TSharedPtr<FComplexPropertyNode>>& RootPropertyNodes)
{
	if (Value.IsEmpty())
	{
		return;
	}

	UEditorMetadataOverrides* MetadataOverrides = GEditor->GetEditorSubsystem<UEditorMetadataOverrides>();
	if (!MetadataOverrides)
	{
		return;
	}

	const FString ValueString(Value);

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* BaseStruct = RootPropertyNode->GetBaseStructure();
		if (BaseStruct != nullptr)
		{

			TArray<FString> ValueList;
			if (MetadataOverrides->GetArrayMetadata(BaseStruct, ListKey, ValueList))
			{
				ValueList.AddUnique(ValueString);
				MetadataOverrides->SetArrayMetadata(BaseStruct, ListKey, ValueList);
			}
			else
			{
				ValueList.Add(ValueString);
				MetadataOverrides->SetArrayMetadata(BaseStruct, ListKey, ValueList);
			}
		}
	}
}

static void RemoveFromEditorMetadataList(FName ListKey, FStringView Value, const TArray<TSharedPtr<FComplexPropertyNode>>& RootPropertyNodes)
{
	if (Value.IsEmpty())
	{
		return;
	}

	UEditorMetadataOverrides* MetadataOverrides = GEditor->GetEditorSubsystem<UEditorMetadataOverrides>();
	if (!MetadataOverrides)
	{
		return;
	}

	const FString ValueString(Value);

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* BaseStruct = RootPropertyNode->GetBaseStructure();
		if (BaseStruct != nullptr)
		{
			TArray<FString> ValueList;
			if (MetadataOverrides->GetArrayMetadata(BaseStruct, ListKey, ValueList))
			{
				ValueList.Remove(ValueString);
				MetadataOverrides->SetArrayMetadata(BaseStruct, ListKey, ValueList);
			}
		}
	}
}
*/
bool SDetailsView::IsGroupFavorite(FStringView GroupPath) const
{
	//static const FName FavoriteGroupsName("FavoriteGroups");
	//return IsInEditorMetadataList(FavoriteGroupsName, GroupPath, RootPropertyNodes);
	return false;
}

void SDetailsView::SetGroupFavorite(FStringView GroupPath, bool IsFavorite)
{
	/*
	static const FName FavoriteGroupsName("FavoriteGroups");
	if (IsFavorite)
	{
		AddToEditorMetadataList(FavoriteGroupsName, GroupPath, RootPropertyNodes);
	}
	else
	{
		RemoveFromEditorMetadataList(FavoriteGroupsName, GroupPath, RootPropertyNodes);
	}
	*/
}

bool SDetailsView::IsCustomBuilderFavorite(FStringView Path) const
{
	/*
	static const FName FavoriteCustomBuildersName("FavoriteCustomBuilders");
	return IsInEditorMetadataList(FavoriteCustomBuildersName, Path, RootPropertyNodes);
	*/
	return false;
}

void SDetailsView::SetCustomBuilderFavorite(FStringView Path, bool IsFavorite)
{
	/*
	static const FName FavoriteCustomBuildersName("FavoriteCustomBuilders");
	if (IsFavorite)
	{
		AddToEditorMetadataList(FavoriteCustomBuildersName, Path, RootPropertyNodes);
	}
	else
	{
		RemoveFromEditorMetadataList(FavoriteCustomBuildersName, Path, RootPropertyNodes);
	}
	*/
}

void SDetailsView::RebuildSectionSelector()
{
	SectionSelectorBox->ClearChildren();
	SectionSelectorBox->SetVisibility(EVisibility::Collapsed);

	if (!DetailsViewArgs.bShowSectionSelector)
	{
		return;
	}

	const TMap<FName, FText> AllSections = GetAllSections();
	if (AllSections.IsEmpty())
	{
		// we've selected something that has no sections - rather than show just "All", hide the box
		return;
	}

	auto CreateSection = [this](FName SectionName, FText SectionDisplayName) 
		-> TSharedRef<SWidget>
	{
		return SNew(SBox)
			[
				SNew(SCheckBox)
				.Style(FSodaStyle::Get(), "DetailsView.SectionButton")
				.OnCheckStateChanged(this, &SDetailsView::OnSectionCheckedChanged, SectionName)
				.IsChecked(this, &SDetailsView::IsSectionChecked, SectionName)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "SmallText")
					.Text(SectionDisplayName)
				]
			];
	};

	TArray<FName> SortedKeys;
	AllSections.GenerateKeyArray(SortedKeys);
	SortedKeys.Sort([](FName A, FName B)
		{
			static const FName General("General");
			static const FName All("All");

			// General first, All last, rest alphabetical
			if (A.IsEqual(General) || B.IsEqual(All))
			{
				return true;
			}
			if (A.IsEqual(All) || B.IsEqual(General))
			{
				return false;
			}
			return A.LexicalLess(B);
		});

	for (const FName& Key : SortedKeys)
	{
		SectionSelectorBox->AddSlot()
		[
			CreateSection(Key, AllSections[Key])
		];
	}

	SectionSelectorBox->AddSlot()
	[
		CreateSection(FName("All"), NSLOCTEXT("UObjectSection", "All", "All"))
	];

	SectionSelectorBox->SetVisibility(EVisibility::Visible);

	CurrentFilter.VisibleSections.Reset();

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* RootBaseStruct = RootPropertyNode->GetBaseStructure();
		const FDetailsViewConfig2* ViewConfig = GetConstViewConfig();
		if (ViewConfig != nullptr)
		{
			const FDetailsSectionSelection2* SectionSelection = ViewConfig->SelectedSections.Find(RootBaseStruct->GetFName());
			if (SectionSelection != nullptr)
			{
				CurrentFilter.VisibleSections = SectionSelection->SectionNames;
			}
		}
	}
}

void SDetailsView::OnSectionCheckedChanged(ECheckBoxState State, FName SectionName)
{
	const bool IsControlDown = FSlateApplication::Get().GetModifierKeys().IsControlDown();

	if (State == ECheckBoxState::Unchecked)
	{
		if (IsControlDown)
		{
			CurrentFilter.VisibleSections.Remove(SectionName);
		}
		else
		{
			CurrentFilter.VisibleSections.Reset();

			if (SectionName != "All")
			{
				CurrentFilter.VisibleSections.Add(SectionName);
			}
		}
	}
	else if (State == ECheckBoxState::Checked)
	{
		if (!IsControlDown)
		{
			CurrentFilter.VisibleSections.Reset();
		}

		if (SectionName != "All")
		{
			CurrentFilter.VisibleSections.Add(SectionName);
		}
	}

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* RootBaseStruct = RootPropertyNode->GetBaseStructure();
		FDetailsViewConfig2* ViewConfig = GetMutableViewConfig();
		if (ViewConfig != nullptr)
		{
			FDetailsSectionSelection2& SectionSelection = ViewConfig->SelectedSections.FindOrAdd(RootBaseStruct->GetFName());
			SectionSelection.SectionNames = CurrentFilter.VisibleSections;

			SaveViewConfig();
		}
	}

	UpdateFilteredDetails();
}

ECheckBoxState SDetailsView::IsSectionChecked(FName Section) const
{
	if (CurrentFilter.VisibleSections.IsEmpty() && Section == "All")
	{
		return ECheckBoxState::Checked;
	}

	for (FName VisibleSection : CurrentFilter.VisibleSections)
	{
		if (Section == VisibleSection)
		{
			return ECheckBoxState::Checked;
		}
	}

	return ECheckBoxState::Unchecked;
}

TMap<FName, FText> SDetailsView::GetAllSections() const
{
	static const FName PropertyEditor("RuntimeEditor");
	FRuntimeEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FRuntimeEditorModule>(PropertyEditor);

	TMap<FName, FText> AllSections;

	// for every category, check every base struct and find the associated section
	// if one exists, add it to the set of valid section names
	// we fetch the list of categories from the layouts since AllTreeNodes gets filtered
	TArray<FName> LayoutCategories;
	for (const FDetailLayoutData& Layout : DetailLayouts)
	{
		for (const TSharedRef<FDetailTreeNode>& TreeNode : Layout.DetailLayout->GetAllRootTreeNodes())
		{
			if (TreeNode->GetNodeType() == EDetailNodeType::Category)
			{
				FDetailCategoryImpl& Category = (FDetailCategoryImpl&) TreeNode.Get();
				TArray<TSharedRef<FDetailTreeNode>> Children;
				Category.GetGeneratedChildren(Children, true, false);

				LayoutCategories.Add(TreeNode->GetNodeName());
			}
		}
	}

	for (const TSharedPtr<FComplexPropertyNode>& RootPropertyNode : RootPropertyNodes)
	{
		const UStruct* RootBaseStruct = RootPropertyNode->GetBaseStructure();
		
		for (const FName& Category : LayoutCategories)
		{
			/*
			TArray<TSharedPtr<FPropertySection>> SectionsForCategory = PropertyModule.FindSectionsForCategory(RootBaseStruct, Category);
			for (const TSharedPtr<FPropertySection>& Section : SectionsForCategory)
			{
				AllSections.Add(Section->GetName(), Section->GetDisplayName());
			}
			*/
		}
	}

	return MoveTemp(AllSections);
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
