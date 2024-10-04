// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/LidarRayTraceSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Physics/PhysicsFiltering.h"
#include "DrawDebugHelpers.h"
#include "UObject/UObjectIterator.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"

DECLARE_STATS_GROUP(TEXT("LidarRayTraceSensor"), STATGROUP_LidarRayTraceSensor, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_TickComponent, STATGROUP_LidarRayTraceSensor);
DECLARE_CYCLE_STAT(TEXT("batch execute"), STAT_BatchExecute, STATGROUP_LidarRayTraceSensor);
DECLARE_CYCLE_STAT(TEXT("Add to batch execute"), STAT_AddToBatch, STATGROUP_LidarRayTraceSensor);
DECLARE_CYCLE_STAT(TEXT("Process query results"), STAT_ProcessQueryResults, STATGROUP_LidarRayTraceSensor);
DECLARE_CYCLE_STAT(TEXT("Publish results"), STAT_PublishResults, STATGROUP_LidarRayTraceSensor);

ULidarRayTraceSensor::ULidarRayTraceSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool ULidarRayTraceSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	return true;
}

void ULidarRayTraceSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Scan.Points.Reset();
	BatchStart.Reset();
	BatchEnd.Reset();
}


void ULidarRayTraceSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	SCOPE_CYCLE_COUNTER(STAT_TickComponent);

	const FVector Loc = GetComponentLocation();
	const FRotator Rot = GetComponentRotation();
	const TArray<FVector>& Rays = GetLidarRays();

	BatchStart.SetNum(Rays.Num(), false);
	BatchEnd.SetNum(Rays.Num(), false);

	{
		SCOPE_CYCLE_COUNTER(STAT_AddToBatch);
		for (int i = 0; i < Rays.Num(); i++)
		{
			const FVector WorldRay = Rot.RotateVector(Rays[i]);
			BatchStart[i] = Loc + WorldRay * GetLidarMinDistance();
			BatchEnd[i] = Loc + WorldRay * GetLidarMaxDistance();
		}
	}

	TArray<FHitResult> OutHits;
	{
		SCOPE_CYCLE_COUNTER(STAT_BatchExecute);

		FSodaPhysicsInterface::RaycastSingleScope(
			GetWorld(), OutHits, BatchStart, BatchEnd,
			ECollisionChannel::ECC_Visibility,
			FCollisionQueryParams(NAME_None, false, GetOwner()),
			FCollisionResponseParams::DefaultResponseParam,
			FCollisionObjectQueryParams::DefaultObjectQueryParam);

	};
	check(OutHits.Num() == Rays.Num());

	Scan.Points.SetNum(Rays.Num(), false);
	Scan.HorizontalAngleMax = GetFOVHorizontMax();
	Scan.HorizontalAngleMin = GetFOVHorizontMin();
	Scan.VerticalAngleMin = GetFOVVerticalMin();
	Scan.VerticalAngleMax = GetFOVVerticalMax();
	Scan.RangeMin = GetLidarMinDistance();
	Scan.RangeMax = GetLidarMaxDistance();
	Scan.Size = GetLidarSize();

	for (int k = 0; k < Rays.Num(); ++k)
	{
		Scan.Points[k].Location = Rot.UnrotateVector(OutHits[k].Location - Loc);
		Scan.Points[k].Depth = Scan.Points[k].Location.Size();
		if (OutHits[k].bBlockingHit)
		{
			Scan.Points[k].Status = soda::ELidarPointStatus::Valid;

			if (bEnabledGroundFilter)
			{
				if ((Loc.Z - OutHits[k].Location.Z) > DistanceToGround)
				{
					Scan.Points[k].Status = soda::ELidarPointStatus::Filtered;
				}
			}
		}
		else
		{
			Scan.Points[k].Status = soda::ELidarPointStatus::Invalid;
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PublishResults);
		PublishSensorData(DeltaTime, GetHeaderGameThread(), Scan);
	}
	DrawLidarPoints(Scan, false);

	/*

	if(DrawTracedRays || bDrawNotTracedRays) 
	{
		for (int i = 0; i < Channels; i++)
		{
			float VerticalAng = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)Channels * (float)i;
			for (int j = 0; j < Step; j++)
			{
				float HorizontAng = FOV_HorizontMin + (FOV_HorizontMax - FOV_HorizontMin) / (float)Step * (float)j;
				FVector RayNorm = Rot.RotateVector(FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));
				bool Traced = OutHits[i * Step + j].bBlockingHit;
				FVector HitPosition = OutHits[i * Step + j].Location;
				if (DrawTracedRays && Traced)
				{
					DrawDebugLine(GetWorld(), Loc + RayNorm * DistanseMin, HitPosition, FColor(255, 0, 0), false, -1.f, 0, 2.f);
				}
				else if (bDrawNotTracedRays && !Traced)
				{
					DrawDebugLine(GetWorld(), Loc + RayNorm * DistanseMin, Loc + RayNorm * DistanseMax, FColor(0, 255, 0), false, -1.f, 0, 2.f);
				}
			}
		}
	}
	*/
}
