// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1WheeledVehiclePublisher.h"
#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"

bool UProtoV1WheeledVehiclePublisher::Advertise(UVehicleBaseComponent* Parent)
{
	Shutdown();

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Advertise() IP address isn't valid"));
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

void UProtoV1WheeledVehiclePublisher::Shutdown()
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

bool UProtoV1WheeledVehiclePublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const FWheeledVehicleStateExtra& VehicleStateExtra)
{
	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
	VehicleStateExtra.BodyKinematic.CalcIMU(VehicleStateExtra.RelativeTransform, WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();
	FVector LocVel = WorldRot.UnrotateVector(WorldVel);

	static auto ToSodaVec = [](const FVector& V) { return soda::sim::proto_v1::Vector{ V.X, -V.Y, V.Z }; };
	static auto ToSodaRot = [](const FVector& V) { return soda::sim::proto_v1::Vector{ -V.X, V.Y, -V.Z }; };

	soda::sim::proto_v1::GenericVehicleState Msg{};
	Msg.navigation_state.timestamp = soda::RawTimestamp<std::chrono::nanoseconds>(Header.Timestamp);
	//Msg.navigation_state.longitude = Lon;
	//Msg.navigation_state.latitude = Lat;
	//Msg.navigation_state.altitude = Alt;
	Msg.navigation_state.rotation.roll = -NormAngRad(WorldRot.Roll / 180.0 * M_PI); 
	Msg.navigation_state.rotation.pitch = NormAngRad(WorldRot.Pitch / 180.0 * M_PI);
	Msg.navigation_state.rotation.yaw = -NormAngRad(WorldRot.Yaw / 180.0 * M_PI);
	Msg.navigation_state.position = ToSodaVec(WorldLoc * 0.01);
	Msg.navigation_state.world_velocity = ToSodaVec(WorldVel * 0.01);
	Msg.navigation_state.loc_velocity = ToSodaVec(LocVel * 0.01);
	Msg.navigation_state.angular_velocity = ToSodaRot(Gyro);
	//Msg.navigation_state.acceleration = ToSodaVec(LocalAcc * 0.01);
	//Msg.navigation_state.gyro_biases = ToSodaRot(NoiseParams.Gyro.GetAccuracy());
	//Msg.navigation_state.acc_biases = ToSodaVec(NoiseParams.Acceleration.GetAccuracy());
	Msg.gear = soda::sim::proto_v1::EGear(VehicleStateExtra.Gear);
	Msg.mode = soda::sim::proto_v1::EControlMode(VehicleStateExtra.DriveMode);
	Msg.steer = -(VehicleStateExtra.WheelStates[0].Steer + VehicleStateExtra.WheelStates[1].Steer) / 2;
	for (int i = 0; i < 4; i++)
	{
		Msg.wheels_state[i].ang_vel = VehicleStateExtra.WheelStates[i].AngularVelocity;
		Msg.wheels_state[i].torq = VehicleStateExtra.WheelStates[i].Torq;
		Msg.wheels_state[i].brake_torq = VehicleStateExtra.WheelStates[i].BrakeTorq;
	}
	
	return Publish(Msg);
}

bool UProtoV1WheeledVehiclePublisher::Publish(const soda::sim::proto_v1::GenericVehicleState& Scan)
{
	if (!Socket)
	{
		return false;
	}

	if (bAsync)
	{
		if (!AsyncTask->Publish(&Scan, sizeof(Scan)))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1WheeledVehiclePublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((uint8*) &Scan, sizeof(Scan), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehiclePublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}