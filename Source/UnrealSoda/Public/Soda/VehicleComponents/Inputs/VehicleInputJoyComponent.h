// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "VehicleInputJoyComponent.generated.h"

class UVehicleSteeringRackBaseComponent;

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputJoyComponent : public UVehicleInputComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FWheeledVehicleInputState InputState{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleSteeringRackBaseComponent"))
	FSubobjectReference LinkToSteering { TEXT("SteeringRack") };

	/** In the dead zone the break input will be zero. [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, SaveGame, meta = (EditInRuntime))
	float InputBrakeDeadzone = 0.02f;

	/** 
	 * Сontinue driving at a low speed when the accelerator pedal is released.
	 * This is an imitation of the behavior of a real automatic transmission.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, SaveGame, meta = (EditInRuntime))
	bool bCreepMode = false;

	/** Vehicle speed when the accelerator pedal is released and bCreepMode is enabled [km/h] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, SaveGame, meta = (EditInRuntime))
	float CreepSpeed = 7.f;

	/** Maximum throttle input available for creep mode [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, SaveGame, meta = (EditInRuntime))
	float MaxCreepThrottle = 0.2f;

	/** Enable the effect of returning the steering wheel to the center position in relation to vehicle speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	bool bFeedbackAutocenterEnabled = true; 

	/**  Enable the effect of the resistance roation of stering wheel in relation to stering wheel angular velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	bool bFeedbackResistionEnabled = true; 

	/**  Enable the effect of the rotation of the wheel to match the position of the wheels and steering wheel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	bool bFeedbackDiffEnabled = true;

	/** Multiplier for bFeedbackAutocenterEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	float FeedbackAutocenterCoeff = 2.0;

	/** Multipliert for bFeedbackDiffEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	float FeedbackDiffCoeff = 1.0;

	/** Multiplier for bFeedbackResistionEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	float FeedbackResistionCoeff = 0.0;

	/** 
	 * Time axis (X axis) - vehicle speed [km/h]
	 * Value axis (Y axis) - normalized value (force) [0..1] of the force that will be applied at the steering wheel to return it to center position.
	 */
	UPROPERTY(EditAnywhere, Category = SteeringWheelFeedback)
	FRuntimeFloatCurve FeedbackAutocenterCurve;

	/** 
	 * Time axis (X axis) - difference beetween steering wheel and front vehicle wheels [deg].
	 * Value axis (Y axis) - normalized value (force) [0..1] of the force to rotate steering wheels.
	 */
	UPROPERTY(EditAnywhere, Category = SteeringWheelFeedback)
	FRuntimeFloatCurve FeedbackDiffCurve;

	/** 
	 * Time axis (X axis) - steering wheel rotation speed [nozmalized_value/s].
	 * Value axis (Y axis) - normalized value (force) [0..1] of the rotational resistance force.
	 */
	UPROPERTY(EditAnywhere, Category = SteeringWheelFeedback)
	FRuntimeFloatCurve FeedbackResistionCurve;

	/** Сommon filter [0..1] for feedback effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime, UIMin = 0, ClampMin = 0, UIMax = 1, ClampMax = 1))
	float FeedbackFilter = 0.3;

	/** Multiplier for calculated normalized force of user (driver) has attached to the steering wheel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	float CalcDriverForceCompensationCoeff = 1.5;

	/** Area in which the driver (user) force applied to the steering wheel considered as 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringWheelFeedback, SaveGame, meta = (EditInRuntime))
	float CalcDriverForceCompensationDeadZone = 0.4;

	/** Show the steering wheel feedback effect (for steering wheel joystic) debug values on the debug canvas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bShowFeedback = false;

	/** Enable bump feedback effect  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BumpEffect, SaveGame, meta = (EditInRuntime))
	bool bEnableBumpEffect = false;

	/** Threshold front wheel suspension offset changing speed to activete bump feedback effect  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BumpEffect, SaveGame, meta = (EditInRuntime))
	float BumpEffectThreshold = 150.f;

	/** Coef to calculate force for bump feedback effect  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BumpEffect, SaveGame, meta = (EditInRuntime))
	float BumpEffectForceCoef = 30;


public:
	UFUNCTION(Category = "VehicleJoyInput", BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	void ReinitDevice();

public:
	virtual const FWheeledVehicleInputState& GetInputState() const override { return InputState; }
	virtual FWheeledVehicleInputState& GetInputState() override { return InputState; }
	virtual float GetDriverInputSteerTension() const override { return FeedbackDriverSteerTension; }
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController) override;
	virtual void SetHapticAutocenter(bool Enable) { bFeedbackAutocenterEnabled = Enable; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const override;

protected:
	ISodaJoystickPlugin * Joy = nullptr;

	UPROPERTY()
	UVehicleSteeringRackBaseComponent* SteeringRack = nullptr;

	float MaxSteer = 0;

	float FeedbackDiffFactor = 0.f;
	float FeedbackResistionFactor = 0.f;
	float FeedbackAutocenterFactor = 0.f;
	float FeedbackFullFactor = 0.f;
	float FeedbackDriverSteerTension = 0.f;

	float FrontWheelPrevSuspensionOffset[2] = { 0.f, 0.f };
};