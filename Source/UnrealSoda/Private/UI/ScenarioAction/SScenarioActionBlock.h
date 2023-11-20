// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakInterfacePtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SBoxPanel.h"

class SScenarioActionTree;
class UScenarioActionBlock;
class SScenarioActionEditor;
class UScenarioActionCondition;
class UScenarioActionEvent;

class SScenarioActionBlock : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SScenarioActionBlock)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScenarioActionBlock* InBlock, TSharedRef<SScenarioActionEditor> InScenarioEditor);

	void RebuildEventsBlock();
	void RebuildConditionalBlock();

protected:
	TSharedRef<SWidget> GenerateAddConditionalMenu(int RowIndex);
	TSharedRef<SWidget> GenerateAddActionMenu();
	TSharedRef<SWidget> GenerateAddEventMenu();

	void OnAddConditional(UFunction* Function, int RowIndex);
	FReply OnDeleteConditional(UScenarioActionCondition* Condition, int RowIndex);

	void OnAddActorActionNode(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo);
	void OnAddFunctionActionNode(UFunction* Function);

	void OnAddEvent(UClass* Class);
	FReply OnDeleteEvent(UScenarioActionEvent * Event);

	TSharedRef<SWidget> GetComboModeContent();

protected:
	TWeakObjectPtr< UScenarioActionBlock> Block;
	TWeakPtr<SScenarioActionEditor> ScenarioEditor;
	TSharedPtr<SScenarioActionTree> ActionTree;
	TSharedPtr<SVerticalBox> ConditionRows;
	TSharedPtr<SHorizontalBox> Events;
};