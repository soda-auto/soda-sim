// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/LidarDepthSensor2D.h"
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

#if PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#define _DEG2RAD(a) ((a) / (180.0 / M_PI))
#define _RAD2DEG(a) ((a) * (180.0 / M_PI))

ULidarDepthSensor2DComponent::ULidarDepthSensor2DComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("LiDAR Sensors");
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Depth 2D");
	GUI.IcanName = TEXT("SodaIcons.Lidar");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	FOVSetup.Color = FLinearColor(1.0, 0.1, 0.3, 1.0);
	FOVSetup.MaxViewDistance = 700;

	static ConstructorHelpers::FObjectFinder< UObject > PostProcess(TEXT("/SodaSim/Assets/CPP/Lidar/DepthMapLidar"));
	if (PostProcess.Succeeded())
	{
		LidarPostProcessSettings.WeightedBlendables.Array.AddDefaulted(1);
		LidarPostProcessSettings.WeightedBlendables.Array[0].Object = PostProcess.Object;
		LidarPostProcessSettings.WeightedBlendables.Array[0].Weight = 1.f;
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("Absent LidarPostProcessSettings for ULidarDepthSensor2DComponent"));
	}

	DepthMapPublisher = CreateDefaultSubobject< UGenericCameraPublisher>(TEXT("DepthMapPublisher"));
}

void ULidarDepthSensor2DComponent::InitializeComponent()
{
	Super::InitializeComponent();

	CameraWindow = NewObject<UExtraWindow>();
}

void ULidarDepthSensor2DComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void ULidarDepthSensor2DComponent::BeginPlay()
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

void ULidarDepthSensor2DComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	int ChanelsThreadSafe = Channels;
	int StepThreadSafe = Step;
	TTimestamp TimestampThreadSafe = SodaApp.GetSimulationTimestamp();
	FVector Loc = GetComponentLocation();
	FRotator Rot = GetComponentRotation();

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)([this, ChanelsThreadSafe, StepThreadSafe, TimestampThreadSafe, Loc, Rot](FRHICommandListImmediate& RHICmdList)
	{
		if (!IsValid(this) || !IsValid(SceneCaptureComponent2D)) return;

		Points.resize(ChanelsThreadSafe * StepThreadSafe);

		TArrayView<FColor> OutPixels;
		PixelReader.BeginRead(*SceneCaptureComponent2D->TextureTarget, RHICmdList, OutPixels, CameraFrame.ImageStride);

		if (OutPixels.Num() < int(CameraFrame.Height * CameraFrame.Width))
		{
			UE_LOG(LogSoda, Error, TEXT("ULidarDepthSensor2DComponent::TickComponent(); Read %i pixels"), OutPixels.Num());
			PixelReader.EndRead();
			return;
		}

		if (!IsValid(this) || !IsValid(SceneCaptureComponent2D)) return;

		ParallelFor(CameraFrame.Height, [&](uint32 y)
		{
			for (uint32 x = 0; x < CameraFrame.Width; x++)
			{
				Map[y * CameraFrame.Width + x] = Rgba2Float(OutPixels[y * CameraFrame.ImageStride + x]);
			}
		});

		if (bPublishDepthMap)
		{
			CameraFrame.ImageStride = CameraFrame.ImageStride;
			DepthMapPublisher->Publish(OutPixels.GetData(), CameraFrame);
		}

		PixelReader.EndRead();

		int PointsSkiped = 0;

		for (int k = 0; k < ChanelsThreadSafe * StepThreadSafe; ++k)
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
						++PointsSkiped;
						break;
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
						++PointsSkiped;
						break;
					}

					Depth = Map[y1 * CameraFrame.Width + x1];
				}
				break;
			}

			FVector RayPoint = LidarRays[k] * Depth;
			float RayLen = RayPoint.Size();

			if (RayLen < DistanseMin / 100.0 || RayLen > DistanseMax / 100.0)
			{
				RayPoint = FVector(0.0, 0.0, 0.0);
			}
				
			soda::LidarScanPoint& pt = Points[k];

			pt.coords.x = RayPoint.X;
			pt.coords.y = RayPoint.Y;
			pt.coords.z = RayPoint.Z;

			if (Common.bDrawDebugCanvas)
			{
				RayPoint *= 100.f;
				RayPoint = Rot.RotateVector(RayPoint) + Loc;
				DrawDebugPoint(GetWorld(), RayPoint, 5, FColor::Green, false, -1.0f);
			}

		}

		PointCloudPublisher.Publish(TimestampThreadSafe, Points);

		if (PointsSkiped)
		{
			UE_LOG(LogSoda, Warning, TEXT("ULidarDepthSensor2DComponent::TickComponent(); %i point(s) did't include into the FOV of %s"), PointsSkiped, *GetFName().ToString());
		}

	});
	RenderFence.BeginFence();
}

bool ULidarDepthSensor2DComponent::OnActivateVehicleComponent()
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

	DepthMaterialDyn->SetScalarParameterValue("MaxDist", DistanseMax);

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

	FQuat CommonQuat = FQuat(FRotator(0, 0, 0));
	FMatrix ViewRotationMatrix = FRotationMatrix::Make(CommonQuat).Inverse() * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));
	FMatrix ViewProjectionMatrix = ViewRotationMatrix * ProjectionMatrix;

	LidarRays.SetNum(Channels * Step);
	UVs.SetNum(Channels * Step);
	for (int v = 0; v < Channels; ++v)
	{
		for (int u = 0; u < Step; ++u)
		{
			LidarRays[Step * v + u] = FVector::ForwardVector;

			float AngleX = FOV_Horizontal / -2.f + FOV_Horizontal / (float)(Step - 1) * (float)u;
			float AngleY = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)(Channels - 1) * (float)v;

			LidarRays[Step * v + u] = LidarRays[Step * v + u].RotateAngleAxis(AngleY, FVector(0.f, 1.f, 0.f));
			LidarRays[Step * v + u] = LidarRays[Step * v + u].RotateAngleAxis(AngleX, FVector(0.f, 0.f, 1.f));

			LidarRays[Step * v + u] *= DepthMapNorm;

			float LenCoef = cos(_DEG2RAD(AngleX)) * cos(_DEG2RAD(AngleY));
			LidarRays[Step * v + u] /= LenCoef;

			static FIntRect ViewRect(0, 0, Width - 1, Height - 1);
			UVs[Step * v + u] = FVector2D::ZeroVector;
			FVector2D UV;
			if (FSceneView::ProjectWorldToScreen(LidarRays[Step * v + u] * 100, ViewRect, ViewProjectionMatrix, UV))
			{
				UVs[Step * v + u] = UV;
			}
		}
	}

	// SceneCaptureComponent2D
	SceneCaptureComponent2D = NewObject< USceneCaptureComponent2D >(this);
	SceneCaptureComponent2D->SetupAttachment(this);
	SceneCaptureComponent2D->CaptureSortPriority = 2;
	SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComponent2D->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
	SceneCaptureComponent2D->TextureTarget->InitCustomFormat(Width, Height, /*EPixelFormat::PF_A16B16G16R16*/ EPixelFormat::PF_B8G8R8A8, true);
	SceneCaptureComponent2D->TextureTarget->AddressX = TextureAddress::TA_Clamp;
	SceneCaptureComponent2D->TextureTarget->AddressY = TextureAddress::TA_Clamp;
	SceneCaptureComponent2D->bCaptureOnMovement = false;
	SceneCaptureComponent2D->bCaptureEveryFrame = true;
	SceneCaptureComponent2D->FOVAngle = FOVAngle;
	SceneCaptureComponent2D->SetRelativeRotation(CommonQuat);
	SceneCaptureComponent2D->RegisterComponent();
	SceneCaptureComponent2D->bCaptureOnMovement = PointCloudPublisher.IsAdvertised();
	SceneCaptureComponent2D->bCaptureEveryFrame = PointCloudPublisher.IsAdvertised();
	SceneCaptureComponent2D->PostProcessSettings = LidarPostProcessSettings;
	SceneCaptureComponent2D->HideComponent(this);

	CameraFrame.Height = Height;
	CameraFrame.Width = Width;
	CameraFrame.OutFormat = ECameraSensorShader::Depth8;

	Map.resize(Height * Width);

	// Publisher
	PointCloudPublisher.Advertise();
	DepthMapPublisher->Advertise();

	if (!PointCloudPublisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void ULidarDepthSensor2DComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	RenderFence.Wait();

	PointCloudPublisher.Shutdown();
	DepthMapPublisher->Shutdown();

	if (IsValid(SceneCaptureComponent2D)) SceneCaptureComponent2D->ConditionalBeginDestroy();
	SceneCaptureComponent2D = nullptr;
	
	Map.clear();
	LidarRays.Empty();
	UVs.Empty();

	if (IsValid(CameraWindow)) CameraWindow->Close();
	CameraWindow = nullptr;
}

void ULidarDepthSensor2DComponent::GetRemark(FString& Info) const
{
	Info = PointCloudPublisher.Address + ":" + FString::FromInt(PointCloudPublisher.Port);
}

void ULidarDepthSensor2DComponent::SetPrecipitation(float Probability)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("Precipitation", Probability * 5.0);
	}
}

void ULidarDepthSensor2DComponent::SetNoize(float Probability, float MaxDistance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("NoizeProbability", Probability);
		DepthMaterialDyn->SetScalarParameterValue("NoizeMaxDistance", MaxDistance);
	}
}

void ULidarDepthSensor2DComponent::SetDepthMapMaxView(float Distance)
{
	if (DepthMaterialDyn)
	{
		DepthMaterialDyn->SetScalarParameterValue("MaxDistance", Distance);
	}
}

UTextureRenderTarget2D* ULidarDepthSensor2DComponent::GetRenderTarget2DTexture()
{
	if (SceneCaptureComponent2D != nullptr)
		return SceneCaptureComponent2D->TextureTarget;
	return nullptr;
}

void ULidarDepthSensor2DComponent::ShowRenderTarget()
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

bool ULidarDepthSensor2DComponent::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateSimpleFOVMesh(
		FOV_Horizontal,
		FOV_VerticalMax - FOV_VerticalMin,
		20, 20, FOVSetup.MaxViewDistance,
		0, (FOV_VerticalMin + FOV_VerticalMax) / 2,
		MeshData.Vertices, MeshData.Indices))
	{
		for (auto& it : MeshData.Vertices) it.Color = FOVSetup.Color.ToFColor(true);
		Meshes.Add(MoveTempIfPossible(MeshData));
		return true;
	}
	else
	{
		return false;
	}
}

bool ULidarDepthSensor2DComponent::NeedRenderSensorFOV() const
{
	return (FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && IsVehicleComponentSelected());
}

FBoxSphereBounds ULidarDepthSensor2DComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FMeshGenerationUtils::CalcFOVBounds(FOV_Horizontal, FOV_VerticalMax - FOV_VerticalMin, FOVSetup.MaxViewDistance)).TransformBy(LocalToWorld);;
}