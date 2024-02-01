// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
#include "VehicleEngineGasComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleEngineGasComponent : public UVehicleEngineBaseComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	/** Torque [N*m] at a given RPM */
	UPROPERTY(EditAnywhere, Category = VehicleEngine)
	FRuntimeFloatCurve TorqueCurve;

	/** Muliplayer for TorqueCurve. Result engine torque = TorqueCurve(RPM) * TorqueCurveMultiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine, SaveGame, meta = (EditInRuntime))
	float TorqueCurveMultiplier = 1.f;

	/** Allow set engine torque from default the UVhicleInputComponent */
	UPROPERTY(EditAnywhere, Category = VehicleEngine, SaveGame, meta = (EditInRuntime))
	bool bAcceptPedalFromVehicleInput = true;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual void RequestByTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override { return AngularVelocity; }
	virtual float GetMaxTorque() const override { return MaxTorq; }
	virtual float GetTorque() const override { return ActualTorque; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

public:
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	virtual void PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

protected:
	float AngularVelocity = 0.f;
	float MaxTorq = 0.f;
	float RequestedTorque = 0.f;
	float ActualTorque = 0.0f;
	float PedalPos = 0;

	float FrictionTorque = 0.f;
	float FrictionCoeff = 0.02f;
	float StartFriction = 50.0f; //experimental recieved value when engine didnt work
	float EffectiveTorque = 0.0f;
	float EngineInertia = 0.0f;

};
