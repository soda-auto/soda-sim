// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputKeyboardComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
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

	if (PlayerController->IsInputKeyDown(Settings->NeutralGearKeyInput))
		GearInput = ENGear::Neutral;
	else if (PlayerController->IsInputKeyDown(Settings->ParkGearKeyInput))
		GearInput = ENGear::Park;

	if (bAutoGearBox)
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
			if (GearInput == ENGear::Drive)
			{
				RawThrottleInput = Throttle;
				RawBrakeInput = 0;
			}
			else if (GearInput == ENGear::Reverse)
			{
				RawThrottleInput = 0;
				RawBrakeInput = Throttle;
				if (AutoGearBrakeTimeCounter > AutoGearChangeTime)
					GearInput = ENGear::Drive;
			}
			else
			{
				GearInput = ENGear::Drive;
				RawThrottleInput = Throttle;
				RawBrakeInput = 0;
			}
		}

		if (Brake > 0.1)
		{
			if (GearInput == ENGear::Drive)
			{
				RawThrottleInput = 0;
				RawBrakeInput = Brake;
				if (AutoGearBrakeTimeCounter > AutoGearChangeTime)
					GearInput = ENGear::Reverse;
			}
			else if (GearInput == ENGear::Reverse)
			{
				RawThrottleInput = Brake;
				RawBrakeInput = 0;
			}
			else
			{
				GearInput = ENGear::Reverse;
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
			GearInput = ENGear::Drive;
		else if (PlayerController->IsInputKeyDown(Settings->ReverseGearKeyInput))
			GearInput = ENGear::Reverse;
	}

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
		SteeringInput = SteerRate.InterpInputValue(DeltaTime, SteeringInput, SteerInputTarget);
	}
	else
	{
		SteeringInput = SteerInputTarget;
	}

	/* Compute Throttle Input */
	ThrottleInput = ThrottleInputRate.InterpInputValue(DeltaTime, ThrottleInput, RawThrottleInput);

	/* Compute Brake Input*/
	BrakeInput = BrakeInputRate.InterpInputValue(DeltaTime, BrakeInput, RawBrakeInput);
}

float UVehicleInputKeyboardComponent::GetSteeringInput() const
{
	return SteeringInput * InputSteeringLimit;
}

float UVehicleInputKeyboardComponent::GetThrottleInput() const
{ 
	return GetCruiseControlModulatedThrottleInput(ThrottleInput * InputThrottleLimit);
}

float UVehicleInputKeyboardComponent::GetBrakeInput() const
{
	return BrakeInput *  InputBrakeLimit; 
}

void UVehicleInputKeyboardComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	if (GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		Super::DrawDebug(Canvas, YL, YPos);

		if (Common.bDrawDebugCanvas)
		{
			UFont* RenderFont = GEngine->GetSmallFont();
			Canvas->SetDrawColor(FColor::White);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Steering: %.2f"), GetSteeringInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Throttle: %.2f"), GetThrottleInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Brake: %.2f"), GetBrakeInput()), 16, YPos);
			Canvas->DrawText(RenderFont, TEXT("DriverTension:"), 16, YPos);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackDriverSteerTension, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackDriverSteerTension * 100)));
		}
	}
}

void UVehicleInputKeyboardComponent::CopyInputStates(UVehicleInputComponent* Previous)
{
	if (Previous)
	{
		RawSteeringInput = SteeringInput = Previous->GetSteeringInput();
		RawThrottleInput = ThrottleInput = Previous->GetThrottleInput();
		RawBrakeInput = BrakeInput = Previous->GetBrakeInput();
		GearInput = Previous->GetGearInput();
	}
}


