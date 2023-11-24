// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "InputCoreTypes.h"
#include <fstream>
#include "CsvLogger.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCSVLoggerComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float Period = 0.1f; // (sec) Logging period

	UPROPERTY(BlueprintReadWrite, Category = Sensor, meta = (EditInRuntime))
	bool bAddTimeMarkToNextTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	FString CvsFileNameBase = "log";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	FKey ToggleKeyInput = FKey("EIGHT");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	FKey PutTimeMarkKeyInput = FKey("NINE");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame)
	int CvsFileNameIndex = 0;

	std::int64_t TimeTravelled = 0; // (ms)
	float DistanceTravelled; // (m)

	FVector PrevLocaion;
	FVector StartLocation;
	std::int64_t PrevTimestamp = 0;
	std::int64_t PrevLoggedTimeMark= 0;

	std::ofstream CsvOutFile;
};
