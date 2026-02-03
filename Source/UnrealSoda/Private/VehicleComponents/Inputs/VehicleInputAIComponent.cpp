// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

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
#include "Engine/Engine.h"
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

	bSimulationStarted = true;

	return true;
}

void UVehicleInputAIComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	bSimulationStarted = false;
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


}

void UVehicleInputAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsValid(RouteSpline))
	{
		RouteSpline->DestroyComponent();
	}
}



bool UVehicleInputAIComponent::FindRoute()
{
	USplineComponent* NearestSpline = nullptr;
	float NearestDistance = TNumericLimits< float >::Max();
	float NearestSplineKey = 0;
	float NearestSplineKeyDistance = 0;

	FVector VehicleLocation = GetVehicle()->GetActorLocation();
	FVector VehicleForwardVector = GetVehicle()->GetActorForwardVector();

	ANavigationRoute* NearestSplineRoute = nullptr;

	// Use forced route selected in user interface or perform a search

	if (ForcedRoute.IsValid())
	{
		NearestSplineRoute = ForcedRoute.Get();
		NearestSpline = NearestSplineRoute->Spline;
	}
	else
	{


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
						NearestSplineRoute = (*It);
					}
				}
			}
		}
	}



	AIControllerState.bRouteSearchIsDone = true;

	if (NearestSpline)
	{
		ProcessedRoutes.Add(NearestSplineRoute);
		RouteSpline = NearestSpline;
		MonitorNextRoute();

		return SampleTrajectory(NearestSpline);

	}
	return false;
}


float UVehicleInputAIComponent::GetSpeedLimit()
{
	FSpeedProfilePrms SpdProf;

	bool bSuccess = GetSpeedProfilePrms(SpdProf);

	if (bSuccess)
	{
		return SpdProf.SpdProfVLgtMax;
	}

	return -1.0;

}

void UVehicleInputAIComponent::SetSpeedLimit(float InSpeedLimit)
{
	FSpeedProfilePrms* CurrentSpdProf = GetSpeedProfilePrmsPrt();

	if (CurrentSpdProf)
	{
		CurrentSpdProf->SpdProfVLgtMax = InSpeedLimit;
		RecalculateSpeedProfile();
	}


}

FSpeedProfilePrms* UVehicleInputAIComponent::GetSpeedProfilePrmsPrt()
{
	if (SpeedProfilePrms.IsValidIndex(AIControllerState.CurrentLap))
	{
		return &SpeedProfilePrms[AIControllerState.CurrentLap];
	}
	else if (SpeedProfilePrms.Num() > 0)
	{
		int32 LastIdx = SpeedProfilePrms.Num() - 1;
		return &SpeedProfilePrms[LastIdx];
	}
	else
	{
		// No speed profile at all
		UE_LOG(LogTemp, Error, TEXT("No elements in speed profile!"));
		return nullptr;
	}



}

void UVehicleInputAIComponent::OnLapCounterTriggerBeginOverlap(ALapCounter* LapCounter, const FHitResult& SweepResult)
{
	if (bSimulationStarted && bUseExternalLapCounter && GetWorld()->GetTimeSeconds() - AIControllerState.LastTimeLapCounterTirggered > LapCounterOverlapCheckTiThd)
	{
		AIControllerState.LastTimeLapCounterTirggered = GetWorld()->GetTimeSeconds();
		AIControllerState.CurrentLap++;
		AIControllerState.LapTimePrev.Add(AIControllerState.LapTimeCurrent);
		AIControllerState.LapTimeCurrent = 0.0;
		RecalculateSpeedProfile();
		//CalculateSpeedProfile(GetWheeledVehicle()->GetWheeledComponentInterface()->GetVehicleMass(), 0, AIControllerState.VehSpd);
	}

}


bool UVehicleInputAIComponent::GetSpeedProfilePrms(FSpeedProfilePrms& SpeedProfilePrmsOut)
{
	if (SpeedProfilePrms.IsValidIndex(AIControllerState.CurrentLap))
	{
		SpeedProfilePrmsOut = SpeedProfilePrms[AIControllerState.CurrentLap];
		return true;

	}
	else if (SpeedProfilePrms.Num() > 0)
	{
		int32 LastIdx = SpeedProfilePrms.Num() - 1;
		SpeedProfilePrmsOut = SpeedProfilePrms[LastIdx];
		return true;
	}
	else
	{
		// No speed profile at all
		UE_LOG(LogTemp, Error, TEXT("No elements in speed profile!"));
		return false;
	}


}

void UVehicleInputAIComponent::UpdateInputStatesInner(float DeltaTime)
{
	if (!GetWheeledVehicle()->GetWheeledComponentInterface()) return;

	if (!AIControllerState.bRouteSearchIsDone)
	{
		AIControllerState.bRouteFound = FindRoute();
	}

	if (!AIControllerState.bRouteFound)
	{
		return;
	}



	CalculateTrajErrors();
	GenerateControl();

}


bool UVehicleInputAIComponent::SampleTrajectory(USplineComponent* RouteSplineToSample)
{
	if (!RouteSplineToSample || RouteSplineToSample->GetNumberOfSplinePoints() < 2)
	{
		return false;
	}



	RouteSpline = RouteSplineToSample;

	FVector VehicleLocation = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation()
		+ GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.Rotator().RotateVector(FVector(1.151 * 100, 0, 0));


	float ClosestDstToSampledSpline = TNumericLimits<float>::Max();
	int32 ClosestIdxToSampledSpline = -1;
	float GlobalDistance = 0;
	int32 SplineIdx = 0;

	for (auto& Route : RouteSplines)
	{



		float PositionStep = TrajDisc;
		float SplineLength = Route->GetSplineLength() / 100;

		int32 NumberOfPoints = FMath::CeilToInt32(SplineLength / PositionStep);

		//UE_LOG(LogTemp, Warning, TEXT("Checking new spline, num of points %d"), NumberOfPoints);

		for (int k = 0; k < NumberOfPoints; k++)
		{

			float Distance = (float)k * PositionStep;
			float StepSize = PositionStep / 2;

			if (Distance > SplineLength)
			{
				break;
			}

			if (!bCircularRoute && Distance + StepSize > SplineLength)
			{
				break;
			}

			float PointBeforeAlongSpline = (Distance - StepSize * 2);
			float PointAfterAlongSpline = (Distance + StepSize * 2);

			if (bCircularRoute)
			{
				// We are sampling the end of the trajectory in case if the distance is negative

				if (Distance - StepSize * 2 < 0)
				{
					//UE_LOG(LogTemp, Warning, TEXT("Adjust position from %f to %f, spline length %f"), PointBeforeAlongSpline, SplineLength + (Distance - StepSize * 2), SplineLength);
					PointBeforeAlongSpline = SplineLength + (Distance - StepSize * 2);

				}

				if (Distance + StepSize * 2 > SplineLength)
				{
					PointAfterAlongSpline = SplineLength - (Distance + StepSize * 2);
				}

			}

			FVector PointBefore = Route->GetLocationAtDistanceAlongSpline(PointBeforeAlongSpline * 100, ESplineCoordinateSpace::World);
			FVector PointMiddle = Route->GetLocationAtDistanceAlongSpline((Distance) * 100, ESplineCoordinateSpace::World);
			FVector PointAfter = Route->GetLocationAtDistanceAlongSpline(PointAfterAlongSpline * 100, ESplineCoordinateSpace::World);

			/*	if (k == 0)
				{
					DrawDebugPoint(GetWorld(), PointBefore, 15, FColor::Green, true);
					DrawDebugPoint(GetWorld(), PointMiddle, 15, FColor::Yellow, true);
					DrawDebugPoint(GetWorld(), PointAfter, 15, FColor::Red, true);
				}*/

			float Dst = (PointMiddle - VehicleLocation).Size();
			if (Dst < ClosestDstToSampledSpline)
			{
				ClosestDstToSampledSpline = Dst;
				ClosestIdxToSampledSpline = k;
			}

			float Curvature = CalculateCurvature(PointBefore / 100, PointMiddle / 100, PointAfter / 100);


			// Get the tangent at the given distance
			FVector Tangent = Route->GetTangentAtDistanceAlongSpline(Distance * 100, ESplineCoordinateSpace::World).GetSafeNormal();

			// Project the tangent onto the X-Y plane
			FVector2D TangentXY(Tangent.X, Tangent.Y);

			// Calculate the yaw angle in degrees
			float Yaw = FMath::Atan2(TangentXY.Y, TangentXY.X) * 180.0f / PI;



			TrajectorySampled.Curvature.Add(Curvature);
			TrajectorySampled.Distance.Add(GlobalDistance + Distance);
			//UE_LOG(LogTemp, Warning, TEXT("Resulting %d add value %f, global %f, distance local %f"), SplineIdx,GlobalDistance + Distance,GlobalDistance, Distance);
			TrajectorySampled.SpeedProfile.Add(0.0);
			TrajectorySampled.Yaw.Add(Yaw);






		}
		GlobalDistance = GlobalDistance + SplineLength;
		//UE_LOG(LogTemp, Warning, TEXT("Update of the global distance is done %f"), GlobalDistance);
		SplineIdx++;
	}

	int32 ww = 0;

	//UE_LOG(LogTemp, Warning, TEXT("Length of sampled traj %d, closest idx %d"), TrajectorySampled.Curvature.Num(), ClosestIdxToSampledSpline);

	//for (auto& Elem : TrajectorySampled.Curvature)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Full traj %d before curv %f"), TrajectorySampled.Curvature.Num(),TrajectorySampled.Curvature[ww]);
	//	ww++;
	//}

	// Calculate smoothed curvature
	TrajectorySampled.Curvature = CalculateMovingAverage(TrajectorySampled.Curvature, CurvatureSmoothMovingWindow);

	// Calculate initial speedprofile
	CalculateSpeedProfile(GetWheeledVehicle()->GetWheeledComponentInterface()->GetVehicleMass(), FMath::Max(0, (bSpdProfCalcFromClosestPoint ? (ClosestIdxToSampledSpline - 1) : 0)));


	AIControllerState.TotalDistance = TrajectorySampled.Distance[TrajectorySampled.Distance.Num() - 1];




	ww = 0;


	/*for (auto& Elem : TrajectorySampled.Curvature)
	{
		UE_LOG(LogTemp, Warning, TEXT("Full traj %d after curv %f"), TrajectorySampled.Curvature.Num(),TrajectorySampled.Curvature[ww]);
		ww++;
	}*/


	return true;
}


void UVehicleInputAIComponent::CalculateTrajErrors()
{
	// Find closest point of the vehicle COG and generate line segment with a center in closest point

	FVector CurVehLoc = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation()
		+ GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.Rotator().RotateVector(FVector(1.151 * 100, 0, 0));


	float Now = GetWorld()->GetTimeSeconds();


	AIControllerState.LapTimeCurrent = AIControllerState.LapTimeCurrent + GetWorld()->GetDeltaSeconds();




	float VehSplineKey = USodaStatics::FindSplineInputKeyClosestToWorldLocationFastLimited(CurVehLoc, CurrentSplineSegment, RouteSpline, 2, 1);
	float KeyRemaped = LinInterp1(ExpandOfCentralPointSpline, 0, RouteSpline->GetSplineLength(), 0, 1);



	//UE_LOG(LogTemp, Warning, TEXT("CurVehLoc = %s, spline key %f, segment %d"), *CurVehLoc.ToString(), VehSplineKey, CurrentSplineSegment);

	float CurrentDistance = RouteSpline->GetDistanceAlongSplineAtSplineInputKey(VehSplineKey);

	// switch to the next spline if the current one is finished and there is the next one
	if (CurrentDistance >= RouteSpline->GetSplineLength())
	{

		bool bSwitchIsDone = false;
		UE_LOG(LogTemp, Warning, TEXT("route is circular %d segment is final %d"), bCircularRoute, AIControllerState.bFinalSegment);

		if (bCircularRoute && AIControllerState.bFinalSegment)
		{
			AIControllerState.CurrentSplineIdx = 0;

			if (!bCircularRoute)
			{
				AIControllerState.bFinalSegment = false;
			}

			AIControllerState.DistanceCumulative = 0;




			CurrentSplineSegment = 0;
			CurrentSplineSegmentPred = 0;

			RouteSpline = RouteSplines[AIControllerState.CurrentSplineIdx];

			VehSplineKey = RouteSpline->FindInputKeyClosestToWorldLocation(CurVehLoc);
			KeyRemaped = LinInterp1(ExpandOfCentralPointSpline, 0, RouteSpline->GetSplineLength(), 0, 1);
			CurrentDistance = RouteSpline->GetDistanceAlongSplineAtSplineInputKey(VehSplineKey);

			if (!bUseExternalLapCounter)
			{
				AIControllerState.CurrentLap++;
				AIControllerState.LapTimePrev.Add(AIControllerState.LapTimeCurrent);
				AIControllerState.LapTimeCurrent = 0.0;

			}

			//RecalculateSpeedProfile();
			CalculateSpeedProfile(GetWheeledVehicle()->GetWheeledComponentInterface()->GetVehicleMass(), 0, AIControllerState.VehSpd);

			UE_LOG(LogTemp, Warning, TEXT("Switch to the FIRST route was done, distance %f, current %f "), AIControllerState.DistanceCumulative, CurrentDistance / 100);

			bSwitchIsDone = true;
		}


		if (!AIControllerState.bFinalSegment && !bSwitchIsDone)
		{
			AIControllerState.CurrentSplineIdx = FMath::Min(AIControllerState.CurrentSplineIdx + 1, RouteSplines.Num() - 1);
			if (AIControllerState.CurrentSplineIdx == RouteSplines.Num() - 1)
			{
				AIControllerState.bFinalSegment = true;
			}

			float LengthOfPrevSpline = RouteSpline->GetSplineLength() / 100;
			RouteSplinePrev = RouteSpline;
			RouteSpline = RouteSplines[AIControllerState.CurrentSplineIdx];

			if (RouteSplines.IsValidIndex(AIControllerState.CurrentSplineIdx + 1))
			{
				RouteSplineNext = RouteSplines[AIControllerState.CurrentSplineIdx + 1];
			}



			CurrentSplineSegment = 0;
			CurrentSplineSegmentPred = 0;
			AIControllerState.DistanceCumulative = AIControllerState.DistanceCumulative + LengthOfPrevSpline;
			VehSplineKey = USodaStatics::FindSplineInputKeyClosestToWorldLocationFastLimited(CurVehLoc, CurrentSplineSegment, RouteSpline, 2, 1);
			KeyRemaped = LinInterp1(ExpandOfCentralPointSpline, 0, RouteSpline->GetSplineLength(), 0, 1);
			CurrentDistance = RouteSpline->GetDistanceAlongSplineAtSplineInputKey(VehSplineKey);

			UE_LOG(LogTemp, Warning, TEXT("Switch to the next route was done, distance cumulative %f, current %f "), AIControllerState.DistanceCumulative, CurrentDistance / 100);
		}



	}

	if (Now - AIControllerState.LastTimeObstacleChecked > DetectionInterval)
	{
		AIControllerState.LastTimeObstacleChecked = Now;
		PerformObstacleDetection();

	}

	CheckSlowdownForObstacle();


	EDistanceRemapResult RemapLookBack;
	EDistanceRemapResult RemapLookForward;
	//EDistanceRemapResult RemapLookBackPrev;
	//EDistanceRemapResult RemapLookForwardPrev;


	float CurrentDistanceSampled = AIControllerState.DistanceCumulative + CurrentDistance / 100;

	float LengthOfTheSpline = RouteSpline->GetSplineLength();
	float DstToLookBack = CurrentDistance - ExpandOfCentralPointSpline / 2;
	float DstToLookAhead = CurrentDistance + ExpandOfCentralPointSpline / 2;




	if (bCircularRoute)
	{

		DstToLookBack = WrapDistanceToSpline(CurrentDistance, -ExpandOfCentralPointSpline / 2.0, LengthOfTheSpline, LengthOfTheSpline, LengthOfTheSpline, RemapLookBack);
		DstToLookAhead = WrapDistanceToSpline(CurrentDistance, ExpandOfCentralPointSpline / 2.0, LengthOfTheSpline, LengthOfTheSpline, LengthOfTheSpline, RemapLookForward);



		//if (DstToLookBack < 0.0)
		//{			
		//	DstToLookBack = LengthOfTheSpline - (ExpandOfCentralPointSpline / 2 - CurrentDistance);
		//	UE_LOG(LogTemp, Warning, TEXT("Remap Lookback distance from %f to %f"), CurrentDistance - ExpandOfCentralPointSpline / 2, DstToLookBack);
		//}
		//
		//
		//if (DstToLookAhead > LengthOfTheSpline)
		//{
		//	DstToLookAhead = CurrentDistance + ExpandOfCentralPointSpline / 2 - LengthOfTheSpline;
		//	UE_LOG(LogTemp, Warning, TEXT("Remap Lookahead distance from %f to %f"), CurrentDistance + ExpandOfCentralPointSpline / 2, DstToLookAhead);
		//}


	}


	// Dst now 10
	// Lookahed = 100
	// Maxlength = 2000
	// we have to get 1910
	// 1910 = 2000 - (100-10)


	//float DstToLookBackPred = CurrentDistance - ExpandOfCentralPointSpline / 2;
	//float DstToLookAheadPred = CurrentDistance + ExpandOfCentralPointSpline / 2;


	FVector PosnBehindVehicle = RouteSpline->GetWorldLocationAtDistanceAlongSpline(DstToLookBack);
	FVector PosnInFrontOfVehicle = RouteSpline->GetWorldLocationAtDistanceAlongSpline(DstToLookAhead);

	/*FVector PosnBehindVehicle = RouteSpline->GetLocationAtSplineInputKey(VehSplineKey - KeyRemaped, ESplineCoordinateSpace::World);
	FVector PosnInFrontOfVehicle = RouteSpline->GetLocationAtSplineInputKey(VehSplineKey + KeyRemaped, ESplineCoordinateSpace::World);*/





	float TargetYaw = LookupTable1d(CurrentDistanceSampled, TrajectorySampled.Distance, TrajectorySampled.Yaw);




	// Prediction term
	const FVector ForwardNew = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetRotation().Rotator().Vector();

	FVector PredictedLocation = CurVehLoc + PredictionDistance * ForwardNew * 100;





	float PredPosnKey = RouteSpline->FindInputKeyClosestToWorldLocation(PredictedLocation);// USodaStatics::FindSplineInputKeyClosestToWorldLocationFastLimited(PredictedLocation, CurrentSplineSegmentPred, RouteSpline, 2, 1);


	//FVector PredictedLocationOnSpline = RouteSpline->FindLocationClosestToWorldLocation(ForwardNew, ESplineCoordinateSpace::World);
	float DistanceForPredictedLocation = RouteSpline->GetDistanceAlongSplineAtSplineInputKey(PredPosnKey);

	float CurrentDistancePredictedSampled = AIControllerState.DistanceCumulative + DistanceForPredictedLocation / 100;

	FVector PredPosnBehindVehicle = RouteSpline->GetLocationAtSplineInputKey(PredPosnKey - KeyRemaped, ESplineCoordinateSpace::World);
	FVector PredPosnInFrontOfVehicle = RouteSpline->GetLocationAtSplineInputKey(PredPosnKey + KeyRemaped, ESplineCoordinateSpace::World);







	float TargetYawPred = LookupTable1d(CurrentDistancePredictedSampled, TrajectorySampled.Distance, TrajectorySampled.Yaw);


	// Speed term

	float TargetSpeed = 0.0;

	if (AIControllerState.bFinalBrakeIsActive)
	{
		TargetSpeed = LinInterp1(CurrentDistanceSampled, AIControllerState.FixedDistanceOfBraking, AIControllerState.TotalDistance, AIControllerState.FixedSpeedOfBraking, 0);
	}
	else
	{
		TargetSpeed = LookupTable1d(CurrentDistanceSampled, TrajectorySampled.Distance, TrajectorySampled.SpeedProfile) * 3.6;
	}



	// Calculate errors for controller and fill up the state

	float Yaw = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.Rotator().Yaw;
	float VehSpeed = Chaos::CmSToKmH(GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X);

	AIControllerState.SideError = CalculateSideError(CurVehLoc, PosnBehindVehicle, PosnInFrontOfVehicle);
	AIControllerState.SidePredError = CalculateSideError(PredictedLocation, PredPosnBehindVehicle, PredPosnInFrontOfVehicle);
	AIControllerState.YawError = FMath::FindDeltaAngleRadians(Yaw / 180 * PI, TargetYaw / 180 * PI);
	AIControllerState.YawPredError = FMath::FindDeltaAngleRadians(Yaw / 180 * PI, TargetYawPred / 180 * PI);
	AIControllerState.TargetSpeed = TargetSpeed;
	AIControllerState.VehSpd = VehSpeed;
	AIControllerState.TargetCurvature = LookupTable1d(CurrentDistanceSampled, TrajectorySampled.Distance, TrajectorySampled.Curvature);
	AIControllerState.CurrentDstOnTrajectory = CurrentDistanceSampled * 100;
	AIControllerState.DistanceToObstacle = DetectedObstacle.GetDistanceAlongTheTrajectory(GetWheeledVehicle());
	AIControllerState.RelativeVelocityToObstacle = DetectedObstacle.GetRelativeVelocity(GetWheeledVehicle());

	//UE_LOG(LogTemp, Warning, TEXT("Side error %f, target speed %f, target curv %f, current dst on traj %f, key remaped %f, veh key %f, veh pred key %f"), 
	//	AIControllerState.SideError, AIControllerState.TargetSpeed, AIControllerState.TargetCurvature, AIControllerState.CurrentDstOnTrajectory,
	//	KeyRemaped,VehSplineKey,PredPosnKey);

	MonitorReadyToSmoothStop();


	if (bDrawDebugPrimitives)
	{
		DrawDebugPoint(GetWorld(), PosnBehindVehicle, 30.0, FColor::Red, false, 0.01, 15);
		DrawDebugPoint(GetWorld(), PosnInFrontOfVehicle, 30.0, FColor::Green, false, 0.01, 15);
		DrawDebugPoint(GetWorld(), CurVehLoc, 30.0, FColor::Black, false, 0.01, 15);

		DrawDebugPoint(GetWorld(), PredictedLocation, 30.0, FColor::Cyan, false, 0.0, 0);
		DrawDebugPoint(GetWorld(), PredPosnBehindVehicle, 30.0, FColor::Green, false, 0.0, 0);
		DrawDebugPoint(GetWorld(), PredPosnInFrontOfVehicle, 30.0, FColor::Red, false, 0.0, 0);
	}






}

void UVehicleInputAIComponent::MonitorNextRoute()
{



	RouteSplines.Add(RouteSpline);


	if (bTryToAutoAppendNextRoutes)
	{
		bool bSearchSuccesfull = true;
		int32 NumberOfAttempts = 0;

		while (NumberOfAttempts < MaxNumOfAutoAppendedRoutes && bSearchSuccesfull)
		{
			float MaxDst = 10000;



			USplineComponent* CurrentSpline = RouteSplines[RouteSplines.Num() - 1];
			FVector FinalPositionCurrentSpline = CurrentSpline->GetLocationAtSplinePoint(CurrentSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

			ANavigationRoute* NextRouteCandidate = nullptr;

			bSearchSuccesfull = false;

			for (TActorIterator< ANavigationRoute > It(GetWorld()); It; ++It)
			{
				// If spline is not already in sequence and correct, compare it's first point with latest spline last point

				if (!ProcessedRoutes.Contains(*It) && (*It)->bAllowForVehicles && (*It)->Spline->GetNumberOfSplinePoints() > 1)
				{

					FVector StartPositionNewSpline = (*It)->Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
					float EndAndStartDistance = (StartPositionNewSpline - FinalPositionCurrentSpline).Size();

					if (EndAndStartDistance < AutoAppendNextRoutesDstThd && EndAndStartDistance < MaxDst)
					{
						MaxDst = EndAndStartDistance;
						NextRouteCandidate = *It;
						bSearchSuccesfull = true;
					}
				}
			}

			NumberOfAttempts++;

			if (bSearchSuccesfull)
			{
				RouteSplines.Add((NextRouteCandidate->Spline));
				ProcessedRoutes.Add(NextRouteCandidate);
			}

		}

	}


	if (RouteSplines.Num() > 1)
	{
		AIControllerState.bFinalSegment = false;
	}



}

void UVehicleInputAIComponent::GenerateControl()
{

	// Longitudinal control


	AIControllerState.SpeedError = (AIControllerState.TargetSpeed - AIControllerState.VehSpd);
	AIControllerState.DistanceError = 0.0;

	if (AIControllerState.SpeedControlIsActive)
	{
		// Trivial ACC controller	

		DetectedObstacle.UpdateObstacleData();

		float RelVel = -DetectedObstacle.GetRelativeVelocity(GetWheeledVehicle());
		float RelDst = DetectedObstacle.GetDistanceAlongTheTrajectory(GetWheeledVehicle()) / 100 - TargetDistanceToObstacle;

		Throttle = RelVel * SpeedGain + RelDst * DstGain;

	}
	else
	{
		Throttle = AIControllerState.SpeedError * SpeedGain;
	}


	if (AIControllerState.bFinalBrakeIsActive)
	{
		if (AIControllerState.TargetSpeed < EndOfTheFullBrakeSpdThd)
		{
			Throttle = -1;
		}
	}

	Gear = EGearState::Drive;




	// Lateral control

	float LatErrGainScheduling = 1.0;
	float YawErrGainScheduling = 1.0;

	if (bDoGainScheduling)
	{
		LatErrGainScheduling = LinInterp1(AIControllerState.VehSpd, GainSchedulingVLgtStart, GainSchedulingVLgtEnd, 1.0, GainSchedulingLatErr);
		YawErrGainScheduling = LinInterp1(AIControllerState.VehSpd, GainSchedulingVLgtStart, GainSchedulingVLgtEnd, 1.0, GainSchedulingYawErr);
	}

	float WhlBase = 1.597 + 1.151;
	float RoadWhlAngFeedforward = AIControllerState.TargetCurvature * WhlBase * 180 / PI;
	float YawRate = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.AngularVelocity.Z;

	float PredictionRWA = PredictionGainLat * AIControllerState.SidePredError + PredictionGainYaw * AIControllerState.YawPredError;


	// Remap road wheel angle into -1..1

	Steering = LinInterp1(RoadWhlAngFeedforward * FFWGain + FMath::Clamp(AIControllerState.SideError, -LatErrClamp * 100, LatErrClamp * 100) * LatErrGain * LatErrGainScheduling + AIControllerState.YawError * YawGain * YawErrGainScheduling + YawRate * YawRateGain + PredictionRWA, -MaxRoadWheelAngle, MaxRoadWheelAngle, -1, 1);

}


void UVehicleInputAIComponent::MonitorReadyToSmoothStop()
{
	bool bSafeStopCondition = FMath::Abs(AIControllerState.SideError) > SafeStopSideErrorThd * 100;



	if (AIControllerState.bFinalSegment || AIControllerState.CurrentDstOnTrajectory / 100 > AIControllerState.FixedDistanceOfBrakingNominal || bSafeStopCondition)
	{
		// Perform comparison of predicted stop distance with the distance left to travel along the trajectory to initiate smooth braking with maximum allowed deceleration

		float StoppingDistance = (FMath::Square(AIControllerState.VehSpd / 3.6)) / (2.0f * EndOfTheRouteADece);

		if ((AIControllerState.CurrentDstOnTrajectory / 100 + StoppingDistance > AIControllerState.TotalDistance && !AIControllerState.bFinalBrakeIsActive && bPerformSmoothBrakeAtTheEndOfTheRoute)
			|| bSafeStopCondition)
		{
			AIControllerState.bFinalBrakeIsActive = true;

			AIControllerState.FixedDistanceOfBraking = AIControllerState.CurrentDstOnTrajectory / 100;
			AIControllerState.FixedSpeedOfBraking = AIControllerState.VehSpd;
		}


		if (bSafeStopCondition)
		{
			AIControllerState.bSafeStopTriggered = true;

		}
	}


	if (AIControllerState.bFinalBrakeIsActive && AIControllerState.VehSpd < 1.0)
	{
		AIControllerState.AITaskIsDone = true;
	}
}








void UVehicleInputAIComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	check(GetWheeledVehicle() != nullptr);

	SCOPE_CYCLE_COUNTER(STAT_AI_TickComponent);

	if (InitToDrivePrms.bDoInitToDriveSequence && !InitToDrivePrms.bInitSequenceIsDone)
	{
		DoInitToDriveSequence(DeltaTime);
	}
	else
	{
		UpdateInputStatesInner(DeltaTime);
	}



	if (!bForbidSetBrake)
	{
		InputState.Brake = BrakeInputRate.InterpInputValue(DeltaTime, InputState.Brake, FMath::Clamp(-1.f * Throttle, 0.f, 1.f));
	}
	else if (bForbidSetBrake && bResetBrakeToZeroOnForbid)
	{
		if (bSmoothResetInput)
		{
			InputState.Brake = BrakeInputRate.InterpInputValue(DeltaTime, InputState.Brake, 0.0);
		}
		else
		{
			InputState.Brake = 0.0;
		}

	}

	if (!bForbidSetThrottle)
	{
		InputState.Throttle = ThrottleInputRate.InterpInputValue(DeltaTime, InputState.Throttle, FMath::Clamp(Throttle, 0.f, 1.f));
	}
	else if (bForbidSetThrottle && bResetThrottleToZeroOnForbid)
	{
		if (bSmoothResetInput)
		{
			InputState.Throttle = ThrottleInputRate.InterpInputValue(DeltaTime, InputState.Throttle, 0.0);
		}
		else
		{
			InputState.Throttle = 0.0;
		}
	}

	if (!bForbidSetSteering)
	{
		InputState.Steering = SteerInputRate.InterpInputValue(DeltaTime, InputState.Steering, Steering);
	}
	else if (bForbidSetSteering && bResetSteeringToZeroOnForbid)
	{
		if (bSmoothResetInput)
		{
			InputState.Steering = SteerInputRate.InterpInputValue(DeltaTime, InputState.Steering, 0.0);
		}
		else
		{
			InputState.Steering = 0.0;
		}
	}


	if (!bForbidSetGear) InputState.SetGearState(Gear);
}




void UVehicleInputAIComponent::DoInitToDriveSequence(float DeltaTime)
{
	// Performs init to drive sequence that could be important when running the simulation in HiL with real hardware

	FInitToDrivePrms& Prm = InitToDrivePrms;

	Prm.InitSequenceLocalTime = Prm.InitSequenceLocalTime + DeltaTime;


	float TimeToEndBrakePress = Prm.TimeToStayInPark + Prm.TimeToPressBrakePedal;
	float TimeToPressToDrive = TimeToEndBrakePress + Prm.TimeToWaitBeforeSwitchToDrive;
	float TimeToStartBrakeRelease = TimeToPressToDrive + Prm.TimeToStayInDriveWithPedalPressed;
	float TimeToFinishInit = TimeToStartBrakeRelease + Prm.TimeToReleaseBrakePedal;


	if (Prm.InitSequenceLocalTime < TimeToEndBrakePress)
	{
		Prm.CurPedlPosn = LinInterp1(Prm.InitSequenceLocalTime, Prm.TimeToStayInPark, TimeToEndBrakePress, 0, 1);
	}
	else if (Prm.InitSequenceLocalTime > TimeToStartBrakeRelease)
	{
		Prm.CurPedlPosn = LinInterp1(Prm.InitSequenceLocalTime, TimeToStartBrakeRelease, TimeToStartBrakeRelease + Prm.TimeToReleaseBrakePedal, 1, 0);
	}

	Throttle = -Prm.CurPedlPosn;

	if (Prm.InitSequenceLocalTime < TimeToPressToDrive)
	{
		Gear = EGearState::Park;
	}
	else
	{
		Gear = EGearState::Drive;
	}

	if (Prm.InitSequenceLocalTime > TimeToFinishInit)
	{
		Prm.bInitSequenceIsDone = true;
	}

}

float UVehicleInputAIComponent::WrapDistanceToSpline(float CurrentDistance, float StepToMove, float LengthOfPreviousSpline, float LengthOfCurrentSpline, float LengthOfNextSpline, EDistanceRemapResult& RemapResult)
{
	float NextDistanceNormal = CurrentDistance + StepToMove;
	float NextDistanceAdjusted = NextDistanceNormal;

	if (NextDistanceNormal < 0.0)
	{
		NextDistanceAdjusted = LengthOfPreviousSpline - (ExpandOfCentralPointSpline / 2 - CurrentDistance);
		RemapResult = EDistanceRemapResult::VE_MapToPreviousSpline;
		//UE_LOG(LogTemp, Warning, TEXT("Remap distance from %f to %f, step was %f"), NextDistanceNormal, NextDistanceAdjusted,StepToMove);
	}


	if (NextDistanceNormal > LengthOfCurrentSpline)
	{
		NextDistanceAdjusted = CurrentDistance + ExpandOfCentralPointSpline / 2 - LengthOfNextSpline;
		RemapResult = EDistanceRemapResult::VE_MapToNextSpline;
		//	UE_LOG(LogTemp, Warning, TEXT("Remap distance from %f to %f, step was %f"), NextDistanceNormal, NextDistanceAdjusted, StepToMove);
	}

	RemapResult = EDistanceRemapResult::VE_NoAction;
	return NextDistanceAdjusted;
}





void UVehicleInputAIComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SpeedError: %f"), AIControllerState.SpeedError), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("YawError: %f"), AIControllerState.YawError), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SideError: %f"), AIControllerState.SideError), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("YawPredError: %f"), AIControllerState.YawPredError), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SidePredError: %f"), AIControllerState.SidePredError), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("TargetCurvature: %f"), AIControllerState.TargetCurvature), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("TargetSpeed: %f"), AIControllerState.TargetSpeed / 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("VehSpd: %f"), AIControllerState.VehSpd / 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("CurrentDstOnTrajectory: %f"), AIControllerState.CurrentDstOnTrajectory), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("FixedDistanceOfBraking: %f"), AIControllerState.FixedDistanceOfBraking), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("FixedSpeedOfBraking: %f"), AIControllerState.FixedSpeedOfBraking), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bRouteFound: %d"), (int32)AIControllerState.bRouteFound), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bRouteSearchIsDone: %d"), (int32)AIControllerState.bRouteSearchIsDone), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bFinalSegment: %d"), (int32)AIControllerState.bFinalSegment), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("CurrentSplineIdx: %d"), AIControllerState.CurrentSplineIdx), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("DistanceCumulative: %f"), AIControllerState.DistanceCumulative), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("TotalDistance: %f"), AIControllerState.TotalDistance), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bFinalBrakeIsActive: %d"), (int32)AIControllerState.bFinalBrakeIsActive), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("DistanceToObstacle: %f"), AIControllerState.DistanceToObstacle), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("RelativeVelocity: %f"), AIControllerState.RelativeVelocityToObstacle), 16, YPos);
	}




}





void UVehicleInputAIComponent::CalculateSpeedProfile(float Mass, int32 StartPoint, float StartingSpeed)
{


	float Curv = 0.0;
	float UNew = 0.0;
	float MinVal = 0.0;





	FSpeedProfilePrms SpdProf;

	bool bSpdProfSuccess = GetSpeedProfilePrms(SpdProf);

	if (!bSpdProfSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("No elements in speed profile!"));
		return;
	}



	float AMax = SpdProf.SpdProfAMax;
	float ALatLim = SpdProf.SpdProfALatLim;
	float DeceMax = -SpdProf.SpdProfADece;
	float SpdProfMuDesArg = SpdProf.SpdProfMuDes;
	float SpdProfVLgtMaxArg = SpdProf.SpdProfVLgtMax;





	// Will raise to true in case if speedprofile was triggered to recalculate with lower limits
	bool bNeedForInitialDeceleration = false;

	if (StartPoint >= TrajectorySampled.Curvature.Num())
	{
		return;
	}

	TArray<float> DirectPass;
	TArray<float> ForwardPass;
	TArray<float> ReversePass;

	DirectPass.Init(0.0, TrajectorySampled.Curvature.Num());
	ForwardPass.Init(0.0, TrajectorySampled.Curvature.Num());
	ReversePass.Init(0.0, TrajectorySampled.Curvature.Num());


	/*
	Direct pass - calculate speed limit based on curvature and maximum velocity, assuming infinite acceleration;
	Forward pass - adjust speed limit based on maximum acceleration limit;
	Reverse pass - adjust speed limit based on maximum deceleration limit;
	*/


	// Create trivial speed profile
	if (bSpdProfUseOnlyVLgtMax)
	{
		for (auto& Elem : TrajectorySampled.SpeedProfile)
		{
			Elem = SpdProfVLgtMaxArg / 3.6;
		}

		return;
	}

	float MuRoad = 1.0;

	// Direct pass 
	for (int32 Idx = StartPoint; Idx <= TrajectorySampled.Curvature.Num() - 1; Idx++)
	{
		Curv = abs(TrajectorySampled.Curvature[Idx]);
		Curv = FMath::Clamp(Curv, 0.000001, 100);

		DirectPass[Idx] = (FMath::Min(FMath::Sqrt(ALatLim * MuRoad * SpdProfMuDesArg / Curv), SpdProfVLgtMaxArg / 3.6));
	}

	// Forward pass + update initial conditions
	ForwardPass[StartPoint] = StartingSpeed / 3.6;

	if (ForwardPass[StartPoint] > DirectPass[StartPoint])
	{
		bNeedForInitialDeceleration = true;
	}

	for (int32 Idx = StartPoint + 1; Idx <= TrajectorySampled.Curvature.Num() - 1; Idx++)
	{

		Curv = abs(TrajectorySampled.Curvature[Idx]);

		float ALatCurr = FMath::Square(ForwardPass[Idx - 1]) * Curv;
		float ARealMax = 0.0;


		if (ForwardPass[Idx - 1] > DirectPass[Idx] && bNeedForInitialDeceleration)
		{
			// has to be smooth braking
			ARealMax = FMath::Sqrt(FMath::Max((1 - (FMath::Square(ALatCurr * MuRoad * SpdProfMuDesArg) / FMath::Square(ALatLim * MuRoad * SpdProfMuDesArg))) * FMath::Square((DeceMax - 0) * MuRoad * SpdProfMuDesArg), 0.0));
			UNew = FMath::Sqrt(FMath::Max(FMath::Square(ForwardPass[Idx - 1]) - 2 * ARealMax * TrajDisc, 0.0));
			MinVal = FMath::Max(UNew, DirectPass[Idx]);

			if (UNew < DirectPass[Idx])
			{
				// braking process completed, proceed with regular speed profile generation
				bNeedForInitialDeceleration = false;
			}
		}
		else
		{
			ARealMax = FMath::Sqrt(FMath::Max((1 - (FMath::Square(ALatCurr * MuRoad * SpdProfMuDesArg) / FMath::Square(ALatLim * MuRoad * SpdProfMuDesArg))) * FMath::Square((AMax + 0.0) * MuRoad * SpdProfMuDesArg), 0.0));
			UNew = FMath::Sqrt(FMath::Square(ForwardPass[Idx - 1]) + 2 * ARealMax * TrajDisc);
			MinVal = FMath::Min(UNew, DirectPass[Idx]);
		}

		ForwardPass[Idx] = (MinVal);
	}


	// Reverse pass
	ReversePass[TrajectorySampled.Curvature.Num() - 1] = ForwardPass[TrajectorySampled.Curvature.Num() - 1];

	for (int32 Idx = TrajectorySampled.Curvature.Num() - 2; Idx > StartPoint - 1; Idx--)
	{
		// Forward pass 
		Curv = abs(TrajectorySampled.Curvature[Idx]);

		float ALatCurr = FMath::Square(ReversePass[Idx + 1]) * Curv;
		float DeceMaxReal = FMath::Sqrt(FMath::Max((1 - (FMath::Square(ALatCurr * MuRoad * SpdProfMuDesArg) / FMath::Square(ALatLim * MuRoad * SpdProfMuDesArg))) * FMath::Square((DeceMax - 0) * MuRoad * SpdProfMuDesArg), 0.0));

		UNew = FMath::Sqrt(FMath::Square(ReversePass[Idx + 1]) + 2 * DeceMaxReal * TrajDisc);
		MinVal = FMath::Min(UNew, ForwardPass[Idx]);

		ReversePass[Idx] = (MinVal);

		TrajectorySampled.SpeedProfile[Idx] = MinVal;

		// Check stopping distance according to the current target speed
		float StoppingDistance = (FMath::Square(MinVal / 3.6)) / (2.0f * EndOfTheRouteADece);
		float CurrentDstOnTrajectory = TrajectorySampled.Distance[Idx];

		if (CurrentDstOnTrajectory + StoppingDistance > AIControllerState.TotalDistance)
		{
			AIControllerState.FixedDistanceOfBrakingNominal = CurrentDstOnTrajectory;
		}


	}

	// Finish speed profile generation by copying velocity from missing point in cycle for reverse pass
	TrajectorySampled.SpeedProfile[TrajectorySampled.Curvature.Num() - 1] = TrajectorySampled.SpeedProfile[TrajectorySampled.Curvature.Num() - 2];

	if (bDebugSpeedProfile)
	{
		LogSimulationStart();

		if (bRecordingOk)
		{
			for (int32 k = 0; k < ReversePass.Num(); k++)
			{
				FString Output;
				Output += FString::SanitizeFloat((float)k) + ", ";
				Output += FString::SanitizeFloat(TrajectorySampled.Distance[k]) + ", ";
				Output += FString::SanitizeFloat(StartingSpeed / 3.6) + ", ";
				Output += FString::SanitizeFloat((float)StartPoint) + ", ";
				Output += FString::SanitizeFloat(TrajectorySampled.Curvature[k]) + ", ";
				Output += FString::SanitizeFloat(DirectPass[k]) + ", ";
				Output += FString::SanitizeFloat(ForwardPass[k]) + ", ";
				Output += FString::SanitizeFloat(ReversePass[k]) + ", ";
				CsvOutFile << TCHAR_TO_UTF8(*Output) << "\n";


				//UE_LOG(LogTemp, Warning, TEXT("k = %d, Starting speed %f, idx %d, curvature %f, Direct pass %f, forward pass %f, reverse pass %f"), k, StartingSpeed / 3.6, StartPoint,TrajectorySampled.Curvature[k],DirectPass[k], ForwardPass[k], ReversePass[k]);
			}
		}

		LogSimulationComplete();
	}

}


float UVehicleInputAIComponent::CalculateCurvature(const FVector& PointA, const FVector& PointB, const FVector& PointC)
{
	// Compute the lengths of the triangle sides
	float AB = FVector::Dist(PointA, PointB);
	float BC = FVector::Dist(PointB, PointC);
	float CA = FVector::Dist(PointC, PointA);

	//// Semi-perimeter of the triangle
	//float SemiPerimeter = (AB + BC + CA) / 2.0f;

	//// Area of the triangle using Heron's formula
	//float UnderPerimeter = SemiPerimeter * (SemiPerimeter - AB) * (SemiPerimeter - BC) * (SemiPerimeter - CA); 

	//float Area = 0.0;
	//
	//if (UnderPerimeter > 0.0001)
	//{
	//	Area=FMath::Sqrt(UnderPerimeter);
	//}
	//



	float AreaSign = 0.5 * (PointA.X * (PointB.Y - PointC.Y) + PointB.X * (PointC.Y - PointA.Y) + PointC.X * (PointA.Y - PointB.Y));// (PointB.X - PointA.X)* (PointC.Y - PointA.Y) - (PointB.Y - PointA.Y) * (PointC.X - PointA.X);

	//UE_LOG(LogTemp, Warning, TEXT("area %f, area sign %f"), Area,AreaSign);





	// If the area is zero (collinear points), return zero curvature
	if (FMath::IsNearlyZero(AreaSign))
	{
		return 0.0f;
	}

	// Circumcircle radius (R)
	float Radius = (AB * BC * CA) / (4.0f * AreaSign) * (bInvertCurvatureSign ? -1.0 : 1.0);

	// Curvature is the reciprocal of the radius







	return 1.0f / Radius;
}



TArray<float> UVehicleInputAIComponent::CalculateMovingAverage(const TArray<float>& InputArray, int32 WindowSize)
{
	// Validate input
	if (WindowSize <= 0 || InputArray.Num() < WindowSize)
	{
		return TArray<float>();
	}
	const int32 NumElements = InputArray.Num();
	TArray<float> MovingAverageArray;
	MovingAverageArray.Reserve(NumElements);

	if (bCircularRoute)
	{
		// special case where we do regular average +- window size 
				// Circular moving average
		for (int32 i = 0; i < NumElements; i++)
		{
			float Sum = 0.0f;
			int32 Count = 0;

			// Look WindowSize elements backward and forward
			for (int32 j = -WindowSize; j <= WindowSize; j++)
			{
				if (j == 0) continue; // skip the current element itself if you want

				// Wrap index circularly
				int32 Index = (i + j + NumElements) % NumElements;
				Sum += InputArray[Index];
				Count++;
			}

			// Regular average
			MovingAverageArray.Add(Sum / Count);
		}

		return MovingAverageArray;
	}


	//MovingAverageArray.Reserve(InputArray.Num() - WindowSize + 1);

	float WindowSum = 0.0f;

	// Calculate the sum of the first window
	for (int32 i = 0; i < WindowSize; ++i)
	{
		if (i < WindowSize - 1)
		{
			MovingAverageArray.Add(InputArray[i]);
		}

		WindowSum += InputArray[i];
	}

	// Add the first average to the result
	MovingAverageArray.Add(WindowSum / WindowSize);

	// Slide the window across the array
	for (int32 i = WindowSize; i < InputArray.Num(); ++i)
	{
		// Subtract the element that is sliding out and add the new element
		WindowSum += InputArray[i] - InputArray[i - WindowSize];

		// Add the new average to the result
		MovingAverageArray.Add(WindowSum / WindowSize);
	}

	return MovingAverageArray;
}

void UVehicleInputAIComponent::RecalculateSpeedProfile()
{
	if (TrajectorySampled.Distance.Num() < 2)
	{

		return;
	}

	float SmallestDiff = 100000;
	int32 ClosestIdx = 0;



	for (int32 i = 0; i < TrajectorySampled.Distance.Num(); i++)
	{
		float DstDiff = FMath::Abs(TrajectorySampled.Distance[i] - AIControllerState.CurrentDstOnTrajectory / 100);


		//UE_LOG(LogTemp, Warning, TEXT("i = %d, Sampled dst %f, CurrentDst %f"), i, TrajectorySampled.Distance[i], AIControllerState.CurrentDstOnTrajectory / 100);


		if (DstDiff < SmallestDiff)
		{
			ClosestIdx = i;
			SmallestDiff = DstDiff;
		}

	}

	UE_LOG(LogTemp, Warning, TEXT("SPEED PROF RECALCULATE FROM IDX %d"), ClosestIdx);
	CalculateSpeedProfile(GetWheeledVehicle()->GetWheeledComponentInterface()->GetVehicleMass(), ClosestIdx, AIControllerState.VehSpd);

}


void UVehicleInputAIComponent::CheckSlowdownForObstacle()
{
	if (DetectedObstacle.bObstaclePresented)
	{
		DetectedObstacle.UpdateObstacleData();


		if (AIControllerState.SpeedControlIsActive && !DetectedObstacle.CheckIsItNewOne())
		{
			// no need to change the status
			return;
		}

		float RelVel = DetectedObstacle.GetRelativeVelocity(GetWheeledVehicle());

		if (RelVel > 0)
		{
			float StoppingDistance = FMath::Square(RelVel / 3.6) / (2.0f * EndOfTheRouteADece);

			if (StoppingDistance >= DetectedObstacle.GetDistanceAlongTheTrajectory(GetWheeledVehicle()) / 100 - TargetDistanceToObstacle)
			{
				AIControllerState.SpeedControlIsActive = true;
				return;
			}
		}


		// If no obstacle is close enough, go with a target speedprofile
		AIControllerState.SpeedControlIsActive = false;
		return;
	}



}

float UVehicleInputAIComponent::CalculateSideError(const FVector& VehPosn, const FVector& PosnBehindVeh, const FVector& PosnPredicted)
{
	// returns closest distance from point to the line with a sign 

	const float& x1 = PosnPredicted.X;
	const float& x2 = PosnBehindVeh.X;
	const float& y1 = PosnPredicted.Y;
	const float& y2 = PosnBehindVeh.Y;

	float Div = sqrtf(powf(x2 - x1, 2) + powf(y2 - y1, 2));

	if (Div < 0.0001)
	{
		return 0;
	}

	return -((x2 - x1) * (y1 - VehPosn.Y) - (x1 - VehPosn.X) * (y2 - y1)) / sqrtf(powf(x2 - x1, 2) + powf(y2 - y1, 2));
}



void UVehicleInputAIComponent::PerformObstacleDetection()
{

	if (!bPerformCollisionDetection)
	{
		return;
	}

	CurrentSplineSegmentCollisionDetn = -1;
	float NewDistance = AIControllerState.CurrentDstOnTrajectory;
	float RelativeDistance = 0;

	FVector VehCur = GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation()
		+ GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.Rotator().RotateVector(FVector(1.151 * 100, 0, 0));
	float VehSplineKey = USodaStatics::FindSplineInputKeyClosestToWorldLocationFastLimited(VehCur, CurrentSplineSegment, RouteSpline, 2, 1);
	FVector StartLocation = RouteSpline->GetLocationAtSplineInputKey(VehSplineKey, ESplineCoordinateSpace::World);

	bool bObstacleWasDetected = false;

	// Do a sequence of sweep collision checks along the trajectory

	while (RelativeDistance < DetectionMaxLength)
	{
		float Now = GetWorld()->GetTimeSeconds();



		VehSplineKey = USodaStatics::FindSplineInputKeyClosestToWorldLocationFastLimited(StartLocation, CurrentSplineSegmentCollisionDetn, RouteSpline, 1000, 1);
		float CurrentDistance = RouteSpline->GetDistanceAlongSplineAtSplineInputKey(VehSplineKey);

		float DetectionStep = DetectionFixedStepDistanceMin * 100;
		float NextDistance = CurrentDistance + DetectionStep;

		// Curvature based adjustment, move in larger steps if curvature is small to save some performance
		if (bCurvatureBasedSampling)
		{
			float CurrentDistanceSampled = AIControllerState.DistanceCumulative + NextDistance / 100;

			float LocalCurvature = LookupTable1d(CurrentDistanceSampled, TrajectorySampled.Distance, TrajectorySampled.Curvature);

			// Change detection step linearly with curvature increased 
			DetectionStep = LinInterp1(FMath::Abs(LocalCurvature), 0, 1.0 / 250.0, DetectionFixedStepDistanceMax * 100, DetectionFixedStepDistanceMin * 100);
			NextDistance = CurrentDistance + DetectionStep;
		}




		FVector EndLocation = RouteSpline->GetWorldLocationAtDistanceAlongSpline(NextDistance);
		FVector FixedZOffset = FVector(0, 0, 50);

		bool bResult = SweepObstacle(StartLocation + CollisionBoxOffset, EndLocation + CollisionBoxOffset, 1.0, GetOwner(), DetectedObstacle);

		// Collision detected, no need to check for a new one
		if (bResult)
		{
			return;
		}

		RelativeDistance += DetectionStep / 100;
		StartLocation = EndLocation;
	}

	if (!bObstacleWasDetected)
	{
		DetectedObstacle.ResetObstacle();
	}



}

bool UVehicleInputAIComponent::SweepObstacle(const FVector& StartLocation, const FVector& EndLocation, float RadiusM, const AActor* Owner, FDetectedObstacle& Obstacle)
{



	FCollisionShape SweepShape = FCollisionShape::MakeBox(CollisionBoxHalfExtent);

	FQuat SweepQuat = (EndLocation - StartLocation).ToOrientationQuat();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = true;

	FHitResult HitResult;
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		(EndLocation - StartLocation).ToOrientationQuat(),
		CollisionChannelToTrace,
		SweepShape,
		QueryParams
	);

	if (bDrawDebugPrimitives)
	{
		FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugBox(GetWorld(), (StartLocation + EndLocation) * 0.5, CollisionBoxHalfExtent, SweepQuat, FColor::Blue, false, DetectionInterval);
	}




	// If a hit occurs, mark the hit location
	if (bHit)
	{

		if (HitResult.GetActor())
		{
			Obstacle.ObstacleAsActor = HitResult.GetActor();
			Obstacle.bObstaclePresented = true;
			Obstacle.UpdateObstacleData();
		}
		else
		{
			Obstacle.DetectStaticObstacle(HitResult.ImpactPoint);
		}

		if (bDrawDebugPrimitives)
		{
			DrawDebugBox(GetWorld(), HitResult.ImpactPoint, CollisionBoxHalfExtent, SweepQuat, FColor::Red, false, DetectionInterval);
		}

	}

	return bHit;




}


float FDetectedObstacle::GetDistanceAlongTheTrajectory(const ASodaWheeledVehicle* OwnerVehicle)
{
	if (OwnerVehicle)
	{
		return (OwnerVehicle->GetSimData().VehicleKinematic.Curr.GlobalPose.GetLocation() - ObstaclePosition).Size();
	}
	return 0.0;
}

float FDetectedObstacle::GetRelativeVelocity(const ASodaWheeledVehicle* OwnerVehicle)
{
	if (OwnerVehicle)
	{
		return OwnerVehicle->GetSimData().VehicleKinematic.Curr.GetLocalVelocity().X / 100.0 * 3.6 - ObstacleVelocity.Size() / 100 * 3.6;
	}
	return 0.0;
}


void UVehicleInputAIComponent::LogSimulationStart()
{
	CsvOutFile.close();

	FString BaseDirectory = FPaths::ProjectSavedDir();
	FString Filename;

	// Keep incrementing until we find a free file name
	do
	{
		Filename = BaseDirectory + CvsFileNameBase;
		Filename.Appendf(TEXT("_%d.csv"), CvsFileNameIndex);
		CvsFileNameIndex++;
	} while (FPaths::FileExists(Filename));


	CsvOutFile.open(TCHAR_TO_UTF8(*Filename), std::ofstream::out);

	if (CsvOutFile.is_open())
	{
		CsvOutFile << "idx,trajectory distance,"
			<< "starting speed, start point,curvature,direct pass,forward pass,reverse pass"
			<< "\n";
		CvsFileNameIndex++;
		bRecordingOk = true;
	}
	else
	{
		bRecordingOk = false;
	}




}

void UVehicleInputAIComponent::LogSimulationComplete()
{
	CsvOutFile.close();
}

void UVehicleInputAIComponent::LogSimulation()
{







}