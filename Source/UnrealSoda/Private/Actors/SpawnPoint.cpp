// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/SpawnPoint.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"

ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/Spawn_Point"));

	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	SpriteComponent->Sprite = IconTextureObject.Get();
	SpriteComponent->Mobility = EComponentMobility::Movable;
	SpriteComponent->SetHiddenInGame(false);
	SpriteComponent->bIsScreenSizeScaled = true;
	RootComponent = SpriteComponent;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->Mobility = EComponentMobility::Movable;
	ArrowComponent->SetHiddenInGame(false);
	ArrowComponent->bIsScreenSizeScaled = true;
	ArrowComponent->ArrowSize = 2.5;
	ArrowComponent->SetArrowColor(FLinearColor(0, 0.35, 1));
	
	TriggerVolume = CreateDefaultSubobject< UBoxComponent >(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetHiddenInGame(true);
	TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetBoxExtent(FVector{ 50.0f, 50.0f, 50.0f });
	TriggerVolume->SetMobility(EComponentMobility::Movable);
}

#if WITH_EDITOR
void ASpawnPoint::PostEditChangeProperty(struct FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
}
#endif // WITH_EDITOR


void ASpawnPoint::BeginPlay()
{
	Super::BeginPlay();
}

void ASpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASpawnPoint::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASpawnPoint::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();
	if (IsValid(SpawnedActor))
	{
		SpawnedActor->Destroy();
		SpawnedActor = nullptr;
	}

	if (bIsActivePoint)
	{
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass, GetActorTransform());

		if (bSetVehicleToAIMode)
		{
			if (ASodaWheeledVehicle* Vehicle = Cast<ASodaWheeledVehicle>(SpawnedActor))
			{
				if (UVehicleInputAIComponent* Input = Cast<UVehicleInputAIComponent>(Vehicle->SetActiveVehicleInputByType(EVehicleInputType::AI)))
				{
					Input->SetSpeedLimit(VehicleAISpeed);
				}
			}
		}

		OnActorSpawned(SpawnedActor);
	}
}

void ASpawnPoint::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();
	if (IsValid(SpawnedActor))
	{
		SpawnedActor->Destroy();
		SpawnedActor = nullptr;
	}
}

const FSodaActorDescriptor* ASpawnPoint::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Spawn Point"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Point"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}
