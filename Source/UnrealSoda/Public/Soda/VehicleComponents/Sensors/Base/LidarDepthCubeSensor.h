// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarSensor.h"
#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "LidarDepthCubeSensor.generated.h"

class FLidarCubeFrontBackAsyncTask;
class FLidarCubeAsyncTask;


UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarDepthCubeSensor : public ULidarSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray< USceneCaptureComponent2D* >  SceneCaptureComponent2D;

public:
	UPROPERTY(interp, Category = Sensor, meta = (ShowOnlyInnerProperties))
	FPostProcessSettings LidarPostProcessSettings;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > HiddenActors;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > ShowOnlyActors;

	UPROPERTY(EditAnywhere, Category = PlanarReflection, meta = (UIMin = ".1", UIMax = "10"), AdvancedDisplay)
	float LODDistanceFactor = 1; 

	UPROPERTY(EditAnywhere, Category = Sensor, meta = (UIMin = "100", UIMax = "10000"))
	float MaxViewDistanceOverride = -1;

	UPROPERTY(EditAnywhere, Category = Sensor)
	int32 CaptureSortPriority = 1;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay)
	int Size = 512;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay)
	ELidarInterpolation Interpolation = ELidarInterpolation::Min;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay)
	float DepthMapNorm = 200.0;

public:
	virtual bool PublishBitmapData(const FCameraFrame& Frame, const TArray<FColor>& BGRA8, uint32 ImageStride) { return false; }

	struct FFace
	{
		TArray<FVector> Rays;
		TArray<FVector2D> UVs;
	};

	virtual const TArray<FFace>& GetLidarFases() const {  return Fases; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;


	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetPrecipitation(float Probability);

	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetNoize(float Probability, float MaxDistance);

	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetDepthMapMaxView(float Distance);

	UFUNCTION(BlueprintCallable, Category = Debug, meta = (CallInRuntime))
	virtual void ShowRenderTarget();

public:
	//ULidarDepthSensor2DComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual UTextureRenderTarget2D* GetRenderTarget2DTexture() override;

private:
	FCameraPixelReader PixelReader;
	FRenderCommandFence RenderFence;
	UMaterialInstanceDynamic* DepthMaterialDyn = NULL;

	
	FCameraFrame CameraFrame;
	TSharedPtr <FLidarCubeFrontBackAsyncTask> AsyncTask;

	TArray<FFace> Fases;

};
