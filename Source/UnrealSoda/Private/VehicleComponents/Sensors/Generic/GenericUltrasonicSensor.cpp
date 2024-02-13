// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericUltrasonicSensor.h"

UGenericUltrasonicHubSensor::UGenericUltrasonicHubSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Ultrasonic Hub");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericUltrasonicHubSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericUltrasonicHubSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericUltrasonicHubSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		return PublisherHelper.Advertise();
	}
	return true;
}

void UGenericUltrasonicHubSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
}

bool UGenericUltrasonicHubSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericUltrasonicHubSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericUltrasonicHubSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericUltrasonicHubSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

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
