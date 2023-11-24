// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Misc/Time.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "IWheeledVehicleMovementInterface.generated.h"

class ASodaWheeledVehicle;
class USodaVehicleWheelComponent;

/**
 * The IWheeledVehicleMovementInterface provides only tire and suspension physical simulation. 
 * It does not include simulation of braking and steering systems, diffs, transmissions, motors, etc.
 * The IWheeledVehicleMovementInterface must provide to call of PrePhysicSimulation, PostPhysicSimulation, VehicleIsReady delegates.
 * Also the IWheeledVehicleMovementInterface must perform requested values (ReqTorq, ReqBrakeTorque, ReqSteer) from USodaVehicleWheelComponent for each wheel 
 * and update valuse of VehicleKinematic and USodaVehicleWheelComponent.
 */

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UNREALSODA_API UWheeledVehicleMovementInterface : public UInterface
{
	GENERATED_BODY()
};

class UNREALSODA_API IWheeledVehicleMovementInterface
{
	GENERATED_BODY()

public:
	virtual ASodaWheeledVehicle* GetWheeledVehicle() const { check(0); return nullptr; }

	virtual float GetVehicleMass() const { check(0); return 0; }

	virtual const FVehicleSimData& GetSimData() const { check(0); return FVehicleSimData::Zero; }

	/**
	 * Must be called just before stimulation step.
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating the sub-steps).
	 */
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/**
	 * Must be called just after stimulation step .
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating the sub-steps).
	 */
	virtual void PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/**
	  * Must be called just after PostPhysicSimulation().
	  * This function call must correspond to real time.
	  */
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/** Set vehicle world location. */
	virtual bool SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation) { return false; }

	/** Compute distance between front left and rear left wheels. */
	virtual float GetWheelBaseWidth() const;

	/** Compute distance between front left and front right wheels. */
	virtual float GetTrackWidth() const;

protected:
	virtual void OnSetActiveMovement();

	//virtual bool StartSimulation() = 0;
	//virtual bool StopSimulation() = 0;
	//virtual bool IsSimulate() = 0;
	//virtual bool CanSimulate() = 0;
};
