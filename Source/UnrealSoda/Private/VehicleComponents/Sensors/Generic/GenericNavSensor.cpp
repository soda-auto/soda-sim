// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericNavSensor.h"
#include "Soda/UnrealSoda.h"
#include <iomanip>

UGenericNavSensor::UGenericNavSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Nav");
	GUI.bIsPresentInAddMenu = true;
}

bool UGenericNavSensor::OnActivateVehicleComponent()
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

void UGenericNavSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
}

bool UGenericNavSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericNavSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericNavSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, RelativeTransform, VehicleKinematic, NoiseParams);
	}
	return false;
}

void UGenericNavSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericNavSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
