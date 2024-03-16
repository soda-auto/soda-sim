// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/EngineTypes.h"
#include "Components/TextRenderComponent.h"
#include "Soda/ISodaActor.h"
#include "RefPoint.generated.h"


/**
 * ARefPoint
 * This ISodaActor allows to define a correspondance between the UE origin and an actual geographic location on a planet.
 * It is mainly used by GPS sensors to convert coordinates from local to geographic.
 * TODO: Try to use AGeoReferencingSystem insted ARefPoint
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ARefPoint : 
	public AActor,
	public ISodaActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RefPoint, meta = (EditInRuntime))
	double Latitude = 59.995;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RefPoint, meta = (EditInRuntime))
	double Longitude = 30.13;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RefPoint, meta = (EditInRuntime))
	double Altitude = 10.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visibility, SaveGame, meta = (EditInRuntime))
	bool bDrawAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visibility, SaveGame, meta = (EditInRuntime))
	bool bDrawOverTop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visibility, SaveGame, meta = (EditInRuntime))
	bool bScreenAutoScale = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Visibility, SaveGame, meta = (EditInRuntime))
	float ScreenSize = 0.001f;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = RefPoint, meta = (CallInRuntime))
	void UpdateGlobalRefPoint();

public:
	ARefPoint();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;


public:
	/* Override from ISodaActor */
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor * GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual bool ShowSelectBox() const override { return false; }

protected:
	UPROPERTY()
	UProceduralMeshComponent * Shaft;

	UPROPERTY()
	UMaterialInterface* MatDense;

	UPROPERTY()
	UMaterialInterface* MatOpacity;

	UPROPERTY()
	UMaterialInterface* MatText;

	UPROPERTY(Transient)
	UTextRenderComponent* TextRenderNorth;

	UPROPERTY(Transient)
	UTextRenderComponent* TextRenderSouth;

	UPROPERTY(Transient)
	UTextRenderComponent* TextRenderWest;

	UPROPERTY(Transient)
	UTextRenderComponent* TextRenderEast;
};
