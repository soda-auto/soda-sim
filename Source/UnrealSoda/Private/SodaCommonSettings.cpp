// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaCommonSettings.h"
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

#define LOCTEXT_NAMESPACE "SodaCommonSettings"

#define GPS_LEAP_SECONDS_OFFSET 17

//FString USodaCommonSettings::SodaUserSettingsIni = TEXT("SodaUserSettings.ini");

USodaCommonSettings::USodaCommonSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetToDefault();
}

FName USodaCommonSettings::GetMenuItemIconName() const
{
	static FName IconName = "Icons.Adjust";
	return IconName;
}
FText USodaCommonSettings::GetMenuItemText() const
{
	return LOCTEXT("SodaCommonSettings_Description", "Common Settings");
}

FText USodaCommonSettings::GetMenuItemDescription() const
{
	return LOCTEXT("SodaCommonSettings_Description", "SODA.Sim common settings");
}

/*
void USodaCommonSettings::LoadSettings(bool bForceReload)
{
	
	//if (bForceReload)
	//{
	//	if (OnUpdateGameUserSettingsFileFromCloud.IsBound())
	//	{
	//		FString IniFileLocation = FPaths::GeneratedConfigDir() + UGameplayStatics::GetPlatformName() + "/" + GGameUserSettingsIni + ".ini";
	//	}

	//	LoadConfigIni(bForceReload);
	//}
	

	LoadConfig(GetClass(), *SodaUserSettingsIni);
}

void USodaCommonSettings::SaveSettings()
{
	SaveConfig(CPF_Config, *SodaUserSettingsIni);
	ApplyGraphicSettings();
}

void USodaCommonSettings::LoadConfigIni(bool bForceReload)
{
	FConfigContext Context = FConfigContext::ReadIntoGConfig();
	Context.bForceReload = bForceReload;
	Context.Load(TEXT("SodaUserSettings"), SodaUserSettingsIni);
}
*/

void USodaCommonSettings::ResetToDefault()
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

	GPSLeapOffset = GPS_LEAP_SECONDS_OFFSET;
}

void USodaCommonSettings::ReadGraphicSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	check(Settings);

	Settings->LoadSettings();

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

void USodaCommonSettings::ApplyGraphicSettings()
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
	//Settings->SetLandscapeQuality((int32)LandscapeQuality);
	Settings->SetFrameRateLimit((int32)FrameRateLimit);

	Settings->ApplySettings(false);

}
