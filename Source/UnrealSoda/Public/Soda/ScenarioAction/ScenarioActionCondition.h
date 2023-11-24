// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

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
#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealNames.h"
#include "UObject/UnrealType.h"
#include "Widgets/Views/ITableRow.h"
#include "Widgets/SNullWidget.h"
#include "ScenarioActionCondition.generated.h"

class FStructOnScope;

/**
 * UScenarioActionCondition
 */
UCLASS(abstract)
class UNREALSODA_API UScenarioActionCondition : public UObject
{
	GENERATED_BODY()

public:
	UScenarioActionCondition(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionCondition() {};

public:
	virtual bool Execute() { return false; }
	virtual TSharedRef< SWidget > MakeWidget() { return SNullWidget::NullWidget; }
	virtual FText GetDisplayName() const { return DisplayName; }
	
protected:
	FText DisplayName;
};


/**
 * UScenarioActionConditionFunction
 */
UCLASS()
class UNREALSODA_API UScenarioActionConditionFunction : public UScenarioActionCondition
{
	GENERATED_BODY()

public:
	UScenarioActionConditionFunction(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionConditionFunction() {};

	virtual void Serialize(FArchive& Ar) override;

public:
	virtual bool Execute();
	virtual TSharedRef< SWidget > MakeWidget() override;

public:
	void SetFunction(UFunction* Function, UClass * OwnerClass);
	UFunction* GetFunction() { return Function.Get(); }
	const UFunction* GetFunction() const { return Function.Get(); }

protected:
	TSharedPtr<FStructOnScope> StructOnScope;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<UFunction> Function;

	UPROPERTY(SaveGame)
	TSoftClassPtr<UClass> FunctionOwner;
};