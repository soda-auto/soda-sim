// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/CameraSensor.h"
#include "Engine/Scene.h"
#include "Soda/Transport/GenericLidarPublisher.h"
#include "Soda/Transport/GenericCameraPublisher.h"
#include "LidarDepthSensorCube.generated.h"

UENUM(BlueprintType)
enum class ELidarInterpolation : uint8
{
	Bilinear,
	Min,
	Nearest
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarDepthSensorCubeComponent : public USensorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray< USceneCaptureComponent2D* >  SceneCaptureComponent2D;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericLidarPublisher PointCloudPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TObjectPtr<UGenericCameraPublisher> DepthMapPublisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool PublishDepthMap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float Period = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Channels = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int StepToFace = 250;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float FOV_HorizontMin = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float FOV_HorizontMax = 360;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FOV_VerticalMin = -15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FOV_VerticalMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DistanseMin = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DistanseMax = 12000;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float AbsorptionCoefficient = 0.005;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float ReflectionCoefficient = 0.8;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float LaserIntensity = 100000;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float SensorSize = 0.01;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//float SeaLevel = 10140.0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	//int FacePublish = 0;

	UPROPERTY(interp, Category = Sensor, meta = (ShowOnlyInnerProperties))
	FPostProcessSettings LidarPostProcessSettings;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > HiddenActors;

	UPROPERTY(EditAnywhere, Category = Sensor)
	TArray< AActor* > ShowOnlyActors;

	/*
	UPROPERTY(EditAnywhere, Category=Sensor)
	bool bCaptureEveryFrame = true;

	UPROPERTY(EditAnywhere, Category=Sensor)
	bool bCaptureOnMovement = true;
	*/

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

	//UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay)
	//bool ShowDebug = false;

	UPROPERTY(EditAnywhere, Category = Sensor, AdvancedDisplay)
	float DepthMapNorm = 200.0;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString & Info) const override;
	//virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override; // TODO
	//virtual bool NeedRenderSensorFOV() const; // TODO

	UFUNCTION(BlueprintCallable, Category = Sensor)
	void SetPrecipitation(float Probability);

	UFUNCTION(BlueprintCallable, Category = Sensor)
	void SetNoize(float Probability, float MaxDistance);

	UFUNCTION(BlueprintCallable, Category = Sensor)
	void SetDepthMapMaxView(float Distance);

public:
	ULidarDepthSensorCubeComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FCameraPixelReader PixelReader;
	FRenderCommandFence RenderFence;
	int FaceReady;
	std::vector< float > Map;
	UMaterialInstanceDynamic* DepthMaterialDyn = NULL;
	FCameraFrame CameraFrame;
	std::vector<soda::LidarScanPoint> Points;
};
