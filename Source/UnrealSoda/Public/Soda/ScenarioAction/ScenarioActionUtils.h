// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class AActor;
class UActorComponent;
class UFunction;
class UClass;
class UStruct;

namespace FScenarioActionUtils
{

void GetComponentsByCategory(AActor* Actor, TMap<FString, TArray<UActorComponent*>>& Out);

void GetScenarioFunctionsByCategory(UClass* Class, TMap<FString, TArray<UFunction*>> & Out);

void GetScenarioPropertiesByCategory(UClass* Class, TMap<FString, TArray<FProperty*>>& Out);

void GetScenarioActionFunctionsByCategory(TMap<FString, TArray<UFunction*>>& Out);

void GetScenarioConditionFunctionsByCategory(TMap<FString, TArray<UFunction*>>& Out);

void GetScenarioActionEvents(TMap<FString, TArray<UClass*>>& Out);

bool SerializeUStruct(FArchive& Ar, UStruct* Function, uint8* StructMemory, bool bForceSaveGame);

} // FScenarioActionUtils