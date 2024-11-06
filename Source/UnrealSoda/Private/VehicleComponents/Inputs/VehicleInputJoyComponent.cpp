// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputJoyComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "Soda/SodaStatics.h"
#include "SodaJoystick.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleInputJoyComponent::UVehicleInputJoyComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Joy");
	GUI.IcanName = TEXT("SodaIcons.Joystick");
	GUI.bIsPresentInAddMenu = true;

	InputType = EVehicleInputType::Joy;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> FeedbackAutocenterCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/JoyInput/FeedbackAutocenterCurve.FeedbackAutocenterCurve"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> FeedbackDiffCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/JoyInput/FeedbackDiffCurve.FeedbackDiffCurve"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> FeedbackResistionCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/JoyInput/FeedbackResistionCurve.FeedbackResistionCurve"));

	FeedbackAutocenterCurve.ExternalCurve = FeedbackAutocenterCurvePtr.Object;
	FeedbackDiffCurve.ExternalCurve = FeedbackDiffCurvePtr.Object;
	FeedbackResistionCurve.ExternalCurve = FeedbackResistionCurvePtr.Object;
}

bool UVehicleInputJoyComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!ISodaJoystickPlugin::IsAvailable())
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Warning, TEXT("UVehicleInputJoyComponent::OnActivateVehicleComponent(); ISodaJoystickPlugin not avalible"));
		return false;
	}

	Joy = &ISodaJoystickPlugin::Get();
	check(Joy);
	if (Joy->GetJoyNum() <= 0)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Warning, TEXT("UVehicleInputJoyComponent::OnActivateVehicleComponent(); SodaJoystick is not find"));
		return false;
	}

	if (!FeedbackAutocenterCurve.ExternalCurve)
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("FeedbackAutocenterCurve isn't set"));
	}

	if (!FeedbackDiffCurve.ExternalCurve)
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("FeedbackDiffCurve isn't set"));
	}

	if (!FeedbackResistionCurve.ExternalCurve)
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("FeedbackResistionCurve isn't set"));
	}

	if (!GetWheeledComponentInterface())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Need WheeledComponentInterface"));
		return false;
	}

	SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner());
	if (SteeringRack)
	{
		MaxSteer = SteeringRack->GetMaxSteer() / M_PI * 180.0;
	}
	else
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("LinkToSteering isn't set"));
		MaxSteer = 30 / M_PI * 180.0;
	}

	return HealthIsWorkable();
}

void UVehicleInputJoyComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (Joy)
	{
		Joy->StopConstantForceEffect();
	}
	Joy = nullptr;
	SteeringRack = nullptr;
}

void UVehicleInputJoyComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	if (!GetWheeledVehicle()->GetWheeledComponentInterface()) return;

	if (!Joy || !PlayerController || !HealthIsWorkable())
	{
		//Throttle = 0;
		//Steering = 0;
		//Brake = 1.0;
		return;
	}

	USodaUserSettings* Settings = SodaApp.GetSodaUserSettings();

	const float PrevSteeringInput = InputState.Steering;

	InputState.Throttle = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->ThrottleJoyInput), 0.f, 1.f);
	InputState.Steering = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->SteeringJoyInput), -1.f, 1.f);
	InputState.Brake = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->BrakeJoyInput), 0.f, 1.f);

	if (std::abs(InputState.Brake) < InputBrakeDeadzone)
	{
		InputState.Brake = 0.f;
	}

	bool GearChangePossible = bCreepMode == false || InputState.Brake > 0.1f;

	/**********************************
	* Change Gear
	***********************************/
	if (PlayerController->IsInputKeyDown(Settings->NeutralGearJoyInput) || PlayerController->IsInputKeyDown(Settings->NeutralGearKeyInput))
	{
		InputState.SetGearState(EGearState::Neutral);
	}
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->DriveGearJoyInput) || PlayerController->IsInputKeyDown(Settings->DriveGearKeyInput)))
	{
		InputState.SetGearState(EGearState::Drive);
	}
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->ReverseGearJoyInput) || PlayerController->IsInputKeyDown(Settings->ReverseGearKeyInput)))
	{
		InputState.SetGearState(EGearState::Reverse);
	}
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->ParkGearJoyInput) || PlayerController->IsInputKeyDown(Settings->ParkGearKeyInput)))
	{
		InputState.SetGearState(EGearState::Park);
	}

	InputState.bWasGearUpPressed = PlayerController->WasInputKeyJustPressed(Settings->GearUpJoyInput);
	InputState.bWasGearDownPressed = PlayerController->WasInputKeyJustPressed(Settings->GearDownJoyInput);

	if (bCreepMode && (InputState.IsForwardGear() || InputState.IsReversGear()))
	{
		float SpeedError = CreepSpeed - std::abs(ForwardSpeed);
		float CreepThrottleInput = FMath::Clamp(SpeedError / CreepSpeed, 0.0f, MaxCreepThrottle);
		InputState.Throttle = FMath::Max(CreepThrottleInput, InputState.Throttle);
	}
		
	const float InputSteeringSpeed = (InputState.Steering - PrevSteeringInput) / DeltaTime;
	const float CurrSteer = SteeringRack ? SteeringRack->GetCurrentSteer() / M_PI * 180.0 : 0;

	FeedbackAutocenterFactor = 0;
	FeedbackDiffFactor = 0;
	FeedbackResistionFactor = 0;

	if (FeedbackAutocenterCurve.ExternalCurve && bFeedbackAutocenterEnabled)
	{
		FeedbackAutocenterFactor = FeedbackAutocenterCurve.ExternalCurve->GetFloatValue(ForwardSpeed) *
			CurrSteer / MaxSteer * FeedbackAutocenterCoeff;
	}

	if (FeedbackDiffCurve.ExternalCurve && bFeedbackDiffEnabled)
	{
		FeedbackDiffFactor = FeedbackDiffCurve.ExternalCurve->GetFloatValue(MaxSteer * InputState.Steering - CurrSteer) * FeedbackDiffCoeff;
	}

	if (FeedbackResistionCurve.ExternalCurve && bFeedbackResistionEnabled)
	{
		FeedbackResistionFactor = FeedbackResistionCurve.ExternalCurve->GetFloatValue(InputSteeringSpeed) * FeedbackResistionCoeff;
	}

	FeedbackFullFactor =
		(FeedbackFullFactor * FeedbackFilter) +
		(FeedbackAutocenterFactor + FeedbackDiffFactor + FeedbackResistionFactor) * (1.0 - FeedbackFilter);

	Joy->ApplyConstantForce(FMath::Clamp(int(FeedbackFullFactor * 32767 + 0.5), -32768, 32767));

	FeedbackDriverSteerTension = FeedbackFullFactor;

	if (FeedbackDriverSteerTension > 0)
	{
		FeedbackDriverSteerTension -= CalcDriverForceCompensationDeadZone;
		if (FeedbackDriverSteerTension < 0) FeedbackDriverSteerTension = 0;
	}
	else if (FeedbackDriverSteerTension < 0)
	{
		FeedbackDriverSteerTension += CalcDriverForceCompensationDeadZone;
		if (FeedbackDriverSteerTension > 0) FeedbackDriverSteerTension = 0;
	}

	FeedbackDriverSteerTension *= CalcDriverForceCompensationCoeff;

	//UE_LOG(LogSoda, Warning, TEXT("---- %f %f"), GetMaxSteerAngle() * SteeringInput, OutputRegs.GetSteer() / M_PI * 180.0);
	//UE_LOG(LogSoda, Warning, TEXT("**** %f %f %f %f %f"), DriverInputSteerTension, WheelTension, AutocenterCurveValue, FullTension, SteeringInputSpeed);

	if (bEnableBumpEffect && GetWheeledVehicle()->IsXWDVehicle(4))
	{
		float NewOffsetLeft = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->SuspensionOffset2.Length();
		float NewOffsetRight = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->SuspensionOffset2.Length();
		float SuspensionTranslationSpeedLeft = (FrontWheelPrevSuspensionOffset[0] - NewOffsetLeft) / DeltaTime;
		float SuspensionTranslationSpeedRight = (FrontWheelPrevSuspensionOffset[1] - NewOffsetRight) / DeltaTime;
		FrontWheelPrevSuspensionOffset[0] = NewOffsetLeft;
		FrontWheelPrevSuspensionOffset[1] = NewOffsetRight;
		float RightLeftDiff = SuspensionTranslationSpeedRight - SuspensionTranslationSpeedLeft;
		float RightLeftSumm = SuspensionTranslationSpeedRight + SuspensionTranslationSpeedLeft;
		if (fabs(RightLeftDiff) > BumpEffectThreshold)
		{
			const int BaseFarceVal = 7000;
			const int BaseLen = 100000;
			int Force = (int)((BaseFarceVal * ((RightLeftDiff < 0) ? -1 : 1) + RightLeftDiff * BumpEffectForceCoef) * ((RightLeftSumm < 0) ? 1 : -1));
			int Len = (int)FMath::Clamp(float(BaseLen) / (ForwardSpeed + 1.f), 50.f, 1000.f);
			Joy->ApplyBumpEffect(Force, Len);
			//UE_LOG(LogSoda, Log, TEXT("Appying bump effect. Force:= %d, Len = %d"), Force, Len);
		}
	}
}

void UVehicleInputJoyComponent::ReinitDevice()
{
	if (Joy)
	{
		Joy->Reset();
	}
}

void UVehicleInputJoyComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetWheeledVehicle() && (GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		if (Joy)
		{
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Joy Num: %i"), Joy->GetJoyNum()), 16, YPos);
		}

		if (bShowFeedback)
		{
			Canvas->SetDrawColor(FColor::White);

			YPos += Canvas->DrawText(RenderFont, TEXT("Feedback:"), 16, YPos + 4);

			Canvas->DrawText(RenderFont, TEXT("DiffFactor:"), 16, YPos + 4);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackDiffFactor, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackDiffFactor * 100)));

			Canvas->DrawText(RenderFont, TEXT("ResistionFactor:"), 16, YPos + 4);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackResistionFactor, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackResistionFactor * 100)));

			Canvas->DrawText(RenderFont, TEXT("AutocenterFactor:"), 16, YPos + 4);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackAutocenterFactor, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackAutocenterFactor * 100)));

			Canvas->DrawText(RenderFont, TEXT("FullFactor:"), 16, YPos + 4);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackFullFactor, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackFullFactor * 100)));

			Canvas->DrawText(RenderFont, TEXT("DriverTension:"), 16, YPos + 4);
			YPos += USodaStatics::DrawProgress(Canvas, 120, YPos + 4, 120, 16, FeedbackDriverSteerTension, -1.0, 1.0, true,
				FString::Printf(TEXT("%i%%"), int(FeedbackDriverSteerTension * 100)));
		}
	}
}

void UVehicleInputJoyComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
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
			<< "FeedbackDiffFactor" << FeedbackDiffFactor
			<< "FeedbackResistionFactor" << FeedbackResistionFactor
			<< "FeedbackAutocenterFactor" << FeedbackAutocenterFactor
			<< "FeedbackFullFactor" << FeedbackFullFactor
			<< "FeedbackDriverSteerTension" << FeedbackDriverSteerTension
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}