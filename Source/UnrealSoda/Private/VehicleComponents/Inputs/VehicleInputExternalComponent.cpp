// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputExternalComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UVehicleInputExternalComponent::UVehicleInputExternalComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Exteran");
	GUI.IcanName = TEXT("SodaIcons.External");
	GUI.bIsPresentInAddMenu = true;

	InputType = EVehicleInputType::External;
}

void UVehicleInputExternalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	SyncDataset();
}

