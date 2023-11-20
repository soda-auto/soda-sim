// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "SodaSimProto/Lidar.hpp"
#include "Soda/Misc/Time.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "GenericLidarPublisher.generated.h"

class FLidarFrontBackAsyncTask;
class FLidarAsyncTask;
struct FLidarFrame;

/***********************************************************************************************
    FGenericLidarPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FGenericLidarPublisher
{
	GENERATED_BODY()

	friend FLidarFrontBackAsyncTask;
	friend FLidarAsyncTask;

	FGenericLidarPublisher();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "239.0.0.230";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	int PointsPerDatagram = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	int DeviceID = 0;

	void Advertise();
	void Shutdown();

	void Publish(TTimestamp Timestamp, std::vector<soda::LidarScanPoint>& Points, bool bAsync = true);
	bool IsAdvertised() { return !!Socket; }

	
protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <FLidarFrontBackAsyncTask> AsyncTask;
	soda::LidarScan Scan;
	uint32 ScanID = 0;

	void PublishSync(const FLidarFrame& LidarFrame, std::vector<soda::LidarScanPoint>& Points);
	void PublishSync(const soda::LidarScan& Scan);
};



