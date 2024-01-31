// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericLidarDepth2DSensor.h"
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

UGenericLidarDepth2DSensor::UGenericLidarDepth2DSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic LiDAR 2D Depth");
	GUI.bIsPresentInAddMenu = true;
}

void UGenericLidarDepth2DSensor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericLidarDepth2DSensor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
}

bool UGenericLidarDepth2DSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	LidarRays.SetNum(Channels * Step);

	for (int v = 0; v < Channels; ++v)
	{
		const float AngleY = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)(Channels - 1) * (float)v;
		for (int u = 0; u < Step; ++u)
		{
			auto& Ray = LidarRays[Step * v + u] = FVector::ForwardVector;

			const float AngleX = FOV_HorizontMin + (FOV_HorizontMax - FOV_HorizontMin) / (float)(Step - 1) * (float)u;
			//const float LenCoef = cos(FMath::DegreesToRadians(AngleX)) * cos(FMath::DegreesToRadians(AngleY));

			Ray = Ray.RotateAngleAxis(AngleY, FVector(0.f, 1.f, 0.f));
			Ray = Ray.RotateAngleAxis(AngleX, FVector(0.f, 0.f, 1.f));
			//Ray = Ray / LenCoef;
		}
	}

	int PointsSkiped = GenerateUVs(LidarRays, CameraFOV, TextureWidth, TextureHeight, Interpolation, UVs);

	if (PointsSkiped)
	{
		AddDebugMessage(EVehicleComponentHealth::Warning, FString::Printf(TEXT("%i point(s) did't include into the FOV"), PointsSkiped));
	}

	return PublisherHelper.Advertise();
}

void UGenericLidarDepth2DSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PublisherHelper.Shutdown();
	LidarRays.Reset();
	UVs.Reset();
}

bool UGenericLidarDepth2DSensor::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericLidarDepth2DSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericLidarDepth2DSensor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericLidarDepth2DSensor::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
}
#endif

void UGenericLidarDepth2DSensor::PostProcessSensorData(soda::FLidarScan& InScan)
{
	check(InScan.Points.Num() == Channels * Step);

	for (int k = 0; k < Channels * Step; ++k)
	{
		InScan.Points[k].Layer = Channels - k / Step - 1;
	}
}

bool UGenericLidarDepth2DSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, Scan);
	}
	return false;
}
