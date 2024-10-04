// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleAnimationInstance.h"
#include "WheelsSkidEffect.generated.h"

class ASodaWheeledVehicle;
class UParticleSystemComponent;

/**
 * UWheelsAnimationComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UWheelsSkidEffectComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Mark emitter template for skid effect. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SlipEffect)
	UParticleSystem* MarkEmitterTemplate = nullptr;

	/** Smoke emitter template for skid effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SlipEffect)
	UParticleSystem* SmokeEmitterTemplate = nullptr;

	/** Enable skid effect. It can affect on performance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SlipEffect, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bEnableSkidEffect = false;

	/** Skid effect velocity threshold [cm/s]*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SlipEffect, SaveGame, meta = (EditInRuntime))
	float SkidEffectTreshold = 27.7778 * 10; //27.7778 - 1 km/h

	/** Skid effect smoke particale rate = SlipVelocity[cm/s] * SkidSmokeRateMultiplier  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SlipEffect, SaveGame, meta = (EditInRuntime))
	float SkidSmokeRateMultiplier = 0.5;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	UPROPERTY(Transient)
	TArray<UParticleSystemComponent*> MarkParticleSystem;

	UPROPERTY(Transient)
	TArray<UParticleSystemComponent*> SmokeParticleSystem;
};
