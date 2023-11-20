// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleAnimationInstance.h"
#include "WheelsRendering.generated.h"

class ASodaWheeledVehicle;

/**
 * UWheelsAnimationComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UWheelsRenderingComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Custom static wheel mesh  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering)
	UStaticMesh *FrontWheelMesh;

	/** Custom static wheel mesh  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering)
	UStaticMesh *RearWheelMesh;

	/** Flip wheel mesh  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering, SaveGame, meta = (EditInRuntime))
	bool bFlipWheelsMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering, SaveGame, meta = (EditInRuntime))
	bool bStagecoachEffect = false;

	/** Custom static wheel mesh components */
	UPROPERTY(transient, duplicatetransient, BlueprintReadOnly, Category = WheelsRendering)
	TArray<UStaticMeshComponent *> WheelMesheComponents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering, SaveGame, meta = (EditInRuntime))
	float ShutterSpeed = 30.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering, SaveGame, meta = (EditInRuntime))
	float MaxAngularVelocity = 256.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelsRendering, SaveGame, meta = (EditInRuntime))
	int WheelSpokeCount = 5;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	TArray<FWheelAnimationData> WheelsAnimationData;
};
