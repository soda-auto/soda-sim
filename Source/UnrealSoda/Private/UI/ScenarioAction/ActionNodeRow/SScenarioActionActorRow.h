// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Widgets/Views/STableRow.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FMenuBuilder;
class UActorComponent;
class FProperty;
class UFunction;

class SScenarioActionActorRow : public STableRow<TWeakObjectPtr<UScenarioActionNode>>
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionActorRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTable, TWeakObjectPtr<UScenarioActionActorNode> Node, const TSharedRef<SScenarioActionTree>& OwnerTree);

	TWeakObjectPtr<UScenarioActionActorNode> Node;
	//bool bIsDragEventRecognized = false;
	TSharedPtr<SScenarioActionTree> Tree;

private:
	TSharedRef<SWidget> GenerateAddMenu();
	void GenerateAddComponentMenu(FMenuBuilder& MenuBuilder);
	void GenerateAddFunctionsMenu(FMenuBuilder& MenuBuilder);
	void OnAddComponentNode(UActorComponent* Component);
	void OnAddPropertyNode(FProperty* Property);
	void OnAddFunctionNode(UFunction* Function);
	FReply OnDeleteAction();
};
