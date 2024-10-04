// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Templates/SharedPointer.h"

class SScenarioActionFunctionBodyRow : public STableRow<TWeakObjectPtr<UScenarioActionNode>>
{
	SLATE_BEGIN_ARGS(SScenarioActionFunctionBodyRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTable, TWeakObjectPtr<UScenarioActionFunctionBodyNode> Node, const TSharedRef<SScenarioActionTree>& OwnerTree);

	TWeakObjectPtr<UScenarioActionFunctionBodyNode> Node;
	//bool bIsDragEventRecognized = false;
	TSharedPtr<SScenarioActionTree> Tree;
};
