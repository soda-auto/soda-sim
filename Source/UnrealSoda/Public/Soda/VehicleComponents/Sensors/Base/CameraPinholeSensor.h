// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "CameraPinholeSensor.generated.h"

UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCameraPinholeSensor : public UCameraSensor
{
	GENERATED_UCLASS_BODY()

public:
	/** Capture priority within the frame to sort scene capture on GPU to resolve interdependencies between multiple capture components. Highest come first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=SceneCapture)
	int32 CaptureSortPriority = 1;

	/** Camera field of view (in degrees). */
	UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category=CameraSensor, SaveGame, meta=(DisplayName = "Field of View", UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0",EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOVAngle = 90.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = CameraSensor)
	class USceneCaptureComponent2D* CaptureComponent;

	UPROPERTY(BlueprintReadOnly, Category = CameraSensor)
	UMaterialInstanceDynamic * PostProcessMat;

	virtual float GetHFOV() const { return FOVAngle; }
	virtual float GetVFOV() const { return FOVAngle / Width * Height; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;

protected:
	virtual USceneCaptureComponent2D* GetSceneCaptureComponent2D() { return CaptureComponent; }

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void Clean();
};

