// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "DummyComponent.generated.h"

UENUM(BlueprintType)
enum class EDummyType : uint8
{
   ECU UMETA(DisplayName = "ECU"),
   Emotor UMETA(DisplayName = "Emotor"),
   FrontLeftLight UMETA(DisplayName = "Front left light"),
   FrontRightLight UMETA(DisplayName = "Front right light"),
   RearLeftPositionLight UMETA(DisplayName = "Rear left position light"),
   RearLeftReverseLight UMETA(DisplayName = "Rear left reverse light"),
   RearLeftTurnIndicator UMETA(DisplayName = "Rear left turn indicator"),
   RearRightPositionLight UMETA(DisplayName = "Rear right position light"),
   RearRightReverseLight UMETA(DisplayName = "Rear right reverse light"),
   RearRightTurnIndicator UMETA(DisplayName = "Rear right turn indicator"),
   RoofLight UMETA(DisplayName = "Roof light")
};

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UDummyComponent : public UVehicleBaseComponent
{
   GENERATED_UCLASS_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DummyProperties, meta = (EditInRuntime))
   EDummyType DummyType;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mesh)
   UStaticMeshComponent* DummyMesh;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Identification)
   FString UUID;

   void UpdateDummyLocation(const FVector& NewLocation);
   void SetLabelText(const FString& NewLabelText);
   bool IsInPlace() const;

protected:
   virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
   virtual bool OnActivateVehicleComponent() override;
   virtual void OnDeactivateVehicleComponent() override;
   virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
   virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

private:
   UPROPERTY()
   TMap<EDummyType, UStaticMesh*> DummyMeshMap;

   UPROPERTY()
   UStaticMeshComponent* CurrentMeshComponent;

   bool bIsActivated;
   bool bShouldMove;

   FVector TargetLocation;
   FString LabelText;

   void InitializeDummyMeshMap();
   void CreateAndAttachStaticMesh();
   void RemoveCurrentMeshComponent();
};

