// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputKeyboardComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleInputKeyboardComponent::UVehicleInputKeyboardComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Keyboard");
	GUI.IcanName = TEXT("SodaIcons.Keyboard");
	GUI.bIsPresentInAddMenu = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;

	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> InputSteeringSpeedCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/KeyboardInput/KeyInputSteeringSpeed.KeyInputSteeringSpeed"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> InputMaxSteerPerSpeedCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/KeyboardInput/KeyInputMaxSteeringAngel.KeyInputMaxSteeringAngel"));

	InputSteeringSpeedCurve.ExternalCurve = InputSteeringSpeedCurvePtr.Object;
	InputMaxSteerPerSpeedCurve.ExternalCurve = InputMaxSteerPerSpeedCurvePtr.Object;

	InputType = EVehicleInputType::Keyboard;
}

bool UVehicleInputKeyboardComponent::OnActivateVehicleComponent()
{
	return Super::OnActivateVehicleComponent();
}

void UVehicleInputKeyboardComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleInputKeyboardComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	if (!PlayerController)
	{
		return;
	}

	USodaUserSettings* Settings = SodaApp.GetSodaUserSettings();

	RawSteeringInput = -PlayerController->GetInputAnalogKeyState(Settings->SteeringLeftKeyInput) + PlayerController->GetInputAnalogKeyState(Settings->SteeringRigtKeyInput);
	RawSteeringInput = FMath::Clamp(RawSteeringInput, -InputSteeringLimit, InputSteeringLimit);


	if (bAutoReavers)
	{
		const float Up = PlayerController->GetInputAnalogKeyState(Settings->ThrottleKeyInput);
		const float Down = PlayerController->GetInputAnalogKeyState(Settings->BrakeKeyInput);

		//if (std::abs(ForwardSpeed) < 2.0)
		//	CurrentGearChangeTime += DeltaTime;
		//else
		//	CurrentGearChangeTime = 0;

		RawThrottleInput = 0.f;
		RawBrakeInput = 0.f;

		if (Up > 0.1)
		{
			if (InputState.IsForwardGear())
			{
				if (ForwardSpeed >= -1.0)
				{
					RawThrottleInput = Up;
					RawBrakeInput = 0;
				}
				else
				{
					RawThrottleInput = 0;
					RawBrakeInput = Up;
				}
			}
			else
			{
				InputState.SetGearState(EGearState::Drive);
				CurrentGearChangeTime = GearChangeTime;

				if (ForwardSpeed >= -1.0)
				{
					RawThrottleInput = Up;
					RawBrakeInput = 0;
				}
				else
				{
					RawThrottleInput = 0;
					RawBrakeInput = Up;
				}
			}
		}

		if (Down > 0.1)
		{
			if (InputState.IsReversGear())
			{
				if (ForwardSpeed >= 1.0)
				{
					RawThrottleInput = 0;
					RawBrakeInput = Down;
				}
				else
				{
					RawThrottleInput = Down;
					RawBrakeInput = 0;
				}
			}
			else
			{
				InputState.SetGearState(EGearState::Reverse);
				CurrentGearChangeTime = GearChangeTime;
				if (ForwardSpeed >= 1.0)
				{
					RawThrottleInput = 0;
					RawBrakeInput = Down;
				}
				else
				{
					RawThrottleInput = Down;
					RawBrakeInput = 0;
				}
			}
		}
	}
	else
	{
		RawThrottleInput = PlayerController->GetInputAnalogKeyState(Settings->ThrottleKeyInput);
		RawBrakeInput = PlayerController->GetInputAnalogKeyState(Settings->BrakeKeyInput);
		if (PlayerController->IsInputKeyDown(Settings->DriveGearKeyInput))
		{
			InputState.SetGearState(EGearState::Drive);
		}
		else if (PlayerController->IsInputKeyDown(Settings->ReverseGearKeyInput))
		{
			InputState.SetGearState(EGearState::Reverse);
		}
	}

	RawThrottleInput = FMath::Clamp(RawThrottleInput, 0.0f, InputThrottleLimit);
	RawBrakeInput = FMath::Clamp(RawBrakeInput, 0.0f, InputBrakeLimit);
	

	if (std::abs(RawSteeringInput) > 0.1)
	{
		if (RawSteeringInput > 0) FeedbackDriverSteerTension = 1.0;
		else FeedbackDriverSteerTension = -1.0;
	}
	else
	{
		FeedbackDriverSteerTension = 0;
	}

	
	/* Compute Steering input */
	float SteerInputTarget;
	if (InputMaxSteerPerSpeedCurve.ExternalCurve)
	{
		SteerInputTarget = InputMaxSteerPerSpeedCurve.ExternalCurve->GetFloatValue(ForwardSpeed) * RawSteeringInput;
	}
	else
	{
		SteerInputTarget = RawSteeringInput;
	}

	if (InputSteeringSpeedCurve.ExternalCurve)
	{
		SteerRate.RiseRate = InputSteeringSpeedCurve.ExternalCurve->GetFloatValue(ForwardSpeed);
		SteerRate.FallRate = InputSteeringSpeedCurve.ExternalCurve->GetFloatValue(ForwardSpeed);
		InputState.Steering = SteerRate.InterpInputValue(DeltaTime, InputState.Steering, SteerInputTarget);
	}
	else
	{
		InputState.Steering = SteerInputTarget;
	}

	if (CurrentGearChangeTime > 0)
	{
		CurrentGearChangeTime -= DeltaTime;
		InputState.Throttle = 0;
	}
	else
	{
		InputState.Throttle = ThrottleInputRate.InterpInputValue(DeltaTime, InputState.Throttle, RawThrottleInput);
	}

	InputState.Brake = BrakeInputRate.InterpInputValue(DeltaTime, InputState.Brake, RawBrakeInput);
}

void UVehicleInputKeyboardComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		Canvas->DrawText(RenderFont, TEXT("DriverTension:"), 16, YPos);
		YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackDriverSteerTension, -1.0, 1.0, true, FString::Printf(TEXT("%i%%"), int(FeedbackDriverSteerTension * 100)));
	}
}

void UVehicleInputKeyboardComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
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
			<< "Throttle" << GetInputState().Throttle
			<< "Brake" << GetInputState().Brake
			<< "Steering" << GetInputState().Steering
			<< "GearState" << int(GetInputState().GearState)
			<< "GearNum" << GetInputState().GearNum
			<< "bADModeEnbaled" << GetInputState().bADModeEnbaled
			<< "bSafeStopEnbaled" << GetInputState().bSafeStopEnbaled
			<< "SteerTension" << FeedbackDriverSteerTension
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}

