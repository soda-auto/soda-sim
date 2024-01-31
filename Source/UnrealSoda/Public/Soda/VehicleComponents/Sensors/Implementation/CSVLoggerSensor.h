// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/ImuGnssSensor.h"
#include <fstream>
#include "CSVLoggerSensor.generated.h"


/**
 * UCSVLoggerSensorComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCSVLoggerSensorComponent : public UImuGnssSensor
{
	GENERATED_UCLASS_BODY()

public:
	/** File path for bSavePath. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString CSVPath = "csv_logger.csv";

protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	std::ofstream OutFile;
};
