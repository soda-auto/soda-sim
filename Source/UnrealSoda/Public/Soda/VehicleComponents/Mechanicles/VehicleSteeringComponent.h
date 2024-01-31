// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Curves/CurveFloat.h"
#include "VehicleSteeringComponent.generated.h"

/**
 * UVehicleSteeringRackComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleSteeringRackBaseComponent  : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

	/** [Rad] */
	UFUNCTION(BlueprintCallable, Category = SteeringRack, meta = (ScenarioAction))
	virtual void RequestByAngle(float InAngle) { }

	/** [-1..1] */
	UFUNCTION(BlueprintCallable, Category = SteeringRack, meta = (ScenarioAction))
	virtual void RequestByRatio(float InRatio) { RequestByAngle(GetMaxSteer() * InRatio); }

	/** [Rad] */
	UFUNCTION(BlueprintCallable, Category = SteeringRack)
	virtual float GetCurrentSteer() const { return 0; }

	/** [Rad] */
	UFUNCTION(BlueprintCallable, Category = SteeringRack)
	virtual float GetMaxSteer() const { return 0; }
};

/**
 * UVehicleSteeringRackSimpleComponent
*/

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleSteeringRackSimpleComponent : public UVehicleSteeringRackBaseComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SteeringRack, SaveGame, meta = (EditInRuntime))
	float MaxSteerAngle = 30;

	/** Steering speed [deg/s] versus forward speed [km/h] */
	UPROPERTY(EditAnywhere, Category = SteeringRack)
	FRuntimeFloatCurve SteeringCurve;

	/** Allow set brake from default the UVhicleInputComponent */
	UPROPERTY(EditAnywhere, Category = BrakeSystem, SaveGame, meta = (EditInRuntime))
	bool bAcceptSteerFromVehicleInput = true;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual void RequestByAngle(float InAngle) override { TargetSteerAng = InAngle; }
	virtual float GetCurrentSteer() const override { return CurrentSteerAng; }
	virtual float GetMaxSteer() const override { return MaxSteerAngle / 180 * M_PI; }

public:
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

	virtual void UpdateSteer(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

protected:
	float CurrentSteerAng = 0;
	float TargetSteerAng = 0;
	float SteerInputRatio = 0;
};



