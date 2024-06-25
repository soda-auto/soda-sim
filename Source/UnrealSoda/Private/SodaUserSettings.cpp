// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaUserSettings.h"
//#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigContext.h"
//#include "Misc/FileHelper.h"
//#include "GenericPlatform/GenericApplication.h"
//#include "Misc/App.h"
//#include "EngineGlobals.h"
//#include "Kismet/KismetSystemLibrary.h"
//#include "Kismet/GameplayStatics.h"
//#include "UnrealEngine.h"
//#include "Framework/Application/SlateApplication.h"
//#include "Engine/GameEngine.h"
#include "GameFramework/GameUserSettings.h"

#define GPS_LEAP_SECONDS_OFFSET 17

FString USodaUserSettings::SodaUserSettingsIni = TEXT("SodaUserSettings.ini");

USodaUserSettings::USodaUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetToDefaults();
}

void USodaUserSettings::LoadSettings(bool bForceReload/*=false*/)
{
	/*
	if (bForceReload)
	{
		if (OnUpdateGameUserSettingsFileFromCloud.IsBound())
		{
			FString IniFileLocation = FPaths::GeneratedConfigDir() + UGameplayStatics::GetPlatformName() + "/" + GGameUserSettingsIni + ".ini";
		}

		LoadConfigIni(bForceReload);
	}
	*/

	LoadConfig(GetClass(), *SodaUserSettingsIni);
}

void USodaUserSettings::SaveSettings()
{
	SaveConfig(CPF_Config, *SodaUserSettingsIni);
	ApplyGraphicSettings();
}

void USodaUserSettings::LoadConfigIni(bool bForceReload/*=false*/)
{
	FConfigContext Context = FConfigContext::ReadIntoGConfig();
	Context.bForceReload = bForceReload;
	Context.Load(TEXT("SodaUserSettings"), SodaUserSettingsIni);
}

void USodaUserSettings::SetToDefaults()
{
	KeyCruiseControl = EKeys::Divide;
	KeySpeedLimiter = EKeys::Multiply;
	ThrottleKeyInput = FKey("W");
	SteeringLeftKeyInput = FKey("A");
	SteeringRigtKeyInput = FKey("D");
	BrakeKeyInput = FKey("S");
	NeutralGearKeyInput = FKey("One");
	DriveGearKeyInput = FKey("Two");
	ReverseGearKeyInput = FKey("Three");
	ParkGearKeyInput = FKey("Four");
	GearUpKeyInput = FKey("E");
	GearDownKeyInput = FKey("Q");
	ChangeModeKeyInput = FKey("M");
	SafeStopKeyInput = FKey("K");
	HeadlightsKeyInput = FKey("P");
	HornKeyInput = FKey("B");
	ZoomUpKeyInput = FKey("MouseScrollDown");
	ZoomDownKeyInput = FKey("MouseScrollUp");

	ThrottleJoyInput = FKey("SodaAxis2");
	SteeringJoyInput = FKey("SodaAxis0");
	BrakeJoyInput = FKey("SodaAxis3");
	NeutralGearJoyInput = FKey("SodaButton0");
	DriveGearJoyInput = FKey("SodaButton1");
	ReverseGearJoyInput = FKey("SodaButton2");
	ParkGearJoyInput = FKey("SodaButton3");
	GearUpJoyInput = FKey("SodaButton4");
	GearDownJoyInput = FKey("SodaButton5");

	DPIScale = 1.0f;
	VehicleDebugAreaWidth = 400;
	bTagActorsAtBeginPlay = true;

	MongoURL = "mongodb://localhost:27017";
	DatabaseName = "sodasim";
	bAutoConnect = false;
	bRecordDataset = false;

	GPSLeapOffset = GPS_LEAP_SECONDS_OFFSET;
}

void USodaUserSettings::ReadGraphicSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	ResolutionScale = Settings->GetResolutionScaleNormalized() * 100;
	ViewDistanceQuality = (EQualityLevel)Settings->GetViewDistanceQuality();
	AntiAliasingQuality = (EQualityLevel)Settings->GetAntiAliasingQuality();
	ShadowQuality = (EQualityLevel)Settings->GetShadowQuality();
	GlobalIlluminationQuality = (EQualityLevel)Settings->GetGlobalIlluminationQuality();
	ReflectionQuality = (EQualityLevel)Settings->GetReflectionQuality();
	PostProcessQuality = (EQualityLevel)Settings->GetPostProcessingQuality();
	TextureQuality = (EQualityLevel)Settings->GetTextureQuality();
	EffectsQuality = (EQualityLevel)Settings->GetVisualEffectQuality();
	FoliageQuality = (EQualityLevel)Settings->GetFoliageQuality();
	ShadingQuality = (EQualityLevel)Settings->GetShadingQuality();
	FrameRateLimit = Settings->GetFrameRateLimit();
}

void USodaUserSettings::ApplyGraphicSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	Settings->SetResolutionScaleValueEx(ResolutionScale);
	Settings->SetViewDistanceQuality((int32)ViewDistanceQuality);
	Settings->SetAntiAliasingQuality((int32)AntiAliasingQuality);
	Settings->SetShadowQuality((int32)ShadowQuality);
	Settings->SetGlobalIlluminationQuality((int32)GlobalIlluminationQuality);
	Settings->SetReflectionQuality((int32)ReflectionQuality);
	Settings->SetPostProcessingQuality((int32)PostProcessQuality);
	Settings->SetTextureQuality((int32)TextureQuality);
	Settings->SetVisualEffectQuality((int32)EffectsQuality);
	Settings->SetFoliageQuality((int32)FoliageQuality);
	Settings->SetShadingQuality((int32)ShadingQuality);
	Settings->SetFrameRateLimit((int32)FrameRateLimit);

	Settings->ApplySettings(false);
	
}