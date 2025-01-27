// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovements/NCOMWheeledVehicleMovement.h"
#include "Soda/VehicleComponents/Sensors/Implementation/OXTS/NComRxC.h"
#include "Soda/VehicleComponents/Sensors/Implementation/OXTS/NComRxDefines.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/CollisionProfile.h"
#include "Soda/SodaStatics.h"
#include "Soda/LevelState.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"


UNCOMWheeledVehicleMovement::UNCOMWheeledVehicleMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("NCOM Vehicle");

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bWantsInitializeComponent = true;
}

void UNCOMWheeledVehicleMovement::InitializeComponent()
{
	Super::InitializeComponent();

	LevelState = ALevelState::Get();
}

void UNCOMWheeledVehicleMovement::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void UNCOMWheeledVehicleMovement::BeginPlay()
{
	Super::BeginPlay();
	check(LevelState);
}

void UNCOMWheeledVehicleMovement::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UDPReceiver)
	{
		UDPReceiver->Stop();
		delete UDPReceiver;
	}

	if (ListenSocket)
	{
		delete ListenSocket;
	}
}

void UNCOMWheeledVehicleMovement::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	if (GetWheeledVehicle()->IsXWDVehicle(4))
	{
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->Radius = FrontWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->Radius = FrontWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->Radius = RearWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->Radius = RearWheelRadius;
	}


	OnSetActiveMovement();
}

bool UNCOMWheeledVehicleMovement::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	InitTransform = UpdatedPrimitive->GetComponentTransform();

	USkinnedMeshComponent* Mesh = Cast< USkinnedMeshComponent >(UpdatedPrimitive);
	if (!Mesh)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Forgot UNCOMWheeledVehicleMovement"));
		UE_LOG(LogSoda, Error, TEXT("Forgot to set Mesh to UNCOMWheeledVehicleMovement?"));
		return false;
	}

	Mesh->SetEnableGravity(false);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	VehicleSimData.VehicleKinematic.Curr.GlobalPose = UpdatedPrimitive->GetComponentTransform();


	FIPv4Endpoint Endpoint(FIPv4Address(0, 0, 0, 0), NCOMPort);
	ListenSocket = FUdpSocketBuilder(TEXT("NCOMWheeledVehicleMovement"))
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(0xFFFF);

	if (ListenSocket)
	{
		if (!ListenSocket->SetReuseAddr(true))
		{
			UE_LOG(LogSoda, Warning, TEXT("UNCOMWheeledVehicleMovement::BeginPlay(). Can't set SetReuseAddr(true)"));
		}
		FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
		UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("NCOMWheeledVehicleMovement"));
		UDPReceiver->OnDataReceived().BindLambda([this](const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
			{
				Recv(ArrayReaderPtr, EndPt);
			});
		UDPReceiver->Start();
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::BeginPlay(). Can't create socket"));
		return false;
	}

	return true;
}

void UNCOMWheeledVehicleMovement::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

}

void UNCOMWheeledVehicleMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	/*
	if(bLogPhysStemp)
	{
		UE_LOG(LogSoda, Warning, TEXT("2WDVehicle, FixSimSetp: %i, FixSimTimeStep: %s , Pos: %s"), RenderFrameSimulatedStep, *USodaStatics::Int64ToString(FixedSimTimeStep), *NewLoc.ToString());
	}
	*/

	static const std::chrono::nanoseconds Timeout(100000000ll);

	bIsConnected = (soda::Now() - LastPacketTimestamp) < Timeout;

	if (bIsConnected && bIsOXTSDataValid)
	{

		Mutex.lock();
		FPhysBodyKinematic SavedVehicleKinematic = VehicleSimData.VehicleKinematic;
		VehicleSimData.RenderStep = VehicleSimData.SimulatedStep;
		VehicleSimData.RenderTimestamp = VehicleSimData.SimulatedTimestamp;
		Mutex.unlock();

		if (UpdatedPrimitive)
		{
			UpdatedPrimitive->SetWorldLocationAndRotationNoPhysics(
				SavedVehicleKinematic.Curr.GlobalPose.GetTranslation(),
				SavedVehicleKinematic.Curr.GlobalPose.Rotator());
		}
	}
}

void UNCOMWheeledVehicleMovement::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	
	if (Common.bDrawDebugCanvas && GetHealth() != EVehicleComponentHealth::Disabled)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		if (bIsConnected)
		{
			if (!bIsOXTSDataValid)
			{
				Canvas->SetDrawColor(FColor::Red);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Connected: true")), 16, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("NCOM data not valid")), 16, YPos);
				Canvas->SetDrawColor(FColor::White);
			}
			else
			{
				Canvas->SetDrawColor(FColor::White);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Connected: true")), 16, YPos);
			}
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_SatellitesNumber: %i"), OXTS_SatellitesNumber), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_PositionMode: %i"), OXTS_PositionMode), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_VelocityMode: %i"), OXTS_VelocityMode), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_OrientationMode: %i"), OXTS_OrientationMode), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_GPSMinutes: %i"), OXTS_GPSMinutes), 16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS_Miliseconds: %i"), OXTS_Miliseconds), 16, YPos);
		}
		else
		{
			Canvas->SetDrawColor(FColor::Red);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Connected: false")), 16, YPos);
		}
	}
}

bool UNCOMWheeledVehicleMovement::SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation)
{
	return false;
}

void UNCOMWheeledVehicleMovement::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	bool bIsOXTSDataValidTmp = true;
	if (ArrayReaderPtr->Num() != 72)
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::Recv(); Protocol error, recve %d bytes, was expected 72 bytes"), ArrayReaderPtr->Num());
		bIsOXTSDataValidTmp = false;
		return;
	}

	BitStream.Buf.SetNum(72);
	::memcpy(BitStream.Buf.GetData(), ArrayReaderPtr->GetData(), 72);

	BitStream.SetOffset(0);
	OXTSPacket.sync       = BitStream.GetBytes<uint8_t>(1);
	OXTSPacket.time       = BitStream.GetBytes<uint16_t>(2);
	OXTSPacket.accel_x    = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.accel_y    = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.accel_z    = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.gyro_x     = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.gyro_y     = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.gyro_z     = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.nav_status = BitStream.GetBytes<uint8_t>(1);
	OXTSPacket.chksum1    = BitStream.GetBytes<uint8_t>(1);
	BitStream.GetBytes(&OXTSPacket._lat.c, 8);
	BitStream.GetBytes(&OXTSPacket._lon.c, 8);
	BitStream.GetBytes(&OXTSPacket._alt.c, 4);
	OXTSPacket.vel_north  = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.vel_east   = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.vel_down   = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.heading    = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.pitch      = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.roll       = BitStream.GetBytes<int32_t>(3);
	OXTSPacket.chksum2    = BitStream.GetBytes<uint8_t>(1);
	OXTSPacket.channel    = BitStream.GetBytes<uint8_t>(1);
	BitStream.GetBytes(&OXTSPacket.chan.bytes, 8);
	OXTSPacket.chksum3    = BitStream.GetBytes<uint8_t>(1);
	check(BitStream.GetOffset() == 72 * 8);

	auto Timestemp = soda::Now();
	double DeltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(Timestemp - LastPacketTimestamp).count();
	LastPacketTimestamp = Timestemp;
	
	FVector WorldPos;
	FRotator WorldRot;
	FVector LocalAcc;
	FVector AngularVelocity;
	FVector WorldVel;


	OXTS_Miliseconds = OXTSPacket.time;
	//OXTSPacket.sync == NCOM_SYNC;
	//OXTSPacket.nav_status == NAVIGATION_STATUS_LOCKED;
	//OXTSPacket.chksum1 == FOXTSPublisher::Checksum(OXTSPacket, sizeof(OXTSPacket.sync), offsetof(soda::OxtsPacket, chksum1));
	//OXTSPacket.chksum2 == FOXTSPublisher::Checksum(OXTSPacket, sizeof(OXTSPacket.sync), offsetof(soda::OxtsPacket, chksum2));
	//OXTSPacket.chksum3 == FOXTSPublisher::Checksum(OXTSPacket, sizeof(OXTSPacket.sync), offsetof(soda::OxtsPacket, chksum3));

	LocalAcc.X = -float(OXTSPacket.accel_x) * 100.0 * ACC2MPS2;
	LocalAcc.Y = -float(OXTSPacket.accel_y) * 100.0 * ACC2MPS2;
	LocalAcc.Z =  float(OXTSPacket.accel_z) * 100.0 * ACC2MPS2;
	AngularVelocity.X = -float(OXTSPacket.gyro_x) * RATE2RPS;
	AngularVelocity.Y = -float(OXTSPacket.gyro_y) * RATE2RPS;
	AngularVelocity.Z =  float(OXTSPacket.gyro_z) * RATE2RPS;
	double Lon = OXTSPacket._lon.longitude / M_PI * 180;
	double Lat = OXTSPacket._lat.latitude / M_PI * 180;
	double Alt = OXTSPacket._alt.altitude;
	WorldVel.X = -float(OXTSPacket.vel_east) * 100.0 * VEL2MPS;
	WorldVel.Y =  float(OXTSPacket.vel_north) * 100.0 * VEL2MPS;
	WorldVel.Z = -float(OXTSPacket.vel_down) * 100.0 * VEL2MPS;
	WorldRot.Yaw   = float(OXTSPacket.heading) * ANG2RAD / M_PI * 180.0  + 90;
	WorldRot.Pitch = float(OXTSPacket.pitch) * ANG2RAD / M_PI * 180.0;
	WorldRot.Roll  = float(OXTSPacket.roll) * ANG2RAD / M_PI * 180.0;
	LevelState->GetLLConverter().LLA2UE(WorldPos, Lon, Lat, Alt);
	WorldRot = LevelState->GetLLConverter().ConvertRotationBackward(WorldRot);
	WorldVel = LevelState->GetLLConverter().ConvertDirBackward(WorldVel);

	if (WorldPos.ContainsNaN() || (WorldPos.GetAbsMax() > 2000000)) // 20km 
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::Recv(), Got invalid package; Lon: %f; Lat: %f; Alt: %f; WorldPos: %s"), Lon, Lat, Alt, *WorldPos.ToString());
		WorldPos = FVector(0, 0, 0);
		bIsOXTSDataValidTmp = false;
	}

	if (WorldRot.ContainsNaN()) 
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::Recv(), Got invalid package; Rot: %s"), *WorldRot.ToString());
		WorldRot = FRotator(0, 0, 0);
		bIsOXTSDataValidTmp = false;
	}

	if (WorldPos.ContainsNaN() || (WorldPos.GetAbsMax() > 2000000)) // 20km 
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::Recv(), Got invalid package; Lon: %f; Lat: %f; Alt: %f; WorldPos: %s"), Lon, Lat, Alt, *WorldPos.ToString());
		WorldPos = FVector(0, 0, 0);
		bIsOXTSDataValidTmp = false;
	}

	if (WorldRot.ContainsNaN()) 
	{
		UE_LOG(LogSoda, Error, TEXT("UNCOMWheeledVehicleMovement::Recv(), Got invalid package; Rot: %s"), *WorldRot.ToString());
		WorldRot = FRotator(0, 0, 0);
		bIsOXTSDataValidTmp = false;
	}

	switch (OXTSPacket.channel)
	{
	case 0:
		OXTS_SatellitesNumber = OXTSPacket.chan.chan0.num_sats;
		OXTS_PositionMode = OXTSPacket.chan.chan0.position_mode;
		OXTS_VelocityMode = OXTSPacket.chan.chan0.velocity_mode;
		OXTS_OrientationMode = OXTSPacket.chan.chan0.orientation_mode;
		OXTS_GPSMinutes = OXTSPacket.chan.chan0.gps_minutes;
		break;
	}

	PrePhysicSimulation(LastDeltatime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

	Mutex.lock();

	bIsOXTSDataValid = bIsOXTSDataValidTmp;

	for (auto& Wheel : GetWheeledVehicle()->GetWheelsSorted())
	{
		Wheel->AngularVelocity = 0;
		Wheel->AngularVelocity = 0;
		Wheel->AngularVelocity = 0;
		Wheel->AngularVelocity = 0;

		Wheel->Steer = 0;
		Wheel->Steer = 0;

		Wheel->Slip = FVector2D::ZeroVector;
		Wheel->Slip = FVector2D::ZeroVector;
		Wheel->Slip = FVector2D::ZeroVector;
		Wheel->Slip = FVector2D::ZeroVector;
	}

	VehicleSimData.VehicleKinematic.Push(DeltaTime);
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(NCOMRotation, NCOMOffest).Inverse() *  FTransform(WorldRot, WorldPos);
	VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = WorldVel + (AngularVelocity ^ (CenterOfMass - NCOMOffest)); //TODO: Need to check it
	VehicleSimData.VehicleKinematic.Curr.AngularVelocity = AngularVelocity;
	VehicleSimData.VehicleKinematic.Curr.CenterOfMassLocal = CenterOfMass;

	++VehicleSimData.SimulatedStep;
	VehicleSimData.SimulatedTimestamp = soda::Now();

	Mutex.unlock();


	PostPhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
	PostPhysicSimulationDeferred(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

	LastDeltatime = DeltaTime;

	SyncDataset();
}

void UNCOMWheeledVehicleMovement::ResetPosition()
{
	FVector OrignShift = LevelState->GetLLConverter().OrignShift;
	float OrignDYaw = LevelState->GetLLConverter().OrignDYaw;

	FVector RawPos = (UpdatedPrimitive->GetComponentTransform().GetTranslation() - OrignShift).RotateAngleAxis(-OrignDYaw, FVector(0, 0, 1));
	FVector NewPos = RawPos.RotateAngleAxis(OrignDYaw, FVector(0, 0, 1)) + OrignShift;

	LevelState->SetGeoReference(
		LevelState->GetLLConverter().OrignLat,
		LevelState->GetLLConverter().OrignLon,
		LevelState->GetLLConverter().OrignAltitude,
		OrignShift + (InitTransform.GetTranslation() - NewPos),
		OrignDYaw + (InitTransform.Rotator().Yaw - UpdatedPrimitive->GetComponentTransform().Rotator().Yaw)
	);
}