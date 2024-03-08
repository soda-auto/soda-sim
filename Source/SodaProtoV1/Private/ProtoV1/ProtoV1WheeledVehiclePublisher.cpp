// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1WheeledVehiclePublisher.h"
#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"

bool UProtoV1WheeledVehiclePublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't set SetBroadcast mode"));
			Shutdown();
			return false;
		}
	}
	else
	{
		bool Valid;
		Addr->SetIp((const TCHAR*)(*Address), Valid);
		if (!Valid)
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() IP address isn't valid"));
			Shutdown();
			return false;
		}
	}

	// Create AsyncTask
	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);

	return true;
}

void UProtoV1WheeledVehiclePublisher::Shutdown()
{
	if (AsyncTask)
	{
		AsyncTask->Finish();
		SodaApp.EthTaskManager.RemoteTask(AsyncTask);
		AsyncTask.Reset();
	}

	if (Socket)
	{
		Socket->Close();
		Socket.Reset();
	}

	if (Addr)
	{
		Addr.Reset();
	}
}

bool UProtoV1WheeledVehiclePublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const FWheeledVehicleSensorData& SensorData)
{
	soda::sim::proto_v1::GenericVehicleState Msg{};
	Msg.gear_state = soda::sim::proto_v1::EGearState(SensorData.GearBox ? SensorData.GearBox->GetGearState() : EGearState::Neutral);
	Msg.gear_num = SensorData.GearBox ? SensorData.GearBox->GetGearNum() : 0;
	Msg.mode = soda::sim::proto_v1::EControlMode(SensorData.VehicleDriver ? SensorData.VehicleDriver->GetDriveMode() : ESodaVehicleDriveMode::Manual);
	Msg.steer = SensorData.WheeledVehicle->Is4WDVehicle()
		? -(SensorData.WheeledVehicle->GetWheel4WD(E4WDWheelIndex::FL)->Steer + SensorData.WheeledVehicle->GetWheel4WD(E4WDWheelIndex::FR)->Steer) / 2
		: 0;
	if (SensorData.WheeledVehicle->Is4WDVehicle())
	{
		for (int i = 0; i < 4; i++)
		{
			const auto & Wheel = SensorData.WheeledVehicle->GetWheels4WD()[i];
			Msg.wheels_state[i].ang_vel = Wheel->AngularVelocity;
			Msg.wheels_state[i].torq = Wheel->ReqTorq;
			Msg.wheels_state[i].brake_torq = Wheel->ReqBrakeTorque;
		}
	}
	Msg.timestemp = soda::RawTimestamp<std::chrono::nanoseconds>(Header.Timestamp);
	return Publish(Msg);
}

bool UProtoV1WheeledVehiclePublisher::Publish(const soda::sim::proto_v1::GenericVehicleState& Scan)
{
	if (!Socket)
	{
		return false;
	}

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Scan, sizeof(Scan)))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1WheeledVehiclePublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((uint8*) &Scan, sizeof(Scan), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1WheeledVehiclePublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}