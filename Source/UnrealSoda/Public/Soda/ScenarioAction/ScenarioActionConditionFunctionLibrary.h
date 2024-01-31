// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenarioActionConditionFunctionLibrary.generated.h"

class ASodaVehicle;

UCLASS()
class UNREALSODA_API UScenarioActionConditionFunctionLibraryBase : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
};


UCLASS(meta=(RuntimeMetaData))
class UNREALSODA_API UScenarioActionConditionFunctionLibrary : public UScenarioActionConditionFunctionLibraryBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Test)
	static bool TestFunction1();

	UFUNCTION(BlueprintCallable, Category = Test)
	static bool TestFunction2(bool Param1);

	UFUNCTION(BlueprintCallable, Category = Test)
	static bool TestFunction3(bool Param1, int Param2, const FString & Param3);

	UFUNCTION(BlueprintCallable, Category = Test)
	static bool TestFunction4(const FVector & Param1, const FRotator & Param2);



	static bool AtScenarioTime(float Time);
	static bool ReachPosition(ASodaVehicle * Vehicle, FVector2D & Position, float Width, float Delay);
	static bool ReltiveDistance(AActor* FromActor, AActor* ToActor, float Distance, float Delay); // Add: LessThan/GreaterThan; Position/ClosesPoint; Longotude/Euclidean
	static bool RelativeSpeed(AActor* FromActor, AActor* ToActor, float Speed, float Delay); // Add: LessThan/GreaterThan
	static bool StandStill(AActor* Actor, float Duration, float SpeedTolerance, float Delay);
	// static bool TrafficLightState(ATrafficLight* TrafficLight, State, float Delay);


};


