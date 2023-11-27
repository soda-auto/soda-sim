// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineTypes.h"
#include "SodaUserSettings.generated.h"

#define GPS_EPOCH_OFFSET 315964800

UCLASS(ClassGroup = Soda, config = SodaUserSettings, configdonotcheckdefaults)
class UNREALSODA_API USodaUserSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ThrottleKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey SteeringLeftKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey SteeringRigtKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey BrakeKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey NeutralGearKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey DriveGearKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ReverseGearKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ParkGearKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ChangeModeKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey SafeStopKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ZoomUpKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey ZoomDownKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey HeadlightsKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey HornKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey KeyCruiseControl;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey KeySpeedLimiter;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey ThrottleJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey SteeringJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey BrakeJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey NeutralGearJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey DriveGearJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey ReverseGearJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey ParkGearJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = UI, meta = (EditInRuntime))
	float DPIScale;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = UI, meta = (EditInRuntime))
	float VehicleDebugAreaWidth;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = DB, meta = (EditInRuntime))
	FString MongoURL;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = DB, meta = (EditInRuntime))
	FString DatabaseName;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = DB, meta = (EditInRuntime))
	bool bRecordDataset;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = DB, meta = (EditInRuntime))
	bool bAutoConnect;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Common, meta = (EditInRuntime))
	bool bTagActorsAtBeginPlay;

	/** [s] */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	int GPSLeapOffset;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	TEnumAsByte<ECollisionChannel>  SelectTraceChannel = ECollisionChannel::ECC_GameTraceChannel16;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	TEnumAsByte<ECollisionChannel>  RadarCollisionChannel = ECollisionChannel::ECC_GameTraceChannel11;

public:
	UFUNCTION(BlueprintCallable, Category = UI)
	float GetDPIScale() { return FMath::Clamp(DPIScale, 0.3f, 3.0f); }

	/** [s] */
	UFUNCTION(BlueprintCallable, Category = GPS)
	int GetGPSTimestempOffset() { return GPSLeapOffset - GPS_EPOCH_OFFSET; }

public:
	/** Loads the user settings from persistent storage */
	UFUNCTION(BlueprintCallable, Category=Settings)
	virtual void LoadSettings(bool bForceReload = false);

	/** Save the user settings to persistent storage (automatically happens as part of ApplySettings) */
	UFUNCTION(BlueprintCallable, Category=Settings)
	virtual void SaveSettings();

	UFUNCTION(BlueprintCallable, Category=Settings)
	virtual void SetToDefaults();

	/** Loads the user .ini settings into GConfig */
	static void LoadConfigIni(bool bForceReload = false);

protected:
	static FString SodaUserSettingsIni;
};
