// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/Transport/GenericLidarPublisher.h"
#include "LidarRayTraceSensor.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarRayTraceSensorComponent : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericLidarPublisher PointCloudPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float Period = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	int Channels = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	int Step = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float FOV_HorizontMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float FOV_HorizontMax = 360;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float FOV_VerticalMin = -15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float FOV_VerticalMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float DistanseMin = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float DistanseMax = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float AbsorptionCoefficient = 0.005;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float ReflectionCoefficient = 0.8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float LaserIntensity = 100000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float SensorSize = 0.01;

	//UPROPERTY(EditAnywhere, Category = Sensor)
	//float SeaLevel = 10140.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool DrawTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool DrawNotTracedRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	TArray< AActor* > IgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool EnabledGroundFilter = false;

	/** Distance to ground (cm)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float DistanceToGround = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool PublishAbsoluteCoordinates = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString & Info) const override;
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

public:
	ULidarRayTraceSensorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


protected:
	std::vector<soda::LidarScanPoint> Points;

	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};
