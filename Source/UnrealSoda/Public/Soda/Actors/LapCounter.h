// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Soda/ISodaActor.h"
#include "LapCounter.generated.h"

UINTERFACE(BlueprintType)
class UNREALSODA_API ULapCounterTriggeredComponent : public UInterface
{
	GENERATED_BODY()
};

class UNREALSODA_API ILapCounterTriggeredComponent
{
	GENERATED_BODY()

public:
	virtual void OnLapCounterTriggerBeginOverlap(ALapCounter* LapCounter, const FHitResult& SweepResult) = 0;
};

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ALapCounter :
	public AActor,
	public ISodaActor
{
GENERATED_BODY()

public:
	UPROPERTY(Category = LapCounter, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBillboardComponent *SpriteComponent;

	UPROPERTY(Category = LapCounter, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBoxComponent* TriggerVolume;

	UPROPERTY(Category = LapCounter, VisibleDefaultsOnly, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FVector Extent;

	UPROPERTY(Category = LapCounter, VisibleDefaultsOnly, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDrawBox = true;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadOnly, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	UFUNCTION(BlueprintCallable, Category = LapCounter, meta = (CallInRuntime))
	void UpdatetActor();
	
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

public:
	ALapCounter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
