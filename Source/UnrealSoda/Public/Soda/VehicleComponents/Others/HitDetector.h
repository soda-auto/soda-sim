// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "UObject/StrongObjectPtr.h"
#include "HitDetector.generated.h"



UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UHitDetectorComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Scenario, SaveGame, meta = (EditInRuntime))
	bool bStopScenarioIfHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Scenario, SaveGame, meta = (EditInRuntime))
	float ImpulseThresholdScenarioStop = 10000;

public:
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	UFUNCTION()
	virtual void OnVehicleHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
};
