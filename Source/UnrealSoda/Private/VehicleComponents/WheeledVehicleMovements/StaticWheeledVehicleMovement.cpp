// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovements/StaticWheeledVehicleMovement.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"

UStaticWheeledVehicleMovementComponent::UStaticWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Static Vehicle");

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bWantsInitializeComponent = true;
}

void UStaticWheeledVehicleMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UStaticWheeledVehicleMovementComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void UStaticWheeledVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStaticWheeledVehicleMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UStaticWheeledVehicleMovementComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();


	OnSetActiveMovement();
}

bool UStaticWheeledVehicleMovementComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (UpdatedPrimitive)
	{
		UpdatedPrimitive->SetSimulatePhysics(false);
		UpdatedPrimitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		UpdatedPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	return true;
}

void UStaticWheeledVehicleMovementComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UStaticWheeledVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	PrePhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

	VehicleSimData.VehicleKinematic.Curr.GlobalPose = UpdatedPrimitive->GetComponentTransform();
	VehicleSimData.VehicleKinematic.Prev.GlobalPose = UpdatedPrimitive->GetComponentTransform();

	++VehicleSimData.SimulatedStep;
	VehicleSimData.SimulatedTimestamp = SodaApp.GetSimulationTimestamp();

	VehicleSimData.RenderStep = VehicleSimData.SimulatedStep;
	VehicleSimData.RenderTimestamp = VehicleSimData.SimulatedTimestamp;

	VehicleSimData.VehicleKinematic.Curr.GlobalPose = UpdatedPrimitive->GetComponentTransform();
	VehicleSimData.VehicleKinematic.Prev.GlobalPose = UpdatedPrimitive->GetComponentTransform();

	PostPhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
	PostPhysicSimulationDeferred(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
}

bool UStaticWheeledVehicleMovementComponent::SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation)
{
	if (UpdatedPrimitive)
	{
		VehicleSimData.VehicleKinematic.Prev.GlobalPose = VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(NewRotation, NewLocation, FVector(1.0, 1.0, 1.0));
		UpdatedPrimitive->SetWorldLocationAndRotation(NewLocation, NewRotation, true, nullptr, ETeleportType::TeleportPhysics);
		return true;
	}
	else
	{
		return false;
	}
}
