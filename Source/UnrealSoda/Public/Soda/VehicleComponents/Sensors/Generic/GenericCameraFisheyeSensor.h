// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/CameraFisheyeSensor.h"
#include "Soda/VehicleComponents/GenericPublishers/GenericCameraPublisher.h"
#include "GenericCameraFisheyeSensor.generated.h"

/**
 * UGenericCameraFisheyeSensor
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericCameraFisheyeSensor : public UCameraFisheyeSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericCameraPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericCameraPublisher> Publisher;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& CameraFrame, const TArray<FColor>& BGRA8, uint32 ImageStride) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

	FGenericPublisherHelper<UGenericCameraFisheyeSensor, UGenericCameraPublisher> PublisherHelper{ this, &UGenericCameraFisheyeSensor::PublisherClass, &UGenericCameraFisheyeSensor::Publisher };
};
