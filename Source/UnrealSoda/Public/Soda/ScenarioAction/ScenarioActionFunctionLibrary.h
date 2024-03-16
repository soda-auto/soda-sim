// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenarioActionFunctionLibrary.generated.h"



UCLASS()
class UNREALSODA_API UScenarioActionFunctionLibraryBase : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
};


UCLASS(meta=(RuntimeMetaData))
class UNREALSODA_API UScenarioActionFunctionLibrary : public UScenarioActionFunctionLibraryBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool TestFunction1();

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool TestFunction2(bool Param1);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool TestFunction3(bool Param1, int Param2, const FString & Param3);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool TestFunction4(const FVector & Param1, const FRotator & Param2);
};


