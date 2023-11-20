// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Actors/NavigationRoute.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/SodaStatics.h"
#include "Engine/Canvas.h"
#include "VehicleUtility.h"
#include "WheeledVehiclePawn.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"

#define _DEG2RAD(a) ((a) / (180.0 / M_PI))
#define _RAD2DEG(a) ((a) * (180.0 / M_PI))

DECLARE_STATS_GROUP(TEXT("UVehicleInputAIComponent"), STATGROUP_UWheeledVehicleAIControllerComponent, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_AI_TickComponent, STATGROUP_UWheeledVehicleAIControllerComponent);
DECLARE_CYCLE_STAT(TEXT("CalcStreeringValue"), STAT_AI_CalcStreeringValue, STATGROUP_UWheeledVehicleAIControllerComponent);
DECLARE_CYCLE_STAT(TEXT("GetDistanceToObstacle"), STAT_AI_GetDistanceToObstacle, STATGROUP_UWheeledVehicleAIControllerComponent);
DECLARE_CYCLE_STAT(TEXT("CalcSpeedValue"), STAT_AI_CalcSpeedValue, STATGROUP_UWheeledVehicleAIControllerComponent);
DECLARE_CYCLE_STAT(TEXT("FindInputKeyClosestToWorldLocation"), STAT_AI_FindInputKeyClosestToWorldLocation, STATGROUP_UWheeledVehicleAIControllerComponent);
DECLARE_CYCLE_STAT(TEXT("GetDirectionAtSplineInputKey"), STAT_AI_GetDirectionAtSplineInputKey, STATGROUP_UWheeledVehicleAIControllerComponent);

static float GetDistanceFromLineToPoint(FVector Start, FVector End, FVector Pnt, bool LimitByLineEnd)
{
	FVector Line = (End - Start);
	float Len = Line.Size();
	Line.Normalize();

	FVector v = Pnt - Start;
	float d = FVector::DotProduct(v, Line);
	if (LimitByLineEnd)
		d = FMath::Clamp(d, 0.f, Len);
	return (Pnt - (Start + Line * d)).Size();
}

UVehicleInputAIComponent::UVehicleInputAIComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("AI");
	GUI.IcanName = TEXT("SodaIcons.AI");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	InputType = EVehicleInputType::AI;
}

bool UVehicleInputAIComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	ResetAutopilot();

	return true;
}

void UVehicleInputAIComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleInputAIComponent::BeginPlay()
{
	Super::BeginPlay();

	RouteSpline = NewObject< USplineComponent >(GetWorld());
	RouteSpline->RegisterComponentWithWorld(GetWorld());
}

void UVehicleInputAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	if (bForceUpdateInputStates && GetWheeledVehicle()->GetActiveVehicleInput() != this)
	{
		UpdateInputStatesInner(DeltaTime);
	}
}

void UVehicleInputAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsValid(RouteSpline))
	{
		RouteSpline->DestroyComponent();
	}
}

void UVehicleInputAIComponent::ResetAutopilot()
{
	TargetLocations.Reset(0);
}

bool UVehicleInputAIComponent::TryToStartRandomRoute()
{
	//TODO: Check this code insted previews
	TSet< AActor* > OverlappingActors;
	GetVehicle()->GetOverlappingActors(OverlappingActors, ANavigationRoute::StaticClass());
	for (auto & It : OverlappingActors)
	{
		ANavigationRoute* RoutePlanner = Cast<ANavigationRoute>(It);
		if (RoutePlanner && RoutePlanner->bAllowForVehicles)
		{
			return AssignRoute(RoutePlanner);
		}
	}
	

	/*
	for (TActorIterator< ANavigationRoute > It(GetWorld()); It; ++It)
	{
		ANavigationRoute* RoutePlanner = *It;

		if (RoutePlanner->bAllowForVehicles)
		{
			// Check if we are inside this route planner.
			TSet< AActor* > OverlappingActors;
			RoutePlanner->TriggerVolume->GetOverlappingActors(OverlappingActors, ASodaWheeledVehicle::StaticClass());
			for (auto* Actor : OverlappingActors)
			{
				if (Actor == GetVehicle())
				{
					return AssignRoute(RoutePlanner);
				}
			}
		}
	}
	*/

	return false;
}

bool UVehicleInputAIComponent::TryToFindNearestRoute()
{
	USplineComponent * NearestSpline = nullptr;
	float NearestDistance = TNumericLimits< float >::Max();
	float NearestSplineKey = 0;
	float NearestSplineKeyDistance = 0;

	FVector VehicleLocation = GetVehicle()->GetActorLocation();
	FVector VehicleForwardVector = GetVehicle()->GetActorForwardVector();

	for (TActorIterator< ANavigationRoute > It(GetWorld()); It; ++It)
	{
		if ((*It)->bAllowForVehicles)
		{
			float Key = (*It)->Spline->FindInputKeyClosestToWorldLocation(VehicleLocation);
			FVector Location = (*It)->Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
			FVector Direction = (*It)->Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);

			float Distance = (VehicleLocation - Location).Size();
			float Angle = FMath::Acos(VehicleForwardVector.CosineAngle2D(Direction));

			if ((Distance < 200) && (Distance < NearestDistance) && (Angle < M_PI / 2))
			{
				float KeyDistance = USodaStatics::GetDistanceAlongSpline(Key, (*It)->Spline);
				if ((*It)->Spline->GetSplineLength() - KeyDistance > 200)
				{
					NearestDistance = Distance;
					NearestSpline = (*It)->Spline;
					NearestSplineKey = Key;
					NearestSplineKeyDistance = KeyDistance;
				}
			}
		}
	}

	if (NearestSpline)
	{
		return SetFixedRouteBySpline(NearestSpline, bDriveBackvard, false, NearestSplineKeyDistance);
	}
	return false;
}

bool UVehicleInputAIComponent::TryToAppendRandomRoute()
{
	for (TActorIterator< ANavigationRoute > It(GetWorld()); It; ++It)
	{
		ANavigationRoute* RoutePlanner = *It;

		if (RoutePlanner->bAllowForVehicles)
		{
			// Check if last target location is inside this route planner.
			if (IsPointInsideBox(TargetLocations.Last(), RoutePlanner->TriggerVolume) && RoutePlanner->bDriveBackvard == bDriveBackvard)
			{
				return AssignRoute(RoutePlanner);
			}
		}
	}
	return false;
}

void UVehicleInputAIComponent::SetFixedRouteByWaypoints(const TArray< FVector >& Locations, bool bNewDriveBackvard, const bool bOverwriteCurrent)
{
	if (bOverwriteCurrent)
	{
		TargetLocations.Reset(0);
	}

	bDriveBackvard = bNewDriveBackvard;

	for (auto& Location : Locations)
	{
		if (TargetLocations.Num() == 0)
			TargetLocations.Add(Location);
		else if ((TargetLocations.Last() - Location).Size() > SamePointsDelta)
			TargetLocations.Add(Location);
		else if (bDebugOutputLog)
			if(bDebugOutputLog) UE_LOG(LogSoda, Error, TEXT("UVehicleInputAIComponent Waypoint (%f, %f, %f) purged."), Location.X, Location.Y, Location.Z);
	}
	RouteSpline->ClearSplinePoints(false);
	RouteSpline->SetSplinePoints(TargetLocations, ESplineCoordinateSpace::World, true);
	CurrentSplineSegment = -1;
}

bool UVehicleInputAIComponent::SetFixedRouteBySpline(const USplineComponent* SplineRoute, bool bNewDriveBackvard, bool bOverwriteCurrent, float StartOffset)
{

	TArray< FVector > WayPoints;
	const float Step = 50; // Points step in [cm]
	const auto Size = (size_t)((SplineRoute->GetSplineLength() - StartOffset) / Step);

	if (Size > 1)
	{
		WayPoints.Reserve(Size);
		for (auto i = 1; i < Size; ++i)
		{
			WayPoints.Add(SplineRoute->GetLocationAtDistanceAlongSpline(StartOffset + i * Step, ESplineCoordinateSpace::World));
		}
		SetFixedRouteByWaypoints(WayPoints, bNewDriveBackvard, bOverwriteCurrent);
		return true;
	}
	else
	{
		if(bDebugOutputLog) UE_LOG(LogSoda, Error, TEXT("UVehicleInputAIComponent '%s' has a route with zero way-points."), *GetName());
		return false;
	}
}

void UVehicleInputAIComponent::UpdateInputStatesInner(float DeltaTime)
{
	if (!GetWheeledVehicle()->GetWheeledComponentInterface()) return;
	
	const auto Speed = Chaos::CmSToKmH(GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X);
	const auto AbsSpeed = fabsf(Speed);
	const FVector CurrentLocation = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation();
	CurrentObstacleDistance = GetDistanceToObstacle();
	const float TargetSpeed = CalcSpeedValue(CurrentObstacleDistance);
	const FVector Forward = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetRotation().Rotator().Vector() *
		(bDriveBackvard ? -1.0 : 1.0);

	// Clear target locations
	while (TargetLocations.Num() > 0 && (TargetLocations[0] - CurrentLocation).CosineAngle2D(Forward) < 0 /*&& AbsSpeed > 1.f*/)
	{
		TargetLocations.RemoveAt(0);
	}

	// Append route on the end of current spline
	if (TargetLocations.Num() > 2)
	{
		auto PrevLocation = CurrentLocation;
		float DistToRouteEnd = 0;
		for (size_t i = 0; i < TargetLocations.Num(); ++i)
		{
			DistToRouteEnd += FVector::Dist(TargetLocations[i], PrevLocation);
			PrevLocation = TargetLocations[i];
		}

		const float RoutePreloadDistance = 10000.f; // [cm]
		if (DistToRouteEnd < RoutePreloadDistance || DistToRouteEnd < GetSlowingDistance())
		{
			TryToAppendRandomRoute();
		}
	}

	// Finding new spline for route
	if (TargetLocations.Num() == 0)
	{
		if (!TryToStartRandomRoute())
		{
			TryToFindNearestRoute();
		}
	}

	//Compute Throttle, Steering, Gear
	Throttle = 0;
	Steering = TargetLocations.Num() ? CalcStreeringValue(DeltaTime) : 0.0f;
	Gear = bDriveBackvard ? ENGear::Reverse : ENGear::Drive;

	if (CurrentObstacleDistance > 0.f && CurrentObstacleDistance < ObstacleStopDistance * 100 && !bDriveBackvard) // Stop if Obstacle
	{
		Throttle = ThrottleStop(AbsSpeed);
	}
	else if ((Speed > 0.05f && bDriveBackvard) || (Speed < -0.05f && !bDriveBackvard)) // Stop if wrong moving diretion
	{
		Throttle = ThrottleStop(AbsSpeed);
		Steering = 0.0f;
	}
	else if (TargetLocations.Num() == 0) // Route is end. Stopping...
	{
		Throttle = ThrottleStop(AbsSpeed);
		Steering = 0.0f;
		if (AbsSpeed < 0.1f)
		{
			Gear = ENGear::Park;
		}

	}
	else // Continue moving
	{
		Throttle = ThrottleMove(AbsSpeed, TargetSpeed);
	}

	if(bDrawCurrentRoute)
	{
		DrawCurrentRoute();
	}
}

void UVehicleInputAIComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	check(GetWheeledVehicle() != nullptr);

	SCOPE_CYCLE_COUNTER(STAT_AI_TickComponent);	

	UpdateInputStatesInner(DeltaTime);

	BrakeInput    = BrakeInputRate.InterpInputValue(DeltaTime, BrakeInput, FMath::Clamp(-1.f * Throttle, 0.f, 1.f)); 
	ThrottleInput = ThrottleInputRate.InterpInputValue(DeltaTime, ThrottleInput, FMath::Min(FMath::Clamp(Throttle, 0.f, 1.f), ThrottlePedalLimit));
	SteeringInput = SteerInputRate.InterpInputValue(DeltaTime, SteeringInput, Steering);
	GearInput = Gear;
}

float UVehicleInputAIComponent::CalcStreeringValue(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_CalcStreeringValue);

	const FVector CurrentLocation = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation();
	const FVector Forward = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetRotation().Rotator().Vector() *
		(bDriveBackvard ? -1.0 : 1.0);
	const auto Speed = Chaos::CmSToKmH(GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X);

	const auto ForvardedLocationDir = CurrentLocation + Forward.GetSafeNormal() * ForwardingDistanceDir;
	const auto ForvardedLocationLoc = CurrentLocation + Forward.GetSafeNormal() * ForwardingDistanceLoc;

	const float SplineKey = USodaStatics::FindSplineInputKeyClosestToWorldLocationFast(CurrentLocation, CurrentSplineSegment, RouteSpline);
	const float SplineDirAtCurrentLoc = _RAD2DEG(RouteSpline->GetDirectionAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World).GetUnsafeNormal().UnitCartesianToSpherical().Y);

	float SplineInputKeyDir = 0;
	float SplineInputKeyLoc = 0;

	{
		SCOPE_CYCLE_COUNTER(STAT_AI_FindInputKeyClosestToWorldLocation);
		SplineInputKeyDir = USodaStatics::FindSplineInputKeyClosestToWorldLocationFast(ForvardedLocationDir, CurrentSplineSegment, RouteSpline);
		SplineInputKeyLoc = USodaStatics::FindSplineInputKeyClosestToWorldLocationFast(ForvardedLocationLoc, CurrentSplineSegment, RouteSpline);
	}

	FVector ForwardedSplineDirection;
	FVector ForwardedSplineLocation;
	{
		SCOPE_CYCLE_COUNTER(STAT_AI_GetDirectionAtSplineInputKey);
		ForwardedSplineDirection = RouteSpline->GetDirectionAtSplineInputKey(SplineInputKeyDir, ESplineCoordinateSpace::World).GetUnsafeNormal();
		ForwardedSplineLocation = RouteSpline->GetLocationAtSplineInputKey(SplineInputKeyLoc, ESplineCoordinateSpace::World);
	}

	SideError = (RouteSpline->GetLocationAtSplineInputKey(SplineKey, ESplineCoordinateSpace::World) - CurrentLocation).Size2D();

	auto LocErrorVector = ForwardedSplineLocation - CurrentLocation;
	LocErrorVector.Z = 0;

	FVector DirectionToCorrectSideError = LocErrorVector.GetUnsafeNormal().GetSafeNormal();

	float ForwardedSplineDirAngle = _RAD2DEG(ForwardedSplineDirection.UnitCartesianToSpherical().Y);
	float DirectionToCorrectSideErrorAngle = _RAD2DEG(DirectionToCorrectSideError.UnitCartesianToSpherical().Y);
	float ActorAngle = _RAD2DEG(Forward.UnitCartesianToSpherical().Y);

	float ErrorAngle = FMath::UnwindDegrees(DirectionToCorrectSideErrorAngle - ActorAngle) * SteerDirToLocErrorCoef + FMath::UnwindDegrees(ForwardedSplineDirAngle - ActorAngle) * (1.f - SteerDirToLocErrorCoef);

	if (bDriveBackvard)
	{
		ErrorAngle *= -1.f;
	}

	float FeedForwardAngle = FMath::UnwindDegrees(ForwardedSplineDirAngle - SplineDirAtCurrentLoc);

	if (bDriveBackvard)
	{
		FeedForwardAngle *= -1.f;
	}

	const float MaxSpeedToErrorReducing = 50.f; // [km/h]
	ErrorAngle *= FMath::Lerp(SteeringDirErrorReducingAtLowSpeedCoef, 1.f, FMath::Clamp(fabsf(Speed) / MaxSpeedToErrorReducing, 0.f, 1.f));

	return FMath::Clamp(SteeringPID.CalculatePID(FeedForwardAngle, ErrorAngle, DeltaTime), -1.f, 1.f);
}

float UVehicleInputAIComponent::CalcSpeedValue(float ObstacleDistance)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_CalcSpeedValue);

	const FVector CurrentLocation = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation();

	if (TargetLocations.Num() == 0)
	{
		return 0;
	}
	else if (TargetLocations.Num() == 1)
	{
		return SpeedSlow;
	}

	float Dist = FVector::Dist(TargetLocations[0], CurrentLocation);
	float DistAlongRoute = 0;
	FVector PrevLocation = CurrentLocation;
	float Speed = Chaos::CmSToKmH(GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X);
	float SlowingDistance = GetSlowingDistance();
	float FusedCurvature = 0;

	float DistToRouteEnd = 0;
	for (size_t i = 0; i < TargetLocations.Num(); ++i)
	{
		DistToRouteEnd += FVector::Dist(TargetLocations[i], PrevLocation);
		PrevLocation = TargetLocations[i];
	}
	if (!bDriveBackvard && ObstacleDistance > 0.f)
		DistToRouteEnd = fminf(DistToRouteEnd, ObstacleDistance - ObstacleStopDistance * 100);

	float MaxSpeed = FMath::Lerp(0.0f, SpeedLimit, FMath::Clamp(DistToRouteEnd / SlowingDistance, 0.f, 1.f));
	float RetSpeed = SpeedLimit;

	float DebugAngle = 0;
	float DebugMeanCurv = 0;
	float DebugFusedCurvature = 0;
	float DebugCalcSpeed = 0;
	float DebugNextTurnAngle = 0;
	float DebugDistAlongRoute = 0;

	if (TargetLocations.Num() >= 2)
	{
		PrevLocation = TargetLocations[0];

		for (size_t i = 1; i < TargetLocations.Num() - 1 && DistAlongRoute < SlowingDistance; ++i)
		{
			DistAlongRoute += FVector::Dist(TargetLocations[i], PrevLocation);
			if (acos(FMath::Clamp((TargetLocations[i] - PrevLocation).CosineAngle2D(TargetLocations[i + 1] - TargetLocations[i]), -1.f, 1.f)) > PI * 0.75f || FVector::Dist(TargetLocations[i], PrevLocation) < 20.f || FVector::Dist(TargetLocations[i], TargetLocations[i + 1]) < 20.f)
			{
				PrevLocation = TargetLocations[i];
				continue;
			}

			float Angle = acos(FMath::Clamp((TargetLocations[i] - PrevLocation).CosineAngle2D(TargetLocations[i + 1] - TargetLocations[i]), -1.f, 1.f));
			float MeanCurv = Angle / FVector::Dist(TargetLocations[i], PrevLocation);
			FusedCurvature = (1.f - CurvatreSmoothingCoef) * FusedCurvature + CurvatreSmoothingCoef * MeanCurv;
			float CalcSpeed = fminf(MaxSpeed, fmaxf(SpeedSlow, 1.0f / (FusedCurvature * SpeedToCurvatureCoef + SideError * SideErrorSpeedCoef + 0.0001f)));
			float NewRetSpeed = Rate * FMath::Lerp(CalcSpeed, MaxSpeed, FMath::Clamp(DistAlongRoute / SlowingDistance, 0.f, 1.f));
			if (NewRetSpeed < RetSpeed)
			{
				RetSpeed = NewRetSpeed;
				DebugAngle = Angle;
				DebugFusedCurvature = FusedCurvature * SpeedToCurvatureCoef;
				DebugCalcSpeed = CalcSpeed;
				DebugDistAlongRoute = DistAlongRoute;
			}

			PrevLocation = TargetLocations[i];
		}
	}

	if (bDebugOutputLog)
	{
		UE_LOG(LogSoda, Warning, TEXT(/*"DistToRouteEnd = %f, ObstacleDist = %f, DistAlongRoute = %f, */"SlowingDist = %f, Angle = %f, FusedCurvPart = %f, SideErrorPart = %f, CalcSpeed = %f, RetSpeed = %f, CurSpeed = %f"),
			   /*DistToRouteEnd * 0.01f, ObstacleDistance * 0.01f, DebugDistAlongRoute * 0.01f, */SlowingDistance * 0.01f, DebugAngle, DebugFusedCurvature, SideError * SideErrorSpeedCoef, DebugCalcSpeed, RetSpeed, Speed);
	}
	return RetSpeed;
}

float UVehicleInputAIComponent::GetSlowingDistance() const
{
	float Speed = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X * 0.01f;
	float BreakingTime = Speed / MaxBreakingAcceleration;
	return 100 * (Speed * (BreakingTime + BreakingApplyingLag) - (MaxBreakingAcceleration * BreakingTime * BreakingTime) / 2);
}

float UVehicleInputAIComponent::ThrottleMove(float CurSpeed, float TargetSpeed)
{
	if (TargetSpeed < 0.1f && (CurSpeed * ThrottlePropCoef) < 1.f)
		return -1.f;
	return FMath::Clamp((TargetSpeed - CurSpeed) * ThrottlePropCoef, -1.f, 1.f);
}

float UVehicleInputAIComponent::RayCast(const FVector& Start, const FVector& End)
{	
	FHitResult OutHit;
	static FName TraceTag = FName(TEXT("VehicleTrace"));
	FCollisionQueryParams CollisionParams(TraceTag, true);
	CollisionParams.AddIgnoredActor(GetVehicle());
	CollisionParams.bFindInitialOverlaps = true;

	const bool Success = GetVehicle()->GetWorld()->SweepSingleByChannel(
		OutHit, Start, End, (End - Start).Rotation().Quaternion(),
		ECollisionChannel::ECC_Visibility,
		FCollisionShape::MakeBox(FVector(50, 150, 100)), CollisionParams);

	if (Success && OutHit.bBlockingHit)
	{
		return OutHit.Distance;
	}

	return -1.f;
}

float UVehicleInputAIComponent::CheckIntersectionWithOtherActors(const FVector& Start, const FVector& End, const TArray<AActor*> ActorsToCheck)
{
	const float DistanceToDetectIntersection = 100.f;
	for (AActor* Actor : ActorsToCheck)
	{
		FVector ActorLoc = Actor->GetActorLocation();
		ActorLoc.Z = Start.Z;
		if (bDrawCurrentRoute)
			DrawDebugString(GetWorld(), ActorLoc, *FString::Printf(TEXT("ActorToCheck=%s"), *Actor->GetName()), NULL, FColor::Red, 0.01f, false);

		if (GetDistanceFromLineToPoint(Start, End, ActorLoc, false) < DistanceToDetectIntersection)
		{
			FVector Line = (End - Start);
			Line.Normalize();

			FVector Vec = Actor->GetActorLocation() - Start;
			float Dist = FVector::DotProduct(Vec, Line);
			if (Dist > 0.f)
				return Dist;
		} 
	}
	return -1.f;
}

float UVehicleInputAIComponent::GetDistanceToObstacle()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_GetDistanceToObstacle);

	if (TargetLocations.Num() < 2)
		return -1.f;

	float DistAlongRoute = 0;
	size_t SegmentBeginIndex = 0;
	int CollidersUsed = 0;

	const float LookoutDistance = GetSlowingDistance() + ObstacleStopDistance * 100.f;

	TArray<AActor*> ActorsToCheck;
	if (bUseActorsPositionChecking)
	{
		TArray<AActor*> TempActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWheeledVehiclePawn::StaticClass(), ActorsToCheck);
		ActorsToCheck.Append(TempActors);
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASodaWheeledVehicle::StaticClass(), TempActors);
		ActorsToCheck.Append(TempActors);

		ActorsToCheck.RemoveAll([this, LookoutDistance](const AActor* Ptr) {
			return ((Ptr->GetActorLocation() - TargetLocations[0]).Size() > LookoutDistance ||
					 Ptr->GetName().Contains("BP_ArrivalFakeCar_C"));
		});
		ActorsToCheck.Remove(GetVehicle());

		if (ActorsToCheck.Num() == 0)
			return -1.f;
	}

	for (size_t i = 0; i + 1 < TargetLocations.Num() && DistAlongRoute < LookoutDistance; i++)
	{
		float Error = GetDistanceFromLineToPoint(TargetLocations[SegmentBeginIndex], TargetLocations[SegmentBeginIndex + 1], TargetLocations[i + 1], false);
		if (i + 2 >= TargetLocations.Num() ||
			Error > RouteSectionPointMaxSideDeviation)
		{
			const FVector StartCenter = TargetLocations[SegmentBeginIndex];
			const FVector EndCenter = TargetLocations[i];

			float HitDistance = -1;
			if (bUseRayCast)
			{
				HitDistance = RayCast(StartCenter, EndCenter);
				CollidersUsed++;
			}
			if (bUseActorsPositionChecking)
				HitDistance = CheckIntersectionWithOtherActors(StartCenter, EndCenter, ActorsToCheck);

			if (bDrawTargetLocations)
			{
				DrawDebugString(GetWorld(), TargetLocations[SegmentBeginIndex], *FString::Printf(TEXT("PointSideError=%f, Segment: BeginIndex=%d, EndIndex=%d"), Error, SegmentBeginIndex, i), NULL, FColor::Yellow, 0.01f, false);

				DrawDebugLine(
					GetWorld(),
					StartCenter,
					EndCenter,
					HitDistance > 0.f ? FColor::Red : FColor::Yellow,
					false, -1.f, 0, 2.f);
				DrawDebugPoint(GetWorld(), EndCenter, 10, HitDistance > 0.f ? FColor::Red : FColor::Yellow, false, -1.0f);
			}

			if (HitDistance > 0.f)
			{
				if (bUseRayCast && bDrawTargetLocations) DrawDebugString(GetWorld(), TargetLocations[0] + FVector(0, 0, 20.f), *FString::Printf(TEXT("Total colliders used=%d"), CollidersUsed), NULL, FColor::Green, 0.01f, false);

				return HitDistance + DistAlongRoute;
			}

			SegmentBeginIndex = i;
			DistAlongRoute += (EndCenter - StartCenter).Size();
		}
	}
	if (bUseRayCast && bDrawTargetLocations) DrawDebugString(GetWorld(), TargetLocations[0] + FVector(0, 0, 20.f), *FString::Printf(TEXT("Total colliders used=%d"), CollidersUsed), NULL, FColor::Green, 0.01f, false);

	return -1.f;
}

bool UVehicleInputAIComponent::IsPointInsideBox(FVector Point, UBoxComponent* BoxComponent) const
{
	float xmin = BoxComponent->GetComponentLocation().X - BoxComponent->GetScaledBoxExtent().X - 100;
	float xmax = BoxComponent->GetComponentLocation().X + BoxComponent->GetScaledBoxExtent().X + 100;
	float ymin = BoxComponent->GetComponentLocation().Y - BoxComponent->GetScaledBoxExtent().Y - 100;
	float ymax = BoxComponent->GetComponentLocation().Y + BoxComponent->GetScaledBoxExtent().Y + 100;
	float zmin = BoxComponent->GetComponentLocation().Z - BoxComponent->GetScaledBoxExtent().Z - 100;
	float zmax = BoxComponent->GetComponentLocation().Z + BoxComponent->GetScaledBoxExtent().Z + 100;

	if ((Point.X <= xmax && Point.X >= xmin) && (Point.Y <= ymax && Point.Y >= ymin) && (Point.Z <= zmax && Point.Z >= zmin))
	{
		return true;
	}
	return false;
}

void UVehicleInputAIComponent::DrawCurrentRoute()
{
	bool ModeAI = GetWheeledVehicle()->GetActiveVehicleInput() == this;

	for (int j = 0, lenNumPoints = TargetLocations.Num() - 1; j < lenNumPoints; ++j)
	{
		const FVector p0 = TargetLocations[j + 0];
		const FVector p1 = TargetLocations[j + 1];

		static const float MinThickness = 3.f;
		static const float MaxThickness = 15.f;

		const float Dist = (float)j / (float)lenNumPoints;
		const float OneMinusDist = 1.f - Dist;
		const float Thickness = OneMinusDist * MaxThickness + MinThickness;

		if (!ModeAI)
		{
			// from blue to black
			DrawDebugLine(
				GetWorld(), p0, p1, FColor(0, 0, 255 * OneMinusDist),
				false, -1.f, 0, Thickness);
		}
		else
		{
			// from green to black
			DrawDebugLine(
				GetWorld(), p0, p1, FColor(0, 255 * OneMinusDist, 0),
				false, -1.f, 0, Thickness);
		}
	}
}

void UVehicleInputAIComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	if (GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		Super::DrawDebug(Canvas, YL, YPos);

		if (Common.bDrawDebugCanvas)
		{
			UFont* RenderFont = GEngine->GetSmallFont();
			Canvas->SetDrawColor(FColor::White);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Steering: %.2f"), GetSteeringInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Throttle: %.2f"), GetThrottleInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Brake: %.2f"), GetBrakeInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("TargetLocations: %i"), TargetLocations.Num()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SideError: %f"), SideError), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("ObstacleDistance: %f"), CurrentObstacleDistance), 16, YPos);
		}
	}
}

bool UVehicleInputAIComponent::AssignRoute(const ANavigationRoute* RoutePlanner)
{
	check(RoutePlanner);
	if (auto* Route = RoutePlanner->GetSpline())
	{
		SetFixedRouteBySpline(Route, RoutePlanner->bDriveBackvard, false);
	}
	return false;
}

void UVehicleInputAIComponent::CopyInputStates(UVehicleInputComponent* Previous)
{
	if (Previous)
	{
		SteeringInput = Previous->GetSteeringInput();
		ThrottleInput = Previous->GetThrottleInput();
		BrakeInput = Previous->GetBrakeInput();
		GearInput = Previous->GetGearInput();
	}
}