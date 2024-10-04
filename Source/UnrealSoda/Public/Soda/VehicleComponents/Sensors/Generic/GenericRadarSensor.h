// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/RadarSensor.h"
#include "Soda/GenericPublishers/GenericRadarPublisher.h"
#include "GenericRadarSensor.generated.h"

/**
 * UGeneralRadarSensorComponent
 */

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericRadarSensor : public URadarSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericRadarPublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericRadarPublisher> Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TArray<FRadarParams> RadarParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	ERadarMode RadarMode = ERadarMode::ClusterMode;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual FString GetRemark() const override;

protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FRadarClusters& Clusters, const FRadarObjects& Objects) override;
	virtual ERadarMode GetRadarMode() const override { return RadarMode; }
	virtual const TArray<FRadarParams>& GetRadarParams() const override { return RadarParams; }

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

	FGenericPublisherHelper<UGenericRadarSensor, UGenericRadarPublisher> PublisherHelper{ this, &UGenericRadarSensor::PublisherClass, &UGenericRadarSensor::Publisher };
};
