// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Implementation/CSVLoggerSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/LevelState.h"
#include <iomanip>
#include <iostream>

UCSVLoggerSensorComponent::UCSVLoggerSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("CSV Logger");
	GUI.bIsPresentInAddMenu = true;
	GUI.IcanName = TEXT("SodaIcons.GPS");
}

bool UCSVLoggerSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}
	OutFile.close();
	OutFile.open(TCHAR_TO_UTF8(*CSVPath), std::ios::trunc);

	if (!OutFile.is_open())
	{
		SetHealth(EVehicleComponentHealth::Error, FString::Printf(TEXT("Can't open file \"%s\""), *CSVPath));
		return false;
	}

	OutFile << "LocX,LocY,LocZ,Heading,Pitch,Roll,VelEast,VelNorth,VelDown,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,Lon,Lat,Alt,Timestemp\n";
		
	NoiseParams.UpdateParameters();

	return true;
}

void UCSVLoggerSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (OutFile.is_open())
	{
		OutFile.close();
	}
}

bool UCSVLoggerSensorComponent::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic)
{
	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro = VehicleKinematic.Curr.AngularVelocity;
	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();

	if (bImuNoiseEnabled)
	{
		FVector RotNoize = NoiseParams.Rotation.Step();
		WorldRot += FRotator(RotNoize.Y, RotNoize.Z, RotNoize.X);
		WorldLoc += NoiseParams.Location.Step() * 100;
		LocalAcc += NoiseParams.Acceleration.Step() * 100;
		WorldVel += NoiseParams.Velocity.Step() * 100;
		Gyro += NoiseParams.Gyro.Step();
	}

	double Lon, Lat, Alt;
	GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);
	WorldRot = GetLevelState()->GetLLConverter().ConvertRotationForward(WorldRot);
	WorldVel = GetLevelState()->GetLLConverter().ConvertDirForward(WorldVel);

	OutFile << std::setprecision(12)
		<< WorldLoc.X / 100.0 << "," << -WorldLoc.Y / 100.0 << "," << WorldLoc.Z / 100.0 << ","
		<< NormAngDeg(WorldRot.Yaw - 90) << ","
		<< NormAngDeg(WorldRot.Pitch) << ","
		<< NormAngDeg(WorldRot.Roll) << ","
		<< -WorldVel.X / 100.0 << "," << WorldVel.Y / 100.0 << "," << -WorldVel.Z / 100.0 << ","
		<< -LocalAcc.X / 100.0 << "," << -LocalAcc.Y / 100.0 << "," << LocalAcc.Z / 100.0 << ","
		<< -Gyro.X << "," << -Gyro.Y << "," << Gyro.Z << ","
		<< Lon << "," << Lat << "," << Alt << "," << soda::RawTimestamp<std::chrono::milliseconds>(Header.Timestamp) << "\n";

	return true;
}
