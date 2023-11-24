// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/CameraSensor.h"
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
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"

/*
DECLARE_STATS_GROUP(TEXT("CameraSensor"), STATGROUP_CameraSensor, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_CameraTick, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("Publish"), STAT_CameraPublish, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("Compute"), STAT_CameraCompute, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("ReadPixels"), STAT_CameraRead, STATGROUP_CameraSensor);
DECLARE_CYCLE_STAT(TEXT("BigBlockMemcpy"), STAT_CameraBigBlockMemcpy, STATGROUP_CameraSensor);
*/

/* **********************************************************************************************
 * UCameraSensorComponent
 * **********************************************************************************************/

UCameraSensorComponent::UCameraSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Camera Sensors");
	GUI.ComponentNameOverride = TEXT("Generic Camera");
	GUI.bIsPresentInAddMenu = true;

	FOVSetup.Color = FLinearColor(0.1, 0.5, 0.5, 1.0);
	FOVSetup.MaxViewDistance = 300;
}

bool UCameraSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	Clean();

	if (bUseCustomAutoExposure)
	{
		PostProcessSettings.AutoExposureMinBrightness = AutoExposureMinBrightness;
		PostProcessSettings.AutoExposureMaxBrightness = AutoExposureMaxBrightness;
		PostProcessSettings.AutoExposureBias = AutoExposureBias;

		PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		PostProcessSettings.bOverride_AutoExposureBias = true;
	}

	switch (Format)
	{
	case ECameraSensorShader::Segm8:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.Segm, this);
		break;

	case ECameraSensorShader::CFA:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.CFA, this);
		break;

	case ECameraSensorShader::SegmBGR8:
		PostProcessMat = UMaterialInstanceDynamic::Create(CameraPostProcessMaterals.SegmColor, this);
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

	CaptureComponent = NewObject< USceneCaptureComponent2D >(this);
	CaptureComponent->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
	CaptureComponent->TextureTarget->InitCustomFormat(Width, Height, EPixelFormat::PF_B8G8R8A8, !IsColorFormat(Format) || bForceLinearGamma);
	CaptureComponent->bCaptureEveryFrame = true;
	CaptureComponent->bCaptureOnMovement = true;
	CaptureComponent->PrimitiveRenderMode = PrimitiveRenderMode;
	CaptureComponent->HiddenActors = HiddenActors;
	CaptureComponent->ShowOnlyActors = ShowOnlyActors;
	CaptureComponent->bAlwaysPersistRenderingState = bAlwaysPersistRenderingState;
	CaptureComponent->LODDistanceFactor = LODDistanceFactor;
	CaptureComponent->MaxViewDistanceOverride = MaxViewDistanceOverride;
	CaptureComponent->CaptureSortPriority = CaptureSortPriority;
	CaptureComponent->ShowFlags = ShowFlags;
	//CaptureComponent->ProjectionType = ProjectionType;
	CaptureComponent->FOVAngle = FOVAngle;
	//CaptureComponent->OrthoWidth = OrthoWidth;
	CaptureComponent->CompositeMode = CompositeMode;
	CaptureComponent->PostProcessSettings = PostProcessSettings;
	CaptureComponent->PostProcessBlendWeight = PostProcessBlendWeight;
	CaptureComponent->ShowFlagSettings = ShowFlagSettings;
	CaptureComponent->ShowFlags.EnableAdvancedFeatures();
	CaptureComponent->CaptureSource = (Format == ECameraSensorShader::HdrRGB8) ? ESceneCaptureSource::SCS_SceneColorHDR : ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->HideComponent(this);
	CaptureComponent->SetupAttachment(this);
	CaptureComponent->RegisterComponent();

	return true;
}

void UCameraSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	Clean();
}

void UCameraSensorComponent::Clean()
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
}

void UCameraSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

}

bool UCameraSensorComponent::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateSimpleFOVMesh(
		GetHFOV(),
		GetVFOV(),
		20, 20, FOVSetup.MaxViewDistance, 0, 0,
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