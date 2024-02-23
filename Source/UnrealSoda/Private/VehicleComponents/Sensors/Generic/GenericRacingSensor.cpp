// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericRacingSensor.h"

UGenericRacingSensor::UGenericRacingSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Racing Sensor");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericRacingSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericRacingSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericRacingSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		return PublisherHelper.Advertise();
	}
	return true;
}

void UGenericRacingSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
}

bool UGenericRacingSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericRacingSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericRacingSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericRacingSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

bool UGenericRacingSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& SensorData)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, SensorData);
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
