// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/LidarDepthSensorCube.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DynamicMeshBuilder.h"

#if PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wshadow"
#endif


ULidarDepthSensorCubeComponent::ULidarDepthSensorCubeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("LiDAR Sensors");
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Depth Cube");
	GUI.IcanName = TEXT("SodaIcons.Lidar");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SceneCaptureComponent2D.SetNum(4, true);

	DepthMapPublisher = CreateDefaultSubobject< UGenericCameraPublisher>(TEXT("DepthMapPublisher"));
}

bool ULidarDepthSensorCubeComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	for (int i = 0; i < 4; i++)
	{
		if (SceneCaptureComponent2D[i])SceneCaptureComponent2D[i]->ConditionalBeginDestroy();
		SceneCaptureComponent2D[i] = NewObject< USceneCaptureComponent2D >(this);
		SceneCaptureComponent2D[i]->SetupAttachment(this);
		SceneCaptureComponent2D[i]->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
		SceneCaptureComponent2D[i]->TextureTarget->InitCustomFormat(Size, Size, EPixelFormat::PF_B8G8R8A8, true);
		SceneCaptureComponent2D[i]->PostProcessSettings = LidarPostProcessSettings;
		SceneCaptureComponent2D[i]->HiddenActors = HiddenActors;
		SceneCaptureComponent2D[i]->ShowOnlyActors = ShowOnlyActors;
		SceneCaptureComponent2D[i]->bCaptureEveryFrame = true;
		SceneCaptureComponent2D[i]->bCaptureOnMovement = false;
		SceneCaptureComponent2D[i]->LODDistanceFactor = LODDistanceFactor;
		SceneCaptureComponent2D[i]->MaxViewDistanceOverride = MaxViewDistanceOverride;
		SceneCaptureComponent2D[i]->CaptureSortPriority = CaptureSortPriority;
		SceneCaptureComponent2D[i]->SetRelativeRotation(FRotator(0.0, (float)(i) * 90.0, 0.0));
		SceneCaptureComponent2D[i]->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		SceneCaptureComponent2D[i]->HideComponent(this);
		SceneCaptureComponent2D[i]->RegisterComponent();			
	}

	Points.resize(Channels * StepToFace * 4);

	CameraFrame.Height = Size;
	CameraFrame.Width = Size * 4;
	CameraFrame.OutFormat = ECameraSensorShader::Depth8;
	CameraFrame.ImageStride = Size * 4;

	Map.resize(Size * Size * 4);

	PointCloudPublisher.Advertise();
	DepthMapPublisher->Advertise();

	if (!PointCloudPublisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void ULidarDepthSensorCubeComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	RenderFence.Wait();

	PointCloudPublisher.Shutdown();
	DepthMapPublisher->Shutdown();

	for (int i = 0; i < 4; i++)
	{
		SceneCaptureComponent2D[i]->ConditionalBeginDestroy();
		SceneCaptureComponent2D[i] = nullptr;
	}

	Map.clear();
}

void ULidarDepthSensorCubeComponent::GetRemark(FString & Info) const
{
	Info =  PointCloudPublisher.Address + ":" + FString::FromInt(PointCloudPublisher.Port);
}

void ULidarDepthSensorCubeComponent::BeginPlay()
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

void ULidarDepthSensorCubeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	TTimestamp Timestemp = SodaApp.GetSimulationTimestamp();
	CameraFrame.Timestamp = Timestemp;

	FaceReady = 0;

	for (int iFace = 0; iFace < 4; iFace++)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[this, iFace, Timestemp](FRHICommandListImmediate& RHICmdList)
		{
			TArrayView<FColor> OutPixels;
			uint32 OutWidth;
			PixelReader.BeginRead(*SceneCaptureComponent2D[iFace]->TextureTarget, RHICmdList, OutPixels, OutWidth);

			if (OutPixels.Num() < int(OutWidth * Size))
			{
				UE_LOG(LogSoda, Error, TEXT("ULidarDepthSensorCubeComponent::TickComponent(); Read %i pixels"), OutPixels.Num());
				PixelReader.EndRead();
				return;
			}

			for (int i = 0; i < Size; i++)
				for (int j = 0; j < Size; j++)
				{
					float Depth = Rgba2Float(OutPixels[OutWidth * i + j]);
					Map[i * Size * 4 + iFace * Size + j] = Depth;
				}

			PixelReader.EndRead();

			for (int i = 0; i < Channels; i++)
			{
				float ai = (FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)(Channels - 1) * (float)i) / 180.0 * M_PI;

				for (int j = 0; j < StepToFace; j++)
				{
					float aj = -M_PI / 4.0 + M_PI / 2.0 / (float)StepToFace * (float)j;

					float ajj = (1 + tanf(aj)) * Size / 2.0;
					float aii = (1 - tanf(ai) / cos(aj)) * Size / 2.0;

					float x = ajj + iFace * Size;
					float y = aii;

					float Depth = 0;

					switch (Interpolation)
					{
					case ELidarInterpolation::Min:
					case ELidarInterpolation::Bilinear:
					{

						int x1 = (int)(x);
						int x2 = (int)(x + 1.0);
						int y1 = (int)(y);
						int y2 = (int)(y + 1.0);
						if (x1 < 0)
							x1 = Size * 4 - 1;
						if (x2 >= Size * 4)
							x2 = 0;
						float q11 = Map[y1 * Size * 4 + x1];
						float q12 = Map[y2 * Size * 4 + x1];
						float q21 = Map[y1 * Size * 4 + x2];
						float q22 = Map[y2 * Size * 4 + x2];

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
						if (x1 >= Size * 4)
							x1 = 0;
						Depth = Map[y1 * Size * 4 + x1];
					}
					break;
					}

					float pz = Depth * DepthMapNorm;
					float px = (-1 + 2.0 / (float)Size * ajj) * pz;
					float py = (+1 - 2.0 / (float)Size * aii) * pz;

					FVector RayPoint = FVector(pz, px, py);
					float RayLen = RayPoint.Size();

					if (RayLen > DistanseMin / 100.0 && RayLen < DistanseMax / 100.0)
					{
						RayPoint = FRotator(0.0, iFace * 90.0, 0.0).RotateVector(RayPoint);
					}
					else
					{
						RayPoint = FVector(0.0, 0.0, 0.0);
					}

					soda::LidarScanPoint& pt = Points[(StepToFace * 4 * i) + (j + StepToFace * iFace)];

					pt.coords.x = RayPoint.X;
					pt.coords.y = RayPoint.Y;
					pt.coords.z = RayPoint.Z;
				}
			}

			FaceReady++;

			if (FaceReady == 4)
			{
				if (SceneCaptureComponent2D[iFace]->TextureTarget)
				{
					PointCloudPublisher.Publish(Timestemp, Points);
					//DepthMapPublisher.Publish(&Map[0]);
				}
			}
		});
	}
	RenderFence.BeginFence();
}

void ULidarDepthSensorCubeComponent::SetPrecipitation(float Probability)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("Precipitation", Probability * 5.0);
	}
}

void ULidarDepthSensorCubeComponent::SetNoize(float Probability, float MaxDistance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("NoizeProbability", Probability);
		DepthMaterialDyn->SetScalarParameterValue("NoizeMaxDistance", MaxDistance);
	}
}

void ULidarDepthSensorCubeComponent::SetDepthMapMaxView(float Distance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("MaxDistance", Distance);
	}
}