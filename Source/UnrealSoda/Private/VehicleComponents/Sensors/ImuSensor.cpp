// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/ImuSensor.h"
#include "Soda/UnrealSoda.h"
#include <iomanip>

UImuSensorComponent::UImuSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IMU & GNSS Sensors");
	GUI.IcanName = TEXT("SodaIcons.IMU");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePostDeferredPhysTick = true;
}

bool UImuSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (bSaveCSV)
	{
		OutFile.close();
		OutFile.open(TCHAR_TO_UTF8(*CSVPath));
		if (OutFile.is_open())
		{
			OutFile << "LocX,LocY,LocZ,Heading,Pitch,Roll,VelEast,VelNorth,VelDown,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,Lon,Lat,Alt,Timestemp\n";
		}
	}

	NoiseParams.UpdateParameters();

	return true;
}

void UImuSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (OutFile.is_open())
	{
		OutFile.close();
	}
}

void UImuSensorComponent::SetImuNoiseParams(const FImuNoiseParams & NewImuNoiseParams)
{
	if (!bIsStoredParams)
	{
		StoredParams = NoiseParams;
		bIsStoredParams = true;
	}
	NoiseParams = NewImuNoiseParams;
	NoiseParams.UpdateParameters();
}

void UImuSensorComponent::RestoreBaseImuNoiseParams()
{
	if (bIsStoredParams)
	{
		NoiseParams = StoredParams;
		NoiseParams.UpdateParameters();
		bIsStoredParams = false;
	}
}

void UImuSensorComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& TimestampIn)
{
	Super::PostPhysicSimulationDeferred(DeltaTime,  VehicleKinematic, TimestampIn);
}

void UImuSensorComponent::WriteCsvLog(
	const FVector& InWorldLoc,
	const FRotator& InWorldRot,
	const FVector& InWorldVel,
	const FVector& InLocalAcc,
	const FVector& InGyro,
	double InLon,
	double InLat,
	double InAlt,
	const TTimestamp& Timestamp)
{
	OutFile << std::setprecision(12)
		<< InWorldLoc.X / 100.0 << "," << -InWorldLoc.Y / 100.0 << "," << InWorldLoc.Z / 100.0 << ","
		<< NormAngDeg(InWorldRot.Yaw - 90) << ","
		<< NormAngDeg(InWorldRot.Pitch) << ","
		<< NormAngDeg(InWorldRot.Roll) << ","
		<< -InWorldVel.X / 100.0 << "," << InWorldVel.Y / 100.0 << "," << -InWorldVel.Z / 100.0 << ","
		<< -InLocalAcc.X / 100.0 << "," << -InLocalAcc.Y / 100.0 << "," << InLocalAcc.Z / 100.0 << ","
		<< -InGyro.X << "," << -InGyro.Y << "," << InGyro.Z << ","
		<< InLon << "," << InLat << "," << InAlt << "," << soda::RawTimestamp<std::chrono::milliseconds>(Timestamp) << "\n";
}