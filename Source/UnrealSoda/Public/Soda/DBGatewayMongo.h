// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/DBGateway.h"
#include "mongocxx/instance.hpp"
#include "mongocxx/pool.hpp"

namespace soda
{

/**
 * FDBGatewayMongo
 */
class FDBGatewayMongo : public FDBGateway
{
	friend FDBGateway;

public:
	virtual void Configure(const FString& URL, const FString& DatabaseName, bool bSync) override;
	virtual void Disable() override;
	virtual ~FDBGatewayMongo();

public:
	mongocxx::pool::entry GetConnection();
	bsoncxx::stdx::optional<mongocxx::pool::entry> TryGetConnection();
	bsoncxx::stdx::optional<mongocxx::pool::entry>& GetGameThreadClient() { return GameThreadClient; }
	bsoncxx::stdx::optional<mongocxx::pool::entry>& GetMongoThreadClient() { return MongoThreadClient; }
	TFuture<bool> AsyncTask(TUniqueFunction < bool()> Function, bool bSync = false);

public:
	virtual bool BeginRecordDataset() override;
	virtual void EndRecordDataset(EScenarioStopReason Reasone, bool bImmediately = false) override;
	virtual TSharedPtr<FActorDatasetData> CreateActorDataset(const FString& ActorName, const FString& ActorType, const FString& ActorClassName, const bsoncxx::builder::stream::document& Description) override;

public:
	virtual bool SaveVehicleData(const FString& VehicleName, const bsoncxx::document::value& Data) override;
	virtual bsoncxx::document::view LoadVehicleData(const FString& VehicleName) override;
	virtual bool DeleteVehicleData(const FString& VehicleName) override;
	virtual bool LoadVehiclesList(TArray<FVechicleSaveAddress>& Addresses) override;

public:
	/** return ScenarioID or -1 if error */
	virtual int64 SaveLevelData(const FLevelStateSlotDescription& Slot, const TArray<uint8>& ObjectBytes) override;
	virtual bool LoadLevelData(int64 ScenarioID, TArray<uint8>& OutObjectBytes) override;
	virtual bool DeleteLevelData(int64 ScenarioID) override;
	virtual bool LoadLevelList(const FString& LevelName, bool bSortByDateTime, TArray<FLevelStateSlotDescription>& OutSlots) override;

protected:
	FDBGatewayMongo();
	void Disable_MongoThread();
	void Tick_MongoThread();

	TUniquePtr<mongocxx::instance> Inst;
	TUniquePtr<mongocxx::pool> Pool;
	bsoncxx::stdx::optional<mongocxx::pool::entry> GameThreadClient;
	bsoncxx::stdx::optional<mongocxx::pool::entry> MongoThreadClient;
	soda::FAsyncTaskManager MongoTaskManager;
	FRunnableThread* MongoTaskManagerThread = nullptr;
};

} // namespace soda