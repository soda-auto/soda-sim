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



	return true;
}

void ULightController::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}


void ULightController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();



	if (LightSet && VehicleInput)
	{

		const TMap<FName, UVehicleLightItem*>& AllLights = LightSet->GetLightItems();

		// Perform testing configuration
		if (bDoTesting)
		{	
				EnableLightByName(AllLights, FName("Turn_FL"), LigthTestingPrms.TestTurnL);
				EnableLightByName(AllLights, FName("Turn_RL"), LigthTestingPrms.TestTurnL);
				EnableLightByName(AllLights, FName("Turn_FR"), LigthTestingPrms.TestTurnR);
				EnableLightByName(AllLights, FName("Turn_RR"), LigthTestingPrms.TestTurnR);
				EnableLightByName(AllLights, FName("Brake_L"), LigthTestingPrms.TestBrakeL);
				EnableLightByName(AllLights, FName("Brake_R"), LigthTestingPrms.TestBrakeR);
				EnableLightByName(AllLights, FName("HighBeam_L"), LigthTestingPrms.TestHighBeamL);
				EnableLightByName(AllLights, FName("HighBeam_R"), LigthTestingPrms.TestHighBeamR);
				EnableLightByName(AllLights, FName("LowBeam_L"), LigthTestingPrms.TestLowBeamL);
				EnableLightByName(AllLights, FName("LowBeam_R"), LigthTestingPrms.TestLowBeamR);
				EnableLightByName(AllLights, FName("Tail_FL"), LigthTestingPrms.TestTailFL);
				EnableLightByName(AllLights, FName("Tail_FR"), LigthTestingPrms.TestTailFR);
				EnableLightByName(AllLights, FName("Tail_RL"), LigthTestingPrms.TestTailRL);
				EnableLightByName(AllLights, FName("Tail_RR"), LigthTestingPrms.TestTailRR);
				EnableLightByName(AllLights, FName("Reverse_L"), LigthTestingPrms.TestReverseL);
				EnableLightByName(AllLights, FName("Reverse_R"), LigthTestingPrms.TestReverseR);
		}
		else
		{

		// Perform light control based on InputState

			if (VehicleInput->GetInputState().Brake > BrakeLightPdlPosnThd)
			{
				EnableLightByName(AllLights, FName("Brake_L"), true);
				EnableLightByName(AllLights, FName("Brake_R"), true);
			}
			else
			{
				EnableLightByName(AllLights, FName("Brake_L"), false);
				EnableLightByName(AllLights, FName("Brake_R"), false);
			}


			if (GetWorld()->GetTimeSeconds() - LastTimeBlinked > TimeBetweenTurnLightActivation)
			{
				LastTimeBlinked = GetWorld()->GetTimeSeconds();
				bBlinkAllwd = !bBlinkAllwd;
			}

			bBlinkAllwd = bBlinkAllwd && bAllowTurnLights;
		
			EnableLightByName(AllLights, FName("Turn_FL"), VehicleInput->GetInputState().bLeftTurnLightsEnabled && bBlinkAllwd);
			EnableLightByName(AllLights, FName("Turn_RL"), VehicleInput->GetInputState().bLeftTurnLightsEnabled && bBlinkAllwd);
		
			EnableLightByName(AllLights, FName("Turn_FR"), VehicleInput->GetInputState().bRightTurnLightsEnabled && bBlinkAllwd);
			EnableLightByName(AllLights, FName("Turn_RR"), VehicleInput->GetInputState().bRightTurnLightsEnabled && bBlinkAllwd);
	

			EnableLightByName(AllLights, FName("Tail_FL"), bAllowSideLights);
			EnableLightByName(AllLights, FName("Tail_FR"), bAllowSideLights);
			EnableLightByName(AllLights, FName("Tail_RL"), bAllowSideLights);
			EnableLightByName(AllLights, FName("Tail_RR"), bAllowSideLights);

			EnableLightByName(AllLights, FName("Reverse_L"), VehicleInput->GetInputState().IsReversGear());
			EnableLightByName(AllLights, FName("Reverse_R"), VehicleInput->GetInputState().IsReversGear());
			
			EnableLightByName(AllLights, FName("HighBeam_L"), bEnableHighBeam);
			EnableLightByName(AllLights, FName("HighBeam_R"), bEnableHighBeam);
			EnableLightByName(AllLights, FName("LowBeam_L"), bEnableLowBeam);
			EnableLightByName(AllLights, FName("LowBeam_R"), bEnableLowBeam);


		}

	}	

}


void ULightController::EnableLightByName(const TMap<FName, UVehicleLightItem*>& AllLights, FName LightName, bool bNewActivation)
{
	if (AllLights.Contains(LightName))
	{
		AllLights[LightName]->SetEnabled(bNewActivation, bNewActivation ? 1.0 : 0.0);
	}
}