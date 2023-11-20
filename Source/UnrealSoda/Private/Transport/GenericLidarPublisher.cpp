// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/GenericLidarPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/TcpSocketBuilder.h"
#include <numeric>
#include <sstream>

struct FLidarFrame
{
	uint16 DeviceID = 0;
	int32 ScanID = 0;
	int64 Timestamp = 0;
	int PointsPerDatagram = 0;
};

/***********************************************************************************************
	FLidarAsyncTask
***********************************************************************************************/
class FLidarAsyncTask : public soda::FAsyncTask
{
public:
	virtual ~FLidarAsyncTask() {}
	virtual FString ToString() const override { return "LidarAsyncTask"; }
	virtual void Initialize() override { bIsDone = false; }
	virtual bool IsDone() const override { return bIsDone; }
	virtual bool WasSuccessful() const override { return true; }
	virtual void Tick() override
	{
		check(!bIsDone);
		Publisher->PublishSync(LidarFrame, Points);
		bIsDone = true;
	}

public:
	FLidarFrame LidarFrame;
	std::vector<soda::LidarScanPoint> Points;
	FGenericLidarPublisher* Publisher = nullptr;

protected:
	bool bIsDone = true;
};

/***********************************************************************************************
	FLidarFrontBackAsyncTask
***********************************************************************************************/
class FLidarFrontBackAsyncTask : public soda::FDoubleBufferAsyncTask<FLidarAsyncTask>
{
public:
	FLidarFrontBackAsyncTask(FGenericLidarPublisher* Publisher)
	{
		FrontTask->Publisher = Publisher;
		BackTask->Publisher = Publisher;
	}
	virtual ~FLidarFrontBackAsyncTask() {}
	virtual FString ToString() const override { return "FLidarFrontBackAsyncTask"; }
};

/***********************************************************************************************
	FGenericLidarPublisher
***********************************************************************************************/
FGenericLidarPublisher::FGenericLidarPublisher()
{
	AsyncTask = MakeShareable(new FLidarFrontBackAsyncTask(this));
}

void FGenericLidarPublisher::Advertise()
{
	Shutdown();

	Socket = TSharedPtr<FSocket> (ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("default"), false));
	Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetPort(Port);

	bool valid;
	Addr->SetIp((const TCHAR*)(*Address), valid);

	if (!valid)
	{
		UE_LOG(LogSoda, Error, TEXT("FGenericLidarPublisher::Advertise() IP address isn't valid"));
		Shutdown();
		return;
	}

	AsyncTask->Start();
	SodaApp.EthTaskManager.AddTask(AsyncTask);
}

void FGenericLidarPublisher::Shutdown()
{
	AsyncTask->Finish();
	SodaApp.EthTaskManager.RemoteTask(AsyncTask);

	if (Socket)
	{
		Socket->Close();
		Socket.Reset();
	}
}

void FGenericLidarPublisher::PublishSync(const soda::LidarScan& ScanToPublish)
{
	if (!Socket)
		return;

	std::stringstream sout(std::ios_base::out | std::ios_base::binary);
	int const points_count = soda::write(sout, ScanToPublish);
	if (!sout)
		UE_LOG(LogSoda, Fatal, TEXT("FGenericLidarPublisher::PublishSync() failed to serialize scan"));
	sout.seekp(0, std::ios_base::end);
	auto const length = sout.tellp();

	int32 BytesSent;
	if (!Socket->SendTo((const uint8*)sout.str().data(), sout.str().length(), BytesSent, *Addr))
	{
		ESocketErrors ErrorCode = ISocketSubsystem::Get()->GetLastErrorCode();
		UE_LOG(LogSoda, Error, TEXT("FGenericLidarPublisher::PublishSync() Can't send(), error code %i, error = %s"), int32(ErrorCode), ISocketSubsystem::Get()->GetSocketError(ErrorCode));
	}
}

void FGenericLidarPublisher::PublishSync(const FLidarFrame& LidarFrame, std::vector<soda::LidarScanPoint>& Points)
{
	Scan.device_timestamp = LidarFrame.Timestamp;
	Scan.scan_id = LidarFrame.ScanID;
	Scan.block_id = 0;
	Scan.block_count = (Points.size() / LidarFrame.PointsPerDatagram) + ((Points.size() % LidarFrame.PointsPerDatagram) ? 1 : 0);

	for (int k = 0; k < Points.size(); )
	{
		int block_size = Points.size() - k;
		if (block_size > LidarFrame.PointsPerDatagram) block_size = LidarFrame.PointsPerDatagram;

		Scan.points.resize(block_size);
		std::memcpy(&Scan.points[0], &Points[k], block_size * sizeof(soda::LidarScanPoint));
		PublishSync(Scan);

		k += block_size;
		++Scan.block_id;
	}
}

void FGenericLidarPublisher::Publish(TTimestamp Timestamp, std::vector<soda::LidarScanPoint>& Points, bool bAsync)
{
	if (bAsync)
	{
		TSharedPtr<FLidarAsyncTask> Task = AsyncTask->LockFrontTask();
		if (!Task->IsDone())
		{
			UE_LOG(LogSoda, Warning, TEXT("FGenericLidarPublisher::Publish(). Skipped one frame"));
		}
		Task->LidarFrame.DeviceID = DeviceID;
		Task->LidarFrame.ScanID = ScanID++;
		Task->LidarFrame.Timestamp = soda::RawTimestamp<std::chrono::milliseconds>(Timestamp);
		Task->LidarFrame.PointsPerDatagram = PointsPerDatagram;
		Task->Points = Points;
		Task->Initialize();
		AsyncTask->UnlockFrontTask();
		SodaApp.EthTaskManager.Trigger();
	}
	else
	{
		FLidarFrame LidarFrame;
		LidarFrame.DeviceID = DeviceID;
		LidarFrame.ScanID = ScanID++;
		LidarFrame.Timestamp = soda::RawTimestamp<std::chrono::milliseconds>(Timestamp);
		LidarFrame.PointsPerDatagram = PointsPerDatagram;
		PublishSync(LidarFrame, Points);
	}
}