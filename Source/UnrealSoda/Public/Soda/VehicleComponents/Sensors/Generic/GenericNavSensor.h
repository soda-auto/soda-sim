// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"
#include "Soda/GenericPublishers/GenericNavPublisher.h"
#include "GenericNavSensor.generated.h"


/**
 * UGenericNavSensor
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericNavSensor : public UNavSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericNavPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericNavPublisher> Publisher;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

	FGenericPublisherHelper<UGenericNavSensor, UGenericNavPublisher> PublisherHelper{ this, &UGenericNavSensor::PublisherClass, &UGenericNavSensor::Publisher };

};
