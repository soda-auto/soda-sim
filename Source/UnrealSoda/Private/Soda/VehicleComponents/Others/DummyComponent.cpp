#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/StaticMesh.h"
#include "ConstructorHelpers.h"

UDummyComponent::UDummyComponent(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer), bIsActivated(false)
{
   GUI.Category = TEXT("Other");
   GUI.ComponentNameOverride = TEXT("Dummy component");
   GUI.bIsPresentInAddMenu = true;

   DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
   DummyMesh->SetupAttachment(this);

   InitializeDummyMeshMap();
   CurrentMeshComponent = nullptr;
}

void UDummyComponent::UpdateDummyLocation(const FVector& NewLocation)
{
   if (CurrentMeshComponent)
   {
      CurrentMeshComponent->SetRelativeLocation(NewLocation);
   }
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
   ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh1(TEXT("/SodaSim/DummyComponents/Cube.Cube"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh2(TEXT("/SodaSim/DummyComponents/Sphere.Sphere"));
   ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh3(TEXT("/SodaSim/DummyComponents/Cylinder.Cylinder"));

   if (Mesh1.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType1, Mesh1.Object);
   if (Mesh2.Succeeded()) DummyMeshMap.Add(EDummyType::DummyType2, Mesh2.Object);
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
