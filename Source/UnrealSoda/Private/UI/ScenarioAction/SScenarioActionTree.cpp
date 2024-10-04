// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SScenarioActionTree.h"
#include "Soda/UnrealSoda.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "SPositiveActionButton.h"
#include "Kismet/GameplayStatics.h"
//#include "Soda/ISodaActor.h"
//#include "RuntimeEditorModule.h"
//#include "RuntimePropertyEditor/IDetailsView.h"
//#include "Soda/SodaStatics.h"
//#include "Soda/SodaSubsystem.h"
//#include "Soda/Vehicles/SodaVehicle.h"
//#include "Soda/UI/ToolBoxes/Common/SVehicleComponentClassCombo.h"
//#include "Soda/UI/ToolBoxes/Common/SPinWidget.h"
//#include "Soda/UI/SMessageBox.h"
//#include "Soda/SodaGameViewportClient.h"


#include "Soda/ScenarioAction/ScenarioAction.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionNode.h"

#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "ScenarioActionTree"


/***********************************************************************************/
void SScenarioActionTree::Construct( const FArguments& InArgs, TWeakObjectPtr<UScenarioActionBlock> InScenarioBlock)
{
	ScenarioBlock = InScenarioBlock;

	ChildSlot
	[
		SAssignNew(TreeView, STreeView<TWeakObjectPtr<UScenarioActionNode>>)
		.SelectionMode(ESelectionMode::Single)
		.TreeItemsSource(&RootNodes)
		.OnGenerateRow(this, &SScenarioActionTree::MakeRowWidget)
		.OnSelectionChanged(this, &SScenarioActionTree::OnSelectionChanged)
		.OnContextMenuOpening(this, &SScenarioActionTree::OnContextMenuOpening)
		.OnGetChildren(this, &SScenarioActionTree::OnGetChildren)
		//.OnMouseButtonDoubleClick(this, &SScenarioActionTree::OnDoubleClick)
	];
	
	RebuildNodes();
}

void SScenarioActionTree::RebuildNodes()
{
	//SExpandableArea

	RootNodes.Empty();
	if (ScenarioBlock.IsValid())
	{
		for (auto& It : ScenarioBlock.Get()->ActionRootNodes)
		{
			if (It->IsNodeValid())
			{
				RootNodes.Add(It);
			}
		}
	}
	TreeView->RebuildList();

	/*
	for (auto& It : RootNodes)
	{
		TreeView->SetItemExpansion(It, true);
	}
	*/
}

TSharedRef< ITableRow > SScenarioActionTree::MakeRowWidget(TWeakObjectPtr<UScenarioActionNode> Node, const TSharedRef< STableViewBase >& OwnerTable)
{
	return Node->MakeRowWidget(OwnerTable, SharedThis(this));
}

void SScenarioActionTree::OnGetChildren(TWeakObjectPtr<UScenarioActionNode> Node, TArray< TWeakObjectPtr<UScenarioActionNode> >& OutChildren)
{
	OutChildren = Node->GetChildren();
}

void SScenarioActionTree::OnSelectionChanged(TWeakObjectPtr<UScenarioActionNode> Node, ESelectInfo::Type SelectInfo)
{
}

TSharedPtr<SWidget> SScenarioActionTree::OnContextMenuOpening()
{
	return TSharedPtr<SWidget>();
}

FReply SScenarioActionTree::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SScenarioActionTree::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

#undef LOCTEXT_NAMESPACE
