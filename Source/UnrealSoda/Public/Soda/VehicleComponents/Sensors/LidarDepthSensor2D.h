// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/LidarDepthSensorCube.h"
#include "LidarDepthSensor2D.generated.h"

class UExtraWindow;

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarDepthSensor2DComponent : public USensorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	USceneCaptureComponent2D*  SceneCaptureComponent2D;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericLidarPublisher PointCloudPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TObjectPtr<UGenericCameraPublisher> DepthMapPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bPublishDepthMap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Channels = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Step = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMin = -15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_VerticalMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_Horizontal = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMin = 50; // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMax = 20000; // cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > HiddenActors;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > ShowOnlyActors;

	UPROPERTY(EditAnywhere, Category = PlanarReflection, meta = (UIMin = ".1", UIMax = "10"), AdvancedDisplay)
	float LODDistanceFactor = 1;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay, SaveGame, meta = (EditInRuntime))
	float DepthMapNorm = 200.0;

	UPROPERTY(EditAnywhere, Category = Sensor, meta = (UIMin = "100", UIMax = "10000"))
	float MaxViewDistanceOverride = -1;

	UPROPERTY(EditAnywhere, Category = Sensor)
	int32 CaptureSortPriority = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Width = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Height = 1208;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FOVAngle = 65;

	UPROPERTY(interp, Category = Sensor, meta = (ShowOnlyInnerProperties))
	FPostProcessSettings LidarPostProcessSettings;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	ELidarInterpolation Interpolation = ELidarInterpolation::Min;

	UPROPERTY(BlueprintReadOnly, Category = Sensor)
	UExtraWindow* CameraWindow;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString & Info) const override;
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetPrecipitation(float Probability);

	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetNoize(float Probability, float MaxDistance);

	UFUNCTION(BlueprintCallable, Category = Sensor, meta = (ScenarioAction))
	void SetDepthMapMaxView(float Distance);

	UFUNCTION(BlueprintCallable, Category = Debug, meta = (CallInRuntime))
	virtual void ShowRenderTarget();

public:
	ULidarDepthSensor2DComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual UTextureRenderTarget2D* GetRenderTarget2DTexture() override;

protected:
	FCameraPixelReader PixelReader;
	FRenderCommandFence RenderFence;
	std::vector< float > Map;
	UMaterialInstanceDynamic* DepthMaterialDyn = NULL;

	TArray<FVector2D> UVs;
	TArray<FVector> LidarRays;

	FCameraFrame CameraFrame;
	std::vector<soda::LidarScanPoint> Points;
};
