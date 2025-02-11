// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/LightController.h"
#include "Soda/VehicleComponents/Others/LightSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "VehicleUtility.h"
#include "Components/LightComponent.h"

ULightController::ULightController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("LightController");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	

	Common.Activation = EVehicleComponentActivation::OnStartScenario;

	LightTestingPrms.Add(VehicleLightNames::TurnFL, {});
	LightTestingPrms.Add(VehicleLightNames::TurnFR, {});
	LightTestingPrms.Add(VehicleLightNames::TurnRL, {});
	LightTestingPrms.Add(VehicleLightNames::TurnRR, {});
	LightTestingPrms.Add(VehicleLightNames::BrakeL, {});
	LightTestingPrms.Add(VehicleLightNames::BrakeR, {});
	LightTestingPrms.Add(VehicleLightNames::HighBeamL, {});
	LightTestingPrms.Add(VehicleLightNames::HighBeamR, {});
	LightTestingPrms.Add(VehicleLightNames::LowBeamL, {});
	LightTestingPrms.Add(VehicleLightNames::LowBeamR, {});
	LightTestingPrms.Add(VehicleLightNames::TailFL, {});
	LightTestingPrms.Add(VehicleLightNames::TailFR, {});
	LightTestingPrms.Add(VehicleLightNames::TailRL, {});
	LightTestingPrms.Add(VehicleLightNames::TailRR, {});
	LightTestingPrms.Add(VehicleLightNames::ReverseL, {});
	LightTestingPrms.Add(VehicleLightNames::ReverseR, {});

}


void ULightController::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

}

bool ULightController::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}


	if (!GetWheeledVehicle()->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Supports only 4WD vehicle"));
		return false;
	}

	if (GetWheeledVehicle())
	{
		LightSet = LinkToLightSet.GetObject< ULightSetComponent>(GetOwner());
	}

	if (!LightSet)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Invalid LightSet"));
		return false;
	}

	const TMap<FName, UVehicleLightItem*>& AllLights = LightSet->GetLightItems();


	auto InitLight = [&](FName LightName)->UVehicleLightItem*
	{
		if (auto* Found = AllLights.Find(LightName))
		{
			return *Found;
		}
		else
		{
			SetHealth(EVehicleComponentHealth::Error, FString::Printf(TEXT("Invalid light %s"), *LightName.ToString()));
			return nullptr;
		}
	
	};

	LightItem_TurnFL = InitLight(VehicleLightNames::TurnFL);
	LightItem_TurnFR = InitLight(VehicleLightNames::TurnFR);
	LightItem_TurnRL = InitLight(VehicleLightNames::TurnRL);
	LightItem_TurnRR = InitLight(VehicleLightNames::TurnRR);
	LightItem_BrakeL = InitLight(VehicleLightNames::BrakeL);
	LightItem_BrakeR = InitLight(VehicleLightNames::BrakeR);
	LightItem_HighBeamL = InitLight(VehicleLightNames::HighBeamL);
	LightItem_HighBeamR = InitLight(VehicleLightNames::HighBeamR);
	LightItem_LowBeamL = InitLight(VehicleLightNames::LowBeamL);
	LightItem_LowBeamR = InitLight(VehicleLightNames::LowBeamR);
	LightItem_TailFL = InitLight(VehicleLightNames::TailFL);
	LightItem_TailFR = InitLight(VehicleLightNames::TailFR);
	LightItem_TailRL = InitLight(VehicleLightNames::TailRL);
	LightItem_TailRR = InitLight(VehicleLightNames::TailRR);
	LightItem_ReverseL = InitLight(VehicleLightNames::ReverseL);
	LightItem_ReverseR = InitLight(VehicleLightNames::ReverseR);


	for (auto& [Key, Value] : LightTestingPrms)
	{
		Value.LinkToLightComponent = InitLight(Key);
	}
	
	return (GetHealth() == EVehicleComponentHealth::Error) ? false : true;

}

void ULightController::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	// Delete old pointers
}


void ULightController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();



	if (LightSet && VehicleInput)
	{
		// Perform testing configuration
		if (bDoTesting)
		{	
			for (auto& [Key, Value] : LightTestingPrms)
			{
				ChangeLightSt(Value);
			}
		}
		else
		{

		// Perform light control based on InputState

			if (VehicleInput->GetInputState().Brake > BrakeLightPdlPosnThd)
			{
				ChangeLightSt(LightItem_BrakeL,true);
				ChangeLightSt(LightItem_BrakeR, true);
			}
			else
			{
				ChangeLightSt(LightItem_BrakeL, false);
				ChangeLightSt(LightItem_BrakeR, false);
			}


			if (GetWorld()->GetTimeSeconds() - LastTimeBlinked > TimeBetweenTurnLightActivation)
			{
				LastTimeBlinked = GetWorld()->GetTimeSeconds();
				bBlinkAllwd = !bBlinkAllwd;
			}

			bBlinkAllwd = bBlinkAllwd && bAllowTurnLights;
		

			ChangeLightSt(LightItem_TurnFL, VehicleInput->GetInputState().bLeftTurnLightsEnabled && bBlinkAllwd);
			ChangeLightSt(LightItem_TurnFR, VehicleInput->GetInputState().bLeftTurnLightsEnabled && bBlinkAllwd);
			ChangeLightSt(LightItem_TurnRL, VehicleInput->GetInputState().bRightTurnLightsEnabled && bBlinkAllwd);
			ChangeLightSt(LightItem_TurnRR, VehicleInput->GetInputState().bRightTurnLightsEnabled && bBlinkAllwd);

			ChangeLightSt(LightItem_TailFL, bAllowSideLights);
			ChangeLightSt(LightItem_TailFR, bAllowSideLights);
			ChangeLightSt(LightItem_TailRL, bAllowSideLights);
			ChangeLightSt(LightItem_TailRR, bAllowSideLights);

			ChangeLightSt(LightItem_ReverseL, VehicleInput->GetInputState().IsReversGear());
			ChangeLightSt(LightItem_ReverseR, VehicleInput->GetInputState().IsReversGear());

			ChangeLightSt(LightItem_HighBeamL, bEnableHighBeam);
			ChangeLightSt(LightItem_HighBeamR, bEnableHighBeam);
			ChangeLightSt(LightItem_LowBeamL, bEnableLowBeam);
			ChangeLightSt(LightItem_LowBeamR, bEnableLowBeam);


		}

	}	

}



void ULightController::ChangeLightSt(const FLinkToLight& LightItem)
{
	if (LightItem.LinkToLightComponent)
	{
		LightItem.LinkToLightComponent->SetEnabled(LightItem.bIsActive, LightItem.bIsActive ? 1.0 : 0.0);
	}

}

void ULightController::ChangeLightSt(TObjectPtr<UVehicleLightItem> LightItem, bool bNewActive)
{
	if (LightItem)
	{
		LightItem->SetEnabled(bNewActive, bNewActive ? 1.0 : 0.0);
	}

}