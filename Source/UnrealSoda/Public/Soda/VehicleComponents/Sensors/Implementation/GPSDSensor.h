// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/ImuGnssSensor.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Common/TcpListener.h"
#include <mutex>
#include "GPSDSensor.generated.h"

USTRUCT(BlueprintType)
struct UNREALSODA_API FGpsDConnection
{
	GENERATED_BODY()

	FGpsDConnection()
	{
		ConnectionSocket = 0;
		ConnectionAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	};

	FGpsDConnection(FSocket* NewSocket, TSharedPtr< FInternetAddr > NewAddress)
	{
		ConnectionSocket = NewSocket;
		ConnectionAddress = NewAddress;
	};

	FSocket* ConnectionSocket;
	TSharedPtr< FInternetAddr > ConnectionAddress;

	bool WatcherMode = false;
	bool WatcherJson = false;

	FString RequestNameBuffer;
	FString RequestJsonBuffer;

	bool SocketDead = false;

	bool ParseAsJson = false;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGpsDSensorComponent : public UImuGnssSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int TcpPort = 2947;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "0.0.0.0";

	UPROPERTY(EditAnywhere, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bBaseDebugOutput = false;
	
	UPROPERTY(EditAnywhere, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bLogTcpDebugOutput = false;

	/** Watcher mode send period [sec] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float WatcherModePeriod = 0.1f;

	/** Send SKY data in the Watcher Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bWatcherModeSKY = true;

	/** Send TPV data in the Watcher Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bWatcherModeTPV = true;

	/** Send ATT data in the Watcher Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bWatcherModeATT = true;

	/** Send IMU data in the Watcher Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bWatcherModeIMU = false; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	FString DeviceName = "Simulator_GPSD";

protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	void Shutdown();
	bool OnConnected(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint);

	void SendTPV(FGpsDConnection& Connection);
	void SendATT_IMU(FGpsDConnection& Connection, bool bIsATT);
	void SendSKY(FGpsDConnection& Connection);
	void SendVersion(FGpsDConnection& Connection);
	void SendWatch(FGpsDConnection& Connection);
	void SendDevices(FGpsDConnection& Connection);
	bool Send(FGpsDConnection& Connection, const FString& Data);
	void SPrintfFormatedTime(int64_t Timestamp, char* Buffer);

	FSocket* TcpServerSocket = 0;
	FTcpListener * TcpListener;
	TArray<FGpsDConnection> Connections;
	TTimestamp Timestamp;
	double Lon = 0;
	double Lat = 0;
	double Alt = 0;
	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FRotator WorldRot;
	FVector WorldLoc;
	FVector Gyro;
	FPhysBodyKinematic CurVehicleKinematic;
	TTimestamp PrevSendTimeMark;
	std::mutex Mutex;
};
