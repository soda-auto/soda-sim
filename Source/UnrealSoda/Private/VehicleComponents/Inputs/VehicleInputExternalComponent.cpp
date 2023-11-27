// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputExternalComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UVehicleInputExternalComponent::UVehicleInputExternalComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Exteran");
	GUI.IcanName = TEXT("SodaIcons.External");
	GUI.bIsPresentInAddMenu = true;

	InputType = EVehicleInputType::External;
}

bool UVehicleInputExternalComponent::OnActivateVehicleComponent()
{
	return Super::OnActivateVehicleComponent();
}

void UVehicleInputExternalComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleInputExternalComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	if ((GetWheeledVehicle()->GetActiveVehicleInput() == this))
	{
		Super::DrawDebug(Canvas, YL, YPos);

		if (Common.bDrawDebugCanvas && GetWheeledVehicle())
		{
			UFont* RenderFont = GEngine->GetSmallFont();
			Canvas->SetDrawColor(FColor::White);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Steering: %.2f"), GetSteeringInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Throttle: %.2f"), GetThrottleInput()), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Brake: %.2f"), GetBrakeInput()), 16, YPos);
		}
	}
}

void UVehicleInputExternalComponent::CopyInputStates(UVehicleInputComponent* Previous)
{
	if (Previous)
	{
		SteeringInput = Previous->GetSteeringInput();
		ThrottleInput = Previous->GetThrottleInput();
		BrakeInput = Previous->GetBrakeInput();
		GearInput = Previous->GetGearInput();
	}
}

void UVehicleInputExternalComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	ThrottleInput = ThrottleInputRate.InterpInputValue(DeltaTime, ThrottleInput, ThrottleInputTarget);
	BrakeInput = BrakeInputRate.InterpInputValue(DeltaTime, BrakeInput, BrakeInputTarget);
	SteeringInput = SteerInputRate.InterpInputValue(DeltaTime, SteeringInput, SteeringInputTarget);
}
