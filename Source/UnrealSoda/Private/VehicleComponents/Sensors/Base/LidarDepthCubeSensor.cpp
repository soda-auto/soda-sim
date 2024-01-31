// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/LidarDepthCubeSensor.h"
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


#if PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wshadow"
#endif

/***********************************************************************************************
	FLidar2DAsyncTask
***********************************************************************************************/
class FLidarCubeAsyncTask : public soda::FAsyncTask
{
public:
	virtual ~FLidarCubeAsyncTask() {}
	virtual FString ToString() const override { return "FLidarCubeAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override
	{
		/*
		check(!bIsDone);
		check(OutPixels.Num() > 0 && uint32(OutPixels.Num()) >= ImageStride * CameraFrame.Height);

		Map.SetNum(CameraFrame.Height * CameraFrame.Width);
		Map.SetNum(Size * Size * 4);

		const TArray<ULidarDepthCubeSensor::FFace> & Faces = Sensor->GetLidarFases();
		check(Faces.Num() == 4);

		Scan.Timestamp = CameraFrame.Timestamp;
		Scan.Index = CameraFrame.Index;
		Scan.Points.SetNum(0, false);

		for (int iFace = 0; iFace < 4; ++iFace)
		{
			ParallelFor(Size, [&](uint32 y)
			{
				for (uint32 x = 0; x < CameraFrame.Width; x++)
				{
					Map[y * Size * 4 + iFace * Size + x] = Rgba2Float(OutPixels[y * ImageStride + x]);
				}
			});


			//Sensor->PublishBitmapData(CameraFrame, OutPixels, ImageStride);
		


			float * MapFace = &Map[Size * iFace];


			for (int k = 0; k < Faces.Num(); ++k)
			{

				auto& Ray = Faces[iFace].Rays[k];
				auto & UV = Faces[iFace].UVs[k];

				float Depth = 0;
				const float x = UV.X;
				const float y = UV.Y;

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

					float q11 = MapFace[y1 * CameraFrame.Width + x1];
					float q12 = MapFace[y2 * CameraFrame.Width + x1];
					float q21 = MapFace[y1 * CameraFrame.Width + x2];
					float q22 = MapFace[y2 * CameraFrame.Width + x2];

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

					Depth = MapFace[y1 * CameraFrame.Width + x1];
				}
				break;
				}

				FVector RayPoint = Ray * Depth * DepthMapNorm;
				float RayLen = RayPoint.Size();

				if (RayLen < DistanseMin || RayLen > DistanseMax)
				{
					RayPoint = FVector(0.0, 0.0, 0.0);
				}

				Scan.Points.Add({ RayPoint });

			}

		}
		

		Sensor->DrawLidarPoints(Scan);
		Sensor->PublishSensorData(Scan);
		*/

		bIsDone = true;
	}

	void Setup(ULidarDepthCubeSensor* InSensor)
	{
		Sensor = InSensor;
		Interpolation = InSensor->Interpolation;
		DistanseMax = InSensor->GetLidarMaxDistance();
		DistanseMin = InSensor->GetLidarMinDistance();
		DepthMapNorm = InSensor->DepthMapNorm;
	}

public:
	FCameraFrame CameraFrame;

	struct FTexData
	{
		TArray<FColor> Colors;
		uint32 ImageStride = 0; // Original texture width in pixels
	};
	TArray<FTexData> TexData;


protected:
	TWeakObjectPtr<ULidarDepthCubeSensor> Sensor;
	ELidarInterpolation Interpolation;
	float DistanseMax;
	float DistanseMin;
	float DepthMapNorm;

protected:
	bool bIsDone = true;
	TArray<float> Map;
	soda::FLidarScan Scan;
};


/***********************************************************************************************
	FLidarCubeFrontBackAsyncTask
***********************************************************************************************/
class FLidarCubeFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FLidarCubeAsyncTask>
{
public:
	FLidarCubeFrontBackAsyncTask(ULidarDepthCubeSensor* Sensor)
	{
		FrontTask->Setup(Sensor);
		BackTask->Setup(Sensor);
	}
	virtual ~FLidarCubeFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "FLidarCubeFrontBackAsyncTask"; }
};


/***********************************************************************************************
	ULidarDepthCubeSensor
***********************************************************************************************/
ULidarDepthCubeSensor::ULidarDepthCubeSensor(const FObjectInitializer& ObjectInitializer)
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
		UE_LOG(LogSoda, Error, TEXT("Absent LidarPostProcessSettings for ULidarDepthCubeSensor"));
	}


	SceneCaptureComponent2D.SetNum(4, true);

}

void ULidarDepthCubeSensor::BeginPlay()
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


void ULidarDepthCubeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool ULidarDepthCubeSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}


	// TODO: Not supported yet
	return false;
}

void ULidarDepthCubeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

}

void ULidarDepthCubeSensor::SetPrecipitation(float Probability)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("Precipitation", Probability * 5.0);
	}
}

void ULidarDepthCubeSensor::SetNoize(float Probability, float MaxDistance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("NoizeProbability", Probability);
		DepthMaterialDyn->SetScalarParameterValue("NoizeMaxDistance", MaxDistance);
	}
}

void ULidarDepthCubeSensor::SetDepthMapMaxView(float Distance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("MaxDistance", Distance);
	}
}

UTextureRenderTarget2D* ULidarDepthCubeSensor::GetRenderTarget2DTexture()
{
	//if (SceneCaptureComponent2D != nullptr)
	//	return SceneCaptureComponent2D->TextureTarget;
	return nullptr;
}

void ULidarDepthCubeSensor::ShowRenderTarget()
{
	/*
	if (!IsValid(CameraWindow))
	{
		CameraWindow = NewObject<UExtraWindow>();
	}
	if (SceneCaptureComponent2D && SceneCaptureComponent2D->TextureTarget)
	{
		CameraWindow->OpenCameraWindow(SceneCaptureComponent2D->TextureTarget);
	}
	*/
}