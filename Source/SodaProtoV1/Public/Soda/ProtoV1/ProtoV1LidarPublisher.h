// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericPublishers/GenericLidarPublisher.h"
#include "soda/sim/proto-v1/lidar.hpp"
#include "Soda/Misc/Time.h"
#include "Soda/Misc/UDPAsyncTask.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "ProtoV1LidarPublisher.generated.h"

struct FLidarFrame
{
	uint16 DeviceID = 0;
	int32 ScanID = 0;
	int64 Timestamp = 0;
	int PointsPerDatagram = 0;
};

/**
* USodaSimLidarPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1LidarPublisher : public UGenericLidarPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "239.0.0.230";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bAsync = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	int PointsPerDatagram = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	int DeviceID = 0;

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) override;
	virtual void Shutdown() override;
	virtual bool IsOk() const override { return !!Socket; }
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan) override;
	virtual FString GetRemark() const override;

protected:
	bool Publish(const soda::sim::proto_v1::LidarScan& Scan);

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
	soda::sim::proto_v1::LidarScan Msg;
	uint32 ScanID = 0;
};



