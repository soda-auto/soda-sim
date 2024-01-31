// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarSensor.h"
#include "LidarRayTraceSensor.generated.h"

UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarRayTraceSensor : public ULidarSensor
{
	GENERATED_UCLASS_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	//float AbsorptionCoefficient = 0.005;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	//float ReflectionCoefficient = 0.8;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	//float LaserIntensity = 100000;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	//float SensorSize = 0.01;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//TArray< AActor* > IgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bEnabledGroundFilter = false;

	/** Distance to ground [cm] for bEnabledGroundFilter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float DistanceToGround = 0;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;


public:
	//ULidarRayTraceSensorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	soda::FLidarScan Scan;
	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};
