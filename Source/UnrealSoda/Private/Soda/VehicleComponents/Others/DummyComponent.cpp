#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

UDummyComponent::UDummyComponent(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer), bIsActivated(false), bShouldMove(false), TargetLocation(FVector::ZeroVector)
{
   GUI.Category = TEXT("Other");
   GUI.ComponentNameOverride = TEXT("Dummy component");
   GUI.bIsPresentInAddMenu = true;

   DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
   DummyMesh->SetupAttachment(this);

   UUID = "";

   InitializeDummyMeshMap();
   CurrentMeshComponent = nullptr;

   PrimaryComponentTick.bCanEverTick = true;

   TargetLocation = GetRelativeLocation();
}

void UDummyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   if (bIsActivated && bShouldMove)
   {
      FVector CurrentLocation = GetRelativeLocation();
      if (!CurrentLocation.Equals(TargetLocation, 0.1f))
      {
         FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 0.4f);
         SetRelativeLocation(NewLocation);
      }
      else
      {
         bShouldMove = false;
      }

   }

   if (bIsActivated && !LabelText.IsEmpty()) // Add check for LabelText
   {
      FVector ComponentLocation = GetComponentLocation();
      float Scale = 1.0f;
      DrawDebugString(GetWorld(), ComponentLocation + FVector(0, 0, 10), LabelText, nullptr, FColor::White, 0.0001f, false, Scale);
   }
}

void UDummyComponent::UpdateDummyLocation(const FVector& NewLocation)
{
   TargetLocation = NewLocation;
   bShouldMove = true;
}

void UDummyComponent::SetLabelText(const FString& NewLabelText)
{
   LabelText = NewLabelText;
}

bool UDummyComponent::OnActivateVehicleComponent()
{
   if (!Super::OnActivateVehicleComponent())
   {
      return false;
   }

   bIsActivated = true;
   CreateAndAttachStaticMesh();

   return true;
}

void UDummyComponent::OnDeactivateVehicleComponent()
{
   Super::OnDeactivateVehicleComponent();
   bIsActivated = false;
   RemoveCurrentMeshComponent();
}

void UDummyComponent::InitializeDummyMeshMap()
{
   ConstructorHelpers::FObjectFinder<UStaticMesh> ECUMesh(TEXT("/SodaSim/DummyComponents/Mesh/ECU.ECU"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> EmotorMesh(TEXT("/SodaSim/DummyComponents/Mesh/Emotor.Emotor"));

   ConstructorHelpers::FObjectFinder<UStaticMesh> FrontLeftLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/FrontLeftLight.FrontLeftLight"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> FrontRightLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/FrontRightLight.FrontRightLight"));

   ConstructorHelpers::FObjectFinder<UStaticMesh> RearLeftPositionLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearLeftPositionLight.RearLeftPositionLight"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> RearLeftReverseLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearLeftReverseLight.RearLeftReverseLight"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> RearLeftTurnIndicatorMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearLeftTurnIndicator.RearLeftTurnIndicator"));

   ConstructorHelpers::FObjectFinder<UStaticMesh> RearRightPositionLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearRightPositionLight.RearRightPositionLight"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> RearRightReverseLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearRightReverseLight.RearRightReverseLight"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> RearRightTurnIndicatorMesh(TEXT("/SodaSim/DummyComponents/Mesh/RearRightTurnIndicator.RearRightTurnIndicator"));

   ConstructorHelpers::FObjectFinder<UStaticMesh> RoofLightMesh(TEXT("/SodaSim/DummyComponents/Mesh/RoofLight.RoofLight"));


   if (ECUMesh.Succeeded()) DummyMeshMap.Add(EDummyType::ECU, ECUMesh.Object);
   if (EmotorMesh.Succeeded()) DummyMeshMap.Add(EDummyType::Emotor, EmotorMesh.Object);

   if (FrontLeftLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::FrontLeftLight, FrontLeftLightMesh.Object);
   if (FrontRightLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::FrontRightLight, FrontRightLightMesh.Object);

   if (RearLeftPositionLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearLeftPositionLight, RearLeftPositionLightMesh.Object);
   if (RearLeftReverseLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearLeftReverseLight, RearLeftReverseLightMesh.Object);
   if (RearLeftTurnIndicatorMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearLeftTurnIndicator, RearLeftTurnIndicatorMesh.Object);

   if (RearRightPositionLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearRightPositionLight, RearRightPositionLightMesh.Object);
   if (RearRightReverseLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearRightReverseLight, RearRightReverseLightMesh.Object);
   if (RearRightTurnIndicatorMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RearRightTurnIndicator, RearRightTurnIndicatorMesh.Object);

   if (RoofLightMesh.Succeeded()) DummyMeshMap.Add(EDummyType::RoofLight, RoofLightMesh.Object);
}

void UDummyComponent::CreateAndAttachStaticMesh()
{
   RemoveCurrentMeshComponent();

   if (DummyMeshMap.Contains(DummyType))
   {
      CurrentMeshComponent = NewObject<UStaticMeshComponent>(this);
      if (CurrentMeshComponent)
      {
         CurrentMeshComponent->SetStaticMesh(*DummyMeshMap.Find(DummyType));
         CurrentMeshComponent->SetupAttachment(this);
         CurrentMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
         CurrentMeshComponent->RegisterComponent();
         CurrentMeshComponent->SetVisibility(true);
      }
   }
}

void UDummyComponent::RemoveCurrentMeshComponent()
{
   if (CurrentMeshComponent)
   {
      CurrentMeshComponent->DestroyComponent();
      CurrentMeshComponent = nullptr;
   }
}

void UDummyComponent::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
   Super::RuntimePostEditChangeProperty(PropertyChangedEvent);

   if (bIsActivated)
   {
      FProperty* Property = PropertyChangedEvent.Property;
      const FName PropertyName = Property ? Property->GetFName() : NAME_None;

      if (PropertyName == GET_MEMBER_NAME_CHECKED(UDummyComponent, DummyType))
      {
         CreateAndAttachStaticMesh();
      }
   }
}

void UDummyComponent::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
   Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

   if (bIsActivated)
   {
      FProperty* Property = PropertyChangedEvent.Property;
      const FName PropertyName = Property ? Property->GetFName() : NAME_None;

      if (PropertyName == GET_MEMBER_NAME_CHECKED(UDummyComponent, DummyType))
      {
         CreateAndAttachStaticMesh();
      }
   }
}
