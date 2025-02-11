// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"

UGenericWheeledVehicleSensor::UGenericWheeledVehicleSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Wheeled Vehicle");
	GUI.ComponentNameOverride = TEXT("Generic Wheeled Vehicle Sensor");
	GUI.bIsPresentInAddMenu = true;
	GUI.IcanName = TEXT("SodaIcons.Tire");

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePostDeferredPhysTick = true;
}

void UGenericWheeledVehicleSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericWheeledVehicleSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericWheeledVehicleSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetVehicle());

	if (!WheeledVehicle && !WheeledVehicle->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	//Engine = LinkToEngine.GetObject<UVehicleEngineBaseComponent>(GetOwner());
	//SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner());
	//BrakeSystem = LinkToBrakeSystem.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
	//HandBrake = LinkToHandBrake.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
	GearBox = LinkToGearBox.GetObject<UVehicleGearBoxBaseComponent>(GetOwner());
	VehicleDriver = LinkToGearBox.GetObject<UVehicleDriverComponent>(GetOwner());

	/*
	if (!Engine)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Engine  isn't connected"));
	}

	if (!BrakeSystem)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Brake system isn't connected"));
	}

	if (!SteeringRack)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Steering rack isn't connected"));
	}

	if (!HandBrake)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Hand brake isn't connected"));
	}

	if (!GearBox)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("GearBox isn't connected"));
	}
	

	if (!Engine || !SteeringRack || !BrakeSystem || !HandBrake)
	{
		SetHealth(EVehicleComponentHealth::Warning);
	}
	*/

	return PublisherHelper.Advertise();
}

void UGenericWheeledVehicleSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();

	//Engine = nullptr;
	//SteeringRack = nullptr;
	//BrakeSystem = nullptr;
	//HandBrake = nullptr;
	GearBox = nullptr;
	VehicleDriver = nullptr;
}

bool UGenericWheeledVehicleSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericWheeledVehicleSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericWheeledVehicleSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericWheeledVehicleSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

bool UGenericWheeledVehicleSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FWheeledVehicleSensorData& VehicleStateExtra)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, VehicleStateExtra);
	}
	return false;
}

void UGenericWheeledVehicleSensor::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	SensorData = FWheeledVehicleSensorData(&VehicleKinematic, GetRelativeTransform(), WheeledVehicle, GearBox, VehicleDriver);

	PublishSensorData(DeltaTime, GetHeaderVehicleThread(), SensorData);
}

void UGenericWheeledVehicleSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericWheeledVehicleSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
