// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/OXTSPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>

FOXTSPublisher::FOXTSPublisher()
{

}

void FOXTSPublisher::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("FOXTSPublisher::Advertise() Can't create socket"));
		Shutdown();
		return;
	}
	if (bIsBroadcast)
	{
		Socket->SetBroadcast(true);
	}

	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("FOXTSPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return;
	}

	bool Valid;
	Addr->SetIp((const TCHAR*)(*Address), Valid);
	if (!Valid)
	{
		UE_LOG(LogSoda, Error, TEXT("FOXTSPublisher::Advertise() IP address isn't valid"));
		Shutdown();
		return;
	}
	Addr->SetPort(Port);

	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);
}

void FOXTSPublisher::Shutdown()
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


void FOXTSPublisher::Publish(const soda::OxtsPacket& Msg, bool bAsync)
{
	if (!Socket) return;

	BitStream.SetOffset(0);
	BitStream.SetBytes(Msg.sync, 1);
	BitStream.SetBytes(Msg.time, 2);
	BitStream.SetBytes(Msg.accel_x, 3);
	BitStream.SetBytes(Msg.accel_y, 3);
	BitStream.SetBytes(Msg.accel_z, 3);
	BitStream.SetBytes(Msg.gyro_x, 3);
	BitStream.SetBytes(Msg.gyro_y, 3);
	BitStream.SetBytes(Msg.gyro_z, 3);
	BitStream.SetBytes(Msg.nav_status, 1);
	BitStream.SetBytes(Msg.chksum1, 1);
	BitStream.SetBytes(Msg._lat.c, 8);
	BitStream.SetBytes(Msg._lon.c, 8);
	BitStream.SetBytes(Msg._alt.c, 4);
	BitStream.SetBytes(Msg.vel_north, 3);
	BitStream.SetBytes(Msg.vel_east, 3);
	BitStream.SetBytes(Msg.vel_down, 3);
	BitStream.SetBytes(Msg.heading, 3);
	BitStream.SetBytes(Msg.pitch, 3);
	BitStream.SetBytes(Msg.roll, 3);
	BitStream.SetBytes(Msg.chksum2, 1);
	BitStream.SetBytes(Msg.channel, 1);
	BitStream.SetBytes(Msg.chan.bytes, 8);
	BitStream.SetBytes(Msg.chksum3, 1);
	check(BitStream.GetOffset() == 72 * 8);

	if (bAsync)
	{
		if (!AsyncTask->Publish(BitStream.Buf.GetData(), 72))
		{
			UE_LOG(LogSoda, Warning, TEXT("FOXTSPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo(BitStream.Buf.GetData(), 72, BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FOXTSPublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
		}
	}

	chan_index = (chan_index + 1) % 128;
}

std::uint8_t FOXTSPublisher::Checksum(soda::OxtsPacket const& Packet, std::size_t Start, std::size_t End) noexcept
{
	auto* const RawData = reinterpret_cast< std::uint8_t const* >(&Packet);
	return std::accumulate(RawData + Start, RawData + End, std::uint8_t(0));
}
