// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakInterfacePtr.h"
#include "Widgets/Views/STreeView.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionNode.h"



class  SScenarioActionTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionTree)
	{ }
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TWeakObjectPtr<UScenarioActionBlock> ScenarioBlock);
	void RebuildNodes();
	void RequestSelectRow(UScenarioActionNode* Node) { RequestedSelecteNode = Node; }
	const TSharedPtr<STreeView<TWeakObjectPtr<UScenarioActionNode>>>& GetTreeView() const { return TreeView; }

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	TSharedRef< ITableRow > MakeRowWidget(TWeakObjectPtr<UScenarioActionNode> Node, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TWeakObjectPtr<UScenarioActionNode> Component, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnGetChildren(TWeakObjectPtr<UScenarioActionNode> InTreeNode, TArray< TWeakObjectPtr<UScenarioActionNode> >& OutChildren);

protected:
	TArray<TWeakObjectPtr<UScenarioActionNode>> RootNodes;
	TSharedPtr<STreeView<TWeakObjectPtr<UScenarioActionNode>>> TreeView;
	TWeakObjectPtr<UScenarioActionBlock> ScenarioBlock;
	TWeakObjectPtr<UScenarioActionNode> RequestedSelecteNode;
};
