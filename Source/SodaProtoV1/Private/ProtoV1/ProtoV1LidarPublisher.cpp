// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1LidarPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/LidarSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"


bool UProtoV1LidarPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1LidarPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1LidarPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1LidarPublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1LidarPublisher::Advertise() IP address isn't valid"));
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

void UProtoV1LidarPublisher::Shutdown()
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

	ScanID = 0;
}

bool UProtoV1LidarPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan)
{
	Msg.device_id = DeviceID;
	Msg.device_timestamp = soda::RawTimestamp<std::chrono::milliseconds>(Header.Timestamp);
	Msg.scan_id = ScanID++;
	Msg.block_id = 0;
	Msg.block_count = (Scan.Points.Num() / PointsPerDatagram) + ((Scan.Points.Num() % PointsPerDatagram) ? 1 : 0);

	for (int k = 0; k < Scan.Points.Num(); )
	{
		int BlockSize = Scan.Points.Num() - k;
		if (BlockSize > PointsPerDatagram) BlockSize = PointsPerDatagram;

		Msg.points.resize(BlockSize);
		for (int i = 0; i < BlockSize; ++i)
		{
			auto& Src = Scan.Points[k + i];
			auto& Dst = Msg.points[i];

			Dst.coords.x = Src.Location.X / 100;
			Dst.coords.y = -Src.Location.Y / 100;
			Dst.coords.z = Src.Location.Z / 100;
			Dst.layer = Src.Layer;
			Dst.properties = (Src.Status == soda::ELidarPointStatus::Valid 
				? soda::sim::proto_v1::LidarScanPoint::Properties::Valid 
				: soda::sim::proto_v1::LidarScanPoint::Properties::None);
		}
		
		Publish(Msg);
		k += BlockSize;
		++Msg.block_id;
	}
	return true;
}

bool UProtoV1LidarPublisher::Publish(const soda::sim::proto_v1::LidarScan& ScanToPublish)
{
	if (!Socket)
	{
		return false;
	}

	std::stringstream sout(std::ios_base::out | std::ios_base::binary);
	int const points_count = soda::sim::proto_v1::write(sout, ScanToPublish);
	if (!sout)
	{
		UE_LOG(LogSoda, Fatal, TEXT("UProtoV1LidarPublisher::Publish() failed to serialize scan"));
	}
	sout.seekp(0, std::ios_base::end);
	auto const length = sout.tellp();

	if (bAsync)
	{
		if (!AsyncTask->Publish((const uint8*)sout.str().data(), sout.str().length()))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1LidarPublisher::Publish(). Skipped one frame"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1LidarPublisher::Publish() Can't send(), error code %i, error = %s"), int32(ErrorCode), ISocketSubsystem::Get()->GetSocketError(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1LidarPublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}