// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericLidarRayTraceSensor.h"

UGenericLidarRayTraceSensor::UGenericLidarRayTraceSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Ray Trace");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericLidarRayTraceSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericLidarRayTraceSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
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

	return PublisherHelper.Advertise();
}

void UGenericLidarRayTraceSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
	LidarRays.Reset();
}

bool UGenericLidarRayTraceSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericLidarRayTraceSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericLidarRayTraceSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericLidarRayTraceSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

void UGenericLidarRayTraceSensor::PostProcessSensorData(soda::FLidarScan& InScan)
{
	check(InScan.Points.Num() == Channels * Step);

	for (int k = 0; k < Channels * Step; ++k)
	{
		InScan.Points[k].Layer = Channels - k / Step - 1;
	}
}

bool UGenericLidarRayTraceSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& InScan)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, InScan);
	}
	return false;
}