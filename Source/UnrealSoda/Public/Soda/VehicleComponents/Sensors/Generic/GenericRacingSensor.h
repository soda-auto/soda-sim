// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/RacingSensor.h"
#include "Soda/GenericPublishers/GenericRacingPublisher.h"
#include "GenericRacingSensor.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericRacingSensor : public URacingSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericRacingPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericRacingPublisher> Publisher;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& SensorData) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

	FGenericPublisherHelper<UGenericRacingSensor, UGenericRacingPublisher> PublisherHelper{ this, &UGenericRacingSensor::PublisherClass, &UGenericRacingSensor::Publisher };

};

