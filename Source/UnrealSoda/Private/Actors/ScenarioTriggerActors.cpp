// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/ScenarioTriggerActors.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TriggerBox.h"
#include "Engine/TriggerCapsule.h"
#include "Engine/TriggerSphere.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/ShapeComponent.h"

namespace
{
	static const FColor TriggerBaseColor(100, 255, 100, 255);
	static const FName TriggerCollisionProfileName(TEXT("Trigger"));
}

AScenarioTriggerBase::AScenarioTriggerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetHidden(false);
	SetCanBeDamaged(false);

	// AScenarioTriggerBase is requesting UShapeComponent which is abstract, however it is responsibility
	// of a derived class to override this type with ObjectInitializer.SetDefaultSubobjectClass.
	CollisionComponent = CreateDefaultSubobject<UShapeComponent>(TEXT("CollisionComp"));
	if (CollisionComponent)
	{
		RootComponent = CollisionComponent;
		CollisionComponent->bHiddenInGame = false;
	}

	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TriggerTextureObject;
			FName ID_Triggers;
			FText NAME_Triggers;
			FConstructorStatics()
				: TriggerTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_Trigger"))
				, ID_Triggers(TEXT("ScenarioTriggers"))
				, NAME_Triggers(NSLOCTEXT("SpriteCategory", "ScenarioTriggers", "ScenarioTriggers"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.TriggerTextureObject.Get();
		SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		SpriteComponent->bHiddenInGame = false;
#if WITH_EDITORONLY_DATA
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Triggers;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Triggers;
#endif
		SpriteComponent->bIsScreenSizeScaled = true;
	}
}

//----------------------------------------------------------------------------------------------
AScenarioTriggerCapsule::AScenarioTriggerCapsule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCapsuleComponent>(TEXT("CollisionComp")))
{
	UCapsuleComponent* CapsuleCollisionComponent = CastChecked<UCapsuleComponent>(GetCollisionComponent());
	CapsuleCollisionComponent->ShapeColor = TriggerBaseColor;
	CapsuleCollisionComponent->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	CapsuleCollisionComponent->SetCollisionProfileName(TriggerCollisionProfileName);

	bCollideWhenPlacing = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	if (UBillboardComponent* TriggerSpriteComponent = GetSpriteComponent())
	{
		TriggerSpriteComponent->SetupAttachment(CapsuleCollisionComponent);
	}

	if (GetSpriteComponent())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TriggerTextureObject;
			FName ID_Triggers;
			FText NAME_Triggers;
			FConstructorStatics()
				: TriggerTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_TriggerCapsule"))
				, ID_Triggers(TEXT("ScenarioTriggers"))
				, NAME_Triggers(NSLOCTEXT("SpriteCategory", "ScenarioTriggers", "ScenarioTriggers"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		GetSpriteComponent()->Sprite = ConstructorStatics.TriggerTextureObject.Get();
	}
}

/*
void AScenarioTriggerCapsule::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	UCapsuleComponent * CapsuleComponent = CastChecked<UCapsuleComponent>(GetRootComponent());

	float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
	float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		
	CapsuleRadius += ModifiedScale.X;
	CapsuleRadius = FMath::Max( 0.0f, CapsuleRadius );

	// If non-uniformly scaling, Z scale affects height and Y can affect radius too.
	if ( !ModifiedScale.AllComponentsEqual() )
	{
		// *2 to keep the capsule more capsule shaped during scaling
		CapsuleHalfHeight+= ModifiedScale.Z * 2.0f;
		CapsuleHalfHeight = FMath::Max( 0.0f, CapsuleHalfHeight );

		CapsuleRadius += ModifiedScale.Y;
		CapsuleRadius = FMath::Max( 0.0f, CapsuleRadius );
	}
	else
	{
		// uniform scaling, so apply to height as well

		// *2 to keep the capsule more capsule shaped during scaling
		CapsuleHalfHeight += ModifiedScale.Z * 2.0f;
		CapsuleHalfHeight = FMath::Max( 0.0f, CapsuleHalfHeight );
	}

	CapsuleComponent->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	
}
*/

void AScenarioTriggerCapsule::BeginPlay()
{
	Super::BeginPlay();

	CastChecked<UCapsuleComponent>(GetCollisionComponent())->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
}

void AScenarioTriggerCapsule::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);
}

void AScenarioTriggerCapsule::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr && 
		(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AScenarioTriggerCapsule, CapsuleRadius) ||
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AScenarioTriggerCapsule, CapsuleHalfHeight)))
	{
		UCapsuleComponent* CapsuleCollisionComponent = CastChecked<UCapsuleComponent>(GetCollisionComponent());
		CapsuleCollisionComponent->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}
}

const FSodaActorDescriptor* AScenarioTriggerCapsule::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Trigger Capsule"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Triggers"), /*SubCategory*/
		TEXT("ClassIcon.TriggerCapsule"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}

//----------------------------------------------------------------------------------------------
AScenarioTriggerBox::AScenarioTriggerBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(TEXT("CollisionComp")))
{
	UBoxComponent* BoxCollisionComponent = CastChecked<UBoxComponent>(GetCollisionComponent());

	BoxCollisionComponent->ShapeColor = TriggerBaseColor;
	BoxCollisionComponent->InitBoxExtent(Extent);
	BoxCollisionComponent->SetCollisionProfileName(TriggerCollisionProfileName);

	if (UBillboardComponent* TriggerSpriteComponent = GetSpriteComponent())
	{
		TriggerSpriteComponent->SetupAttachment(BoxCollisionComponent);
	}

	if (GetSpriteComponent())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TriggerTextureObject;
			FName ID_Triggers;
			FText NAME_Triggers;
			FConstructorStatics()
				: TriggerTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_TriggerBox"))
					, ID_Triggers(TEXT("ScenarioTriggers"))
					, NAME_Triggers(NSLOCTEXT("SpriteCategory", "ScenarioTriggers", "ScenarioTriggers"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		GetSpriteComponent()->Sprite = ConstructorStatics.TriggerTextureObject.Get();
	}
}

/*
void AScenarioTriggerBox::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	UBoxComponent * BoxComponent = CastChecked<UBoxComponent>(GetRootComponent());
	if ( bCtrlDown )
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(0, 0, ModifiedScale.X);
		Extent.Z = FMath::Max<FVector::FReal>(0, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
	else
	{
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(ModifiedScale.X, ModifiedScale.Y, ModifiedScale.Z);
		Extent.X = FMath::Max<FVector::FReal>(0, Extent.X);
		Extent.Y = FMath::Max<FVector::FReal>(0, Extent.Y);
		Extent.Z = FMath::Max<FVector::FReal>(0, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
}
*/

void AScenarioTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	CastChecked<UBoxComponent>(GetCollisionComponent())->SetBoxExtent(Extent);
}

void AScenarioTriggerBox::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);
}

void AScenarioTriggerBox::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	FProperty* MemberProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();

	if (MemberProperty &&
		MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AScenarioTriggerBox, Extent))
	{
		UBoxComponent* BoxCollisionComponent = CastChecked<UBoxComponent>(GetCollisionComponent());
		BoxCollisionComponent->SetBoxExtent(Extent);
	}
}

const FSodaActorDescriptor* AScenarioTriggerBox::GenerateActorDescriptor() const
{

	static FSodaActorDescriptor Desc{
		TEXT("Trigger Box"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Triggers"), /*SubCategory*/
		TEXT("ClassIcon.TriggerBox"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}

//----------------------------------------------------------------------------------------------
AScenarioTriggerSphere::AScenarioTriggerSphere(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USphereComponent>(TEXT("CollisionComp")))
{
	USphereComponent* SphereCollisionComponent = CastChecked<USphereComponent>(GetCollisionComponent());

	SphereCollisionComponent->ShapeColor = TriggerBaseColor;
	SphereCollisionComponent->InitSphereRadius(SphereRadius);
	SphereCollisionComponent->SetCollisionProfileName(TriggerCollisionProfileName);
	if (UBillboardComponent* TriggerSpriteComponent = GetSpriteComponent())
	{
		TriggerSpriteComponent->SetupAttachment(SphereCollisionComponent);
	}

	if (GetSpriteComponent())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TriggerTextureObject;
			FName ID_Triggers;
			FText NAME_Triggers;
			FConstructorStatics()
				: TriggerTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/S_TriggerSphere"))
				, ID_Triggers(TEXT("ScenarioTriggers"))
				, NAME_Triggers(NSLOCTEXT("SpriteCategory", "ScenarioTriggers", "ScenarioTriggers"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		GetSpriteComponent()->Sprite = ConstructorStatics.TriggerTextureObject.Get();
	}
}

/*
void AScenarioTriggerSphere::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	USphereComponent * SphereComponent = CastChecked<USphereComponent>(GetRootComponent());
	SphereComponent->SetSphereRadius(FMath::Max<float>(0.0f, SphereComponent->GetUnscaledSphereRadius() + ModifiedScale.X));
}
*/

void AScenarioTriggerSphere::BeginPlay()
{
	Super::BeginPlay();

	CastChecked<USphereComponent>(GetCollisionComponent())->SetSphereRadius(SphereRadius);
}

void AScenarioTriggerSphere::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);
}

void AScenarioTriggerSphere::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AScenarioTriggerSphere, SphereRadius))
	{
		USphereComponent* SphereCollisionComponent = CastChecked<USphereComponent>(GetCollisionComponent());
		SphereCollisionComponent->SetSphereRadius(SphereRadius);
	}
}

const FSodaActorDescriptor* AScenarioTriggerSphere::GenerateActorDescriptor() const
{

	static FSodaActorDescriptor Desc{
		TEXT("Trigger Sphere"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Triggers"), /*SubCategory*/
		TEXT("ClassIcon.TriggerSphere"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}
