// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Actors/LapCounter.h"
#include "Soda/Misc/PIDController.h"
#include <fstream>
#include <iostream>
#include "VehicleInputAIComponent.generated.h"

class ASodaWheeledVehicle;
class ANavigationRoute;
class USplineComponent;


/**
* Struct to handle sampled trajectory for high fidelity speed profile generation
*/
struct FTrajectorySampled
{


public:
	TArray<float> Curvature;
	TArray<float> Distance;
	TArray<float> Yaw;
	TArray<float> SpeedProfile;


	void ReserveArrays(int32 Size)
	{
		Curvature.Reserve(Size);
		Distance.Reserve(Size);
		Yaw.Reserve(Size);
		SpeedProfile.Reserve(Size);

	}
};

UENUM(BlueprintType)
enum class EDistanceRemapResult : uint8 {
	VE_NoAction     UMETA(DisplayName = "VE_NoAction"),
	VE_MapToPreviousSpline     UMETA(DisplayName = "VE_MapToPreviousSpline"),
	VE_MapToNextSpline     UMETA(DisplayName = "VE_MapToNextSpline"),
};

/**
* Struct to handle AI controller states
*/
struct FAIControllerState
{


public:
	float SpeedError = 0;
	float YawError = 0;
	float SideError = 0;
	float YawPredError = 0;
	float SidePredError = 0;
	float TargetCurvature = 0;
	float TargetSpeed = 0;

	float VehSpd = 0;
	float CurrentDstOnTrajectory = 0;
	// Actual fixed distance to start braking
	float FixedDistanceOfBraking = 0;
	// Fixed distance to start braking if vehicle is following speedprofile ideally
	float FixedDistanceOfBrakingNominal = 0;
	float FixedSpeedOfBraking = 0;

	bool bRouteFound = false;
	bool bRouteSearchIsDone = false;

	int32 CurrentSplineIdx = 0;
	int32 SplineIdxForFinalStop = 0;
	int32 CurrentLap = 0;
	TArray<float> LapTimePrev;
	float LapTimeCurrent = 0.0;
	bool bFinalSegment = true;
	bool bFinalBrakeIsActive = false;
	float DistanceCumulative = 0;
	float TotalDistance = 0;

	float LastTimeLapCounterTirggered = 0.0;
	float LastTimeObstacleChecked = 0;
	float DistanceToObstacle = 0;
	float RelativeVelocityToObstacle = 0;
	float LastTimeSlowdownCommanded = 0;
	float DistanceError = 0;

	bool SpeedControlIsActive = false;
	bool bSafeStopTriggered = false;

	bool AITaskIsDone = false;

};

/**
* Struct to work with an obstacle
*/

struct FDetectedObstacle
{


public:
	UPROPERTY()
	TWeakObjectPtr<AActor> ObstacleAsActor;
	UPROPERTY()
	TWeakObjectPtr<AActor> ObstacleAsActorPrev;

	FVector ObstaclePosition;
	FVector ObstacleVelocity;

	bool bObstaclePresented = false;

	void UpdateObstacleData()
	{
		if (ObstacleAsActor.IsValid() && bObstaclePresented)
		{
			ObstaclePosition = ObstacleAsActor->GetActorLocation();
			ObstacleVelocity = ObstacleAsActor->GetVelocity();
			ObstacleAsActorPrev = ObstacleAsActor;
		}
	}

	void DetectStaticObstacle(FVector StaticPosn)
	{
		ObstacleAsActor = nullptr;
		bObstaclePresented = true;
		ObstaclePosition = StaticPosn;
		ObstacleVelocity = FVector(0, 0, 0);
	}

	float GetDistanceAlongTheTrajectory(const ASodaWheeledVehicle* OwnerVehicle);
	float GetRelativeVelocity(const ASodaWheeledVehicle* OwnerVehicle);
	float GetObstacleVelocity()
	{
		return ObstacleVelocity.Size();
	}

	void ResetObstacle()
	{
		ObstacleAsActor = nullptr;
		ObstaclePosition = FVector(0, 0, 0);
		ObstacleVelocity = FVector(0, 0, 0);
		bObstaclePresented = false;
	}

	bool CheckIsItNewOne()
	{
		if (ObstacleAsActorPrev != nullptr && ObstacleAsActor != nullptr)
		{
			if (ObstacleAsActor != ObstacleAsActorPrev)
			{
				return true;
			}
		}

		return false;

	}

};


USTRUCT(BlueprintType)
struct UNREALSODA_API FSpeedProfilePrms
{
	GENERATED_USTRUCT_BODY()

	/** Speedprofile generation, longitudinal acceleration limit */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float SpdProfAMax = 2;

	/** Speedprofile generation, lateral acceleration limit  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float SpdProfALatLim = 4;

	/** Speedprofile generation, longitudinal acceleration limit  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float SpdProfADece = 8;

	/** Speedprofile generation, general scaling factor  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.01"))
	float SpdProfMuDes = 1;

	/** Speedprofile generation, speed limit in kph */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float SpdProfVLgtMax = 100;

};


/**
* Wheeled vehicle controller with optional AI.
*/
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleInputAIComponent : public UVehicleInputComponent, public ILapCounterTriggeredComponent
{
	GENERATED_BODY()

public:




	UPROPERTY(EditAnywhere, Category = "General", meta = (EditInRuntime))
	FWheeledVehicleInputState InputState{};

	/** Rate at which input throttle can rise and fall */
	UPROPERTY(Category = "General", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate ThrottleInputRate;

	/** Rate at which input brake can rise and fall */
	UPROPERTY(Category = "General", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate BrakeInputRate;

	/** Rate at which input steering can rise and fall */
	UPROPERTY(Category = "General", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FInputRate SteerInputRate;





	/** True to perform simple collision detection along the route
	This will trigger a simplified version of Adaptive Cruise Control, that is not covering extereme cases
	Collision detection assuming that the obstacle is moving along the same route for simplicity
	Collision detection is performed along the current chunk of the trajectory only
	*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bPerformCollisionDetection = false;

	/** Interval to trace the route for new obstacles, sec */
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float DetectionInterval = 0.1;

	/** Maximum lookahead distance for collision tracing, m*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float DetectionMaxLength = 50;

	/** Smallest step distance to perform sweep requests, m*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float DetectionFixedStepDistanceMin = 5;

	/** Largest step distance to perform sweep requests, m*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float DetectionFixedStepDistanceMax = 15;

	/** The distance to keep between vehicle and an obstacle, m*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float TargetDistanceToObstacle = 10;

	/** Additional distance to take into accound when perfoming slowdown for obstacle avoidance, m*/
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float CollisionDistanceSafetyMargin = 10;

	/** True to use curvature based sampling to reduce the number of collision sweeps requiests on the staright parts of the route */
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bCurvatureBasedSampling = true;

	/** Collision channel to trace during obstacle detection */
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	TEnumAsByte<ECollisionChannel> CollisionChannelToTrace = ECC_WorldStatic;

	/** Collision detection box offset (prevent detection of the road as an obstacle), cm */
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FVector CollisionBoxOffset = FVector(0, 0, 50);

	/** Collision detection box half extent, cm */
	UPROPERTY(Category = "CollisionDetection", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FVector CollisionBoxHalfExtent = FVector(200, 50, 25);


	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetBrake = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetThrottle = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetSteering = false;

	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bForbidSetGear = false;

	/** Set of parameters to perform a typical init to drive sequence, useful moslty in HIL scenario to prevent rapid state changes and allow for safety checks to be passed on ECUs  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "InitSequence", meta = (EditCondition = "bDoInitSequence"))
	FInitToDrivePrms InitToDrivePrms;



	/** Longitudinal channel gain  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float SpeedGain = 0.2;

	/** Longitudinal channel gain when obstacle is detected and adaptive cruise control is enabled  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float DstGain = 0.25;

	/** Maximum road wheel angle that could be produced by AI controller (will be mapped into [-1,1])  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float MaxRoadWheelAngle = 30;

	/** Lateral error gain  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float LatErrGain = 0.45;

	/** Clamp lateral error to prevent large steering output when vehicle is far from trajectory, m  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float LatErrClamp = 2.0;

	/** Yaw Rate gain (damp oscillations)  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float YawRateGain = 0.0;

	/** Yaw error gain  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float YawGain = 0.75;

	/** Curvature based FFW gain. Improves the quality of control if the trajectory is very smooth, otherwise the movement could be unstable  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float FFWGain = 0;

	/** Lookahead distance for prediction gains, m */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float PredictionDistance = 5;

	/** Predicted lateral error gain  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float PredictionGainLat = 0.05;

	/** Predicted yaw error gain  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	float PredictionGainYaw = 0;



	/** Discrete to sample trajectory, m  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.1"))
	float TrajDisc = 1.0;

	/** Set of core speed profile parameters, adding more items will trigger recalculation on each new lap (if circular route is selected)  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	TArray<FSpeedProfilePrms> SpeedProfilePrms = { FSpeedProfilePrms() };


	///** Speedprofile generation, longitudinal acceleration limit */
	//UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	//float SpdProfAMax = 2;

	///** Speedprofile generation, lateral acceleration limit  */
	//UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	//float SpdProfALatLim = 4;

	///** Speedprofile generation, longitudinal acceleration limit  */
	//UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.0"))
	//float SpdProfADece = 8;

	///** Speedprofile generation, general scaling factor  */
	//UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0.01"))
	//float SpdProfMuDes = 1;

	///** Speedprofile generation, speed limit in kph */
	//UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
	//float SpdProfVLgtMax = 100;

	/** True to output speed profile to the console when triggered */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
	bool bDebugSpeedProfile = false;

	/** True to disable speed profile generation and use VLgtMax as target speed */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bSpdProfUseOnlyVLgtMax = false;

	/** True to invert calculated curvature sign */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bInvertCurvatureSign = false;

	/** Speedprofile generation, flag to calculate speedprofile from the closest starting point of the path, false to force calculation from the beggining  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bSpdProfCalcFromClosestPoint = true;


	/** Number of samples that will be used in moving average to smooth curvature of the trajectory. Each sample = TracDisc  */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
	int32 CurvatureSmoothMovingWindow = 30;

	/** Maximum longitudinal braking acceleration to allow smooth braking of the vehicle at the end of the route  */
	UPROPERTY(Category = "SmoothStop", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), meta = (ClampMin = "0"))
	float EndOfTheRouteADece = 4;

	/** True to do a smooth braking of the vehicle at the end of the route. Set to false for circular (looped) trajectories  */
	UPROPERTY(Category = "SmoothStop", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bPerformSmoothBrakeAtTheEndOfTheRoute = true;

	/** Longitudinal velocity to stop vehicle completely in smooth braking case, kph  */
	UPROPERTY(Category = "SmoothStop", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float EndOfTheFullBrakeSpdThd = 3;


	/** Do Center+X, Center-X to get a current line segment on spline to calculate the lateral error, cm  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float ExpandOfCentralPointSpline = 100;

	/** True to enable gain scheduling, can improve stability for larger speeds  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDoGainScheduling = false;

	/** Gain multiplayer for lateral error, moving from 1 to this value according to speed breakpoints  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float GainSchedulingLatErr = 0.5;

	/** Gain multiplayer for yaw error, moving from 1 to this value according to speed breakpoints  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float GainSchedulingYawErr = 1.0;
	/** Breakpoint for gain schedluing, VLgt in kph  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float GainSchedulingVLgtStart = 30.0;

	/** Breakpoint for gain schedluing, VLgt in kph  */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float GainSchedulingVLgtEnd = 100.0;

	/** True to display points on the route that are using for control  */
	UPROPERTY(Category = "Debug", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugPrimitives = false;

	/** In case if the trajectory is looped and you want vehicle to keep moving on it   */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bCircularRoute = false;

	/** True to use external lap counter actor from the scene   */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bUseExternalLapCounter = false;

	/** Time threshold to prevent double lap counter overlap trigger   */
	UPROPERTY(Category = "SpeedProfile", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float LapCounterOverlapCheckTiThd = 2.5;

	/** Side error to force vehicle to stop, m   */
	UPROPERTY(Category = "Controllers", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SafeStopSideErrorThd = 5.0;

	/** Force to use specific route instead of the closests one  */
	UPROPERTY(EditAnywhere, Category = Connections, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRoute> ForcedRoute;

	/** True to automatically append routes that initial point is close to the final point of the previous one  */
	UPROPERTY(Category = "Routes", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bTryToAutoAppendNextRoutes = true;

	/** How many routes could be appended  */
	UPROPERTY(Category = "Routes", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	int32 MaxNumOfAutoAppendedRoutes = 10;

	/** If the distance between routes ends is smaller than thd, append, cm  */
	UPROPERTY(Category = "Routes", EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float AutoAppendNextRoutesDstThd = 50;







	// Internal states 
	FTrajectorySampled TrajectorySampled;
	FAIControllerState AIControllerState;
	FDetectedObstacle DetectedObstacle;

	bool bResetBrakeToZeroOnForbid = false;
	bool bResetThrottleToZeroOnForbid = false;
	bool bResetSteeringToZeroOnForbid = false;
	bool bSmoothResetInput = false;



	/** Triggers recalculation of the speed profile from the current point with current velocity as initial value  */
	UFUNCTION(Category = "SpeedProfile", BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	void RecalculateSpeedProfile();

	void CalculateSpeedProfile(float Mass, int32 StartPoint, float StartingSpeed = 0);
	float CalculateCurvature(const FVector& PointA, const FVector& PointB, const FVector& PointC);
	TArray<float> CalculateMovingAverage(const TArray<float>& InputArray, int32 WindowSize);
	float CalculateSideError(const FVector& VehPosn, const FVector& PosnBehindVeh, const FVector& PosnPredicted);

	bool FindRoute();
	void MonitorNextRoute();
	bool SampleTrajectory(USplineComponent* RouteToSample);
	void CalculateTrajErrors();
	void GenerateControl();
	void MonitorReadyToSmoothStop();
	void CheckSlowdownForObstacle();
	void PerformObstacleDetection();
	bool SweepObstacle(const FVector& StartLocation, const FVector& EndLocation, float RadiusM, const AActor* Owner, FDetectedObstacle& Obstacle);

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
	float GetSpeedLimit();

	/** Set vehicle's speed limit in [km/h] */
	UFUNCTION(Category = "AI controller", BlueprintCallable)
	void SetSpeedLimit(float InSpeedLimit);

	/** Get side error in cm */
	UFUNCTION(Category = "AI controller", BlueprintCallable)
	float GetSideError() const { return AIControllerState.SideError; }


	bool GetSpeedProfilePrms(FSpeedProfilePrms& SpeedProfilePrmsOut);
	FSpeedProfilePrms* GetSpeedProfilePrmsPrt();


	void OnLapCounterTriggerBeginOverlap(ALapCounter* LapCounter, const FHitResult& SweepResult) override;

private:


	/** Perform main control sequence */
	void UpdateInputStatesInner(float DeltaTime);

	/** Perform init to drive sequence if required */
	void DoInitToDriveSequence(float DeltaTime);


	float WrapDistanceToSpline(float CurrentDistance, float StepToMove, float LengthOfPreviousSpline, float LengthOfCurrentSpline, float LengthOfNextSpline, EDistanceRemapResult& RemapResult);



protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;


private:



	UPROPERTY()
	USplineComponent* RouteSpline;
	UPROPERTY()
	USplineComponent* RouteSplinePrev;
	UPROPERTY()
	USplineComponent* RouteSplineNext;

	UPROPERTY()
	TArray<USplineComponent*> RouteSplines;

	UPROPERTY()
	TArray<ANavigationRoute*> ProcessedRoutes;



	bool bSimulationStarted = false;
	int32 CurrentSplineSegment = -1;
	int32 CurrentSplineSegmentPred = -1;
	int32 CurrentSplineSegmentCollisionDetn = -1;

	float Throttle = 0.f;
	float Steering = 0.f;
	EGearState Gear = EGearState::Drive;


	std::ofstream CsvOutFile;
	bool bRecordingOk = false;
	FString CvsFileNameBase = "SpeedProfileLog";
	int CvsFileNameIndex = 0;
	float LastRecordTimeStamp = 0.0;

	void LogSimulationStart();
	void LogSimulationComplete();
	void LogSimulation();


};
