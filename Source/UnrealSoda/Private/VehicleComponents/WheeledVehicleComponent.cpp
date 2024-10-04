// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Canvas.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaSubsystem.h"
#include "RuntimeEditorModule.h"

UWheeledVehicleComponent::UWheeledVehicleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Common.bIsTopologyComponent = true;
}

void UWheeledVehicleComponent::InitializeComponent()
{
	Super::InitializeComponent();
	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetOwner());
}

void UWheeledVehicleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(WheeledVehicle))
	{
		WheeledComponentInterface = WheeledVehicle->GetWheeledComponentInterface();
	}
}

bool UWheeledVehicleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (IsValid(WheeledVehicle))
	{
		WheeledComponentInterface = WheeledVehicle->GetWheeledComponentInterface();
	}

	if (!GetWheeledComponentInterface())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Need WheeledComponentInterface"));
		return false;
	}

	return true;
}

void UWheeledVehicleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UWheeledVehicleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
{
	Super::OnPushDataset(Dataset);
}