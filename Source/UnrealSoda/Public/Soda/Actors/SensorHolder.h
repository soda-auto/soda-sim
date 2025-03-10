// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaDataset.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "SensorHolder.generated.h"

class USensorComponent;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ASensorHolder 
	: public AActor
	, public ISodaActor
	, public IObjectDataset
{
GENERATED_BODY()

public:
	UPROPERTY(Category = SensorHolder, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBillboardComponent *SpriteComponent;

	UPROPERTY(Category = SensorHolder, VisibleDefaultsOnly, BlueprintReadOnly)
	class UArrowComponent *ArrowComponent;

	UPROPERTY(Category = SpawnPoint, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBoxComponent* TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SensorHolder, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TSubclassOf<USensorComponent> SensorClass;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	UFUNCTION(BlueprintCallable, Category = RouteBuilder)
	USensorComponent * GetSensor() { return SensorComponent; }

	UFUNCTION(BlueprintCallable, Category = RouteBuilder)
	virtual void RecreateSensor();

	UFUNCTION()
	void OnSelected();

	UFUNCTION()
	void OnUnselected();

public:
	ASensorHolder();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Serialize(FArchive& Ar) override;

public:
	/* Override from ISodaActor */
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;

protected:
	UPROPERTY()
	USensorComponent* SensorComponent = nullptr;

	FComponentRecord ComponentRecord;
};