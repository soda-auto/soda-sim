// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericRacingSensor.h"

UGenericRacingSensor::UGenericRacingSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Racing Sensor");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericRacingSensor::OnActivateVehicleComponent()
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

void UGenericRacingSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
}

bool UGenericRacingSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericRacingSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericRacingSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& InSensorData)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, InSensorData);
	}
	return false;
}

void UGenericRacingSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericRacingSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
