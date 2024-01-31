// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/CsvLogger.h"
#include "Soda/UnrealSoda.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "DrawDebugHelpers.h"

UCSVLoggerComponent::UCSVLoggerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("CSV Logger");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePostDeferredPhysTick = true;
}


void UCSVLoggerComponent::PostPhysicSimulationDeferred(float DeltaTimeIn, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& TimestampIn)
{
	Super::PostPhysicSimulationDeferred(DeltaTimeIn, VehicleKinematic, TimestampIn);

	/*
	if (Health != EVehicleComponentHealth::Ok) return;
>>>>>>> release_0.12.0:Source/UnrealSoda/Private/VehicleComponents/Others/CsvLogger.cpp

	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);

	auto DeltaTime = TimestampIn - PrevTimestamp;
	PrevTimestamp = TimestampIn;

	auto LogDeltaTime = TimestampIn - PrevLoggedTimeMark;
	const std::chrono::nanoseconds Period2((int64)(Period * 1000));

	if (LogDeltaTime < Period2)
		return;

	if (LogDeltaTime < Period2)
		PrevLoggedTimeMark += Period2;
	else
		PrevLoggedTimeMark = TimestampIn;

	FVector WorldLoc = GetComponentLocation();
	FRotator WorldRot = GetComponentRotation();
	FVector LocalVel = WorldPose.Rotator().UnrotateVector(WorldVel);

	float DeltaTravel = (WorldLoc - PrevLocaion).Size() / 100;
	PrevLocaion = WorldLoc;

	UVehicleInputComponent* VehicleInput = WheeledVehicle->GetActiveVehicleInput();
	
	if (VehicleInput)
	{
		DistanceTravelled += DeltaTravel;

		float VelMPH = (fabs(LocalVel.X) / 100) * 2.23694f; // convert to mph 

		TimeTravelled += DeltaTime;

		FVector Travel = WorldLoc - StartLocation;

		FString Output;
		Output += FString::FromInt(TimeTravelled) + ", ";
		Output += FString::SanitizeFloat(DistanceTravelled) + ", ";
		Output += FString(LocalVel.X > 0.f ? "Forward" : "Backward") + ", ";
		Output += FString::SanitizeFloat(VelMPH) + ", ";
		Output += FString::SanitizeFloat(LocalAcc.X / 100) + ", ";
		Output += FString::SanitizeFloat(Travel.X / 100) + ", " + FString::SanitizeFloat(Travel.Y / 100) + ", ";
		Output += FString::SanitizeFloat(LocalAcc.Y / 100) + ", ";
		Output += FString::SanitizeFloat(VehicleInput->GetThrottleInput() * 100) + ", ";
		Output += FString::SanitizeFloat(VehicleInput->GetBrakeInput() * 100) + ", ";
		Output += FString::SanitizeFloat(VehicleInput->GetSteeringInput() * 100) + ", ";

		switch (VehicleInput->GetGearInput())
		{
		case ENGear::Park:
			Output += "Park";
			break;

		case ENGear::Reverse:
			Output += "Reverse";
			break;

		case ENGear::Neutral:
			Output += "Neutral";
			break;

		case ENGear::Drive:
			Output += "Drive";
			break;
		}

		if (bAddTimeMarkToNextTick)
		{
			Output += FString::Printf(TEXT(", %lld"), TimestampIn);
			bAddTimeMarkToNextTick = false;
		}

		CsvOutFile << TCHAR_TO_UTF8(*Output) << "\n";
		if (bDrawDebugCanvas)
			UE_LOG(LogSoda, Log, TEXT("UCSVLoggerComponent: %s"), *Output);
	}
	*/
}

void UCSVLoggerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	/*
	if (!IsTickOnCurrentFrame()) return;

	if (GetHealth() == EVehicleComponentHealth::Ok)
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, FString("DATALOG REC"));

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if ((PlayerController != nullptr) && (WheeledVehicle != nullptr) && (WheeledVehicle->IsPlayerControlled()))
	{
		if (PlayerController->WasInputKeyJustPressed(ToggleKeyInput))
		{
			Toggle();
		}

		bAddTimeMarkToNextTick = PlayerController->WasInputKeyJustPressed(PutTimeMarkKeyInput);
	}
	*/

}

bool UCSVLoggerComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	/*

	CsvOutFile.close();

	FString Filename = CvsFileNameBase;
	Filename.Appendf(TEXT("_%d.csv"), CvsFileNameIndex);
	CsvOutFile.open(TCHAR_TO_UTF8(*Filename), std::ofstream::out);

	if (CsvOutFile.is_open())
	{
		CsvOutFile << "Time travelled (ms), Distance travelled (m), Movement direction, Speed(mph), Longitudinal acceleration (m/s2), Movement of the vehicle relative to the starting position (X Y) (m), Lateral acceleration(m/s2), Throttle input(%), Brake input(%), Steering wheel input(%), Gear, TimeMark\n";
		CvsFileNameIndex++;
	}
	else
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}
	*/

	return true;
}

void UCSVLoggerComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	/*
	CsvOutFile.close();
	*/
}