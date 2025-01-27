// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/LevelState.h"

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
