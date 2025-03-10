// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/UnrealSoda.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"

UVehicleGearBoxBaseComponent::UVehicleGearBoxBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.GearBox");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

FString UVehicleGearBoxBaseComponent::GetGearChar() const
{
	switch (GetGearState())
	{
	case EGearState::Neutral: return TEXT("N");
	//case EGearState::Drive: return TEXT("D");
	case EGearState::Reverse: return TEXT("R");
	case EGearState::Park: return TEXT("P");
	}
	
	if (GetGearNum() == 0)
	{
		return TEXT("N");
	}
	else if(GetGearNum() == -1)
	{
		return TEXT("R");
	}
	else
	{
		return FString::FromInt(GetGearNum());
	}
}

bool UVehicleGearBoxBaseComponent::AcceptGearFromVehicleInput(UVehicleInputComponent* VehicleInput)
{
	if (!VehicleInput)
	{
		return false;
	}

	if (VehicleInput->GetInputState().GearInputMode == EGearInputMode::ByState)
	{
		EGearState NewGear = VehicleInput->GetInputState().GearState;
		if (GetGearState() != NewGear)
		{
			SetGearByState(NewGear);
		}
	}
	if (VehicleInput->GetInputState().GearInputMode == EGearInputMode::ByNum)
	{
		int NewGear = VehicleInput->GetInputState().GearNum;
		if (GetGearNum() != NewGear)
		{
			SetGearByNum(NewGear);
		}
	}
	if (VehicleInput->GetInputState().bWasGearUpPressed)
	{
		if (GetGearNum() >= 0 && GetGearNum() < GetForwardGearsCount())
		{
			SetGearByNum(GetGearNum() + 1);
			VehicleInput->GetInputState().GearNum = GetGearNum();
			VehicleInput->GetInputState().GearState = GetGearState();
		}
		VehicleInput->GetInputState().bWasGearUpPressed = false;
	}
	if (VehicleInput->GetInputState().bWasGearDownPressed)
	{
		if (GetGearNum() > 0)
		{
			SetGearByNum(GetGearNum() - 1);
			VehicleInput->GetInputState().GearNum = GetGearNum();
			VehicleInput->GetInputState().GearState = GetGearState();
		}
		VehicleInput->GetInputState().bWasGearDownPressed = false;
	}
	return true;
}

//------------------------------------------------------------------------------------------------------------

UVehicleGearBoxSimpleComponent::UVehicleGearBoxSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Gear Box Simple");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	ReverseGearRatios.Add({ 10});
	ForwardGearRatios.Add({ 10});
}

bool UVehicleGearBoxSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (GetVehicle())
	{
		UObject* TorqueTransmissionObject = LinkToTorqueTransmission.GetObject<UObject>(GetOwner());
		ITorqueTransmission* TorqueTransmissionInterface = Cast<ITorqueTransmission>(TorqueTransmissionObject);
		if (TorqueTransmissionObject && TorqueTransmissionInterface)
		{
			OutputTorqueTransmission.SetInterface(TorqueTransmissionInterface);
			OutputTorqueTransmission.SetObject(TorqueTransmissionObject);
		}
		else
		{
			AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Transmission isn't connected"));
		}
	}

	if (!OutputTorqueTransmission)
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	return true;
}

void UVehicleGearBoxSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}


void UVehicleGearBoxSimpleComponent::PassTorque(float InTorque)
{
	InTorq = InTorque;
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		OutTorq = InTorque * Ratio;
		OutputTorqueTransmission->PassTorque(OutTorq);
	}
	else
	{
		InTorq = 0;
		OutTorq = 0;
	}
}

float UVehicleGearBoxSimpleComponent::ResolveAngularVelocity() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		InAngularVelocity = OutputTorqueTransmission->ResolveAngularVelocity();
		OutAngularVelocity = InAngularVelocity * Ratio;

		SyncDataset();

		return OutAngularVelocity;
	}
	else
	{
		InAngularVelocity = 0;
		OutAngularVelocity = 0;

		SyncDataset();

		return 0;
	}
}


bool UVehicleGearBoxSimpleComponent::FindWheelRadius(float& OutRadius) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		return OutputTorqueTransmission->FindWheelRadius(OutRadius);
	}
	return false;
}

bool UVehicleGearBoxSimpleComponent::FindToWheelRatio(float& OutRatio) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		float PrevRatio;
		if (OutputTorqueTransmission->FindToWheelRatio(PrevRatio))
		{
			OutRatio = PrevRatio * Ratio;
			return true;
		}
	}
	return false;
}

bool UVehicleGearBoxSimpleComponent::SetGearByState(EGearState InGearState)
{
	if (InGearState == TargetGearState)
	{
		return true;
	}

	TargetGearState = InGearState;
	switch(InGearState)
	{
	case EGearState::Drive:
		if (GetGearNum() <= 0) return SetGearByNum(1);
		else return true;
	case EGearState::Reverse:
		if (GetGearNum() >= 0) return SetGearByNum(-1);
		else return true;
	case EGearState::Park:
	case EGearState::Neutral:
		return SetGearByNum(0);
	default:
		return false;
	};
}

bool UVehicleGearBoxSimpleComponent::SetGearByNum(int InGearNum)
{
	if (TargetGearNum == InGearNum)
	{
		return true;
	}

	if (InGearNum > 0) 
	{
		if (InGearNum > ForwardGearRatios.Num())
		{
			return false;
		}
		TargetGearState = EGearState::Drive;
		TargetGearNum = InGearNum;
		CurrentGearChangeTime = GearChangeTime;
		return true;
	}
	else if (InGearNum < 0)
	{
		if (FMath::Abs(InGearNum) > ReverseGearRatios.Num())
		{
			return false;
		}
		TargetGearState = EGearState::Reverse;
		TargetGearNum = InGearNum;
		CurrentGearChangeTime = GearChangeTime;
		return true;
	}
	else // InGearNum == 0
	{

		if (TargetGearState != EGearState::Park)
		{
			TargetGearState = EGearState::Neutral;
		}
		TargetGearNum = 0;
		CurrentGearChangeTime = GearChangeTime;
		return true;
	}
}

void UVehicleGearBoxSimpleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAcceptGearFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			AcceptGearFromVehicleInput(VehicleInput);
		}
	}

	if (bUseAutomaticGears && GetGearState() == EGearState::Drive)
	{
		if (FMath::Abs(OutAngularVelocity) * ANG2RPM > ChangeUpRPM)
		{
			if (GetGearNum() < ForwardGearRatios.Num())
			{
				//UE_LOG(LogSoda, Warning, TEXT("UVehicleGearBoxSimpleComponent::SetGearByNum(%i); Up "), GetGearNum() + 1);
				SetGearByNum(GetGearNum() + 1);
				OutAngularVelocity = InAngularVelocity * Ratio;
				
			}
		}
		else if (FMath::Abs(OutAngularVelocity) * ANG2RPM < ChangeDownRPM)
		{
			if (GetGearNum() > 1)
			{
				//UE_LOG(LogSoda, Warning, TEXT("UVehicleGearBoxSimpleComponent::SetGearByNum(%i); Down"), GetGearNum() - 1);
				SetGearByNum(GetGearNum() - 1);
				OutAngularVelocity = InAngularVelocity * Ratio;
			}
		}
	}

	if (CurrentGearNum != TargetGearNum || TargetGearState != CurrentGearState)
	{
		CurrentGearChangeTime -= DeltaTime;
		if (CurrentGearChangeTime <= 0.f)
		{
			CurrentGearChangeTime = 0.f;
			CurrentGearNum = TargetGearNum;
			CurrentGearState = TargetGearState;

			if (CurrentGearNum > 0)
			{
				Ratio = ForwardGearRatios[CurrentGearNum - 1];
			}
			else if (CurrentGearNum < 0)
			{
				Ratio = -ReverseGearRatios[FMath::Abs(CurrentGearNum) - 1];
			}
			else // CurrentGearNum == 0
			{
				Ratio = 0.f;
			}
		}
	}
}

void UVehicleGearBoxSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Gear: %s "), *GetGearChar()), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ratio: %.2f "), GetGearRatio()), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InTorq: %.2f "), InTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutTorq: %.2f "), OutTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InAngVel: %.2f "), InAngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutAngVel: %.2f "), OutAngularVelocity), 16, YPos);
	}
}
