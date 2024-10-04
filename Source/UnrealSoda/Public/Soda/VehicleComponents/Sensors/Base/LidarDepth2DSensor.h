// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/LidarSensor.h"
#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "LidarDepth2DSensor.generated.h"

class UExtraWindow;

class FLidar2DFrontBackAsyncTask;
class FLidar2DAsyncTask;

UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarDepth2DSensor : public ULidarSensor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	USceneCaptureComponent2D*  SceneCaptureComponent2D;

public:
	//UPROPERTY(EditAnywhere, Category = Sensor)
	//TArray< AActor* > HiddenActors;

	//UPROPERTY(EditAnywhere, Category = Sensor)
	//TArray< AActor* > ShowOnlyActors;

	//UPROPERTY(EditAnywhere, Category = PlanarReflection, meta = (UIMin = ".1", UIMax = "10"), AdvancedDisplay)
	//float LODDistanceFactor = 1;

	//UPROPERTY(EditAnywhere, Category = Sensor, meta = (UIMin = "100", UIMax = "10000"))
	//float MaxViewDistanceOverride = -1;

	UPROPERTY(EditAnywhere, Category = Sensor)
	int32 CaptureSortPriority = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int TextureWidth = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int TextureHeight = 1208;

	/** [deg] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float CameraFOV = 65;

	/** Normalizing coefficient for the post process material  [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DepthMapNorm = 20000.0;

	UPROPERTY(interp, Category = Sensor, meta = (ShowOnlyInnerProperties))
	FPostProcessSettings LidarPostProcessSettings;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	ELidarInterpolation Interpolation = ELidarInterpolation::Min;

	UPROPERTY(BlueprintReadOnly, Category = Sensor)
	UExtraWindow* CameraWindow = nullptr;

public:
	virtual bool PublishBitmapData(const FCameraFrame& Frame, const TArray<FColor>& BGRA8, uint32 ImageStride) { return false; }
	virtual const TArray<FVector2D>& GetLidarUVs() const { static TArray<FVector2D> Dummy;  return Dummy; }

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
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual UTextureRenderTarget2D* GetRenderTarget2DTexture() override;

	// Return num of skipped points 
	static int GenerateUVs(TArray<FVector>& InOutLidarRays, float FOVAngle, int Width, int Height, ELidarInterpolation Interpolation, TArray<FVector2D> & UVs);

protected:
	FCameraPixelReader PixelReader;
	FRenderCommandFence RenderFence;
	UMaterialInstanceDynamic* DepthMaterialDyn = NULL;
	FCameraFrame CameraFrame;
	TSharedPtr <FLidar2DFrontBackAsyncTask> AsyncTask;
};
