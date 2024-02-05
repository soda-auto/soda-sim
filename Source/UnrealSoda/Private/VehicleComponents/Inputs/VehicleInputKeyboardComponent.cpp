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
		float Throttle = PlayerController->GetInputAnalogKeyState(Settings->ThrottleKeyInput);
		float Brake = PlayerController->GetInputAnalogKeyState(Settings->BrakeKeyInput);

		if (std::abs(ForwardSpeed) < 2.0)
			AutoGearBrakeTimeCounter += DeltaTime;
		else
			AutoGearBrakeTimeCounter = 0;

		RawThrottleInput = 0.f;
		RawBrakeInput = 0.f;

		if (Throttle > 0.1)
		{
			if (InputState.IsForwardGear())
			{
				RawThrottleInput = Throttle;
				RawBrakeInput = 0;
			}
			else if (InputState.IsReversGear())
			{
				RawThrottleInput = 0;
				RawBrakeInput = Throttle;
				if (AutoGearBrakeTimeCounter > AutoGearChangeTime)
					InputState.SetGearState(EGearState::Drive);
			}
			else
			{
				InputState.SetGearState(EGearState::Drive);
				RawThrottleInput = Throttle;
				RawBrakeInput = 0;
			}
		}

		if (Brake > 0.1)
		{
			if (InputState.IsForwardGear())
			{
				RawThrottleInput = 0;
				RawBrakeInput = Brake;
				if (AutoGearBrakeTimeCounter > AutoGearChangeTime)
				{
					InputState.SetGearState(EGearState::Reverse);
				}
			}
			else if (InputState.IsReversGear())
			{
				RawThrottleInput = Brake;
				RawBrakeInput = 0;
			}
			else
			{
				InputState.SetGearState(EGearState::Reverse);
				RawThrottleInput = 0;
				RawBrakeInput = Brake;
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

	/* Compute Throttle Input */
	InputState.Throttle = ThrottleInputRate.InterpInputValue(DeltaTime, InputState.Throttle, RawThrottleInput);

	/* Compute Brake Input*/
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


