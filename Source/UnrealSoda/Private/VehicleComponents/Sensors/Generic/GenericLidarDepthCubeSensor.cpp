// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericLidarDepthCubeSensor.h"

UGenericLidarDepthCubeSensor::UGenericLidarDepthCubeSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Cube Depth");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericLidarDepthCubeSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericLidarDepthCubeSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
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

	return PublisherHelper.Advertise();
}

void UGenericLidarDepthCubeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
	LidarRays.Reset();
}

bool UGenericLidarDepthCubeSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericLidarDepthCubeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericLidarDepthCubeSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericLidarDepthCubeSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

void UGenericLidarDepthCubeSensor::PostProcessSensorData(soda::FLidarScan& InScan)
{
	check(InScan.Points.Num() == Channels * Step);

	for (int k = 0; k < Channels * Step; ++k)
	{
		InScan.Points[k].Layer = Channels - k / Step - 1;
	}
}

bool UGenericLidarDepthCubeSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, Scan);
	}
	return false;
}
