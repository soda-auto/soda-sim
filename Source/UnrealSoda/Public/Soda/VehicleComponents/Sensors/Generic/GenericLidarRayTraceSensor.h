// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarRayTraceSensor.h"
#include "Soda/VehicleComponents/GenericPublishers/GenericLidarPublisher.h"
#include "GenericLidarRayTraceSensor.generated.h"



UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericLidarRayTraceSensor : public ULidarRayTraceSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericLidarPublisher> PublisherClass;

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
	float DistanseMin = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMax = 10000;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;

protected:
	virtual float GetFOVHorizontMax() const override { return FOV_HorizontMax; }
	virtual float GetFOVHorizontMin() const override { return FOV_HorizontMin; }
	virtual float GetFOVVerticalMax() const override { return FOV_VerticalMax; }
	virtual float GetFOVVerticalMin() const override { return FOV_VerticalMin; }
	virtual float GetLidarMinDistance() const override { return DistanseMin; }
	virtual float GetLidarMaxDistance() const override { return DistanseMax; }
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

	FGenericPublisherHelper<UGenericLidarRayTraceSensor, UGenericLidarPublisher> PublisherHelper{ this, &UGenericLidarRayTraceSensor::PublisherClass, &UGenericLidarRayTraceSensor::Publisher };

};
