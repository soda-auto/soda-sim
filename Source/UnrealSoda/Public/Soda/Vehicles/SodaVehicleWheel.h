// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SodaVehicleWheel.generated.h"

/**
 * Wheeled vehicle wheel indexes
 */
UENUM(BlueprintType)
enum class  E4WDWheelIndex: uint8
{
	/** Front left */
	FL = 0,

	/** Front right */
	FR = 1,

	/** Rear left */
	RL = 2,

	/** Rear right */
	RR = 3,

	/** Current vehicle isn't 4WD */
	None = 0xFF,
};


/**
 * USodaVehicleWheelComponent object
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))   
class UNREALSODA_API USodaVehicleWheelComponent 
	: public UWheeledVehicleComponent
	, public ITorqueTransmission
{
	GENERATED_UCLASS_BODY()

public:
	/** Bone name on mesh to create wheel at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup)
	FName BoneName = NAME_None;

	/** if current vehicle is 4WD, this is index of current wheel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup)
	E4WDWheelIndex WheelIndex4WD = E4WDWheelIndex::None;

	/** Additional offset to give the wheels for this axle */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup)
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
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	float SuspensionOffset = 0;

public:
	/** Wheel radius [Cm]*/
	UPROPERTY(Category = VehicleWheel, BlueprintReadOnly)
	float Radius = 0;


public:
	/** Compute wheel postion in local or global space. */
	virtual FVector GetWheelLocation(bool bWithSuspensionOffset = true, bool bInWorldSpace = false) const;

	/** Compute vecloty of the  wheel center in the wheel bone coordinate system. */
	virtual FVector GetWheelLocalVelocity() const;


public:
	/* Override ITorqueTransmission*/
	virtual void PassTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override;
	virtual float FindWheelRadius() const override { return Radius; }
	virtual float FindToWheelRatio() const override { return 1.0; }

public:
	//virtual void OnRegistreVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
};
