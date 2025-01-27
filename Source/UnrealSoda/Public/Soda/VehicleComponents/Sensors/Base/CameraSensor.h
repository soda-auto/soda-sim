// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Soda/Misc/PixelReader.h"
#include "Soda/SodaTypes.h"
#include "CameraSensor.generated.h"

class UExtraWindow;

class FCameraFrontBackAsyncTask;
class FCameraAsyncTask;

/**
 * ECameraSensorShader indicates which PostProcess shader (material) is applied to USceneCaptureComponent
 */
UENUM(BlueprintType)
enum class ECameraSensorShader : uint8
{
	ColorBGR8 = 0,
	Depth8 = 2,
	DepthFloat32 = 3,
	Depth16 = 4,
	SegmBGR8 = 5,
	Segm8 = 6,
	HdrRGB8 = 7,

	/** Color Filter Array.See https ://en.wikipedia.org/wiki/Color_filter_array */
	CFA = 8, 
};

/**
 * FCameraIntrinsics
 */
class FCameraIntrinsics
{
public:
	double K1 = 0;
	double K2 = 0;
	double K3 = 0;
	double K4 = 0;

	double Cx = 0.5;
	double Cy = 0.5;
	double Fx = 1;
	double Fy = 1;
};


/**
 *	Descriptor for the texture (frame) captured from a USceneCaptureComponent
 */
struct UNREALSODA_API FCameraFrame
{
	FCameraFrame();
	FCameraFrame(ECameraSensorShader Shader);

	uint32 Height = 0;
	uint32 Width = 0;

	/**  Valid only if Shader == DepthFloat32, [m] */
	float MaxDepthDistance = 0;
	//int64 Index = 0;
	//TTimestamp Timestamp{};

	void SetShader(ECameraSensorShader Shader);
	ECameraSensorShader GetShader() const { return Shader; }
	soda::EDataType GetDataType() const { return DType; }
	uint8 GetChannels() const { return Channels; }

	uint32 ComputeRawBufferSize() const;

	/** Be sure size of DstBuf >= FCameraFrame::ComputeRawBufferSize(Frame) */
	void ColorToRawBuffer(const TArray<FColor>& Color, uint8* DstBuf, uint32 ImageStride) const;

protected:
	ECameraSensorShader Shader;
	soda::EDataType DType;
	uint8 Channels;
};

/**
 * FCameraPostProcessMaterals
 */
USTRUCT()
struct FCameraPostProcessMaterals
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UMaterialInterface* DepthAbs;

	UPROPERTY()
	class UMaterialInterface* DepthAbsEnc;

	UPROPERTY()
	class UMaterialInterface* DepthNorm;

	UPROPERTY()
	class UMaterialInterface* DepthNormEnc;

	UPROPERTY()
	class UMaterialInterface* Segm;

	UPROPERTY()
	class UMaterialInterface* SegmColor;

	UPROPERTY()
	class UMaterialInterface* CFA;
};


/**
 * UCameraSensor
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCameraSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

	friend FCameraFrontBackAsyncTask;
	friend FCameraAsyncTask;

public:
	/** Controls what primitives get rendered into the scene capture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture)
	ESceneCapturePrimitiveRenderMode PrimitiveRenderMode;

	/** The actors to hide in the scene capture. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category=SceneCapture)
	TArray<AActor*> HiddenActors;

	/** The only actors to be rendered by this scene capture, if PrimitiveRenderMode is set to UseShowOnlyList.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category=SceneCapture)
	TArray<AActor*> ShowOnlyActors;
	
	/** Whether to persist the rendering state even if bCaptureEveryFrame==false.  This allows velocities for Motion Blur and Temporal AA to be computed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SceneCapture)
	bool bAlwaysPersistRenderingState = false;

	/** Scales the distance used by LOD. Set to values greater than 1 to cause the scene capture to use lower LODs than the main view to speed up the scene capture pass. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = ".1", UIMax = "10"), AdvancedDisplay)
	float LODDistanceFactor = 1.0f;

	/** if > 0, sets a maximum render distance override.  Can be used to cull distant objects from a reflection if the reflecting plane is in an enclosed area like a hallway or room */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture, meta=(UIMin = "100", UIMax = "10000"))
	float MaxViewDistanceOverride = -1;

	/** ShowFlags for the SceneCapture's ViewFamily, to control rendering settings for this view. Hidden but accessible through details customization */
	UPROPERTY(EditAnywhere, interp, Category=SceneCapture)
	TArray<struct FEngineShowFlagsSetting> ShowFlagSettings;

	// TODO: Make this a UStruct to set directly?
	/** Settings stored here read from the strings and int values in the ShowFlagSettings array */
	FEngineShowFlags ShowFlags;

	/** When enabled, the scene capture will composite into the render target instead of overwriting its contents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture)
	TEnumAsByte<enum ESceneCaptureCompositeMode> CompositeMode;

	UPROPERTY(interp, Category=PostProcessVolume, meta=(ShowOnlyInnerProperties))
	struct FPostProcessSettings PostProcessSettings;

	/** Range (0.0, 1.0) where 0 indicates no effect, 1 indicates full effect. */
	UPROPERTY(interp, Category=PostProcessVolume, BlueprintReadWrite, meta=(UIMin = "0.0", UIMax = "1.0"))
	float PostProcessBlendWeight = 1.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	ECameraSensorShader Format = ECameraSensorShader::ColorBGR8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	int Width = 800;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	int Height = 800;

	/** [m] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float MaxDepthDistance = 200.0;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bUseCustomAutoExposure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float AutoExposureMinBrightness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float AutoExposureMaxBrightness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float AutoExposureBias;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraSensor, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bForceLinearGamma = false;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	//bool bDrawDebugText = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogTick = false;

	/** {0} - will be replaced by datatime format - yyyy.mm.dd-hh.mm.ss */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Screenshot, SaveGame, meta = (EditInRuntime))
	FString DefaultSavePath = "{0}.png";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CEF, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FLinearColor CFA_0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CEF, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FLinearColor CFA_1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CEF, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FLinearColor CFA_2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CEF, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FLinearColor CFA_3;

	UPROPERTY(BlueprintReadOnly, Category = CameraSensor)
	UExtraWindow* CameraWindow;

	UPROPERTY(Transient)
	UTexture2D* CFATexture;

	UPROPERTY()
	FCameraPostProcessMaterals CameraPostProcessMaterals;

public:
	UFUNCTION(Category = CameraSensor, meta = (CallInRuntime))
	virtual void SaveCameraIntrinsicsAs() const;

	UFUNCTION(BlueprintCallable, Category = Screenshot, meta = (ScenarioAction))
	virtual void MakeScreenshot(const FString& InFileName);

	UFUNCTION(BlueprintCallable, Category = Screenshot, meta = (CallInRuntime, ScenarioAction))
	virtual void MakeScreenshotToDefault();

	UFUNCTION(BlueprintCallable, Category = Debug, meta = (CallInRuntime))
	virtual void ShowRenderTarget();

	virtual FCameraIntrinsics GetCameraIntrinsics() const;

	static bool IsColorFormat(ECameraSensorShader Fromat);

	virtual float GetHFOV() const { check(0); return 0; }
	virtual float GetVFOV() const { check(0); return 0; }
	
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& Frame, UTextureRenderTarget2D& RenderTarget, FRHICommandListImmediate& RHICmdList) { return false; }

	/** Will invoke if NeedPublishCPUData() is true */
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& Frame, const TArray<FColor>& BGRA8, uint32 ImageStride) { SyncDataset(); return false; }

	virtual bool NeedPublishCPUData() const { return true; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual UTextureRenderTarget2D* GetRenderTarget2DTexture() override;
	virtual bool NeedRenderSensorFOV() const;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

protected:
	virtual USceneCaptureComponent2D* GetSceneCaptureComponent2D() { check(0); return nullptr; }

	FCameraPixelReader PixelReader;
	FRenderCommandFence RenderFence;
	TSharedPtr <FCameraFrontBackAsyncTask> AsyncTask;
};

