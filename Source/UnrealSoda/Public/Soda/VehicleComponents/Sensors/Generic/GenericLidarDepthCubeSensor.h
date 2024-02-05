// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarDepthCubeSensor.h"
#include "Soda/VehicleComponents/GenericPublishers/GenericLidarPublisher.h"
#include "Soda/VehicleComponents/GenericPublishers/GenericCameraPublisher.h"
#include "GenericLidarDepthCubeSensor.generated.h"


UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericLidarDepthCubeSensor : public ULidarDepthCubeSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericLidarPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericLidarPublisher> Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericCameraPublisher> CameraPublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericCameraPublisher> CameraPublisher;

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
	float DistanseMin = 50; // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMax = 20000; // cm

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
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
	virtual float GetLidarMinDistance() const override { return DistanseMin; }
	virtual float GetLidarMaxDistance() const override { return DistanseMax; }
	virtual int GetLidarNumLayers() const { return Channels; }
	virtual const TArray<FVector>& GetLidarRays() const override { return LidarRays; }
	virtual void PostProcessSensorData(soda::FLidarScan& Scan) override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

protected:
	TArray<FVector> LidarRays;

	FGenericPublisherHelper<UGenericLidarDepthCubeSensor, UGenericLidarPublisher> PublisherHelper{ this, &UGenericLidarDepthCubeSensor::PublisherClass, &UGenericLidarDepthCubeSensor::Publisher };
	FGenericPublisherHelper<UGenericLidarDepthCubeSensor, UGenericCameraPublisher> CameraPublisherHelper{ this, &UGenericLidarDepthCubeSensor::CameraPublisherClass, &UGenericLidarDepthCubeSensor::CameraPublisher, "PublisherClass", "CameraPublisher", "CameraPublisherRecord" };
};
