#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "ConstructorHelpers.h"

UDummyComponent::UDummyComponent(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer), bIsActivated(false)
{
   GUI.Category = TEXT("Other");
   GUI.ComponentNameOverride = TEXT("Dummy component");
   GUI.bIsPresentInAddMenu = true;

   DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
   DummyMesh->SetupAttachment(this);

   UUID = "";

   InitializeDummyMeshMap();
   CurrentMeshComponent = nullptr;
}

void UDummyComponent::UpdateDummyLocation(const FVector& NewLocation)
{
   SetRelativeLocation(NewLocation);
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
   ConstructorHelpers::FObjectFinder<UStaticMesh> Emotor(TEXT("/SodaSim/DummyComponents/Mesh/Emotor.Emotor"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh3(TEXT("/SodaSim/DummyComponents/Cylinder.Cylinder"));

   if (ECUMesh.Succeeded()) DummyMeshMap.Add(EDummyType::ECU, ECUMesh.Object);
   if (Emotor.Succeeded()) DummyMeshMap.Add(EDummyType::Emotor, Emotor.Object);
   if (Mesh3.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType3, Mesh3.Object);
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
