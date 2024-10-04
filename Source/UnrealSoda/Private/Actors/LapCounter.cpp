// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/LapCounter.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "UObject/ConstructorHelpers.h"

ALapCounter::ALapCounter()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/Finish"));

	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteComponent"));
	SpriteComponent->Sprite = IconTextureObject.Get();
	SpriteComponent->Mobility = EComponentMobility::Movable;
	SpriteComponent->SetHiddenInGame(false);
	SpriteComponent->bIsScreenSizeScaled = true;
	RootComponent = SpriteComponent;
	
	TriggerVolume = CreateDefaultSubobject< UBoxComponent >(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	TriggerVolume->SetHiddenInGame(!bDrawBox);
	//TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	//TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	//TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//TriggerVolume->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	//TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->InitBoxExtent(Extent);
	//TriggerVolume->SetMobility(EComponentMobility::Movable);

}

void ALapCounter::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->SetBoxExtent(Extent);
	TriggerVolume->SetHiddenInGame(!bDrawBox);
	if (!TriggerVolume->OnComponentBeginOverlap.IsAlreadyBound(this, &ALapCounter::OnTriggerBeginOverlap))
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALapCounter::OnTriggerBeginOverlap);
	}
}

void ALapCounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (TriggerVolume->OnComponentBeginOverlap.IsAlreadyBound(this, &ALapCounter::OnTriggerBeginOverlap))
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &ALapCounter::OnTriggerBeginOverlap);
	}
}

void ALapCounter::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASodaVehicle* Vehicle = Cast<ASodaVehicle>(OtherActor))
	{
		TArray<UActorComponent*> Components = Vehicle->GetComponentsByInterface(ULapCounterTriggeredComponent::StaticClass());
		for (auto& Component : Components)
		{
			if (ILapCounterTriggeredComponent* TriggeredComponent = Cast<ILapCounterTriggeredComponent>(Component))
			{
				TriggeredComponent->OnLapCounterTriggerBeginOverlap(this, SweepResult);
			}
		}
	}
}

void ALapCounter::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	FProperty* MemberProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();
	if (MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ALapCounter, Extent))
	{
		TriggerVolume->SetBoxExtent(Extent);
	}
	if (MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ALapCounter, bDrawBox))
	{
		TriggerVolume->SetHiddenInGame(!bDrawBox);
	}
}

const FSodaActorDescriptor* ALapCounter::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Lap Counter"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Experimental"),
		TEXT("SodaIcons.LapCount"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}