// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Soda/ISodaActor.h"
#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"
#include "NoiseBox.generated.h"

/**
 * ANoiseBox
 * This is a trigger inside of which new FImuNoiseParams will be assigned to any UImuSensorComponent.
 * When the sensor leaves the trigger, the previous values ??will be returned to it.
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ANoiseBox : 
	public AActor,
	public ISodaActor
{
GENERATED_BODY()

public:
	UPROPERTY(Category = NoiseBox, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBillboardComponent *SpriteComponent;

	UPROPERTY(Category = NoiseBox, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBoxComponent* TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NoiseBox, SaveGame, meta = (EditInRuntime))
	TMap< FString, FImuNoiseParams > NoisePresets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NoiseBox, SaveGame, meta = (EditInRuntime))
	FString DefaultProfileName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NoiseBox, SaveGame, meta = (EditInRuntime))
	FImuNoiseParams DefaultNoiseParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NoiseBox, SaveGame, meta = (EditInRuntime))
	FVector Extent = FVector(40.0f, 40.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scenario, SaveGame, meta = (EditInRuntime))
	bool bActiveOnlyIfScenario = true;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;
	
public:
	ANoiseBox();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = NoiseBox, meta = (CallInRuntime))
	void UpdateDefaultNoiseParams();

protected:
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

//#if WITH_EDITOR
//	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
//#endif

protected:
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	//virtual void ScenarioBegin() override;
	//virtual void ScenarioEnd() override;

};
