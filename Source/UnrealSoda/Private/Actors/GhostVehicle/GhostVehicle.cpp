// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/GhostVehicle/GhostVehicle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soda/Actors/NavigationRoute.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaStatics.h"
#include "Actors/GhostVehicle/SpeedProfile/UnifiedSpeedProfile.h"
#include "Actors/GhostVehicle/SpeedProfile/Curvature.h"
#include "Soda/DBGateway.h"
#include "Soda/SodaApp.h"
#include "Math/UnrealMathUtility.h"
#include "VehicleUtility.h"
#include "Templates/Tuple.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"


AGhostVehicle::AGhostVehicle()
{
	Mesh = CreateDefaultSubobject< USkeletalMeshComponent >(TEXT("VehicleMesh"));
	
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->SetSimulatePhysics(false);
	/*
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
	*/

	RootComponent = Mesh;
	PrimaryActorTick.bCanEverTick = true;
	USodaStatics::TagActor(this, true, EActorTagMethod::ForceCustomLabel, ESegmObjectLabel::Vehicles);
}

void AGhostVehicle::BeginPlay()
{
	Super::BeginPlay();

	InitTransform = GetActorTransform();

	TrajectoryPlaner.Init(GetWorld());

	BaseLength = 0;
	if (Mesh)
	{
		FBox Box;
		for (auto& Wheel : Wheels)
		{
			int Ind = Mesh->GetBoneIndex(Wheel.BoneName);
			if (Ind != INDEX_NONE)
			{
				Wheel.Location = Mesh->GetBoneLocation(Wheel.BoneName, EBoneSpaces::ComponentSpace);
				//Wheel.Rotation = Mesh->GetBoneQuaternion(It->BoneName, EBoneSpaces::ComponentSpace).Rotator();
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("AGhostVehicle::BeginPlay(); Can't find vehicle bone name: %s"), *Wheel.BoneName.ToString());
			}

			Box += Wheel.Location;
		}

		if (Box.IsValid)
		{
			BaseLength = (Box.Max - Box.Min).X;
		}
	}

	RealAxesOffset = {};
	VehicleRWheel = 0;

	if (Wheels.Num())
	{
		double MinX = Wheels[0].Location.X;
		for (auto& it : Wheels)
		{
			VehicleRWheel += it.Radius;
			if (it.Location.X < MinX) MinX = it.Location.X;
		}
		VehicleRWheel /= Wheels.Num();
		RealAxesOffset = FVector(MinX, 0, 0);
	}
}

void AGhostVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	TrajectoryPlaner.Deinit();
}

FVector AGhostVehicle::GetVelocity() const
{
	if (bIsMoving && TrajectoryPlaner.GetWayPointsNum())
	{
		return GetActorTransform().TransformVectorNoScale(FVector(CurrentVelocity, 0, 0));
	}
	else
	{
		return FVector::ZeroVector;
	}
}

void AGhostVehicle::InputWidgetDelta(const USceneComponent* WidgetTargetComponent, FTransform& NewWidgetTransform)
{
	if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
	{
		if (bJoinToInitialRoute)
		{
			const float Key = RoutePlanner->Spline->FindInputKeyClosestToWorldLocation(NewWidgetTransform.GetTranslation());
			NewWidgetTransform = RoutePlanner->Spline->GetTransformAtSplineInputKey(Key, ESplineCoordinateSpace::World);
			InitialRouteOffset = RoutePlanner->Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		}
		else
		{
			UpdateJoinCurve();
		}
	}
}

void AGhostVehicle::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	check(PlayerController);

	USodaSubsystem * SodaSubsystem  = USodaSubsystem::GetChecked();

	if (bIsMoving)
	{
		if (TrajectoryPlaner.UpdateTarjectory(GetWorld(), GetTransform(), 10000, AllowedRouteTags))
		{
			CalculateSpeedProfile();
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
		}

		if (CurrentSplineOffset > TrajectoryPlaner.GetRouteSpline()->GetSplineLength())
		{
			CurrentVelocity = 0;
			CurrentAcc = 0;
		}

		if (TrajectoryPlaner.GetWayPointsNum())
		{

			const int WayPointInd = FMath::Clamp<int>(TrajectoryPlaner.GetCurrentSplineKey(), 0, TrajectoryPlaner.GetWayPointsNum() - 1);
			check(WayPointInd < Velocities.size() && WayPointInd < Accelerations.size());
			const float Vel = Velocities[(WayPointInd == Velocities.size() - 1) ? WayPointInd : WayPointInd + 1] * 100;

			CurrentAcc = Accelerations[WayPointInd] * 100;
			CurrentSplineOffset += DeltaTime * CurrentVelocity;
			CurrentVelocity = FMath::Clamp(CurrentVelocity + CurrentAcc * DeltaTime, 0.f, Vel);

			if (BaseLength > 0)
			{
				const double SteerAngle = (TrajectoryPlaner.GetCurvatureAtKey(TrajectoryPlaner.GetCurrentSplineKey()) * BaseLength / 100.0) / M_PI * 180;
				for (auto& Wheel : Wheels)
				{
					if (Wheel.bApplySteer)
					{
						Wheel.SteerAngle = FMath::Clamp(SteerAngle, Wheel.SteerAngle - SteeringAnimMaxSpeed * DeltaTime, Wheel.SteerAngle + SteeringAnimMaxSpeed * DeltaTime);
					}
				}
			}

			TrajectoryPlaner.SetSplineOffset(CurrentSplineOffset);

			FTransform VehicleTransform = TrajectoryPlaner.GetRouteSpline()->GetTransformAtSplineInputKey(TrajectoryPlaner.GetCurrentSplineKey(), ESplineCoordinateSpace::World);

			struct FIndVec
			{
				int Ind;
				FVector Vec;
			};
			TArray<FIndVec> Points;
			Points.Reserve(Wheels.Num());
			for (int i = 0; i < Wheels.Num(); ++i)
			{
				auto& Wheel = Wheels[i];
				GetWorld()->LineTraceSingleByChannel(Wheel.Hit, VehicleTransform.TransformPosition(Wheel.Location + FVector(0, 0, Wheel.Radius)), VehicleTransform.TransformPosition(Wheel.Location + FVector(0, 0, -Wheel.Radius - RayCastDepth)),
					ECollisionChannel::ECC_WorldDynamic,
					FCollisionQueryParams(NAME_None, false, this));

				if (Wheel.Hit.bBlockingHit)
				{
					Points.Add({ i, Wheel.Hit.Location });
				}

				Wheel.RotationAngularVelocit = -CurrentVelocity  / Wheel.Radius;
				Wheel.RotationAngle = NormAngDeg( Wheel.RotationAngle + Wheel.RotationAngularVelocit * DeltaTime / M_PI * 180);
			}

			if (Points.Num() >= 3)
			{
				Points.Sort([](const auto& Pt1, const auto& Pt2) { return Pt1.Vec.Z > Pt2.Vec.Z; });
				FPlane PlaneRoad(Points[0].Vec, Points[1].Vec, Points[2].Vec);
				if (PlaneRoad.GetNormal().Z < 0) PlaneRoad = PlaneRoad.Flip();

				FPlane PlaneVehicle(
					VehicleTransform.TransformPosition(Wheels[Points[0].Ind].Location + FVector(0, 0, -Wheels[Points[0].Ind].Radius)),
					VehicleTransform.TransformPosition(Wheels[Points[1].Ind].Location + FVector(0, 0, -Wheels[Points[1].Ind].Radius)),
					VehicleTransform.TransformPosition(Wheels[Points[2].Ind].Location + FVector(0, 0, -Wheels[Points[2].Ind].Radius))
				);
				if (PlaneVehicle.GetNormal().Z < 0) PlaneVehicle = PlaneVehicle.Flip();

				FQuat dQuat = FQuat::FindBetweenNormals(PlaneVehicle.GetNormal(), PlaneRoad.GetNormal());
				VehicleTransform.SetRotation(dQuat * VehicleTransform.GetRotation());

				const double Distance = (
					PlaneRoad.PlaneDot(VehicleTransform.TransformPosition(Wheels[Points[0].Ind].Location + FVector(0, 0, -Wheels[Points[0].Ind].Radius))) +
					PlaneRoad.PlaneDot(VehicleTransform.TransformPosition(Wheels[Points[1].Ind].Location + FVector(0, 0, -Wheels[Points[1].Ind].Radius))) +
					PlaneRoad.PlaneDot(VehicleTransform.TransformPosition(Wheels[Points[2].Ind].Location + FVector(0, 0, -Wheels[Points[2].Ind].Radius)))) / 3;
				VehicleTransform.SetTranslation(VehicleTransform.GetTranslation() - VehicleTransform.InverseTransformVector(PlaneRoad.GetNormal() * Distance));
				
			}

			VehicleTransform.SetLocation(
				VehicleTransform.GetLocation() - 
				VehicleTransform.GetRotation().RotateVector(VehicleTransform.InverseTransformPosition(VehicleTransform.GetLocation()) + RealAxesOffset)
			);
			SetActorTransform(VehicleTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		if (Dataset)
		{
			Dataset->BeginRow();
			OnPushDataset();
			Dataset->EndRow();
		}

	}
	else
	{
		if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
		{
			if (RoutePlanner->Spline->GetSplineGuid() != SplineGuid)
			{
				if (bJoinToInitialRoute)
				{
					FTransform VehicleTransform = RoutePlanner->Spline->GetTransformAtDistanceAlongSpline(InitialRouteOffset, ESplineCoordinateSpace::World);
					VehicleTransform.SetLocation(
						VehicleTransform.GetLocation() -
						VehicleTransform.GetRotation().RotateVector(VehicleTransform.InverseTransformPosition(VehicleTransform.GetLocation()) + RealAxesOffset)
					);
					SetActorTransform(VehicleTransform, false, nullptr, ETeleportType::TeleportPhysics);
				}
				else
				{
					UpdateJoinCurve();
				}
				SplineGuid = RoutePlanner->Spline->GetSplineGuid();
			}
		}
	}

	if (!SodaSubsystem->IsScenarioRunning() && !bJoinToInitialRoute)
	{
		for (int i = 1; i < JoiningCurvePoints.Num(); ++i)
		{
			const FVector & P0 = JoiningCurvePoints[i - 1];
			const FVector & P1 = JoiningCurvePoints[i];
			DrawDebugLine(GetWorld(), P0, P1, FColor(180, 180, 255), false, -1.f, 0, 2);
		}
	}

	if (bDrawDebugRoute)
	{
		TrajectoryPlaner.DrawRoute(this);
	}

}


void AGhostVehicle::GoToLeftRoute(float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving)
	{
		if (TrajectoryPlaner.GoToLeftRoute(GetWorld(), GetRealAxesTransform(), StaticOffset + CurrentVelocity * VelocityToOffset))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
			CalculateSpeedProfile();
		}
	}
}

void AGhostVehicle::GoToRightRoute(float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving)
	{
		if (TrajectoryPlaner.GoToRightRoute(GetWorld(), GetRealAxesTransform(), StaticOffset + CurrentVelocity * VelocityToOffset))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
			CalculateSpeedProfile();
		}
	}
}

void AGhostVehicle::GoToRoute(TSoftObjectPtr<ANavigationRoute> Route, float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving && Route.Get())
	{
		if (TrajectoryPlaner.GoToRoute(GetWorld(), GetRealAxesTransform(), StaticOffset + CurrentVelocity * VelocityToOffset, Route.Get()))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
			CalculateSpeedProfile();
		}
	}
}

void AGhostVehicle::StartMoving()
{
	if (!bIsMoving)
	{
		CurrentVelocity = Chaos::KmHToCmS(InitVelocity);
		CurrentSplineOffset = 0;
		TrajectoryPlaner.Reset();
		InitTransform = GetActorTransform();
		bIsMoving = true;


		if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
		{
			if (!bJoinToInitialRoute)
			{
				UpdateJoinCurve();
				auto Poits = JoiningCurvePoints;
				Poits.Pop(false);
				TrajectoryPlaner.AddRouteByWaypoints(Poits, RoutePlanner, false, false, false);
			}
			TrajectoryPlaner.AddRouteBySpline(RoutePlanner->Spline, InitialRouteOffset, RoutePlanner, false, false, true);
		}
		CalculateSpeedProfile();

		ReceiveStartMoving();
	}
}

void AGhostVehicle::StopMoving()
{
	if (bIsMoving)
	{
		SetActorTransform(InitTransform, false, nullptr, ETeleportType::TeleportPhysics);
		bIsMoving = false;
		CurrentVelocity = 0;
		CurrentAcc = 0;
		CurrentSplineOffset = 0;
		TrajectoryPlaner.Reset();
		CalculateSpeedProfile();

		ReceiveStopMoving();
	}
}

void AGhostVehicle::CalculateSpeedProfile()
{
	if (TrajectoryPlaner.GetWayPointsNum() == 0)
	{
		Velocities.clear();
		Accelerations.clear();
		return;
	}

	std::vector<SpeedProfile::FTrajectoryPoint> Path(TrajectoryPlaner.GetWayPointsNum());
	for (int i = 0; i < TrajectoryPlaner.GetWayPointsNum(); ++i)
	{
		Path[i].point = Eigen::Vector2d(TrajectoryPlaner.GetWayPoints()[i].Location.X / 100, TrajectoryPlaner.GetWayPoints()[i].Location.Y / 100);
	}
	for (int i = 0; i < TrajectoryPlaner.GetWayPointsNum() - 1; ++i)
	{
		Eigen::Vector2d Dir = Path[i + 1].point - Path[i].point;
		Path[i].yaw = std::atan2(Dir.y(), Dir.x());
	}
	Path[TrajectoryPlaner.GetWayPointsNum() - 1].yaw = Path[TrajectoryPlaner.GetWayPointsNum() - 2].yaw;
	for (int i = 1; i < TrajectoryPlaner.GetWayPointsNum() - 1; ++i)
	{
		Path[i].curvature = SpeedProfile::calculateCurvatureInfo(Path[i - 1].point, Path[i].point, Path[i + 1].point).curve;
	}
	Path[0].curvature = Path[1].curvature;
	Path[TrajectoryPlaner.GetWayPointsNum() - 1].curvature = Path[TrajectoryPlaner.GetWayPointsNum() - 2].curvature;

	SpeedProfile::FVehicleDynamicParams Dynamics;
	Dynamics.max_velocity = Chaos::KmHToCmS(VehicleMaxVelocity) / 100.0;
	Dynamics.car_mass = VehicleMass;
	Dynamics.max_power = VehicleMaxPower;
	Dynamics.max_torque = VehicleMaxTorque;
	Dynamics.r_wheel = VehicleRWheel / 100.0;
	SpeedProfile = MakeShareable(new SpeedProfile::FSpeedProfile(Dynamics));

	SpeedProfile::FPointLimits MainLimit;
	MainLimit.maxVelocity = Chaos::KmHToCmS(LimitMaxVelocity) / 100.0;
	MainLimit.mu = LimitMu;
	MainLimit.maxAcceleration = LimitMaxAcceleration;
	MainLimit.maxDeceleration = LimitMaxDeceleration;

	auto MainLimits = SpeedProfile::FTrajectoryLimits(Path.size(), MainLimit);
	auto VelocityProfile = SpeedProfile->calculateVelocities(CurrentVelocity / 100, Path, MainLimits, bIsAggressive);

	Velocities = VelocityProfile.velocities;
	Accelerations = SpeedProfile->calculateAccelerations(Path, VelocityProfile.velocities);
}

bool AGhostVehicle::IsLinkedToRoute() const
{
	return !TrajectoryPlaner.IsEnded();
}

void AGhostVehicle::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();

	if (bRecordDataset && soda::FDBGateway::Instance().GetStatus() == soda::EDBGatewayStatus::Connected && soda::FDBGateway::Instance().IsDatasetRecording())
	{
		soda::FBsonDocument Doc;
		GenerateDatasetDescription(Doc);
		Dataset = soda::FDBGateway::Instance().CreateActorDataset(GetName(), "vehicle", GetClass()->GetName(), *Doc);
	}

	StartMoving();
}

void AGhostVehicle::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();
	StopMoving();

	if (Dataset)
	{
		Dataset->PushAsync();
		Dataset.Reset();
	}
}

void AGhostVehicle::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	IEditableObject::RuntimePostEditChangeProperty(PropertyChangedEvent);
}

void AGhostVehicle::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) 
{
	IEditableObject::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
	SplineGuid = FGuid::NewGuid();
}

void AGhostVehicle::UpdateJoinCurve()
{
	JoiningCurvePoints.Reset();

	if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
	{
		const FTransform StartTransform = GetActorTransform();
		const FTransform EndTransform = RoutePlanner->Spline->GetTransformAtDistanceAlongSpline(InitialRouteOffset, ESplineCoordinateSpace::World, false);
		const float TangentSize = (StartTransform.GetTranslation() - EndTransform.GetTranslation()).Size();

		FInterpCurveVector JoiningCurve;
		JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
			0,
			StartTransform.GetTranslation(),
			FVector::ZeroVector,
			StartTransform.GetUnitAxis(EAxis::X) * TangentSize,
			CIM_CurveUser
		));
		JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
			1,
			EndTransform.GetTranslation(),
			EndTransform.GetUnitAxis(EAxis::X) * TangentSize,
			FVector::ZeroVector,
			CIM_CurveUser
		));

		const int Steps = 20;
		for (int i = 0; i <= Steps; ++i)
		{
			JoiningCurvePoints.Add(JoiningCurve.Eval(i / float(Steps)));
		}
	}
}

FTransform AGhostVehicle::GetRealAxesTransform() const
{
	FTransform Transform = GetActorTransform();
	Transform.SetLocation(
		Transform.GetLocation() -
		Transform.GetRotation().RotateVector(Transform.InverseTransformPosition(Transform.GetLocation()) - RealAxesOffset));
	return Transform;
}


void AGhostVehicle::GenerateDatasetDescription(soda::FBsonDocument& Doc) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	FExtent Extent = USodaStatics::CalculateActorExtent(this);
	*Doc
		<< "Extents" << open_document
		<< "Forward" << Extent.Forward
		<< "Backward" << Extent.Backward
		<< "Left" << Extent.Left
		<< "Right" << Extent.Right
		<< "Up" << Extent.Up
		<< "Down" << Extent.Down
		<< close_document;
}

void AGhostVehicle::OnPushDataset() const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	if (Dataset.IsValid())
	{
		try
		{
			FVector Location = GetActorLocation();
			FRotator Rotation = GetActorRotation();
			FVector Vel = FVector(CurrentVelocity, 0, 0);
			FVector Acc = FVector(CurrentAcc, 0, 0);
			//FVector AngVel = FVector(0, 0, 0);

			Dataset->GetRowDoc()
				<< "Ts" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SodaApp.GetSimulationTimestamp()))
				<< "Loc" << open_array << Location.X << Location.Y << Location.Z << close_array
				<< "Rot" << open_array << Rotation.Pitch << Rotation.Yaw << Rotation.Roll << close_array
				<< "Vel" << open_array << Vel.X << Vel.Y << Vel.Z << close_array
				<< "Acc" << open_array << Acc.X << Acc.Y << Acc.Z << close_array;
				//<< "ang_vel" << open_array << AngVel.X << AngVel.Y << AngVel.Z << close_array;

			auto RouteDoc = Dataset->GetRowDoc() << "route" << open_document;
			if (const FTrajectoryPlaner::FWayPoint* WayPoint = TrajectoryPlaner.GetCurrentWayPoint())
			{
				if (WayPoint->OwndedRoute.IsValid())
				{
					RouteDoc << "Name" << TCHAR_TO_UTF8(*WayPoint->OwndedRoute->GetName());
				}
				else
				{
					RouteDoc << "Name" << "";
				}
			}
			else
			{
				RouteDoc << "Name" << "";
			}
			RouteDoc << close_document;
				
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("AGhostPedestrian::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
		}
	}
}

const FSodaActorDescriptor* AGhostVehicle::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Ghost Car"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Car"), /*Icon*/
		true, /*bAllowTransform*/
		false, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}