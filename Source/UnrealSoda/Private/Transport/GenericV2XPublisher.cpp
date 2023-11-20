// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericV2XPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

FGenericV2XPublisher::FGenericV2XPublisher()
{
}

void FGenericV2XPublisher::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericV2XPublisher::Advertise() Can't create socket"));
		Shutdown();
		return;
	}

	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericV2XPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return;
	}

	Addr->SetBroadcastAddress();
	Addr->SetPort(Port);
	Socket->SetBroadcast(true);

	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);
}

void FGenericV2XPublisher::Shutdown()
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


void FGenericV2XPublisher::Publish(const soda::V2VSimulatedObject& Msg, bool bAsync)
{
	if (!Socket) return;

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Msg, sizeof(Msg)))
		{
			UE_LOG(LogSoda, Warning, TEXT("FGenericLidarPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)&Msg, sizeof(Msg), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FGenericV2XPublisher::Publish(); Can't send(), error code %i"), int32(ErrorCode));
		}
	}
}

void FGenericV2XPublisher::Publish(const soda::V2XRenderObject& Msg, bool bAsync)
{
	if (!Socket) return;

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Msg, sizeof(Msg)))
		{
			UE_LOG(LogSoda, Warning, TEXT("FGenericLidarPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)&Msg, sizeof(Msg), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FGenericV2XPublisher::Publish(); Can't send(), error code %i"), int32(ErrorCode));
		}
	}
}