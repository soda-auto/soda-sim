// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/CameraFisheyeSensor.h"
#include "Soda/UnrealSoda.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DrawDebugHelpers.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "KismetProceduralMeshLibrary.h"
#include "Containers/UnrealString.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Engine/Engine.h"
#include <random>
#include "Misc/SecureHash.h"
#include "DesktopPlatformModule.h"
#include "UObject/ConstructorHelpers.h"
#include "Async/Async.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "Async/Future.h"
#include "DynamicMeshBuilder.h"
#include "Components/StaticMeshComponent.h"
#include "SceneView.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

template <typename  T>
static void AddToBuf(TArray<uint8>& Buf, T && Value)
{
	for (int i = 0; i < sizeof(Value); ++i) Buf.Add(((uint8*)&Value)[i]);
}

USceneCaptureDeferredComponent2D::USceneCaptureDeferredComponent2D(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USceneCaptureDeferredComponent2D::UpdateSceneCaptureContents(FSceneInterface* Scene)
{
	if (IsValid(CustomWorld))
	{
		USceneCaptureComponent2D::UpdateSceneCaptureContents(CustomWorld->Scene);
	}
}

bool FFisheyeCameraModel::ProjectUV2XYZ(const FVector2D& UV, FVector & Normal) const
{
	const double x_double_primed = ((double)UV.X - Cx) / Fx;
	const double y_double_primed = -((double)UV.Y - Cy) / Fy; //(-) - from UE4 UVs to OpenCV UVs

	const double r_double_primed = std::sqrt(x_double_primed*x_double_primed + y_double_primed*y_double_primed);

	const double cos_phi = x_double_primed / r_double_primed;
	const double sin_phi = y_double_primed / r_double_primed;

	//initial value for the ray incidence angle, theta
	const double theta_init = r_double_primed;
	double theta = theta_init;

	//find ray incidence angle using Newton's method
	for(int i = 0; i < N_Iter_Max; ++i) 
	{
		theta = theta - (R_D(theta) - r_double_primed) / R_D_Deriv(theta);
		if (std::fabs((R_D(theta) - r_double_primed)) < Eps) break;
	}

	//if (std::fabs(theta - theta_init) / M_PI > 0.1)
	//	return false;

	//Unit vector in the direction of the ray corresponding to the (u, v) pixel
	const double x = std::sin(theta) * cos_phi;
	const double y = std::sin(theta) * sin_phi;
	const double z = std::cos(theta);

	//From OpenCV to UE4 camera coordinate space
	Normal = FVector(+z, +x, -y); // Why +x, not -x ? I dont know...
	return true;
}

void FFisheyeCameraModel::ProjectXYZ2UV(const FVector& Point, FVector2D & UV) const
{
	//From UE4 to  OpenCV camera coordinate space
	const double x = +Point.Y; // Why +y, not -y ? I dont know...
	const double y = -Point.Z; 
	const double z = +Point.X;

	const double r = std::sqrt(x * x + y * y) + 1e-8;
	const double cs = x / r;
	const double sn = y / r;
	const double theta = std::atan2(r, z);

	const double theta_nl = Kb_Compute_Theta_Nl(theta);

	const double xn = cs * theta_nl;
	const double yn = sn * theta_nl;

	UV.X = Fx * xn + Cx;
	UV.Y = -Fy * yn + Cy; //(-) - From UE4 UVs to OpenCV UVs
}

double FFisheyeCameraModel::R_D(double theta) const
{
	return theta * (1.0 + K1 * std::pow(theta, 2.0) + K2 * std::pow(theta, 4.0) + K3 * std::pow(theta, 6.0) + K4 * std::pow(theta, 8.0));
}

double FFisheyeCameraModel::R_D_Deriv(double theta) const
{
	return (1 + 3.0 * K1 * std::pow(theta, 2.0) + 5.0 * K2 * std::pow(theta, 4.0) + 7.0 * K3 * std::pow(theta, 6.0) + 9.0 * K4 * std::pow(theta, 8.0));
}

double FFisheyeCameraModel::Kb_Compute_Theta_Nl(double theta) const
{
	return theta + K1 * std::pow(theta, 3) + K2 * std::pow(theta, 5) + K3 * std::pow(theta, 7) + K4 * std::pow(theta, 9);
}

UCameraFisheyeSensor::UCameraFisheyeSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)

{
	GUI.ComponentNameOverride = TEXT("Generic Fisheye Camera");
	GUI.bIsPresentInAddMenu = true;

	static ConstructorHelpers::FObjectFinder< UStaticMesh > Mesh1(TEXT("/Engine/BasicShapes/Plane"));
	static ConstructorHelpers::FObjectFinder< UMaterial > Mat1(TEXT("/SodaSim/Assets/CPP/FisheyeCamera/FisheyeMat1"));
	static ConstructorHelpers::FObjectFinder< UMaterial > Mat2(TEXT("/SodaSim/Assets/CPP/FisheyeCamera/FisheyeMat2"));
	static ConstructorHelpers::FObjectFinder< UMaterial > Mat3(TEXT("/SodaSim/Assets/CPP/FisheyeCamera/FisheyeMat3"));
	static ConstructorHelpers::FObjectFinder< UMaterial > Mat4(TEXT("/SodaSim/Assets/CPP/FisheyeCamera/FisheyeMat4"));
	static ConstructorHelpers::FObjectFinder< UMaterial > Mat5(TEXT("/SodaSim/Assets/CPP/FisheyeCamera/FisheyeMat5"));

	SqureMaterials.SetNum(5);

	if (Mesh1.Succeeded()) StaticMeshSqure = Mesh1.Object;
	if (Mat1.Succeeded()) SqureMaterials[0] = Mat1.Object;
	if (Mat2.Succeeded()) SqureMaterials[1] = Mat2.Object;
	if (Mat3.Succeeded()) SqureMaterials[2] = Mat3.Object;
	if (Mat4.Succeeded()) SqureMaterials[3] = Mat4.Object;
	if (Mat5.Succeeded()) SqureMaterials[4] = Mat5.Object;

	Format = ECameraSensorShader::HdrRGB8;

	DebugRings.Add(60);
	DebugRings.Add(90);
	DebugRings.Add(120);
	DebugRings.Add(150);
	DebugRings.Add(180);

	Width = 1920;
	Height = 1208;

	FOVSetup.Color = FLinearColor(0.1, 0.5, 0.5, 1.0);
	FOVSetup.MaxViewDistance = 200;

	bUseCustomAutoExposure = true;
	AutoExposureMinBrightness = 3.0;
	AutoExposureMaxBrightness = 4.0;
	AutoExposureBias = 0.0;
}

void UCameraFisheyeSensor::InitializeComponent()
{
	Super::InitializeComponent();
	TmpFileName = FPaths::CreateTempFilename(*(FPaths::ProjectSavedDir() / TEXT("FisheyeMap")), TEXT(""), TEXT(".tmp"));
}

void UCameraFisheyeSensor::Clean()
{
	if (IsValid(CaptureComponent))
	{
		CaptureComponent->DestroyComponent();
		if (CaptureComponent->TextureTarget)
		{
			CaptureComponent->TextureTarget->ConditionalBeginDestroy();
		}
	}
	CaptureComponent = nullptr;

	if (IsValid(PostProcessMat)) PostProcessMat->ConditionalBeginDestroy();
	PostProcessMat = nullptr;
	PostProcessSettings.WeightedBlendables.Array.Empty();

	if (IsValid(Mesh)) Mesh->DestroyComponent();
	Mesh = nullptr;

	if (IsValid(MaterialDyn)) MaterialDyn->ConditionalBeginDestroy();
	MaterialDyn = nullptr;
	
	if (CustomWorld)
	{
		GEngine->DestroyWorldContext(CustomWorld);
		CustomWorld->DestroyWorld(false);
		CustomWorld = nullptr;
	}

	for (int i = 0; i < FisheyeSides.Num(); ++i)
	{
		if (IsValid(FisheyeSides[i].SceneCapture))
		{
			FisheyeSides[i].SceneCapture->DestroyComponent();
			if (FisheyeSides[i].SceneCapture->TextureTarget)
			{
				FisheyeSides[i].SceneCapture->TextureTarget->ConditionalBeginDestroy();
			}
		}
	}
	FisheyeSides.Empty();
}

void UCameraFisheyeSensor::UninitializeComponent()
{
	Super::UninitializeComponent();

	if (FPaths::FileExists(*TmpFileName))
	{
		IFileManager::Get().Delete(*TmpFileName);
	}
}

void UCameraFisheyeSensor::ComputeProjectionMap(TArray<FVector>& ProjMap) const
{
	ProjMap.Empty();

	/*
	 * Try load projection map from temp file
	 */
	if (bStoreTempProjMap && FPaths::FileExists(*TmpFileName))
	{
		TArray<uint8> Data;
		if (!FFileHelper::LoadFileToArray(Data, *TmpFileName))
		{
			UE_LOG(LogSoda, Error, TEXT("UCameraFisheyeSensor::ComputeProjectionMap(); Can't load fisheye map '%s'"), *TmpFileName);
		}
		else
		{
			FMemoryReader MemoryReader(Data, true);
			MemoryReader << ProjMap;
			const int ExpectSize = Height * Width * FisheyeSides.Num();
			if (ProjMap.Num() != ExpectSize)
			{
				UE_LOG(LogSoda, Error, TEXT("UCameraFisheyeSensor::ComputeProjectionMap(); \"%s\" not suitable; Red %i , expect %i"), *TmpFileName, ProjMap.Num(), ExpectSize);
				ProjMap.Empty();
			}
		}
	}

	/*
	 * Otherwise compute projection map
	 */
	if (ProjMap.Num() == 0)
	{
		//auto t0 = soda::NowRaw();

		TArray<FVector> FisheyeXYZ;
		{
			FisheyeXYZ.SetNum(Width * Height);
			for (int v = 0; v < Height; ++v)
				for (int u = 0; u < Width; ++u)
				{
					FVector2D UV(u, Height - v);
					if (!FisheyeModel.ProjectUV2XYZ(UV, FisheyeXYZ[v * Width + u]))
						FisheyeXYZ[v * Width + u] = FVector::ZeroVector;
				}
		}

		float EdgeWidth = 0;
		if (bDrawEdges) EdgeWidth = 0.01;

		ProjMap.SetNum(Width * Height * FisheyeSides.Num());

		for (int i = 0; i < FisheyeSides.Num(); ++i)
		{
			FMatrix ProjectionMatrix;
			if ((int32)ERHIZBuffer::IsInverted)
			{
				ProjectionMatrix = FReversedZPerspectiveMatrix(
					FisheyeSides[i].FOV * (float)PI / 360.0f,
					FisheyeSides[i].FOV * (float)PI / 360.0f,
					1.0f,
					FisheyeSides[i].Width / (float)FisheyeSides[i].Height,
					GNearClippingPlane,
					GNearClippingPlane);
			}
			else
			{
				ProjectionMatrix = FPerspectiveMatrix(
					FisheyeSides[i].FOV * (float)PI / 360.0f,
					FisheyeSides[i].FOV * (float)PI / 360.0f,
					1.0f,
					FisheyeSides[i].Width / (float)FisheyeSides[i].Height,
					GNearClippingPlane,
					GNearClippingPlane);
			}

			FMatrix ViewRotationMatrix = FRotationMatrix::Make(CommonQuat * FisheyeSides[i].CamRotator).Inverse() * FMatrix(
				FPlane(0, 0, 1, 0),
				FPlane(1, 0, 0, 0),
				FPlane(0, 1, 0, 0),
				FPlane(0, 0, 0, 1));
			FMatrix ViewProjectionMatrix = ViewRotationMatrix * ProjectionMatrix;

			for (int n = 0; n < Width * Height; ++n)
			{
				FVector XYZ = FisheyeXYZ[n];
				FVector2D UV;
				static FIntRect ViewRect(0, 0, 1, 1);
				if (!XYZ.IsZero() && FSceneView::ProjectWorldToScreen(XYZ * 100, ViewRect, ViewProjectionMatrix, UV))
				{
					ProjMap[n + Width * Height * i].X = UV.X;
					ProjMap[n + Width * Height * i].Y = UV.Y;
					ProjMap[n + Width * Height * i].Z = (UV.X >= EdgeWidth && UV.X < (1 - EdgeWidth) && UV.Y >= EdgeWidth && UV.Y < (1 - EdgeWidth)) ? 1 : 0;
				}
				else
				{
					ProjMap[n + Width * Height * i] = FVector::ZeroVector;
				}
			}
		}

		//UE_LOG(LogSoda, Log, TEXT("Calculated/loaded fisheye camera model for %d ms"), int(soda::NowRaw() - t0));

		/*
		 * Save projection map
		 */
		if (bStoreTempProjMap)
		{
			TArray<uint8> Data;
			FMemoryWriter MemoryWriter(Data, true);
			MemoryWriter << ProjMap;
			if (!FFileHelper::SaveArrayToFile(Data, *TmpFileName))
			{
				UE_LOG(LogSoda, Error, TEXT("UCameraFisheyeSensor::ComputeProjectionMap(); Can't save fisheye map to '%s'"), *TmpFileName);
			}
		}
	}
}

bool UCameraFisheyeSensor::ApplyProjectionMap(const TArray<FVector> & ProjMap)
{
	check(CaptureComponent);
	check(MaterialDyn);
	check(ProjMap.Num() == Width * Height * FisheyeSides.Num());

	CaptureComponent->bCaptureEveryFrame = false;
	for (int i = 0; i < FisheyeSides.Num(); i++) FisheyeSides[i].SceneCapture->bCaptureEveryFrame = false;
	
	for(int i = 0; i < FisheyeSides.Num(); ++i)
	{
		FisheyeSides[i].SceneCapture->TextureTarget->InitAutoFormat(FisheyeSides[i].Width, FisheyeSides[i].Height);
		FisheyeSides[i].UVTexture = UTexture2D::CreateTransient(Width, Height, EPixelFormat::PF_A32B32G32R32F);
		checkf(FisheyeSides[i].UVTexture, TEXT("Can't create texture"));
		FisheyeSides[i].UVTexture->AddressX = TextureAddress::TA_Clamp;
		FisheyeSides[i].UVTexture->AddressY = TextureAddress::TA_Clamp;

		struct FTexData { float R; float G; float B; float A; };
		check(sizeof(FTexData) == 4*4);

		FTexData * TextureData = (FTexData*)FisheyeSides[i].UVTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		check(TextureData);

		for(int n = 0; n < Width * Height; ++n)
		{
			TextureData[n].R = ProjMap[n + Width * Height * i].X;
			TextureData[n].G = ProjMap[n + Width * Height * i].Y;
			TextureData[n].A = ProjMap[n + Width * Height * i].Z;
			TextureData[n].B = 0;
		}

		FisheyeSides[i].UVTexture->GetPlatformData()->Mips[0].BulkData.Unlock();


	}

	auto UpdateTex = [this]()
	{
		for (int i = 0; i < FisheyeSides.Num(); ++i)
		{
			FisheyeSides[i].UVTexture->UpdateResource();
			MaterialDyn->SetTextureParameterValue(*(FString("bitmap_tex") + FString::FromInt(i)), FisheyeSides[i].SceneCapture->TextureTarget);
			MaterialDyn->SetTextureParameterValue(*(FString("uv_tex") + FString::FromInt(i)), FisheyeSides[i].UVTexture);
		}
	};

	if (!IsInGameThread())
	{
		::AsyncTask(ENamedThreads::GameThread, [this, UpdateTex]()
		{
			if(IsValid(this) && FisheyeSides.Num())
			{
				UpdateTex();
			} 
		});
	}
	else
	{
		UpdateTex();
	}

	CaptureComponent->bCaptureEveryFrame = true;
	for (int i = 0; i < FisheyeSides.Num(); i++) FisheyeSides[i].SceneCapture->bCaptureEveryFrame = true;

	return true;
}

void UCameraFisheyeSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugRings)
	{
		static const  int ReingSteps = 80;
		static const float RayLength = 1;

		FTransform Trans = GetComponentTransform();
		for (float RingAng : DebugRings)
		{
			float X = RayLength * cos(RingAng / 2.0 / 180.0 * M_PI);
			float R = RayLength * sin(RingAng / 2.0 / 180.0 * M_PI);
			FVector Pt1 = Trans.TransformPosition(FVector(X, R, 0.0));
			for (int i = 0; i < ReingSteps; i++)
			{
				float A = float((i + 1) % ReingSteps) / float(ReingSteps) * 2.0 * M_PI;
				FVector Pt2 = Trans.TransformPosition(FVector(X, cos(A) * R, sin(A) * R));
				DrawDebugLine(GetWorld(), Pt1, Pt2, FColor(0, 255, 0), false, 0.0f, 0, 0.005f);
				Pt1 = Pt2;
			}
		}
	}

	//if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

}

bool UCameraFisheyeSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (InitializeProjectionFuture.valid()) InitializeProjectionFuture.wait();

	Clean();

	FisheyeModel.K1 = IntrinsicsD0;
	FisheyeModel.K2 = IntrinsicsD1;
	FisheyeModel.K3 = IntrinsicsD2;
	FisheyeModel.K4 = IntrinsicsD3;
	FisheyeModel.Cx = IntrinsicsCX;
	FisheyeModel.Cy = IntrinsicsCY;
	FisheyeModel.Fx = IntrinsicsFX;
	FisheyeModel.Fy = IntrinsicsFY;

	CustomWorld = UWorld::CreateWorld(EWorldType::Game, false);
	check(CustomWorld);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(CustomWorld);

	switch (Format)
	{
	case ECameraSensorShader::Segm8:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.Segm, this);
		break;

	case ECameraSensorShader::SegmBGR8:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.SegmColor, this);
		break;

	case ECameraSensorShader::CFA:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.CFA, this);
		break;

	case ECameraSensorShader::DepthFloat32:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.DepthAbsEnc, this);
		break;

	case ECameraSensorShader::Depth8:
	case ECameraSensorShader::Depth16:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.DepthNormEnc, this);
		PostProcessMat->SetScalarParameterValue("MaxDist", MaxDepthDistance * 100.f);
		break;
	}

	if (PostProcessMat)
	{
		PostProcessSettings.WeightedBlendables.Array.AddDefaulted(1);
		PostProcessSettings.WeightedBlendables.Array[0].Object = PostProcessMat;
		PostProcessSettings.WeightedBlendables.Array[0].Weight = 1.f;
	}

	const FFisheyeSide FisheyeSidesCenter(FQuat(FRotator(0, 0, 0)), CenterFaceFOV, CenterFaceTexWidth, CenterFaceTexHeight);
	const FFisheyeSide FisheyeSidesLeft(FQuat(FRotator(0, -(CenterFaceFOV + LeftRightFaceFOV) / 2, 0)), LeftRightFaceFOV, LeftRightFaceTexWidth, LeftRightFaceTexHeight);
	const FFisheyeSide FisheyeSidesRight(FQuat(FRotator(0, (CenterFaceFOV + LeftRightFaceFOV) / 2, 0)), LeftRightFaceFOV, LeftRightFaceTexWidth, LeftRightFaceTexHeight);
	const FFisheyeSide FisheyeSidesUp(FQuat(FRotator((CenterFaceFOV + UpDownFaceFOV) / 2, 0, 0)), UpDownFaceFOV, UpDownFaceTexWidth, UpDownFaceTexHeight);
	const FFisheyeSide FisheyeSidesDown(FQuat(FRotator(-(CenterFaceFOV + UpDownFaceFOV) / 2, 0, 0)), UpDownFaceFOV, UpDownFaceTexWidth, UpDownFaceTexHeight);

	switch (FisheyeConstructMode)
	{
	case EFisheyeConstructMode::OneFace:
		FisheyeSides.SetNum(1);
		FisheyeSides[0] = FisheyeSidesCenter;
		CommonQuat = FQuat(FRotator(0, 0, 0));
		break;

	case EFisheyeConstructMode::TowFaces:
		FisheyeSides.SetNum(2);
		FisheyeSides[0] = FisheyeSidesCenter;
		FisheyeSides[1] = FisheyeSidesLeft;
		CommonQuat = FQuat(FRotator(0, CenterFaceFOV / 2, 0));
		break;

	case EFisheyeConstructMode::ThreeFacesLine:
		FisheyeSides.SetNum(3);
		FisheyeSides[0] = FisheyeSidesCenter;
		FisheyeSides[1] = FisheyeSidesLeft;
		FisheyeSides[2] = FisheyeSidesRight;
		CommonQuat = FQuat(FRotator(0, 0, 0));
		break;

	case EFisheyeConstructMode::ThreeFaces:
		FisheyeSides.SetNum(3);
		FisheyeSides[0] = FisheyeSidesCenter;
		FisheyeSides[1] = FisheyeSidesLeft;
		FisheyeSides[2] = FisheyeSidesDown;
		CommonQuat = FQuat(FRotator(45, 0, -45));
		break;

	case EFisheyeConstructMode::FourFaces:
		FisheyeSides.SetNum(4);
		FisheyeSides[0] = FisheyeSidesCenter;
		FisheyeSides[1] = FisheyeSidesLeft;
		FisheyeSides[2] = FisheyeSidesRight;
		FisheyeSides[3] = FisheyeSidesDown;
		CommonQuat = FQuat(FRotator(CenterFaceFOV / 2, 0, 0));
		break;

	case EFisheyeConstructMode::FiveFaces:
		FisheyeSides.SetNum(5);
		FisheyeSides[0] = FisheyeSidesCenter;
		FisheyeSides[1] = FisheyeSidesLeft;
		FisheyeSides[2] = FisheyeSidesRight;
		FisheyeSides[3] = FisheyeSidesUp;
		FisheyeSides[4] = FisheyeSidesDown;
		CommonQuat = FQuat(FRotator(0, 0, 0));
		break;

	default:
		check(0);
	}

	if (bUseCustomProjectRotation)
	{
		CommonQuat = FQuat(CustomProjectRotation);
	}

	for (int i = 0; i < FisheyeSides.Num(); ++i)
	{
		FisheyeSides[i].SceneCapture = NewObject< USceneCaptureComponent2D >(this);
		FisheyeSides[i].SceneCapture->HideComponent(this);
		FisheyeSides[i].SceneCapture->CaptureSortPriority = 2;
		FisheyeSides[i].SceneCapture->CaptureSource = (Format == ECameraSensorShader::HdrRGB8) ? ESceneCaptureSource::SCS_SceneColorHDR : ESceneCaptureSource::SCS_FinalColorLDR;
		FisheyeSides[i].SceneCapture->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
		FisheyeSides[i].SceneCapture->TextureTarget->bForceLinearGamma = true;
		FisheyeSides[i].SceneCapture->TextureTarget->RenderTargetFormat = RTF_RGBA16f;
		FisheyeSides[i].SceneCapture->TextureTarget->AddressX = TextureAddress::TA_Clamp;
		FisheyeSides[i].SceneCapture->TextureTarget->AddressY = TextureAddress::TA_Clamp;
		if (Format == ECameraSensorShader::Segm8) FisheyeSides[i].SceneCapture->TextureTarget->Filter = TextureFilter::TF_Nearest;
		FisheyeSides[i].SceneCapture->FOVAngle = FisheyeSides[i].FOV;
		FisheyeSides[i].SceneCapture->PrimitiveRenderMode = PrimitiveRenderMode;
		FisheyeSides[i].SceneCapture->HiddenActors = HiddenActors;
		FisheyeSides[i].SceneCapture->ShowOnlyActors = ShowOnlyActors;
		FisheyeSides[i].SceneCapture->bAlwaysPersistRenderingState = bAlwaysPersistRenderingState;
		FisheyeSides[i].SceneCapture->LODDistanceFactor = LODDistanceFactor;
		FisheyeSides[i].SceneCapture->MaxViewDistanceOverride = MaxViewDistanceOverride;
		FisheyeSides[i].SceneCapture->ShowFlags = ShowFlags;
		//FisheyeSides[i].SceneCapture->ShowFlags.EnableAdvancedFeatures();
		FisheyeSides[i].SceneCapture->CompositeMode = CompositeMode;
		FisheyeSides[i].SceneCapture->PostProcessSettings = PostProcessSettings;
		FisheyeSides[i].SceneCapture->PostProcessBlendWeight = PostProcessBlendWeight;
		FisheyeSides[i].SceneCapture->ShowFlagSettings = ShowFlagSettings;
		FisheyeSides[i].SceneCapture->bCaptureOnMovement = false;
		FisheyeSides[i].SceneCapture->bCaptureEveryFrame = false;
		FisheyeSides[i].SceneCapture->SetupAttachment(this);
		FisheyeSides[i].SceneCapture->SetRelativeRotation(CommonQuat * FisheyeSides[i].CamRotator);
		FisheyeSides[i].SceneCapture->RegisterComponent();
		//FisheyeSides[i].SceneCapture->HideComponent(Mesh);
	}

	MaterialDyn = UMaterialInstanceDynamic::Create(SqureMaterials[FisheyeSides.Num() - 1], CustomWorld);
	Mesh = NewObject< UStaticMeshComponent >(CustomWorld);
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetStaticMesh(StaticMeshSqure);
	Mesh->SetRelativeRotation(FRotator(0.0, 90.0, 90.0));
	Mesh->SetRelativeScale3D(FVector(1, 1 * float(Height) / Width, 1.0));
	Mesh->SetMaterial(0, MaterialDyn);
	Mesh->SetWorldLocation(FVector(1, 0, 0));
	Mesh->RegisterComponentWithWorld(CustomWorld);

	if (bUseCustomAutoExposure)
	{
		FisheyePostProcessSettings.AutoExposureMinBrightness = AutoExposureMinBrightness;
		FisheyePostProcessSettings.AutoExposureMaxBrightness = AutoExposureMaxBrightness;
		FisheyePostProcessSettings.AutoExposureBias = AutoExposureBias;

		FisheyePostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		FisheyePostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		FisheyePostProcessSettings.bOverride_AutoExposureBias = true;
	}

	FisheyePostProcessSettings.bOverride_BloomIntensity = true;
	FisheyePostProcessSettings.BloomIntensity = BloomIntensity;

	CaptureComponent = NewObject< USceneCaptureDeferredComponent2D >(this);
	CaptureComponent->CustomWorld = CustomWorld;
	CaptureComponent->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
	CaptureComponent->TextureTarget->InitCustomFormat(Width, Height, EPixelFormat::PF_B8G8R8A8, !IsColorFormat(Format) || bForceLinearGamma);
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureComponent->OrthoWidth = 100;
	CaptureComponent->PostProcessSettings = FisheyePostProcessSettings;
	CaptureComponent->PostProcessBlendWeight = 1.0;
	CaptureComponent->CaptureSource = (IsColorFormat(Format) ? ESceneCaptureSource::SCS_FinalColorLDR : ESceneCaptureSource::SCS_SceneColorHDR);
	CaptureComponent->ShowOnlyComponent(Mesh);
	CaptureComponent->SetWorldLocation(FVector(0, 0, 0));
	CaptureComponent->RegisterComponent();

	bIsProjectionInitializing = true;
	InitializeProjectionFuture = std::async(std::launch::async, [this]()
	{
		TArray<FVector> ProjMap;
		ComputeProjectionMap(ProjMap);
		ApplyProjectionMap(ProjMap);
		
		bIsProjectionInitializing = false;
	});
	
	return true;
}

void UCameraFisheyeSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (InitializeProjectionFuture.valid()) InitializeProjectionFuture.wait();

	Clean();
}

void UCameraFisheyeSensor::RecalculateProjection()
{
	if (FPaths::FileExists(*TmpFileName))
	{
		IFileManager::Get().Delete(*TmpFileName);
	}

	TArray<FVector> ProjMap;
	ComputeProjectionMap(ProjMap);
	if (HealthIsWorkable())
	{
		ApplyProjectionMap(ProjMap);
	}

	MarkRenderStateDirty();
}

bool UCameraFisheyeSensor::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	static const  int RingsSteps = 30;
	static const int RadStep = 15;


	FVector2D UV1, UV2;
	FisheyeModel.K1 = IntrinsicsD0;
	FisheyeModel.K2 = IntrinsicsD1;
	FisheyeModel.K3 = IntrinsicsD2;
	FisheyeModel.K4 = IntrinsicsD3;
	FisheyeModel.Cx = IntrinsicsCX;
	FisheyeModel.Cy = IntrinsicsCY;
	FisheyeModel.Fx = IntrinsicsFX;
	FisheyeModel.Fy = IntrinsicsFY;
	FisheyeModel.ProjectXYZ2UV(FVector(1, 0, 0).RotateAngleAxis(MaxFOV / 2, FVector(0, 0, 1)), UV1);
	FisheyeModel.ProjectXYZ2UV(FVector(1, 0, 0).RotateAngleAxis(-MaxFOV / 2, FVector(0, 0, 1)), UV2);
	
	const float UVMaxRad = std::abs(UV1.X - UV2.X) / 2;
	const FVector2D Center(float(Width) / 2, float(Height) / 2);

	TArray<FVector> Cloud;
	TArray<FVector2D> CloudUV;
	TArray <int32> Hull;

	Cloud.Add(FVector(1, 0, 0) * FOVSetup.MaxViewDistance);
	CloudUV.Add(Center);

	for (int j = 0; j < RingsSteps; j++)
	{
		float A = float((j + 1) % RingsSteps) / float(RingsSteps) * 2 * M_PI;
		FVector2D Vec(cos(A), sin(A));
		bool IsAdded = false;
		for (int i = 1; i <= RadStep; i++)
		{
			float R = UVMaxRad / float(RadStep) * i;
			FVector2D UV = Vec * R + Center;
			FVector2D UVt = UV;
			UV.Y = Height - UV.Y;
			bool IsEnd = false;
			if (UV.X < 0)
			{ 
				if (!Intersection(Center, UV, FVector2D(0, 0), FVector2D(0, Height), &UV)) break;
				IsEnd = true;
			} 
			else if (UV.Y < 0)
			{
				if (!Intersection(Center, UV, FVector2D(0, 0), FVector2D(Width, 0), &UV)) break;
				IsEnd = true;
			}
			else if (UV.X >= Width)
			{
				if (!Intersection(Center, UV, FVector2D(Width, 0), FVector2D(Width, Height), &UV)) break;
				IsEnd = true;
			}
			else if (UV.Y >= Height)
			{
				if (!Intersection(Center, UV, FVector2D(0, Height), FVector2D(Width, Height), &UV)) break;
				IsEnd = true;
			}
						
			FVector Norm;
			if (FisheyeModel.ProjectUV2XYZ(UV, Norm))
			{
				Cloud.Add(Norm * FOVSetup.MaxViewDistance);
				CloudUV.Add((UVt));
				IsAdded = true;
			}			
			
			if (IsEnd) break;
		}

		if(IsAdded) Hull.Add(CloudUV.Num() - 1);

	}

	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateFOVMesh2D(Cloud, CloudUV, Hull, MeshData.Vertices, MeshData.Indices))
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

void UCameraFisheyeSensor::MakeScreenshot(const FString& InFileName)
{
	if (!HealthIsWorkable()) return;

	if (!CaptureComponent->bCaptureEveryFrame)
	{
		for (int i = 0; i < FisheyeSides.Num(); i++)
		{
			FisheyeSides[i].SceneCapture->CaptureScene();
		}
	}

	Super::MakeScreenshot(InFileName);
}

