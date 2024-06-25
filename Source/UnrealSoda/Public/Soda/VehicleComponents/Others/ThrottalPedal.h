// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "ThrottalPedal.generated.h"


UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UThrottalPedalComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.IOBusComponent"))
	FSubobjectReference LinkToIOBus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ThrottalPedal, SaveGame, meta = (EditInRuntime, ReactivateActor))
	FName IOBusNodeName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ThrottalPedal, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TArray<FIOPinSetup> Pins;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	UPROPERTY();
	UIOBusComponent* IOBus{};

	UPROPERTY();
	UIOBusNode* Node {};
};
