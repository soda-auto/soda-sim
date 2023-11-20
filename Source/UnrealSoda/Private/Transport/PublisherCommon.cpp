// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/PublisherCommon.h"
#include "Soda/UnrealSoda.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>

namespace soda
{
	void FUDPAsyncTask::Tick()
	{
		check(!bIsDone);
		int32 BytesSent;
		if (!Socket->SendTo(Buf.GetData(), Buf.Num(), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("FUDPAsyncTask::Tick() Can't send(), error code %i, error = %s"), int32(ErrorCode), ISocketSubsystem::Get()->GetSocketError(ErrorCode));
		}
		bIsDone = true;
	}

	FUDPFrontBackAsyncTask::FUDPFrontBackAsyncTask(TSharedPtr< FSocket >& Socket, TSharedPtr< FInternetAddr >& Addr)
	{
		FrontTask->Socket = Socket;
		FrontTask->Addr = Addr;
		BackTask->Socket = Socket;
		BackTask->Addr = Addr;
	}

	bool FUDPFrontBackAsyncTask::Publish(const void* Buf, int Len)
	{
		bool Ret = true;
		TSharedPtr<FUDPAsyncTask> Task = LockFrontTask();
		if (!Task->IsDone()) Ret = false;
		Task->Buf.SetNum(Len);
		FMemory::BigBlockMemcpy(Task->Buf.GetData(), Buf, Len);
		Task->Buf.SetNum(Len);
		Task->Initialize();
		UnlockFrontTask();
		return Ret;
	}

} //namespace soda