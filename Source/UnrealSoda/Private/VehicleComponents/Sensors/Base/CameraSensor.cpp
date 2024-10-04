// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Public/SceneView.h"
#include "Runtime/RHI/Public/RHICommandList.h"
//#include "Runtime/Renderer/Private/PostProcess/SceneRenderTargets.h"
#include "Brushes/SlateImageBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Soda/SodaStatics.h"
#include "Async/ParallelFor.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "KismetProceduralMeshLibrary.h"
#include "ImagePixelData.h"
#include "ImageWriteTask.h"
#include "HighResScreenshot.h"
#include "ImageWriteQueue.h"
#include "DesktopPlatformModule.h"
#include "Soda/Misc/ExtraWindow.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IStructureDetailsView.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"
#include "Async/ParallelFor.h"

/*
DECLARE_STATS_GROUP(TEXT("CameraSensor"), STATGROUP_CameraSensor, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_CameraTick, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("Publish"), STAT_CameraPublish, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("Compute"), STAT_CameraCompute, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("ReadPixels"), STAT_CameraRead, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("BigBlockMemcpy"), STAT_CameraBigBlockMemcpy, STATGROUP_CameraSensor);
*/

static FString WrapParamStingToYaml(const FString& ParamName, const FString& String, int Indent)
{
	FString Yaml;
	for (int i = 0; i < Indent; ++i)
	{
		Yaml.Append(" ");
	}

	if (!ParamName.IsEmpty())
	{
		Yaml += ParamName + ": ";
	}

	Yaml.Append(String);
	Yaml.Append("\r\n");
	return Yaml;
}


/***********************************************************************************************
	FCameraAsyncTask
***********************************************************************************************/
class FCameraAsyncTask : public soda::FAsyncTask
{
public:
	virtual ~FCameraAsyncTask() {}
	virtual FString ToString() const override { return "CameraAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override
	{
		check(!bIsDone);
		check(ImageBuffer.Num() > 0 && uint32(ImageBuffer.Num()) >= ImageStride * CameraFrame.Height);
		Sensor->PublishSensorData(DeltaTime, Header, CameraFrame, ImageBuffer, ImageStride);
		bIsDone = true;
	}

public:
	float DeltaTime{};
	FSensorDataHeader Header{};
	FCameraFrame CameraFrame;
	TArray<FColor> ImageBuffer;
	uint32 ImageStride = 0; // Original texture width in pixels
	TWeakObjectPtr<UCameraSensor> Sensor;

protected:
	bool bIsDone = true;
};


/***********************************************************************************************
	FCameraFrontBackAsyncTask
***********************************************************************************************/
class FCameraFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FCameraAsyncTask>
{
public:
	FCameraFrontBackAsyncTask(UCameraSensor* Sensor)
	{
		FrontTask->Sensor = Sensor;
		BackTask->Sensor = Sensor;
	}
	virtual ~FCameraFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "CameraFrontBackAsyncTask"; }
};


/***********************************************************************************************
	FCameraFrame
***********************************************************************************************/
FCameraFrame::FCameraFrame()
{
	SetShader(ECameraSensorShader::ColorBGR8);
}

FCameraFrame::FCameraFrame(ECameraSensorShader InShader)
{
	SetShader(InShader);
}

void FCameraFrame::SetShader(ECameraSensorShader InShader)
{
	Shader = InShader;
	switch (Shader)
	{
	case ECameraSensorShader::SegmBGR8:
	case ECameraSensorShader::ColorBGR8:
	case ECameraSensorShader::HdrRGB8:
	case ECameraSensorShader::CFA:
		DType = soda::EDataType::CV_8U;
		Channels = 3;
		break;

	case ECameraSensorShader::Segm8:
	case ECameraSensorShader::Depth8:
		DType = soda::EDataType::CV_8U;
		Channels = 1;
		break;

	case ECameraSensorShader::DepthFloat32:
		DType = soda::EDataType::CV_32F;
		Channels = 1;
		break;

	case ECameraSensorShader::Depth16:
		DType = soda::EDataType::CV_16U;
		Channels = 1;
		break;

	default:
		check(0);
	}
}

uint32 FCameraFrame::ComputeRawBufferSize() const
{
	return soda::GetDataTypeSize(GetDataType()) * GetChannels() * Height * Width;
}

void FCameraFrame::ColorToRawBuffer(const TArray<FColor>& ColorBuf, uint8* DstBuf, uint32 ImageStride) const
{
	switch (GetShader())
	{
	case ECameraSensorShader::Segm8:
		ParallelFor(Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Width; j++)
				{
					DstBuf[i * Width + j] = ColorBuf[i * ImageStride + j].R;  //Only R cnannel
				}
			});
		break;

	case ECameraSensorShader::ColorBGR8:
	case ECameraSensorShader::SegmBGR8:
	case ECameraSensorShader::HdrRGB8:
	case ECameraSensorShader::CFA:
		ParallelFor(Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Width; j++)
				{
					uint32 out_ind = (i * Width + j) * 3;
					uint32 in_ind = (i * ImageStride + j);
					*(uint32*)&DstBuf[out_ind + 0] = *(uint32*)&ColorBuf[in_ind + 0];
				}
			});
		break;


	case ECameraSensorShader::DepthFloat32:
		ParallelFor(Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Width; j++)
				{
					((float*)DstBuf)[i * Width + j] = Rgba2Float(ColorBuf[ImageStride * i + j]) * MaxDepthDistance;
				}
			});
		break;


	case ECameraSensorShader::Depth16:
		ParallelFor(Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Width; j++)
				{
					((uint16_t*)DstBuf)[i * Width + j] = static_cast<uint16_t>(Rgba2Float(ColorBuf[ImageStride * i + j]) * 0xFFFF + 0.5f);
				}
			});
		break;

	case ECameraSensorShader::Depth8:
		ParallelFor(Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Width; j++)
				{
					DstBuf[i * Width + j] = static_cast<uint8_t>(Rgba2Float(ColorBuf[ImageStride * i + j]) * 0xFF + 0.5f);
				}
			});
		break;
	}
}



/* **********************************************************************************************
 * UCameraSensor
 * **********************************************************************************************/

UCameraSensor::UCameraSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ShowFlags(FEngineShowFlags(ESFIM_Game))
{
	GUI.Category = TEXT("Camera Sensors");
	GUI.ComponentNameOverride = TEXT("Base Camera");
	GUI.IcanName = TEXT("SodaIcons.Camera");
	GUI.bIsPresentInAddMenu = false;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;

	//ShowFlags.SetMotionBlur(0);
	//ShowFlags.SetSeparateTranslucency(0);
	//ShowFlags.SetHMDDistortion(0);

	//CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	
	//PostProcessSettings.AutoExposureMinBrightness = 0.3;
	//PostProcessSettings.AutoExposureMaxBrightness = 0.6;
	//PostProcessSettings.AutoExposureBias = 0.4;
	//PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	//PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	//PostProcessSettings.bOverride_AutoExposureBias = true;

	AutoExposureMinBrightness = PostProcessSettings.AutoExposureMinBrightness;
	AutoExposureMaxBrightness = PostProcessSettings.AutoExposureMaxBrightness;
	AutoExposureBias = PostProcessSettings.AutoExposureBias;

	static ConstructorHelpers::FObjectFinder< UMaterial > CameraDepthAbsMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraDepthAbs"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraDepthAbsEncMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraDepthAbsEnc"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraDepthNormMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraDepthNorm"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraDepthNormEncMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraDepthNormEnc"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraSegmMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraSegm"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraSegmColorMatPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CameraSegmColor"));
	static ConstructorHelpers::FObjectFinder< UMaterial > CameraCFAPtr(TEXT("/SodaSim/Assets/CPP/CameraPostProcess/CFA"));

	if (CameraDepthAbsMatPtr.Succeeded())     CameraPostProcessMaterals.DepthAbs = CameraDepthAbsMatPtr.Object;
	if (CameraDepthAbsEncMatPtr.Succeeded())  CameraPostProcessMaterals.DepthAbsEnc = CameraDepthAbsEncMatPtr.Object;
	if (CameraDepthNormMatPtr.Succeeded())    CameraPostProcessMaterals.DepthNorm = CameraDepthNormMatPtr.Object;
	if (CameraDepthNormEncMatPtr.Succeeded()) CameraPostProcessMaterals.DepthNormEnc = CameraDepthNormEncMatPtr.Object;
	if (CameraSegmMatPtr.Succeeded())         CameraPostProcessMaterals.Segm = CameraSegmMatPtr.Object;
	if (CameraSegmColorMatPtr.Succeeded())    CameraPostProcessMaterals.SegmColor = CameraSegmColorMatPtr.Object;
	if (CameraCFAPtr.Succeeded())             CameraPostProcessMaterals.CFA = CameraCFAPtr.Object;
}

bool UCameraSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	AsyncTask = MakeShareable(new FCameraFrontBackAsyncTask(this));
	AsyncTask->Start();
	SodaApp.CamTaskManager.AddTask(AsyncTask);


	// Create CFA Texture
	/*
	CFATexture = UTexture2D::CreateTransient(2, 2, EPixelFormat::PF_B8G8R8A8);
	checkf(CFATexture);
	CFATexture->AddressX = TextureAddress::TA_Clamp;
	CFATexture->AddressY = TextureAddress::TA_Clamp;

	struct FTexData { uint8 B; uint8 G; uint8 R; uint8 A; };
	check(sizeof(FTexData) == 4);

	FTexData* TextureData = (FTexData*)TextureData->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	check(TextureData);

	TextureData[0] = CFA_0.ToFColor();
	TextureData[1] = CFA_1.ToFColor();
	TextureData[2] = CFA_2.ToFColor();
	TextureData[3] = CFA_3.ToFColor();

	TextureData->PlatformData->Mips[0].BulkData.Unlock();
	*/

	return true;

}

void UCameraSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	RenderFence.Wait();

	AsyncTask->Finish();
	SodaApp.CamTaskManager.RemoteTask(AsyncTask);
	AsyncTask.Reset();

	if (IsValid(CameraWindow)) CameraWindow->Close();
	CameraWindow = nullptr;
}

void UCameraSensor::SaveCameraIntrinsicsAs() const
{
	FCameraIntrinsics Intrinsics = GetCameraIntrinsics();

	FString Yaml;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("UCameraSensor::SaveCameraIntrinsicsAs() Can't get the IDesktopPlatform ref"));
		return;
	}

	const FString FileTypes = TEXT("Intrinsics YAML (*.yaml)|*.yaml");

	TArray<FString> OpenFilenames;
	if (!DesktopPlatform->SaveFileDialog(nullptr, TEXT("Export Intrinsics to YAML"), FPaths::GetProjectFilePath(), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames) || OpenFilenames.Num() <= 0)
	{
		return;
	}

	int Indent = 4;

	Yaml += WrapParamStingToYaml("version", "1.1", 0);
	Yaml += WrapParamStingToYaml("frame_width", FString::FromInt(Width), 0);
	Yaml += WrapParamStingToYaml("frame_height", FString::FromInt(Height), 0);

	Yaml += WrapParamStingToYaml("K", "!!opencv-matrix", 0);
	Yaml += WrapParamStingToYaml("rows", "3", Indent);
	Yaml += WrapParamStingToYaml("cols", "3", Indent);
	Yaml += WrapParamStingToYaml("dt", "d", Indent);

	//wrap K matrix
	int DataIndent = Indent + 7;
	FString Data;
	Data.Append(" [ ");
	Data.Append(FString::SanitizeFloat(Intrinsics.Fx) + ", " + "0., " + FString::SanitizeFloat(Intrinsics.Cx) + ",\r\n");

	for (int i = 0; i < DataIndent; ++i)
	{
		Data.Append(" ");
	}

	Data.Append("0., " + FString::SanitizeFloat(Intrinsics.Fy) + ", " + FString::SanitizeFloat(Intrinsics.Cy) + ",\r\n");
	for (int i = 0; i < DataIndent; ++i)
	{
		Data.Append(" ");
	}

	Data.Append("0., 0., 1.");
	Data.Append(" ]\r\n");

	Yaml += WrapParamStingToYaml("data", Data, Indent);

	//wrap D matrix
	
	Yaml += WrapParamStingToYaml("D", "!!opencv-matrix", 0);
	Yaml += WrapParamStingToYaml("rows", "5", Indent);
	Yaml += WrapParamStingToYaml("cols", "1", Indent);
	Yaml += WrapParamStingToYaml("dt", "d", Indent);
	Yaml += WrapParamStingToYaml("data", 
		"[ " + FString::SanitizeFloat(Intrinsics.K1) +
		", " + FString::SanitizeFloat(Intrinsics.K2) +
		", " + FString::SanitizeFloat(Intrinsics.K3) +
		", " + FString::SanitizeFloat(Intrinsics.K4) +
		", " + FString::SanitizeFloat(0) +
		" ]\r\n", Indent);

	USodaStatics::WriteStringToFile(OpenFilenames[0], Yaml, true);
	return;
}

UTextureRenderTarget2D* UCameraSensor::GetRenderTarget2DTexture()
{
	if (GetSceneCaptureComponent2D() != nullptr)
		return GetSceneCaptureComponent2D()->TextureTarget;
	return nullptr;
}

void UCameraSensor::ShowRenderTarget()
{
	if (!IsValid(CameraWindow))
	{
		CameraWindow = NewObject<UExtraWindow>();
	}
	if (GetSceneCaptureComponent2D() && GetSceneCaptureComponent2D()->TextureTarget)
	{
		CameraWindow->OpenCameraWindow(GetSceneCaptureComponent2D()->TextureTarget);
	}
}

void UCameraSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	//SCOPE_CYCLE_COUNTER(STAT_CameraTick);


	GetWorld()->GetCanvasForRenderingToTarget(); // Create Canvas if needed

	FCameraFrame CameraFrame(Format);
	CameraFrame.Height = GetSceneCaptureComponent2D()->TextureTarget->SizeY;
	CameraFrame.Width = GetSceneCaptureComponent2D()->TextureTarget->SizeX;
	CameraFrame.MaxDepthDistance = MaxDepthDistance;

	/*
	if (bLogTick)
	{
		UE_LOG(LogSoda, Warning, TEXT("Camera name: %s, timestamp: %s, Index: %s"),
			*GetFName().ToString(),
			*soda::ToString(CameraFrame.Timestamp),
			*USodaStatics::Int64ToString(CameraFrame.Index));
	}
	*/

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)([Sensor=this, CameraFrame, DeltaTime, Header=GetHeaderGameThread()](FRHICommandListImmediate& RHICmdList)
	{
		if(!IsValid(Sensor) || !IsValid(Sensor->GetSceneCaptureComponent2D())) return;

		// TODO: This has not been working from the UE5, need somehow move it to the GameTread
		/*
		if (bDrawDebugText)
		{
			if (ASodaVehicle* Vehicle = GetVehicle())
			{
				FDrawToRenderTargetContext Context;
				UCanvas* Canvas;
				FVector2D CanvasSize(CameraFrame.Width, CameraFrame.Height);
				UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, GetSceneCaptureComponent2D()->TextureTarget, Canvas, CanvasSize, Context);
				UFont* RenderFont = GEngine->GetSmallFont();
				Canvas->SetDrawColor(FColor::Green);
				float YPos = 10;
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Camera name: %s"), *GetFName().ToString()), 4, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Frame timestamp: %s"), *soda::ToString(Timestamp)), 4, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Frame Index: %s"), *USodaStatics::Int64ToString(IndexSaved)), 4, YPos) + 4;

				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Phys step: %i"), Vehicle->GetSimData(true).SimulatedStep), 4, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Phys timestamp: %s"), *soda::ToString(Vehicle->GetSimData(true).SimulatedTimestamp)), 4, YPos) + 4;

				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Fixed phys step: %i"), Vehicle->GetSimData(true).RenderStep), 4, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Fixed phys timestamp: %s"), *soda::ToString(Vehicle->GetSimData(true).RenderTimestamp)), 4, YPos) + 4;

				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vehicle pos: %s"), *Vehicle->GetActorLocation().ToString()), 4, YPos);
				UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
			}
		}
		*/

		Sensor->PublishSensorData(DeltaTime, Header, CameraFrame, *Sensor->GetSceneCaptureComponent2D()->TextureTarget, RHICmdList);

		if (Sensor->NeedPublishCPUData())
		{
			TSharedPtr<FCameraAsyncTask> Task = Sensor->AsyncTask->LockFrontTask();
			if (!Task->IsDone())
			{
				UE_LOG(LogSoda, Warning, TEXT("UCameraSensor::TickComponent(). Skipped one frame"));
				return;
			}
			Task->Initialize();
			Task->DeltaTime = DeltaTime;
			Task->Header = Header;
			Task->CameraFrame = CameraFrame;
			FCameraPixelReader::ReadPixels(*Sensor->GetSceneCaptureComponent2D()->TextureTarget, RHICmdList, Task->ImageBuffer, Task->ImageStride);
			Sensor->AsyncTask->UnlockFrontTask();
			SodaApp.CamTaskManager.Trigger();
		}

	});
	RenderFence.BeginFence();
}

FCameraIntrinsics UCameraSensor::GetCameraIntrinsics() const
{
	float FocalLength = Width / (std::tan(GetHFOV() / 2 / 180 * M_PI) * 2);
	float Cx = (Width / 2);
	float Cy = (Height / 2);
	return { 0.f, 0.f, 0.f, 0.f, Cx, Cy, FocalLength, FocalLength };
}

void UCameraSensor::MakeScreenshot(const FString& InFileName)
{
	/*
	if (!GetSceneCaptureComponent2D()->bCaptureEveryFrame)
	{
		GetSceneCaptureComponent2D()->CaptureScene();
	}

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)([this, InFileName](FRHICommandListImmediate& RHICmdList)
	{
		switch (Format)
		{
		case ECameraSensorShader::ColorBGR8:
		case ECameraSensorShader::SegmBGR8:
		case ECameraSensorShader::HdrRGB8:
		case ECameraSensorShader::CFA:
		{
			TArrayView<FColor> OutPixels;
			uint32 ImageStride;
			PixelReader.BeginRead(*GetSceneCaptureComponent2D()->TextureTarget, RHICmdList, OutPixels, ImageStride);

			TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(CameraFrame.Width, CameraFrame.Height));
			PixelData->Pixels.SetNum(CameraFrame.Width * CameraFrame.Height, false);
			ParallelFor(CameraFrame.Height, [&](int32 i)
			{
				FMemory::Memcpy(
					&PixelData->Pixels[i * CameraFrame.Width],
					&OutPixels[i * ImageStride],
					CameraFrame.Width * 4);
			});

			TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
			ImageTask->PixelData = MoveTemp(PixelData);
			ImageTask->Filename = InFileName;
			ImageTask->Format = EImageFormat::PNG;
			ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
			ImageTask->bOverwriteFile = true;
			//ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
			FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
			//TFuture<bool> Res = 
			HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
			break;
		}

		default:
			UE_LOG(LogSoda, Error, TEXT("The output image format is not supported for taking screenshots"));
		}
	});
	*/
}

void UCameraSensor::MakeScreenshotToDefault()
{
	MakeScreenshot(FString::Format(*DefaultSavePath, { FDateTime::Now().ToString() }));
}

bool UCameraSensor::IsColorFormat(ECameraSensorShader Fromat)
{
	switch (Fromat)
	{
	case ECameraSensorShader::ColorBGR8:
	case ECameraSensorShader::HdrRGB8:
	case ECameraSensorShader::CFA:
	case ECameraSensorShader::SegmBGR8:
		return true;

	case ECameraSensorShader::Segm8:
	case ECameraSensorShader::Depth8:
	case ECameraSensorShader::DepthFloat32:
	case ECameraSensorShader::Depth16:
	default:
		return false;
	}
}


bool UCameraSensor::NeedRenderSensorFOV() const
{
	return (FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && IsVehicleComponentSelected());
}

FBoxSphereBounds UCameraSensor::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FMeshGenerationUtils::CalcFOVBounds(GetVFOV(), GetHFOV(), FOVSetup.MaxViewDistance)).TransformBy(LocalToWorld);
}