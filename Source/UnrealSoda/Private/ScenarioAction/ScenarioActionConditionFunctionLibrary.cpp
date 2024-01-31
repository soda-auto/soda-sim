// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionConditionFunctionLibrary.h"

UScenarioActionConditionFunctionLibraryBase::UScenarioActionConditionFunctionLibraryBase(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

UScenarioActionConditionFunctionLibrary::UScenarioActionConditionFunctionLibrary(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

bool UScenarioActionConditionFunctionLibrary::TestFunction1()
{
	return true;
}

bool UScenarioActionConditionFunctionLibrary::TestFunction2(bool Param1)
{
	return false;
}

bool UScenarioActionConditionFunctionLibrary::TestFunction3(bool Param1, int Param2, const FString& Param3)
{
	return false;
}

bool UScenarioActionConditionFunctionLibrary::TestFunction4(const FVector& Param1, const FRotator& Param2)
{
	return false;
}