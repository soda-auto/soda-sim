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

UENUM(BlueprintType)
enum class  EGearInputMode : uint8
{
	ByState,
	ByNum,
};

USTRUCT(BlueprintType)
struct UNREALSODA_API FWheeledVehicleInputState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	float Steering = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	float Throttle = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	float Brake = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	EGearInputMode GearInputMode = EGearInputMode::ByState;

	/** Valid only if  GearInputMode==EGearInputMode::ByState */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	EGearState GearState = EGearState::Neutral;

	/** Valid only if  GearInputMode==EGearInputMode::ByNum */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	int GearNum = 0;

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bWasGearUpPressed = false;

	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bWasGearDownPressed = false;

	/** External request to enable the AD mode. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bADModeEnbaled = false;

	/** External request to enable the safe stop mode. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bSafeStopEnbaled = false;

	/** External request to enable the daytime running lights. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bDaytimeRunningLightsEnabled = false;

	/** External request to enable the revers lights. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bHeadlightsEnabled = false;

	/** External request to enable the horn. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bHornEnabled = false;

	/** External request to enable the TVC */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bTVCEnabled = false;

	/** External request to enable the TVC */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bSpeedLimitEnabled = false;

	/** External request to enable the TVC */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	float SpeedLimit = 0;

	/** External request to enable the left turn lights. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bLeftTurnLightsEnabled = false;

	/** External request to enable the right turn lights. */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	bool bRightTurnLightsEnabled = false;

	/** Cruise Control / Speed Limiter mode */
	UPROPERTY(BlueprintReadWrite, Category = WheeledVehicleInputState, meta = (EditInRuntime))
	ECruiseControlMode CruiseControlMode = ECruiseControlMode::Off;

	TTimestamp UpdateTimestamp;

	void SetGearState(EGearState InGearState);
	void SetGearNum(int InGearNum);
	void GearUp();
	void GearDown();
	bool IsForwardGear() const;
	bool IsReversGear() const;
	bool IsNeutralGear() const;
	bool IsParkGear() const;
};



/**
 * Init to drive parameters
 */
USTRUCT(BlueprintType)
struct FInitToDrivePrms
{
	GENERATED_BODY()

public:
	/** True to perform init to drive sequence  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence")
		bool bDoInitToDriveSequence = false;

	/** Time to keep vehicle stationary in Park, sec  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence")
		float TimeToStayInPark = 5.0;

	/** Time to press brake pedal from 0 to 100%, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence", meta = (ClampMin = "1"))
		float TimeToPressBrakePedal = 0.5;

	/** Time to wait after full press to request Drive, sec  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence")
		float TimeToWaitBeforeSwitchToDrive = 4;

	/** Time to keep pedal pressed after switch to Drive, sec  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence")
		float TimeToStayInDriveWithPedalPressed = 1;

	/** Time to release brake pedal from 100% to 0%, sec  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence")
		float TimeToReleaseBrakePedal = 0.25;

	// Local states of the initialization process
	float InitSequenceLocalTime = 0;
	bool bInitSequenceIsDone = false;
	float CurPedlPosn = 0;



};

UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInput)
	EVehicleInputType InputType = EVehicleInputType::Other;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleInput, SaveGame, meta = (EditInRuntime))
	bool bUpdateDefaultsButtonsFromKeyboard = true;
	
	//TODO: Move CruiseControl to the VehicleDriver
	/** Cruise Control / Speed Limiter terget speed [km/h] */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, meta = (EditInRuntime))
	//float CruiseControlTargetSpeed = 0.f;

	/** Maximum throttle input available for Cruse Control input [0..1] */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, SaveGame, meta = (EditInRuntime))
	//float MaxCruiseControlThrottle = 0.7f;

	/** Cruse Control Proportional Coef */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl, SaveGame, meta = (EditInRuntime))
	//float CruiseControlPropCoef = 1.f;

	/** Cruise Control / Speed Limiter Throttle Input */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CruiseControl)
	//float CCThrottleInput = 0.f;
	

public:
	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual const FWheeledVehicleInputState& GetInputState() const { static FWheeledVehicleInputState Dummy; return Dummy; }
	virtual FWheeledVehicleInputState& GetInputState() { static FWheeledVehicleInputState Dummy; return Dummy; }

	/** Get normailzed torque (-1...1) which the user (driver) has attached to the steering wheel. */
	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual float GetDriverInputSteerTension() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual void SetHapticAutocenter(bool Enable) {}

	//UFUNCTION(BlueprintCallable, Category = VehicleInput)
	//virtual float GetCruiseControlModulatedThrottleInput(float ThrottleInput) const;

	virtual void UpdateInputStatesDefaultsButtons(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController);

public:
	virtual void CopyInputStates(UVehicleInputComponent* PreviousInput);
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController);

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
};