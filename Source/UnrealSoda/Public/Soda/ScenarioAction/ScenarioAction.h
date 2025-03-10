// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Soda/ISodaActor.h"
#include "ScenarioAction.generated.h"

class USodaSubsystem;
class UScenarioActionBlock;
class SScenarioActionEditor;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AScenarioAction 
	: public AActor
	, public ISodaActor
{
	GENERATED_BODY()

public:

	//UPROPERTY()
	//TArray<UScenarioInitialization> ScenarioInitializations;

	UPROPERTY()
	TArray<UScenarioActionBlock*> ScenarioBlocks;

	//UPROPERTY()
	//TArray<UScenarioFinalization> ScenarioFinalization;

public:
	UScenarioActionBlock* CreateNewBlock();
	bool RemoveBlock(UScenarioActionBlock* Block);

	void CloseEditorWindow();

public:
	/* Override from ISodaActor */
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;
	virtual TSharedPtr<SWidget> GenerateToolBar() override;
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;



public:
	AScenarioAction();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	TSharedPtr<SScenarioActionEditor> ScenarioActionEditor;
};
