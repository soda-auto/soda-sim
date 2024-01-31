// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Drivers/GenericVehicleDriverComponent.h"
#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleBrakeSystemComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleHandBrakeComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/LevelState.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include <cmath>


static inline float NormAng(float a)
{
	return (a > M_PI) ? (a - 2.0 * M_PI) : ((a < -M_PI) ? (a + 2 * M_PI) : a);
}

UGenericVehicleDriverComponentComponent::UGenericVehicleDriverComponentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Vehicle Driver");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostDeferredPhysTick = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

bool UGenericVehicleDriverComponentComponent::OnActivateVehicleComponent()
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

	Engine = LinkToEngine.GetObject<UVehicleEngineBaseComponent>(GetOwner()); 
	SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner());
	BrakeSystem = LinkToBrakeSystem.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
	HandBrake = LinkToHandBrake.GetObject<UVehicleHandBrakeBaseComponent>(GetOwner());
	GearBox = LinkToGearBox.GetObject<UVehicleGearBoxBaseComponent>(GetOwner());

	if (!Engine)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Engine  isn't connected"));
	}

	if (!BrakeSystem)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Brake system isn't connected"));
	}

	if (!SteeringRack)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Steering rack isn't connected"));
	}

	if (!HandBrake)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Hand brake isn't connected"));
	}

	if (!GearBox)
	{
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("GearBox isn't connected"));
	}

	if (!Engine || !SteeringRack || !BrakeSystem || !HandBrake)
	{
		SetHealth(EVehicleComponentHealth::Warning);
	}

	PublisherHelper.Advertise();
	ListenerHelper.StartListen();

	return true;
}

void UGenericVehicleDriverComponentComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();


	PublisherHelper.Shutdown();
	ListenerHelper.StopListen();
}

void UGenericVehicleDriverComponentComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	if (!HealthIsWorkable())
	{
		return;
	}

	if (Publisher && Publisher->IsOk())
	{
		FWheeledVehicleStateExtra Extra{ VehicleKinematic, GetRelativeTransform() };

		if (GearBox)
		{
			Extra.Gear = GearBox->GetGear();
		}

		if (GetWheeledVehicle()->Is4WDVehicle())
		{
			auto WheelDataConv = [](const USodaVehicleWheelComponent& In, FWheeledVehicleWheelState& Out)
				{
					Out.AngularVelocity = In.AngularVelocity;
					Out.Torq = In.ReqTorq;
					Out.BrakeTorq = In.ReqBrakeTorque;
					Out.Steer = In.Steer;
				};

			WheelDataConv(*GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL), Extra.WheelStates[0]);
			WheelDataConv(*GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR), Extra.WheelStates[1]);
			WheelDataConv(*GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL), Extra.WheelStates[2]);
			WheelDataConv(*GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR), Extra.WheelStates[3]);
		}

		Publisher->Publish(DeltaTime, GetHeaderVehicleThread(), Extra);
	}

}

void UGenericVehicleDriverComponentComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	static const std::chrono::nanoseconds Timeout(500000000ll); // 500ms

	soda::FWheeledVehiclControl Control;

	bVapiPing = bIsVehicleDriveDebugMode;

	if (VehicleControl && VehicleControl->GetControl(Control))
	{
		bVapiPing = ((SodaApp.GetRealtimeTimestamp() - Control.RecvTimestamp > Timeout));
	}

	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();

	if (VehicleInput)
	{
		bIsADMode = VehicleInput->bADModeInput;
	}
	
	if (GetDriveMode() == ESodaVehicleDriveMode::Manual || GetDriveMode() == ESodaVehicleDriveMode::ReadyToAD) // Manual mode
	{
		if (VehicleInput)
		{
			Gear = VehicleInput->GetGearInput();
			float SteerReq = VehicleInput->GetSteeringInput();
			float ThrottleReq = VehicleInput->GetThrottleInput();
			float BrakeReq = VehicleInput->GetBrakeInput();
			float HandBrakeReq = 0;

			switch (Gear)
			{
			case ENGear::Neutral:
				ThrottleReq = 0;
				break;
			case ENGear::Drive:
			case ENGear::Reverse:
				break;
			case ENGear::Park:
				ThrottleReq = 0;
				BrakeReq = 0;
				HandBrakeReq = 1;
			}

			if (GearBox) GearBox->SetGear(Gear);
			if (Engine) Engine->RequestByRatio(ThrottleReq);
			if (BrakeSystem) BrakeSystem->RequestByRatio(BrakeReq, DeltaTime);
			if (SteeringRack) SteeringRack->RequestByRatio(SteerReq);
			if (HandBrake) HandBrake->RequestByRatio(HandBrakeReq);
		}
	}
	else if ((GetDriveMode() == ESodaVehicleDriveMode::SafeStop) || (GetDriveMode() == ESodaVehicleDriveMode::AD && !bVapiPing)) // SafeStop mode
	{
		if (GearBox) GearBox->SetGear(ENGear::Neutral);
		if (Engine) Engine->RequestByRatio(0.0);
		if (BrakeSystem) BrakeSystem->RequestByRatio(0.0, DeltaTime);
		if (SteeringRack) SteeringRack->RequestByRatio(0.0);
		if (HandBrake) HandBrake->RequestByRatio(1.0);
	}
	else // AD mode
	{
		Gear = (ENGear)Control.GearReq;
		float SteerReq = Control.SteerReq;
		float AccReq = 0;
		float DeaccReq = 0;
		float HandBrakeReq = 0;

		if (Control.AccDecelReq >= 0)
		{
			AccReq = Control.AccDecelReq;
		}
		else
		{
			DeaccReq = -Control.AccDecelReq;
		}

		switch (Gear)
		{
		case ENGear::Neutral:
			AccReq = 0;
			break;
		case ENGear::Drive:
		case ENGear::Reverse:
			break;
		case ENGear::Park:
			AccReq = 0;
			DeaccReq = 0;
			HandBrakeReq = 1;
			break;
		}

		if (GearBox) GearBox->SetGear(Gear);
		if (Engine) Engine->RequestByTorque(AccReq / EngineToWheelsRatio * VehicleWheelRadius / 100.0);
		if (BrakeSystem) BrakeSystem->RequestByAcceleration(DeaccReq, DeltaTime);
		if (SteeringRack) SteeringRack->RequestByAngle(SteerReq);
		if (HandBrake) HandBrake->RequestByRatio(HandBrakeReq);
	}
}

ESodaVehicleDriveMode UGenericVehicleDriverComponentComponent::GetDriveMode() const
{
	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();

	if (!VehicleInput || VehicleInput->bSafeStopInput) return ESodaVehicleDriveMode::SafeStop;

	if (bVapiPing)
	{

		if (VehicleInput->bADModeInput) return  ESodaVehicleDriveMode::AD;
		else return ESodaVehicleDriveMode::ReadyToAD;

	}

	if (VehicleInput->bADModeInput) return ESodaVehicleDriveMode::SafeStop;
	
	return ESodaVehicleDriveMode::Manual;
}

void UGenericVehicleDriverComponentComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetHealth() != EVehicleComponentHealth::Disabled)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		Canvas->SetDrawColor(FColor::White);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SteerReq: %.2f"), MsgIn.steer_req), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("AccDecelReq: %.1f"), MsgIn.acc_decel_req), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("GearReq: %d"), (int)MsgIn.gear_req), 16, YPos);
	}
}


void UGenericVehicleDriverComponentComponent::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	ListenerHelper.OnPropertyChanged(PropertyChangedEvent);
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
}

void UGenericVehicleDriverComponentComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	PublisherHelper.OnSerialize(Ar);
	ListenerHelper.OnSerialize(Ar);
}


bool UGenericVehicleDriverComponentComponent::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}

void UGenericVehicleDriverComponentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PublisherHelper.Tick();
}

#if WITH_EDITOR
void UGenericVehicleDriverComponentComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PublisherHelper.OnPropertyChanged(PropertyChangedEvent);
	ListenerHelper.OnPropertyChanged(PropertyChangedEvent);
}

void UGenericVehicleDriverComponentComponent::PostInitProperties()
{
	Super::PostInitProperties();
	PublisherHelper.RefreshClass();
	ListenerHelper.RefreshClass();
}
#endif
