// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"

class SScenarioActionFunctionRow : public STableRow<TWeakObjectPtr<UScenarioActionNode>>
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionFunctionRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTable, TWeakObjectPtr<UScenarioActionFunctionNode> Node, const TSharedRef<SScenarioActionTree>& OwnerTree);

	FReply OnDeleteAction();

	TWeakObjectPtr<UScenarioActionFunctionNode> Node;
	//bool bIsDragEventRecognized = false;
	TSharedPtr<SScenarioActionTree> Tree;
};
