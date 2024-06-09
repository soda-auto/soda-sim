﻿#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "DummyComponent.generated.h"

UENUM(BlueprintType)
enum class EDummyType : uint8
{
	DummyType1 UMETA(DisplayName = "Dummy Type 1"),
	DummyType2 UMETA(DisplayName = "Dummy Type 2"),
	DummyType3 UMETA(DisplayName = "Dummy Type 3")
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

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

private:
	UPROPERTY()
	TMap<EDummyType, UStaticMesh*> DummyMeshMap;

	UPROPERTY()
	UStaticMeshComponent* CurrentMeshComponent;

	void InitializeDummyMeshMap();
	void CreateAndAttachStaticMesh();
	void RemoveCurrentMeshComponent();
};