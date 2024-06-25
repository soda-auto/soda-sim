// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovementBaseComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/UnrealSoda.h"

UWheeledVehicleMovementBaseComponent::UWheeledVehicleMovementBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Physic");
	GUI.IcanName = TEXT("SodaIcons.Tire");
	GUI.bIsPresentInAddMenu = true;
	Common.UniqueVehiceComponentClass = UWheeledVehicleMovementInterface::StaticClass();
	Common.bIsTopologyComponent = true;
}

void UWheeledVehicleMovementBaseComponent::InitializeComponent()
{
	Super::InitializeComponent();
	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetOwner());
}

ASodaVehicle* UWheeledVehicleMovementBaseComponent::GetVehicle() const 
{
	return Cast<ASodaVehicle>(GetOwner()); 
}

bool UWheeledVehicleMovementBaseComponent::OnActivateVehicleComponent()
{
	if (!ISodaVehicleComponent::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!GetWheeledVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Paren isn't ASodaWheeledVehicle"));
		UE_LOG(LogSoda, Error, TEXT("UWheeledVehicleMovementBaseComponent::OnActivateVehicleComponent(); Parent actor must be ASodaWheeledVehicle"));
		return false;
	}

	ReceiveActivateVehicleComponent();

	return true;
}

void UWheeledVehicleMovementBaseComponent::OnDeactivateVehicleComponent()
{
	ISodaVehicleComponent::OnDeactivateVehicleComponent();
	ReceiveDeactivateVehicleComponent();
}

#if WITH_EDITOR
void UWheeledVehicleMovementBaseComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
}

void UWheeledVehicleMovementBaseComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		Super::PostEditChangeChainProperty(PropertyChangedEvent);
	}
}
#endif