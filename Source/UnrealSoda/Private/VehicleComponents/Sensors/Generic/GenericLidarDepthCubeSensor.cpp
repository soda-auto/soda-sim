// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericLidarDepthCubeSensor.h"

UGenericLidarDepthCubeSensor::UGenericLidarDepthCubeSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Cube Depth");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericLidarDepthCubeSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	LidarRays.SetNum(Channels * Step);

	for (int v = 0; v < Channels; ++v)
	{
		const float AngleY = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)(Channels - 1) * (float)v;
		for (int u = 0; u < Step; ++u)
		{
			auto& Ray = LidarRays[Step * v + u];

			const float AngleX = FOV_HorizontMin + (FOV_HorizontMax - FOV_HorizontMin) / (float)(Step - 1) * (float)u;
			const float LenCoef = cos(FMath::DegreesToRadians(AngleX)) * cos(FMath::DegreesToRadians(AngleY));

			Ray = Ray.RotateAngleAxis(AngleY, FVector(0.f, 1.f, 0.f));
			Ray = Ray.RotateAngleAxis(AngleX, FVector(0.f, 0.f, 1.f));
			Ray = Ray / LenCoef;
		}
	}

	return LidarRays.Num() > 0;

	if (IsValid(Publisher))
	{
		return Publisher->AdvertiseAndSetHealth(this);
	}
}

void UGenericLidarDepthCubeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
	LidarRays.Reset();
}

bool UGenericLidarDepthCubeSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericLidarDepthCubeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericLidarDepthCubeSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarSensorData& Scan)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, Scan);
	}
	return false;
}

void UGenericLidarDepthCubeSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericLidarDepthCubeSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
