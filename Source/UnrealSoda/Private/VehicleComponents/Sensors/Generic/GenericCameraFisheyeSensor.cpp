// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericCameraFisheyeSensor.h"

UGenericCameraFisheyeSensor::UGenericCameraFisheyeSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Fisheye Camera");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericCameraFisheyeSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		if (IsValid(Publisher))
		{
			return Publisher->AdvertiseAndSetHealth(this);
		}
	}
	return true;
}

void UGenericCameraFisheyeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
}

bool UGenericCameraFisheyeSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericCameraFisheyeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

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
