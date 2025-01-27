// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Dom/JsonObject.h"
#include "Soda/Vehicles/ISodaVehicleExporter.h"

class UNavSensor;
class UCameraFisheyeSensor;
class UCameraSensor;
class USensorComponent;

class FSodaVehicleCommonExporter : public ISodaVehicleExporter
{
public:
	virtual bool ExportToString(const ASodaVehicle* Vehicle, FString& String) override;
	virtual const FName& GetExporterName() const override { return ExporterName; }
	virtual const FString& GetFileTypes() const override { return ExporterFileType; }

	static const FName ExporterName;
	static const FString ExporterFileType;

	static TSharedPtr<FJsonObject> GetImuIntrinsics(const UNavSensor* Sensor);
	static TSharedPtr<FJsonObject> GetFisheyeIntrinsics(const UCameraFisheyeSensor* Sensor);
	static TSharedPtr<FJsonObject> GetCameraIntrinsics(const UCameraSensor* Sensor);
	static TSharedPtr<FJsonObject> GetSensorExtrinsics(const USensorComponent* Sensor);
};