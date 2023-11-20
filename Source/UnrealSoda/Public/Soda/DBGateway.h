// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/SodaTypes.h"
#include "Containers/Queue.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/builder/stream/document.hpp"

struct FVechicleSaveAddress;
struct FLevelStateSlotDescription;

namespace soda
{

enum class EDBGatewayStatus
{
	Connected,
	Disabled,
	Faild,
	Connecting
};

/**
 * Helper struct to hide the Bson defenitions
 */
struct FBsonDocument
{
	bsoncxx::builder::stream::document Document;

	bsoncxx::builder::stream::document& operator *()
	{
		return Document;
	}

	bsoncxx::builder::stream::document& operator->()
	{
		return Document;
	}
};

class FActorDatasetData;

/**
 * FDBGateway
 */
class FDBGateway
{
public:
	static FDBGateway& Instance();
	virtual ~FDBGateway() {}

	virtual void Configure(const FString& URL, const FString& DatabaseName, bool bSync) = 0;
	virtual void Disable() = 0;

	static FString ScenarioStopReasonToString(EScenarioStopReason Reason);

public:
	EDBGatewayStatus GetStatus() const { return Status; }
	bool IsConnected() const { return Status == EDBGatewayStatus::Connected; }
	const FString& GetDatabaseName() const { return DatabaseName; }
	const FString& GetLastError() const { return LastError; }
	void SetLastError(const FString& InLastError) { LastError = InLastError; }

public:
	virtual bool BeginRecordDataset() = 0;
	/** bImmediately - don't wait until all datasets are written to the DB */
	virtual void EndRecordDataset(EScenarioStopReason Reasone, bool bImmediately = false) = 0;
	bool IsDatasetRecording() const { return bIsDatasetRecording; }
	int64 GetDatasetID() const { return  DatasetID; }
	const FString& GetDatasetCollectionName() const { return DatasetCollectionName; }
	virtual TSharedPtr<FActorDatasetData> CreateActorDataset(const FString& ActorName, const FString& ActorType, const FString & ActorClassName, const bsoncxx::builder::stream::document& Description) = 0;
	void ClearDatasetsQueue();
	int GetDasetesQueueNum() const;
	void StallDatasetRecording();
	bool IsDatasetRecordingStalled() const { return bIsDatasetRecordingStalled; }

public:
	virtual bool SaveVehicleData(const FString& VehicleName, const bsoncxx::document::value& Data) = 0;
	virtual bsoncxx::document::view LoadVehicleData(const FString& VehicleName) = 0;
	virtual bool DeleteVehicleData(const FString& VehicleName) = 0;
	virtual bool LoadVehiclesList(TArray<FVechicleSaveAddress>& Addresses) = 0;

public:
	/** return ScenarioID or -1 if error */
	virtual int64 SaveLevelData(const FLevelStateSlotDescription& Slot, const TArray<uint8>& ObjectBytes) = 0;
	virtual bool LoadLevelData(int64 ScenarioID, TArray<uint8>& OutObjectBytes) = 0;
	virtual bool DeleteLevelData(int64 ScenarioID) = 0;
	virtual bool LoadLevelList(const FString& LevelName, bool bSortByDateTime, TArray<FLevelStateSlotDescription>& OutSlots) = 0;

protected:
	FDBGateway() {}
	FString DatasetCollectionName;
	FString DatabaseName;
	EDBGatewayStatus Status = EDBGatewayStatus::Disabled;
	bool bIsDatasetRecording = false;
	int64 DatasetID = -1;
	TArray<TSharedPtr<FActorDatasetData>> Datasets;
	mutable FCriticalSection DatasetsLock;
	FString LastError = "";
	bool bIsDatasetRecordingStalled = false;
};

/**
 * FActorDatasetData
 */
class FActorDatasetData
{
public:
	FActorDatasetData(const FString& ActorName);
	bsoncxx::builder::stream::document& GetRowDoc() { return RowDoc; }
	void BeginRow();
	void EndRow();
	void PushAsync();
	bool QueueIsEmpty() const { return DocQueue.IsEmpty(); }
	TSharedPtr<bsoncxx::builder::stream::document> Dequeue();
	void SetIsInvalidate() { bIsInvalidate = true; }
	bool IsInvalidate() const { return bIsInvalidate; }
	int GetQueueNum() const { return QueueNum; }

protected:
	TQueue<TSharedPtr<bsoncxx::builder::stream::document>> DocQueue;
	bsoncxx::builder::stream::array CurrentArray;
	bsoncxx::builder::stream::document RowDoc;
	int DocPart = 0;
	int DocsPerPart = 0;
	FString ActorName;
	FString DatasetCollectionName;
	FString DatabaseName;
	const int MaxDocsPerPart = 10000;
	bool bIsInvalidate = false;
	int QueueNum = 0;
};

} // namespace soda