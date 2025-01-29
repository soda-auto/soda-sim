// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "Soda/Misc/PIDController.h"
#include "VehicleInputOpenLoopComponent.generated.h"

class ASodaWheeledVehicle;
class USodaVehicleWheelComponent;
class IWheeledVehicleMovementInterface;


UENUM(BlueprintType)
enum class EOpenLoopManeuverType : uint8 {
	RampSteer      UMETA(DisplayName = "RampSteer"),
	DoubleTripleStepSteer      UMETA(DisplayName = "DoubleTripleStepSteer"),
	AccelerationAndBraking      UMETA(DisplayName = "AccelerationAndBraking"),
	CustomInputFile UMETA(DisplayName = "CustomInputFile"),
	LookupTable UMETA(DisplayName = "LookupTable"),
};


UENUM(BlueprintType)
enum class EOutputMode : uint8 {
	DirectControl      UMETA(DisplayName = "DirectControl"),
	InputComponent      UMETA(DisplayName = "InputComponent"),

};



USTRUCT(BlueprintType)
struct FTimeVsValue
{
	GENERATED_BODY()

public:
	/** Time for lookup tables, values have to be incrementing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		float Time = 0;

	/** Corresponded value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		float Value = 0;
};

USTRUCT(BlueprintType)
struct FTimeVsGear
{
	GENERATED_BODY()

public:
	/** Time for lookup tables, values have to be incrementing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		float Time = 0;

	/** Gear */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		EGearState GearState = EGearState::Park;
};





USTRUCT(BlueprintType)
struct FLookupTablePrms
{
	GENERATED_BODY()

public:
	/** Lut for braking pedal position, sec vs % */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		TArray<FTimeVsValue> BrkPedl;

	/** Lut for acceleration pedal position, sec vs % */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		TArray<FTimeVsValue> AccrPedl;

	/** Lut for road wheel angle, sec vs deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		TArray<FTimeVsValue> Steering;

	/** Lut for gear position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		TArray<FTimeVsGear> Gear;

	/** True to use gear lookup table, otherwise automatically move to D */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		bool bUseGearPosnLut = false;

};


USTRUCT(BlueprintType)
struct FDoubleStepSteerPrms
{
	GENERATED_BODY()

public:
	/** Timout before start of the acceleration, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float TimeToStartScenario = 1.0;

	/** Rate of steering input, deg/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.1"), meta = (ClampMax = "5000"))
		float SteeringRate = 50;

	/** deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "1"), meta = (ClampMax = "100"))
		float MaxSteeringAngle = 25;

	/** Speed to start steering input, m/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"), meta = (ClampMax = "300"))
		float StartSpeed = 15;

	/** Time to hold maximum steering angle, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float TimeToKeepMaxSteering = 0.3;

	/** True to add additional step input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		bool bTripleSteer = false;

	/** True to apply steering with + first, false to apply with - first */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		bool bSteerPositive = true;

};

USTRUCT(BlueprintType)
struct FRampSteerPrms
{
	GENERATED_BODY()

public:

	/** Rate of steering increment, deg/sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.1"), meta = (ClampMax = "1000"))
		float SteerRate = 4.5;

	/** deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "1"), meta = (ClampMax = "100"))
		float MaxSteeringAngle = 25;

	/** m/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"), meta = (ClampMax = "300"))
		float StartSpeed = 15;

	/** True to apply steering with +, false to apply with - */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		bool bSteerPositive = true;
};

USTRUCT(BlueprintType)
struct FAccelerationAndBraking
{
	GENERATED_BODY()

public:
	/** Timout before start of the acceleration, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float TimeToStartScenario = 1.0;

	/** sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float AccelerationDuration = 4.0;

	/** Total acceleration torque applied to the vehicle, will be distributed to wheels, N*m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		float AccelerationTorqueTotal = 1500;

	/** sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float DecelerationDuration = 4.0;

	/** Total deceleration torque applied to the vehicle, will be distributed to wheels, N*m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
		float DecelerationTorqueTotal = 1500;

	/** Time to build up a torque from 0 to max value, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
		float TorqueBuildUpTime = 0.01;

};



USTRUCT(BlueprintType)
struct FOpenLoopExternalInput4Wheels
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float> Time = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>SteerFL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>SteerFR = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>SteerRL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>SteerRR = { 0.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>TracTqFL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>TracTqFR = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>TracTqRL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>TracTqRR = { 0.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>BrkTqFL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>BrkTqFR = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>BrkTqRL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		TArray<float>BrkTqRR = { 0.0 };

};


USTRUCT(BlueprintType)
struct FOpenLoopInput4Wheels
{
	GENERATED_BODY()

public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float SteerFL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float SteerFR = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float SteerRL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float SteerRR = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float TracTqFL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float TracTqFR = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float TracTqRL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float TracTqRR = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float BrkTqFL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float BrkTqFR = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float BrkTqRL = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		float BrkTqRR = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
		EGearState GearState = EGearState::Park;

};



UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputOpenLoopComponent : public UVehicleInputComponent
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "AI controller", meta = (EditInRuntime))
		FWheeledVehicleInputState InputState {};

	/** Structure to work with external input tables */
	FOpenLoopExternalInput4Wheels OpenLoopExternalInput;
	/** Structure to keep current open loop signal */
	FOpenLoopInput4Wheels OpenLoopInput;

	UPROPERTY()
		UCurveFloat* CurveFloatSteering;
	UPROPERTY()
		UCurveFloat* CurveFloatTraction;
	UPROPERTY()
		UCurveFloat* CurveFloatBraking;


	/** Direct control mode will send torques per wheel directly to the vehicle model, Input Component mode will generate human-like input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Setup")
		EOutputMode OutputMode = EOutputMode::DirectControl;

	/** Total torque value equal to 100% of the pedal to be used in InputComponent mode. Values above that will clip to 100%  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Setup", meta = (ClampMin = "1"), meta = (ClampMax = "100000"))
		float MaxTorque2Pedal = 2000;
	/** Maximum road wheel angle (deg) to be used in InputComponent mode. Should be equivalent to max angle in SteeringComponent for correct results  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Setup", meta = (ClampMin = "1"), meta = (ClampMax = "100000"))
		float MaxSteeringAngle = 30;

	/** Longitudinal speed controller, proportional gain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "LongitudinalControl")
		float VLgtCtlP = -250;
	/** Longitudinal speed controller, integral gain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "LongitudinalControl")
		float VLgtCtlI = -10;
	/** Longitudinal speed controller, tolerance to consider that acceleration part of the scenario is done */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "LongitudinalControl", meta = (ClampMin = "0"), meta = (ClampMax = "100"))
		float VLgtTolerance = 1.0;

	float VLgtIError = 0.0;

	/** Balance of the traction torque, 1 for FWD, 0 for RWD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "TorqueBalance", meta = (ClampMin = "0"), meta = (ClampMax = "1"))
		float TracTqBalance = 0.5;

	/** Balance of the braking torque, 1 for FWD, 0 for RWD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "TorqueBalance", meta = (ClampMin = "0"), meta = (ClampMax = "1"))
		float BrkTqBalance = 0.5;

	/** True to stop the vehicle to zero velocity when scenario is done  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters")
		bool bBrakeWhenScenarioIsDone = true;

	/** True to generate notification when scenario is done  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters")
		bool bGenerateNotificationWhenDone = true;

	/** Total braking torque request to apply when scenario is done, affected by BrkTqBalance, N*m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "bBrakeWhenScenarioIsDone"))
		float BrkTqReqWhenScenarioIsDone = 2000;


	/** Select maneuver type to unlock the settings for it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters")
		EOpenLoopManeuverType OpenLoopManeuverType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::DoubleTripleStepSteer"))
		FDoubleStepSteerPrms DoubleTripleStepSteerPrm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::RampSteer"))
		FRampSteerPrms RampSteerPrms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::AccelerationAndBraking"))
		FAccelerationAndBraking AccelerationAndBraking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::LookupTable"))
		FLookupTablePrms LookupTable;


	/** True to start the scenario with vehicle being in P for some time and switch gear to D with brake pedal pressed  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence", meta = (EditCondition = "bDoInitSequence"))
		FInitToDrivePrms InitToDrivePrms;



	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Load CSV file with input", CallInRuntime))
		void LoadInputFromFile();
	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Generate CSV file template for custom input", CallInRuntime))
		void GenerateInputFileTemplate();

	UFUNCTION()
		bool LoadCustomInput(const FString& InFileName);

	void DoInitSequence(float Ts);


	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Load CSV file with lookup input", CallInRuntime))
		void LoadLutInputFromFile();
	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Generate CSV file template for lookup input", CallInRuntime))
		void GenerateLutInputFileTemplate();

	UFUNCTION()
		bool LoadLutCustomInput(const FString& InFileName);


protected:


	enum class EScenarioStatus : uint8
	{
		NA,
		Preparation,
		Active,
		Error,
		Completed,
	};


	/** Helper to keep scenario state */
	struct FScenarioRunner
	{
		bool bScenarioAcv = true;
		float TimeToStart = 0;
		float TargetVLgt = 0;
		bool bFeedbackControlLongitudinal = false;
		float TimeOfFinish = 0;
		EScenarioStatus ScenarioStatus = EScenarioStatus::NA;
	};

	FScenarioRunner ScenarioSt;
	float LocalTime = 0;
	float GlobalTime = 0;

	void SetupDoubleStepSteer();
	void SetupRampSteer();
	void SetupAccelerationBraking();
	void SetupCustomInputFile();
	void ResetInputCurves();
	void CopyCurveTimestamps(const UCurveFloat* CopyFrom, UCurveFloat* CopyTo);
	void SetupLookupTable();
	bool ValidateLookupInput(const TArray<FTimeVsValue>& Lut);
	void InitZeroLut(TArray<FTimeVsValue>& Lut);
	void GetNewInput(float DeltaTime);
	void ScenarioIsDone();
	void AssignOutputs();


	IWheeledVehicleMovementInterface* WheeledComponentInterface = nullptr;

	UPROPERTY()
		ASodaWheeledVehicle* Veh = nullptr;

	UPROPERTY()
		USodaVehicleWheelComponent* WheelFL = nullptr;

	UPROPERTY()
		USodaVehicleWheelComponent* WheelFR = nullptr;

	UPROPERTY()
		USodaVehicleWheelComponent* WheelRL = nullptr;

	UPROPERTY()
		USodaVehicleWheelComponent* WheelRR = nullptr;


public:


	//virtual void BeginPlay() override;
	//virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual const FWheeledVehicleInputState& GetInputState() const override { return InputState; }
	virtual FWheeledVehicleInputState& GetInputState() override { return InputState; }
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController) override;
	//virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;


public:

	/** Helpers to work with custom input */
	static EGearState LookupTable1dGear(float Time, const TArray<FTimeVsGear>& TimeVsGear);

};

