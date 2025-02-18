// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/SodaUserSettings.h"
#include "Soda/SodaTypes.h"
#include "InputCoreTypes.h"
#include "Engine/EngineTypes.h"
#include "SodaCommonSettings.generated.h"

#define GPS_EPOCH_OFFSET 315964800

UENUM(BlueprintType)
enum class EQualityLevel : uint8
{
	Low,
	Medium,
	High,
	Epic,
	Cinematic
};

UCLASS(ClassGroup = Soda)
class UNREALSODA_API USodaCommonSettings : public USodaUserSettings
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
	FKey GearUpKeyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleKeyInput, meta = (EditInRuntime))
	FKey GearDownKeyInput;

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

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey GearUpJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = VehicleJoyInput, meta = (EditInRuntime))
	FKey GearDownJoyInput;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = UI, meta = (EditInRuntime))
	float DPIScale;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = UI, meta = (EditInRuntime))
	bool bIsDrawVehicleDebugPanel = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = UI, meta = (EditInRuntime))
	float VehicleDebugAreaWidth;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	bool bTagActorsAtBeginPlay;

	/** Resolution scale [0..100] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	float ResolutionScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel ViewDistanceQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel AntiAliasingQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel ShadowQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel GlobalIlluminationQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel ReflectionQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel PostProcessQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel TextureQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel EffectsQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel FoliageQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	EQualityLevel ShadingQuality;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	//EQualityLevel LandscapeQuality;

	/** Sets the user's frame rate limit (0 will disable frame rate limiting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphic Settings", meta = (EditInRuntime))
	float FrameRateLimit;

	/** [s] */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	int GPSLeapOffset;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	TEnumAsByte<ECollisionChannel>  SelectTraceChannel = ECollisionChannel::ECC_GameTraceChannel16;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	TEnumAsByte<ECollisionChannel>  RadarCollisionChannel = ECollisionChannel::ECC_GameTraceChannel11;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Advanced, meta = (EditInRuntime))
	EScenarioStopMode ScenarioStopMode = EScenarioStopMode::RestartLevel;

	UPROPERTY(config)
	bool bShowQuickStartAtStartUp = true;;

	UPROPERTY(config)
	TSet<FName> DatasetNamesToUse;

public:
	UFUNCTION(BlueprintCallable, Category = UI)
	float GetDPIScale() { return FMath::Clamp(DPIScale, 0.3f, 3.0f); }

	/** [s] */
	UFUNCTION(BlueprintCallable, Category = GPS)
	int GetGPSTimestempOffset() const { return GPSLeapOffset - GPS_EPOCH_OFFSET; }

public:
	/** Loads the user settings from persistent storage */
	//UFUNCTION(BlueprintCallable, Category = Settings)
	//virtual void LoadSettings(bool bForceReload = false);

	/** Save the user settings to persistent storage (automatically happens as part of ApplySettings) */
	//UFUNCTION(BlueprintCallable, Category = Settings)
	//virtual void SaveSettings();

	/** Loads the user .ini settings into GConfig */
	//static void LoadConfigIni(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "Graphic Settings")
	void ReadGraphicSettings();

	UFUNCTION(BlueprintCallable, Category = "Graphic Settings", meta=(CallInRuntime))
	void ApplyGraphicSettings();

	virtual void ResetToDefault() override;
	virtual void OnPreOpen() override { ReadGraphicSettings(); }
	virtual FText GetMenuItemText() const override;
	virtual FText GetMenuItemDescription() const override;
	virtual FName GetMenuItemIconName() const override;

protected:
	//static FString SodaUserSettingsIni;
};
