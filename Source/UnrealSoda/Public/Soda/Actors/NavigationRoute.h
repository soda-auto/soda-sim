// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaDataset.h"
#include "NavigationRoute.generated.h"

class USodaRandomEngine;
class UProceduralMeshComponent;


UCLASS(meta = (BlueprintSpawnableComponent))
class UNREALSODA_API URouteSplineComponent 
	: public USplineComponent
{
	GENERATED_BODY()

public:
	virtual void UpdateSpline() override
	{
		Super::UpdateSpline();
		SplineUpdateGuid = FGuid::NewGuid();
	}
	const FGuid& GetSplineGuid() const { return SplineUpdateGuid; }

protected:
	FGuid SplineUpdateGuid;
};

/**
 * ANavigationRoute
 * This is a spline along which some other actors can move. First of all, these are AGhostVehicle, AGhostPedestrian, ASodaVehicle.
 * TODO: Finish Predecessor & Successor logic. Abilty to connect routes throw UI
 * TODO: Look at UPathFollowingComponent
 */
UCLASS()
class UNREALSODA_API ANavigationRoute : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere)
	UBoxComponent* TriggerVolume;

	UPROPERTY(BlueprintReadOnly, Category = NavigationRoute, EditAnywhere)
	UProceduralMeshComponent* ProceduralMesh; 

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere)
	URouteSplineComponent* Spline;

public:
	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	bool bAllowForPedestrians = true;

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	bool bAllowForVehicles = true;

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	bool bDriveBackvard = false;

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	float Probability = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	float ZOffset = 50;

	/** Tags can be used to determine which traffic participants can use a given route and which cannot */
	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	TSet<FString> RouteTags;

	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere, SaveGame, meta = (EditInRuntime))
	FLinearColor BaseColor {0.0, 0.115909, 0.322917, 1.0};

	UPROPERTY(EditAnywhere, Category = Connections, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRoute> LeftRoute;

	UPROPERTY(EditAnywhere, Category = Connections, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRoute>  RightRoute;

	UPROPERTY(EditAnywhere, Category = Connections, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRoute> PredecessorRoute;

	UPROPERTY(EditAnywhere, Category = Connections, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ANavigationRoute> SuccessorRoute;

public:



	UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	bool SetRoutePoints(const TArray< FVector >& RoutePoints, bool bUpdateMesh=true);

	UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	const URouteSplineComponent* GetSpline() const { return Spline; }

	UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	bool IsRouteTagsAllowed(const TSet<FString>& AllowedRouteTags) const;

	//UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	//TArray<ANavigationRoute *> GetPredecessors() const { return Predecessors; }

	//UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	//TArray<ANavigationRoute *> GetSuccessors() const { return  Successors;}

	//UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	//ANavigationRoute* GetRandomSuccessor() const;

	UFUNCTION(Category = NavigationRoute, BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	virtual void UpdateViewMesh();

	UFUNCTION(Category = NavigationRoute, BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	virtual void FitActorPosition();

	UFUNCTION(Category = NavigationRoute, BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	virtual void PullToGround();

	UFUNCTION(Category = NavigationRoute, BlueprintCallable)
	virtual bool UpdateProcedureMeshSegment(int SegmentIndex);

public:
	ANavigationRoute(const FObjectInitializer& ObjectInitializer);

	// TODO: not implemented yet
	static void UpdateRouteNetwork(const ANavigationRoute* Initiator, bool bIsInitiatorPredecessor);

	void PropagetUpdatePredecessorRoute();
	void PropagetUpdateSuccessorRoute();

	bool GroundHitFilter(const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	//UPROPERTY()
	//TArray<ANavigationRoute*> Predecessors;

	//UPROPERTY()
	//TArray<ANavigationRoute*> Successors;

	UPROPERTY()
	USodaRandomEngine* RandomEngine = nullptr;

	UPROPERTY()
	UMaterialInterface* MatProcMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* MatProcMeshDyn = nullptr;
};

/**
 * URoutePlannerEditableNode
 */
UCLASS()
class UNREALSODA_API URoutePlannerEditableNode : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = RoutePlannerEditableNode, EditAnywhere, BlueprintReadWrite)
	FLinearColor BaseColor;

	UPROPERTY(Category = RoutePlannerEditableNode, EditAnywhere, BlueprintReadWrite)
	FLinearColor HoverColor;

public:
	int PointIndex = -1;

public:
	URoutePlannerEditableNode(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void LightNode(bool bLight);

protected:
	UPROPERTY()
	UMaterialInstanceDynamic* MatDyn;

	bool bIsHover = false;
};

/**
 * ANavigationRouteEditable
 * This is version of ANavigationRoute, but avalible for editing in the Scenario Editor Mode
 * TODO: Save route to dataset
 */
UCLASS()
class UNREALSODA_API ANavigationRouteEditable 
	: public ANavigationRoute
	, public ISodaActor
	, public IObjectDataset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = NavigationRoute, EditAnywhere)
	FLinearColor SelectColor {0.208333, 0.103773, 0.0,1.0};

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	void UpdateNodes();

	virtual void PullToGround() override;

public:
	/* Override from ISodaActor */
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	virtual void OnSelect_Implementation(const UPrimitiveComponent* PrimComponent) override;
	virtual void OnUnselect_Implementation() override;
	virtual void OnHover_Implementation() override;
	virtual void OnUnhover_Implementation() override;
	virtual void OnComponentHover_Implementation(const UPrimitiveComponent* PrimComponent) override;
	virtual void OnComponentUnhover_Implementation(const UPrimitiveComponent* PrimComponent) override;
	virtual const FSodaActorDescriptor * GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual bool ShowSelectBox() const override { return false; }
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;


	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Save the data of this route into the file", CallInRuntime))
	void SaveNavigationRouteToFile();
	UFUNCTION(CallInEditor, Category = LoadCustomFile, meta = (DisplayName = "Recreate the data of this route from the file", CallInRuntime))
	void RecreateNavigationRouteFromFile();

public:
	ANavigationRouteEditable(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY()
	TArray<URoutePlannerEditableNode *> EditableNodes;

	UPROPERTY()
	URoutePlannerEditableNode * PreviewNode = nullptr;

	UPROPERTY()
	URoutePlannerEditableNode * SelectedNode = nullptr;

	UPROPERTY()
	URoutePlannerEditableNode * HoverNode = nullptr;

	bool bIsSelected = false;
	bool bIsUnfreezeNotified = false;

	static bool TraceForMousePositionByChanel(const APlayerController * PlayerController, TArray<FHitResult>& HitResults, const TEnumAsByte<ECollisionChannel> & CollisionChannel);
	static bool TraceForMousePositionByObject(const APlayerController* PlayerController, TArray<struct FHitResult>& HitResults, const TEnumAsByte<ECollisionChannel>& CollisionChannel);
};
