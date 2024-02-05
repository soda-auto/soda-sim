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

	bWheelRadiusValid = (Engine) && (Engine->FindWheelRadius(WheelRadius)) && (WheelRadius > 1.0);
	if (!bWheelRadiusValid)
	{
		SetHealth(EVehicleComponentHealth::Warning);
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Can't estimate wheel radius"));
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
			Extra.GearState = GearBox->GetGearState();
			Extra.GearNum = GearBox->GetGearNum();
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

	soda::FWheeledVehiclControlMode1 Control;

	bVapiPing = bIsVehicleDriveDebugMode;

	if (VehicleControl && VehicleControl->GetControl(Control))
	{
		bVapiPing = ((SodaApp.GetRealtimeTimestamp() - Control.RecvTimestamp < Timeout));
	}

	UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput();

	if (VehicleInput)
	{
		bADModeEnbaled = VehicleInput->GetInputState().bADModeEnbaled;
		bSafeStopEnbaled = VehicleInput->GetInputState().bSafeStopEnbaled;
	}
	
	if (GetDriveMode() == ESodaVehicleDriveMode::Manual || GetDriveMode() == ESodaVehicleDriveMode::ReadyToAD) // Manual mode
	{
		if (VehicleInput)
		{
			float SteerReq = VehicleInput->GetInputState().Steering;
			float ThrottleReq = VehicleInput->GetInputState().Throttle;
			float BrakeReq = VehicleInput->GetInputState().Brake;
			float HandBrakeReq = 0;

			if (VehicleInput->GetInputState().IsNeutralGear())
			{
				ThrottleReq = 0;
			}
			else if (VehicleInput->GetInputState().IsParkGear())
			{
				ThrottleReq = 0;
				BrakeReq = 0;
				HandBrakeReq = 1;
			}

			if (GearBox)
			{
				GearBox->AcceptGearFromVehicleInput(VehicleInput);
				GearState = GearBox->GetGearState();
				GearNum = GearBox->GetGearNum();
			}
			if (Engine) Engine->RequestByRatio(ThrottleReq);
			if (BrakeSystem) BrakeSystem->RequestByRatio(BrakeReq, DeltaTime);
			if (SteeringRack) SteeringRack->RequestByRatio(SteerReq);
			if (HandBrake) HandBrake->RequestByRatio(HandBrakeReq);
		}
	}
	else if ((GetDriveMode() == ESodaVehicleDriveMode::SafeStop) || (GetDriveMode() == ESodaVehicleDriveMode::AD && !bVapiPing)) // SafeStop mode
	{
		if (GearBox) GearBox->SetGearByState(EGearState::Neutral);
		if (Engine) Engine->RequestByRatio(0.0);
		if (BrakeSystem) BrakeSystem->RequestByRatio(0.0, DeltaTime);
		if (SteeringRack) SteeringRack->RequestByRatio(0.0);
		if (HandBrake) HandBrake->RequestByRatio(1.0);
	}
	else // AD mode
	{
		GearState = Control.GearStateReq;
		GearNum = Control.GearNumReq;

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

		switch (GearState)
		{
		case EGearState::Neutral:
			AccReq = 0;
			break;
		case EGearState::Drive:
		case EGearState::Reverse:
			break;
		case EGearState::Park:
			AccReq = 0;
			DeaccReq = 0;
			HandBrakeReq = 1;
			break;
		}

		if (GearBox)
		{
			if (GearState == EGearState::Drive && GearNum != 0)
			{
				GearBox->SetGearByNum(GearNum);
			}
			else
			{
				GearBox->SetGearByState(GearState);
			}
		}
		if (Engine)
		{
			float EngineToWheelsRatio;
			const bool bRatioValid = Engine->FindToWheelRatio(EngineToWheelsRatio) && !FMath::IsNearlyZero(EngineToWheelsRatio, 0.01);
			if (bRatioValid && bWheelRadiusValid)
			{
				Engine->RequestByTorque(AccReq / EngineToWheelsRatio * WheelRadius / 100.0 * GetWheeledComponentInterface()->GetVehicleMass());
			}
		}
		if (BrakeSystem) BrakeSystem->RequestByAcceleration(DeaccReq, DeltaTime);
		if (SteeringRack) SteeringRack->RequestByAngle(SteerReq);
		if (HandBrake) HandBrake->RequestByRatio(HandBrakeReq);
	}
}

ESodaVehicleDriveMode UGenericVehicleDriverComponentComponent::GetDriveMode() const
{
	if (bSafeStopEnbaled)
	{
		return ESodaVehicleDriveMode::SafeStop;
	}
	else if (bVapiPing)
	{
		if (bADModeEnbaled)
		{
			return  ESodaVehicleDriveMode::AD;
		}
		else
		{
			return ESodaVehicleDriveMode::ReadyToAD;
		}
	} 
	else if (bADModeEnbaled)
	{
		return ESodaVehicleDriveMode::SafeStop;
	}
	else
	{
		return ESodaVehicleDriveMode::Manual;
	}
}

void UGenericVehicleDriverComponentComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	const FColor SubCaption(170, 170, 170);
	UFont* RenderFont = GEngine->GetSmallFont();

	if (Publisher)
	{
		Canvas->SetDrawColor(SubCaption);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Publisher :")), 16, YPos);
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
	if (VehicleControl)
	{
		Canvas->SetDrawColor(SubCaption);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Control :")), 16, YPos);
		VehicleControl->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericVehicleDriverComponentComponent::GetRemark() const
{
	return VehicleControl ? VehicleControl->GetRemark() : "null";
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
