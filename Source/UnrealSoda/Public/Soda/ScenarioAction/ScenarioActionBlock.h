// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "HAL/Platform.h"
#include "Internationalization/Text.h"
#include "Math/Color.h"
#include "Misc/AssertionMacros.h"
#include "Misc/EnumClassFlags.h"
#include "Misc/Guid.h"
#include "Misc/InlineValue.h"
#include "Templates/SubclassOf.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealNames.h"
#include "UObject/UnrealType.h"
#include "Widgets/Views/ITableRow.h"


#include "ScenarioActionBlock.generated.h"

class AScenarioAction;
class UScenarioActionCondition;
class UScenarioActionNode;
class SScenarioActionEditor;
class UScenarioActionEvent;

UENUM()
enum class EScenarioActionBlockMode: uint8
{
	SingleExecute,
	MultipleExecute
};

USTRUCT()
struct FScenarioConditionRow
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UScenarioActionCondition*> ScenarioConditiones;

	UScenarioActionCondition* operator[](int i) { return ScenarioConditiones[i]; }
	const UScenarioActionCondition* operator[](int i) const { return ScenarioConditiones[i]; }
};

/**
 * UScenarioActionBlock
 */
UCLASS()
class UNREALSODA_API UScenarioActionBlock: public UObject
{
	GENERATED_BODY()

public:
	UScenarioActionBlock(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionBlock() {};

	virtual void Serialize(FArchive& Ar) override;

	bool ExecuteBlock();
	void ExecuteActions();
	bool ExecuteConditionMatrix();

	void SetDisplayName(FName InDisplayName) { DisplayName = InDisplayName; }
	FName GetDisplayName() const { return DisplayName; }

	void SetActionBlockMode(EScenarioActionBlockMode Mode) { ActionBlockMode = Mode; }
	EScenarioActionBlockMode GetActionBlockMode() const { return ActionBlockMode; }

	void ResetExexuteCounter() { ExexuteCounter = 0; }

	void OnEvent() { ExecuteBlock(); }

public:
	UPROPERTY()
	TWeakObjectPtr<AScenarioAction> OwnedScenario;

	UPROPERTY(SaveGame)
	FName DisplayName;

	UPROPERTY()
	TArray<UScenarioActionEvent*> Events;

	/**
	 *  The Condition Matrix;
	 *  Rows - "OR"
	 *  Cols - "AND"
	 */
	UPROPERTY()
	TArray<FScenarioConditionRow> ConditionMatrix;

	UPROPERTY()
	TArray<UScenarioActionNode*> ActionRootNodes;

	UPROPERTY(SaveGame)
	EScenarioActionBlockMode ActionBlockMode;

	int ExexuteCounter = 0;
};
