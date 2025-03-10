// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericLidarRayTraceSensor.h"

UGenericLidarRayTraceSensor::UGenericLidarRayTraceSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Ray Trace");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericLidarRayTraceSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	for (int i = 0; i < Channels; i++)
	{
		const float VerticalAng = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)Channels * (float)i;
		for (int j = 0; j < Step; j++)
		{
			const float HorizontAng = FOV_HorizontMin + (FOV_HorizontMax - FOV_HorizontMin) / (float)Step * (float)j;
			LidarRays.Add(FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));
		}
	}

	if (IsValid(Publisher))
	{
		return Publisher->AdvertiseAndSetHealth(this);
	}
	
	return true;
}

void UGenericLidarRayTraceSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
	LidarRays.Reset();
}

bool UGenericLidarRayTraceSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericLidarRayTraceSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericLidarRayTraceSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarSensorData& InScan)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, InScan);
	}
	return false;
}

void UGenericLidarRayTraceSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericLidarRayTraceSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
