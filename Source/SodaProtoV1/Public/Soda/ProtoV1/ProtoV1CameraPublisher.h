// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericPublishers/GenericCameraPublisher.h"
#include "soda/sim/proto-v1/camera.hpp"
#include "Soda/Misc/AsyncTaskManager.h"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/Time.h"
#include "ProtoV1CameraPublisher.generated.h"


namespace zmq
{
	class socket_t;
}


/**
* UProtoV1CameraPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1CameraPublisher : public UGenericCameraPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericPublisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString ZmqAddress = "tcp://*:9999";

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) override;
	virtual void Shutdown() override;
	virtual bool IsOk() const override { return bIsOk; }
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& CameraFrame, const TArray<FColor>& BGRA8, uint32 ImageStride) override;
	//virtual bool Publish(const void* ImgBGRA8Ptr, const FCameraFrame& CameraFrame, uint32 ImageStride) override;
	//virtual TArray<FColor> & LockBuffer() override;
	//virtual void UnlockBuffer(const FCameraFrame& CameraFrame) override;

protected:
	bool Publish(const void* DataPtr, uint32 Size);
	bool Publish(const soda::sim::proto_v1::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size);

protected:
	zmq::socket_t* SockPub = nullptr;
	bool bIsOk = false;
	TArray<uint8> RawBuffer;
};

