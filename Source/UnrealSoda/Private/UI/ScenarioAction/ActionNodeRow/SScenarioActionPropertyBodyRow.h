// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"

class SScenarioActionPropertyBodyRow : public STableRow<TWeakObjectPtr<UScenarioActionNode>>
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionPropertyBodyRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTable, TWeakObjectPtr<UScenarioActionPropertyBodyNode> Node, const TSharedRef<SScenarioActionTree>& OwnerTree);

	TWeakObjectPtr<UScenarioActionPropertyBodyNode> Node;
	//bool bIsDragEventRecognized = false;
	TSharedPtr<SScenarioActionTree> Tree;

protected:
	void OnDeleteAction(FProperty* Property);
};
