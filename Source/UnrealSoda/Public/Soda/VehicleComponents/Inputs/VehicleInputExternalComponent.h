// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "VehicleInputExternalComponent.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputExternalComponent : public UVehicleInputComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = VehicleInputExternal, meta = (EditInRuntime))
	FWheeledVehicleInputState InputState{};

public:
	virtual const FWheeledVehicleInputState& GetInputState() const override { return InputState; }
	virtual FWheeledVehicleInputState& GetInputState() override { return InputState; }
};