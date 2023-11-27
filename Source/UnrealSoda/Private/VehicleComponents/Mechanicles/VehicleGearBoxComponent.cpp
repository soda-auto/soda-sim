// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

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

UVehicleGearBoxSimpleComponent::UVehicleGearBoxSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Gear Box Simple");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
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
	DebugInTorq = InTorque;
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		DebugOutTorq = InTorque * Ratio;
		OutputTorqueTransmission->PassTorque(DebugOutTorq);
	}
	else
	{
		DebugInTorq = 0;
		DebugOutTorq = 0;
	}
}

float UVehicleGearBoxSimpleComponent::ResolveAngularVelocity() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		DebugInAngularVelocity = OutputTorqueTransmission->ResolveAngularVelocity();
		DebugOutAngularVelocity = DebugInAngularVelocity * Ratio;
		return DebugOutAngularVelocity;
	}
	else
	{
		DebugInAngularVelocity = 0;
		DebugOutAngularVelocity = 0;
		return 0;
	}
}

float UVehicleGearBoxSimpleComponent::FindWheelRadius() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		return OutputTorqueTransmission->FindWheelRadius();
	}
	return 0;
}

float UVehicleGearBoxSimpleComponent::FindToWheelRatio() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		return OutputTorqueTransmission->FindToWheelRatio() * Ratio;
	}
	return 1.0;
}

FString UVehicleGearBoxSimpleComponent::GetGearChar() const
{
	switch (Gear)
	{
	case ENGear::Neutral: return TEXT("N");
	case ENGear::Drive: return TEXT("D");
	case ENGear::Reverse: return TEXT("R");
	case ENGear::Park: return TEXT("P");
	}
	return TEXT("?");
}

void UVehicleGearBoxSimpleComponent::SetGear(ENGear InGear)
{
	Gear = InGear;

	switch(Gear)
	{
	case ENGear::Drive:
		Ratio = DGearRatio;
		break;
	case ENGear::Reverse:
		Ratio = -RGearRatio;
		break;
	case ENGear::Park:
	case ENGear::Neutral:
		Ratio = 0;
		break;
	};
}

void UVehicleGearBoxSimpleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAcceptGearFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			ENGear NewGear = VehicleInput->GetGearInput();
			if (Gear != NewGear)
			{
				SetGear(NewGear);
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
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ratio: %.2f "), Ratio), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InTorq: %.2f "), DebugInTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutTorq: %.2f "), DebugOutTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InAngVel: %.2f "), DebugInAngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutAngVel: %.2f "), DebugOutAngularVelocity), 16, YPos);
	}
}

