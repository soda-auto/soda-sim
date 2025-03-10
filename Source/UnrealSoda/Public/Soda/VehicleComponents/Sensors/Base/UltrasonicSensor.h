// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "UltrasonicSensor.generated.h"

class UUltrasonicHubSensor;

 /**
  * FUltrasonicEcho
  */
USTRUCT(BlueprintType)
struct UNREALSODA_API FUltrasonicEcho
{
	GENERATED_BODY()

	inline static bool DistancePredicate(const FUltrasonicEcho& Eh1, const FUltrasonicEcho& Eh2)
	{
		return (Eh1.BeginDistance < Eh2.BeginDistance);
	}

	inline static bool ReturnPowerPredicate(const FUltrasonicEcho& Eh1, const FUltrasonicEcho& Eh2)
	{
		return (Eh1.ReturnPower > Eh2.ReturnPower);
	}

	/** Echo begin distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	float BeginDistance;

	/** Echo end distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	float EndDistance;

	/** Echo total return signal power */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	float ReturnPower;

	/** Echo position in UE worldspace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	FVector EchoPosition;

	/** Echo hits array */
	TArray<FHitResult*> Hits;
};

/**
 * FUltrasonicEchos
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FUltrasonicEchos
{
	GENERATED_BODY()

	/** Maximun echos number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	int EchosMaxNum = 3;

	/** Echos array */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	TArray<FUltrasonicEcho> Echos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	bool bIsTransmitter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	float FOV_Horizont = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	float FOV_Vertical = 0;

	float CosFovVertical = 0;
	float CosFovHorizontal = 0;

	void Clear() { Echos.Empty(); }
	void AddHit(FHitResult* HitResult, float Distance, float Power, float MinDistGap);
	void RemoveExcessEchos();
};

/**
 * UUltrasonicSensor
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UUltrasonicSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	int Rows = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	int Step = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering, UpdateFOVRendering))
	float FOV_Horizont = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering, UpdateFOVRendering))
	float FOV_Vertical = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering, UpdateFOVRendering))
	float DistanceMin = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering, UpdateFOVRendering))
	float DistanceMax = 300;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

};

/**
 * UUltrasonicSensorHubComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UUltrasonicHubSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float SensorThreshold = 0.3f;

	/** Echo distance min gap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float MinDistGap = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bEnabledGroundFilter = false;

	/** Distance to ground (cm)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float DistanceToGround = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawNotTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogEchos = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawEchos = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& InEchoCollections) { SyncDataset(); return true; }

protected:
	TInlineComponentArray<UUltrasonicSensor*> Sensors;
	TArray < FUltrasonicEchos > EchoCollections;
	int CurrentTransmitter = 0;

	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};




