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

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericRadarPublisher> Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TArray<FRadarParams> RadarParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	ERadarMode RadarMode = ERadarMode::ClusterMode;

protected:
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
