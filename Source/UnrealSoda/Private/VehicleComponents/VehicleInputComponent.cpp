// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/UnrealSoda.h"
#include "InputCoreTypes.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaCommonSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void FWheeledVehicleInputState::SetGearState(EGearState InGearState)
{
	GearState = InGearState;
	GearInputMode = EGearInputMode::ByState;
}

void FWheeledVehicleInputState::SetGearNum(int InGearNum)
{
	GearNum = InGearNum;
	GearInputMode = EGearInputMode::ByNum;
}

void FWheeledVehicleInputState::GearUp()
{
	bWasGearUpPressed = true;
	//GearInputMode = EGearInputMode::ByOffset;
}

void FWheeledVehicleInputState::GearDown()
{
	bWasGearDownPressed = true;
	//GearInputMode = EGearInputMode::ByOffset;
}

bool FWheeledVehicleInputState::IsForwardGear() const
{
	return (GearInputMode == EGearInputMode::ByState && GearState == EGearState::Drive) || (GearInputMode == EGearInputMode::ByNum && GearNum > 0);
}

bool FWheeledVehicleInputState::IsReversGear() const
{
	return (GearInputMode == EGearInputMode::ByState && GearState == EGearState::Reverse) || (GearInputMode == EGearInputMode::ByNum && GearNum < 0);
}

bool FWheeledVehicleInputState::IsNeutralGear() const
{
	return (GearInputMode == EGearInputMode::ByState && GearState == EGearState::Neutral) || (GearInputMode == EGearInputMode::ByNum && GearNum == 0);
}

bool FWheeledVehicleInputState::IsParkGear() const
{
	return (GearInputMode == EGearInputMode::ByState && GearState == EGearState::Park);
}

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

	if (bUpdateDefaultsButtonsFromKeyboard)
	{
		UpdateInputStatesDefaultsButtons(DeltaTime, ForwardSpeed, PlayerController);
	}
}

void UVehicleInputComponent::UpdateInputStatesDefaultsButtons(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	const USodaCommonSettings* Settings = GetDefault<USodaCommonSettings>();

	if (PlayerController->WasInputKeyJustPressed(Settings->ChangeModeKeyInput))
	{
		GetInputState().bADModeEnbaled = !GetInputState().bADModeEnbaled;
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->SafeStopKeyInput))
	{
		GetInputState().bSafeStopEnbaled = !GetInputState().bSafeStopEnbaled;
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->HeadlightsKeyInput))
	{
		GetInputState().bHeadlightsEnabled = !GetInputState().bHeadlightsEnabled;
	}

	GetInputState().bHornEnabled = PlayerController->IsInputKeyDown(Settings->HornKeyInput);

	if (PlayerController->WasInputKeyJustPressed(Settings->KeyCruiseControl))
	{
		if (GetInputState().CruiseControlMode != ECruiseControlMode::CruiseControlActive)
		{
			GetInputState().CruiseControlMode = ECruiseControlMode::CruiseControlActive;
			//CruiseControlTargetSpeed = ForwardSpeed;
		}
		else
		{
			GetInputState().CruiseControlMode = ECruiseControlMode::Off;
		}
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->KeySpeedLimiter))
	{
		if (GetInputState().CruiseControlMode != ECruiseControlMode::SpeedLimiterActive)
		{
			GetInputState().CruiseControlMode = ECruiseControlMode::SpeedLimiterActive;
			//CruiseControlTargetSpeed = ForwardSpeed;
		}
		else
		{
			GetInputState().CruiseControlMode = ECruiseControlMode::Off;
		}
	}

	if (PlayerController->IsInputKeyDown(Settings->NeutralGearKeyInput))
	{
		GetInputState().SetGearState(EGearState::Neutral);
	}
	else if (PlayerController->IsInputKeyDown(Settings->ParkGearKeyInput))
	{
		GetInputState().SetGearState(EGearState::Park);
	}

	if (PlayerController->WasInputKeyJustPressed(Settings->GearUpKeyInput))
	{
		GetInputState().GearUp();
	}
	if (PlayerController->WasInputKeyJustPressed(Settings->GearDownKeyInput))
	{
		GetInputState().GearDown();
	}
}

/*
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
*/

void UVehicleInputComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Steering: %.2f"), GetInputState().Steering), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Throttle: %.2f"), GetInputState().Throttle), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Brake: %.2f"), GetInputState().Brake), 16, YPos);
	}
}

void UVehicleInputComponent::CopyInputStates(UVehicleInputComponent* Previous)
{
	if (Previous)
	{
		GetInputState() = Previous->GetInputState();
	}
}