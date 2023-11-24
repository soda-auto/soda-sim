// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Soda/ISodaActor.h"
#include "SpawnPoint.generated.h"


 /**
  * ASpawnPoint
  */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ASpawnPoint : 
	public AActor,
	public ISodaActor
{
GENERATED_BODY()

public:
	UPROPERTY(Category = SpawnPoint, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBillboardComponent *SpriteComponent;

	UPROPERTY(Category = SpawnPoint, VisibleDefaultsOnly, BlueprintReadOnly)
	class UArrowComponent *ArrowComponent;

	UPROPERTY(Category = SpawnPoint, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBoxComponent* TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpawnPoint, SaveGame, meta = (EditInRuntime))
	TSubclassOf<USodaActor> ActorClass;

	UPROPERTY(EditAnywhere, Category = Scenario, SaveGame, meta = (EditInRuntime))
	bool bIsActivePoint = true;

	/** if ActorClass is ASodaWheeledVehicle wether to set it to AI mode  */
	UPROPERTY(EditAnywhere, Category = Scenario, SaveGame, meta = (EditInRuntime))
	bool bSetVehicleToAIMode = false;

	UPROPERTY(EditAnywhere, Category = Scenario, SaveGame, meta = (EditInRuntime))
	float VehicleAISpeed = 60;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = EditorModeWidget)
	void OnActorSpawned(AActor * Actor);

public:
	UFUNCTION(BlueprintCallable, Category = RouteBuilder)
	AActor * GetSpawnedActor() { return SpawnedActor;  }
	
public:
	ASpawnPoint();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangedEvent);
#endif // WITH_EDITOR

public:
	/* Override from ISodaActor */
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;

protected:
	UPROPERTY()
	AActor * SpawnedActor = nullptr;
};
