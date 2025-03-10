// Fill out your copyright notice in the Description page of Project Settings.


#include "Soda/Actors/RoadConesBuilder.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ARoadConesBuilder::ARoadConesBuilder()
{

	PrimaryActorTick.bCanEverTick = false;

}




void ARoadConesBuilder::UpdateGeneration()
{


	// Clean up existing actors

	for (auto& Actor : SpawnedActors)
	{
		Actor->Destroy();
	}

	SpawnedActors.Empty();

	if (!RoadConeBP)
	{
		return;
	}


	FVector RootPosition = GetActorLocation();
	FRotator RootRotation = GetActorRotation();

	// Generate new ones

	for (int32 i = 0; i < NumberOfCones; i++)
	{

		FVector PositionToSpawn; 


		switch (ConePlacementPattern)
		{
		case EConePlacements::RightSide:
			PositionToSpawn = RootPosition + RootRotation.RotateVector(FVector((float)i * DistanceBetweenCones, RoadWidth/2, 0));
			SpawnNewCone(PositionToSpawn);
			break;
		case EConePlacements::BothSides:
			PositionToSpawn = RootPosition + RootRotation.RotateVector(FVector((float)i * DistanceBetweenCones, -RoadWidth / 2, 0));
			SpawnNewCone(PositionToSpawn);
			PositionToSpawn = RootPosition + RootRotation.RotateVector(FVector((float)i * DistanceBetweenCones, RoadWidth / 2, 0));
			SpawnNewCone(PositionToSpawn);
			break;
		case EConePlacements::LeftSide:
			PositionToSpawn = RootPosition + RootRotation.RotateVector(FVector((float)i*DistanceBetweenCones, -RoadWidth / 2, 0));
			SpawnNewCone(PositionToSpawn);
			break;

		}

	}	

}


void ARoadConesBuilder::SpawnNewCone(FVector Position)
{

	FActorSpawnParameters SpawnParams;
	AActor* NewCone = GetWorld()->SpawnActor<AActor>(RoadConeBP, Position, FRotator::ZeroRotator, SpawnParams);

	if (NewCone)
	{
		SpawnedActors.Add(NewCone);
	}

}


void ARoadConesBuilder::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();

	UpdateGeneration();
}

void ARoadConesBuilder::BeginPlay()
{
	Super::BeginPlay();

	UpdateGeneration();

 }