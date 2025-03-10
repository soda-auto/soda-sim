// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/UltrasonicSensor.h"
#include "Soda/GenericPublishers/GenericUltrasoncPublisher.h"
#include "GenericUltrasonicSensor.generated.h"


UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericUltrasonicHubSensor : public UUltrasonicHubSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericUltrasoncHubPublisher> Publisher;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& InEchoCollections) override;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
