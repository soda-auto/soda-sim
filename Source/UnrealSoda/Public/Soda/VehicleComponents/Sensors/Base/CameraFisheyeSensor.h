// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "GameFramework/SaveGame.h"
#include <future>
#include "CameraFisheyeSensor.generated.h"

UENUM(BlueprintType)
enum class  EFisheyeConstructMode : uint8
{
	OneFace,
	TowFaces,
	ThreeFaces,
	ThreeFacesLine,
	FourFaces,
	FiveFaces
};


class FFisheyeCameraModel: public FCameraIntrinsics
{
public:
	bool ProjectUV2XYZ(const FVector2D& UV, FVector& Normal) const;
	void ProjectXYZ2UV(const FVector& Point, FVector2D& UV) const;

protected:
	double R_D(double Theta) const;
	double R_D_Deriv(double Theta) const;
	double Kb_Compute_Theta_Nl(double Theta) const;

	const double Eps = 1e-6;
	const int N_Iter_Max = 10;
};

USTRUCT()
struct UNREALSODA_API FFisheyeSide
{
	GENERATED_USTRUCT_BODY()

	FFisheyeSide(){}

	FFisheyeSide(const FQuat& InCamRotator, float InFOV, int InWidth, int InHeight):
		CamRotator(InCamRotator), FOV(InFOV), Width(InWidth), Height(InHeight) {}

	UPROPERTY(Transient)
	UTexture2D* UVTexture = nullptr;

	UPROPERTY(Transient)
	USceneCaptureComponent2D * SceneCapture = nullptr;

	FQuat CamRotator; 
	float FOV;
	int Width;
	int Height;
};


UCLASS()
class UNREALSODA_API USceneCaptureDeferredComponent2D : public USceneCaptureComponent2D
{
	GENERATED_UCLASS_BODY()

public:
	void UpdateSceneCaptureContents(FSceneInterface* Scene) override;
    UWorld * CustomWorld = nullptr;
};

/**
 * UCameraFisheyeSensor
 * TODO: Make easy way to parametrisation of Fisheye projection construction  for user 
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCameraFisheyeSensor : public UCameraSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float BloomIntensity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	EFisheyeConstructMode FisheyeConstructMode = EFisheyeConstructMode::ThreeFacesLine;

	/** Intrinsics Distorion Vector D[0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsD0 = -0.0326545688;

	/** Intrinsics Distorion Vector D[1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsD1 = -0.0225569828;

	/** Intrinsics Distorion Vector D[2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsD2 = 0.009552358147;

	/** Intrinsics Distorion Vector D[3] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsD3 = -0.00172665024;

	/** Intrinsics Camera Matrix K[0][2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsCX = 960;

	/** Intrinsics Camera Matrix K[1][2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsCY = 604;

	/** Intrinsics Camera Matrix K[0][0] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsFX = 690;

	/** Intrinsics Camera Matrix K[1][1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float IntrinsicsFY = 690;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	float CenterFaceFOV = 65;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	float LeftRightFaceFOV = 65;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	float UpDownFaceFOV = 90;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int CenterFaceTexWidth = 850;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int CenterFaceTexHeight = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int LeftRightFaceTexWidth = 850;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int LeftRightFaceTexHeight = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int UpDownFaceTexWidth = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	int UpDownFaceTexHeight = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	bool bUseCustomProjectRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	FRotator CustomProjectRotation;

	/** A temporary file used to save the current projection map for loading in the future if the sensor is reactivated.
	  * This is useful for speeding up sensor initialization
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	bool bStoreTempProjMap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime))
	bool bDrawEdges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FisheyeProjection, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	float MaxFOV = 190;

	/**
	 * Effected only if the Format != HdrRGB8
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DrawDebugRings, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugRings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DrawDebugRings, SaveGame)
	TArray< float > DebugRings;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DrawDebugRings, SaveGame, meta = (EditInRuntime))
	float NewDebugRings = 60;

	UPROPERTY(interp, Category= FisheyePostProcess, meta=(ShowOnlyInnerProperties))
	struct FPostProcessSettings FisheyePostProcessSettings;

	UPROPERTY(BlueprintReadOnly, Category = CameraSensor)
	UMaterialInstanceDynamic * PostProcessMat;

	UPROPERTY(Transient, BlueprintReadOnly, Category = CameraSensor)
	class USceneCaptureDeferredComponent2D* CaptureComponent;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialDyn = nullptr;

	UWorld* CustomWorld = nullptr;

public:
	UFUNCTION(Category = FisheyeProjection, meta = (CallInRuntime))
	void RecalculateProjection();

	virtual FCameraIntrinsics GetCameraIntrinsics() const override { return FisheyeModel; }

	UFUNCTION(Category = DrawDebugRings, meta = (CallInRuntime))
	void AddRing() { DebugRings.Add(NewDebugRings); }

	UFUNCTION(Category = DrawDebugRings, meta = (CallInRuntime))
	void ClearRings() { DebugRings.Empty(); }

	virtual void MakeScreenshot(const FString& InFileName) override;


public:
	UCameraFisheyeSensor();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const { return Super::IsVehicleComponentInitializing() || bIsProjectionInitializing; }
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;

protected:
	virtual USceneCaptureComponent2D* GetSceneCaptureComponent2D() { return CaptureComponent; }
	virtual float GetHFOV() const { return MaxFOV; }
	virtual float GetVFOV() const { return MaxFOV / Width * Height; }

protected:
	void ComputeProjectionMap(TArray<FVector>& ProjMap) const;
	bool ApplyProjectionMap(const TArray<FVector>& ProjMap);
	void Clean();

	UPROPERTY()
	TArray<FFisheyeSide> FisheyeSides;

	UPROPERTY(Transient)
	UStaticMeshComponent* Mesh = nullptr;

	UPROPERTY(Transient)
	TArray<UMaterialInterface*> SqureMaterials;

	UPROPERTY(Transient)
	class UStaticMesh* StaticMeshSqure = nullptr;

	FQuat CommonQuat;

	FString TmpFileName;

	FFisheyeCameraModel FisheyeModel;

	bool bIsProjectionInitializing = false;
	std::future<void> InitializeProjectionFuture;
};
