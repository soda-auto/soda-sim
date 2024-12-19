// SodaMassSpawner.cpp

#include "Soda/Actors/SodaMassSpawner.h"
#include "MassSpawnerSubsystem.h"
#include "MassSimulationSubsystem.h"
#include "Components/PrimitiveComponent.h"

ASodaMassSpawner::ASodaMassSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASodaMassSpawner::BeginPlay()
{
	Count = OverridenCount;
	Super::BeginPlay();
}

void ASodaMassSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASodaMassSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASodaMassSpawner::ToggleSpawnTraffic()
{
	bSpawnTrafficOnScenarioStart = !bSpawnTrafficOnScenarioStart;
}

void ASodaMassSpawner::SpawnTraffic()
{
	DoSpawning();
}

void ASodaMassSpawner::DespawnTraffic()
{
	DoDespawning();
}

const FSodaActorDescriptor* ASodaMassSpawner::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Mass Spawner"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("Icons.Box"), /*Icon*/
		false, /*bAllowTransform*/
		false, /*bAllowSpawn*/
		FVector(0, 0, 50) /*SpawnOffset*/
	};
	return &Desc;
}

void ASodaMassSpawner::SetActorHiddenInScenario(bool bInHiddenInScenario)
{
	bHiddenInScenario = bInHiddenInScenario;
	SetActorHiddenInGame(bInHiddenInScenario);
}

bool ASodaMassSpawner::GetActorHiddenInScenario() const
{
	return bHiddenInScenario;
}

void ASodaMassSpawner::ScenarioBegin()
{
	Count = OverridenCount;
	if (bSpawnTrafficOnScenarioStart)
	{
		SpawnTraffic();
	}
}

void ASodaMassSpawner::ScenarioEnd()
{
	DespawnTraffic();
}
