// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Soda/Misc/TrajectoryPlaner.h"
#include "Soda/ISodaActor.h"
#include <vector>
#include "GhostPedestrian.generated.h"


class ANavigationRouteEditable;

namespace soda
{
	class FActorDatasetData;
	struct FBsonDocument;
}

/**
 * AGhostPedestrian 
 * This is an agent for scenarios "pedestrian". In the current implementation, 
 * it is able to simply walk along the ANavigationRoute
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AGhostPedestrian :
	public AActor,
	public ISodaActor
{
GENERATED_BODY()

public:
	UPROPERTY(Category = GhostPedestrian, EditAnywhere, BlueprintReadOnly)
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(Category = GhostPedestrian, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	FVector RouteFollowOffset;

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

	UPROPERTY(EditAnywhere, Category = GhostPedestrian, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	FRotator RotationOffset;

	/** [cm/s] */
	UPROPERTY(EditAnywhere, Category = GhostPedestrian, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float Velocity = 60;

	/** [cm/s] */
	UPROPERTY(EditAnywhere, Category = GhostPedestrian, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float Acceleration = 100;

	UPROPERTY(EditAnywhere, Category = Collision, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bCheckCollision = true;

	/** [cm] */
	UPROPERTY(EditAnywhere, Category = Collision, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float TraceLookAhead = 200;

	UPROPERTY(EditAnywhere, Category = Collision, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float TraceUpOffes = 150;

	UPROPERTY(EditAnywhere, Category = Collision, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float TraceCapsuleRadius = 40;
		
	UPROPERTY(EditAnywhere, Category = Collision, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	float TraceCapsuleHalfHeigh = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bRecordDataset = true;

public:
	UFUNCTION(BlueprintCallable, Category = GhostPedestrian, meta = (ScenarioAction))
	void GoToLeftRoute(float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian, meta = (ScenarioAction))
	void GoToRightRoute(float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian, meta = (ScenarioAction))
	void GoToRoute(TSoftObjectPtr<ANavigationRoute> Route, float StaticOffset = 500, float VelocityToOffset = 0);

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian, meta = (CallInRuntime, ScenarioAction))
	void StartMoving();

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian, meta = (CallInRuntime, ScenarioAction))
	void StopMoving();

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian)
	bool IsLinkedToRoute() const;

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "Start Moving"))
	void ReceiveStartMoving();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "Stop Moving"))
	void ReceiveStopMoving();

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian)
	bool IsMoving() const { return bIsMoving;}

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian)
	float GetCurrentVelocity() const { return CurrentVelocity;}

	UFUNCTION(BlueprintCallable, Category = GhostPedestrian)
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

public:
	AGhostPedestrian();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual FVector GetVelocity() const override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	void UpdateJoinCurve();
	FTransform AdjustTransform(const FTransform& Transform);
	bool CheckCollision(const FTransform& Transform);

	virtual void GenerateDatasetDescription(soda::FBsonDocument& Doc) const;
	virtual void OnPushDataset() const;

	FTransform InitTransform;
	bool bIsMoving = false;
	float CurrentVelocity = 0;
	float CurrentSplineOffset = 0;

	FGuid SplineGuid;

	TArray<FVector> JoiningCurvePoints;

	TSharedPtr<soda::FActorDatasetData> Dataset;
};
