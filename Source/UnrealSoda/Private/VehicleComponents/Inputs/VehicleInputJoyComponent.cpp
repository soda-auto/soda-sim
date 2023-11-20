// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputJoyComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "InputCoreTypes.h"
#include "Soda/SodaStatics.h"
#include "SodaJoystick.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "UObject/ConstructorHelpers.h"

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

	if (UVehicleSteeringRackBaseComponent* SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner()))
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
}

void UVehicleInputJoyComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	if (!GetWheeledVehicle()->GetWheeledComponentInterface()) return;

	if (!Joy || !PlayerController || !HealthIsWorkable())
	{
		//ThrottleInput = 0;
		//SteeringInput = 0;
		//BrakeInput = 1.0;
		return;
	}

	USodaUserSettings* Settings = SodaApp.GetSodaUserSettings();

	const float PrevSteeringInput = SteeringInput;

	ThrottleInput = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->ThrottleJoyInput), 0.f, 1.f);
	SteeringInput = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->SteeringJoyInput), -1.f, 1.f);
	BrakeInput = FMath::Clamp(PlayerController->GetInputAnalogKeyState(Settings->BrakeJoyInput), 0.f, 1.f);

	if (std::abs(BrakeInput) < InputBrakeDeadzone)
	{
		BrakeInput = 0.f;
	}

	bool GearChangePossible = bCreepMode == false || BrakeInput > 0.1f;

	/**********************************
	* Change Gear
	***********************************/
	if (PlayerController->IsInputKeyDown(Settings->NeutralGearJoyInput) || PlayerController->IsInputKeyDown(Settings->NeutralGearKeyInput))
		GearInput = ENGear::Neutral;
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->DriveGearJoyInput) || PlayerController->IsInputKeyDown(Settings->DriveGearKeyInput)))
		GearInput = ENGear::Drive;
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->ReverseGearJoyInput) || PlayerController->IsInputKeyDown(Settings->ReverseGearKeyInput)))
		GearInput = ENGear::Reverse;
	else if (GearChangePossible && (PlayerController->IsInputKeyDown(Settings->ParkGearJoyInput) || PlayerController->IsInputKeyDown(Settings->ParkGearKeyInput)))
		GearInput = ENGear::Park;

	if (bCreepMode && (GearInput == ENGear::Drive || GearInput == ENGear::Reverse))
	{
		float SpeedError = CreepSpeed - std::abs(ForwardSpeed);
		float CreepThrottleInput = FMath::Clamp(SpeedError / CreepSpeed, 0.0f, MaxCreepThrottle);
		ThrottleInput = FMath::Max(CreepThrottleInput, ThrottleInput);
	}
		
	const float InputSteeringSpeed = (SteeringInput - PrevSteeringInput) / DeltaTime;
	const float CurrSteer = GetWheeledVehicle()->GetSteer() / M_PI * 180.0;

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
		FeedbackDiffFactor = FeedbackDiffCurve.ExternalCurve->GetFloatValue(MaxSteer * SteeringInput - CurrSteer) * FeedbackDiffCoeff;
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

	if (bEnableBumpEffect && GetWheeledVehicle()->Is4WDVehicle())
	{
		float NewOffsetLeft = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->SuspensionOffset;
		float NewOffsetRight = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->SuspensionOffset;
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

	ThrottleInput = GetCruiseControlModulatedThrottleInput(ThrottleInput);
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

			if (Joy)
			{
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Joy Num: %i"), Joy->GetJoyNum()), 16, YPos);
				
			}

			if (bShowFeedback)
			{
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
}

void UVehicleInputJoyComponent::CopyInputStates(UVehicleInputComponent* Previous)
{
	if (Previous)
	{
		SteeringInput = Previous->GetSteeringInput();
		ThrottleInput = Previous->GetThrottleInput();
		BrakeInput = Previous->GetBrakeInput();
		GearInput = Previous->GetGearInput();
	}
}
