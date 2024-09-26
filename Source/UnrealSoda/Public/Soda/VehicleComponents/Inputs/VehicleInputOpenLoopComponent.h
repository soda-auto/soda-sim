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
};



USTRUCT(BlueprintType)
struct FDoubleStepSteerPrms
{
	GENERATED_BODY()

public:
	/** Rate of steering input, deg/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0.1"), meta = (ClampMax = "5000"))
	float SteeringRate = 50;

	/** deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "1"), meta = (ClampMax = "100"))
	float MaxSteeringAngle = 25;

	/** Speed to start steering input, m/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"), meta = (ClampMax = "300"))
	float StartSpeed = 15;

	/** Time to hold maximum steering angle, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float TimeToKeepMaxSteering = 0.3;

	/** True to add additional step input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	bool bTripleSteer = false;

	/** True to apply steering with + first, false to apply with - first */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	bool bSteerPositive = true;

};

USTRUCT(BlueprintType)
struct FRampSteerPrms
{
	GENERATED_BODY()

public:

	/** Rate of steering increment, deg/sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0.1"), meta = (ClampMax = "1000"))
	float SteerRate = 4.5;

	/** deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "1"), meta = (ClampMax = "100"))
	float MaxSteeringAngle = 25;

	/** m/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"), meta = (ClampMax = "300"))
	float StartSpeed = 15;

	/** True to apply steering with +, false to apply with - */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	bool bSteerPositive = true;
};

USTRUCT(BlueprintType)
struct FAccelerationAndBraking
{
	GENERATED_BODY()

public:
	/** Timout before start of the acceleration, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float TimeToStartScenario = 1.0;

	/** sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float AccelerationDuration = 4.0;

	/** Total acceleration torque applied to the vehicle, will be distributed to wheels, N*m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	float AccelerationTorqueTotal = 1500;

	/** sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float DecelerationDuration = 4.0;

	/** Total deceleration torque applied to the vehicle, will be distributed to wheels, N*m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	float DecelerationTorqueTotal = 1500;

	/** Time to build up a torque from 0 to max value, sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float TorqueBuildUpTime = 0.01;

};



USTRUCT(BlueprintType)
struct FOpenLoopInput4Wheels
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float> Time = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>SteerFL = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>SteerFR = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>SteerRL = { 0.0 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>SteerRR = { 0.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>TracTqFL = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>TracTqFR = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>TracTqRL = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>TracTqRR = { 0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>BrkTqFL = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>BrkTqFR = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>BrkTqRL = { 0.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime))
	TArray<float>BrkTqRR = { 0.0};

};



UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputOpenLoopComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:

	FOpenLoopInput4Wheels OpenLoopInput;

	UPROPERTY()
	UCurveFloat* CurveFloatSteering;
	UPROPERTY()
	UCurveFloat* CurveFloatTraction;
	UPROPERTY()
	UCurveFloat* CurveFloatBraking;


	/** Longitudinal speed controller, proportional gain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), Category = "LongitudinalControl")
	float VLgtCtlP = -250;
	/** Longitudinal speed controller, integral gain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), Category = "LongitudinalControl")
	float VLgtCtlI = -10;
	/** Longitudinal speed controller, tolerance to consider that acceleration part of the scenario is done */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), Category = "LongitudinalControl", meta = (ClampMin = "0"), meta = (ClampMax = "100"))
	float VLgtTolerance = 1.0;

	float VLgtIError = 0.0;

	/** Balance of the traction torque, 1 for FWD, 0 for RWD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), Category = "TorqueBalance", meta = (ClampMin = "0"), meta = (ClampMax = "1"))
	float TracTqBalance = 0.5;

	/** Balance of the braking torque, 1 for FWD, 0 for RWD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditInRuntime), Category = "TorqueBalance", meta = (ClampMin = "0"), meta = (ClampMax = "1"))
	float BrkTqBalance = 0.5;

	

	/** Select maneuver type to unlock the settings for it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters")
	EOpenLoopManeuverType OpenLoopManeuverType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::DoubleTripleStepSteer"))
	FDoubleStepSteerPrms DoubleTripleStepSteerPrm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::RampSteer"))
	FRampSteerPrms RampSteerPrms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "OpenLoopGeneralParameters", meta = (EditCondition = "OpenLoopManeuverType == EOpenLoopManeuverType::AccelerationAndBraking"))
	FAccelerationAndBraking AccelerationAndBraking;



	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Load CSV file with input", CallInRuntime))
	void LoadInputFromFile();
	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Generate CSV file template for custom input", CallInRuntime))
	void GenerateInputFileTemplate();

	UFUNCTION()
	bool LoadCustomInput(const FString& InFileName);

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
		EScenarioStatus ScenarioStatus = EScenarioStatus::NA;
	};

	FScenarioRunner ScenarioSt;
	float LocalTime = 0;

	void SetupDoubleStepSteer();
	void SetupRampSteer();
	void SetupAccelerationBraking();
	void SetupCustomInputFile();
	void ResetInputCurves();
	void CopyCurveTimestamps(const UCurveFloat* CopyFrom, UCurveFloat* CopyTo);


	void GetNewInput(float DeltaTime);
	void ScenarioIsDone();

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


protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;


public:

	/** Helpers to work with custom input */
	static float Lut1(float x, float x0, float x1, float y0, float y1);
	static float Lut1d(float X, const TArray<float>& BreakPoints, const TArray<float>& Values);

};