// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1RacingPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/RacingSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

bool UProtoV1RacingPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1RacingPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1RacingPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RacingPublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RacingPublisher::Advertise() IP address isn't valid"));
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

void UProtoV1RacingPublisher::Shutdown()
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

bool UProtoV1RacingPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& SensorData)
{
	soda::sim::proto_v1::RacingSensor Msg;

	Msg.left_border_offset = SensorData.LeftBorderOffset / 100.0;
	Msg.right_border_offset = SensorData.RightBorderOffset / 100.0;
	Msg.center_line_yaw = -SensorData.CenterLineYaw;
	Msg.lap_caunter = SensorData.LapCaunter;
	Msg.covered_distance_current_lap = SensorData.CoveredDistanceCurrentLap / 100.0;
	Msg.covered_distance_full = SensorData.CoveredDistanceFull / 100.0;
	Msg.border_is_valid = SensorData.bBorderIsValid;
	Msg.lap_counter_is_valid = SensorData.bLapCounterIsValid;
	Msg.timestemp = soda::RawTimestamp<std::chrono::nanoseconds>(Header.Timestamp);
	Msg.start_timestemp = soda::RawTimestamp<std::chrono::nanoseconds>(SensorData.StartTimestemp);
	Msg.lap_timestemp = soda::RawTimestamp<std::chrono::nanoseconds>(SensorData.LapTimestemp);

	Publish(Msg);
	
	return true;
}

bool UProtoV1RacingPublisher::Publish(const soda::sim::proto_v1::RacingSensor& Msg)
{
	if (!Socket)
	{
		return false;
	}

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Msg, sizeof(Msg)))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1RacingPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)&Msg, sizeof(Msg), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RacingPublisher::Publish(); Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1RacingPublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}