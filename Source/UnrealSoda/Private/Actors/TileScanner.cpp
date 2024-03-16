// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/TileScanner.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "ImagePixelData.h"
#include "ImageWriteTask.h"
#include "HighResScreenshot.h"
#include "ImageWriteQueue.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Async/ParallelFor.h"
#include "RenderingThread.h"
#include "Soda/Misc/PixelReader.h"

ATileScanner::ATileScanner()
{
	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	//CaptureComponent->TextureTarget = CreateDefaultSubobject< UTextureRenderTarget2D >(TEXT("TextureTarget"));
	//CaptureComponent->TextureTarget->InitCustomFormat(TileSize, TileSize, EPixelFormat::PF_B8G8R8A8, false);
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	//CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	//CaptureComponent->OrthoWidth = TileSize;

	/*
	CaptureComponent->PrimitiveRenderMode = PrimitiveRenderMode;
	CaptureComponent->HiddenActors = HiddenActors;
	CaptureComponent->ShowOnlyActors = ShowOnlyActors;
	CaptureComponent->bAlwaysPersistRenderingState = bAlwaysPersistRenderingState;
	CaptureComponent->LODDistanceFactor = LODDistanceFactor;
	CaptureComponent->MaxViewDistanceOverride = MaxViewDistanceOverride;
	CaptureComponent->CaptureSortPriority = CaptureSortPriority;
	CaptureComponent->ShowFlags = ShowFlags;

	CaptureComponent->FOVAngle = FOVAngle;
	CaptureComponent->OrthoWidth = OrthoWidth;
	CaptureComponent->CompositeMode = CompositeMode;
	CaptureComponent->PostProcessSettings = PostProcessSettings;
	CaptureComponent->PostProcessBlendWeight = PostProcessBlendWeight;
	CaptureComponent->ShowFlagSettings = ShowFlagSettings;
	CaptureComponent->ShowFlags.EnableAdvancedFeatures();
	CaptureComponent->CaptureSource = (Format == CameraSensorShader::HdrRGB8) ? ESceneCaptureSource::SCS_SceneColorHDR : ESceneCaptureSource::SCS_FinalColorLDR;
	*/
}

void ATileScanner::BeginPlay()
{
	Super::BeginPlay();

	CaptureComponent->TextureTarget = NewObject< UTextureRenderTarget2D >(this);
	CaptureComponent->TextureTarget->InitCustomFormat(TileSize, TileSize, EPixelFormat::PF_B8G8R8A8, false);
}

void ATileScanner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ATileScanner::MakeScreenshot(const FString& InFileName)
{
	if (!CaptureComponent->bCaptureEveryFrame)
	{
		CaptureComponent->CaptureScene();
	}

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)([this, InFileName](FRHICommandListImmediate& RHICmdList)
	{
		TArray<FColor> OutPixels;
		uint32 ImageStride;
		FCameraPixelReader::ReadPixels(*CaptureComponent->TextureTarget, RHICmdList, OutPixels, ImageStride);
		TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(TileSize, TileSize));
		PixelData->Pixels.SetNum(TileSize * TileSize, false);
		ParallelFor(TileSize, [&](int32 i)
			{
				FMemory::Memcpy(
					&PixelData->Pixels[i * TileSize],
					&OutPixels[i * ImageStride],
					TileSize * 4);
			});

		TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
		ImageTask->PixelData = MoveTemp(PixelData);
		ImageTask->Filename = InFileName;
		ImageTask->Format = EImageFormat::PNG;
		ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
		ImageTask->bOverwriteFile = true;
		ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
		FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
		//TFuture<bool> Res = 
		HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
	});
}

void ATileScanner::Scan()
{
	FVector Loc = GetActorLocation();
	const float H = (OrthoWidth - ZeroLevel) / 2;
	int Ind = 0;
	for (int i = 0; i < UpCount + DownCount; ++i)
	{
		float Y = (-UpCount + i + 0.5) * OrthoWidth;
		for (int j = 0; j < LeftCount + RightCount; ++j)
		{
			float X = (-LeftCount + j + 0.5) * OrthoWidth;
			SetActorLocation(FVector(X, Y, H));
			MakeScreenshot(FString("C:\\Temp\\") + FString::FromInt(Ind));
			++Ind;
		}
	}
	SetActorLocation(Loc);
}
