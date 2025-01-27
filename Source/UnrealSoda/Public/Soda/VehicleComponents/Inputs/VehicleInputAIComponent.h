// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Misc/PIDController.h"
#include "VehicleInputAIComponent.generated.h"

class ASodaWheeledVehicle;
class ANavigationRoute;

/** 
* Wheeled vehicle controller with optional AI.
*/
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputAIComponent : public UVehicleInputComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "AI controller", meta = (EditInRuntime))
	FWheeledVehicleInputState InputState{};

	/** Rate at which input throttle can rise and fall */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate ThrottleInputRate;

	/** Rate at which input brake can rise and fall */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate BrakeInputRate;

	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate SteerInputRate;

	/** Target driving speed [km/h] */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SpeedLimit = 50.0f;

	/** [0..1] */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ThrottlePedalLimit = 1.0f;

	/** Minimum travel speed for extreamly curved routes [km/h]*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SpeedSlow = 10.0f;

	//TODO: 
	/** Maximum speed to drive thru RourePlanner junctions [km/h]*/
	//UPROPERTY(Category = "Traffic Routes", EditAnywhere, SaveGame, meta = (EditInRuntime))
	//float SpeedJunction = 25.0f;

	/** Maximum breaking acceleration avalible [m/s2] */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float MaxBreakingAcceleration = 3.0;

	/** Breaking Applying Lag [s] */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float BreakingApplyingLag = 1.5f;

	/** Curvatre Smoothing Coef */
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float CurvatreSmoothingCoef = 0.4f;

	/** Slowing down in curvature coef*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SpeedToCurvatureCoef = 100.f;

	/** Slowing down by lattitude error coef*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SideErrorSpeedCoef = 0.00005f;

	/** Throttle Proportional Coef*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ThrottlePropCoef = 0.05f;

	/** Point of calculation route spline directonal error longitudal forvarding offset*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ForwardingDistanceDir = 200.0f;

	/** Point of calculation route spline lattitude error longitudal forvarding offset*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ForwardingDistanceLoc = 200.0f;

	/**  Steering calculation:  Direction Error To Location Error Ratio Coef [ from 0.0 to 1.0 ]
	(0.0 - Use only Dir error, 1.0 - use only loc error)*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SteerDirToLocErrorCoef = 0.5f;

	/** Min distance between different target points*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SamePointsDelta = 30.0f;

	/** Steering calculation: Reduce Dir error by multiplying on Coef on Low speed*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SteeringDirErrorReducingAtLowSpeedCoef = 0.33f;

	/** Speed rate coefficient. Use 0.0 to 1.0 values to slow down all driving speeds accordingly*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float Rate = 1.0f;

	/** Distance to obstacle to stop (m)*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ObstacleStopDistance = 10.0f;

	/** Consider route segment as strait if point deviations not exceeds given limit (cm)*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float RouteSectionPointMaxSideDeviation = 10.f;

	/** Steering PID controller*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FPIDController SteeringPID;

	/** Use Raycast to detect obstacles*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bUseRayCast = true;

	/** Use Actors positions to detect obstacles*/
	UPROPERTY(Category = "AI controller", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bUseActorsPositionChecking = false;

	/** Put debug output to log*/
	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDebugOutputLog = false;
	
	/** Draw current route*/
	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDrawCurrentRoute = false;

	/** Draw Debug Information*/
	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDrawTargetLocations = false;

	/** Capturing and updating the route every tick, even if the current input control is not active */
	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForceUpdateInputStates = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetBrake = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetThrottle = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetSteering = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetGear = false;

public:
	UVehicleInputAIComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual const FWheeledVehicleInputState& GetInputState() const override { return InputState; }
	virtual FWheeledVehicleInputState& GetInputState() override { return InputState; }
	virtual void UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController) override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

public:
	/** Get current speed limit in [km/h] */
	UFUNCTION(Category = "AI controller", BlueprintCallable)
	float GetSpeedLimit() const { return SpeedLimit; }

	/** Set vehicle's speed limit in [km/h] */
	UFUNCTION(Category = "AI controller", BlueprintCallable)
	void SetSpeedLimit(float InSpeedLimit) { SpeedLimit = InSpeedLimit; }

	/** Set a fixed route to follow if autopilot is enabled. */
	UFUNCTION(Category = "AI controller", BlueprintCallable)
	void SetFixedRouteByWaypoints(const TArray< FVector >& Locations, bool bNewDriveBackvard = false, bool bOverwriteCurrent = true);

	UFUNCTION(Category = "AI controller", BlueprintCallable)
	bool SetFixedRouteBySpline(const USplineComponent* SplineRoute, bool bNewDriveBackvard = false, bool bOverwriteCurrent = true, float StartOffse = 0);

	UFUNCTION(Category = "AI controller", BlueprintCallable)
	float GetSideError() const { return SideError; }

	UFUNCTION(Category = "AI controller", BlueprintCallable)
	float GetObstacleDistance() const { return CurrentObstacleDistance; }

private:
	void ResetAutopilot();
	bool TryToStartRandomRoute();
	bool TryToAppendRandomRoute();
	bool TryToFindNearestRoute();
	void UpdateInputStatesInner(float DeltaTime);
	float CalcStreeringValue(float DeltaTime); // Returns steering value.
	float CalcSpeedValue(float ObstacleDistance = -1.f); // Returns target speed value.
	float ThrottleStop(float Speed) { return ThrottleMove(Speed, 0); } // Returns throttle value.
	float ThrottleMove(float CurSpeed, float TargetSpeed); // Returns throttle value.
	float RayCast(const FVector& Start, const FVector& End);
	float CheckIntersectionWithOtherActors(const FVector& Start, const FVector& End, const TArray<AActor*> ActorsToCheck);
	float GetDistanceToObstacle();
	float GetSlowingDistance() const;
	bool IsPointInsideBox(FVector point, UBoxComponent* boxComponent) const;
	void DrawCurrentRoute();
	bool AssignRoute(const ANavigationRoute* RoutePlanner);

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

private:
	UPROPERTY()
	USplineComponent* RouteSpline;

	TArray< FVector > TargetLocations;
	float SideError = 0;
	float CurrentObstacleDistance = 0;
	bool bDriveBackvard = false;
	int32 CurrentSplineSegment = -1;

	float Throttle = 0.f;
	float Steering = 0.f;
	EGearState Gear = EGearState::Drive;
};
