// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1NavPublisher.h"
#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include "Soda/LevelState.h"

bool UProtoV1NavPublisher::Advertise(UVehicleBaseComponent* InParent)
{
	Shutdown();

	check(InParent);
	Parent = InParent;

	// Create Socket
	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1NavPublisher::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}

	// Create Addr
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1NavPublisher::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);
	if (bIsBroadcast)
	{
		Addr->SetBroadcastAddress();
		if (!Socket->SetBroadcast(true))
		{
			UE_LOG(LogSoda, Error, TEXT("UProtoV1NavPublisher::Advertise() Can't set SetBroadcast mode"));
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
			UE_LOG(LogSoda, Error, TEXT("UProtoV1NavPublisher::Advertise() IP address isn't valid"));
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

void UProtoV1NavPublisher::Shutdown()
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

bool UProtoV1NavPublisher::Publish(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic, const FImuNoiseParams& Covariance)
{
	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
	VehicleKinematic.CalcIMU(RelativeTransform, WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();
	FVector LocVel = WorldRot.UnrotateVector(WorldVel);

	soda::sim::proto_v1::NavigationState NavStat;

	if (ALevelState* LevelState = Parent->GetLevelState())
	{
		LevelState->GetLLConverter().UE2LLA(WorldLoc, NavStat.longitude, NavStat.latitude, NavStat.altitude);
		WorldRot = LevelState->GetLLConverter().ConvertRotationForward(WorldRot);
		WorldVel = LevelState->GetLLConverter().ConvertDirForward(WorldVel);
	}
	else
	{
		NavStat.longitude = 0;
		NavStat.latitude = 0;
		NavStat.altitude = 0;
	}

	static auto ToSodaVec = [](const FVector& V) { return soda::sim::proto_v1::Vector{ V.X, -V.Y, V.Z }; };
	static auto ToSodaRot = [](const FVector& V) { return soda::sim::proto_v1::Vector{ -V.X, V.Y, -V.Z }; };

	NavStat.position = ToSodaVec(WorldLoc / 100.f );
	NavStat.rotation = { -FMath::DegreesToRadians(WorldRot.Roll), FMath::DegreesToRadians(WorldRot.Pitch), -FMath::DegreesToRadians(WorldRot.Yaw) };
	NavStat.loc_velocity = ToSodaVec(LocVel / 100.0);
	NavStat.world_velocity = ToSodaVec(WorldVel / 100.0);
	NavStat.angular_velocity = ToSodaRot(Gyro);
	NavStat.acceleration = ToSodaVec(LocalAcc / 100.0);

	NavStat.gyro_biases = ToSodaRot(Covariance.Gyro.GetAccuracy());
	NavStat.acc_biases = ToSodaVec(Covariance.Acceleration.GetAccuracy());

	NavStat.timestamp = soda::RawTimestamp<std::chrono::nanoseconds>(Header.Timestamp);

	return Publish(NavStat);
}

bool UProtoV1NavPublisher::Publish(const soda::sim::proto_v1::NavigationState& NavState)
{
	if (!Socket)
	{
		return false;
	}

	if (bAsync)
	{
		if (!AsyncTask->Publish(&NavState, sizeof(NavState)))
		{
			UE_LOG(LogSoda, Warning, TEXT("UProtoV1NavPublisher::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
		return true;
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo((uint8*) &NavState, sizeof(NavState), BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UProtoV1NavPublisher::Publish() Can't send(), error code %i"), int32(ErrorCode));
			return false;
		}
		else
		{
			return true;
		}
	}
}

FString UProtoV1NavPublisher::GetRemark() const
{
	return "udp://" + Address + ":" + FString::FromInt(Port);
}