// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/LidarSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Physics/PhysicsFiltering.h"
#include "DrawDebugHelpers.h"
#include "UObject/UObjectIterator.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"
#include "Async/Async.h"


ULidarSensor::ULidarSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("LiDAR Sensors");
	GUI.IcanName = TEXT("SodaIcons.Lidar");
	GUI.bIsPresentInAddMenu = false;

	FOVSetup.Color = FLinearColor(1.0, 0.1, 0.3, 1.0);
	FOVSetup.MaxViewDistance = 700;
}

void ULidarSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	if (!IsTickOnCurrentFrame()) return;

}

bool ULidarSensor::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateSimpleFOVMesh(
		GetFOVHorizontMax() - GetFOVHorizontMin(),
		GetFOVVerticalMax() - GetFOVVerticalMin(),
		20, 20, FOVSetup.MaxViewDistance,
		(GetFOVHorizontMax() + GetFOVHorizontMin()) / 2,
		(GetFOVVerticalMin() + GetFOVVerticalMax()) / 2,
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

void ULidarSensor::DrawLidarPoints(const soda::FLidarSensorData& Scan, bool bDrawInGameThread)
{
	static auto DrawLidarPointsImpl = [](UWorld * World, const FVector & Loc, const FRotator& Rot, const soda::FLidarSensorData& Scan)
	{
		for (auto& Pt : Scan.Points)
		{
			FVector WorldPoint = Rot.RotateVector(Pt.Location) + Loc;
			DrawDebugPoint(World, WorldPoint, 5, Pt.Status == soda::ELidarPointStatus::Valid ? FColor::Green : FColor::Red, false, -1.0f); 
		}
	};

	if (bDrawLidarPoints)
	{
		if(bDrawInGameThread)
		{
			AsyncTask(ENamedThreads::GameThread, [this, Loc=GetComponentLocation(), Rot=GetComponentRotation(), Scan=Scan]()
			{
				DrawLidarPointsImpl(GetWorld(), Loc, Rot, Scan);
			});	
		}
		else
		{
			DrawLidarPointsImpl(GetWorld(), GetComponentLocation(), GetComponentRotation(), Scan);
		}
	}
}

bool ULidarSensor::NeedRenderSensorFOV() const
{
	return (FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && IsVehicleComponentSelected());
}


FBoxSphereBounds ULidarSensor::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FMeshGenerationUtils::CalcFOVBounds(GetFOVHorizontMax() - GetFOVHorizontMin(), GetFOVVerticalMax() - GetFOVVerticalMin(), FOVSetup.MaxViewDistance)).TransformBy(LocalToWorld);;
}