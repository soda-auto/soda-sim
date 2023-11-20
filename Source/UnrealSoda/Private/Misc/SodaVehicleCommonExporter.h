// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Dom/JsonObject.h"
#include "Soda/Vehicles/ISodaVehicleExporter.h"

class UImuSensorComponent;
class UFisheyeCameraSensorComponent;
class UCameraBaseSensorComponent;
class USensorComponent;

class FSodaVehicleCommonExporter : public ISodaVehicleExporter
{
public:
	virtual bool ExportToString(const ASodaVehicle* Vehicle, FString& String) override;
	virtual const FString& GetExporterName() const override { return ExporterName; }
	virtual const FString& GetFileTypes() const override { return ExporterFileType; }

	static const FString ExporterName;
	static const FString ExporterFileType;

	static TSharedPtr<FJsonObject> GetImuIntrinsics(const UImuSensorComponent* Sensor);
	static TSharedPtr<FJsonObject> GetFisheyeIntrinsics(const UFisheyeCameraSensorComponent* Sensor);
	static TSharedPtr<FJsonObject> GetCameraIntrinsics(const UCameraBaseSensorComponent* Sensor);
	static TSharedPtr<FJsonObject> GetSensorExtrinsics(const USensorComponent* Sensor);
};