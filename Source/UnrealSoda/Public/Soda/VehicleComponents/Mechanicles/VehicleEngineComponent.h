// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleEngineComponent.generated.h"

/**
 * UVehicleEngineBaseComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleEngineBaseComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** 
	 * Name of the vehicle component to which need to connect the engine shaft (wheel, gearbox, transmission, etc).
	 * Default wheels name: WheelFL, WheelFR, WheelRL, WheelRR.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "TorqueTransmission"))
	FSubobjectReference LinkToTorqueTransmission { TEXT("Differential") };


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Engine, SaveGame, meta = (EditInRuntime))
	float Ratio = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bVerboseLog = false;

public:
	UPROPERTY(BlueprintReadOnly, Category = Engin)
	TScriptInterface<ITorqueTransmission> OutputTorqueTransmission;

public:
	/** Request torque [N*m] for engine */
	UFUNCTION(BlueprintCallable, Category = Engine, meta = (ScenarioAction))
	virtual void RequestByTorque(float InTorque) {}

	UFUNCTION(BlueprintCallable, Category = Engine, meta = (ScenarioAction))
	virtual void RequestByRatio(float InRatio) { RequestByTorque(GetMaxTorque() * InRatio); }

	/** Get current angular velocity [Rad/s] on the shaft  */
	UFUNCTION(BlueprintCallable, Category = Engine)
	virtual float ResolveAngularVelocity() const  { return 0; }

	/** Possible maximum possible engine torque [N*m] for current RPM. */
	UFUNCTION(BlueprintCallable, Category = Engine)
	virtual float GetMaxTorque() const { return 0; }

	/** Get current engine torque [N*m] */
	UFUNCTION(BlueprintCallable, Category = Engine)
	virtual float GetTorque() const { return 0; }

	/** [0...1] */
	virtual float GetEngineLoad() const;

	/** Try to find wheel(s) radius to which set this torque [cm] */
	virtual bool FindWheelRadius(float& OutRadius) const;

	/** Try to find ratio beetwen this transmission and connected wheel(s) */
	virtual bool FindToWheelRatio(float& OutRatio) const;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
};

/**
 * UVehicleEngineSimpleComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleEngineSimpleComponent : public UVehicleEngineBaseComponent
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

	UPROPERTY(EditAnywhere, Category = VehicleEngine, SaveGame, meta = (EditInRuntime))
	bool bFlipAngularVelocity = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual void RequestByTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override { return AngularVelocity; }
	virtual float GetMaxTorque() const override { return MaxTorq; }
	virtual float GetTorque() const override { return ActualTorque; }

	float GetAngularVelocity() const { return AngularVelocity; }
	float GetRequestedTorque() const { return RequestedTorque; }
	float GetPedalPos() const { return PedalPos; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

public:
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp) override;
	virtual void PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

protected:
	float AngularVelocity = 0.f;
	float MaxTorq = 0.f;
	float RequestedTorque = 0.f;
	float ActualTorque = 0.0f;
	float PedalPos = 0;
};


