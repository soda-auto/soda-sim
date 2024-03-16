// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1RadarPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/RadarSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>

bool UProtoV1RadarPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1RadarPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1RadarPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RadarPublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RadarPublisher::Advertise() IP address isn't valid"));
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

void UProtoV1RadarPublisher::Shutdown()
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

bool UProtoV1RadarPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray<FRadarParams>& Params, const FRadarClusters& Clusters, const FRadarObjects& Objects)
{
	Msg.device_id = DeviceID;
	Msg.device_timestamp = soda::RawTimestamp<std::chrono::milliseconds>(SodaApp.GetSimulationTimestamp());
	Msg.items.resize(Clusters.Clusters.Num());

	for(int i = 0; i< Clusters.Clusters.Num(); ++i)
	{
		auto& Src = Clusters.Clusters[i];
		auto& Dst = Msg.items[i];

		Dst.id = i;
		Dst.coords.x = Src.LocalHitPosition.X / 100;
		Dst.coords.y = -Src.LocalHitPosition.Y / 100;
		Dst.coords.z = Src.LocalHitPosition.Z / 100;
		Dst.velocity.lat = Clusters.Clusters[i].Lat;
		Dst.velocity.lon = Clusters.Clusters[i].Lon;
		Dst.property = soda::sim::proto_v1::RadarCluster::property_t::moving;
		Dst.RCS = Clusters.Clusters[i].RCS;
	}

	return Publish(Msg);
}

bool UProtoV1RadarPublisher::Publish(const soda::sim::proto_v1::RadarScan& Scan)
{
	if (!Socket)
	{
		return false;
	}

	std::stringstream sout(std::ios_base::out | std::ios_base::binary);
	int const points_count = soda::sim::proto_v1::write(sout, Scan);
	if (!sout) UE_LOG(LogSoda, Fatal, TEXT("UProtoV1RadarPublisher::Publish() failed to serialize scan"));
	sout.seekp(0, std::ios_base::end);

	if (bAsync)
	{
		if (!AsyncTask->Publish(sout.str().data(), sout.str().length()))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1RadarPublisher::PublishAsync(). Skipped one frame"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1RadarPublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1RadarPublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}