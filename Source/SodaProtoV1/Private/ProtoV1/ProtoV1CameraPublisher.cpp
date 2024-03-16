// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1CameraPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/CameraSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/PixelReader.h"
#include "Soda/SodaApp.h"
#include <errno.h>

bool UProtoV1CameraPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

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
		UE_LOG(LogSoda, Error, TEXT("UProtoV1CameraPublisher::Advertise(), can't create socket, errno: %i"), errno);
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
		UE_LOG(LogSoda, Error, TEXT("UProtoV1CameraPublisher::Advertise() SockPub->bind('%s') failed"), *ZmqAddress);
		delete SockPub;
		SockPub = nullptr;
		bIsOk = false;
		return false;
	}
	bIsOk = true;
	return true;
}

void UProtoV1CameraPublisher::Shutdown()
{
	if (SockPub)
	{
		delete SockPub;
	}

	SockPub = nullptr;
	bIsOk = false;

	RawBuffer.Reset();
}

bool UProtoV1CameraPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& CameraFrame, const TArray<FColor>& BGRA8, uint32 ImageStride)
{
	const uint32 ImgRawBufferSize = CameraFrame.ComputeRawBufferSize();

	RawBuffer.SetNum(ImgRawBufferSize + sizeof(soda::sim::proto_v1::TensorMsgHeader), false);
	soda::sim::proto_v1::TensorMsgHeader& MsgHeader = *((soda::sim::proto_v1::TensorMsgHeader*)RawBuffer.GetData());
	uint8* ImgBufferPtr = &RawBuffer[sizeof(soda::sim::proto_v1::TensorMsgHeader)];

	MsgHeader.tenser.shape.height = CameraFrame.Height;
	MsgHeader.tenser.shape.width = CameraFrame.Width;
	MsgHeader.tenser.shape.depth = 1;
	MsgHeader.timestamp = soda::RawTimestamp<std::chrono::milliseconds>(Header.Timestamp);
	MsgHeader.index = Header.FrameIndex;
	MsgHeader.dtype = uint8(CameraFrame.GetDataType());
	MsgHeader.tenser.shape.channels = CameraFrame.GetChannels();

	CameraFrame.ColorToRawBuffer(BGRA8, ImgBufferPtr, ImageStride);

	return Publish(RawBuffer.GetData(), RawBuffer.Num());
}

bool UProtoV1CameraPublisher::Publish(const void * DataPtr, uint32 Size)
{
	if (SockPub)
	{
		SockPub->send(DataPtr, Size);
		return true;
	}
	return false;
}

bool UProtoV1CameraPublisher::Publish(const soda::sim::proto_v1::TensorMsgHeader& Header, const void* ImageDataPtr, uint32 Size)
{
	if (Size < sizeof(soda::sim::proto_v1::TensorMsgHeader))
	{
		return false;
	}
	RawBuffer.SetNum(Size + sizeof(soda::sim::proto_v1::TensorMsgHeader), false);
	*((soda::sim::proto_v1::TensorMsgHeader*)RawBuffer.GetData()) = Header;
	FMemory::BigBlockMemcpy(&RawBuffer[sizeof(soda::sim::proto_v1::TensorMsgHeader)], ImageDataPtr, Size);
	Publish(RawBuffer.GetData(), RawBuffer.Num());
	return true;
}

FString UProtoV1CameraPublisher::GetRemark() const
{
	return ZmqAddress;
}