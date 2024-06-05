// Fill out your copyright notice in the Description page of Project Settings.


#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/StaticMesh.h"

UDummyComponent::UDummyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Dummy component");
	GUI.bIsPresentInAddMenu = true;

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>("DummyMesh");
	DummyMesh->SetupAttachment(this);

	InitializeDummyMeshMap();
}

bool UDummyComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (DummyMeshMap.Contains(DummyType))
	{
		check(DummyMesh->SetStaticMesh(*DummyMeshMap.Find(DummyType)));
	}

	return true;
}

void UDummyComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	DummyMesh->SetStaticMesh(nullptr);
}


void UDummyComponent::InitializeDummyMeshMap()
{
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh1(TEXT("/SodaSim/DummyComponents/Cube.Cube"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh2(TEXT("/SodaSim/DummyComponents/Sphere.Sphere"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh3(TEXT("/SodaSim/DummyComponents/Cylinder.Cylinder"));

	if (Mesh1.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType1, Mesh1.Object);
	if (Mesh2.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType2, Mesh2.Object);
	if (Mesh3.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType3, Mesh3.Object);
}


void UDummyComponent::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UDummyComponent, DummyType))
	{
		if (DummyMeshMap.Contains(DummyType))
		{
			DummyMesh->SetStaticMesh(*DummyMeshMap.Find(DummyType));
		}
	}

}

void UDummyComponent::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UDummyComponent, DummyType))
	{
		if (DummyMeshMap.Contains(DummyType))
		{
			DummyMesh->SetStaticMesh(*DummyMeshMap.Find(DummyType));
		}
	}

}
