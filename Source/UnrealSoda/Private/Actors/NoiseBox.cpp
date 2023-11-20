// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/NoiseBox.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"
#include "Soda/SodaGameMode.h"

ANoiseBox::ANoiseBox()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_Emitter"));

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

	auto & RTKGPS = NoisePresets.Add("RTKGPS");
	RTKGPS.Location.StdDev = FVector(0.08f);
	RTKGPS.Location.ConstBias = FVector(0.001f);
	RTKGPS.Location.GMBiasStdDev = FVector(0.008f);

	auto& StandartGPS = NoisePresets.Add("StandartGPS");
	StandartGPS.Location.StdDev = FVector(2.f);
	StandartGPS.Location.ConstBias = FVector(2.f);
	StandartGPS.Location.GMBiasStdDev = FVector(0.5f);

	auto& GPS_L5 = NoisePresets.Add("GPS_L5");
	GPS_L5.Location.StdDev = FVector(0.3f);
	GPS_L5.Location.ConstBias = FVector(0.001f);
	GPS_L5.Location.GMBiasStdDev = FVector(0.3f);

	DefaultProfileName = "StandartGPS";
}

void ANoiseBox::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	FProperty* MemberProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();
	if (MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ANoiseBox, Extent))
	{
		TriggerVolume->SetBoxExtent(Extent);
	}
}

/*
#if WITH_EDITOR
void ANoiseBox::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	
	//Get the name of the property that was changed
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	UE_LOG(LogSoda, Log, TEXT("PostEditChangeProperty: %s"), *PropertyName.ToString());

	// We test using GET_MEMBER_NAME_CHECKED so that if someone changes the property name
	// in the future this will fail to compile and we can update it.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UImuNoisePresets, Profile))
	{
		if (Profile != EGPSMode::Custom)
		{
			ImuNoiseParams = NoisePresets[Profile];
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, X) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Y) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Z) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, LocShiftStddev) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, LocTimeMeanValue) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, LocTimeStddev) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, LocFusionTime) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, RotShiftStddev) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FImuNoiseParams, RotFusionTime))
	{
		Profile = EGPSMode::Custom;
	}
	

	// Call the base class version
	Super::PostEditChangeProperty(PropertyChangedEvent);
};
#endif
*/

void ANoiseBox::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->SetBoxExtent(Extent);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ANoiseBox::OnBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ANoiseBox::OnEndOverlap);
}

void ANoiseBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &ANoiseBox::OnBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.RemoveDynamic(this, &ANoiseBox::OnEndOverlap);
}

void ANoiseBox::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (!bActiveOnlyIfScenario || (GameMode && GameMode->IsScenarioRunning()))
	{
		if (ASodaVehicle* Vehicle = Cast<ASodaVehicle>(Other))
		{
			for (auto& Component : Vehicle->GetVehicleComponents())
			{
				if (UImuSensorComponent* Sensor = Cast<UImuSensorComponent>(Component))
				{
					Sensor->SetImuNoiseParams(DefaultNoiseParams);
				}
			}
		}
	}
}

void ANoiseBox::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (!bActiveOnlyIfScenario || (GameMode && GameMode->IsScenarioRunning()))
	{
		if (ASodaVehicle* Vehicle = Cast<ASodaVehicle>(Other))
		{
			for (auto& Component : Vehicle->GetVehicleComponents())
			{
				if (UImuSensorComponent* Sensor = Cast<UImuSensorComponent>(Component))
				{
					Sensor->RestoreBaseImuNoiseParams();
				}
			}
		}
	}
}

void ANoiseBox::UpdateDefaultNoiseParams()
{
	if (auto* Profile = NoisePresets.Find(DefaultProfileName))
	{
		DefaultNoiseParams = *Profile;
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ANoiseBox::UpdateDefaultNoiseParams(); Can't find profile \"%s\""), *DefaultProfileName);
	}
}

const FSodaActorDescriptor* ANoiseBox::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Noise Box"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("Icons.Box"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50) /*SpawnOffset*/
	};
	return &Desc;
}
