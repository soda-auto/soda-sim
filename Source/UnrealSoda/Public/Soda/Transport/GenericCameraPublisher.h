// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Misc/AsyncTaskManager.h"
#include "SodaSimProto/Camera.hpp"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/Time.h"
#include "GenericCameraPublisher.generated.h"

#define CV_8U 0
#define CV_8S 1
#define CV_16U 2
#define CV_16S 3
#define CV_32S 4
#define CV_32F 5
#define CV_64F 6

namespace zmq
{
	class socket_t;
}

class FCameraFrontBackAsyncTask;

UENUM(BlueprintType)
enum class ECameraSensorShader : uint8
{
	ColorBGR8 = 0,
	Depth8 = 2,
	DepthFloat32 = 3,
	Depth16 = 4,
	SegmBGR8 = 5,
	Segm8 = 6,
	HdrRGB8 = 7,
	CFA = 8, // Color Filter Array. See https://en.wikipedia.org/wiki/Color_filter_array
};

struct FCameraFrame
{
	uint32 Height = 0;
	uint32 Width = 0; 
	float MaxDepthDistance = 0;
	int64 Index = 0;
	TTimestamp Timestamp ;
	uint32 ImageStride = 0; // Input image width in pixels
	ECameraSensorShader OutFormat = ECameraSensorShader::ColorBGR8;
};

/***********************************************************************************************
	UCameraPublisher
***********************************************************************************************/
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UCameraPublisher : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	virtual bool Advertise() { return false; }
	virtual void Shutdown() {}
	virtual void Publish(const void* ImgBGRA8Ptr, const FCameraFrame& CameraFrame) {}
	virtual void Publish(const void* DataPtr, uint32 Size) {}
	virtual void Publish(const soda::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size) {}
	virtual bool IsInitializing() const { return false; }
	virtual bool IsWorking() const { return false; }
	virtual TArray<FColor>& LockFrontBuffer(const FCameraFrame& CameraFrame) { static TArray<FColor> Dummy; return Dummy; }
	virtual void UnlockFrontBuffer() {}
	virtual ~UCameraPublisher() {}

};


/***********************************************************************************************
    FGenericCameraPublisher
***********************************************************************************************/
UCLASS(ClassGroup = Soda, BlueprintType)
class UNREALSODA_API UGenericCameraPublisher : public UCameraPublisher
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericPublisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString ZmqAddress = "tcp://*:9999";

	virtual bool Advertise() override;
	virtual void Shutdown() override;
	virtual void Publish(const void* ImgBGRA8Ptr, const FCameraFrame & CameraFrame) override;
	virtual void Publish(const void * DataPtr, uint32 Size) override;
	virtual void Publish(const soda::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size) override;
	virtual TArray<FColor> & LockFrontBuffer(const FCameraFrame& CameraFrame) override;
	virtual void UnlockFrontBuffer() override;
	virtual bool IsWorking() const { return bIsWorking; }

protected:
	zmq::socket_t* SockPub = nullptr;
	TSharedPtr <FCameraFrontBackAsyncTask> AsyncTask;
	bool bIsWorking = false;
};

