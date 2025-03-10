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
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include <cmath>


static inline float NormAng(float a)
{
	return (a > M_PI) ? (a - 2.0 * M_PI) : ((a < -M_PI) ? (a + 2 * M_PI) : a);
}

UGenericVehicleDriverComponent::UGenericVehicleDriverComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Vehicle Driver");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostDeferredPhysTick = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

bool UGenericVehicleDriverComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!GetWheeledVehicle()->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	Engine = LinkToEngine.GetObject<UVehicleEngineBaseComponent>(GetOwner()); 
	SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner());
	BrakeSystem = LinkToBrakeSystem.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
	HandBrake = LinkToHandBrake.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
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

	bWheelRadiusValid = false;

	if (IsValid(VehicleControl))
	{
		if (VehicleControl->StartListen(this))
		{
			return true;
		}
		else
		{
			SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize VehicleControl");
			return false;
		}
	}

	return true;
}

void UGenericVehicleDriverComponent::OnPostActivateVehicleComponent()
{
	bWheelRadiusValid = (Engine) && (Engine->FindWheelRadius(WheelRadius)) && (WheelRadius > 1.0);
	if (!bWheelRadiusValid)
	{
		SetHealth(EVehicleComponentHealth::Warning);
		AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Can't estimate wheel radius"));
	}
}

void UGenericVehicleDriverComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(VehicleControl))
	{
		VehicleControl->StopListen();
	}
}

void UGenericVehicleDriverComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	static const std::chrono::nanoseconds Timeout(500000000ll); // 500ms

	bVapiPing = bIsVehicleDriveDebugMode;

	if (VehicleControl && VehicleControl->GetControl(Control))
	{
		bVapiPing = ((SodaApp.GetRealtimeTimestamp() - Control.Timestamp < Timeout));
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
			if (HandBrake) HandBrake->RequestByRatio(HandBrakeReq, DeltaTime);
		}
	}
	else if ((GetDriveMode() == ESodaVehicleDriveMode::SafeStop) || (GetDriveMode() == ESodaVehicleDriveMode::AD && !bVapiPing)) // SafeStop mode
	{
		if (GearBox) GearBox->SetGearByState(EGearState::Neutral);
		if (Engine) Engine->RequestByRatio(0.0);
		if (BrakeSystem) BrakeSystem->RequestByRatio(0.0, DeltaTime);
		if (SteeringRack) SteeringRack->RequestByRatio(0.0);
		if (HandBrake) HandBrake->RequestByRatio(1.0, DeltaTime);
	}
	else // AD mode
	{
		if (Control.bGearIsSet)
		{
			GearState = Control.GearStateReq;
			GearNum = Control.GearNumReq;
		}
		else
		{
			if (!Control.bTargetSpeedIsSet)
			{
				if (Control.TargetSpeedReq > 0)
				{
					GearState = EGearState::Drive;
				}
				else
				{
					GearState = EGearState::Reverse;
				}
			}
			else
			{
				GearState = EGearState::Drive;
			}
		}

		if (GearBox)
		{
			if ((GearState == EGearState::Drive || GearState == EGearState::Reverse) && GearNum != 0)
			{
				GearBox->SetGearByNum(GearNum);
			}
			else
			{
				GearBox->SetGearByState(GearState);
			}
		}

		//TODO: Set brake only if Engine Torque ==0

		if (Engine)
		{
			
			if (GearState == EGearState::Drive || GearState == EGearState::Reverse)
			{
				bool bIsOverSpeed = false;
				if (Control.bTargetSpeedIsSet)
				{
					if (GearState == EGearState::Drive && VehicleKinematic.GetForwardSpeed() > Control.TargetSpeedReq - TargetSpeedDelta)
					{
						bIsOverSpeed = true;
					}
					else if (GearState == EGearState::Reverse && VehicleKinematic.GetForwardSpeed() < Control.TargetSpeedReq + TargetSpeedDelta)
					{
						bIsOverSpeed = true;
					}
				}

				if (bIsOverSpeed)
				{
					Engine->RequestByRatio(0);
				} 
				else if (Control.DriveEffortReqMode == soda::FGenericWheeledVehiclControl::EDriveEffortReqMode::ByAcc)
				{
					if (Control.DriveEffortReq.ByAcc >= 0)
					{
						float EngineToWheelsRatio;
						const bool bRatioValid = Engine->FindToWheelRatio(EngineToWheelsRatio) && !FMath::IsNearlyZero(EngineToWheelsRatio, 0.01);
						if (bRatioValid && bWheelRadiusValid)
						{
							Engine->RequestByTorque(Control.DriveEffortReq.ByAcc / EngineToWheelsRatio * WheelRadius / 100.0 * GetWheeledComponentInterface()->GetVehicleMass());
						}
					}
					else
					{
						Engine->RequestByRatio(0);
					}
				}
				else if (Control.DriveEffortReqMode == soda::FGenericWheeledVehiclControl::EDriveEffortReqMode::ByRatio)
				{
					if (Control.DriveEffortReq.ByRatio >= 0)
					{
						Engine->RequestByRatio(Control.DriveEffortReq.ByRatio);
					}
					else
					{
						Engine->RequestByRatio(0);
					}
				}
			}
			else
			{
				Engine->RequestByRatio(0);
			}
		}

		if (BrakeSystem)
		{
			bool bIsOverSpeed = false;
			if (Control.bTargetSpeedIsSet)
			{
				if (GearState == EGearState::Drive && VehicleKinematic.GetForwardSpeed() < Control.TargetSpeedReq + TargetSpeedDelta)
				{
					bIsOverSpeed = true;
				}
				else if (GearState == EGearState::Reverse && VehicleKinematic.GetForwardSpeed() > Control.TargetSpeedReq - TargetSpeedDelta)
				{
					bIsOverSpeed = true;
				}
			}

			if (bIsOverSpeed)
			{
				BrakeSystem->RequestByRatio(0, DeltaTime);
			}
			else if (Control.DriveEffortReqMode == soda::FGenericWheeledVehiclControl::EDriveEffortReqMode::ByAcc)
			{
				if (Control.DriveEffortReq.ByAcc < 0)
				{
					BrakeSystem->RequestByAcceleration(-Control.DriveEffortReq.ByAcc, DeltaTime);
				}
				else
				{
					BrakeSystem->RequestByRatio(0, DeltaTime);
				}
			}
			else if (Control.DriveEffortReqMode == soda::FGenericWheeledVehiclControl::EDriveEffortReqMode::ByRatio)
			{
				if (Control.DriveEffortReq.ByRatio < 0)
				{
					BrakeSystem->RequestByRatio(-Control.DriveEffortReq.ByRatio, DeltaTime);
				}
				else
				{
					BrakeSystem->RequestByRatio(0, DeltaTime);
				}
			}
		}

		if (SteeringRack)
		{
			// TODO: Support of Control.SteeringAngleVelocity
			if (Control.SteerReqMode == soda::FGenericWheeledVehiclControl::ESteerReqMode::ByAngle)
			{
				SteeringRack->RequestByAngle(Control.SteerReq.ByAngle);
			} 
			else if (Control.SteerReqMode == soda::FGenericWheeledVehiclControl::ESteerReqMode::ByRatio)
			{
				SteeringRack->RequestByRatio(Control.SteerReq.ByRatio);
			}
		}

		if (HandBrake)
		{
			if (GearState == EGearState::Park)
			{
				HandBrake->RequestByRatio(1.0, DeltaTime);
			}
			else
			{
				HandBrake->RequestByRatio(0.0, DeltaTime);
			}
		}
	}

	SyncDataset();
}

ESodaVehicleDriveMode UGenericVehicleDriverComponent::GetDriveMode() const
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

void UGenericVehicleDriverComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	const FColor SubCaption(170, 170, 170);
	UFont* RenderFont = GEngine->GetSmallFont();

	/*
	if (Publisher)
	{
		Canvas->SetDrawColor(SubCaption);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Publisher :")), 16, YPos);
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
	*/
	if (VehicleControl)
	{
		Canvas->SetDrawColor(SubCaption);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Control :")), 16, YPos);
		VehicleControl->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericVehicleDriverComponent::GetRemark() const
{
	return VehicleControl ? VehicleControl->GetRemark() : "null";
}

/*
bool UGenericVehicleDriverComponent::IsVehicleComponentInitializing() const
{
	return PublisherHelper.IsPublisherInitializing();
}
*/

void UGenericVehicleDriverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//PublisherHelper.Tick();
}
