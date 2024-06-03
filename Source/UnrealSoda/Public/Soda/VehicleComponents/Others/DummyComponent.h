// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "DummyComponent.generated.h"

class UStaticMesh;
 /**
  *
  */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UDummyComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** Custom static dummy mesh  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DummyRendering)
	UStaticMesh* DummyMesh;
	
protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

	void SetDummyMesh(UStaticMesh* NewMesh);
};
