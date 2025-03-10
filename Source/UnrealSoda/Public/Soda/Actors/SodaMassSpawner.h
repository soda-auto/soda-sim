// SodaMassSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "Soda/ISodaActor.h"
#include "SodaMassSpawner.generated.h"

UCLASS()
class UNREALSODA_API ASodaMassSpawner : public AMassSpawner, public ISodaActor
{
	GENERATED_BODY()

public:
	ASodaMassSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traffic, SaveGame, meta = (EditInRuntime))
	bool bSpawnTrafficOnScenarioStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traffic, SaveGame, meta = (EditInRuntime))
	int32 OverridenCount = 20;

	UFUNCTION(BlueprintCallable, Category = "Traffic", CallInEditor)
	void ToggleSpawnTraffic();

	void SpawnTraffic();
	void DespawnTraffic();

	// ISodaActor interface
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override;
	virtual bool GetActorHiddenInScenario() const override;
	virtual bool ShowSelectBox() const override { return false; }
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;

private:
	bool bHiddenInScenario = false;
};
