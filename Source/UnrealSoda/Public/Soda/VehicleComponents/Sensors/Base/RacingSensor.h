// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/Actors/LapCounter.h"
#include "RacingSensor.generated.h"

class ATrackBuilder;

namespace soda
{

struct FRacingSensorData
{
	// Valid only if bBorderIsValid [cm]
	float LeftBorderOffset;

	// Valid only if bBorderIsValid [cm]
	float RightBorderOffset;

	// Valid only if bBorderIsValid [rad]
	float CenterLineYaw;

	// Valid only if bLapCounterIsValid
	int LapCaunter;

	// Valid only if bLapCounterIsValid [cm]
	float CoveredDistanceCurrentLap;

	// [cm]
	float CoveredDistanceFull;

	bool bBorderIsValid;
	bool bLapCounterIsValid;

	TTimestamp StartTimestemp;

	// Valid only if bLapCounterIsValid
	TTimestamp LapTimestemp;
};

}


/**
 * URacingSensor 
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API URacingSensor : 
	public USensorComponent,
	public ILapCounterTriggeredComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RacingSensor, SaveGame, meta = (EditInRuntime))
	TSoftObjectPtr<ATrackBuilder> CapturedTrackBuilder;

	const soda::FRacingSensorData& GetSensorData() const { return SensorData; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void OnLapCounterTriggerBeginOverlap(ALapCounter* LapCounter, const FHitResult& SweepResult) override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
protected:
	//virtual void OnTriggerLapCpounter();
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData & OutSensorData) { SyncDataset(); return false; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	FVector PrevLocation;
	soda::FRacingSensorData SensorData;
	bool bCapturedTrackBuilder;
};
