// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1UltrasoncPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/UltrasonicSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>


bool UProtoV1UltrasoncPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1UltrasoncPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1UltrasoncPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1UltrasoncPublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1UltrasoncPublisher::Advertise() IP address isn't valid"));
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

void UProtoV1UltrasoncPublisher::Shutdown()
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

bool UProtoV1UltrasoncPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& EchoCollections)
{
	Msg.device_timestamp = soda::RawTimestamp<std::chrono::milliseconds>(Header.Timestamp);
	Msg.ultrasonics.resize(EchoCollections.Num());

	for (int i = 0; i < EchoCollections.Num(); ++i)
	{
		auto& Src = EchoCollections[i];
		auto& Dst = Msg.ultrasonics[i];

		Dst.hfov = Src.FOV_Horizont;
		Dst.vfov = Src.FOV_Vertical;
		Dst.properties = (Src.bIsTransmitter) ? soda::sim::proto_v1::Ultrasonic::Properties::transmitter : soda::sim::proto_v1::Ultrasonic::Properties::none;
		Dst.echoes.resize(Src.Echos.Num());

		for (size_t j = 0; j < Src.Echos.Num(); ++j)
		{
			Dst.echoes[j] = Src.Echos[j].BeginDistance / 100.f;
		}
	}

	return Publish(Msg);
}

bool UProtoV1UltrasoncPublisher::Publish(const soda::sim::proto_v1::UltrasonicsHub& Scan)
{
	if (!Socket)
	{
		return false;
	}

	std::stringstream sout(std::ios_base::out | std::ios_base::binary);
	int const points_count = soda::sim::proto_v1::write(sout, Scan);
	if (!sout)
		UE_LOG(LogSoda, Fatal, TEXT("UProtoV1UltrasoncPublisher::Publish() failed to serialize scan"));
	sout.seekp(0, std::ios_base::end);

	if (bAsync)
	{
		if (!AsyncTask->Publish(sout.str().data(), sout.str().length()))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1UltrasoncPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)sout.str().data(), sout.str().length(), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1UltrasoncPublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}