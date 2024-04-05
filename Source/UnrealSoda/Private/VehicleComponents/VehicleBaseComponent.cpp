// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Canvas.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaGameMode.h"
#include "Soda/LevelState.h"
#include "RuntimeEditorModule.h"

UVehicleBaseComponent::UVehicleBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	//PrimaryComponentTick.bCanEverTick = true;
	//bTickInEditor = false;
	//PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVehicleBaseComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Vehicle = Cast<ASodaVehicle>(GetOwner());
}

void UVehicleBaseComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void UVehicleBaseComponent::BeginPlay()
{
	GameMode = USodaGameModeComponent::GetChecked();
	LevelState = GameMode->LevelState;
	check(LevelState);

	Super::BeginPlay();
}

void UVehicleBaseComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsVehicleComponentActiveted())
	{
		OnDeactivateVehicleComponent();
	}
}

FSensorDataHeader UVehicleBaseComponent::GetHeaderGameThread() const
{
	return FSensorDataHeader{ SodaApp.GetSimulationTimestamp(), SodaApp.GetFrameIndex() };
}

FSensorDataHeader UVehicleBaseComponent::GetHeaderVehicleThread() const
{
	return FSensorDataHeader{ GetVehicle()->GetSimData().SimulatedTimestamp, GetVehicle()->GetSimData().SimulatedStep };
}

#if WITH_EDITOR
void UVehicleBaseComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		PostEditChangeProperty(PropertyChangedEvent);
	}
}

void UVehicleBaseComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		PostEditChangeChainProperty(PropertyChangedEvent);
	}
}
#endif
