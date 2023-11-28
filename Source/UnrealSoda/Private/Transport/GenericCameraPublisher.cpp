// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericCameraPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include <errno.h>


/***********************************************************************************************
	FCameraAsyncTask
***********************************************************************************************/
class FCameraAsyncTask : public soda::FAsyncTask
{

public:
	virtual ~FCameraAsyncTask() {}
	virtual FString ToString() const override { return "CameraAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override;

public:

	FCameraFrame CameraFrame;
	TArray<FColor> ImgBuffer;

	TWeakObjectPtr<UGenericCameraPublisher> Publisher;

protected:
	bool bIsDone = true;
};

/***********************************************************************************************
	FCameraFrontBackAsyncTask
***********************************************************************************************/
class FCameraFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FCameraAsyncTask>
{
public:
	FCameraFrontBackAsyncTask(UGenericCameraPublisher* Publisher)
	{
		FrontTask->Publisher = Publisher;
		BackTask->Publisher = Publisher;
	}
	virtual ~FCameraFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "CameraFrontBackAsyncTask"; }
};

/***********************************************************************************************
	UGenericCameraPublisher
***********************************************************************************************/
UGenericCameraPublisher::UGenericCameraPublisher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
 {
	 AsyncTask = MakeShareable(new FCameraFrontBackAsyncTask(this));
 }

bool UGenericCameraPublisher::Advertise()
{
	if (!SodaApp.GetZmqContext())
	{
		bIsOk = false;
		return false;
	}

	try
	{
		SockPub = new zmq::socket_t(*SodaApp.GetZmqContext(), ZMQ_PUB);
	} 
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("UGenericCameraPublisher::Advertise(), can't create socket, errno: %i"), errno);
		bIsOk = false;
		return false;
	}

	SockPub->setsockopt(ZMQ_SNDHWM, 1);

	try
	{
		SockPub->bind(std::string(TCHAR_TO_UTF8(*ZmqAddress)));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("UGenericCameraPublisher::Advertise() SockPub->bind('%s') failed"), *ZmqAddress);
		delete SockPub;
		SockPub = nullptr;
		bIsOk = false;
		return false;
	}

	AsyncTask->Start();
	SodaApp.CamTaskManager.AddTask(AsyncTask);
	bIsOk = true;
	return true;
}

void UGenericCameraPublisher::Shutdown()
{
	AsyncTask->Finish();
	SodaApp.CamTaskManager.RemoteTask(AsyncTask);

	if (SockPub)
	{
		delete SockPub;
	}

	SockPub = nullptr;

	bIsOk = false;
}

void UGenericCameraPublisher::Publish(const void* ImgBGRA8Ptr, const FCameraFrame& CameraFrame)
{
	TArray<FColor> & Buf = LockBuffer();
	Buf.SetNum(CameraFrame.ImageStride * CameraFrame.Height, false);
	FMemory::BigBlockMemcpy(Buf.GetData(), ImgBGRA8Ptr, Buf.Num());
	UnlockBuffer(CameraFrame);
}

TArray<FColor> & UGenericCameraPublisher::LockBuffer()
{
	TSharedPtr<FCameraAsyncTask> Task = AsyncTask->LockFrontTask();
	if (!Task->IsDone())
	{
		UE_LOG(LogSoda, Warning, TEXT("UGenericCameraPublisher::LockBuffer(). Skipped one frame") );
	}
	Task->Initialize();
	return Task->ImgBuffer;
}

void UGenericCameraPublisher::UnlockBuffer(const FCameraFrame& CameraFrame)
{
	if (AsyncTask.IsValid())
	{
		AsyncTask->GetLockedFrontTask().CameraFrame = CameraFrame;
		AsyncTask->UnlockFrontTask();
	}
	SodaApp.CamTaskManager.Trigger();
}

void UGenericCameraPublisher::Publish(const void * DataPtr, uint32 Size)
{
	if (!SockPub) return;
	SockPub->send(DataPtr, Size);
}

void UGenericCameraPublisher::Publish(const soda::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size)
{
	if (Size < sizeof(soda::TensorMsgHeader)) return;
	static TArray<uint8> RawBuffer;
	RawBuffer.SetNum(Size + sizeof(soda::TensorMsgHeader), false);
	*((soda::TensorMsgHeader*)RawBuffer.GetData()) = Header;
	FMemory::BigBlockMemcpy(&RawBuffer[sizeof(soda::TensorMsgHeader)], ImageDataPtr, Size);
	Publish(RawBuffer.GetData(), RawBuffer.Num());
}

void FCameraAsyncTask::Tick()
{
	check(!bIsDone);
	check(ImgBuffer.Num() > 0 && uint32(ImgBuffer.Num()) >= CameraFrame.ImageStride * CameraFrame.Height);

	const uint32 ImgRawBufferSize = CameraFrame.ComputeRawBufferSize();

	static TArray<uint8> RawBuffer;
	RawBuffer.SetNum(ImgRawBufferSize + sizeof(soda::TensorMsgHeader), false);
	soda::TensorMsgHeader& Header = *((soda::TensorMsgHeader*)RawBuffer.GetData());
	uint8* ImgBufferPtr = &RawBuffer[sizeof(soda::TensorMsgHeader)];

	Header.tenser.shape.height = CameraFrame.Height;
	Header.tenser.shape.width = CameraFrame.Width;
	Header.tenser.shape.depth = 1;
	Header.timestamp = soda::RawTimestamp<std::chrono::milliseconds>( CameraFrame.Timestamp);
	Header.index = CameraFrame.Index;
	Header.dtype = uint8(CameraFrame.GetDataType());
	Header.tenser.shape.channels = CameraFrame.GetChannels();

	soda::ColorToRawBuffer(ImgBuffer, CameraFrame, ImgBufferPtr);

	Publisher->Publish(RawBuffer.GetData(), RawBuffer.Num());
	bIsDone = true;
}
