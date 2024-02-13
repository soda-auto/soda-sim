// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericCameraFisheyeSensor.h"

UGenericCameraFisheyeSensor::UGenericCameraFisheyeSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Fisheye Camera");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericCameraFisheyeSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericCameraFisheyeSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericCameraFisheyeSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		return PublisherHelper.Advertise();
	}
	return true;
}

void UGenericCameraFisheyeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
}

bool UGenericCameraFisheyeSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericCameraFisheyeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericCameraFisheyeSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericCameraFisheyeSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

bool UGenericCameraFisheyeSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& CameraFrame, const TArray<FColor>& BGRA8, uint32 ImageStride)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, CameraFrame, BGRA8, ImageStride);
	}
	return false;
}

void UGenericCameraFisheyeSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericCameraFisheyeSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
