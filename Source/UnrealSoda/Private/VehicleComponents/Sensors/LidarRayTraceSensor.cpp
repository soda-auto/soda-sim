// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/LidarRayTraceSensor.h"
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

ULidarRayTraceSensorComponent::ULidarRayTraceSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("LiDAR Sensors");
	GUI.ComponentNameOverride = TEXT("Generic LiDAR Ray Trace");
	GUI.IcanName = TEXT("SodaIcons.Lidar");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	FOVSetup.Color = FLinearColor(1.0, 0.1, 0.3, 1.0);
	FOVSetup.MaxViewDistance = 700;
}

bool ULidarRayTraceSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PointCloudPublisher.Advertise();

	if (!PointCloudPublisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void ULidarRayTraceSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	PointCloudPublisher.Shutdown();
}

void ULidarRayTraceSensorComponent::GetRemark(FString & Info) const
{
	Info = PointCloudPublisher.Address + ":" + FString::FromInt(PointCloudPublisher.Port);
}

void ULidarRayTraceSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	SCOPE_CYCLE_COUNTER(STAT_TickComponent);

	if (!IsTickOnCurrentFrame()) return;

	if (!PointCloudPublisher.IsAdvertised())
	{
		return;
	}

	FVector Loc = GetComponentLocation();
	FRotator Rot = GetComponentRotation();
	FTransform RelTrans = GetRelativeTransform();
	
	TTimestamp Timestemp = SodaApp.GetSimulationTimestamp();

	if(bLogTick)
	{
		UE_LOG(LogSoda, Warning, TEXT("LidarRay name: %s, timestamp: %s"), 
			*GetFName().ToString(),  
			*soda::ToString(Timestemp));
	}

	BatchStart.SetNum(Channels * Step, false);
	BatchEnd.SetNum(Channels * Step, false);
	{
		SCOPE_CYCLE_COUNTER(STAT_AddToBatch);
		for (int i = 0; i < Channels; i++)
		{
			float VerticalAng = FOV_VerticalMin + (FOV_VerticalMax - FOV_VerticalMin) / (float)Channels * (float)i;
			for (int j = 0; j < Step; j++)
			{
				float HorizontAng = FOV_HorizontMin + (FOV_HorizontMax - FOV_HorizontMin) / (float)Step * (float)j;

				FVector RayNorm = Rot.RotateVector(FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));

				BatchStart[i * Step + j] = Loc + RayNorm * DistanseMin;
				BatchEnd[i * Step + j] = Loc + RayNorm * DistanseMax;

			}
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
	check(OutHits.Num() == Channels * Step);

	Points.resize(Channels * Step);

	for (int k = 0; k < Channels * Step ; ++k)
	{
		bool Traced = OutHits[k].bBlockingHit;
		FVector HitPosition = PublishAbsoluteCoordinates ? Rot.UnrotateVector(OutHits[k].Location - Loc) : OutHits[k].Location;
		soda::LidarScanPoint& pt = Points[k];
		pt.coords.x = HitPosition.X / 100;
		pt.coords.y = -HitPosition.Y / 100;
		pt.coords.z = HitPosition.Z / 100;
		pt.layer = Channels - k / Step - 1;
		pt.properties = soda::LidarScanPoint::Properties::None;

		if (Traced)
		{
			pt.properties = soda::LidarScanPoint::Properties::Valid;

			if (EnabledGroundFilter)
			{
				if ((Loc.Z - HitPosition.Z) > DistanceToGround)
				{
					pt.properties = soda::LidarScanPoint::Properties::None;
					Traced = false;
				}
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PublishResults);
		PointCloudPublisher.Publish(Timestemp, Points);
	}

	if(DrawTracedRays || DrawNotTracedRays) 
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
				else if (DrawNotTracedRays && !Traced)
				{
					DrawDebugLine(GetWorld(), Loc + RayNorm * DistanseMin, Loc + RayNorm * DistanseMax, FColor(0, 255, 0), false, -1.f, 0, 2.f);
				}
			}
		}
	}
}

bool ULidarRayTraceSensorComponent::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateSimpleFOVMesh(
		FOV_HorizontMax - FOV_HorizontMin,
		FOV_VerticalMax - FOV_VerticalMin,
		20, 20, FOVSetup.MaxViewDistance,
		(FOV_HorizontMax + FOV_HorizontMin) / 2,
		(FOV_VerticalMin + FOV_VerticalMax) / 2,
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

bool ULidarRayTraceSensorComponent::NeedRenderSensorFOV() const
{
	return (FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && IsVehicleComponentSelected());
}


FBoxSphereBounds ULidarRayTraceSensorComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FMeshGenerationUtils::CalcFOVBounds(FOV_HorizontMax - FOV_HorizontMin, FOV_VerticalMax - FOV_VerticalMin, FOVSetup.MaxViewDistance)).TransformBy(LocalToWorld);;
}