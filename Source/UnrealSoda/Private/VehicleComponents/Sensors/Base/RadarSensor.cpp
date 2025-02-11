// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/RadarSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Physics/PhysicsFiltering.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Soda/SodaStatics.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "Soda/SodaCommonSettings.h"
#include "DynamicMeshBuilder.h"

#define _DEG2RAD(a) ((a) / (180.0 / M_PI))
#define _RAD2DEG(a) ((a) * (180.0 / M_PI))
#define CAN_FRAME_TIMEOUT_MS 200
#define RADAR_TOUCH_CNT 10

DEFINE_LOG_CATEGORY(SodaRadar);

DECLARE_STATS_GROUP(TEXT("RadarSensor"), STATGROUP_RadarSensor, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("RadarTickComponent"), STAT_RadarTickComponent, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("batch execute"), STAT_RadarBatchExecute, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Add to batch execute"), STAT_RadarAddToBatch, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Process query results"), STAT_RadarProcessQueryResults, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Publish results"), STAT_RadarPublishResults, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Add Cluster"), STAT_AddCluster, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Find Cluster"), STAT_FindCluster, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Create Cluster"), STAT_CreateCluster, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Add to existing Cluster"), STAT_AddToExistingCluster, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Radar Show Debug"), STAT_RadarShowDebug, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Add Object"), STAT_AddObject, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Add to existing Object"), STAT_AddToExistingObject, STATGROUP_RadarSensor);
DECLARE_CYCLE_STAT(TEXT("Create Object"), STAT_CreateObject, STATGROUP_RadarSensor);

URadarSensor::URadarSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Radar Sensors");
	GUI.IcanName = TEXT("SodaIcons.Radar");
	
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	ObjectCategories.Add(ESegmObjectLabel::Buildings, FRadarObjectCatagory(BuildingsRCS, true));
	ObjectCategories.Add(ESegmObjectLabel::Fences, FRadarObjectCatagory(FencesRCS, false));
	//ObjectCategories.Add(ESegmObjectLabel::Other,        FRadarObjectCatagory(1.0, true));
	ObjectCategories.Add(ESegmObjectLabel::Pedestrians, FRadarObjectCatagory(PedestriansRCS, false));
	ObjectCategories.Add(ESegmObjectLabel::Poles, FRadarObjectCatagory(PolesRCS, false));
	//ObjectCategories.Add(ESegmObjectLabel::RoadLines,    FRadarObjectCatagory(1.0, true));
	//ObjectCategories.Add(ESegmObjectLabel::Roads,        FRadarObjectCatagory(1.0, true));
	//ObjectCategories.Add(ESegmObjectLabel::Sidewalks,    FRadarObjectCatagory(1.0, true));
	ObjectCategories.Add(ESegmObjectLabel::TrafficSigns, FRadarObjectCatagory(TrafficSignsRCS, true));
	ObjectCategories.Add(ESegmObjectLabel::Vegetation,   FRadarObjectCatagory(VegetationRCS, false));
	ObjectCategories.Add(ESegmObjectLabel::Vehicles, FRadarObjectCatagory(VehiclesRCS, true));
	ObjectCategories.Add(ESegmObjectLabel::Walls, FRadarObjectCatagory(WallsRCS, true));

	/*
	FOVSetupNear.Color = FLinearColor(1.0, 1.0, 0.3, 0.5);
	FOVSetupNear.WireframeColor = FLinearColor(0.4, 0.4, 0, 1);
	FOVSetupNear.MaxViewDistance = RadarParamsNear.DistanseMax * 100;

	FOVSetupFar.Color = FLinearColor(1.0, 0.6, 0.3, 0.5);
	FOVSetupFar.WireframeColor = FLinearColor(0.4, 0.4, 0, 1);
	FOVSetupFar.MaxViewDistance = RadarParamsFar.DistanseMax * 100;
	*/
}

bool URadarSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PrevTickTime = SodaApp.GetSimulationTimestamp();


	MarkRenderStateDirty();

	return true;
}

void URadarSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	MarkRenderStateDirty();
}

void URadarSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	SCOPE_CYCLE_COUNTER(STAT_RadarTickComponent);

	TTimestamp Timestamp = SodaApp.GetSimulationTimestamp();

	auto StatusSendDeltaTimeMs = Timestamp - PrevTickTime;
	std::chrono::milliseconds Period_(int64(Period * 1000));
	if (StatusSendDeltaTimeMs < Period_)
		return;

	if (StatusSendDeltaTimeMs < Period_)
		PrevTickTime += Period_;
	else
		PrevTickTime = Timestamp;

	Clusters.Clear();
	Objects.ResetScan();

	for (auto& It : GetRadarParams())
	{
		if (It.bEnabled)
		{
			ProcessRadarBeams(It);
		}
	}

	switch (GetRadarMode())
	{
	case ERadarMode::ClusterMode:
		break;

	case ERadarMode::ObjectMode:
		Objects.FinishScan(*this);
		break;
	}

	PublishSensorData(DeltaTime, GetHeaderGameThread(), Clusters, Objects);

	if (bDrawDebugPrimitives)
	{
		ShowDebudPoints();
	}
}

void URadarSensor::ProcessHit(const FHitResult* Hit, const FRadarParams* Params)
{
	check(Params && Hit);

	if (Hit->Distance <= 0.f)
	{
		return; // TODO: WTF?
	}

	/*
	if (!Hit->IsValidBlockingHit())
	{
		return;
	}
	*/

	const FVector Loc = GetComponentLocation();
	const FRotator Rot = GetComponentRotation();

	FRadarHit RadarHit;
	RadarHit.Hit = Hit;
	RadarHit.SensorView = GetComponentTransform();
	RadarHit.HitPosition = Hit->ImpactPoint;
	RadarHit.LocalHitPosition = RadarHit.SensorView.InverseTransformPosition(RadarHit.HitPosition);
	RadarHit.LocalHitPosition.Z = 0;
	RadarHit.HitPosition.Z = RadarHit.SensorView.TransformPosition(RadarHit.LocalHitPosition).Z;

	if (!Hit->GetActor())
	{
		return;
	}
	const FRotator HitRot = (RadarHit.HitPosition - Loc).Rotation();
	const FRotator RelHitRot = HitRot - Rot;

	RadarHit.Distance = (RadarHit.HitPosition - Loc).Size();
	RadarHit.Azimuth = RelHitRot.Yaw;

	if (RadarHit.Distance > Params->DistanseMax * 100)
	{
		return;
	}

	/*
	if (fabs(FRotator::NormalizeAxis(RadarHit.Azimuth)) > Params->FOV_HorizontMax)
	{
		return;
	}
	*/

	TArray<UStaticMeshComponent *> StaticMeshComponents;
	RadarHit.Hit->GetActor()->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	RadarHit.ObjectCategory = ESegmObjectLabel::None;
	for (UStaticMeshComponent *Component : StaticMeshComponents)
	{
		RadarHit.ObjectCategory = USodaStatics::GetTagOfTaggedComponent(Component);

		if (RadarHit.ObjectCategory != ESegmObjectLabel::None)
		{
			break;
		}
	}
	if (RadarHit.ObjectCategory == ESegmObjectLabel::None)
	{
		// Iterate skeletal meshes.
		TArray< USkeletalMeshComponent* > SkeletalMeshComponents;
		RadarHit.Hit->GetActor()->GetComponents< USkeletalMeshComponent >(SkeletalMeshComponents);
		for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
		{
			RadarHit.ObjectCategory = USodaStatics::GetTagOfTaggedComponent(Component);
			if (RadarHit.ObjectCategory != ESegmObjectLabel::None)
			{
				break;
			}
		}
	}

	if (!ObjectCategories.Contains(RadarHit.ObjectCategory))
	{
		if (bDebugLog && RadarHit.ObjectCategory != ESegmObjectLabel::None)
		{
			UE_LOG(SodaRadar, Log, TEXT("Radar: !ObjectCategories.Contains(Label), Label = %d"), RadarHit.ObjectCategory);
		}
		return;
	}

	RadarHit.RCS = ObjectCategories[RadarHit.ObjectCategory].RCS;
	if (RadarHit.RCS < Params->MinSignalCoef)
	{
		//	UE_LOG(SodaRadar, Log, TEXT("Radar: Yaw = %f, RCS = %f"), HitRot.Yaw, RCS);
		return;
	}

	const FVector Vel = (RadarHit.Hit->GetActor()->GetVelocity() - GetVehicle()->GetVelocity()) * 0.01f; // TODO: compute velocity in the radar point
	RadarHit.VelRelToRadar = Rot.UnrotateVector(Vel);
	RadarHit.Resolution = Params->GetResolutionForAngle(RadarHit.Azimuth);

	switch (GetRadarMode())
	{
	case ERadarMode::ClusterMode:
		Clusters.AddHit(RadarHit, *this);
		break;

	case ERadarMode::ObjectMode:
		RadarHit.RCS *= RCSObjectModeMultiplier;
		Objects.AddHit(RadarHit, *this);
		break;
	}
}

void URadarSensor::ShowDebudPoints()
{
	
	FVector Loc = GetComponentLocation();
	switch (GetRadarMode())
	{
	case ERadarMode::ClusterMode:
	{
		SCOPE_CYCLE_COUNTER(STAT_RadarShowDebug);
		for (auto It = Clusters.Clusters.CreateConstIterator(); It; ++It)
		{
			if (bDrawDebugLinesToObjects)
			{
				DrawDebugLine(GetWorld(), Loc, It->HitPosition, FColor(255, 255, 0), false, -1.f, 0, 2.f);
			}
			if (bDrawDebugTracedPoints)
			{
				DrawDebugPoint(GetWorld(), It->HitPosition, 10, FColor::Yellow, false, -1.0f);
			}
			if (bDrawDebugLabels)
			{
				DrawDebugString(GetWorld(), It->HitPosition, *FString::Printf(TEXT("RCS=%f, hits=%d, Dist=%f"), It->RCS, It->Hits.Num(), It->Distance), NULL, FColor::Yellow, 0.01f, false);

			}
		}

		break;
	}
	case ERadarMode::ObjectMode:
	{
		SCOPE_CYCLE_COUNTER(STAT_RadarShowDebug);
		for (auto It = Objects.Objects.CreateConstIterator(); It; ++It)
		{
			auto& Object = It.Value();
			FVector TracedPoint = GetComponentTransform().TransformPosition(It->Value.GetObjectPoint());
			FColor Color = It->Value.bUpdated ? FColor::Green : FColor::Red;

			if (bDrawDebugLinesToObjects)
			{
				DrawDebugLine(GetWorld(), Loc, TracedPoint, Color, false, -1, 0, 2.f);
			}

			if (bDrawDebugTracedPoints)
			{
				DrawDebugPoint(GetWorld(), TracedPoint, 10, Color);
			}

			if (bDrawDebugLabels)
			{
				DrawDebugString(GetWorld(), TracedPoint, *FString::Printf(TEXT("Id=%d, RCS=%f, Dist=%f, RelLonVel=%f, RelLatVel=%f"),
					Object.RadarObjectId, Object.RCS, Object.GetDistance(), Object.Lon, Object.Lat), NULL, Color, 0);
			}

			FVector Center, Extents;
			Object.LocalBounds.GetCenterAndExtents(Center, Extents);
			DrawDebugBox(GetWorld(), this->GetComponentTransform().TransformPosition(Center), Extents, GetComponentRotation().Quaternion(), Color);

		}
		break;
	}
	}

}

void URadarSensor::ProcessRadarBeams(const FRadarParams & RadarParams)
{
	SCOPE_CYCLE_COUNTER(STAT_RadarAddToBatch);

	static FQuat ZeroQaud = FQuat::MakeFromRotator(FRotator::ZeroRotator);

	BatchStart.SetNum(0, false);
	BatchEnd.SetNum(0, false);

	const FVector Loc = GetComponentLocation();
	const FRotator Rot = GetComponentRotation();
	const FTransform RelTrans = GetRelativeTransform();
	const float BeamVerticalSize = RadarParams.GetBeamHeight();
	const float BeamRadius = RadarParams.GetBeamWidth() / 2;
	const float Step = 2 * RadarParams.FOV_HorizontMax / (float)RadarParams.GetBeamsNum();
	const FCollisionShape Collider = FCollisionShape::MakeCapsule(BeamRadius, BeamVerticalSize * 0.5f);
	const float DistanseMin = FMath::Max(0.f, RadarParams.DistanseMin * 100 - BeamRadius);

	for (int i = 0; i < RadarParams.GetBeamsNum(); i++)
	{
		const float HorizontAng = -1.f * RadarParams.FOV_HorizontMax + Step * i;
		const FVector Norm = Rot.RotateVector(FRotator(0.0f, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));
		const FQuat Q = RelTrans.GetRotation() * FQuat(FRotator(90.f, 0.f, 0.f));
		const float Dist = RadarParams.GetRayLength(HorizontAng) * 100;

		const FVector Start = Loc + Norm * DistanseMin;
		const FVector End = Loc + Norm * (DistanseMin + Dist);

		BatchStart.Add(Start);
		BatchEnd.Add(End);
		
		if (RadarParams.bShowDebugCapsules)
		{
			DrawDebugCapsule(GetWorld(),
				Start + Norm * Dist * RadarParams.DebugCapsulesRang,
				BeamVerticalSize * 0.5f,
				BeamRadius,
				RelTrans.GetRotation(),
				FColor::Cyan);
		}
	}

	UWorld* World = GetWorld();
	TArray<TArray<struct FHitResult>> OutHits;
	bool Res = FSodaPhysicsInterface::GeomSweepMultiScope(World, Collider, ZeroQaud, OutHits, BatchStart, BatchEnd,
		GetDefault<USodaCommonSettings>()->RadarCollisionChannel,
		FCollisionQueryParams(NAME_None, true, GetOwner()), 
		FCollisionResponseParams::DefaultResponseParam,
		FCollisionObjectQueryParams::DefaultObjectQueryParam);

	for (auto& Hits : OutHits)
	{
		for (auto& Hit : Hits)
		{
			ProcessHit(&Hit, &RadarParams);
		}
	}
	

	if (bDrawDebugPrimitives)
	{
		for (auto& Hits : OutHits)
		{
			for (auto& Hit : Hits)
			{
				if (bDrawDebugTracedPoints)
				{
					DrawDebugPoint(World, Hit.ImpactPoint, 10, FColor::Red, false, -1.0f);
				}
			}
		}
	}
}

void FRadarClusters::AddHit(const FRadarHit& RadarHit, const URadarSensor& Radar)
{
	SCOPE_CYCLE_COUNTER(STAT_AddCluster);

	// UE_LOG(SodaRadar, Log, TEXT("Radar: Create Cluster Azimuth = %f, Distance = %f"), Azimuth, NewHit->distance);
	SCOPE_CYCLE_COUNTER(STAT_CreateCluster);
	FRadarCluster & Cluster = Clusters.Add_GetRef(FRadarCluster());
	Cluster.Hits.Add(RadarHit.Hit);
	Cluster.Azimuth = RadarHit.Azimuth;
	Cluster.Distance = RadarHit.Distance * 0.01f;
	Cluster.HitPosition = RadarHit.HitPosition;
	Cluster.LocalHitPosition = RadarHit.LocalHitPosition;
	Cluster.RCS = RadarHit.RCS;
	Cluster.Lat = RadarHit.VelRelToRadar.Y;
	Cluster.Lon = RadarHit.VelRelToRadar.X;
}

void FRadarObjects::ResetScan()
{
	for (auto & It : Objects)
	{
		It.Value.bUpdated = false;
	}
}

void FRadarObjects::FinishScan(const URadarSensor& Radar)
{
	TTimestamp Timestamp = SodaApp.GetSimulationTimestamp();

	for (auto It = Objects.CreateIterator(); It; ++It)
	{
		FRadarObject& Object = It.Value();

		FVector2D LocalObjectVelocity = FVector2D(Object.Lon, Object.Lat);
		Object.TrackingData.LifeCyclesCounter++;
		Object.TrackingData.Acceleration = (LocalObjectVelocity - Object.TrackingData.PrevVelocity) /
			std::chrono::duration_cast<std::chrono::duration<double>>(Timestamp - Object.TrackingData.PrevUpdatedTimestamp).count();
		Object.TrackingData.PrevVelocity = LocalObjectVelocity;
		Object.TrackingData.PrevUpdatedTimestamp = Timestamp;
		if (Object.bUpdated)
		{
			const FBox Bounds = Object.ObjectActor->GetComponentsBoundingBox(false).InverseTransformBy(Radar.GetComponentTransform());
			Object.LocalBounds.Min.Z = Bounds.Min.Z;
			Object.LocalBounds.Max.Z = Bounds.Max.Z;
			const FVector BoundsSize = Object.LocalBounds.GetSize();
			const static float MinBoxSize = 10;
			if (BoundsSize.X < MinBoxSize) Object.LocalBounds.Max.X += (MinBoxSize - BoundsSize.X);
			if (BoundsSize.Y < MinBoxSize) { Object.LocalBounds.Min.Y -= MinBoxSize / 2; Object.LocalBounds.Max.Y += MinBoxSize / 2; }
			if (BoundsSize.Z < MinBoxSize) { Object.LocalBounds.Min.Z -= MinBoxSize / 2; Object.LocalBounds.Max.Z += MinBoxSize / 2; }
			auto PointGlobal = Radar.GetComponentTransform().TransformPosition(Object.LocalBounds.GetCenter());
			Object.BoundCenterActorLocal = Object.ObjectActor->GetActorTransform().InverseTransformPosition(PointGlobal);
			Object.TrackingData.ProbabilityCounter = FMath::Min(Object.TrackingData.ProbabilityCounter + 1, FTrackingObjectData::kProbabilityCounterMax);
		}
		else
		{
			--Object.TrackingData.ProbabilityCounter;
			if (Object.TrackingData.ProbabilityCounter < 0 || !IsValid(Object.ObjectActor))
			{
				It.RemoveCurrent();
				continue;
			}
			else
			{
				auto PointGlobal = Object.ObjectActor->GetActorTransform().TransformPosition(Object.BoundCenterActorLocal);
				auto PointLocal = Radar.GetComponentTransform().InverseTransformPosition(PointGlobal);
				Object.LocalBounds = Object.LocalBounds.MoveTo(PointLocal);
			}
		}

		Object.CachedObjectPoint = FVector(Object.LocalBounds.Min.X, (Object.LocalBounds.Min.Y + Object.LocalBounds.Max.Y) * 0.5f, 0);
		Object.CachedDistance = Object.CachedObjectPoint.Size();
		Object.CachedAzimuth = std::atan2(Object.CachedObjectPoint.Y, Object.CachedObjectPoint.X) / M_PI * 180;
	}
}

void FRadarObjects::AddHit(const FRadarHit& RadarHit, const URadarSensor& Radar)
{

	SCOPE_CYCLE_COUNTER(STAT_AddObject);

	int ActorId = RadarHit.Hit->GetActor()->GetUniqueID();
	AActor* Actor = RadarHit.Hit->GetActor();
	check(Actor);

	FRadarObject* ObjectToAddHit = Objects.Find(ActorId);
	if (ObjectToAddHit == nullptr)
	{
		if (Objects.Num() >= ObjectsMaxNum)
		{
			return;
		}

		FRadarObject& NewObj = Objects.Add(ActorId);
		NewObj.bUpdated = true;
		NewObj.Hits.Add(RadarHit.Hit);
		NewObj.RCS = RadarHit.RCS;
		NewObj.Lat = RadarHit.VelRelToRadar.Y;
		NewObj.Lon = RadarHit.VelRelToRadar.X;
		NewObj.ObjectActorId = ActorId;
		NewObj.ObjectActor = Actor;
		NewObj.ObjectCategory = RadarHit.ObjectCategory;
		NewObj.LocalBounds += Radar.GetComponentTransform().InverseTransformPosition(RadarHit.HitPosition);
		NewObj.TrackingData.RadarObjectId = GetRadarObjectIdForActorId(ActorId);
		NewObj.TrackingData.ProbabilityCounter = 0;
		NewObj.TrackingData.LifeCyclesCounter = 0;
		NewObj.TrackingData.Acceleration = FVector2D(0.f);
		NewObj.TrackingData.PrevVelocity = FVector2D(NewObj.Lon, NewObj.Lat);
		NewObj.TrackingData.PrevUpdatedTimestamp = SodaApp.GetSimulationTimestamp();
	}
	else
	{
		if (ObjectToAddHit->bUpdated)
		{
			ObjectToAddHit->LocalBounds += Radar.GetComponentTransform().InverseTransformPosition(RadarHit.HitPosition);
			ObjectToAddHit->RCS += RadarHit.RCS;
		}
		else
		{
			ObjectToAddHit->bUpdated = true;
			ObjectToAddHit->LocalBounds.Init();
			ObjectToAddHit->LocalBounds += Radar.GetComponentTransform().InverseTransformPosition(RadarHit.HitPosition);
			ObjectToAddHit->RCS = RadarHit.RCS;
			ObjectToAddHit->Hits.Empty();
		}

		ObjectToAddHit->Lat = RadarHit.VelRelToRadar.Y;
		ObjectToAddHit->Lon = RadarHit.VelRelToRadar.X;
		ObjectToAddHit->Hits.Add(RadarHit.Hit);
	}

}

uint8 FRadarObjects::GetFreeRadarObjectId()
{
	if (Objects.Num() == 0 || Objects.Num() >= 256)
	{
		return 0;
	}

	uint8 MaxVal = 0;
	for (auto& Elem : Objects)
	{
		if (Elem.Value.RadarObjectId > MaxVal)
		{
			MaxVal = Elem.Value.RadarObjectId;
		}
	}

	for (uint8 NewId = MaxVal + 1; NewId != MaxVal; ++NewId)
	{
		bool IdExists = false;
		for (auto& CheckElem : Objects)
		{
			if (CheckElem.Value.RadarObjectId == NewId)
			{
				IdExists = true;
				break;
			}
		}
		if (IdExists == false)
		{
			return NewId;
		}
	}
	return 0;
}

uint8 FRadarObjects::GetRadarObjectIdForActorId(uint32 ActorId)
{
	if (Objects.Contains(ActorId))
	{
		return Objects[ActorId].RadarObjectId;
	}
	return GetFreeRadarObjectId();
}

void FRadarObjects::SortObjectsByDistance(TArray<const FRadarObject* >& SortedObjects) const
{
	SortedObjects.SetNum(Objects.Num());
	int i = 0;
	for (auto& Obj : Objects)
	{
		SortedObjects[i] = &Obj.Value;
		++i;
	}
	SortedObjects.Sort([](const FRadarObject& Left, const FRadarObject& Right) { return Left.GetDistance() > Right.GetDistance(); });
}

float FRadarParams::GetBeamWidth() const
{
	return 2 * DistanseMax * 100 * atan(_DEG2RAD(HorizontalBestResolution / 2));
}

float FRadarParams::GetBeamHeight() const
{
	return 2 * DistanseMax * 100 * atan(_DEG2RAD(FOV_Vertical / 2));
}

float FRadarParams::GetResolutionForAngle(float Angle) const
{
	Angle = fabs(Angle);
	if (Angle < FOV_HorizontFullDist)
	{
		return FMath::Lerp(HorizontalBestResolution, HorizontalResolutionFullDistAngle, Angle / FOV_HorizontFullDist);
	}
	else
	{
		return FMath::Lerp(HorizontalResolutionFullDistAngle, HorizontalResolutionMaxAngle, (Angle - FOV_HorizontFullDist) / (FOV_HorizontMax - FOV_HorizontFullDist));
	}
}

float FRadarParams::GetRayLength(float HorizontAng) const
{
	float Dist = DistanseMax;
	if (fabs(HorizontAng) > FOV_HorizontFullDist)
	{
		Dist = FMath::Lerp(Dist, DistanseOnMaxAngle, (fabs(HorizontAng) - FOV_HorizontFullDist) / (FOV_HorizontMax - FOV_HorizontFullDist));
	}
	return Dist - DistanseMin;
}

bool URadarSensor::AddFOVMesh(const FRadarParams& Params, TArray<FSensorFOVMesh>& OutMeshes)
{
	static const float DegPerCell = 5.0;

	const int FovChannels = int((Params.FOV_Vertical) / DegPerCell + 0.5);
	const int FovStep = int(Params.FOV_HorizontMax / DegPerCell + 0.5);

	TArray<FVector> Cloud;
	TArray<FVector2D> CloudUV;

	for (int i = 0; i < FovChannels + 1; i++)
	{
		float VerticalAng = -Params.FOV_Vertical / 2 + Params.FOV_Vertical / (float)FovChannels * (float)i;
		for (int j = 0; j < FovStep + 1; j++)
		{
			float HorizontAng = -Params.FOV_HorizontMax / 2 + Params.FOV_HorizontMax / (float)FovStep * (float)j;
			FVector RayNorm = FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0));

			//TODO: Implement FOV_HorizontMax& FOV_HorizontFullDist view

			/*
			float Distance;
			if (FMath::Abs(HorizontAng) > Params.FOV_HorizontFullDist / 2)
			{
				Distance = FMath::Lerp(
					Params.DistanseMax,
					Params.DistanseOnMaxAngle,
					FMath::Abs((Params.FOV_HorizontFullDist / 2.0 - FMath::Abs(HorizontAng)) / (Params.FOV_HorizontMax / 2.0 - Params.FOV_HorizontFullDist / 2.0))) * 100;
			}
			else
			{
				Distance = Params.DistanseMax * 100;
			}

			Cloud.Add(RayNorm * Distance);
			*/


			Cloud.Add(RayNorm * Params.FOVSetup.MaxViewDistance);
			CloudUV.Add(FVector2D(j, FovChannels - i));
		}
	}

	TArray <int32> Hull;
	FMeshGenerationUtils::GenerateHullInexes(FovStep + 1, FovChannels + 1, Hull);

	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateFOVMesh2D(Cloud, CloudUV, Hull, MeshData.Vertices, MeshData.Indices))
	{
		for (auto& it : MeshData.Vertices) it.Color = Params.FOVSetup.Color.ToFColor(true);
		OutMeshes.Add(MoveTempIfPossible(MeshData));
		return true;
	}
	else
	{
		return false;
	}


}

bool URadarSensor::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{
	Meshes.Empty();
	bool bRet = false;
	const TArray<FRadarParams>& RadarParams = GetRadarParams();
	for (auto & It : RadarParams)
	{
		if (It.bEnabled)
		{
			bRet = AddFOVMesh(It, Meshes) || bRet;
		}
	}
	return bRet;
}

bool URadarSensor::NeedRenderSensorFOV() const
{
	const TArray<FRadarParams>& RadarParams = GetRadarParams();
	const bool bIsSelected = IsVehicleComponentSelected();
	for (auto& It : RadarParams)
	{
		if ((It.FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
			(It.FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && bIsSelected) ||
			(It.FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelectWhenActive && bIsSelected && IsVehicleComponentActiveted()) ||
			(It.FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::EverWhenActive && IsVehicleComponentActiveted()))
		{
			return true;
		}
	}
	return false;
}

FBoxSphereBounds URadarSensor::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox Box;
	const TArray<FRadarParams>& RadarParams = GetRadarParams();
	for (auto& It : RadarParams)
	{
		Box += FMeshGenerationUtils::CalcFOVBounds(It.FOV_HorizontMax, It.FOV_Vertical, It.FOVSetup.MaxViewDistance);
	}
	return FBoxSphereBounds(Box).TransformBy(LocalToWorld);
}
