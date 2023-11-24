// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericVehicleStatePublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>

void FGenericVehicleStatePublisher::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericVehicleStatePublisher::Advertise() Can't create socket"));
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
		UE_LOG(LogSoda, Error, TEXT("FGenericVehicleStatePublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return;
	}

	bool Valid;
	Addr->SetIp((const TCHAR*)(*Address), Valid);
	if (!Valid)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericVehicleStatePublisher::Advertise() IP address isn't valid"));
		Shutdown();
		return;
	}
	Addr->SetPort(Port);

	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);
}

void FGenericVehicleStatePublisher::Shutdown()
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

void FGenericVehicleStatePublisher::Publish(const soda::GenericVehicleState& Msg, bool bAsync)
{
	if (!Socket) return;

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Msg, sizeof(Msg)))
		{
			UE_LOG(LogSoda, Warning, TEXT("FGenericVehicleStatePublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((uint8*) & Msg, sizeof(Msg), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FGenericVehicleStatePublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
		}
	}
}