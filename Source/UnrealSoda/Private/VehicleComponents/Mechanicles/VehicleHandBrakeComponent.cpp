// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleHandBrakeComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UVehicleHandBrakeBaseComponent::UVehicleHandBrakeBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

UVehicleHandBrakeSimpleComponent::UVehicleHandBrakeSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Hand Brake Simple");
	GUI.IcanName = TEXT("SodaIcons.Braking");
	GUI.bIsPresentInAddMenu = true;
}

bool UVehicleHandBrakeSimpleComponent::OnActivateVehicleComponent()
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

void UVehicleHandBrakeSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleHandBrakeSimpleComponent::RequestByRatio(float InRatio)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (HandBrakeMode)
		{
		case EHandBrakeMode::FrontWheels:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		case EHandBrakeMode::RearWheels:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		case EHandBrakeMode::FourWheel:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		}

		CurrentRatio = 0;
	}
}

void UVehicleHandBrakeSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ratio: %.2f"), CurrentRatio), 16, YPos);
	}
}
