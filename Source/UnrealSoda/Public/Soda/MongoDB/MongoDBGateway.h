// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "Soda/SodaTypes.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/LevelState.h"
#include "Soda/SodaUserSettings.h"

#include "mongocxx/instance.hpp"
#include "mongocxx/pool.hpp"
#include "mongocxx/database.hpp"

#include "MongoDBGateway.generated.h"

UCLASS(ClassGroup = Soda)
class UNREALSODA_API USodaMongoDBSettings : public USodaUserSettings
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = MongoDB, meta = (EditInRuntime))
	FString MongoURL;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = MongoDB, meta = (EditInRuntime))
	FString DatabaseName;

	//UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = MongoDB, meta = (EditInRuntime))
	//bool bConnectAtStartUp = false;

	UFUNCTION(Category = MongoDB, meta = (CallInRuntime))
	void Reconnect();

	virtual void ResetToDefault() override;
	virtual FText GetMenuItemText() const override;
	virtual FText GetMenuItemDescription() const override;
	virtual FName GetMenuItemIconName() const override;

};

namespace soda
{

enum class EMongoDBGatewayStatus
{
	Disconnected,
	Connected,
	Connecting,
	Faild
};

/**
 * FMongoDBGetway
 */
class FMongoDBGetway
{
	FMongoDBGetway();

public:
	~FMongoDBGetway();
	static FMongoDBGetway& Get(bool bConnectIfDisconnected);

	bool Connect(const FString& URL, const FString& DatabaseName, bool bSync);
	void Disconnect();

	void Destroy();

	mongocxx::pool::entry GetConnection();
	bsoncxx::stdx::optional<mongocxx::pool::entry> TryGetConnection();
	bsoncxx::stdx::optional<mongocxx::pool::entry>& GetGameThreadClient() { return GameThreadClient; }
	bsoncxx::stdx::optional<mongocxx::pool::entry>& GetMongoThreadClient() { return MongoThreadClient; }
	TFuture<bool> AsyncTask(TUniqueFunction < bool()> Function, bool bSync = false);

	EMongoDBGatewayStatus GetStatus() const { return Status; }
	bool IsConnected() const { return Status == EMongoDBGatewayStatus::Connected;  }
	const FString& GetDatabaseName() const { return DatabaseName; }
	mongocxx::database& GetDB() { return DB; }

	void OnFaild(const FString & What, const FString& FunctionName, bool ShowMessageBox);

	FAsyncTaskManager& GetTaskManager() { return MongoTaskManager; }
	const FAsyncTaskManager& GetTaskManager() const { return MongoTaskManager; }

public:
	bool SaveVehicleData(const FString& VehicleName, const bsoncxx::document::value& Data);
	bsoncxx::document::view LoadVehicleData(const FString& VehicleName);
	bool DeleteVehicleData(const FString& VehicleName);
	bool LoadVehiclesList(TArray<FVechicleSaveAddress>& Addresses);

public:
	/** return ScenarioID or -1 if error */
	int64 SaveLevelData(const FLevelStateSlotDescription& Slot, const TArray<uint8>& ObjectBytes);
	bool LoadLevelData(int64 ScenarioID, TArray<uint8>& OutObjectBytes);
	bool DeleteLevelData(int64 ScenarioID);
	bool LoadLevelList(const FString& LevelName, bool bSortByDateTime, TArray<FLevelStateSlotDescription>& OutSlots);

private: 
	TUniquePtr<mongocxx::instance> Inst;
	soda::FAsyncTaskManager MongoTaskManager;
	FRunnableThread* MongoTaskManagerThread = nullptr;

private:
	TUniquePtr<mongocxx::pool> Pool;
	bsoncxx::stdx::optional<mongocxx::pool::entry> GameThreadClient;
	bsoncxx::stdx::optional<mongocxx::pool::entry> MongoThreadClient;
	FString DatabaseName;
	mongocxx::database DB;

private:
	EMongoDBGatewayStatus Status = EMongoDBGatewayStatus::Disconnected;
	FString LastError;

	void Disconnect_MongoThread();
};

} // namespace soda