// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/GpsDSensor.h"
#include "Soda/UnrealSoda.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include <ctime>
#include "SodaSimProto/NComRxDefines.h"
#include "Soda/LevelState.h"
#include "HAL/UnrealMemory.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"

UGpsDSensorComponent::UGpsDSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)

{
	GUI.ComponentNameOverride = TEXT("GPSD");
	GUI.bIsPresentInAddMenu = true;
	GUI.IcanName = TEXT("SodaIcons.GPS");

	NoiseParams.Gyro.StdDev = FVector(0.00087);
	NoiseParams.Gyro.ConstBias = FVector(0.001);
	NoiseParams.Gyro.GMBiasStdDev = FVector(0.00001);

	NoiseParams.Acceleration.StdDev = FVector(0.02);
	NoiseParams.Acceleration.ConstBias = FVector(0.03);
	NoiseParams.Acceleration.GMBiasStdDev = FVector(0.0001);
}

bool UGpsDSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	Shutdown();

	FIPv4Address V4Addr;
	if (!FIPv4Address::Parse(Address, V4Addr))
	{
		UE_LOG(LogSoda, Error, TEXT("FInnovizProPublisher::OnActivateVehicleComponent() : Tcp address %s parse error"), *Address);
	}
	FIPv4Endpoint Endpoint(V4Addr, TcpPort);
	TcpServerSocket = FTcpSocketBuilder("GpsDSensor").AsNonBlocking().AsReusable().BoundToEndpoint(Endpoint).Listening(8).Build();
	if (!TcpServerSocket)
	{
		UE_LOG(LogSoda, Error, TEXT("UGpsDSensorComponent::OnActivateVehicleComponent(): Can't create TCP server socket"));
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't create server socket"));
		return false;
	}

	TcpListener = new FTcpListener(*TcpServerSocket);
	TcpListener->OnConnectionAccepted().BindUObject(this, &UGpsDSensorComponent::OnConnected);
	if (!TcpListener->Init())
	{
		UE_LOG(LogSoda, Error, TEXT("Can not start listening on port %d"), TcpPort);
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't start server listen"));
		return false;
	}

	return true;
}

void UGpsDSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	Shutdown();
}

void UGpsDSensorComponent::Shutdown()
{
	Mutex.lock();
	for (FGpsDConnection& Connection : Connections)
	{
		Connection.ConnectionSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connection.ConnectionSocket);
	}
	Connections.Empty();
	Mutex.unlock();

	if (TcpListener)
	{
		TcpListener->Stop();
		FPlatformProcess::Sleep(0.f);
		delete TcpListener;
		TcpListener = 0;
	}
	if (TcpServerSocket)
	{
		TcpServerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(TcpServerSocket);
	}
	TcpServerSocket = 0;
}

void UGpsDSensorComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos) 
{
	Super::DrawDebug(Canvas, YL, YPos);
	if(Common.bDrawDebugCanvas)
	{
	}
}

void UGpsDSensorComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& TimestampIn)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, TimestampIn);

	if (!TcpListener || !TcpListener->IsActive() || !GetLevelState()) return;

	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
	WorldRot = WorldPose.Rotator();
	WorldLoc = WorldPose.GetTranslation();
	Timestamp = TimestampIn;

	if (bImuNoiseEnabled)
	{
		FVector RotNoize = NoiseParams.Rotation.Step();
		WorldRot += FRotator(RotNoize.Y, RotNoize.Z, RotNoize.X);
		WorldLoc += NoiseParams.Location.Step() * 100;
		LocalAcc += NoiseParams.Acceleration.Step() * 100;
		WorldVel += NoiseParams.Velocity.Step() * 100;
		Gyro += NoiseParams.Gyro.Step();
	}
	WorldRot = GetLevelState()->GetLLConverter().ConvertRotationForward(WorldRot);
	WorldVel = GetLevelState()->GetLLConverter().ConvertDirForward(WorldVel);
	CurVehicleKinematic = VehicleKinematic;
	GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);

	if (OutFile.is_open())
	{
		WriteCsvLog(WorldLoc, WorldRot, WorldVel, LocalAcc, Gyro, Lon, Lat, Alt, Timestamp);
	}

	Mutex.lock();
	for (size_t i = 0; i < Connections.Num();/*Nothing here*/)
	{
		if (Connections[i].ConnectionSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected || Connections[i].SocketDead)
		{
			if (bBaseDebugOutput)
				UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::UpdateImu(): Closing connection for connection[%d]"), i);

			Connections[i].ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connections[i].ConnectionSocket);
			Connections.RemoveAt(i);
		}
		else
		{
			i++;
		}
	}

	std::chrono::milliseconds WatcherModePeriod_(int64(WatcherModePeriod * 1000 + 0.5));

	auto SendDeltaTime = TimestampIn - PrevSendTimeMark;
	if (SendDeltaTime >= WatcherModePeriod_)
	{
		if (SendDeltaTime < WatcherModePeriod_)
			PrevSendTimeMark += WatcherModePeriod_;
		else
			PrevSendTimeMark = TimestampIn;

		for (size_t i = 0; i < Connections.Num(); ++i)
		{
			if (Connections[i].WatcherMode && Connections[i].WatcherJson)
			{
				if (bLogTcpDebugOutput)
					UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::UpdateImu(): Send data to connection[%d] by Watcher mode"), i);

				if(bWatcherModeSKY) SendSKY(Connections[i]);
				if(bWatcherModeTPV) SendTPV(Connections[i]);
				if(bWatcherModeATT) SendATT_IMU(Connections[i], true);
			}
		}
	}

	if (bWatcherModeIMU)
	{
		for (size_t i = 0; i < Connections.Num(); ++i)
		{
			if (Connections[i].WatcherMode && Connections[i].WatcherJson)
			{
				SendATT_IMU(Connections[i], false);
			}
		}
	}

	for (int i = 0; i < Connections.Num(); ++i)
	{
		TArray< uint8 > ReceivedData;

		uint32 Size;
		int32 Read = 0;
		if (Connections[i].ConnectionSocket->HasPendingData(Size))
		{
			ReceivedData.Init(0, FMath::Min(Size, 65507u));

			Connections[i].ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
			if (bLogTcpDebugOutput)
			{
				UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::UpdateImu(): Data Read from connection[%d]: %d"), i, Read);
			}
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (ReceivedData.Num() <= 0)
		{
			//No Data Received
			continue;
		}

		for (uint8 &Elem : ReceivedData)
		{
			char CharElem = (char)Elem;
			if (CharElem == '?')
			{
				Connections[i].RequestNameBuffer.Empty();
				Connections[i].RequestJsonBuffer.Empty();
				Connections[i].ParseAsJson = false;
			}
			else if (CharElem == '=')
			{
				Connections[i].ParseAsJson = true;
			}
			else if (CharElem == ';' || CharElem == '\n')
			{

				TSharedPtr<FJsonObject> RequestJsonObject = MakeShareable(new FJsonObject);
				TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(Connections[i].RequestJsonBuffer);
				FJsonSerializer::Deserialize(Reader, RequestJsonObject);
				
				if (Connections[i].RequestNameBuffer.Contains(TEXT("WATCH"), ESearchCase::IgnoreCase))
				{
					if (RequestJsonObject.Get())
					{
						const TSharedPtr<FJsonValue> ValEnableField = RequestJsonObject.Get()->TryGetField(TEXT("enable"));
						if (ValEnableField.IsValid())
						{
							if (ValEnableField->Type == EJson::Boolean)
							{
								Connections[i].WatcherMode = ValEnableField->AsBool();
							}
						}
						const TSharedPtr<FJsonValue> ValJsonField = RequestJsonObject.Get()->TryGetField(TEXT("json"));
						if (ValJsonField.IsValid())
						{
							if (ValJsonField->Type == EJson::Boolean)
							{
								Connections[i].WatcherJson = ValJsonField->AsBool();
							}
						}
						const TSharedPtr<FJsonValue> ValDeviceField = RequestJsonObject.Get()->TryGetField(TEXT("device"));
						if (ValDeviceField.IsValid())
						{
							/*
							if (ValDeviceField->Type == EJson::String)
							{
								Connections[i].DeviceName = ValDeviceField->AsString();
							}
							*/
						}
					}
					if (bBaseDebugOutput)
						UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::UpdateImu(): Received Watch to connection[%d]: enable=%d, json=%d, device=%s"), i, Connections[i].WatcherMode, Connections[i].WatcherJson, *DeviceName);
					SendWatch(Connections[i]);
					SendDevices(Connections[i]);

				}

				if (bBaseDebugOutput)
					UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::UpdateImu(): RequestBuffer of connection[%d] emptied on: %s=%s"), i, *Connections[i].RequestNameBuffer, *Connections[i].RequestJsonBuffer);
				Connections[i].RequestNameBuffer.Empty();
				Connections[i].RequestJsonBuffer.Empty();
			}
			else
			{
				if (!Connections[i].ParseAsJson)
				{
					Connections[i].RequestNameBuffer.AppendChar((TCHAR)CharElem);
				}
				else
				{
					Connections[i].RequestJsonBuffer.AppendChar((TCHAR)CharElem);
				}
			}
		}
	}
	Mutex.unlock();
}

bool UGpsDSensorComponent::OnConnected(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	Mutex.lock();
	if (bBaseDebugOutput)
		UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::OnConnected [%d]"), Connections.Num());
	Connections.Add(FGpsDConnection(ClientSocket, ClientEndpoint.ToInternetAddr()));
	SendVersion(Connections.Last());
	Mutex.unlock();
	return true;
}

void UGpsDSensorComponent::SendTPV(FGpsDConnection& Connection)
{
	TSharedPtr<FJsonObject> TPVObject = MakeShareable(new FJsonObject);
	
	TPVObject->SetStringField("class", "TPV");
	TPVObject->SetStringField("device", DeviceName);
	TPVObject->SetNumberField("status", 2);
	char time[100];
	SPrintfFormatedTime(soda::RawTimestamp<std::chrono::milliseconds>(Timestamp), time);
	TPVObject->SetStringField("time", time);
	TPVObject->SetNumberField("leapseconds", 18);
	TPVObject->SetNumberField("mode", 3);
	TPVObject->SetNumberField("lat", Lat);
	TPVObject->SetNumberField("lon", Lon);
	TPVObject->SetNumberField("alt", Alt);
	float Track = WorldVel.Rotation().Yaw - 90.f;
	Track = Track < 0.f ? Track + 360.f : Track > 360.f ? Track - 360.f : Track;
	TPVObject->SetNumberField("track", Track);
	TPVObject->SetNumberField("speed", FVector2D(WorldVel).Size() / 100.f);
	TPVObject->SetNumberField("climb", WorldVel.Z / 100.f);
	
	TPVObject->SetNumberField(TEXT("ept"), 0.005);
	TPVObject->SetNumberField(TEXT("epx") , 13.725);
	TPVObject->SetNumberField(TEXT("epy") , 13.949);
	TPVObject->SetNumberField(TEXT("epv") , 5.808);
	TPVObject->SetNumberField(TEXT("epd") , 71.0492);
	TPVObject->SetNumberField(TEXT("eps") , 0.03);
	TPVObject->SetNumberField(TEXT("epc") , 298.04);
	TPVObject->SetNumberField(TEXT("ecefx") , 3926933.52);
	TPVObject->SetNumberField(TEXT("ecefy") , -91416.57);
	TPVObject->SetNumberField(TEXT("ecefz") , 5008412.79);
	TPVObject->SetNumberField(TEXT("ecefvx") , -0.07);
	TPVObject->SetNumberField(TEXT("ecefvy") , -0.01);
	TPVObject->SetNumberField(TEXT("ecefvz") , -0.04);
	TPVObject->SetNumberField(TEXT("ecefpAcc") , 7.76);
	TPVObject->SetNumberField(TEXT("ecefvAcc") , 0.21);
	TPVObject->SetNumberField(TEXT("eph") , 5.150);
//	TPVObject->SetNumberField(TEXT("sep") , 1899.810);
	
	
	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(TPVObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}
}

void UGpsDSensorComponent::SendATT_IMU(FGpsDConnection& Connection, bool bIsATT)
{
	int64 TimestampMs = soda::RawTimestamp<std::chrono::milliseconds>(Timestamp);

	TSharedPtr<FJsonObject> TPVObject = MakeShareable(new FJsonObject);
	TPVObject->SetStringField("class", bIsATT ? "ATT" : "IMU");
	TPVObject->SetStringField("device", DeviceName);
	time_t CurTime = TimestampMs / 1000;
	tm* ptm = std::gmtime(&CurTime);
	char time[100];
	SPrintfFormatedTime(TimestampMs, time);
	TPVObject->SetStringField("time", time);
	float Heading = WorldRot.Yaw - 90.f;
	Heading = Heading < 0.f ? Heading + 360.f : Heading > 360.f ? Heading - 360.f : Heading;
	TPVObject->SetNumberField("heading", Heading);
	TPVObject->SetNumberField("pitch", WorldRot.Pitch);
	TPVObject->SetStringField("pitch_st", "N");
	TPVObject->SetNumberField("roll", WorldRot.Roll);
	TPVObject->SetStringField("roll_st", "N");
	TPVObject->SetNumberField("yaw", WorldRot.Yaw);
	TPVObject->SetStringField("yaw_st", "N");
	TPVObject->SetNumberField("acc_len", LocalAcc.Size() / 100.0);
	TPVObject->SetNumberField("acc_x", -LocalAcc.X / 100.0);
	TPVObject->SetNumberField("acc_y", -LocalAcc.Y / 100.0);
	TPVObject->SetNumberField("acc_z", LocalAcc.Z / 100.0);
	TPVObject->SetNumberField("gyro_x", -Gyro.X);
	TPVObject->SetNumberField("gyro_y", -Gyro.Y);

	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(TPVObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}
}

void UGpsDSensorComponent::SendSKY(FGpsDConnection& Connection)
{
	TSharedPtr<FJsonObject> SKYObject = MakeShareable(new FJsonObject);
	SKYObject->SetStringField("class", "SKY");
	SKYObject->SetStringField("device", DeviceName);
	//time_t CurTime = Timestamp / 1000;
	//tm* ptm = std::gmtime(&CurTime);
	//char time[100];
	//SPrintfFormatedTime(Timestamp, time);
	//SKYObject->SetStringField("time", time);
	SKYObject->SetNumberField("xdop", 0.76);
	SKYObject->SetNumberField("ydop", 0.61);
	SKYObject->SetNumberField("vdop", 1.03);
	SKYObject->SetNumberField("tdop", 0.74);
	SKYObject->SetNumberField("hdop", 0.89);
	SKYObject->SetNumberField("gdop", 1.55);
	SKYObject->SetNumberField("pdop", 1.37);

	TArray<TSharedPtr<FJsonValue> > SatellitesArray;

	TSharedPtr<FJsonObject> SatelliteObject1 = MakeShareable(new FJsonObject);
	SatelliteObject1->SetNumberField("PRN", 1);
	SatelliteObject1->SetNumberField("el", 85);
	SatelliteObject1->SetNumberField("az", 349);
	SatelliteObject1->SetNumberField("ss", 38);
	SatelliteObject1->SetBoolField("used", true);
	SatelliteObject1->SetNumberField("gnssid", 0);
	SatelliteObject1->SetNumberField("svid", 1);
	TSharedRef< FJsonValueObject > JsonDeviceValue1 = MakeShareable(new FJsonValueObject(SatelliteObject1));
	SatellitesArray.Add(JsonDeviceValue1);

	TSharedPtr<FJsonObject> SatelliteObject2 = MakeShareable(new FJsonObject);
	SatelliteObject2->SetNumberField("PRN", 3);
	SatelliteObject2->SetNumberField("el", 57);
	SatelliteObject2->SetNumberField("az", 224);
	SatelliteObject2->SetNumberField("ss", 36);
	SatelliteObject2->SetBoolField("used", true);
	SatelliteObject2->SetNumberField("gnssid", 0);
	SatelliteObject2->SetNumberField("svid", 3);
	TSharedRef< FJsonValueObject > JsonDeviceValue2 = MakeShareable(new FJsonValueObject(SatelliteObject2));
	SatellitesArray.Add(JsonDeviceValue2);

	TSharedPtr<FJsonObject> SatelliteObject3 = MakeShareable(new FJsonObject);
	SatelliteObject3->SetNumberField("PRN", 4);
	SatelliteObject3->SetNumberField("el", 2);
	SatelliteObject3->SetNumberField("az", 180);
	SatelliteObject3->SetNumberField("ss", 42);
	SatelliteObject3->SetBoolField("used", true);
	SatelliteObject3->SetNumberField("gnssid", 0);
	SatelliteObject3->SetNumberField("svid", 4);
	TSharedRef< FJsonValueObject > JsonDeviceValue3 = MakeShareable(new FJsonValueObject(SatelliteObject3));
	SatellitesArray.Add(JsonDeviceValue3);

	TSharedPtr<FJsonObject> SatelliteObject4 = MakeShareable(new FJsonObject);
	SatelliteObject4->SetNumberField("PRN", 6);
	SatelliteObject4->SetNumberField("el", 25);
	SatelliteObject4->SetNumberField("az", 132);
	SatelliteObject4->SetNumberField("ss", 42);
	SatelliteObject4->SetBoolField("used", true);
	SatelliteObject4->SetNumberField("gnssid", 0);
	SatelliteObject4->SetNumberField("svid", 8);
	TSharedRef< FJsonValueObject > JsonDeviceValue4 = MakeShareable(new FJsonValueObject(SatelliteObject4));
	SatellitesArray.Add(JsonDeviceValue4);

	TSharedPtr<FJsonObject> SatelliteObject5 = MakeShareable(new FJsonObject);
	SatelliteObject5->SetNumberField("PRN", 9);
	SatelliteObject5->SetNumberField("el", 7);
	SatelliteObject5->SetNumberField("az", 94);
	SatelliteObject5->SetNumberField("ss",42);
	SatelliteObject5->SetBoolField("used", true);
	SatelliteObject5->SetNumberField("gnssid", 0);
	SatelliteObject5->SetNumberField("svid", 12);
	TSharedRef< FJsonValueObject > JsonDeviceValue5 = MakeShareable(new FJsonValueObject(SatelliteObject5));
	SatellitesArray.Add(JsonDeviceValue5);

	SKYObject->SetArrayField("satellites", SatellitesArray);

	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(SKYObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}

}

void UGpsDSensorComponent::SendVersion(FGpsDConnection& Connection)
{
	TSharedPtr<FJsonObject> VerObject = MakeShareable(new FJsonObject);
	VerObject->SetStringField("class", "VERSION");
	VerObject->SetStringField("release", "2.40dev");
	VerObject->SetStringField("rev", "06f62e14eae9886cde907dae61c124c53eb1101f");
	VerObject->SetNumberField("proto_major", 3);
	VerObject->SetNumberField("proto_minor", 1);
	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(VerObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}
}


void UGpsDSensorComponent::SendWatch(FGpsDConnection& Connection)
{
	TSharedPtr<FJsonObject> WatchObject = MakeShareable(new FJsonObject);
	WatchObject->SetStringField("class", "WATCH");
	WatchObject->SetBoolField("enable", Connection.WatcherMode);
	WatchObject->SetBoolField("json", Connection.WatcherJson);
	WatchObject->SetBoolField("nmea", false);
	WatchObject->SetNumberField("raw", 0);
	WatchObject->SetBoolField("scaled", false);
	WatchObject->SetBoolField("timing", false);
	WatchObject->SetBoolField("pps", false);

	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(WatchObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}
}

void UGpsDSensorComponent::SendDevices(FGpsDConnection& Connection)
{
	TSharedPtr<FJsonObject> DevicesObject = MakeShareable(new FJsonObject);
	DevicesObject->SetStringField("class", "DEVICES");
	TArray<TSharedPtr<FJsonValue> > ValuesArray;

	TSharedPtr<FJsonObject> DeviceObject = MakeShareable(new FJsonObject);
	DeviceObject->SetStringField("class", "DEVICE");
	//DeviceObject->SetStringField("path", Connection.DeviceName);
	//DeviceObject->SetStringField("activated", time);
	DeviceObject->SetNumberField("native", 0);
	DeviceObject->SetNumberField("bps", 4800);
	DeviceObject->SetStringField("parity", "N");
	DeviceObject->SetNumberField("stopbits", 1);
	DeviceObject->SetNumberField("cycle", 1.00);
	TSharedRef< FJsonValueObject > JsonDeviceValue = MakeShareable(new FJsonValueObject(DeviceObject));
	ValuesArray.Add(JsonDeviceValue);
	DevicesObject->SetArrayField("devices", ValuesArray);
	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	if (FJsonSerializer::Serialize(DevicesObject.ToSharedRef(), Writer))
	{
		Send(Connection, JsonString + TEXT("\r\n"));
	}
}

bool UGpsDSensorComponent::Send(FGpsDConnection &Connection, const FString& Data)
{
	if (Connection.ConnectionSocket)
	{
		TArray<char> ByteArray;

		ByteArray.Append(TCHAR_TO_ANSI(*Data), Data.Len());
		int32 BytesSent;
		bool res = Connection.ConnectionSocket->SendTo((uint8*)ByteArray.GetData(), ByteArray.Num(), BytesSent, *Connection.ConnectionAddress);
		if (bLogTcpDebugOutput || (!res && bBaseDebugOutput))
		{
			UE_LOG(LogSoda, Log, TEXT("UGpsDSensorComponent::Send: returned %d, Size = %d, bytes sent = %d, String = %s"), (int)res, ByteArray.Num(), BytesSent, *Data);
		}
		if (!res)
			Connection.SocketDead = true;
		return res;
	}
	return false;
}

void UGpsDSensorComponent::SPrintfFormatedTime(int64_t TimestampIn, char* Buffer)
{
#if PLATFORM_LINUX
	time_t CurTime = TimestampIn / 1000;
	tm* ptm = std::gmtime(&CurTime);
	sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld", 1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, TimestampIn % 1000);
#else
	time_t CurTime = TimestampIn / 1000;
	tm ptm;
	FMemory::Memzero(&ptm, sizeof(ptm));
	gmtime_s(&ptm, &CurTime);
	sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%03lld", 1900 + ptm.tm_year, 1 + ptm.tm_mon, ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec, TimestampIn % 1000);
#endif
}
