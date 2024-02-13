// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericV2XSensor.h"

UGenericV2XReceiverSensor::UGenericV2XReceiverSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic V2X Receiver");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericV2XReceiverSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericV2XReceiverSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericV2XReceiverSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		return PublisherHelper.Advertise();
	}
	return true;
}

void UGenericV2XReceiverSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
}

bool UGenericV2XReceiverSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericV2XReceiverSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericV2XReceiverSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericV2XReceiverSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

bool UGenericV2XReceiverSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& InTransmitters)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, InTransmitters);
	}
	return false;
}

void UGenericV2XReceiverSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericV2XReceiverSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
