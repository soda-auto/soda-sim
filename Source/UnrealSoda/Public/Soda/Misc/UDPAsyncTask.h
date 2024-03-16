// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "Templates/SharedPointer.h"
//#include "PublisherCommon.generated.h"

class FSocket;
class FInternetAddr;

/***********************************************************************************************
	FUDPAsyncTask
***********************************************************************************************/

namespace soda
{

class UNREALSODA_API FUDPAsyncTask : public soda::FAsyncTask
{
public:
	virtual ~FUDPAsyncTask() {}
	virtual FString ToString() const override { return "FUDPAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override;

public:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TArray<uint8> Buf;

protected:
	bool bIsDone = true;
};

/***********************************************************************************************
	FUDPFrontBackAsyncTask
***********************************************************************************************/
class UNREALSODA_API FUDPFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FUDPAsyncTask>
{
public:
	FUDPFrontBackAsyncTask(TSharedPtr< FSocket >& Socket, TSharedPtr< FInternetAddr >& Addr);
	
	virtual ~FUDPFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "FUDPFrontBackAsyncTask"; }
	bool Publish(const void* Buf, int Len);
};

} // namespace soda