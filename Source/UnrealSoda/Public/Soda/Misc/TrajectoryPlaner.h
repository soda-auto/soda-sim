// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "UObject/Class.h"
#include "Components/SplineComponent.h"
#include "TrajectoryPlaner.generated.h"

class ANavigationRoute;
class USplineComponent;
class UBoxComponent;

USTRUCT(BlueprintType, Blueprintable)
struct FTrajectoryPlaner
{
	GENERATED_USTRUCT_BODY()

public:
	/** Min distance between different target points*/
	UPROPERTY(EditAnywhere, Category = TrajectoryPlaner, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float SamePointsDelta = 30.0f;

	UPROPERTY(EditAnywhere, Category = TrajectoryPlaner, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bAutoSweep = false;

	UPROPERTY(EditAnywhere, Category = WayOutLine, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float WayOutOffset = 2000;

	UPROPERTY(EditAnywhere, Category = WayOutLine, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float WayOutTangentSize = 2000;

public:
	~FTrajectoryPlaner();

	struct FWayPoint
	{
		FWayPoint() {}
		FWayPoint(const FVector& InLocation, ANavigationRoute* InRoute = nullptr);
		FVector Location;
		TWeakObjectPtr<ANavigationRoute> OwndedRoute;
	};

	void Init(UWorld * World);
	void Deinit();

	bool AddRouteBySpline(const USplineComponent* SplineRoute, float StartOffse = 0, ANavigationRoute* OwnedRoute = nullptr, bool bNewDriveBackvard = false, bool bOverwriteCurrent = true, bool bUpdateSpline = true);
	bool AddRouteByWaypoints(const TArray< FVector >& Locations, ANavigationRoute* OwnedRoute = nullptr, bool bNewDriveBackvard = false, bool bOverwriteCurrent = true, bool bUpdateSpline = true);
	bool AddRouteByTransfoms(const FTransform& StartTransform, const FTransform& EndTransform, ANavigationRoute* OwnedRoute = nullptr, bool bNewDriveBackvard = false, bool bOverwriteCurrent = true, bool bUpdateSpline = true);
	bool UpdateTarjectory(const UWorld* World, const FTransform& Transform, float PreloadDistance, const TSet<FString> & AllowedRouteTags = TSet<FString>());
	void UpdateSpline();

	void DrawRoute(const UObject* WorldContextObject) const;

	//bool GoToNearesRoute(const UWorld* World, const FTransform& Transform);
	bool GoToLeftRoute(const UWorld* World, const FTransform& Transform, float Offset);
	bool GoToRightRoute(const UWorld* World, const FTransform& Transform, float Offset);
	bool GoToRoute(const UWorld* World, const FTransform& Transform, float Offset, ANavigationRoute* Route);

	void Reset();

	float GetDistanceAlongSpline() const { return RouteSpline->GetDistanceAlongSplineAtSplineInputKey(CurrentSplineKey); }
	void SetSplineOffset(float SplineOffset) { CurrentSplineKey = RouteSpline->SplineCurves.ReparamTable.Eval(SplineOffset, 0.0f); }

	float GetCurrentSplineKey() const { return CurrentSplineKey; }
	void SetCurrentSplineKey(float Key);

	const FWayPoint* GetCurrentWayPoint() const;
	const TArray< FWayPoint >& GetWayPoints() const { return WayPoints; }
	int GetWayPointsNum() const { return WayPoints.Num(); }
	const USplineComponent * GetRouteSpline() const { return RouteSpline; }
	bool  IsEnded() const { return (WayPoints.Num() - CurrentSplineKey) < 0.01; }

	FVector::FReal GetCurvatureAtKey(float Param) const;

public:
	/**
	 * FindNearestRoute
	 * @param MaxCodirAng      Maximum angle between tangent to spline and ForwardVector
	 * @param MaxDistance      Maximum distance between the closest point on the spline and the Location
	 * @param MaxDistanceToEnd Maximum distance between the closest point on the spline and end of the spline
	 */
	static USplineComponent* FindNearestRoute(
		const UWorld* World, 
		const FTransform& Transform,
		float MaxCodirAng = 90,
		float MaxDistance = 200,
		float MaxDistanceToEnd = 200,
		ANavigationRoute** OutRoutePlanner = nullptr,
		float* OutSplineOffest = nullptr);

	static ANavigationRoute* FindRoutePlannerOverlapedByActor(const AActor* Actor);
	static ANavigationRoute* FindRoutePlannerOverlapedByPoint(const UObject* WorldContextObject, const FVector& TargetLocation, bool bDriveBackvardFilter, const TSet<FString>& AllowedRouteTags = TSet<FString>());
	static bool IsPointInsideBox(const FVector& Point, const UBoxComponent* BoxComponent, float Border = 100);

protected:
	float CurrentSplineKey = 0;

	TArray< FWayPoint > WayPoints;
	//bool bDriveBackvard;

	UPROPERTY()
	USplineComponent * RouteSpline = nullptr;

};