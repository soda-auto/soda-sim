// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericCameraPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Async/ParallelFor.h"
#include <errno.h>

static uint8 GetCVSize(uint8 CVType)
{
	static const uint8 CV_2_SIZE[7] = { 1, 1, 2, 2, 4, 4, 8 };
	return CV_2_SIZE[CVType];
}

UCameraPublisher::UCameraPublisher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

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
		bIsWorking = false;
		return false;
	}

	try
	{
		SockPub = new zmq::socket_t(*SodaApp.GetZmqContext(), ZMQ_PUB);
	} 
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("UGenericCameraPublisher::Advertise(), can't create socket, errno: %i"), errno);
		bIsWorking = false;
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
		bIsWorking = false;
		return false;
	}

	AsyncTask->Start();
	SodaApp.CamTaskManager.AddTask(AsyncTask);
	bIsWorking = true;
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

	bIsWorking = false;
}

void UGenericCameraPublisher::Publish(const void* ImgBGRA8Ptr, const FCameraFrame& CameraFrame)
{
	TArray<FColor> & Buf = LockFrontBuffer(CameraFrame);
	FMemory::BigBlockMemcpy(Buf.GetData(), ImgBGRA8Ptr, Buf.Num());
	UnlockFrontBuffer();

}

TArray<FColor> & UGenericCameraPublisher::LockFrontBuffer(const FCameraFrame& CameraFrame)
{
	TSharedPtr<FCameraAsyncTask> Task = AsyncTask->LockFrontTask();
	if (!Task->IsDone())
	{
		UE_LOG(LogSoda, Warning, TEXT("UGenericCameraPublisher::LockFrontBuffer(). Skipped one frame") );
	}
	Task->ImgBuffer.SetNum(CameraFrame.ImageStride * CameraFrame.Height);
	Task->CameraFrame = CameraFrame;
	Task->Initialize();
	return Task->ImgBuffer;
}

void UGenericCameraPublisher::UnlockFrontBuffer()
{
	if (AsyncTask.IsValid())
	{
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

	uint8 DType = 0;
	uint8 Channels = 0;

	switch (CameraFrame.OutFormat)
	{
	case ECameraSensorShader::SegmBGR8:
	case ECameraSensorShader::ColorBGR8:
	case ECameraSensorShader::HdrRGB8:
	case ECameraSensorShader::CFA:
		DType = CV_8U;
		Channels = 3;
		break;

	case ECameraSensorShader::Segm8:
	case ECameraSensorShader::Depth8:
		DType = CV_8U;
		Channels = 1;
		break;

	case ECameraSensorShader::DepthFloat32:
		DType = CV_32F;
		Channels = 1;
		break;

	case ECameraSensorShader::Depth16:
		DType = CV_16U;
		Channels = 1;
		break;

	default:
		check(0);
	}

	static TArray<uint8> RawBuffer;
	RawBuffer.SetNum(GetCVSize(DType) * Channels * CameraFrame.Height * CameraFrame.Width + sizeof(soda::TensorMsgHeader), false);
	soda::TensorMsgHeader& Header = *((soda::TensorMsgHeader*)RawBuffer.GetData());
	uint8* ImgBufferPtr = &RawBuffer[sizeof(soda::TensorMsgHeader)];

	Header.tenser.shape.height = CameraFrame.Height;
	Header.tenser.shape.width = CameraFrame.Width;
	Header.tenser.shape.depth = 1;
	Header.timestamp = soda::RawTimestamp<std::chrono::milliseconds>( CameraFrame.Timestamp);
	Header.index = CameraFrame.Index;
	Header.dtype = DType;
	Header.tenser.shape.channels = Channels;

	switch (CameraFrame.OutFormat)
	{
		case ECameraSensorShader::Segm8:
			ParallelFor(CameraFrame.Height, [&](int32 i)
				{
					for (uint32 j = 0; j < CameraFrame.Width; j++)
					{
						ImgBufferPtr[i * CameraFrame.Width + j] = ImgBuffer[i * CameraFrame.ImageStride + j].R;  //Only R cnannel
					}
				});
			break;

		case ECameraSensorShader::ColorBGR8:
		case ECameraSensorShader::SegmBGR8:
		case ECameraSensorShader::HdrRGB8:
		case ECameraSensorShader::CFA:
			ParallelFor(CameraFrame.Height, [&](int32 i)
				{
					for (uint32 j = 0; j < CameraFrame.Width; j++)
					{
						uint32 out_ind = (i * CameraFrame.Width + j) * 3;
						uint32 in_ind = (i * CameraFrame.ImageStride + j);
						*(uint32*)&ImgBufferPtr[out_ind + 0] = *(uint32*)&ImgBuffer[in_ind + 0];
					}
				});
			break;
		

		case ECameraSensorShader::DepthFloat32:
			ParallelFor(CameraFrame.Height, [&](int32 i)
				{
					for (uint32 j = 0; j < CameraFrame.Width; j++)
					{
						((float*)ImgBufferPtr)[i * CameraFrame.Width + j] = Rgba2Float(ImgBuffer[CameraFrame.ImageStride * i + j]) * CameraFrame.MaxDepthDistance;
					}
				});
			break;
		

		case ECameraSensorShader::Depth16:
			ParallelFor(CameraFrame.Height, [&](int32 i)
				{
					for (uint32 j = 0; j < CameraFrame.Width; j++)
					{
						((uint16_t*)ImgBufferPtr)[i * CameraFrame.Width + j] = static_cast<uint16_t>(Rgba2Float(ImgBuffer[CameraFrame.ImageStride * i + j]) * 0xFFFF + 0.5f);
					}
				});
			break;

		case ECameraSensorShader::Depth8:
			ParallelFor(CameraFrame.Height, [&](int32 i)
				{
					for (uint32 j = 0; j < CameraFrame.Width; j++)
					{
						ImgBufferPtr[i * CameraFrame.Width + j] = static_cast<uint8_t>(Rgba2Float(ImgBuffer[CameraFrame.ImageStride * i + j]) * 0xFF + 0.5f);
					}
				});
			break;
		
	}
	
	Publisher->Publish(RawBuffer.GetData(), RawBuffer.Num());
	bIsDone = true;
}