// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "ScenarioActionEvent.h"
#include "ScenarioActionEventLibrary.generated.h"

class ASodaVehicle;
class AScenarioTriggerBase;

UENUM()
enum class EScenarioEventConditional: uint8
{
	LessThan,
	GreaterThan
};

UENUM()
enum class EScenarioEventOverlapEstimation : uint8
{
	Position,
	ClosesPoint
};

UENUM()
enum class EScenarioEventDistanceEstimation : uint8
{
	Longotude,
	Euclidean
};

/**
 * UEventAtScenarioBegin
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventAtScenarioBegin : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventAtScenarioBegin(const FObjectInitializer& InInitializer);
	virtual void ScenarioBegin() override { EventDelegate.ExecuteIfBound(); }
};

/**
 * UEventAtScenarioEnd
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventAtScenarioEnd : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventAtScenarioEnd(const FObjectInitializer& InInitializer);
	virtual void ScenarioEnd() override { EventDelegate.ExecuteIfBound(); }
};

/**
 * UEventAtScenarioTick
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventAtScenarioTick : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventAtScenarioTick(const FObjectInitializer& InInitializer);
	virtual void Tick(float DeltaTime) override { EventDelegate.ExecuteIfBound(); }
};

/**
 * UEventAtScenarioTime
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventAtScenarioTime : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventAtScenarioTime(const FObjectInitializer& InInitializer);
	virtual void ScenarioBegin() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	float ScenarioTime;

protected:
	float CurrentTime;
};

/**
 * UEventOverlapTrigger
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventOverlapTrigger : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventOverlapTrigger(const FObjectInitializer& InInitializer);
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta=(ScenarioAction))
	TSoftObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	TSoftObjectPtr<AScenarioTriggerBase> Trigger;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	EScenarioEventOverlapEstimation OverlapEstimation;


	//float Delay;
};

/**
 * UEventRelativeDistance
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventRelativeDistance : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventRelativeDistance(const FObjectInitializer& InInitializer);
	virtual void ScenarioBegin() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	TSoftObjectPtr<AActor> FromActor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	TSoftObjectPtr<AActor> ToActor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	float Distance;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	EScenarioEventConditional Conditional;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	EScenarioEventOverlapEstimation OverlapEstimation;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	EScenarioEventDistanceEstimation DistanceEstimation;

	//float Delay;

protected:
	FBox FromActorExtent;
	FBox ToActorExtent;
};

/**
 * UEventRelativeSpeed
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventRelativeSpeed : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventRelativeSpeed(const FObjectInitializer& InInitializer);
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	TSoftObjectPtr<AActor> FromActor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	TSoftObjectPtr<AActor> ToActor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	float Speed;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	EScenarioEventConditional Conditional;

	//float Delay; 
};

/**
 * UEventStandStill
 */
UCLASS(meta = (RuntimeMetaData))
class UNREALSODA_API UEventStandStill : public UScenarioActionEvent
{
	GENERATED_BODY()

public:
	UEventStandStill(const FObjectInitializer& InInitializer);
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	AActor* Actor;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	float Duration;

	UPROPERTY(EditAnywhere, Category = Event, SaveGame, meta = (ScenarioAction))
	float SpeedTolerance;

	//float Delay;
};
