// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Implementation/OXTSSensor.h"
#include "Soda/VehicleComponents/Sensors/Implementation/OXTS/NComRxC.h"
#include "Soda/VehicleComponents/Sensors/Implementation/OXTS/NComRxDefines.h"
#include "Soda/UnrealSoda.h"
#include "Soda/LevelState.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <cstddef>
#include <numeric>

static std::uint8_t OXTSChecksum(soda::OxtsPacket const& Packet, std::size_t Start, std::size_t End) noexcept
{
	auto* const RawData = reinterpret_cast< std::uint8_t const* >(&Packet);
	return std::accumulate(RawData + Start, RawData + End, std::uint8_t(0));
}

UOXTSSensorComponent::UOXTSSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("OxTS RT3000");
	GUI.bIsPresentInAddMenu = true;
	GUI.IcanName = TEXT("SodaIcons.GPS");
	
	NoiseParams.Gyro.StdDev = FVector(0.007);
	NoiseParams.Gyro.ConstBias = FVector(0.003);
	NoiseParams.Gyro.GMBiasStdDev = FVector(0.000004);

	NoiseParams.Acceleration.StdDev = FVector(0.03);
	NoiseParams.Acceleration.ConstBias = FVector(0.01);
	NoiseParams.Acceleration.GMBiasStdDev = FVector(0.0001);
}

bool UOXTSSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!Advertise())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise"));
		return false;
	}

	return true;
}

void UOXTSSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Shutdown();
}

void UOXTSSensorComponent::GetRemark(FString & Info) const
{
	Info = "Port: " +  FString::FromInt(Port);
}

bool UOXTSSensorComponent::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket>(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	if (!Socket)
	{
		UE_LOG(LogSoda, Error, TEXT("UOXTSSensorComponent::Advertise() Can't create socket"));
		Shutdown();
		return false;
	}
	if (bIsBroadcast)
	{
		Socket->SetBroadcast(true);
	}

	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	if (!Addr)
	{
		UE_LOG(LogSoda, Error, TEXT("UOXTSSensorComponent::Advertise() Can't create internet addr"));
		Shutdown();
		return false;
	}

	bool Valid;
	Addr->SetIp((const TCHAR*)(*Address), Valid);
	if (!Valid)
	{
		UE_LOG(LogSoda, Error, TEXT("UOXTSSensorComponent::Advertise() IP address isn't valid"));
		Shutdown();
		return false;
	}
	Addr->SetPort(Port);

	AsyncTask = MakeShareable(new soda::FUDPFrontBackAsyncTask(Socket, Addr));
	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);

	return true;
}

void UOXTSSensorComponent::Shutdown()
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

void UOXTSSensorComponent::Publish(const soda::OxtsPacket& Msg, bool bAsync)
{
	if (!Socket) return;

	BitStream.SetOffset(0);
	BitStream.SetBytes(Msg.sync, 1);
	BitStream.SetBytes(Msg.time, 2);
	BitStream.SetBytes(Msg.accel_x, 3);
	BitStream.SetBytes(Msg.accel_y, 3);
	BitStream.SetBytes(Msg.accel_z, 3);
	BitStream.SetBytes(Msg.gyro_x, 3);
	BitStream.SetBytes(Msg.gyro_y, 3);
	BitStream.SetBytes(Msg.gyro_z, 3);
	BitStream.SetBytes(Msg.nav_status, 1);
	BitStream.SetBytes(Msg.chksum1, 1);
	BitStream.SetBytes(Msg._lat.c, 8);
	BitStream.SetBytes(Msg._lon.c, 8);
	BitStream.SetBytes(Msg._alt.c, 4);
	BitStream.SetBytes(Msg.vel_north, 3);
	BitStream.SetBytes(Msg.vel_east, 3);
	BitStream.SetBytes(Msg.vel_down, 3);
	BitStream.SetBytes(Msg.heading, 3);
	BitStream.SetBytes(Msg.pitch, 3);
	BitStream.SetBytes(Msg.roll, 3);
	BitStream.SetBytes(Msg.chksum2, 1);
	BitStream.SetBytes(Msg.channel, 1);
	BitStream.SetBytes(Msg.chan.bytes, 8);
	BitStream.SetBytes(Msg.chksum3, 1);
	check(BitStream.GetOffset() == 72 * 8);

	if (bAsync)
	{
		if (!AsyncTask->Publish(BitStream.Buf.GetData(), 72))
		{
			UE_LOG(LogSoda, Warning, TEXT("UOXTSSensorComponent::PublishAsync(). Skipped one frame"));
		}
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		int32 BytesSent;
		if (!Socket->SendTo(BitStream.Buf.GetData(), 72, BytesSent, *Addr))
		{
			ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
			UE_LOG(LogSoda, Error, TEXT("UOXTSSensorComponent::Publish() Can't send(), error code %i"), int32(ErrorCode));
		}
	}
}

bool UOXTSSensorComponent::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic)
{
	if (!IsAdvertised() || !GetLevelState())
	{
		return false;
	}

	int64 TimestampMs = soda::RawTimestamp<std::chrono::milliseconds>(Header.Timestamp);
	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro = VehicleKinematic.Curr.AngularVelocity;
	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();

	if (bImuNoiseEnabled)
	{
		FVector RotNoize = NoiseParams.Rotation.Step();
		WorldRot += FRotator(RotNoize.Y, RotNoize.Z, RotNoize.X);
		WorldLoc += NoiseParams.Location.Step() * 100;
		LocalAcc += NoiseParams.Acceleration.Step() * 100;
		WorldVel += NoiseParams.Velocity.Step() * 100;
		Gyro += NoiseParams.Gyro.Step();
	}

	double Lon, Lat, Alt;
	GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);
	WorldRot = GetLevelState()->GetLLConverter().ConvertRotationForward(WorldRot);
	WorldVel = GetLevelState()->GetLLConverter().ConvertDirForward(WorldVel);


	int64 GPSTimeStemp = TimestampMs + int64(SodaApp.GetSodaUserSettings()->GetGPSTimestempOffset()) * 1000LL;

	soda::OxtsPacket Msg;
	memset(&Msg, 0, sizeof(Msg));
	Msg.sync = NCOM_SYNC;
	Msg.time = static_cast< std::uint16_t >(GPSTimeStemp % 60000);
	Msg.accel_x = std::int32_t(-LocalAcc.X / 100.0 / ACC2MPS2);
	Msg.accel_y = std::int32_t(-LocalAcc.Y / 100.0 / ACC2MPS2);
	Msg.accel_z = std::int32_t(LocalAcc.Z / 100.0 / ACC2MPS2);
	Msg.gyro_x = -std::int32_t(Gyro.X / RATE2RPS);
	Msg.gyro_y = -std::int32_t(Gyro.Y / RATE2RPS);
	Msg.gyro_z = std::int32_t(Gyro.Z / RATE2RPS);
	Msg.nav_status = NAVIGATION_STATUS_LOCKED;
	Msg.chksum1 = OXTSChecksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum1));
	Msg._lon.longitude = Lon / 180.0 * M_PI;
	Msg._lat.latitude = Lat / 180.0 * M_PI;
	Msg._alt.altitude = Alt;
	Msg.vel_east = -std::int32_t(WorldVel.X / 100.0 / VEL2MPS);
	Msg.vel_north = std::int32_t(WorldVel.Y / 100.0 / VEL2MPS);
	Msg.vel_down = -std::int32_t(WorldVel.Z / 100.0 / VEL2MPS);
	Msg.heading = std::int32_t(NormAngRad(WorldRot.Yaw / 180.0 * M_PI - M_PI / 2) / ANG2RAD);
	Msg.pitch = std::int32_t(NormAngRad(WorldRot.Pitch / 180.0 * M_PI) / ANG2RAD);
	Msg.roll = std::int32_t(NormAngRad(WorldRot.Roll / 180.0 * M_PI) / ANG2RAD);
	Msg.chksum2 = OXTSChecksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum2));
	std::memset(&Msg.chan, 0, sizeof(Msg.chan));
	Msg.channel = ChanIndex;
	switch (Msg.channel)
	{
	case 3:
	{
		FVector Accuracy = NoiseParams.Location.GetAccuracy();
		Msg.chan.chan3.acc_position_north = Accuracy.X / (1e-3);
		Msg.chan.chan3.acc_position_east = Accuracy.Y / (1e-3);
		Msg.chan.chan3.acc_position_down = Accuracy.Z / (1e-3);
		break;
	}
	case 4:
	{
		FVector Accuracy = NoiseParams.Velocity.GetAccuracy();
		Msg.chan.chan4.acc_velocity_north = Accuracy.X / (1e-3);
		Msg.chan.chan4.acc_velocity_east = Accuracy.Y / (1e-3);
		Msg.chan.chan4.acc_velocity_down = Accuracy.Z / (1e-3);
		break;
	}
	case 5:
	{
		FVector Accuracy = NoiseParams.Rotation.GetAccuracy();
		Msg.chan.chan5.acc_heading = Accuracy.X / (1e-5);
		Msg.chan.chan5.acc_pitch = Accuracy.Y / (1e-5);
		Msg.chan.chan5.acc_roll = Accuracy.Z / (1e-5);
		break;
	}
	case 20:
		Msg.chan.chan20.base_station_age = DifferentialCorrectionsAge;
		break;
	case 27:
		Msg.chan.chan27.heading_quality =  OXTS_HeadingQuality;
		break;
	case 0:
	default:
		Msg.channel = 0;
		Msg.chan.chan0.num_sats = SatellitesNumber;
		Msg.chan.chan0.position_mode = OXTS_PositionMode;
		Msg.chan.chan0.velocity_mode = OXTS_VelocityMode;
		Msg.chan.chan0.orientation_mode = OXTS_OrientationMode;
		Msg.chan.chan0.gps_minutes = GPSTimeStemp / 60000;
		break;
	}
	Msg.chksum3 = OXTSChecksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum3));

	ChanIndex = (ChanIndex + 1) % 128;

	Publish(Msg);
	return true;
}

void UOXTSSensorComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if(Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS mode: %d"), OXTS_PositionMode), 16, YPos);
	}
}

