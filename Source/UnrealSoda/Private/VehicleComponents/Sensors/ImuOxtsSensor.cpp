// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/ImuOxtsSensor.h"
#include "Soda/UnrealSoda.h"
#include "SodaSimProto/NComRxC.h"
#include "SodaSimProto/NComRxDefines.h"
#include "Soda/LevelState.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Engine/Canvas.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include <cstddef>

UImuOxtsSensorComponent::UImuOxtsSensorComponent(const FObjectInitializer& ObjectInitializer)
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

bool UImuOxtsSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	Index = 0;
	Publisher.Advertise();

	if (!Publisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void UImuOxtsSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Publisher.Shutdown();
}

void UImuOxtsSensorComponent::GetRemark(FString & Info) const
{
	Info = "Port: " +  FString::FromInt(Publisher.Port);
}

void UImuOxtsSensorComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos) 
{
	Super::DrawDebug(Canvas, YL, YPos);

	if(Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS mode: %d"), OXTS_PositionMode), 16, YPos);
	}
}

void UImuOxtsSensorComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	if (!Publisher.IsAdvertised() || !GetLevelState()) return;

	int64 TimestampMs = soda::RawTimestamp<std::chrono::milliseconds>(Timestamp);
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

	if (OutFile.is_open())
	{
		WriteCsvLog(WorldLoc, WorldRot, WorldVel, LocalAcc, Gyro, Lon, Lat, Alt, Timestamp);
	}

	int64 GPSTimeStemp = TimestampMs + int64(SodaApp.GetSodaUserSettings()->GetGPSTimestempOffset()) * 1000LL;

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
	Msg.chksum1 = FOXTSPublisher::Checksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum1));
	Msg._lon.longitude = Lon / 180.0 * M_PI;
	Msg._lat.latitude = Lat / 180.0 * M_PI;
	Msg._alt.altitude = Alt;
	Msg.vel_east = -std::int32_t(WorldVel.X / 100.0 / VEL2MPS);
	Msg.vel_north = std::int32_t(WorldVel.Y / 100.0 / VEL2MPS);
	Msg.vel_down = -std::int32_t(WorldVel.Z / 100.0 / VEL2MPS);
	Msg.heading = std::int32_t(NormAngRad(WorldRot.Yaw / 180.0 * M_PI - M_PI / 2) / ANG2RAD);
	Msg.pitch = std::int32_t(NormAngRad(WorldRot.Pitch / 180.0 * M_PI) / ANG2RAD);
	Msg.roll = std::int32_t(NormAngRad(WorldRot.Roll / 180.0 * M_PI) / ANG2RAD);
	Msg.chksum2 = FOXTSPublisher::Checksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum2));
	std::memset(&Msg.chan, 0, sizeof(Msg.chan));
	Msg.channel = Publisher.GetChanID();
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
	Msg.chksum3 = FOXTSPublisher::Checksum(Msg, sizeof(Msg.sync), offsetof(soda::OxtsPacket, chksum3));

	Publisher.Publish(Msg);
	Index++;
}