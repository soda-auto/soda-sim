// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "VehicleInputKeyboardComponent.generated.h"
 

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputKeyboardComponent : public UVehicleInputComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FWheeledVehicleInputState InputState {};

	/** Rate at which input throttle can rise and fall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	FInputRate ThrottleInputRate;

	/** Rate at which input brake can rise and fall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	FInputRate BrakeInputRate;

	/** Max steer angle (-1..1) versus forward speed (km/h)  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput)
	FRuntimeFloatCurve InputMaxSteerPerSpeedCurve;

	/** Steering speed rate (steering(-1..1)/s) versus forward speed (km/h) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput)
	FRuntimeFloatCurve InputSteeringSpeedCurve;

	/** Trunc the maximum input steering value [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	float InputSteeringLimit = 1;

	/** Trunc the maximum input throttle value [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	float InputThrottleLimit = 1;

	/** Trunc the maximum input brake value [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	float InputBrakeLimit = 1;

	/** Automatic gear shifting when driving from the keyboard */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	bool bAutoReavers = true;

	/** Shift time if bAutoReavers is enbled (sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, SaveGame, meta = (EditInRuntime))
	float GearChangeTime = 0.2;

public:
	virtual const FWheeledVehicleInputState& GetInputState() const override { return InputState; }
	virtual FWheeledVehicleInputState& GetInputState() override { return InputState; }
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController) override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const override;

protected:

	float RawThrottleInput = 0.f;
	float RawSteeringInput = 0.f;
	float RawBrakeInput = 0.f;

	FInputRate SteerRate;
	float CurrentGearChangeTime = -1.f;
	float FeedbackDriverSteerTension = 0.f;
};