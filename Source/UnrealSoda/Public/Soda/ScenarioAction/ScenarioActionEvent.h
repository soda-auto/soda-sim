// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "ScenarioActionEvent.generated.h"

class SWidget;

/**
 * UScenarioActionEvent
 */
UCLASS(abstract, meta = (RuntimeMetaData))
class UNREALSODA_API UScenarioActionEvent : public UObject
{
	GENERATED_BODY()

public:
	UScenarioActionEvent(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionEvent() {};

public:
	virtual TSharedRef< SWidget > MakeWidget();
	virtual FText GetDisplayName() const { return DisplayName; }

	virtual void Tick(float DeltaTime) {}
	virtual void ScenarioBegin() {}
	virtual void ScenarioEnd() {}

	FSimpleDelegate EventDelegate;
	
protected:
	FText DisplayName;
};
