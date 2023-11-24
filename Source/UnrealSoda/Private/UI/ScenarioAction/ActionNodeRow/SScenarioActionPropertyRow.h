// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"

class SScenarioActionPropertyRow : public STableRow<TWeakObjectPtr<UScenarioActionNode>>
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionPropertyRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTable, TWeakObjectPtr<UScenarioActionPropertyNode> Node, const TSharedRef<SScenarioActionTree>& OwnerTree);

	TWeakObjectPtr<UScenarioActionPropertyNode> Node;
	//bool bIsDragEventRecognized = false;
	TSharedPtr<SScenarioActionTree> Tree;

private:
	TSharedRef<SWidget> GenerateAddMenu();
	void OnAddPropertyNode(FProperty* Property);
};
