// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SStructureDetailsView.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
//#include "AssetSelection.h"
#include "RuntimePropertyEditor/DetailCategoryBuilderImpl.h"
#include "RuntimePropertyEditor/UserInterface/PropertyDetails/PropertyDetailsUtilities.h"
#include "RuntimePropertyEditor/StructurePropertyNode.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SSearchBox.h"
#include "RuntimePropertyEditor/DetailsViewPropertyGenerationUtilities.h"


#define LOCTEXT_NAMESPACE "SStructureDetailsView"

namespace soda
{


SStructureDetailsView::~SStructureDetailsView()
{
	auto RootNodeLocal = GetRootNode();

	if (RootNodeLocal.IsValid())
	{
		SaveExpandedItems(RootNodeLocal.ToSharedRef());
	}
}

UStruct* SStructureDetailsView::GetBaseScriptStruct() const
{
	const UStruct* Struct = StructData.IsValid() ? StructData->GetStruct() : NULL;
	return const_cast<UStruct*>(Struct);
}

void SStructureDetailsView::Construct(const FArguments& InArgs)
{
	DetailsViewArgs = InArgs._DetailsViewArgs;

	ColumnSizeData.SetValueColumnWidth(DetailsViewArgs.ColumnWidth);
	ColumnSizeData.SetRightColumnMinWidth(DetailsViewArgs.RightColumnMinWidth);

	CustomName = InArgs._CustomName;

	// Create the root property now
	// Only one root node in a structure details view
	RootNodes.Empty(1);
	RootNodes.Add(MakeShareable(new FStructurePropertyNode));
		
	PropertyUtilities = MakeShareable( new FPropertyDetailsUtilities( *this ) );
	PropertyGenerationUtilities = MakeShareable(new FDetailsViewPropertyGenerationUtilities(*this));
	
	TSharedRef<SScrollBar> ExternalScrollbar = SNew(SScrollBar);

	// See note in SDetailsView for why visibility is set after construction
	ExternalScrollbar->SetVisibility( TAttribute<EVisibility>(this, &SStructureDetailsView::GetScrollBarVisibility) );

	FMenuBuilder DetailViewOptions( true, nullptr );

	FUIAction ShowOnlyModifiedAction( 
		FExecuteAction::CreateSP(this, &SStructureDetailsView::OnShowOnlyModifiedClicked),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SStructureDetailsView::IsShowOnlyModifiedChecked)
	);

	if (DetailsViewArgs.bShowModifiedPropertiesOption)
	{
		DetailViewOptions.AddMenuEntry( 
			LOCTEXT("ShowOnlyModified", "Show Only Modified Properties"),
			LOCTEXT("ShowOnlyModified_ToolTip", "Displays only properties which have been changed from their default"),
			FSlateIcon(),
			ShowOnlyModifiedAction,
			NAME_None,
			EUserInterfaceActionType::ToggleButton 
		);
	}
	if (DetailsViewArgs.bShowKeyablePropertiesOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowOnlyKeyable", "Show Only Keyable Properties"),
			LOCTEXT("ShowOnlyKeyable_ToolTip", "Displays only properties which are keyable"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SStructureDetailsView::OnShowKeyableClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SStructureDetailsView::IsShowKeyableChecked)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	if (DetailsViewArgs.bShowAnimatedPropertiesOption)
	{
		DetailViewOptions.AddMenuEntry(
			LOCTEXT("ShowAnimated", "Show Only Animated Properties"),
			LOCTEXT("ShowAnimated_ToolTip", "Displays only properties which are animated (have tracks)"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SStructureDetailsView::OnShowAnimatedClicked),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SStructureDetailsView::IsShowAnimatedChecked)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	FUIAction ShowAllAdvancedAction( 
		FExecuteAction::CreateSP(this, &SStructureDetailsView::OnShowAllAdvancedClicked),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SStructureDetailsView::IsShowAllAdvancedChecked)
	);

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ShowAllAdvanced", "Show All Advanced Details"),
		LOCTEXT("ShowAllAdvanced_ToolTip", "Shows all advanced detail sections in each category"),
		FSlateIcon(),
		ShowAllAdvancedAction,
		NAME_None,
		EUserInterfaceActionType::ToggleButton 
		);

	DetailViewOptions.AddMenuEntry(
		LOCTEXT("CollapseAll", "Collapse All Categories"),
		LOCTEXT("CollapseAll_ToolTip", "Collapses all root level categories"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SStructureDetailsView::SetRootExpansionStates, /*bExpanded=*/false, /*bRecurse=*/false)));
	DetailViewOptions.AddMenuEntry(
		LOCTEXT("ExpandAll", "Expand All Categories"),
		LOCTEXT("ExpandAll_ToolTip", "Expands all root level categories"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SStructureDetailsView::SetRootExpansionStates, /*bExpanded=*/true, /*bRecurse=*/false)));

	TSharedRef<SHorizontalBox> FilterBoxRow = SNew( SHorizontalBox )
		.Visibility(this, &SStructureDetailsView::GetFilterBoxVisibility)
		+SHorizontalBox::Slot()
		.FillWidth( 1 )
		.VAlign( VAlign_Center )
		[
			// Create the search box
			SAssignNew( SearchBox, SSearchBox )
			.OnTextChanged(this, &SStructureDetailsView::OnFilterTextChanged)
		];

	if (DetailsViewArgs.bShowOptions)
	{
		FilterBoxRow->AddSlot()
			.HAlign(HAlign_Right)
			.AutoWidth()
			[
				SNew( SComboButton )
				.ContentPadding(0)
				.ForegroundColor( FSlateColor::UseForeground() )
				.ButtonStyle( FAppStyle::Get(), "ToggleButton" )
				.MenuContent()
				[
					DetailViewOptions.MakeWidget()
				]
				.ButtonContent()
				[
					SNew(SImage)
					.Image( FAppStyle::GetBrush("GenericViewButton") )
				]
			];
	}

	SAssignNew(DetailTree, SDetailTree)
		.Visibility(this, &SStructureDetailsView::GetTreeVisibility)
		.TreeItemsSource(&RootTreeNodes)
		.OnGetChildren(this, &SStructureDetailsView::OnGetChildrenForDetailTree)
		.OnSetExpansionRecursive(this, &SStructureDetailsView::SetNodeExpansionStateRecursive)
		.OnGenerateRow(this, &SStructureDetailsView::OnGenerateRowForDetailTree)
		.OnExpansionChanged(this, &SStructureDetailsView::OnItemExpansionChanged)
		.SelectionMode(ESelectionMode::None)
		.ExternalScrollbar(ExternalScrollbar);

	ChildSlot
	[
		SNew( SBox )
		.Visibility(this, &SStructureDetailsView::GetPropertyEditingVisibility)
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 0.0f, 0.0f, 2.0f )
			[
				FilterBoxRow
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.Padding(0)
			[
				SNew( SOverlay )
				+ SOverlay::Slot()
				[
					DetailTree.ToSharedRef()
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				[
					SNew( SBox )
					.WidthOverride( 16.0f )
					[
						ExternalScrollbar
					]
				]
			]
		]
	];
}

void SStructureDetailsView::SetStructureData(TSharedPtr<FStructOnScope> InStructData)
{
	TSharedPtr<FComplexPropertyNode> RootNode = GetRootNode();
	//PRE SET
	SaveExpandedItems(RootNode.ToSharedRef() );
	RootNode->AsStructureNode()->SetStructure(TSharedPtr<FStructOnScope>(nullptr));
	RootNodesPendingKill.Add(RootNode);

	RootNodes.Empty(1);
	ExpandedDetailNodes.Empty();

	RootNode = MakeShareable(new FStructurePropertyNode);
	RootNodes.Add(RootNode);

	//SET
	StructData = InStructData;
	RootNode->AsStructureNode()->SetStructure(StructData);
	if (!StructData.IsValid())
	{
		bIsLocked = false;
	}
	
	//POST SET
	DestroyColorPicker();
	ColorPropertyNode = NULL;

	FPropertyNodeInitParams InitParams;
	InitParams.ParentNode = NULL;
	InitParams.Property = NULL;
	InitParams.ArrayOffset = 0;
	InitParams.ArrayIndex = INDEX_NONE;
	InitParams.bAllowChildren = true;
	InitParams.bForceHiddenPropertyVisibility = FPropertySettings::Get().ShowHiddenProperties() || DetailsViewArgs.bForceHiddenPropertyVisibility;
	InitParams.bCreateCategoryNodes = false;

	RootNode->InitNode(InitParams);
	RootNode->SetDisplayNameOverride(CustomName);

	RestoreExpandedItems(RootNode.ToSharedRef());

	UpdatePropertyMaps();

	UpdateFilteredDetails();
}

void SStructureDetailsView::SetCustomName(const FText& Text)
{
	CustomName = Text;
}

void SStructureDetailsView::ForceRefresh()
{
	SetStructureData(StructData);
}

void SStructureDetailsView::ClearSearch()
{
	CurrentFilter.FilterStrings.Empty();
	SearchBox->SetText(FText::GetEmpty());
	RerunCurrentFilter();
}


const TArray< TWeakObjectPtr<UObject> >& SStructureDetailsView::GetSelectedObjects() const
{
	static const TArray< TWeakObjectPtr<UObject> > DummyRef;
	return DummyRef;
}

const TArray< TWeakObjectPtr<AActor> >& SStructureDetailsView::GetSelectedActors() const
{
	static const TArray< TWeakObjectPtr<AActor> > DummyRef;
	return DummyRef;
}


//const fselectedactorinfo& sstructuredetailsview::getselectedactorinfo() const
//{
//	static const fselectedactorinfo dummyref;
//	return dummyref;
//}

bool SStructureDetailsView::IsConnected() const
{
	const FStructurePropertyNode* RootNode = GetRootNode().IsValid() ? GetRootNode()->AsStructureNode() : nullptr;
	return StructData.IsValid() && StructData->IsValid() && RootNode && RootNode->HasValidStructData();
}

FRootPropertyNodeList& SStructureDetailsView::GetRootNodes()
{
	return RootNodes;
}

TSharedPtr<class FComplexPropertyNode> SStructureDetailsView::GetRootNode()
{
	return RootNodes[0];
}

const TSharedPtr<class FComplexPropertyNode> SStructureDetailsView::GetRootNode() const
{
	return RootNodes[0];
}

void SStructureDetailsView::CustomUpdatePropertyMap(TSharedPtr<FDetailLayoutBuilderImpl>& InDetailLayout)
{
	FName StructCategoryName = NAME_None;
	
	if (StructData.IsValid() && StructData->GetStruct())
	{
		TArray<FName> CategoryNames;
		InDetailLayout->GetCategoryNames(CategoryNames);

		int32 StructCategoryNameIndex = INDEX_NONE;
		CategoryNames.Find(StructData->GetStruct()->GetFName(), StructCategoryNameIndex);
		StructCategoryName = StructCategoryNameIndex != INDEX_NONE ? CategoryNames[StructCategoryNameIndex] : NAME_None;
	}
	
	InDetailLayout->DefaultCategory(StructCategoryName).SetDisplayName(NAME_None, CustomName);
}

EVisibility SStructureDetailsView::GetPropertyEditingVisibility() const
{
	const FStructurePropertyNode* RootNode = GetRootNode().IsValid() ? GetRootNode()->AsStructureNode() : nullptr;
	return StructData.IsValid() && StructData->IsValid() && RootNode && RootNode->HasValidStructData() ? EVisibility::Visible : EVisibility::Collapsed;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
