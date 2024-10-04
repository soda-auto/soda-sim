// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Pawn.h"

#include "SplineMeshBuilder.generated.h"




/**
 * FMeshReplacements
 * Struct to configurate replacement of main meshes on spline
 */

USTRUCT(BlueprintType)
struct FMeshReplacements
{
	GENERATED_BODY()

public:

	/** Mesh that will replace main mesh on desired interval */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* MeshOnSplineAlternative;

	/** Interval where replacement will be done, cm. Precision is limited by LengthOfTheSegment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFloatInterval DistanceOnSplineToCover = FFloatInterval(0.0,0.0);

};

/**
 * ASplineMeshBuilder
 * Actor that allows to build a set of meshes along the spline
 * No gimbal lock issue with pitch > 90 deg
 */


UCLASS()
class UNREALSODA_API ASplineMeshBuilder : public AActor
{
	GENERATED_BODY()

public:

	ASplineMeshBuilder();
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Spline")
	USplineComponent* SplineRoadBackComponent;

	/** Material that will override 0 material channel in mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	class UMaterialInterface* OverrideMaterial;


	UPROPERTY()
	TArray<USplineMeshComponent*>  SplineMeshComponentArray;

	/** Main mesh that will be applied along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	UStaticMesh* MeshOnSpline;

	/** Desired length of each mesh segment placed on spline, cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	float LengthOfTheSegment = 1500;

	/** Mesh replacement configurations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	TArray<FMeshReplacements> ReplacementsMeshes;

	/** True to replace main mesh with replacement meshes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	bool bUseReplacementsMeshes = false;

	float GetRoll(float Dst, FRotator RelativeRotation);

	void PlaceMeshesAlongSpline();
	void UpdateUpVectors();

private: 
	bool bClosedLoop = false;


};
