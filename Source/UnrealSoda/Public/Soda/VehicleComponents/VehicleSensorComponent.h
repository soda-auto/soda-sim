// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "VehicleSensorComponent.generated.h"

/**
 * An auxiliary structure that describes the settings for rendering the FOV for a sensor.
 */

USTRUCT(BlueprintType)
struct FSensorFOVRenderer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	EFOVRenderingStrategy FOVRenderingStrategy = EFOVRenderingStrategy::OnSelect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FLinearColor Color = FLinearColor(0.13, 1.0, 0.3, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float MaxViewDistance = 200;
};

struct FSensorFOVMesh
{
	TArray<FDynamicMeshVertex> Vertices;
	TArray<uint32> Indices;
};

/**
 * Base abstract class for all sensors and equipment that can be placed on the vehicle.
 */
UCLASS(Abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API USensorComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Sensor)
	virtual void HideActorComponentsFromSensorView(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = Sensor)
	virtual void HideComponentFromSensorView(UPrimitiveComponent* PrimitiveComponent);

	/** 
	 * If this sensor has the ability to display any data to UTextureRenderTarget2D, then you can override this function.
	 * For example, a camera sensor can render display rgb image, and a lidar sensor can display a depth map here.
	 * The user from the UI will be able to see this texture.
	 */
	UFUNCTION(BlueprintCallable, Category = Sensor)
	virtual UTextureRenderTarget2D* GetRenderTarget2DTexture() { return nullptr; }

	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) { return false; }
	virtual UMaterialInterface* GetSensorFOVMaterial() const { return SensorFOVMaterial; }
	virtual bool NeedRenderSensorFOV() const { return false; }

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

protected:
	UPROPERTY()
	UMaterialInterface* SensorFOVMaterial;
};
