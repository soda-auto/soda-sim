// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericUltrasoncPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>


void FGenericUltrasoncPublisher::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericUltrasoncPublisher::Advertise() Can't create socket"));
		Shutdown();
		return;
	}

	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericUltrasoncPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return;
	}

	Addr->SetPort(Port);
	bool Valid;
	Addr->SetIp((const TCHAR*)(*Address), Valid);
	if (!Valid)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericUltrasoncPublisher::Advertise() IP address isn't valid"));
		Shutdown();
		return;
	}

	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);
}

void FGenericUltrasoncPublisher::Shutdown()
{
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

void FGenericUltrasoncPublisher::Publish(const soda::UltrasonicsHub& Scan, bool bAsync)
{
	if (!Socket)
		return;

	std::stringstream sout(std::ios_base::out | std::ios_base::binary);
	int const points_count = soda::write(sout, Scan);
	if (!sout)
		UE_LOG(LogSoda, Fatal, TEXT("FGenericUltrasoncPublisher::Publish() failed to serialize scan"));
	sout.seekp(0, std::ios_base::end);

	if (bAsync)
	{
		if (!AsyncTask->Publish(sout.str().data(), sout.str().length()))
		{
			UE_LOG(LogSoda, Warning, TEXT("FGenericUltrasoncPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)sout.str().data(), sout.str().length(), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FGenericUltrasoncPublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
		}
	}
}