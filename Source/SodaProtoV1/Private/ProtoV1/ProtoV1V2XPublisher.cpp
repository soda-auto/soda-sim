// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1V2XPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/V2XSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

bool UProtoV1V2XPublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1V2XPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1V2XPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1V2XPublisher::Advertise() Can't set SetBroadcast mode"));
			Shutdown();
			return false;
		}
	}
	else
	{
		bool Valid;
		Addr->SetIp((const TCHAR*)(*Address), Valid);
		if (!Valid)
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1V2XPublisher::Advertise() IP address isn't valid"));
			Shutdown();
			return false;
		}
	}

	// Create AsyncTask
	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);

	return true;
}

void UProtoV1V2XPublisher::Shutdown()
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

bool UProtoV1V2XPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& Transmitters)
{
	for (auto& Transmitter : Transmitters)
	{
		const FTransform Tranasform = Transmitter->GetV2XTransform();
		const FRotator WorldRot = Tranasform.GetTranslation().Rotation();
		const FVector WorldVel = Transmitter->GetV2XWorldVelocity();
		const FVector AngVel = Transmitter->GetV2XWorldAngVelocity();
		const FVector LocalVel = Tranasform.Rotator().UnrotateVector(WorldVel);
		const FVector Extent = Transmitter->Bound.GetExtent();
		const FVector BoxWorldCenter = Tranasform.GetLocation() + Tranasform.GetRotation().RotateVector(Transmitter->Bound.GetCenter());

		soda::sim::proto_v1::V2XRenderObject Msg ;
		Msg.reserved = 0;
		Msg.object_type = (uint32_t)soda::sim::proto_v1::V2XRenderObjectType::VEHICLE;
		Msg.object_id = Transmitter->ID;
		Msg.timestamp = soda::RawTimestamp<std::chrono::duration<double>>(SodaApp.GetSimulationTimestamp());
		Msg.east = -BoxWorldCenter.X / 100.0;
		Msg.north = BoxWorldCenter.Y / 100.0;
		Msg.up = BoxWorldCenter.Z / 100.0;
		Msg.heading = NormAngRad(-WorldVel.ToOrientationRotator().Yaw / 180.0 * M_PI + M_PI);
		Msg.velocity = WorldVel.Size() / 100.0;
		Msg.status = 0;
		Msg.position_accuracy = 0.001;
		Msg.heading_accuracy = 0.001;
		Msg.velocity_accuracy = 0.001;
		Msg.vertical_velocity = WorldVel.Z / 100.0;
		Msg.length = Extent.X / 100.0 * 2.0; 
		Msg.width = Extent.Y / 100 * 2.0;
		Msg.height = Extent.Z / 100 * 2.0;
		Msg.yaw_angle = NormAngRad(-WorldRot.Yaw / 180 * M_PI + M_PI);
		Msg.pitch_angle = WorldRot.Pitch / 180 * M_PI;
		Msg.roll_angle = -WorldRot.Roll / 180 * M_PI;
		Msg.orientation_accuracy = 0.001;

		Publish(Msg);
	}

	return true;
}

bool UProtoV1V2XPublisher::Publish(const soda::sim::proto_v1::V2XRenderObject& Msg)
{
	if (!Socket)
	{
		return false;
	}

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Msg, sizeof(Msg)))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1V2XPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((const uint8*)&Msg, sizeof(Msg), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1V2XPublisher::Publish(); Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1V2XPublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}