// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/RadarBaseSensor.h"
#include "Soda/Transport/GenericRadarPublisher.h"
#include "RadarGenericSensor.generated.h"

/**
 * UGeneralRadarSensorComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGeneralRadarSensorComponent : public URadarBaseSensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Generic radar UDP publisher*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericRadarPublisher PointCloudPublisher;

	/** Parametres of radar near beam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TArray<FRadarParams> RadarParams;

	UGeneralRadarSensorComponent();

protected:
	virtual bool OnActivateVehicleComponent() override; 
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override;

	virtual ERadarMode GetRadarMode() const override { return ERadarMode::ClusterMode; }

protected:
	virtual void PublishResults() override;
	virtual const TArray<FRadarParams>& GetRadarParams() const override { return RadarParams; }

protected:
	soda::RadarScan Scan;
};
