// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"
#include "Soda/ISodaActor.h"
#include "ScenarioTriggerActors.generated.h"

class UBillboardComponent;
class UShapeComponent;

/** An actor used to generate collision events (begin/end overlap) in the level. */
UCLASS(ClassGroup=Soda, abstract, ConversionRoot)
class UNREALSODA_API AScenarioTriggerBase : 
	public AActor,
	public ISodaActor
{
	GENERATED_UCLASS_BODY()

private:
	/** Shape component used for collision */
	UPROPERTY(Category = TriggerBase, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShapeComponent> CollisionComponent;

	/** Billboard used to see the trigger in the editor */
	UPROPERTY(Category = TriggerBase, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> SpriteComponent;

public:
	UPROPERTY(EditAnywhere, Category = Scenario, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bHideInScenario = false;

public:
	/** Returns CollisionComponent subobject **/
	UShapeComponent* GetCollisionComponent() const { return CollisionComponent; }

	/** Returns SpriteComponent subobject **/
	UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }

public: 
	virtual void SetActorHiddenInScenario(bool bInHiddenInScenario) override { bHideInScenario = bInHiddenInScenario; }
	virtual bool GetActorHiddenInScenario() const override { return bHideInScenario; }
};

/** A sphere shaped trigger, used to generate overlap events in the level */
UCLASS()
class UNREALSODA_API AScenarioTriggerSphere : public AScenarioTriggerBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float SphereRadius = 40;

public:
	virtual void BeginPlay() override;

	//virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;

	/* Override from ISodaActor */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
};

/** A capsule shaped trigger, used to generate overlap events in the level */
UCLASS()
class UNREALSODA_API AScenarioTriggerCapsule : public AScenarioTriggerBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float CapsuleRadius = 40.0f;

	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float CapsuleHalfHeight = 80.0f;

public:
	virtual void BeginPlay() override;

	//virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;

	/* Override from ISodaActor */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
};

/** A box shaped trigger, used to generate overlap events in the level */
UCLASS()
class UNREALSODA_API AScenarioTriggerBox : public AScenarioTriggerBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	FVector Extent = FVector(40.0f, 40.0f, 40.0f);

public:
	virtual void BeginPlay() override;

	//virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;

	/* Override from ISodaActor */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
};







