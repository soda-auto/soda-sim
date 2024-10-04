// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/V2VBox.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"

AV2XBox::AV2XBox()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_TriggerBox"));

	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	SpriteComponent->Sprite = IconTextureObject.Get();
	SpriteComponent->Mobility = EComponentMobility::Movable;
	SpriteComponent->SetHiddenInGame(false);
	SpriteComponent->bIsScreenSizeScaled = true;
	RootComponent = SpriteComponent;

	TriggerVolume = CreateDefaultSubobject< UBoxComponent >(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetHiddenInGame(false);
	//TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	//TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	//TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//TriggerVolume->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	//TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->InitBoxExtent(Extent);
	//TriggerVolume->SetMobility(EComponentMobility::Movable);

	V2XTransmitter = CreateDefaultSubobject<UV2XMarkerSensor>(TEXT("V2XTransmitter"));
	V2XTransmitter->SetupAttachment(RootComponent);
}

void AV2XBox::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	FProperty* MemberProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();
	if (MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AV2XBox, Extent))
	{
		TriggerVolume->SetBoxExtent(Extent);
	}
}

void AV2XBox::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->SetBoxExtent(Extent);
}

const FSodaActorDescriptor* AV2XBox::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("V2V Box"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("Icons.Box"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}
