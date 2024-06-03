// Fill out your copyright notice in the Description page of Project Settings.


#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/StaticMesh.h"

UDummyComponent::UDummyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Dummy component");
	GUI.bIsPresentInAddMenu = true;

	DummyMesh = nullptr;
}

void UDummyComponent::SetDummyMesh(UStaticMesh* NewMesh)
{
	if (NewMesh)
	{
		DummyMesh = NewMesh;
	}
}

bool UDummyComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	return true;
}

void UDummyComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}
