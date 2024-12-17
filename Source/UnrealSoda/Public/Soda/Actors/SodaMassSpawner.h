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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traffic, meta = (EditInRuntime))
	bool bSpawnTrafficOnScenarioStart = false;

	UFUNCTION(BlueprintCallable, Category = "Traffic", CallInEditor)
	void ToggleSpawnTraffic();

	void SpawnTraffic();
	void DespawnTraffic();

	// ISodaActor interface
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override;
	virtual bool GetActorHiddenInScenario() const override;
	virtual bool ShowSelectBox() const override { return false; }

private:
	bool bHiddenInScenario = false;
};
