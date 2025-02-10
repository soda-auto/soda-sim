// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/V2XSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Soda/SodaStatics.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "UObject/UObjectIterator.h"

int UV2XMarkerSensor::IDsCounter = 10000;

UV2XMarkerSensor::UV2XMarkerSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("V2X Marker");
	GUI.IcanName = TEXT("SodaIcons.Modem");
	GUI.bIsPresentInAddMenu = true;
}

void UV2XMarkerSensor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoCalculateBoundingBox)
	{
		CalculateBoundingBox();
	}

	if (bAssignUniqueIdAtStartup)
	{
		ID = IDsCounter;
	}

	++IDsCounter;
}

void UV2XMarkerSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;
}

bool UV2XMarkerSensor::OnActivateVehicleComponent()
{
	return Super::OnActivateVehicleComponent();
}

void UV2XMarkerSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

FString UV2XMarkerSensor::GetRemark() const
{
	return "ID: " + FString::FromInt(ID);
}

void UV2XMarkerSensor::CalculateBoundingBox()
{
	if (AActor* ParentActor = GetOwner())
	{
		Bound = USodaStatics::CalculateActorExtent(ParentActor);
	}
}

FTransform UV2XMarkerSensor::GetV2XTransform() const
{
	if (AActor* ParentActor = GetOwner())
	{
		return ParentActor->GetActorTransform();
	}
	else
	{
		return FTransform();
	}
}

FVector UV2XMarkerSensor::GetV2XWorldVelocity() const
{
	if (UPrimitiveComponent* BasePrimComp = Cast< UPrimitiveComponent >(GetAttachParent()))
	{
		return BasePrimComp->GetPhysicsLinearVelocityAtPoint(GetComponentLocation());
	}
	else
	{
		return FVector::ZeroVector;
	}
}

FVector UV2XMarkerSensor::GetV2XWorldAngVelocity() const
{
	if (UPrimitiveComponent* BasePrimComp = Cast< UPrimitiveComponent >(GetAttachParent()))
	{
		return BasePrimComp->GetPhysicsAngularVelocityInRadians();
	}
	else
	{
		return FVector::ZeroVector;
	}
}

/*********************************************************************************************************************/
UV2XReceiverSensor::UV2XReceiverSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("V2X Receiver");
	GUI.IcanName = TEXT("SodaIcons.Modem");

	PrimaryComponentTick.bCanEverTick = true;
}

void UV2XReceiverSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	Transmitters.SetNum(0, false);

	auto World = GetWorld();
	for (TObjectIterator< UV2XMarkerSensor > It; It; ++It)
	{
		if (It->GetWorld() == World)
		{
			if (((*It)->GetComponentLocation() - GetComponentLocation()).Size() < Radius)
			{
				Transmitters.Add(*It);
				if (bDrawDebugBoxes)
				{
					const auto Tranasform = It->GetV2XTransform();
					const auto Center = Tranasform.GetLocation() + Tranasform.GetRotation().RotateVector(It->Bound.GetCenter());
					DrawDebugBox(GetWorld(), Center, It->Bound.GetExtent(), Tranasform.GetRotation(), FColor::Green);
					DrawDebugString(GetWorld(), Center, *FString::Printf(TEXT("V2X ID:%d"), It->ID), NULL, FColor::Green);
				}
			}
		}
	}

	PublishSensorData(DeltaTime, GetHeaderGameThread(), Transmitters);
}
