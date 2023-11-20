// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionFunctionLibrary.h"

UScenarioActionFunctionLibraryBase::UScenarioActionFunctionLibraryBase(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

UScenarioActionFunctionLibrary::UScenarioActionFunctionLibrary(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

bool UScenarioActionFunctionLibrary::TestFunction1()
{
	return true;
}

bool UScenarioActionFunctionLibrary::TestFunction2(bool Param1)
{
	return true;
}

bool UScenarioActionFunctionLibrary::TestFunction3(bool Param1, int Param2, const FString& Param3)
{
	return true;
}

bool UScenarioActionFunctionLibrary::TestFunction4(const FVector& Param1, const FRotator& Param2)
{
	return true;
}
