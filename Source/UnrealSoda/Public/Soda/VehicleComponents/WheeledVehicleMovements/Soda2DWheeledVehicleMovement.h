// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Soda/VehicleComponents/WheeledVehicleMovementBaseComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/Misc/TelemetryGraph.h"
#include "Soda/Misc/GroundScaner.h"
#include "Soda/Misc/PrecisionTimer.hpp"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include <thread>
#include <mutex>

#include "Soda2DWheeledVehicleMovement.generated.h"

class DynamicCar;


/**
 * The USoda2DWheeledVehicleMovementComponent provides a physical vehicle simulation in 2D space.
 * The physics model is calculated only for two wheels (front and rear), but then transferred to the 4-wheel model.
 * Also USoda2DWheeledVehicleMovementComponent simulates a simple four-wheel suspension for the bicycle model.
 */
UCLASS(ClassGroup = Soda,BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API USoda2DWheeledVehicleMovementComponent : public UWheeledVehicleMovementBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Offset from center of the mesh to rear wheel, [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector RearWheelOffset{};

	/** Vehicle mass in kg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float Mass = 1196.0;

	/** Moment of inertia by Z axis for car [kg * m^2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float MomentOfInertia = 1260.0f;

	/** Track Width [cm]*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float TrackWidth = 200;

	/** Distance from CoG to forward wheel [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (DisplayName="CoG to Forward Wheel", EditInRuntime, ReactivateComponent))
	float CoGToForwardWheel = 170;

	/** Distance from CoG to rear wheel [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (DisplayName = "CoG to Rear Wheel", EditInRuntime, ReactivateComponent))
	float CoGToRearWheel = 116.8;

	/** High of CoG [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (DisplayName = "CoG Vert", EditInRuntime, ReactivateComponent))
	float CoGVert = 40;

	/** Friction coefficient */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float Friction = 1.0;

	/** Wheels radius [cm]*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FrontWheelRadius = 31;

	/** Wheels radius [cm]*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float RearWheelRadius = 35;

	/** Rear wheels stiffness of tyres in lateral direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float RearWheelLatStiff = 240000.0;

	/** Forward wheels stiffness of tyres in lateral direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float ForwardWheelLatStiff = 226000.0;

	/** Rear wheels stiffness of tyres in longitudal direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float RearWheelLonStiff = 284000.0;

	/** Forward wheels stiffness of tyres in longitudal direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float ForwardWheelLonStiff = 376000.0;

	/** Rolling resistance [N] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DragRollingResistance = 600.0;

	/** Rolling resistance [N*s/m] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DragRollingResistanceSpeedDepended = 0.0;

	/** Coefficient of aero drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DragCxx = 1.0;

	/** Moment of inertia of front train [kg * m^2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float MomentOfInertiaFrontTrain = 1.42;

	/** Moment of inertia of rear train [kg * m^2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float MomentOfInertiaRearTrain = 2.06;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bPullToGround = true;

	/** From center of promotive [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float PullToGroundOffset = 30;

	/** Experemental featuer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bEnableCollisions = false;

	/** Time of integration in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int IntegrationTimeStep = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	double SpeedFactor = 1.0;

	/** If 1 then drag forces being taked into account*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int FlDragTaking = 1;

	/** 0 || 1 || 2 ; 2 is default value (combined slip mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int FlLongCirc = 3;

	/** 1 for static case of loads on rear and forward wheel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int FlStaticLoad = 0;

	/** 1 for Runge Kutta integration; 0 for Local Grid Refinement (more expensive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int FlRunKut = 0;

	/** Sampling for integration on [t,t+dt] interval */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int NSteps = 10;

	/** Lawes for actuators */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int FlActLow = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int ImplicitSolver = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int NumImpLonlinIt = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CalculationSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int CorrImplStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bLogPhysStemp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vehicle, SaveGame, meta = (EditInRuntime))
	float ModelStartUpDelay = 0.5;

public:
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	bool SetVehicleVelocity(float InVelocity);

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual void UpdateSimulation(const std::chrono::nanoseconds& Deltatime, const std::chrono::nanoseconds& Elapsed);
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	/* Overrides from  ISodaVehicleComponent  */
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPreActivateVehicleComponent() override;
	virtual void OnPreDeactivateVehicleComponent() override;

public:
	/* Overrides from  IWheeledVehicleMovementInterface  */
	virtual float GetVehicleMass() const override { return Mass; }
	virtual const FVehicleSimData& GetSimData() const override { return VehicleSimData; }
	virtual bool SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation) override;

protected:
	FVehicleSimData VehicleSimData;
	std::mutex Mutex;
	TSharedPtr<DynamicCar> DynCar;
	FPrecisionTimer PrecisionTimer;
	bool bSynchronousMode = false;
	FVector CoF;
	float ZOffset;
};