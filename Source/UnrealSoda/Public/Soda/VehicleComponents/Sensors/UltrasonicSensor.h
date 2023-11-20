// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/Transport/GenericUltrasoncPublisher.h"
#include "UltrasonicSensor.generated.h"

class UUltrasonicSensorHubComponent;

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

	void Clear() { Echos.Empty(); }
	void AddHit(FHitResult* HitResult, float Distance, float Power, float MinDistGap);
	void RemoveExcessEchos();
};

/**
 * UUltrasonicSensorComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UUltrasonicSensorComponent : public USensorComponent
{
	friend UUltrasonicSensorHubComponent;

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
	float DistanseMin = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering, UpdateFOVRendering))
	float DistanseMax = 300;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

protected:
	float CosFovVertical = 0;
	float CosFovHorizontal = 0;
};

/**
 * UUltrasonicSensorHubComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UUltrasonicSensorHubComponent : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericUltrasoncPublisher PointCloudPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float SensorThreshold = 0.3f;

	/** Echo distance min gap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float MinDistGap = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool DrawTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool DrawNotTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool DrawEchos = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool EnabledGroundFilter = false;

	/** Distance to ground (cm)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float DistanceToGround = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogEchos = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override;

protected:
	TInlineComponentArray<UUltrasonicSensorComponent*> Sensors;
	TArray < FUltrasonicEchos > EchoCollections;
	int CurrentTransmitter = 0;
	soda::UltrasonicsHub Scan;

	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};




