// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleDifferentialComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"

UVehicleDifferentialBaseComponent::UVehicleDifferentialBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Differential");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

UVehicleDifferentialSimpleComponent::UVehicleDifferentialSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Differential Simple");
	GUI.bIsPresentInAddMenu = true;
}

bool UVehicleDifferentialSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!GetWheeledVehicle()->Is4WDVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	return true;
}

void UVehicleDifferentialSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleDifferentialSimpleComponent::PassTorque(float InTorque)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		DebugInTorq = InTorque;
		DebugOutTorq = InTorque * Ratio * 0.5;

		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqTorq += DebugOutTorq;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqTorq += DebugOutTorq;
			break;

		case EVehicleDifferentialType::Open_RearDrive:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqTorq += DebugOutTorq;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqTorq += DebugOutTorq;
			break;
		}
	}
	else
	{
		DebugInTorq = 0;
		DebugOutTorq = 0;
	}
}

float UVehicleDifferentialSimpleComponent::ResolveAngularVelocity() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			DebugInAngularVelocity = std::max(GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->AngularVelocity, GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->AngularVelocity);
			break;
		case EVehicleDifferentialType::Open_RearDrive:
			DebugInAngularVelocity = std::max(GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->AngularVelocity, GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->AngularVelocity);
			break;
		}
		DebugOutAngularVelocity = DebugInAngularVelocity * Ratio;
		return DebugOutAngularVelocity;
	}
	return 0;
}

float UVehicleDifferentialSimpleComponent::FindWheelRadius() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			return (GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->Radius + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->Radius) / 2.0;
		case EVehicleDifferentialType::Open_RearDrive:
			return (GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->Radius + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->Radius) / 2.0;
		}
	}
	return 0;
}

void UVehicleDifferentialSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InTorq: %.2f "), DebugInTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutTorq: %.2f "), DebugOutTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InAngVel: %.2f "), DebugInAngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutAngVel: %.2f "), DebugOutAngularVelocity), 16, YPos);
	}
}
