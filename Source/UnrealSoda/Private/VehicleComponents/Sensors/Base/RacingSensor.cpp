// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Base/RacingSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Soda/SodaStatics.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Soda/LevelState.h"
#include "UObject/UObjectIterator.h"
#include "Soda/Actors/TrackBuilder.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"


URacingSensor::URacingSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Racing Sensor");
	GUI.IcanName = TEXT("SodaIcons.Modem");
	//GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
}

bool URacingSensor::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PrevLocation = this->GetComponentTransform().GetLocation();
	SensorData.CoveredDistanceCurrentLap = 0;
	SensorData.CoveredDistanceFull = 0;
	SensorData.LapCaunter = -1;
	SensorData.StartTimestemp = soda::Now();

	return true;
}

void URacingSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;


	if (CapturedTrackBuilder.IsValid())
	{
		SensorData.bBorderIsValid = CapturedTrackBuilder->FindNearestBorder(this->GetComponentTransform(), SensorData.LeftBorderOffset, SensorData.RightBorderOffset, SensorData.CenterLineYaw);
	}
	else
	{
		SensorData.bBorderIsValid = false;
	}

	const FVector Location = GetComponentTransform().GetLocation();
	const float Dist = (PrevLocation - Location).Length();
	PrevLocation = Location;

	if (SensorData.LapCaunter >= 0)
	{
		SensorData.CoveredDistanceCurrentLap += Dist;
		SensorData.bLapCounterIsValid = true;
	}
	else
	{
		SensorData.bLapCounterIsValid = false;
	}

	SensorData.CoveredDistanceFull += Dist;

	PublishSensorData(DeltaTime, GetHeaderGameThread(), SensorData);
}

void URacingSensor::OnLapCounterTriggerBeginOverlap(ALapCounter* LapCounter, const FHitResult& SweepResult)
{
	++SensorData.LapCaunter;
	SensorData.CoveredDistanceCurrentLap = 0;
	SensorData.LapTimestemp = soda::Now();
}

void URacingSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && (GetHealth() == EVehicleComponentHealth::Ok))
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bBorderIsValid: %s"), (SensorData.bBorderIsValid ? TEXT("true") : TEXT("false"))), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("bLapCounterIsValid: %s"), (SensorData.bLapCounterIsValid ? TEXT("true") : TEXT("false"))), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("LeftBorderOffset: %f"), SensorData.LeftBorderOffset), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("RightBorderOffset: %f"), SensorData.RightBorderOffset), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("CenterLineYaw: %f"), SensorData.CenterLineYaw / M_PI * 180), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("LapCaunter: %i"), SensorData.LapCaunter), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("CoveredDistanceCurrentLap: %f"), SensorData.CoveredDistanceCurrentLap), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("CoveredDistanceFull: %f"), SensorData.CoveredDistanceFull), 16, YPos);
	}
}

void URacingSensor::OnPushDataset(soda::FActorDatasetData& Dataset) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	try
	{
		Dataset.GetRowDoc()
			<< std::string(TCHAR_TO_UTF8(*GetName())) << open_document
			<< "bBorderIsValid" << SensorData.bBorderIsValid
			<< "bLapCounterIsValid" << SensorData.bLapCounterIsValid
			<< "CenterLineYaw" << SensorData.CenterLineYaw
			<< "CoveredDistanceCurrentLap" << SensorData.CoveredDistanceCurrentLap
			<< "CoveredDistanceFull" << SensorData.CoveredDistanceFull
			<< "LapCaunter" << SensorData.LapCaunter
			<< "LeftBorderOffset" << SensorData.LeftBorderOffset
			<< "RightBorderOffset" << SensorData.RightBorderOffset
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}