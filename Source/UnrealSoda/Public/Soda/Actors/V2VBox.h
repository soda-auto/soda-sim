// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Soda/ISodaActor.h"
#include "Soda/VehicleComponents/Others/V2V.h"
#include "V2VBox.generated.h"


 /**
  * ANoiseBox
  */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AV2VBox :
	public AActor,
	public ISodaActor
{
GENERATED_BODY()

public:
	UPROPERTY(Category = Default, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBillboardComponent *SpriteComponent;

	UPROPERTY(Category = Default, VisibleDefaultsOnly, BlueprintReadOnly)
	class UBoxComponent* TriggerVolume;

	/** Please add a variable description */
	UPROPERTY(Category = Default, VisibleAnywhere, BlueprintReadOnly)
	UV2VTransmitterComponent* V2VTransmitter;

	UPROPERTY(Category = V2VBox, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FVector Extent = FVector(40.0f, 40.0f, 40.0f);

	UPROPERTY(Category = V2VBox, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	int ID = 0;

	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	AV2VBox();

	virtual void BeginPlay() override;

public:
	/* Override from ISodaActor */
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	//virtual void ScenarioBegin() override;
	//virtual void ScenarioEnd() override;
};
