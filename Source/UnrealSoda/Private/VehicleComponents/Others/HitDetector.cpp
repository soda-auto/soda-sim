// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/HitDetector.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaSubsystem.h"

UHitDetectorComponent::UHitDetectorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	GUI.Category = TEXT("Other");
	GUI.IcanName = TEXT("SodaIcons.Soda");
	GUI.ComponentNameOverride = TEXT("Hit Detector");
	GUI.bIsPresentInAddMenu = true;
}

bool UHitDetectorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	GetVehicle()->OnActorHit.AddDynamic(this, &UHitDetectorComponent::OnVehicleHit);
	//GetVehicle->GetMesh()->OnComponentBeginOverlap.AddDynamic(this, &UHitDetectorComponent::OnVehicleHit);

	return true;
}

void UHitDetectorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

/*
void UHitDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HealthIsWorkable())
	{
		return;
	}
}
*/

void UHitDetectorComponent::OnVehicleHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogSoda, Log, TEXT("UHitDetectorComponent::OnVehicleHit() NormalImpulse: %s"), *NormalImpulse.ToString());
	if (bStopScenarioIfHit)
	{
		if (NormalImpulse.Size() > ImpulseThresholdScenarioStop)
		{
			SodaApp.GetSodaSubsystemChecked()->ScenarioStop(EScenarioStopReason::ScenarioStopTrigger, EScenarioStopMode::RestartLevel);
		}
	}
}