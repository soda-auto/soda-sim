// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SodaVehicleWheel.generated.h"

#define WHEEL_CHASSIS_BIT_MASK 0xF
#define WHEEL_CHASSIS_BIT_OFFSET 4
#define WHEEL_SIDE_BIT_MASK 0xF


UENUM(BlueprintType)
enum class  EWheelChassis : uint8
{
	Ch0 = 0,
	Ch1 = 1,
	Ch2 = 2,
	Ch3 = 3,
	Ch4 = 4,
	Ch5 = 5,
	Ch6 = 6,
	Ch7 = 7,
	Undefined = 0xF
};

UENUM(BlueprintType)
enum class  EWheelSida: uint8
{
	Left = 0,
	Right = 1,
	Center = 2,
	Undefined = 0xF
};

/**
 * Wheeled vehicle wheel indexes
 */
UENUM(BlueprintType)
enum class  EWheelIndex : uint8
{
	/* 4WD - classic wheeled vehicle */
	FL = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	FR = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	RL = ((uint8)EWheelChassis::Ch1 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	RR = ((uint8)EWheelChassis::Ch1 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,

	/* xWD - symmetrical wheeled vehicle, the same number of wheels on the left and right sides */
	Ch0L = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch0R = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch1L = ((uint8)EWheelChassis::Ch1 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch1R = ((uint8)EWheelChassis::Ch1 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch2L = ((uint8)EWheelChassis::Ch2 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch2R = ((uint8)EWheelChassis::Ch2 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch3L = ((uint8)EWheelChassis::Ch3 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch3R = ((uint8)EWheelChassis::Ch3 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch4L = ((uint8)EWheelChassis::Ch4 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch4R = ((uint8)EWheelChassis::Ch4 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch5L = ((uint8)EWheelChassis::Ch5 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch5R = ((uint8)EWheelChassis::Ch5 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch6L = ((uint8)EWheelChassis::Ch6 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch6R = ((uint8)EWheelChassis::Ch6 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,
	Ch7L = ((uint8)EWheelChassis::Ch7 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	Ch7R = ((uint8)EWheelChassis::Ch7 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,

	/* 2WD - bicycle */
	FC = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Center,
	RC = ((uint8)EWheelChassis::Ch1 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Center,

	/* Only left & right */
	L = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Left,
	R = ((uint8)EWheelChassis::Ch0 << WHEEL_CHASSIS_BIT_OFFSET) | (uint8)EWheelSida::Right,

	Undefined = 0xFF
};


/**
 * USodaVehicleWheelComponent object
 * TODO: Rename to UVehicleWheelInterfaceComponent 
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))   
class UNREALSODA_API USodaVehicleWheelComponent 
	: public UWheeledVehicleComponent
	, public ITorqueTransmission
{
	GENERATED_UCLASS_BODY()

public:
	/** Bone name on mesh to create wheel at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	EWheelChassis WheelChassis = EWheelChassis::Undefined;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	EWheelSida WheelSida = EWheelSida::Undefined;

	/** Additional offset to give the wheels for this axle */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	FVector AdditionalOffset;

	/** Initially wheel position [Cm]*/
	UPROPERTY(BlueprintReadOnly, Category = WheelSetup)
	FVector RestingLocation;

	/** Initially wheel rotation [Deg]*/
	UPROPERTY(BlueprintReadOnly, Category = WheelSetup)
	FRotator RestingRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bVerboseLog = false;

public:
	/** Requested wheel torque [H/m] */
	UPROPERTY(Category = VehicleWheel, BlueprintReadWrite)
	float ReqTorq = 0;		  

	/** Requested wheel brake torque [H/m] */
	UPROPERTY(Category = VehicleWheel, BlueprintReadWrite)
	float ReqBrakeTorque = 0; 

	/** Requested wheel steering  [Rad] */
	UPROPERTY(Category = VehicleWheel, BlueprintReadWrite)
	float ReqSteer = 0;		

public:
	/** Current wheel steering [Rad] */
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	float Steer = 0;		

    /** [Rad] */
	UPROPERTY(BlueprintReadOnly, Category = BlueprintReadOnly)
	float Pitch = 0;

	/** Current wheel angular velocity  [Rad/s] */
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	float AngularVelocity = 0; 

	/** Current longitude & lateral wheel slip */
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	FVector2D Slip;

	/** Current suspension z - offset in [cm] */
	//UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	//float SuspensionOffset = 0;

	/** Current relative location */
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	FVector SuspensionOffset2;

public:
	/** Wheel radius [Cm]*/
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	float Radius = 0;


public:
	/** Compute wheel postion in local or global space. */
	virtual FVector GetWheelLocation(bool bWithSuspensionOffset = true, bool bInWorldSpace = false) const;

	/** Compute vecloty of the  wheel center in the wheel bone coordinate system. */
	virtual FVector GetWheelLocalVelocity() const;

	inline float GetLinearVelocity() const { return Radius * AngularVelocity; }

	UFUNCTION(BlueprintCallable, Category = VehicleWhee)
	inline EWheelIndex GetWheelIndex() const { return (EWheelIndex)((((uint8)WheelChassis & WHEEL_CHASSIS_BIT_MASK) << WHEEL_CHASSIS_BIT_OFFSET) | ((uint8)WheelSida & WHEEL_SIDE_BIT_MASK)); }


public:
	/* Override ITorqueTransmission*/
	virtual void PassTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override;
	virtual bool FindWheelRadius(float& OutRadius) const override { OutRadius = Radius; return true; }
	virtual bool FindToWheelRatio(float& OutRatio) const override { OutRatio = 1.0; return true; }

public:
	//virtual void OnRegistreVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
};
