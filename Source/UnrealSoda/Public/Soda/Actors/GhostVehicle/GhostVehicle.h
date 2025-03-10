// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Soda/Misc/TrajectoryPlaner.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaDataset.h"
#include <vector>
#include "GhostVehicle.generated.h"


namespace SpeedProfile
{
	class FSpeedProfile;
	struct FVelocityProfile;
}

class ANavigationRouteEditable;

USTRUCT(BlueprintType)
struct FGhostVehicleWheel
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup)
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup)
	bool bApplySteer = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup)
	float Radius = 30;

	FVector Location;
	float RotationAngularVelocit = 0;
	float RotationAngle = 0;
	float SteerAngle = 0;
	float SuspensionOffset = 0;
	FHitResult Hit;

	float GetRotationAngularVelocity() const { return RotationAngularVelocit; }
	float GetRotationAngle() const { return RotationAngle; }
	float GetSteerAngle()  const { return SteerAngle; }
	float GetSuspensionOffset() const { return SuspensionOffset; }
};

/**
 * AGhostVehicle
 * This is an agent for scenarios "wheeled vehicle". In the current implementation,
 * it is able to simply walk along the ANavigationRoute
 * TODO: Support Suspension Offset
 * TODO: Add dumping for pitch, roll, Z
 * TODO: Fix bug of determinate init spline offset when moving vehice by the widget
 */

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AGhostVehicle 
	: public AActor
	, public ISodaActor
	, public IObjectDataset
{
GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = GhostVehicle,  BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category = GhostVehicle, BlueprintReadOnly)
	TArray<FGhostVehicleWheel> Wheels;

	// [deg/s]
	UPROPERTY(EditAnywhere, Category = GhostVehicle, BlueprintReadOnly)
	float SteeringAnimMaxSpeed = 30;

	// [cm]
	UPROPERTY(EditAnywhere, Category = GhostVehicle, BlueprintReadOnly)
	float RayCastDepth = 500;

	UPROPERTY(EditAnywhere, Category = GhostVehicle, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSet<FString> AllowedRouteTags;

	/** [km/h] */
	UPROPERTY(EditAnywhere, Category = GhostVehicle, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float InitVelocity = 0;

	/** [km/h] */
	UPROPERTY(EditAnywhere, Category = VehicleDynamics, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float VehicleMaxVelocity = 55.0;

	/** [kg] */
	UPROPERTY(EditAnywhere, Category = VehicleDynamics, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float VehicleMass = 1196;

	/** [W] */
	UPROPERTY(EditAnywhere, Category = VehicleDynamics, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float VehicleMaxPower = 270000.0;

	/** [Nm/s] */
	UPROPERTY(EditAnywhere, Category = VehicleDynamics, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float VehicleMaxTorque = 2500.0;

	/** [km/h] */
	UPROPERTY(EditAnywhere, Category = VehicleLimits, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float LimitMaxVelocity = 42;

	UPROPERTY(EditAnywhere, Category = VehicleLimits, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float LimitMu = 0.85;

	/** [m/s] */
	UPROPERTY(EditAnywhere, Category = VehicleLimits, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float LimitMaxAcceleration = 5;

	/** [m/s] */
	UPROPERTY(EditAnywhere, Category = VehicleLimits, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float LimitMaxDeceleration = 5;

	UPROPERTY(EditAnywhere, Category = VehicleLimits, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bIsAggressive = false;

	UPROPERTY(EditAnywhere, Category = TrajectoryPlaner, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	FTrajectoryPlaner TrajectoryPlaner;

	UPROPERTY(EditAnywhere, Category = InitialRoute, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRouteEditable> InitialRoute;

	UPROPERTY(EditAnywhere, Category = InitialRoute, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float InitialRouteOffset = 0;

	UPROPERTY(EditAnywhere, Category = InitialRoute, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bJoinToInitialRoute = true;

	UPROPERTY(EditAnywhere, Category = Debug, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugRoute = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bRecordDataset = true;

public:
	UFUNCTION(BlueprintCallable, Category = GhostVehicle, meta = (ScenarioAction))
	void GoToLeftRoute(float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostVehicle, meta = (ScenarioAction))
	void GoToRightRoute(float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostVehicle, meta = (ScenarioAction))
	void GoToRoute(TSoftObjectPtr<ANavigationRoute> Route, float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostVehicle, meta = (CallInRuntime, ScenarioAction))
	void StartMoving();

	UFUNCTION(BlueprintCallable, Category = GhostVehicle, meta = (CallInRuntime, ScenarioAction))
	void StopMoving();

	UFUNCTION(BlueprintCallable, Category = GhostVehicle)
	bool IsLinkedToRoute() const;

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "Start Moving"))
	void ReceiveStartMoving();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "Stop Moving"))
	void ReceiveStopMoving();

	UFUNCTION(BlueprintCallable, Category = GhostVehicle)
	bool IsMoving() const { return bIsMoving;}

	UFUNCTION(BlueprintCallable, Category = GhostVehicle)
	float GetCurrentVelocity() const { return CurrentVelocity;}

	UFUNCTION(BlueprintCallable, Category = GhostVehicle)
	float GetCurrentAcc() const { return CurrentAcc; }

	UFUNCTION(BlueprintCallable, Category = GhostVehicle)
	float GetCurrentSplineOffset() const { return CurrentSplineOffset;}
	
public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor * GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;
	virtual void InputWidgetDelta(const USceneComponent* WidgetTargetComponent, FTransform& NewWidgetTransform) override;
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool ShouldRecordDataset() const override { return bRecordDataset; }

public:
	AGhostVehicle();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual FVector GetVelocity() const override;
	virtual void Serialize(FArchive& Ar) override;

	FTransform GetRealAxesTransform() const;

protected:
	void CalculateSpeedProfile();
	void UpdateJoinCurve();

	FTransform InitTransform;
	bool bIsMoving = false;
	double CurrentVelocity = 0;
	double CurrentAcc = 0;
	double CurrentSplineOffset = 0;
	double BaseLength = 0;
	float VehicleRWheel = 35; 
	FVector RealAxesOffset;

	FGuid SplineGuid;

	TSharedPtr<SpeedProfile::FSpeedProfile> SpeedProfile;
	std::vector<double> Velocities;
	std::vector<double> Accelerations;

	TArray<FVector> JoiningCurvePoints;

};
