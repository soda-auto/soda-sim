// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Soda/ScenarioAction/ScenarioActionCondition.h"
#include "Soda/ScenarioAction/ScenarioActionEvent.h"
#include "Soda/Misc/SerializationHelpers.h"

UScenarioActionBlock::UScenarioActionBlock(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

void UScenarioActionBlock::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame())
	{
		
		if (Ar.IsSaving())
		{
			// Saving the ConditionMatrix
			int ConditionRowNum = ConditionMatrix.Num();
			Ar << ConditionRowNum;
			for (int i = 0; i < ConditionRowNum; ++i)
			{
				int ColNum = ConditionMatrix[i].ScenarioConditiones.Num();
				Ar << ColNum;
				for (int j = 0; j < ColNum; ++j)
				{
					UScenarioActionCondition* ActionCondition = ConditionMatrix[i].ScenarioConditiones[j];
					check(IsValid(ActionCondition));
					FObjectRecord Record;
					Record.SerializeObject(ActionCondition);
					Ar << Record;
				}
			}

			// Saving the ActionNodes
			int ActionRootNum = ActionRootNodes.Num();
			Ar << ActionRootNum;
			for (int i = 0; i < ActionRootNum; ++i)
			{
				UScenarioActionNode* ActionNode = ActionRootNodes[i];
				check(IsValid(ActionNode));
				FObjectRecord Record;
				Record.SerializeObject(ActionNode);
				Ar << Record;
			}

			// Saving the Events
			int EventsNum = Events.Num();
			Ar << EventsNum;
			for (int i = 0; i < EventsNum; ++i)
			{
				UScenarioActionEvent* Event = Events[i];
				check(IsValid(Event));
				FObjectRecord Record;
				Record.SerializeObject(Event);
				Ar << Record;
			}
			
		}
		else if (Ar.IsLoading())
		{
			// Loading the ConditionMatrix
			int ConditionRowNum = 0;
			Ar << ConditionRowNum;
			ConditionMatrix.SetNum(ConditionRowNum);
			for (int i = 0; i < ConditionRowNum; ++i)
			{
				int ColNum = 0;
				Ar << ColNum;
				for (int j = 0; j < ColNum; ++j)
				{
					FObjectRecord Record;
					Ar << Record;
					if (Record.IsRecordValid())
					{
						UScenarioActionCondition *& ActionCondition = ConditionMatrix[i].ScenarioConditiones.Add_GetRef(nullptr);
						ActionCondition = NewObject<UScenarioActionCondition>(this, Record.Class.Get());
						check(ActionCondition);
						Record.DeserializeObject(ActionCondition);
					}
				}
			}

			// Loading the ActionNodes
			int ActionRootNum = 0;
			Ar << ActionRootNum;
			for (int i = 0; i < ActionRootNum; ++i)
			{
				FObjectRecord Record;
				Ar << Record;
				if (Record.IsRecordValid())
				{
					UScenarioActionNode*& ActionNode = ActionRootNodes.Add_GetRef(nullptr);
					ActionNode = NewObject<UScenarioActionNode>(this, Record.Class.Get());
					check(ActionNode);
					Record.DeserializeObject(ActionNode);
				}
			}

			// Loading the Events
			int EventsNum = Events.Num();
			Ar << EventsNum;
			for (int i = 0; i < EventsNum; ++i)
			{
				FObjectRecord Record;
				Ar << Record;
				if (Record.IsRecordValid())
				{
					UScenarioActionEvent*& Event = Events.Add_GetRef(nullptr);
					Event = NewObject<UScenarioActionEvent>(this, Record.Class.Get());
					check(Event);
					Record.DeserializeObject(Event);
				}
			}
		}
	}
}

void UScenarioActionBlock::ExecuteActions()
{
	for (auto& It : ActionRootNodes)
	{
		if (IsValid(It))
		{
			It->Execute();
		}
	}
	++ExexuteCounter;
}

bool UScenarioActionBlock::ExecuteConditionMatrix()
{
	bool ThereIsCell = false;

	for (auto& Row : ConditionMatrix)
	{
		bool RowRes = true;
		for (auto& Col : Row.ScenarioConditiones)
		{
			ThereIsCell = true;
			RowRes = RowRes && Col->Execute();
			if (!RowRes) break;
		}
		if (RowRes) return true;
	}

	return !ThereIsCell;
}

bool UScenarioActionBlock::ExecuteBlock()
{
	if (GetActionBlockMode() == EScenarioActionBlockMode::MultipleExecute || ExexuteCounter == 0)
	{
		if (ExecuteConditionMatrix())
		{
			ExecuteActions();
			return true;
		}
	}
	return false;
}