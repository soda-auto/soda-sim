// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarRayTraceSensor.h"
#include "Soda/GenericPublishers/GenericLidarPublisher.h"
#include "GenericLidarRayTraceSensor.generated.h"



UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericLidarRayTraceSensor : public ULidarRayTraceSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericLidarPublisher> Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, ReactivateComponent))
	int Channels = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, ReactivateComponent))
	int Step = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontMax = 360;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMin = -15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanceMin = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanceMax = 10000;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;

protected:
	virtual float GetFOVHorizontMax() const override { return FOV_HorizontMax; }
	virtual float GetFOVHorizontMin() const override { return FOV_HorizontMin; }
	virtual float GetFOVVerticalMax() const override { return FOV_VerticalMax; }
	virtual float GetFOVVerticalMin() const override { return FOV_VerticalMin; }
	virtual float GetLidarMinDistance() const override { return DistanceMin; }
	virtual float GetLidarMaxDistance() const override { return DistanceMax; }
	virtual TOptional<FUintVector2> GetLidarSize() const { return TOptional<FUintVector2>({ uint32(Step), uint32(Channels) }); }
	virtual const TArray<FVector>& GetLidarRays() const override { return LidarRays; }
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarSensorData& Scan) override;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	TArray<FVector> LidarRays;
};
