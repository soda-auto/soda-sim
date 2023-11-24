// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Drivers/VehicleDriverGenericComponent.h"
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

UVehicleDriverGenericComponent::UVehicleDriverGenericComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("General UDP");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostDeferredPhysTick = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

bool UVehicleDriverGenericComponent::OnActivateVehicleComponent()
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

	Shutdown();

	Engine = LinkToEngine.GetObject<UVehicleEngineBaseComponent>(GetOwner()); 
	SteeringRack = LinkToSteering.GetObject<UVehicleSteeringRackBaseComponent>(GetOwner());
	BrakeSystem = LinkToBrakeSystem.GetObject<UVehicleBrakeSystemBaseComponent>(GetOwner());
	HandBrake = LinkToHandBrake.GetObject<UVehicleHandBrakeBaseComponent>(GetOwner());
	GearBox = LinkToGearBox.GetObject<UVehicleGearBoxBaseComponent>(GetOwner());

	//Send socket
	Publisher.Advertise();
	if (!Publisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("Can't init SendSocket"));
		return false;
	}

	//Recv socket
	FIPv4Endpoint Endpoint(FIPv4Address(0, 0, 0, 0), RecvPort);
	ListenSocket = FUdpSocketBuilder(TEXT("PathViwer"))
					   .AsNonBlocking()
					   .AsReusable()
					   .BoundToEndpoint(Endpoint)
					   .WithReceiveBufferSize(0xFFFF);
	check(ListenSocket);
	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
	UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("VApiUdp"));
	check(UDPReceiver);
	UDPReceiver->OnDataReceived().BindUObject(this, &UVehicleDriverGenericComponent::Recv);
	UDPReceiver->Start();

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

	return true;
}

void UVehicleDriverGenericComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	Shutdown();
}

void UVehicleDriverGenericComponent::Shutdown()
{
	if (UDPReceiver)
	{
		UDPReceiver->Stop();
		delete UDPReceiver;
	}
	UDPReceiver = 0;

	if (ListenSocket)
	{
		ListenSocket->Close();
		delete ListenSocket;
	}
	ListenSocket = 0;

	Publisher.Shutdown();

	Engine = nullptr;
	SteeringRack = nullptr;
	BrakeSystem = nullptr;
	HandBrake = nullptr;
	GearBox = nullptr;
}

void UVehicleDriverGenericComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	if (HealthIsWorkable())
	{
		FTransform WorldPose;
		FVector WorldVel;
		FVector LocalAcc;
		FVector Gyro;
		VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
		FRotator WorldRot = WorldPose.Rotator();
		FVector WorldLoc = WorldPose.GetTranslation();
		FVector LocVel = WorldRot.UnrotateVector(WorldVel);
		double Lon, Lat, Alt;
		GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);

		static auto ToSodaVec = [](const FVector& V) { return soda::Vector{ V.X, -V.Y, V.Z }; };
		static auto ToSodaRot = [](const FVector& V) { return soda::Vector{ -V.X, V.Y, -V.Z }; };

		soda::GenericVehicleState MsgOut {};
		MsgOut.navigation_state.timestamp = soda::RawTimestamp<std::chrono::nanoseconds>(Timestamp);
		MsgOut.navigation_state.longitude = Lon;
		MsgOut.navigation_state.latitude = Lat;
		MsgOut.navigation_state.altitude = Alt;
		MsgOut.navigation_state.rotation.roll = -NormAngRad(WorldRot.Roll / 180.0 * M_PI);
		MsgOut.navigation_state.rotation.pitch = NormAngRad(WorldRot.Pitch / 180.0 * M_PI);
		MsgOut.navigation_state.rotation.yaw = -NormAngRad(WorldRot.Yaw / 180.0 * M_PI);
		MsgOut.navigation_state.position = ToSodaVec(WorldLoc * 0.01);
		MsgOut.navigation_state.world_velocity = ToSodaVec(WorldVel * 0.01);
		MsgOut.navigation_state.loc_velocity = ToSodaVec(LocVel * 0.01);
		MsgOut.navigation_state.angular_velocity = ToSodaRot(Gyro);
		MsgOut.navigation_state.acceleration = ToSodaVec(LocalAcc * 0.01);
		MsgOut.steer = SteeringRack ? -SteeringRack->GetCurrentSteer() : 0;
		MsgOut.gear = soda::EGear(GetGear());
		MsgOut.mode = soda::EControlMode(GetDriveMode());
		for (int i = 0; i < 4; i++)
		{
			USodaVehicleWheelComponent* Wheel = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex(i));
			MsgOut.wheels_state[i].ang_vel = Wheel->AngularVelocity;
			MsgOut.wheels_state[i].torq = Wheel->ReqTorq;
			MsgOut.wheels_state[i].brake_torq = Wheel->ReqBrakeTorque;
		}
		Publisher.Publish(MsgOut);
	}
}

void UVehicleDriverGenericComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	static const std::chrono::nanoseconds Timeout(500000000ll); // 500ms

	bVapiPing = bIsVehicleDriveDebugMode || ((SodaApp.GetRealtimeTimestamp() - RecvTimestamp > Timeout));

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
		Gear = (ENGear)MsgIn.gear_req;
		float SteerReq = -MsgIn.steer_req;
		float AccReq = 0;
		float DeaccReq = 0;
		float HandBrakeReq = 0;

		if (MsgIn.acc_decel_req >= 0)
		{
			AccReq = MsgIn.acc_decel_req;
		}
		else
		{
			DeaccReq = -MsgIn.acc_decel_req;
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

ESodaVehicleDriveMode UVehicleDriverGenericComponent::GetDriveMode() const
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

void UVehicleDriverGenericComponent::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{	
	if (ArrayReaderPtr->Num() == sizeof(MsgIn))
	{
		::memcpy(&MsgIn, ArrayReaderPtr->GetData(), ArrayReaderPtr->Num());
		RecvTimestamp = SodaApp.GetRealtimeTimestamp();
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("UVehicleDriverGenericComponent::Recv(); Got %i bytes, expected %i"), ArrayReaderPtr->Num(), sizeof(MsgIn));
	}
}

void UVehicleDriverGenericComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetHealth() != EVehicleComponentHealth::Disabled)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("SteerReq: %.2f"), MsgIn.steer_req), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("AccDecelReq: %.1f"), MsgIn.acc_decel_req), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("GearReq: %d"), (int)MsgIn.gear_req), 16, YPos);
	}
}