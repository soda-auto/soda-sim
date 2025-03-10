// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/ISodaDataset.h"
#include "Framework/Notifications/NotificationManager.h"

#include "mongocxx/database.hpp"
#include "mongocxx/collection.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/uri.hpp"
#include "mongocxx/instance.hpp"
#include "mongocxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"

namespace soda
{
class FAsyncTask;

namespace mongodb 
{
	class FObjectDatasetMongDBHandler;

/**
 * FMongoDBDatasetManager
 */
class FMongoDBDatasetManager : public soda::IDatasetManager
{
public:
	FMongoDBDatasetManager();
	virtual ~FMongoDBDatasetManager() {}

	virtual bool BeginRecording() override;
	virtual void EndRecording(EScenarioStopReason Reasone, bool bImmediately) override;
	//virtual EDatasetManagerStatus GetStatus() const override;
	virtual FText GetToolTip() const override;
	virtual FText GetDisplayName() const override;
	virtual FName GetIconName() const override;
	virtual EDatasetManagerStatus GetStatus() const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ClearDatasetsQueue() override;

public:
	int64 GetDatasetID() const { return  DatasetID; }
	const FString& GetCollectionName() const { return CollectionName; }

	//int GetDasetesQueueNum() const;

	void OnFaild(const FString & What, const FString & FunctionName, bool bShowMessageBox);

	bool IsDatasetWriting() const { return DatasetHandlers.Num() > 0; }

public:
	template <typename FuncType>
	void RegisterObjectHandler(UClass* Class, FuncType&& CreateFunc )
	{
		RegistredHandlers.Emplace(Class, Forward<FuncType>(CreateFunc));
	}

private:
	TSharedPtr<soda::FAsyncTask> MongoTask;

	FString CollectionName;
	mongocxx::collection Collectioin;
	bsoncxx::oid DecriptorOID;
	//EDatasetManagerStatus Status;
	bool bIsRecordingStarted = false;
	bool bWasFaild = false;

	int64 DatasetID = -1;
	TSharedPtr<SNotificationItem> ProgressNotification;
	TArray<TSharedPtr<FObjectDatasetMongDBHandler>> DatasetHandlers;
	//mutable FCriticalSection DatasetHandlersLock;

	TMap<UClass*, TFunction<TSharedRef<FObjectDatasetMongDBHandler>(UObject*)>> RegistredHandlers;

	void ReleaseRecording_MongoThread();
	void Tick_MongoThread();
};

/**
 * FObjectDatasetMongDBHandler
 */
class FObjectDatasetMongDBHandler: public IObjectDatasetHandler
{
public:
	FObjectDatasetMongDBHandler();
	FObjectDatasetMongDBHandler(const UObject* Object);

	virtual ~FObjectDatasetMongDBHandler();

	virtual void BeginRecording() override {}
	virtual void EndRecording() override {}
	virtual bool Sync() override;
	virtual bool Sync(bsoncxx::builder::stream::document& Doc) { return false; }

	virtual void CreateObjectDescription(bsoncxx::builder::stream::document& Doc);
	virtual int GetMaxBatchSize() const { return 10000; }

	//bsoncxx::builder::stream::document& GetBatchItemDoc() { return BatchItemDoc; }
	bsoncxx::builder::stream::document& BeginBatchItem();
	void EndBatchItem();
	void FinalizeBatch();
	bool QueueIsEmpty() const { return DocQueue.IsEmpty(); }
	TSharedPtr<bsoncxx::builder::stream::document> Dequeue();
	//bool IsValid() const { return bIsValid; }
	int GetQueueNum() const { return QueueNum; }

protected:
	TQueue<TSharedPtr<bsoncxx::builder::stream::document>> DocQueue;
	bsoncxx::builder::stream::array BatchArray;
	bsoncxx::builder::stream::document BatchItemDoc;
	int DocPart = 0;
	int DocsPerPart = 0;
	FString ObjectName{};
	//FString ObjectType{};
	FString ObjectClass{};
	int QueueNum = 0;

};

} // namespace mongodb
} // namespace soda