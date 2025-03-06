// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/LidarDepth2DSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "DrawDebugHelpers.h"
#include "Soda/Misc/ExtraWindow.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"
#include "Async/ParallelFor.h"
#include "SceneView.h"
#include "Components/LineBatchComponent.h"

#if PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wshadow"
#endif


/***********************************************************************************************
	FLidar2DAsyncTask
***********************************************************************************************/
class FLidar2DAsyncTask : public soda::FAsyncTask
{
public:
	virtual ~FLidar2DAsyncTask() {}
	virtual FString ToString() const override { return "FLidar2DAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override
	{
		check(!bIsDone);
		check(OutPixels.Num() > 0 && uint32(OutPixels.Num()) >= ImageStride * CameraFrame.Height);

		Map.SetNum(CameraFrame.Height * CameraFrame.Width);
		
		ParallelFor(CameraFrame.Height, [&](uint32 y)
		{
			for (uint32 x = 0; x < CameraFrame.Width; x++)
			{
				Map[y * CameraFrame.Width + x] = Rgba2Float(OutPixels[y * ImageStride + x]);
			}
		});

		Sensor->PublishBitmapData(CameraFrame, OutPixels, ImageStride);
		
		const TArray<FVector> & LidarRays = Sensor->GetLidarRays();
		const TArray<FVector2D> & UVs = Sensor->GetLidarUVs();

		check(LidarRays.Num() == UVs.Num());

		Scan.Points.SetNum(LidarRays.Num());
		Scan.HorizontalAngleMax = Sensor->GetFOVHorizontMax();
		Scan.HorizontalAngleMin = Sensor->GetFOVHorizontMin();
		Scan.VerticalAngleMin = Sensor->GetFOVVerticalMin();
		Scan.VerticalAngleMax = Sensor->GetFOVVerticalMax();
		Scan.RangeMin = Sensor->GetLidarMinDistance();
		Scan.RangeMax = Sensor->GetLidarMaxDistance();
		Scan.Size = Sensor->GetLidarSize();

		for (int k = 0; k < LidarRays.Num(); ++k)
		{
			float Depth = 0;
			float x = UVs[k].X;
			float y = UVs[k].Y;

			switch (Interpolation)
			{
			case ELidarInterpolation::Min:
			case ELidarInterpolation::Bilinear:
			{
				int x1 = (int)(x);
				int x2 = (int)(x + 1.0);
				int y1 = (int)(y);
				int y2 = (int)(y + 1.0);

				if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 || x1 >= (int)CameraFrame.Width || x2 >= (int)CameraFrame.Width || y1 >= (int)CameraFrame.Height || y2 >= (int)CameraFrame.Height)
				{
					break; // skip;
				}

				float q11 = Map[y1 * CameraFrame.Width + x1];
				float q12 = Map[y2 * CameraFrame.Width + x1];
				float q21 = Map[y1 * CameraFrame.Width + x2];
				float q22 = Map[y2 * CameraFrame.Width + x2];

				if (Interpolation == ELidarInterpolation::Bilinear)
				{
					Depth = BilinearInterpolation(q11, q12, q21, q22, x1, x2, y1, y2, x, y);
				}
				else
				{
					Depth = std::min(std::min(q11, q12), std::min(q21, q22));
				}
			}
			break;

			case ELidarInterpolation::Nearest:
			{
				int x1 = (int)(x + 0.5);
				int y1 = (int)(y + 0.5);

				if (x1 < 0 || y1 < 0 || x1 >= (int)CameraFrame.Width || y1 >= (int)CameraFrame.Height)
				{
					break; // skip;
				}

				Depth = Map[y1 * CameraFrame.Width + x1];
			}
			break;
			}

			
			Scan.Points[k].Location = LidarRays[k] * Depth * DepthMapNorm;
			Scan.Points[k].Depth = Scan.Points[k].Location.Size();

			if (Depth >= 1 || Scan.Points[k].Depth < DistanceMin || Scan.Points[k].Depth > DistanceMax)
			{
				Scan.Points[k].Status = soda::ELidarPointStatus::Invalid;
			}
			else
			{
				Scan.Points[k].Status = soda::ELidarPointStatus::Valid;
			}
		}

		Sensor->PublishSensorData(DeltaTime, Header, Scan);
		Sensor->DrawLidarPoints(Scan, true);
		bIsDone = true;
	}

	void Setup(ULidarDepth2DSensor* InSensor)
	{
		Sensor = InSensor;
		Interpolation = InSensor->Interpolation;
		DistanceMax = InSensor->GetLidarMaxDistance();
		DistanceMin = InSensor->GetLidarMinDistance();
		DepthMapNorm = InSensor->DepthMapNorm;
	}

public:
	float DeltaTime{};
	FSensorDataHeader Header;
	FCameraFrame CameraFrame;
	TArray<FColor> OutPixels;
	uint32 ImageStride = 0; // Original texture width in pixels


protected:
	TWeakObjectPtr<ULidarDepth2DSensor> Sensor;
	ELidarInterpolation Interpolation;
	float DistanceMax;
	float DistanceMin;
	float DepthMapNorm;

protected:
	bool bIsDone = true;
	TArray<float> Map;
	soda::FLidarSensorData Scan;
};


/***********************************************************************************************
	FLidar2DFrontBackAsyncTask
***********************************************************************************************/
class FLidar2DFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FLidar2DAsyncTask>
{
public:
	FLidar2DFrontBackAsyncTask(ULidarDepth2DSensor* Sensor)
	{
		FrontTask->Setup(Sensor);
		BackTask->Setup(Sensor);
	}
	virtual ~FLidar2DFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "FLidar2DFrontBackAsyncTask"; }
};

/***********************************************************************************************
	ULidarDepth2DSensor
***********************************************************************************************/
ULidarDepth2DSensor::ULidarDepth2DSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	static ConstructorHelpers::FObjectFinder< UObject > PostProcess(TEXT("/SodaSim/Assets/CPP/Lidar/DepthMapLidar"));
	if (PostProcess.Succeeded())
	{
		LidarPostProcessSettings.WeightedBlendables.Array.AddDefaulted(1);
		LidarPostProcessSettings.WeightedBlendables.Array[0].Object = PostProcess.Object;
		LidarPostProcessSettings.WeightedBlendables.Array[0].Weight = 1.f;
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("Absent LidarPostProcessSettings for ULidarDepth2DSensor"));
	}
}


void ULidarDepth2DSensor::BeginPlay()
{
	Super::BeginPlay();

	if (LidarPostProcessSettings.WeightedBlendables.Array.Num() && LidarPostProcessSettings.WeightedBlendables.Array[0].Object)
	{
		UMaterialInterface* DepthMaterialInterface = Cast< UMaterialInterface >(LidarPostProcessSettings.WeightedBlendables.Array[0].Object);
		if (DepthMaterialInterface)
		{
			DepthMaterialDyn = UMaterialInstanceDynamic::Create(DepthMaterialInterface, this);
			LidarPostProcessSettings.WeightedBlendables.Array[0].Object = DepthMaterialDyn;
		}
	}

}

void ULidarDepth2DSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	//CameraFrame.Timestamp = SodaApp.GetSimulationTimestamp();
	//CameraFrame.Index = SodaApp.GetFrameIndex();
	

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)([Sensor=this, CameraFrame=CameraFrame, DeltaTime, Header=GetHeaderGameThread()](FRHICommandListImmediate& RHICmdList)
	{
		if (!IsValid(Sensor) || !IsValid(Sensor->SceneCaptureComponent2D)) return;

		TSharedPtr<FLidar2DAsyncTask> Task = Sensor->AsyncTask->LockFrontTask();
		if (!Task->IsDone())
		{
			UE_LOG(LogSoda, Warning, TEXT("ULidarDepth2DSensor::TickComponent(). Skipped one frame"));
			return;
		}
		Task->Initialize();
		Task->DeltaTime = DeltaTime;
		Task->Header = Header;
		Task->CameraFrame = CameraFrame;
		FCameraPixelReader::ReadPixels(*Sensor->SceneCaptureComponent2D->TextureTarget, RHICmdList, Task->OutPixels, Task->ImageStride);
		Sensor->AsyncTask->UnlockFrontTask();
		SodaApp.CamTaskManager.Trigger();
	});
	RenderFence.BeginFence();
}

bool ULidarDepth2DSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!DepthMaterialDyn)
	{
		UE_LOG(LogSoda, Error, TEXT("%s - LidarPostProcess Material NOT loaded!"), *(this->GetName()));
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	DepthMaterialDyn->SetScalarParameterValue("MaxDist", GetLidarMaxDistance());

	// SceneCaptureComponent2D
	SceneCaptureComponent2D = NewObject< USceneCaptureComponent2D >(this);
	SceneCaptureComponent2D->SetupAttachment(this);
	SceneCaptureComponent2D->CaptureSortPriority = 2;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComponent2D->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
	SceneCaptureComponent2D->TextureTarget->InitCustomFormat(TextureWidth, TextureHeight, /*EPixelFormat::PF_A16B16G16R16*/ EPixelFormat::PF_B8G8R8A8, true);
	SceneCaptureComponent2D->TextureTarget->AddressX = TextureAddress::TA_Clamp;
	SceneCaptureComponent2D->TextureTarget->AddressY = TextureAddress::TA_Clamp;
	SceneCaptureComponent2D->FOVAngle = CameraFOV;
	SceneCaptureComponent2D->SetRelativeRotation(FRotator(0, 0, 0));
	SceneCaptureComponent2D->RegisterComponent();
	SceneCaptureComponent2D->bCaptureOnMovement = true;
	SceneCaptureComponent2D->bCaptureEveryFrame = true;
	SceneCaptureComponent2D->PostProcessSettings = LidarPostProcessSettings;
	SceneCaptureComponent2D->HideComponent(this);
	SceneCaptureComponent2D->HideComponent(GetWorld()->LineBatcher.Get());
	SceneCaptureComponent2D->HideComponent(GetWorld()->PersistentLineBatcher.Get());
	SceneCaptureComponent2D->HideComponent(GetWorld()->ForegroundLineBatcher.Get());

	CameraFrame = FCameraFrame(ECameraSensorShader::Depth8);
	CameraFrame.Height = TextureHeight;
	CameraFrame.Width = TextureWidth;
	//CameraFrame.MaxDepthDistance = MaxDepthDistance;

	AsyncTask = MakeShareable(new FLidar2DFrontBackAsyncTask(this));
	AsyncTask->Start();
	SodaApp.CamTaskManager.AddTask(AsyncTask);

	return true;
}

void ULidarDepth2DSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	RenderFence.Wait();

	AsyncTask->Finish();
	SodaApp.CamTaskManager.RemoteTask(AsyncTask);
	AsyncTask.Reset();

	if (IsValid(SceneCaptureComponent2D)) SceneCaptureComponent2D->ConditionalBeginDestroy();
	SceneCaptureComponent2D = nullptr;
	
	if (IsValid(CameraWindow)) CameraWindow->Close();
	CameraWindow = nullptr;
}


void ULidarDepth2DSensor::SetPrecipitation(float Probability)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("Precipitation", Probability * 5.0);
	}
}

void ULidarDepth2DSensor::SetNoize(float Probability, float MaxDistance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("NoizeProbability", Probability);
		DepthMaterialDyn->SetScalarParameterValue("NoizeMaxDistance", MaxDistance);
	}
}

void ULidarDepth2DSensor::SetDepthMapMaxView(float Distance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("MaxDistance", Distance);
	}
}

UTextureRenderTarget2D* ULidarDepth2DSensor::GetRenderTarget2DTexture()
{
	if (SceneCaptureComponent2D != nullptr)
		return SceneCaptureComponent2D->TextureTarget;
	return nullptr;
}

void ULidarDepth2DSensor::ShowRenderTarget()
{
	if (!IsValid(CameraWindow))
	{
		CameraWindow = NewObject<UExtraWindow>();
	}
	if (SceneCaptureComponent2D && SceneCaptureComponent2D->TextureTarget)
	{
		CameraWindow->OpenCameraWindow(SceneCaptureComponent2D->TextureTarget);
	}
}

int ULidarDepth2DSensor::GenerateUVs(TArray<FVector>& InOutLidarRays, float FOVAngle, int Width, int Height, ELidarInterpolation Interpolation, TArray<FVector2D>& UVs)
{
	// Computing LidarRays and UVs
	FMatrix ProjectionMatrix;
	if ((int32)ERHIZBuffer::IsInverted)
	{
		ProjectionMatrix = FReversedZPerspectiveMatrix(
			FOVAngle * (float)PI / 360.0f,
			FOVAngle * (float)PI / 360.0f,
			1.0f,
			Width / (float)Height,
			GNearClippingPlane,
			GNearClippingPlane);
	}
	else
	{
		ProjectionMatrix = FPerspectiveMatrix(
			FOVAngle * (float)PI / 360.0f,
			FOVAngle * (float)PI / 360.0f,
			1.0f,
			Width / (float)Height,
			GNearClippingPlane,
			GNearClippingPlane);
	}

	const FMatrix ViewRotationMatrix = FRotationMatrix::Make(FQuat(FRotator(0, 0, 0))).Inverse() * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));
	const FMatrix ViewProjectionMatrix = ViewRotationMatrix * ProjectionMatrix;

	UVs.SetNum(InOutLidarRays.Num());

	const FIntRect ViewRect(0, 0, Width - 1, Height - 1);
	for (int i = 0; i < InOutLidarRays.Num(); ++i)
	{
		UVs[i] = FVector2D::ZeroVector;
		auto& Ray = InOutLidarRays[i];
		const float LenCoef = FMath::Sqrt(1.0 - FMath::Square(Ray.Y)) * FMath::Sqrt(1.0 - FMath::Square(Ray.Z));
		Ray = Ray / LenCoef;
		FSceneView::ProjectWorldToScreen(Ray, ViewRect, ViewProjectionMatrix, UVs[i]);
	}

	// Check skipped points
	int PointsSkiped = 0;
	for (int k = 0; k < InOutLidarRays.Num(); ++k)
	{
		float x = UVs[k].X;
		float y = UVs[k].Y;

		switch (Interpolation)
		{
		case ELidarInterpolation::Min:
		case ELidarInterpolation::Bilinear:
		{
			int x1 = (int)(x);
			int x2 = (int)(x + 1.0);
			int y1 = (int)(y);
			int y2 = (int)(y + 1.0);

			if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 || x1 >= (int)Width || x2 >= (int)Width || y1 >= (int)Height || y2 >= (int)Height)
			{
				++PointsSkiped;
			}
		}
		break;

		case ELidarInterpolation::Nearest:
		{
			int x1 = (int)(x + 0.5);
			int y1 = (int)(y + 0.5);

			if (x1 < 0 || y1 < 0 || x1 >= (int)Width || y1 >= (int)Height)
			{
				++PointsSkiped;
			}
		}
		break;
		}
	}

	return PointsSkiped;
}