// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/TrajectoryPlaner.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaGameMode.h"
#include "Soda/SodaStatics.h"
#include "Soda/Actors/NavigationRoute.h"
#include "EngineUtils.h"

FTrajectoryPlaner::FWayPoint::FWayPoint(const FVector& InLocation, ANavigationRoute* InRoute)
	: Location(InLocation)
	, OwndedRoute(InRoute)
{}


FTrajectoryPlaner::~FTrajectoryPlaner()
{
	Deinit();
}

void FTrajectoryPlaner::Init(UWorld* World)
{
	check(World);
	check(RouteSpline == nullptr);

	RouteSpline = NewObject< USplineComponent >(World);
	RouteSpline->RegisterComponentWithWorld(World);
}

void FTrajectoryPlaner::Deinit()
{
	if (IsValid(RouteSpline))
	{
		RouteSpline->DestroyComponent();
	}
	RouteSpline = nullptr;
}

USplineComponent* FTrajectoryPlaner::FindNearestRoute(
	const UWorld* World,
	const FTransform& Transform,
	float MaxCodirAng,
	float MaxDistance,
	float MaxDistanceToEnd,
	ANavigationRoute** OutRoutePlanner,
	float* OutSplineOffest
)
{
	USplineComponent* NearestSpline = nullptr;
	float NearestDistance = TNumericLimits< float >::Max();
	float NearestSplineKey = 0;
	float NearestSplineKeyDistance = 0;

	const FVector ActorLocation = Transform.GetTranslation();
	const FVector ActorForwardVector = Transform.GetUnitAxis(EAxis::X);

	for (TActorIterator< ANavigationRoute > It(World); It; ++It)
	{
		if ((*It)->bAllowForVehicles)
		{
			float Key = (*It)->Spline->FindInputKeyClosestToWorldLocation(ActorLocation);
			FVector Location = (*It)->Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
			FVector Direction = (*It)->Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);

			float Distance = (Location - Location).Size();
			float Angle = FMath::Acos(ActorForwardVector.CosineAngle2D(Direction)) / M_PI * 180;

			if ((Distance < MaxDistance) && (Distance < NearestDistance) && (Angle < MaxCodirAng))
			{
				float KeyDistance = USodaStatics::GetDistanceAlongSpline(Key, (*It)->Spline);
				if ((*It)->Spline->GetSplineLength() - KeyDistance > 200)
				{
					NearestDistance = Distance;
					NearestSpline = (*It)->Spline;
					NearestSplineKey = Key;
					NearestSplineKeyDistance = KeyDistance;
					if (OutRoutePlanner) *OutRoutePlanner = *It;
				}
			}
		}
	}

	if (OutSplineOffest) *OutSplineOffest = NearestSplineKeyDistance;

	return NearestSpline;
}

ANavigationRoute* FTrajectoryPlaner::FindRoutePlannerOverlapedByActor(const AActor* InActor)
{
	for (TActorIterator< ANavigationRoute > It(InActor->GetWorld()); It; ++It)
	{
		ANavigationRoute* RoutePlanner = *It;

		if (RoutePlanner->bAllowForVehicles)
		{
			// Check if we are inside this route planner.
			TSet< AActor* > OverlappingActors;
			RoutePlanner->TriggerVolume->GetOverlappingActors(OverlappingActors, ASodaWheeledVehicle::StaticClass());
			for (auto* Actor : OverlappingActors)
			{
				if (Actor == InActor)
				{
					return RoutePlanner;
				}
			}
		}
	}
	return nullptr;
}

ANavigationRoute* FTrajectoryPlaner::FindRoutePlannerOverlapedByPoint(const UObject* WorldContextObject, const FVector& TargetLocation, bool bDriveBackvard, const TSet<FString>& AllowedRouteTags)
{
	for (TActorIterator< ANavigationRoute > It(WorldContextObject->GetWorld()); It; ++It)
	{
		ANavigationRoute* RoutePlanner = *It;

		if (AllowedRouteTags.Num())
		{
			if (!RoutePlanner->IsRouteTagsAllowed(AllowedRouteTags))
			{
				continue;
			}
		}

		if (RoutePlanner->bAllowForVehicles)
		{
			if (IsPointInsideBox(TargetLocation, RoutePlanner->TriggerVolume) && RoutePlanner->bDriveBackvard == bDriveBackvard)
			{
				return RoutePlanner;
			}
		}
	}
	return nullptr;
}

bool FTrajectoryPlaner::IsPointInsideBox(const FVector& Point, const UBoxComponent* BoxComponent, float Border)
{
	float xmin = BoxComponent->GetComponentLocation().X - BoxComponent->GetScaledBoxExtent().X - Border;
	float xmax = BoxComponent->GetComponentLocation().X + BoxComponent->GetScaledBoxExtent().X + Border;
	float ymin = BoxComponent->GetComponentLocation().Y - BoxComponent->GetScaledBoxExtent().Y - Border;
	float ymax = BoxComponent->GetComponentLocation().Y + BoxComponent->GetScaledBoxExtent().Y + Border;
	float zmin = BoxComponent->GetComponentLocation().Z - BoxComponent->GetScaledBoxExtent().Z - Border;
	float zmax = BoxComponent->GetComponentLocation().Z + BoxComponent->GetScaledBoxExtent().Z + Border;

	if ((Point.X <= xmax && Point.X >= xmin) && (Point.Y <= ymax && Point.Y >= ymin) && (Point.Z <= zmax && Point.Z >= zmin))
	{
		return true;
	}
	return false;
}

bool FTrajectoryPlaner::UpdateTarjectory(const UWorld* World, const FTransform& Transform, float PreloadDistance, const TSet<FString>& AllowedRouteTags)
{
	bool bAddedSpline = false;

	FVector CurrentLocation = Transform.GetTranslation();
	FRotator CurrentRotation = Transform.GetRotation().Rotator();

	/* Remove passed points */
	/*
	if (bAutoSweep)
	{
		for (int i = int(CurrentSplineKey); i < WayPoints.Num(); ++i)
		{
			if ((WayPoints[i].Location - CurrentLocation).CosineAngle2D(CurrentRotation.Vector()) < 0)
			{
				CurrentSplineKey = i;
			}
			else
			{
				break;
			}
		}
	}
	*/

	int WayPointsNum = WayPoints.Num() - int(CurrentSplineKey);

	/* Try to append route */
	if (WayPointsNum > 1)
	{
		FVector PrevLocation = CurrentLocation;
		float DistToRouteEnd = 0;

		for (size_t i = (size_t)CurrentSplineKey; i < WayPoints.Num(); ++i)
		{
			DistToRouteEnd += FVector::Dist(WayPoints[i].Location, PrevLocation);
			PrevLocation = WayPoints[i].Location;
		}

		if (DistToRouteEnd < PreloadDistance)
		{
			if (ANavigationRoute* RoutePlanner = FindRoutePlannerOverlapedByPoint(World, WayPoints.Last().Location, false, AllowedRouteTags))
			{
				if (const USplineComponent* Route = RoutePlanner->GetSpline())
				{
					AddRouteBySpline(Route, 0, RoutePlanner, RoutePlanner->bDriveBackvard, false);
					bAddedSpline = true;
				}
			}
		}
	}

	/* Try to find new route */

	/*
	if (WayPointsNum == 0)
	{
		ANavigationRoute* RoutePlanne = FindRoutePlannerOverlapedByActor(Actor);
		const USplineComponent* Route = nullptr;
		float RouteOffest = 0;
		if (RoutePlanne)
		{
			Route = RoutePlanne->GetSpline();
		}
		if (!Route)
		{
			Route = FindNearestRoute(World, Transform, 90, 500, 500, &RoutePlanne, &RouteOffest);
		}
		if (RoutePlanne && Route)
		{
			SetFixedRouteBySpline(Route, RoutePlanne->bDriveBackvard, false, RouteOffest);
			bAddedSpline = true;
		}
	}
	*/

	return bAddedSpline;
}

bool FTrajectoryPlaner::AddRouteBySpline(const USplineComponent* SplineRoute, float StartOffset, ANavigationRoute* OwnedRoute, bool bNewDriveBackvard, bool bOverwriteCurrent, bool bUpdateSpline)
{
	const float Step = 50; // Points step in [cm]
	const int Size = ((SplineRoute->GetSplineLength() - StartOffset) / Step);

	if (Size <= 1)
	{
		return false;
	}

	if (bOverwriteCurrent)
	{
		WayPoints.Reset(0);
		CurrentSplineKey = 0;
	}
	else
	{
		WayPoints.RemoveAt(0, (int)CurrentSplineKey);
		CurrentSplineKey = CurrentSplineKey - (int)CurrentSplineKey;
	}

	for (auto i = 0; i < Size; ++i)
	{
		WayPoints.Add(FWayPoint(SplineRoute->GetLocationAtDistanceAlongSpline(StartOffset + i * Step, ESplineCoordinateSpace::World), OwnedRoute));
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}

	return true;
}

bool FTrajectoryPlaner::AddRouteByWaypoints(const TArray< FVector >& Locations, ANavigationRoute* OwnedRoute, bool bNewDriveBackvard, bool bOverwriteCurrent, bool bUpdateSpline)
{
	if (bOverwriteCurrent)
	{
		WayPoints.Reset(0);
		CurrentSplineKey = 0;
	}
	else
	{
		WayPoints.RemoveAt(0, (int)CurrentSplineKey);
		CurrentSplineKey = CurrentSplineKey - (int)CurrentSplineKey;
	}

	for (auto& Location : Locations)
	{
		if (WayPoints.Num() == 0)
			WayPoints.Add({Location, OwnedRoute});
		else if ((WayPoints.Last().Location - Location).Size() > SamePointsDelta)
			WayPoints.Add({ Location, OwnedRoute });
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}

	return !!Locations.Num();
}

bool FTrajectoryPlaner::AddRouteByTransfoms(const FTransform& StartTransform, const FTransform& EndTransform, ANavigationRoute* OwnedRoute, bool bNewDriveBackvard, bool bOverwriteCurrent, bool bUpdateSpline)
{
	const float TangentSize = (StartTransform.GetTranslation() - EndTransform.GetTranslation()).Size();
	FInterpCurveVector JoiningCurve;
	JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
		0,
		StartTransform.GetTranslation(),
		FVector::ZeroVector,
		StartTransform.GetUnitAxis(EAxis::X) * TangentSize,
		CIM_CurveUser));
	JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
		1,
		EndTransform.GetTranslation(),
		EndTransform.GetUnitAxis(EAxis::X) * TangentSize,
		FVector::ZeroVector,
		CIM_CurveUser));

	TArray< FVector > Points;
	const int Steps = 20;
	Points.Reserve(Steps + 1);
	for (int i = 0; i <= Steps; ++i)
	{
		Points.Add(JoiningCurve.Eval(i / float(Steps)));
	}

	return AddRouteByWaypoints(Points, OwnedRoute, bNewDriveBackvard, bOverwriteCurrent, bUpdateSpline);
}

void FTrajectoryPlaner::UpdateSpline()
{
	if (IsValid(RouteSpline))
	{
		RouteSpline->ClearSplinePoints(false);

		TArray<FVector> Tmp;
		Tmp.Reserve(WayPoints.Num());
		for (auto& It : WayPoints)  Tmp.Add(It.Location);
		RouteSpline->ClearSplinePoints(false);
		RouteSpline->SetSplinePoints(Tmp, ESplineCoordinateSpace::World, true);
	}
}

void FTrajectoryPlaner::DrawRoute(const UObject* WorldContextObject) const
{
	for (int j = 0, lenNumPoints = WayPoints.Num() - 1; j < lenNumPoints; ++j)
	{
		FVector p0 = WayPoints[j + 0].Location;
		FVector p1 = WayPoints[j + 1].Location;

		if (CurrentSplineKey <= j)
		{
			DrawDebugLine(WorldContextObject->GetWorld(), p0, p1, FColor(0, 255, 0), false, -1.f, 0, 20);
		}
		else
		{
			p0.Z -= 20;
			p1.Z -= 20;
			DrawDebugLine(WorldContextObject->GetWorld(), p0, p1, FColor(150, 150, 150), false, -1.f, 0, 20);
		}
	}
}

/*
bool FTrajectoryPlaner::GoToNearesRoute(const UWorld * World, const FTransform & InTransform)
{
	float SplineOffest = 0;
	USplineComponent* NearestSpline = FindNearestRoute(
		World, InTransform, 45, 200, 200, nullptr, &SplineOffest);

	if (!NearestSpline)
	{
		return false;
	}

	const FTransform Transform = NearestSpline->GetTransformAtDistanceAlongSpline(SplineOffest + WayOutOffset, ESplineCoordinateSpace::World, false);
	const FVector StartLocation = InTransform.GetTranslation();
	const FVector StartForwardVector = InTransform.GetUnitAxis(EAxis::X);
	const FVector EndLocation = Transform.GetTranslation();
	const FVector EndForwardVector = Transform.GetRotation().GetForwardVector();

	check(IsValid(RouteSpline));
	RouteSpline->ClearSplinePoints(false);
	RouteSpline->AddPoint(FSplinePoint(0, StartLocation, StartForwardVector * WayOutTangentSize, StartForwardVector * WayOutTangentSize), false);
	RouteSpline->AddPoint(FSplinePoint(1, EndLocation, EndForwardVector * WayOutTangentSize, EndForwardVector * WayOutTangentSize), true);

	AddRouteBySpline(RouteSpline, false, true, 0, false);
	AddRouteBySpline(NearestSpline, false, false, SplineOffest + WayOutOffset, true);

	CurrentSplineKey = 0;

	return true;
}
*/

void FTrajectoryPlaner::Reset()
{
	CurrentSplineKey = 0;
	WayPoints.Reset(0);
	RouteSpline->ClearSplinePoints(true);
}

bool FTrajectoryPlaner::GoToLeftRoute(const UWorld* World, const FTransform& Transform, float Offset)
{
	if (int(CurrentSplineKey) < WayPoints.Num())
	{
		if (ANavigationRoute* OwndedRoute = WayPoints[int(CurrentSplineKey)].OwndedRoute.Get())
		{
			if (ANavigationRoute* LeftRoute = OwndedRoute->LeftRoute.Get())
			{
				return GoToRoute(World, Transform, Offset, LeftRoute);
			}
		}
	}
	return false;
}

bool FTrajectoryPlaner::GoToRightRoute(const UWorld* World, const FTransform& Transform, float Offset)
{
	if (int(CurrentSplineKey) < WayPoints.Num())
	{
		if (ANavigationRoute* OwndedRoute = WayPoints[int(CurrentSplineKey)].OwndedRoute.Get())
		{
			if (ANavigationRoute* RightRoute =OwndedRoute->RightRoute.Get())
			{
				return GoToRoute(World, Transform, Offset, RightRoute);
			}
		}
	}
	return false;
}

bool FTrajectoryPlaner::GoToRoute(const UWorld* World, const FTransform& Transform, float Offset, ANavigationRoute* Route)
{
	if (Route)
	{
		const float Key = Route->Spline->FindInputKeyClosestToWorldLocation(Transform.GetTranslation());
		const float RouteOffset = Route->Spline->GetDistanceAlongSplineAtSplineInputKey(Key) + Offset;

		if (RouteOffset <= Route->Spline->GetSplineLength())
		{
			const FTransform EndTransform = Route->Spline->GetTransformAtDistanceAlongSpline(RouteOffset, ESplineCoordinateSpace::World);
			AddRouteByTransfoms(Transform, EndTransform, Route, false, true, false);
			WayPoints.Pop();
			AddRouteBySpline(Route->Spline, RouteOffset, Route, false, false, true);
			CurrentSplineKey = 0;
			return true;
		}
	}
	return false;
}

void FTrajectoryPlaner::SetCurrentSplineKey(float Key)
{
	CurrentSplineKey = FMath::Clamp<float>(Key, 0.f, WayPoints.Num() - 1);
}

const FTrajectoryPlaner::FWayPoint* FTrajectoryPlaner::GetCurrentWayPoint() const
{
	int Ind = int(CurrentSplineKey);
	if (Ind < WayPoints.Num())
	{
		return &WayPoints[Ind];
	}
	else
	{
		return nullptr;
	}
}

FVector::FReal FTrajectoryPlaner::GetCurvatureAtKey(float Param) const
{
	// Since we need the first derivative (e.g. very similar to direction) to have its norm, we'll get the value directly
	const FVector FirstDerivative = RouteSpline->SplineCurves.Position.EvalDerivative(Param, FVector::ZeroVector);
	const FVector::FReal FirstDerivativeLength = FMath::Max(FirstDerivative.Length(), UE_DOUBLE_SMALL_NUMBER);
	const FVector ForwardVector = FirstDerivative / FirstDerivativeLength;
	const FVector SecondDerivative = RouteSpline->SplineCurves.Position.EvalSecondDerivative(Param, FVector::ZeroVector);
	// Orthogonalize the second derivative and obtain the curvature vector
	const FVector CurvatureVector = SecondDerivative - (SecondDerivative | ForwardVector) * ForwardVector;

	// Finally, the curvature is the ratio of the norms of the curvature vector over the first derivative norm
	const FVector::FReal Curvature = CurvatureVector.Length() / FirstDerivativeLength;

	// Compute sign based on sign of curvature vs. right axis
	const FVector RightVector = RouteSpline->GetRightVectorAtSplineInputKey(Param, ESplineCoordinateSpace::Local);
	return FMath::Sign(RightVector | CurvatureVector) * Curvature;
}