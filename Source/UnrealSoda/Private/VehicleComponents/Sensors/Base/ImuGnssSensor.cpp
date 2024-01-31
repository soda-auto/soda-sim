// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/ImuGnssSensor.h"
#include "Soda/UnrealSoda.h"
#include <iomanip>

UImuGnssSensor::UImuGnssSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IMU & GNSS Sensors");
	GUI.IcanName = TEXT("SodaIcons.IMU");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePostDeferredPhysTick = true;
}

bool UImuGnssSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	NoiseParams.UpdateParameters();

	return true;
}

void UImuGnssSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UImuGnssSensor::SetImuNoiseParams(const FImuNoiseParams & NewImuNoiseParams)
{
	if (!bIsStoredParams)
	{
		StoredParams = NoiseParams;
		bIsStoredParams = true;
	}
	NoiseParams = NewImuNoiseParams;
	NoiseParams.UpdateParameters();
}

void UImuGnssSensor::RestoreBaseImuNoiseParams()
{
	if (bIsStoredParams)
	{
		NoiseParams = StoredParams;
		NoiseParams.UpdateParameters();
		bIsStoredParams = false;
	}
}

void UImuGnssSensor::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime,  VehicleKinematic, Timestamp);

	if (HealthIsWorkable())
	{
		PublishSensorData(DeltaTime, GetHeaderVehicleThread(), GetRelativeTransform(), VehicleKinematic);
	}
}
