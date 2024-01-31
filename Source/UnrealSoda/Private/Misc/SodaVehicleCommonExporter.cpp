// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaVehicleCommonExporter.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/VehicleComponents/Sensors/Base/ImuGnssSensor.h"
#include "Soda/VehicleComponents/Sensors/Base/CameraFisheyeSensor.h"
#include "Soda/UnrealSodaVersion.h"

const FString FSodaVehicleCommonExporter::ExporterName = TEXT("Soda Sensors Common");
const FString FSodaVehicleCommonExporter::ExporterFileType = TEXT("Soda Sensors Common (*.json)|*.json");

bool FSodaVehicleCommonExporter::ExportToString(const ASodaVehicle* Vehicle, FString& JsonString)
{
	JsonString.Empty();

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField("unreal_soda_ver", UNREALSODA_VERSION_STRING);
	TSharedPtr<FJsonObject> SensorsObject = MakeShared<FJsonObject>();
	RootObject->SetObjectField("sensors", SensorsObject);

	TArray<USensorComponent*> Sensors;
	Vehicle->GetComponents<USensorComponent>(Sensors);
	for (auto Sensor : Sensors)
	{
		if (IsValid(Sensor))
		{
			TSharedPtr<FJsonObject> SensorObject = MakeShared<FJsonObject>();
			TSharedPtr<FJsonObject> Int;
			TSharedPtr<FJsonObject> Ext = GetSensorExtrinsics(Sensor);
			if (UCameraFisheyeSensor* FisheyeCameraSensor = Cast<UCameraFisheyeSensor>(Sensor))
			{
				Int = GetFisheyeIntrinsics(FisheyeCameraSensor);
			}
			else if (UCameraSensor* CameraSensor = Cast<UCameraSensor>(Sensor))
			{
				Int = GetCameraIntrinsics(CameraSensor);
			}
			else if (UImuGnssSensor* ImuSensor = Cast<UImuGnssSensor>(Sensor))
			{
				Int = GetImuIntrinsics(ImuSensor);
			}

			if (Int)
			{
				SensorObject->SetObjectField("intrinsics", Int);
			}
			if (Ext)
			{
				SensorObject->SetObjectField("extrinsics", Ext);
			}
			SensorsObject->SetStringField("class", Sensor->GetClass()->GetName());
			SensorsObject->SetObjectField(Sensor->GetName(), SensorObject);
		}
	}

	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogSoda, Error, TEXT("FSodaVehicleCommonExporter::ExportToString(); Can't serialize ot JSON "));
		JsonString = "{}";
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FSodaVehicleCommonExporter::GetSensorExtrinsics(const USensorComponent* Sensor)
{
	FVector Loc = Sensor->GetRelativeLocation();
	FRotator Rot = Sensor->GetRelativeRotation();
	Loc = FVector(Loc.X, -Loc.Y, Loc.Z) / 100;
	Rot = FRotator(Rot.Pitch, -Rot.Yaw, -Rot.Roll);
	FMatrix Matrix = FRotationMatrix::Make(Rot);

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> T = MakeShared<FJsonObject>();
	JsonObject->SetObjectField(TEXT("R"), R);
	JsonObject->SetObjectField(TEXT("T"), T);

	R->SetNumberField("cols", 3);
	R->SetNumberField("rows", 3);
	R->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Matrix.M[0][0]),
		MakeShared<FJsonValueNumber>(Matrix.M[0][1]),
		MakeShared<FJsonValueNumber>(Matrix.M[0][2]),
		MakeShared<FJsonValueNumber>(Matrix.M[1][0]),
		MakeShared<FJsonValueNumber>(Matrix.M[1][1]),
		MakeShared<FJsonValueNumber>(Matrix.M[1][2]),
		MakeShared<FJsonValueNumber>(Matrix.M[2][0]),
		MakeShared<FJsonValueNumber>(Matrix.M[2][1]),
		MakeShared<FJsonValueNumber>(Matrix.M[2][2])
	});

	T->SetNumberField("cols", 1);
	T->SetNumberField("rows", 3);
	T->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Loc.X),
		MakeShared<FJsonValueNumber>(Loc.Y),
		MakeShared<FJsonValueNumber>(Loc.Z)
	});

	return JsonObject;
}

TSharedPtr<FJsonObject> FSodaVehicleCommonExporter::GetCameraIntrinsics(const UCameraSensor* Sensor)
{
	FCameraIntrinsics Intrinsics = Sensor->GetCameraIntrinsics();

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> D = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> K = MakeShared<FJsonObject>();

	D->SetNumberField("cols", 1);
	D->SetNumberField("rows", 4);
	D->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Intrinsics.K1),
		MakeShared<FJsonValueNumber>(Intrinsics.K2),
		MakeShared<FJsonValueNumber>(Intrinsics.K3),
		MakeShared<FJsonValueNumber>(Intrinsics.K4)
	});

	K->SetNumberField("cols", 3);
	K->SetNumberField("rows", 3);
	K->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Intrinsics.Fx),
		MakeShared<FJsonValueNumber>(0),
		MakeShared<FJsonValueNumber>(Intrinsics.Cx),
		MakeShared<FJsonValueNumber>(0),
		MakeShared<FJsonValueNumber>(Intrinsics.Fy),
		MakeShared<FJsonValueNumber>(Intrinsics.Cy),
		MakeShared<FJsonValueNumber>(0),
		MakeShared<FJsonValueNumber>(0),
		MakeShared<FJsonValueNumber>(1.0)
	});

	JsonObject->SetObjectField("D", D);
	JsonObject->SetObjectField("K", K);
	JsonObject->SetNumberField("hfov", Sensor->GetHFOV());
	JsonObject->SetNumberField("vfov", Sensor->GetVFOV());
	JsonObject->SetNumberField("frame_width", Sensor->Width);
	JsonObject->SetNumberField("frame_height", Sensor->Height);
	JsonObject->SetStringField("projection_model", "opencv");

	return JsonObject;
}

TSharedPtr<FJsonObject> FSodaVehicleCommonExporter::GetFisheyeIntrinsics(const UCameraFisheyeSensor* Sensor)
{
	TSharedPtr<FJsonObject> JsonObject = GetCameraIntrinsics(Sensor);
	check(JsonObject);
	JsonObject->SetStringField("projection_model", "fisheye_opencv");
	return JsonObject;
}

TSharedPtr<FJsonObject> FSodaVehicleCommonExporter::GetImuIntrinsics(const UImuGnssSensor* Sensor)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> GyroBias = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> AccBias = MakeShared<FJsonObject>();

	GyroBias->SetNumberField("cols", 1);
	GyroBias->SetNumberField("rows", 3);
	GyroBias->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Gyro.ConstBias.X),
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Gyro.ConstBias.Y),
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Gyro.ConstBias.Z),
	});

	AccBias->SetNumberField("cols", 1);
	AccBias->SetNumberField("rows", 3);
	AccBias->SetArrayField("data", {
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Acceleration.ConstBias.X),
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Acceleration.ConstBias.X),
		MakeShared<FJsonValueNumber>(Sensor->NoiseParams.Acceleration.ConstBias.X),
	});

	JsonObject->SetObjectField("gyro_bias", GyroBias);
	JsonObject->SetObjectField("acc_bias", AccBias);

	return JsonObject;
}
