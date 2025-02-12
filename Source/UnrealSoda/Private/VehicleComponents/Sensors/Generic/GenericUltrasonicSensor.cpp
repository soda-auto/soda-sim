// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericUltrasonicSensor.h"

UGenericUltrasonicHubSensor::UGenericUltrasonicHubSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Ultrasonic Hub");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericUltrasonicHubSensor::OnActivateVehicleComponent()
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

void UGenericUltrasonicHubSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
}

bool UGenericUltrasonicHubSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericUltrasonicHubSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericUltrasonicHubSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& InEchoCollections)
{
	
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, InEchoCollections);
	}
	return false;
}

void UGenericUltrasonicHubSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericUltrasonicHubSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
