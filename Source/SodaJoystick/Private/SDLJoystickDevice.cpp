// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SDLJoystickDevice.h"
#include "Framework/Application/SlateApplication.h"
#include "SodaJoystickPrivatePCH.h"
#include "IInputInterface.h"
#include <algorithm>

DEFINE_LOG_CATEGORY(SodaSDLJoystickDevice);


FSDLJoystickDevice::FSDLJoystickDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) :
	MessageHandler(InMessageHandler)
{
	Settings = GetMutableDefault<UJoystickGameSettings>();

	if (SDL_WasInit(0) != 0)
	{
	//	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_WasInit(0) - SDL already loaded"));
	}
	else
	{
	//	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDLJoystickDevice::InitSDL() SDL init 0"));
		SDL_Init(0);
	}
	
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Initialize the joystick subsystem
	int result = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	result = SDL_InitSubSystem(SDL_INIT_HAPTIC);

	if (!SDLStartup())
	{
	//	ActualizeAxesNum();
	//	ActualizeButtonsNum();
	}
}


void FSDLJoystickDevice::SetCentringForce(int Percent)
{
	CentringForceMax = Percent;
}


FSDLJoystickDevice::~FSDLJoystickDevice()
{
	// Close your device here
}


void FSDLJoystickDevice::Tick(float DeltaTime)
{
	if (!Joys.Num())
	{
		if (NoJoyWaitTimer < 0)
		{
			SDLStartup();
			NoJoyWaitTimer = NoJoyWaitTime;
		}
		else
			NoJoyWaitTimer -= DeltaTime;
	}
}


void FSDLJoystickDevice::SendControllerEvents()
{
	if (SDL_NumJoysticks() <= 0 || !Joys.Num())
		return;

	// Poll your device here and fire off events related to its current state
	SDL_JoystickUpdate();

	FInputDeviceId PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(FSlateApplicationBase::SlateAppPrimaryPlatformUser);

	for (int i = 0; i < AxesNum; ++i)
	{
		int AxixValue = (int)SDL_JoystickGetAxis(Joys[AnalogAxes[i].DeviceIndex], AnalogAxes[i].Index);
		FSlateApplication::Get().OnControllerAnalog(*(AnalogAxes[i].Name), FSlateApplicationBase::SlateAppPrimaryPlatformUser, PrimaryInputDevice, AnalogAxes[i].SettingsStruct->GetScaledValue(AxixValue));
		if (Settings)
		{
			Settings->CurrentAxesValues[i] = AnalogAxes[i].SettingsStruct->GetScaledValue(AxixValue);
		}

		//		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Sending dummy analog controller event! Name = %s,  Value = %d"), *(AnalogAxes[i].Name), AxixValue);
	}

	for (int i = 0; i < ButtonsNum; ++i)
	{
		int ButtonValue = SDL_JoystickGetButton(Joys[Buttons[i].DeviceIndex], i);
		FSlateApplication::Get().OnControllerAnalog(*Buttons[i].Name, FSlateApplicationBase::SlateAppPrimaryPlatformUser, PrimaryInputDevice, ButtonValue);

		if (ButtonValue && Settings)
		{
			Settings->LastButtonDown = FText::FromString(FString::Printf(TEXT("%s"), *Buttons[i].Key.ToString()));
		}

		//		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Button %d: Value = %d"), i, ButtonValue);
	}
}


void FSDLJoystickDevice::SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
{
	MessageHandler = InMessageHandler;
}


bool FSDLJoystickDevice::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// Nothing necessary to do (boilerplate code to complete the interface)
	return false;
}


void FSDLJoystickDevice::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
	//	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SetChannelValue"));
		// Nothing necessary to do (boilerplate code to complete the interface)
}


void FSDLJoystickDevice::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values)
{
	//	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SetChannelValues"));
		// Nothing necessary to do (boilerplate code to complete the interface)
}

void FSDLJoystickDevice::UpdateEKeys(int NumAxes, int NumButtons)
{
#define LOCTEXT_NAMESPACE "InputKeys"

	for (int i = RegisteredAxesNum; i < NumAxes; ++i)
	{
		EKeys::AddKey(FKeyDetails(
			FKey(*FString::Printf(TEXT("SodaAxis%d"), i)),
			FText::Format(LOCTEXT("Soda axis", "Soda Analog Axis {0}"), i),
			FKeyDetails::GamepadKey | FKeyDetails::Axis1D,
			TEXT("SodaJoy")));
	}

	for (int i = RegisteredButtonsNum; i < NumButtons; ++i)
	{
		EKeys::AddKey(FKeyDetails(
			FKey(*FString::Printf(TEXT("SodaButton%d"), i)),
			FText::Format(LOCTEXT("Soda button", "Soda Button {0}"), i),
			FKeyDetails::GamepadKey,
			TEXT("SodaJoy")));
	}
#undef LOCTEXT_NAMESPACE

	RegisteredAxesNum = std::max(RegisteredAxesNum, NumAxes);
	RegisteredButtonsNum = std::max(RegisteredButtonsNum, NumButtons);
}

bool FSDLJoystickDevice::SDLStartup()
{
	if (SDL_NumJoysticks() <= 0)
	{
		UpdateEKeys(20, 20); //Registre 20 Axes & 20 Butonts by default
		return false;
	}

	AxesNum = 0;
	ButtonsNum = 0;

	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDLStartup"));
	UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Found %d joysticks:\n"), SDL_NumJoysticks());
	for (size_t i = 0; i < SDL_NumJoysticks(); i++)
	{
		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("\tJoystick %d: %s"), i, *FString(SDL_JoystickNameForIndex(i)));
		if (SDL_IsGameController(i))
		{
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT(" - game controller.\n"));
		}
		else
		{
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT(".\n"));
		}
	}
	int CurAxis = 0;
	int CurButton = 0;

	// Check for joystick
	for (int i = 0; i < SDL_NumJoysticks(); ++i) 
	{

		if (SDL_IsGameController(i))
		{
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Selected joystick %d is game controller and will by opened by UE4.\n"), i);
			return false;
		}
		// Open joystick
		Joys.Add(SDL_JoystickOpen(i));

		if (Joys.Last()) {
			AxesNum += SDL_JoystickNumAxes(Joys.Last());
			ButtonsNum += SDL_JoystickNumButtons(Joys.Last());

			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Opened Joystick %d\n"), i);
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("\tName: %s\n"), *FString(SDL_JoystickNameForIndex(i)));
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("\tNumber of Axes: %d\n"), SDL_JoystickNumAxes(Joys.Last()));
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("\tNumber of Buttons: %d\n"), SDL_JoystickNumButtons(Joys.Last()));
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("\tNumber of Balls: %d\n"), SDL_JoystickNumBalls(Joys.Last()));
		}
		else {
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Couldn't open Joystick %i\n"), i);
		}

		Settings->SetAnalogAxesNum(AxesNum);

		for (int j = 0; j < SDL_JoystickNumAxes(Joys.Last()); ++j)
		{
			AnalogAxes.push_back(UAxis(*FString::Printf(TEXT("SodaAxis%d"), CurAxis), i, j));
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("AddKey %s"), *(AnalogAxes.back().Name));
			AnalogAxes.back().SettingsStruct = Settings->GetAxisSettings(CurAxis);
			if (AnalogAxes.back().SettingsStruct) AnalogAxes.back().SettingsStruct->Name = AnalogAxes.back().Name;
			CurAxis++;
		}

		if (Settings->VirtualButtonsNum < ButtonsNum)
		{
			Settings->VirtualButtonsNum = ButtonsNum;
		}

		for (int b = 0; b < SDL_JoystickNumButtons(Joys.Last()); ++b)
		{
			Buttons.push_back(UAxis(*FString::Printf(TEXT("SodaButton%d"), CurButton), i, b));
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("AddKey %s"), *Buttons.back().Name);
			CurButton++;
		}

		// Open the device
		Haptics.Add(SDL_HapticOpenFromJoystick(Joys.Last()));
		if (Haptics.Last() == 0)
			UE_LOG(SodaSDLJoystickDevice, Error, TEXT("SDL_HapticOpenFromJoystick[%d] - %s"), i, UTF8_TO_TCHAR(SDL_GetError()));
		if (Haptics.Num() > i)
		{
			SDL_HapticSetAutocenter(Haptics[i], 0);

			// Check to see what it supports
			unsigned int haptic_query = 0; //Properties of the haptic device
			UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuerycrequest for Joy[%d]:"), Haptics.Num() - 1);
			haptic_query = SDL_HapticQuery(Haptics[i]);

			if (haptic_query & SDL_HAPTIC_CONSTANT)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_CONSTANT"));
			if (haptic_query & SDL_HAPTIC_SINE)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_SINE"));
			if (haptic_query & SDL_HAPTIC_TRIANGLE)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_TRIANGLE"));
			if (haptic_query & SDL_HAPTIC_SAWTOOTHUP)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_SAWTOOTHUP"));
			if (haptic_query & SDL_HAPTIC_SAWTOOTHDOWN)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_SAWTOOTHDOWN "));
			if (haptic_query & SDL_HAPTIC_SPRING)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_SPRING  "));
			if (haptic_query & SDL_HAPTIC_DAMPER)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_DAMPER"));
			if (haptic_query & SDL_HAPTIC_INERTIA)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_INERTIA"));
			if (haptic_query & SDL_HAPTIC_FRICTION)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_FRICTION"));
			if (haptic_query & SDL_HAPTIC_CUSTOM)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_CUSTOM"));
			if (haptic_query & SDL_HAPTIC_GAIN)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_GAIN"));
			if (haptic_query & SDL_HAPTIC_AUTOCENTER)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_AUTOCENTER"));
			if (haptic_query & SDL_HAPTIC_STATUS)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_STATUS"));
			if (haptic_query & SDL_HAPTIC_PAUSE)
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticQuery - SDL_HAPTIC_PAUSE "));

			EffectConstant.type = SDL_HAPTIC_CONSTANT;
			EffectConstant.constant.direction.type = SDL_HAPTIC_CARTESIAN;
			EffectConstant.constant.direction.dir[0] = 0;
			EffectConstant.constant.direction.dir[1] = 0;

			/* Replay */
			EffectConstant.constant.length = 0xffff;
			EffectConstant.constant.delay = 0;

			/* Trigger */
			EffectConstant.constant.button = 0;
			EffectConstant.constant.interval = 0;

			/* Constant */
			EffectConstant.constant.level = 0;
			/* Envelope */
			EffectConstant.constant.attack_length = 0;   /**< Duration of the attack. */
			EffectConstant.constant.attack_level = 0;    /**< Level at the start of the attack. */
			EffectConstant.constant.fade_length = 0;     /**< Duration of the fade. */
			EffectConstant.constant.fade_level = 0;      /**< Level at the end of the fade. */

			EffectConstantIds.Add(SDL_HapticNewEffect(Haptics[i], &EffectConstant));

			if (EffectConstantIds.Last() < 0)
			{
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticNewEffect - failed for Joy [%d]"), i);
			}

			EffectBump.type = SDL_HAPTIC_RAMP;
			EffectBump.ramp.attack_length = 150;
			EffectBump.ramp.attack_level = 0;
			EffectBump.ramp.fade_length = 200;
			EffectBump.ramp.fade_level = 0;
			EffectBump.ramp.length = 300;
			EffectBump.ramp.delay = 0;
			EffectBump.ramp.button = 0;
			EffectBump.ramp.interval = 0;
			EffectBump.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
			EffectBump.ramp.direction.dir[0] = 0;
			EffectBump.ramp.direction.dir[1] = 0;

			EffectBumpIds.Add(SDL_HapticNewEffect(Haptics[i], &EffectBump));
			if (EffectBumpIds.Last() < 0)
			{
				UE_LOG(SodaSDLJoystickDevice, Log, TEXT("SDL_HapticNewEffect - failed to add EffectBump for Joy [%d]"), i);
			}
		}
	}

	UpdateEKeys(AxesNum, ButtonsNum);

	return true;
}

void FSDLJoystickDevice::RotateToPosition(float TargetPosition)
{
	for (size_t i = 0; i < Haptics.Num() && i < EffectConstantIds.Num(); ++i)
	{
		if (Haptics[i] && EffectConstantIds[i] != -1)
		{
			float CurrentPosition = Settings->CurrentAxesValues[0];
			EffectConstant.constant.level = FMath::Clamp((int)((TargetPosition - CurrentPosition) * Settings->RotatingForce), -32768, 32767);
			//		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("RotateToPosition force level: %d"), EffectConstant.constant.level);
			SDL_HapticUpdateEffect(Haptics[i], EffectConstantIds[i], &EffectConstant);
			SDL_HapticRunEffect(Haptics[i], EffectConstantIds[i], 1);
		}
	}
}

void FSDLJoystickDevice::ApplyConstantForce(int ForceVal)
{
	for (size_t i = 0; i < Haptics.Num() && i < EffectConstantIds.Num(); ++i)
	{
		if (Haptics[i] && EffectConstantIds[i] != -1)
		{
			EffectConstant.constant.level = FMath::Clamp(ForceVal, -32768, 32767);
			//		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Appling force level: %d"), EffectConstant.constant.level);
			SDL_HapticUpdateEffect(Haptics[i], EffectConstantIds[i], &EffectConstant);
			SDL_HapticRunEffect(Haptics[i], EffectConstantIds[i], 1);
		}
	}
}

void FSDLJoystickDevice::StopConstantForceEffect()
{
	for (size_t i = 0; i < Haptics.Num() && i < EffectConstantIds.Num(); ++i)
	{
		if (Haptics[i] && EffectConstantIds[i] != -1)
		{
			SDL_HapticStopEffect(Haptics[i], EffectConstantIds[i]);
		}
	}
}

void FSDLJoystickDevice::ApplyBumpEffect(int ForceVal, int Len)
{
	for (size_t i = 0; i < Haptics.Num() && i < EffectBumpIds.Num(); ++i)
	{
		if (Haptics[i] && EffectBumpIds[i] != -1)
		{
			EffectBump.ramp.attack_length = Len / 2;
			EffectBump.ramp.attack_level = 0;
			EffectBump.ramp.fade_length = Len / 3;
			EffectBump.ramp.fade_level = 0;
			EffectBump.ramp.length = Len;
			EffectBump.ramp.start = FMath::Clamp(ForceVal, -32768, 32767) / 2;
			EffectBump.ramp.end = FMath::Clamp(ForceVal, -32768, 32767);
			//		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("Appling bump level: %d"), ForceVal);
			SDL_HapticUpdateEffect(Haptics[i], EffectBumpIds[i], &EffectBump);
			SDL_HapticRunEffect(Haptics[i], EffectBumpIds[i], 1);
		}
	}
}

void FSDLJoystickDevice::ActualizeAxesNum()
{
/*	Settings->SetAnalogAxesNum(AxesNum);
#define LOCTEXT_NAMESPACE "InputKeys"
	// Register the FKeys (Gamepad key for controllers, Mouse for mice, FloatAxis for non binary values e.t.c.)

	for (int i = 0; i < AxesNum; ++i)
	{
		AnalogAxes.push_back(UAxis(*FString::Printf(TEXT("SodaAxis%d"), DeviceIndex, i), i));
		EKeys::AddKey(FKeyDetails(AnalogAxes.back().Key, FText::Format(LOCTEXT("Soda axis", "Soda Analog Axis {0}"), i), FKeyDetails::GamepadKey | FKeyDetails::FloatAxis));
		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("AddKey %s"), *(AnalogAxes.back().Name));
		AnalogAxes.back().SettingsStruct = Settings->GetAxisSettings(DeviceIndex, i);
		if (AnalogAxes.back().SettingsStruct)
			AnalogAxes.back().SettingsStruct->Name = AnalogAxes.back().Name;
	}
#undef LOCTEXT_NAMESPACE*/
}

void FSDLJoystickDevice::ActualizeButtonsNum()
{
/*	if (Settings->VirtualButtonsNum < ButtonsNum)
	{
		Settings->VirtualButtonsNum = ButtonsNum;
	}
#define LOCTEXT_NAMESPACE "InputKeys"
	for (int i = Buttons.size(); i < Settings->VirtualButtonsNum; ++i)
	{
		Buttons.push_back(UAxis(*FString::Printf(TEXT("SodaButton%d_%d"), DeviceIndex, i), i));
		EKeys::AddKey(FKeyDetails(Buttons.back().Key, FText::Format(LOCTEXT("Soda button", "Soda Button {0}"), i), FKeyDetails::GamepadKey));
		UE_LOG(SodaSDLJoystickDevice, Log, TEXT("AddKey %s"), *Buttons.back().Name);
	}
#undef LOCTEXT_NAMESPACE*/
}

void FSDLJoystickDevice::CloseDevice()
{
	for (size_t i = 0; i < Haptics.Num() && i < EffectConstantIds.Num(); ++i)
	{
		if (Haptics[i] && EffectConstantIds[i] >= 0)
		{
			SDL_HapticStopEffect(Haptics[i], EffectConstantIds[i]);
		}
	}

	for (SDL_Joystick* Joy : Joys)
	{
		if (Joy && SDL_JoystickGetAttached(Joy))
		{
			SDL_JoystickClose(Joy);
		}
	}
}