// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarDepthCubeSensor.h"
#include "Soda/GenericPublishers/GenericLidarPublisher.h"
#include "Soda/GenericPublishers/GenericCameraPublisher.h"
#include "GenericLidarDepthCubeSensor.generated.h"


UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericLidarDepthCubeSensor : public ULidarDepthCubeSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericLidarPublisher> Publisher;

	//UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TObjectPtr<UGenericCameraPublisher> CameraPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bPublishDepthMap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Channels = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Step = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontMin = -32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontMax = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMin = -15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanceMin = 50; // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanceMax = 20000; // cm

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
