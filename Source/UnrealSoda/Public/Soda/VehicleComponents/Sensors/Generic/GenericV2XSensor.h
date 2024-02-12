// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/V2XSensor.h"
#include "Soda/GenericPublishers/GenericV2XPublisher.h"
#include "GenericV2XSensor.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericV2XReceiverSensor: public UV2XReceiverSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericV2XPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericV2XPublisher> Publisher;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& InTransmitters) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

	FGenericPublisherHelper<UGenericV2XReceiverSensor, UGenericV2XPublisher> PublisherHelper{ this, &UGenericV2XReceiverSensor::PublisherClass, &UGenericV2XReceiverSensor::Publisher };

};

