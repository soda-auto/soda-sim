// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/LevelState.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UNavSensor::UNavSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IMU & GNSS Sensors");
	GUI.IcanName = TEXT("SodaIcons.IMU");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePostDeferredPhysTick = true;
}

bool UNavSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	NoiseParams.UpdateParameters();

	return true;
}

void UNavSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UNavSensor::SetImuNoiseParams(const FImuNoiseParams & NewImuNoiseParams)
{
	if (!bIsStoredParams)
	{
		StoredParams = NoiseParams;
		bIsStoredParams = true;
	}
	NoiseParams = NewImuNoiseParams;
	NoiseParams.UpdateParameters();
}

void UNavSensor::RestoreBaseImuNoiseParams()
{
	if (bIsStoredParams)
	{
		NoiseParams = StoredParams;
		NoiseParams.UpdateParameters();
		bIsStoredParams = false;
	}
}

void UNavSensor::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime,  VehicleKinematic, Timestamp);

	StoredBodyKinematic = VehicleKinematic;

	if (HealthIsWorkable())
	{
		PublishSensorData(DeltaTime, GetHeaderVehicleThread(), GetRelativeTransform(), VehicleKinematic);
	}
}

void UNavSensor::OnPushDataset(soda::FActorDatasetData& Dataset) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
	StoredBodyKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();
	FVector LocVel = WorldRot.UnrotateVector(WorldVel);

	double Longitude = 0;
	double Latitude = 0;
	double Altitude = 0;

	if (GetLevelState())
	{
		GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Longitude, Latitude, Altitude);
		WorldRot = GetLevelState()->GetLLConverter().ConvertRotationForward(WorldRot);
		WorldVel = GetLevelState()->GetLLConverter().ConvertDirForward(WorldVel);
	}

	try
	{
		Dataset.GetRowDoc()
			<< std::string(TCHAR_TO_UTF8(*GetName())) << open_document
			<< "Position" << open_array << WorldLoc.X << WorldLoc.Y << WorldLoc.Z << close_array
			<< "Rotation" << open_array << WorldRot.Roll << WorldRot.Pitch << WorldRot.Yaw << close_array
			<< "LocVel" << open_array << LocVel.X << LocVel.Y << LocVel.Z << close_array
			<< "WorldVel" << open_array << WorldVel.X << WorldVel.Y << WorldVel.Z << close_array
			<< "AngularVel" << open_array << Gyro.X << Gyro.Y << Gyro.Z << close_array
			<< "Acc" << open_array << LocalAcc.X << LocalAcc.Y << LocalAcc.Z << close_array
			<< "Lon" << Longitude
			<< "Lat" << Latitude
			<< "Alt" << Altitude
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("UNavSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}
