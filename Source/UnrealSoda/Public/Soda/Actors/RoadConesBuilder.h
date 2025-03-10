// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Soda/ISodaActor.h"

#include "RoadConesBuilder.generated.h"




/**
 * EConePlacements
 * Struct to configurate placement of the cones
 */

UENUM(BlueprintType)
enum class EConePlacements : uint8 {
	BothSides      UMETA(DisplayName = "BothSides"),
	LeftSide      UMETA(DisplayName = "LeftSide"),
	RightSide      UMETA(DisplayName = "RightSide"),
};


/**
 * ARoadConesBuilder
 * Actor that allows to place cones on scene to be used in vehicle simulation
 */


UCLASS()
class UNREALSODA_API ARoadConesBuilder : 
	public AActor,
	public ISodaActor
{
	GENERATED_BODY()

public:

	ARoadConesBuilder();


	/** Number of cones to generate on each selected side */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Generation Parameters")
	int32 NumberOfCones = 1;

	/** Cone placement pattern */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Generation Parameters")
	EConePlacements ConePlacementPattern = EConePlacements::BothSides;

	/** Distance between cones, cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Generation Parameters")
	float DistanceBetweenCones = 250.0;

	/** Road width, cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime), Category = "Generation Parameters")
	float RoadWidth = 300.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RoadConeBP")
	TSubclassOf<class AActor> RoadConeBP;

	UFUNCTION(CallInEditor, Category = Generation, meta = (DisplayName = "Regenerate Cones", CallInRuntime))
	void UpdateGeneration();

	virtual void ScenarioBegin() override;
	virtual void BeginPlay() override;

private: 

	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	void SpawnNewCone(FVector Position);

};
