// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/UnrealSoda.h"
#include "InputCoreTypes.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"

UVehicleInputComponent::UVehicleInputComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Inputs");
	Common.UniqueVehiceComponentClass = UVehicleInputComponent::StaticClass();
	Common.bDrawDebugCanvas = true;
}

bool UVehicleInputComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	UVehicleInputComponent* ActiveVehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();
	CopyInputStates(ActiveVehicleInput);
	GetWheeledVehicle()->OnSetActiveVehicleInput(this);

	return true;
}

void UVehicleInputComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleInputComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	USodaUserSettings* Settings = SodaApp.GetSodaUserSettings();

	if (PlayerController->WasInputKeyJustPressed(Settings->ChangeModeKeyInput))
	{
		bADModeInput = !bADModeInput;
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->SafeStopKeyInput))
	{
		bSafeStopInput = !bSafeStopInput;
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->HeadlightsKeyInput))
	{
		bEnabledHeadlightsInput = !bEnabledHeadlightsInput;
	}

	bEnabledHornInput = PlayerController->IsInputKeyDown(Settings->HornKeyInput);


	/**********************************
	* Change Cruise Control Mode
	***********************************/
	if (PlayerController->WasInputKeyJustPressed(Settings->KeyCruiseControl))
	{

		if (CruiseControlMode != ECruiseControlMode::CruiseControlActive)
		{
			CruiseControlMode = ECruiseControlMode::CruiseControlActive;
			CruiseControlTargetSpeed = ForwardSpeed;
		}
		else
		{
			CruiseControlMode = ECruiseControlMode::Off;
		}
	}

	/**********************************
	* Change Speed Limiter Mode
	***********************************/
	if (PlayerController->WasInputKeyJustPressed(Settings->KeySpeedLimiter))
	{
		if (CruiseControlMode != ECruiseControlMode::SpeedLimiterActive)
		{
			CruiseControlMode = ECruiseControlMode::SpeedLimiterActive;
			CruiseControlTargetSpeed = ForwardSpeed;
		}
		else
		{
			CruiseControlMode = ECruiseControlMode::Off;
		}
	}

	if ((CruiseControlMode == ECruiseControlMode::CruiseControlActive && GetBrakeInput() > 0.1f) || GetGearInput() != ENGear::Drive)
	{
		CruiseControlMode = ECruiseControlMode::Off;
	}

	CCThrottleInput = FMath::Clamp(CruiseControlPropCoef * (CruiseControlTargetSpeed - std::abs(ForwardSpeed)), 0.0f, CruiseControlMode == ECruiseControlMode::CruiseControlActive ? MaxCruiseControlThrottle : 1.f);
}

float UVehicleInputComponent::GetCruiseControlModulatedThrottleInput(float ThrottleInput) const
{
	switch (CruiseControlMode)
	{
	case ECruiseControlMode::CruiseControlActive:
		ThrottleInput = FMath::Max(CCThrottleInput, ThrottleInput);
		break;

	case ECruiseControlMode::SpeedLimiterActive:
		ThrottleInput = FMath::Min(CCThrottleInput, ThrottleInput);
		break;
	}
	return ThrottleInput;
}


void UVehicleInputComponent::SetSteeringInput(float /*Value*/)
{
	UE_LOG(LogSoda, Warning, TEXT("UVehicleInputComponent::SetSteeringInput() is not supported for this InputComponent"));
}

void UVehicleInputComponent::SetThrottleInput(float /*Value*/) 
{
	UE_LOG(LogSoda, Warning, TEXT("UVehicleInputComponent::SetThrottleInput() is not supported for this InputComponent"));
}

void UVehicleInputComponent::SetBrakeInput(float /*Value*/) 
{
	UE_LOG(LogSoda, Warning, TEXT("UVehicleInputComponent::SetBrakeInput() is not supported for this InputComponent"));
}

void UVehicleInputComponent::SetGearInput(ENGear /*Value*/) 
{
	UE_LOG(LogSoda, Warning, TEXT("UVehicleInputComponent::SetGearInput() is not supported for this InputComponent"));
}

