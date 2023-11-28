// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Transport/CameraPublisher.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "SodaSimProto/Camera.hpp"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/Time.h"
#include "GenericCameraPublisher.generated.h"


namespace zmq
{
	class socket_t;
}

class FCameraFrontBackAsyncTask;


/**
* FGenericCameraPublisher
*/
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
	virtual TArray<FColor> & LockBuffer() override;
	virtual void UnlockBuffer(const FCameraFrame& CameraFrame) override;
	virtual bool IsOk() const { return bIsOk; }

public:
	virtual void Publish(const void* DataPtr, uint32 Size);
	virtual void Publish(const soda::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size);

protected:
	zmq::socket_t* SockPub = nullptr;
	TSharedPtr <FCameraFrontBackAsyncTask> AsyncTask;
	bool bIsOk = false;
};

