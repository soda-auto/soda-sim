// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Drivers/SimpleVehicleDriverComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
//#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
//#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
//#include "Soda/VehicleComponents/Mechanicles/VehicleBrakeSystemComponent.h"
//#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"

//#include <cmath>


USimpleVehicleDriverComponent::USimpleVehicleDriverComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Simple Vehicle Driver");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostDeferredPhysTick = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

bool USimpleVehicleDriverComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}


	for (auto& it : WheelsData)
	{
		it.Wheel = GetWheeledVehicle()->GetWheelByIndex(it.WheelIndex);
		if (!it.Wheel)
		{
			SetHealth(EVehicleComponentHealth::Error, FString::Printf(TEXT("Can't fint wheel with index %i"), it.WheelIndex));
			return false;
		}
	}

	return true;
}

void USimpleVehicleDriverComponent::OnPostActivateVehicleComponent()
{
}

void USimpleVehicleDriverComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void USimpleVehicleDriverComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);


	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();

	if (VehicleInput)
	{
		float SteerReq = VehicleInput->GetInputState().Steering;
		float ThrottleReq = VehicleInput->GetInputState().Throttle;
		float BrakeReq = VehicleInput->GetInputState().Brake;

		if (VehicleInput->GetInputState().IsForwardGear())
		{

		}
		else if (VehicleInput->GetInputState().IsReversGear())
		{
			ThrottleReq = -ThrottleReq;
		}

		for (auto& it : WheelsData)
		{
			if (it.bApplyTorq)
			{
				it.Wheel->ReqTorq = ThrottleReq * it.TorqDistribution * MaxTorqReq;
			}
			if (it.bApplyBrakeTorq)
			{
				it.Wheel->ReqBrakeTorque = BrakeReq * it.BrakeTorqDistribution * MaxBrakeTorqReq;
			}
			if (it.bApplySteer)
			{
				it.Wheel->ReqSteer = FMath::DegreesToRadians(SteerReq * MaxSteer * (it.bInversSteer ? -1 : 1));
			}
		}
	}

	SyncDataset();
}

void USimpleVehicleDriverComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	const FColor SubCaption(170, 170, 170);
	UFont* RenderFont = GEngine->GetSmallFont();

}

FString USimpleVehicleDriverComponent::GetRemark() const
{
	return TEXT("");  
}


void USimpleVehicleDriverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
