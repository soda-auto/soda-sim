// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/SodaVehicleWheel.h"
#include "SimpleVehicleDriverComponent.generated.h"

class UVehicleBrakeSystemBaseComponent;
class UVehicleEngineBaseComponent;
class UVehicleSteeringRackBaseComponent;
class UVehicleGearBoxBaseComponent;

USTRUCT(BlueprintType)
struct FRover6WDDriverWheelData
{
	GENERATED_USTRUCT_BODY();

	//FSubobjectReference LinkToEngineFL { TEXT("Engine") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	EWheelIndex WheelIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bApplySteer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bInversSteer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bApplyTorq = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bApplyBrakeTorq = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	float TorqDistribution = 0.25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	float BrakeTorqDistribution = 0.25;


	UPROPERTY()
	USodaVehicleWheelComponent* Wheel = nullptr;
};

/**
 * A simple implementation for the VehicleDriver that directly controls each wheel without motors, braking systems, transmission etc.
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API USimpleVehicleDriverComponent : public UVehicleDriverComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = SimpleVehicleDriver, BlueprintReadOnly, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TArray<FRover6WDDriverWheelData> WheelsData;

	/** Max cumulative torque to all wheels  [N/m] */
	UPROPERTY(EditAnywhere, Category = SimpleVehicleDriver, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float MaxTorqReq = 2000;

	/** Max cumulative torque to all wheels  [N/m] */
	UPROPERTY(EditAnywhere, Category = SimpleVehicleDriver, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float MaxBrakeTorqReq = 2000;

	/** [deg] */
	UPROPERTY(EditAnywhere, Category = SimpleVehicleDriver, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float MaxSteer = 30;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TSubclassOf<UGenericWheeledVehiclePublisher> PublisherClass;

	//UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TObjectPtr<UGenericWheeledVehiclePublisher> Publisher;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TSubclassOf<UGenericWheeledVehicleControlListener> VehicleControlClass;

	//UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TObjectPtr<UGenericWheeledVehicleControlListener> VehicleControl;


	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineFL { TEXT("EngineFL") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineFR { TEXT("EngineFR") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineML { TEXT("EngineML") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineMR { TEXT("EngineMR") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineRL { TEXT("EngineRL") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngineRR { TEXT("EngineRR") };


public:
	//UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	//UVehicleBrakeSystemBaseComponent * BrakeSystem = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	//UVehicleEngineBaseComponent * Engine = nullptr;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float EngineToWheelsRatio = 1.f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float VehicleWheelRadius = 35.0f;

	/** [cm/s] */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float TargetSpeedDelta = 10;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnPostActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	//virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	//virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

public:
	virtual EGearState GetGearState() const override { return EGearState::Neutral; }
	virtual bool IsADPing() const override { return false; }
	virtual ESodaVehicleDriveMode GetDriveMode() const override { return ESodaVehicleDriveMode::Manual; }

	//virtual bool IsEnabledLeftTurningLights() const { return false; }
	//virtual bool IsEnabledRightTurningLights() const { return false; }
	//virtual bool IsEnabledBrakeLight() const;
	//virtual bool IsEnabledHeadlights() const { return false; }
	//virtual bool IsEnabledDaytimeRunningLights() const { return false; }
	//virtual bool IsEnabledReversLights() const;
	//virtual bool IsEnabledHorn() const { return false; }
	//virtual FColor GetLED() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

};
