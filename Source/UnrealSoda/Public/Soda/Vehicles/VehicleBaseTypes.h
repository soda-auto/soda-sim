// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Misc/Time.h"
#include "VehicleBaseTypes.generated.h"


/**
 * Gears for electric vehicles
 */
UENUM(BlueprintType)
enum class EGearState : uint8
{
	Neutral = 0,
	Drive = 1,
	Reverse = 2,
	Park = 3,
};

UENUM(BlueprintType)
enum class  EVehicleInputType : uint8
{
	Other = 0,
	Keyboard = 1,
	Joy = 2,
	External = 3,
	AI = 4
};


/**
 * Helper structure that allows to change any value of a variable over time depending on speed (RiseRate and FallRate)
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FInputRate
{
	GENERATED_USTRUCT_BODY()

	/** Rate at which the input value rises */
	UPROPERTY(EditAnywhere, Category = VehicleInputRate, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float RiseRate;

	/** Rate at which the input value falls */
	UPROPERTY(EditAnywhere, Category = VehicleInputRate, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float FallRate;

	FInputRate() : RiseRate(5.0f), FallRate(5.0f) {}
	FInputRate(float RiseRate, float FallRate) : RiseRate(RiseRate), FallRate(FallRate) {}

	/** Change an output value using max rise and fall rates */
	float InterpInputValue(float DeltaTime, float CurrentValue, float NewValue) const
	{
		const float DeltaValue = NewValue - CurrentValue;

		// We are "rising" when DeltaValue has the same sign as CurrentValue (i.e. delta causes an absolute magnitude gain)
		// OR we were at 0 before, and our delta is no longer 0.
		const bool bRising = ((DeltaValue > 0.0f) == (CurrentValue > 0.0f)) ||
							 ((DeltaValue != 0.f) && (CurrentValue == 0.f));

		const float MaxDeltaValue = DeltaTime * (bRising ? RiseRate : FallRate);
		const float ClampedDeltaValue = FMath::Clamp(DeltaValue, -MaxDeltaValue, MaxDeltaValue);
		return CurrentValue + ClampedDeltaValue;
	}
};


UENUM(BlueprintType)
enum class EVehicleComponentPrePhysTickGroup : uint8
{
	/** Reserved */
	TickGroup0 = 0,

	/** Recomended for the drive control simulation */
	TickGroup1,

	/** Reserved */
	TickGroup2,

	/** Recomended for the motors simulation  */
	TickGroup3,

	/** Reserved */
	TickGroup4,

	/** Recomended for the gear box simulation */
	TickGroup5,

	/** Reserved */
	TickGroup6,

	/** Recomended for the brake, steering, differential  simulation  */
	TickGroup7,

	/** Reserved */
	TickGroup8
};

UENUM(BlueprintType)
enum class EVehicleComponentPostPhysTickGroup : uint8
{
	/** Reserved */
	TickGroup0 = 0,

	/** Recomended for the brake, steering, differential  simulation  */
	TickGroup1,

	/** Reserved */
	TickGroup2,

	/** Recomended for the gear box simulation */
	TickGroup3,

	/** Reserved */
	TickGroup4,

	/** Recomended for the motors simulation  */
	TickGroup5,

	/** Reserved */
	TickGroup6,

	/** Recomended for the drive control simulation */
	TickGroup7,

	/** Reserved */
	TickGroup8
};

UENUM(BlueprintType)
enum class EVehicleComponentPostDeferredPhysTickGroup : uint8
{
	TickGroup0 = 0,
	TickGroup1,
	TickGroup2,
	TickGroup3,
	TickGroup4,
	TickGroup5,
	TickGroup6,
	TickGroup7,
	TickGroup8
};

UINTERFACE(Blueprintable)
class UNREALSODA_API UTorqueTransmission : public UInterface
{
	GENERATED_BODY()
};

class UNREALSODA_API ITorqueTransmission
{
	GENERATED_BODY()
public:
	/** [N/m] */
	virtual void PassTorque(float InTorque) = 0;

	/** [Rad/s] */
	virtual float ResolveAngularVelocity() const = 0;

	/** Try to find wheel(s) radius to which set this torque [cm] */
	virtual bool FindWheelRadius(float & OutRadius) const = 0;

	/** Try to find ratio beetwen this transmission and connected wheel(s) */
	virtual bool FindToWheelRatio(float & OutRatio) const = 0;
};

/*
* 
USTRUCT(BlueprintType)
struct FWheeledVehicleWheelState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	float AngularVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	float Torq;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	float BrakeTorq;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	float Steer;
};

USTRUCT(BlueprintType)
struct FWheeledVehicleState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheeledVehicleState)
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FVector AngularVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FVector Acc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FWheeledVehicleWheelState WheelsFL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FWheeledVehicleWheelState WheelsFR;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FWheeledVehicleWheelState WheelsRL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleInputRate)
	FWheeledVehicleWheelState WheelsRR;
};
*/

struct FVehicleSimData
{
	/** Actual Vehicle kinematic structure. */
	FPhysBodyKinematic VehicleKinematic;

	/** Actual simulation step number. */
	int SimulatedStep = 0;

	/** SimulatedStep for current game frame. */
	int RenderStep = 0;

	/** Actual simulation unix time stemp for current game frame. */
	TTimestamp SimulatedTimestamp{};

	/** SimulatedTimestamp for current game frame. */
	TTimestamp RenderTimestamp{};

	static FVehicleSimData Zero;
};


inline float GearToRatio(EGearState Gear)
{
	static float GearRatios[] = { 0, 1, -1, 0 };
	check((uint8)Gear < 5);
	return GearRatios[int(Gear)];
}


