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
	/** Rate at which input throttle can rise and fall */
	UPROPERTY(Category = VehicleInputExternal, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate ThrottleInputRate;

	/** Rate at which input brake can rise and fall */
	UPROPERTY(Category = VehicleInputExternal, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate BrakeInputRate;

	UPROPERTY(Category = VehicleInputExternal, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate SteerInputRate;

	UPROPERTY(BlueprintReadWrite, Category = VehicleInputExternal, meta = (EditInRuntime))
	float SteeringInputTarget = 0;

	UPROPERTY(BlueprintReadWrite, Category = VehicleInputExternal, meta = (EditInRuntime))
	float ThrottleInputTarget = 0;

	UPROPERTY(BlueprintReadWrite, Category = VehicleInputExternal, meta = (EditInRuntime))
	float BrakeInputTarget = 0;

	UPROPERTY(BlueprintReadWrite, Category = VehicleInputExternal, meta = (EditInRuntime))
	ENGear GearInput = ENGear::Park;

public:
	virtual void CopyInputStates(UVehicleInputComponent* Previous) override;
	virtual float GetSteeringInput() const override { return SteeringInput; }
	virtual float GetThrottleInput() const override { return GetCruiseControlModulatedThrottleInput(ThrottleInput); }
	virtual float GetBrakeInput() const override { return BrakeInput; }
	virtual ENGear GetGearInput() const override { return  GearInput; }
	//virtual float GetDriverInputSteerTension() const override { return 0; }
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController) override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void SetSteeringInput(float Value) override { SteeringInputTarget = Value; }
	virtual void SetThrottleInput(float Value) override { ThrottleInputTarget = Value; }
	virtual void SetBrakeInput(float Value) override { BrakeInputTarget = Value; }
	virtual void SetGearInput(ENGear Value) override { GearInput = Value; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	float SteeringInput = 0;
	float ThrottleInput = 0;
	float BrakeInput = 0;
};