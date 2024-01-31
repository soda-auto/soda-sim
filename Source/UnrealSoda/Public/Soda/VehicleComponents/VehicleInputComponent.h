// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "VehicleInputComponent.generated.h"

class ISodaJoystickPlugin;
class IWheeledVehicleMovementInterface;
class ASodaWheeledVehicle;

/**
 * Cruise Control / Speed Limiter mode
 */
UENUM(BlueprintType)
enum class  ECruiseControlMode : uint8
{
	/** Cruise Control / Speed Limiter off */
	Off = 0,

	/** Cruise Control Active. */
	CruiseControlActive = 1,

	/** Speed Limiter Active */
	SpeedLimiterActive = 2,
};

UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInput)
	EVehicleInputType InputType = EVehicleInputType::Other;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, meta = (EditInRuntime))
	bool bADModeInput = false;

	/** External request to enable the safe stop mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, meta = (EditInRuntime))
	bool bSafeStopInput = false;

	/** External request to enable the daytime running lights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, meta = (EditInRuntime))
	bool bEnabledDaytimeRunningLightsInput = false;

	/** External request to enable the revers lights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, meta = (EditInRuntime))
	bool bEnabledHeadlightsInput = false;

	/** External request to enable the horn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, meta = (EditInRuntime))
	bool bEnabledHornInput = false;

	/** Cruise Control / Speed Limiter mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, meta = (EditInRuntime))
	ECruiseControlMode CruiseControlMode = ECruiseControlMode::Off;

	/** Cruise Control / Speed Limiter terget speed [km/h] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, meta = (EditInRuntime))
	float CruiseControlTargetSpeed = 0.f;

	/** Maximum throttle input available for Cruse Control input [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, SaveGame, meta = (EditInRuntime))
	float MaxCruiseControlThrottle = 0.7f;

	/** Cruse Control Proportional Coef */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, SaveGame, meta = (EditInRuntime))
	float CruiseControlPropCoef = 1.f;

	/** Cruise Control / Speed Limiter Throttle Input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl)
	float CCThrottleInput = 0.f;

public:
	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetSteeringInput() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetThrottleInput() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetBrakeInput() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual ENGear GetGearInput() const { return  ENGear::Park; }

	/** Get normailzed torque (-1...1) which the user (driver) has attached to the steering wheel. */
	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetDriverInputSteerTension() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual void SetHapticAutocenter(bool Enable) {}

	UFUNCTION(BlueprintCallable, Category = VehicleInput, meta = (ScenarioAction))
	virtual void SetADMode(bool bIsADMode) {}

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetCruiseControlModulatedThrottleInput(float ThrottleInput) const;

	UFUNCTION(BlueprintCallable, Category = VehicleInput, meta = (ScenarioAction))
	virtual void SetSteeringInput(float Value);

	UFUNCTION(BlueprintCallable, Category = VehicleInput, meta = (ScenarioAction))
	virtual void SetThrottleInput(float Value);

	UFUNCTION(BlueprintCallable, Category = VehicleInput, meta = (ScenarioAction))
	virtual void SetBrakeInput(float Value);

	UFUNCTION(BlueprintCallable, Category = VehicleInput, meta = (ScenarioAction))
	virtual void SetGearInput(ENGear Value);

public:
	virtual void CopyInputStates(UVehicleInputComponent* PreviousInput) {}
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController);

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
};