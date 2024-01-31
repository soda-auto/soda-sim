// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/SensorHolder.h"
#include "Soda/UnrealSoda.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

ASensorHolder::ASensorHolder()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_RadForce"));

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

void ASensorHolder::BeginPlay()
{
	Super::BeginPlay();
	/*
	EditableComponent->VariantParams->OnParamChanged.AddDynamic(this, &ASensorHolder::OnParamChanged);
	EditableComponent->VariantParams->AddObjectParams(EditableComponent);
	EditableComponent->VariantParams->AddObjectParams(this);
	EditableComponent->OnSelected.AddDynamic(this, &ASensorHolder::OnSelected);
	EditableComponent->OnUnselected.AddDynamic(this, &ASensorHolder::OnUnselected);
	*/

	if (SensorClass)
	{
		SensorComponent = NewObject<USensorComponent>(this, SensorClass);
		check(SensorComponent);
		if (ComponentRecord.IsRecordValid())
		{
			ComponentRecord.DeserializeComponent(SensorComponent);
		}
		SensorComponent->SetupAttachment(RootComponent);
		SensorComponent->RegisterComponent();
		SensorComponent->HideActorComponentsFromSensorView(this);
		//EditableComponent->VariantParams->AddObjectParams(SensorComponent);
	}
}

void ASensorHolder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASensorHolder::RecreateSensor()
{
	//EditableComponent->VariantParams->Params.Empty();

	if (SensorClass)
	{
		USensorComponent* NewSensor = NewObject<USensorComponent>(this, SensorClass);
		check(NewSensor);
		if (SensorComponent && SensorClass == SensorComponent->GetClass())
		{
			FComponentRecord TmpRecord;
			TmpRecord.SerializeComponent(SensorComponent);
			TmpRecord.DeserializeComponent(NewSensor);
		}
		NewSensor->SetupAttachment(RootComponent);
		NewSensor->RegisterComponent();
		NewSensor->HideActorComponentsFromSensorView(this);

		if (SensorComponent)
		{
			SensorComponent->RemoveVehicleComponent();
		}
		SensorComponent = NewSensor;
	}
	else
	{
		if (SensorComponent)
		{
			SensorComponent->RemoveVehicleComponent();
			SensorComponent = nullptr;
		}
	}

	/*
	EditableComponent->VariantParams->AddObjectParams(this);
	if (SensorComponent) EditableComponent->VariantParams->AddObjectParams(SensorComponent);
	*/
}

/*
void ASensorHolder::OnParamChanged(UVariantParam* VariantParam)
{
	if (VariantParam->NeedUpdate())
	{
		RecreateSensor();
	}
}
*/

void ASensorHolder::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaving())
	{
		if (SensorComponent)
		{
			ComponentRecord.SerializeComponent(SensorComponent);
			Ar << ComponentRecord;
		}

	}
	else if (Ar.IsLoading())
	{
		Ar << ComponentRecord;
	}
}

void ASensorHolder::OnSelected()
{	
}

void ASensorHolder::OnUnselected()
{
}

const FSodaActorDescriptor* ASensorHolder::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Sensor Holder"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("SodaIcons.Sensor"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}