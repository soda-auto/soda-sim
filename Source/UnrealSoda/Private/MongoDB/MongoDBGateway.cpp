// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/MongoDB/MongoDBGateway.h"
#include "Soda/UnrealSoda.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/LevelState.h"
#include "Soda/Misc/Time.h"
#include "Soda/UI/SMessageBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/RunnableThread.h"
#include "Async/Async.h"

#include "mongocxx/database.hpp"
#include "mongocxx/collection.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/uri.hpp"
#include "mongocxx/instance.hpp"
#include "mongocxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"

#define LOCTEXT_NAMESPACE "MongoDBGateway"

#define COLLECTION_NAME_VEHICLES "vehicles"
#define COLLECTION_NAME_DATASETES "datasets"
#define COLLECTION_NAME_SCENARIOS "scenarios"
#define COLLECTION_NAME_DATASETE_PREFIX "dataset"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

USodaMongoDBSettings::USodaMongoDBSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetToDefault();
}

void USodaMongoDBSettings::ResetToDefault()
{
	MongoURL = TEXT("mongodb://localhost:27017");
	DatabaseName = TEXT("sodasim");
}

FName USodaMongoDBSettings::GetMenuItemIconName() const
{ 
	static FName IconName = "SodaIcons.MongoDB";
	return IconName;
}
FText USodaMongoDBSettings::GetMenuItemText() const
{
	return LOCTEXT("SodaMongoDBSettings_Description", "MongoDB Settings");
}

FText USodaMongoDBSettings::GetMenuItemDescription() const
{
	return LOCTEXT("SodaMongoDBSettings_Description", "SODA.Sim MongoDB settings");
}

void USodaMongoDBSettings::Reconnect()
{
	soda::FMongoDBGetway & Getway = soda::FMongoDBGetway::Get(false);
	Getway.Disconnect();
	Getway.Get(true);
}

namespace soda
{

FMongoDBGetway& FMongoDBGetway::Get(bool bConnectIfDisconnected)
{
	static FMongoDBGetway Getway;
	if (Getway.GetStatus() == EMongoDBGatewayStatus::Disconnected && bConnectIfDisconnected)
	{
		Getway.Connect(GetDefault<USodaMongoDBSettings>()->MongoURL, GetDefault<USodaMongoDBSettings>()->DatabaseName, true);
	}
	return Getway;
}

FMongoDBGetway::FMongoDBGetway()
{
}

FMongoDBGetway::~FMongoDBGetway()
{
	Destroy();
}

void FMongoDBGetway::Destroy()
{
	if (MongoTaskManagerThread)
	{
		MongoTaskManagerThread->Kill(true);
		delete MongoTaskManagerThread;
		MongoTaskManagerThread = nullptr;
	}

	Disconnect_MongoThread();
}


bool FMongoDBGetway::Connect(const FString& URL, const FString& InDatabaseName, bool bSync)
{
	if (Status == EMongoDBGatewayStatus::Connecting)
	{
		return false;
	}

	Status = EMongoDBGatewayStatus::Connecting;

	if (!Inst)
	{
		Inst = MakeUnique<mongocxx::instance>();
	}

	if (!MongoTaskManagerThread)
	{
		MongoTaskManagerThread = FRunnableThread::Create(&MongoTaskManager, TEXT("MongoTaskManager"));
		if (MongoTaskManagerThread == nullptr)
		{
			UE_LOG(LogSoda, Fatal, TEXT("FMongoDBGetway::Connect(); Can't create MongoTaskManager"));
		}
	}

	MongoTaskManager.ClearQueue();
	
	auto Future = AsyncTask([this, InDatabaseName, URL]()
	{
		Disconnect_MongoThread();

		Pool = MakeUnique<mongocxx::pool>(mongocxx::uri{ TCHAR_TO_UTF8(*URL) });
		GameThreadClient = Pool->try_acquire();
		MongoThreadClient = Pool->try_acquire();
		DatabaseName = InDatabaseName;

		try
		{
			if (MongoThreadClient && *MongoThreadClient && **MongoThreadClient)
			{
				DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
				bsoncxx::document::value RetVal = DB.run_command(document{} << "ping" << 1 << finalize);
				Status = EMongoDBGatewayStatus::Connected;
			}
			else
			{
				OnFaild(TEXT("Can't connect to server; Can't can't create client"), ANSI_TO_TCHAR(__FUNCTION__), true);
				return false; 
			}
		}
		catch (const std::system_error& e)
		{
			OnFaild(FString::Printf(TEXT("Can't connect to server; MongoDb error: "), UTF8_TO_TCHAR(e.what())), ANSI_TO_TCHAR(__FUNCTION__), true);
			return false;
		}

		::AsyncTask(ENamedThreads::GameThread, [this, URL=URL]()
		{
			FNotificationInfo Msg(FText::FromString(FString::Printf(TEXT("Connected to MongoDB: %s"), *URL)));
			Msg.ExpireDuration = 5.0f;
			Msg.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.InfoWithColor"));
			FSlateNotificationManager::Get().AddNotification(Msg);
		});

		return true;

	});

	if (bSync)
	{
		return Future.Get();
	}
	else
	{
		return true;
	}
}

TFuture<bool> FMongoDBGetway::AsyncTask(TUniqueFunction < bool()> Function, bool bSync)
{
	if (!MongoTaskManagerThread)
	{
		UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::AsyncTask(); FDBGatewayMongo isn't configured"));
		TPromise<bool> Promise;
		Promise.SetValue(false);
		return Promise.GetFuture();
	}

	return soda::AsyncTask(MongoTaskManager, MoveTemp(Function), bSync);
}

void FMongoDBGetway::OnFaild(const FString& What, const FString& FunctionName, bool ShowMessageBox)
{
	LastError = What;
	UE_LOG(LogSoda, Error, TEXT("%s; %s"), *FunctionName, *What);
	Status = EMongoDBGatewayStatus::Faild;

	if (ShowMessageBox)
	{
		::AsyncTask(ENamedThreads::GameThread, [LastError = LastError]()
		{
			USodaSubsystem::GetChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "MongoDB Faild", *LastError);
		});
	}
}

void FMongoDBGetway::Disconnect()
{
	if (MongoTaskManagerThread)
	{
		AsyncTask([this]()
		{
			Disconnect_MongoThread();
			return true;
		}).Get();
	}
}

void FMongoDBGetway::Disconnect_MongoThread()
{
	GameThreadClient.reset();
	MongoThreadClient.reset();
	Pool.Reset();
	DatabaseName = "";
	DB = {};
	LastError = "";
	Status = EMongoDBGatewayStatus::Disconnected;


	//FScopeLock ScopeLock(&DatasetsLock);
	//Datasets.Empty();
	//bIsDatasetRecording = false;
	//DatasetID = -1;
	//DatasetCollectionName = "";
	//bIsDatasetRecordingStalled = false;
}


mongocxx::pool::entry FMongoDBGetway::GetConnection()
{
	return Pool->acquire();
}

bsoncxx::stdx::optional<mongocxx::pool::entry> FMongoDBGetway::TryGetConnection()
{
	return Pool->try_acquire();
}

bool FMongoDBGetway::SaveVehicleData(const FString& VehicleName, const bsoncxx::document::value& Data)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, VehicleName, &Data]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::SaveVehicleData(); MongoDB isn't connected"));
			return false;
		}

		mongocxx::database DB = (**GetMongoThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
		mongocxx::collection Collectioin = DB[COLLECTION_NAME_VEHICLES];

		try
		{
			mongocxx::options::replace Opts;
			Opts.upsert(true);
			Collectioin.replace_one(
				document{} << "_id" << TCHAR_TO_UTF8(*VehicleName) << finalize,
				document{} 
					<< "_id" << TCHAR_TO_UTF8(*VehicleName)
					<< "timestamp" << std::int64_t((FDateTime::Now().GetTicks() - FDateTime(1970, 1, 1).GetTicks()) / ETimespan::TicksPerMicrosecond)
					<< "soda.sim" << Data
					<< finalize,
				Opts
			);
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::SaveVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

bsoncxx::document::view FMongoDBGetway::LoadVehicleData(const FString& VehicleName)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	bsoncxx::document::view View;

	auto Future = AsyncTask([this, VehicleName, &View]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadVehicleData(); MongoDB isn't connected"));
			return false;
		}

		mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
		mongocxx::collection Collectioin = DB[COLLECTION_NAME_VEHICLES];

		try
		{
			auto Value = Collectioin.find_one(document{} << "_id" << TCHAR_TO_UTF8(*VehicleName) << finalize);
			if (!Value)
			{
				return false;
			}

			View = Value->view()["soda.sim"].get_document().view();
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}

		return true;
	});

	Future.Wait();// Get();
	return View;
}

bool FMongoDBGetway::DeleteVehicleData(const FString& VehicleName)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, VehicleName]()
	{
		try
		{
			mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_VEHICLES];

			auto Res = Collectioin.delete_one(document{} << "_id" << TCHAR_TO_UTF8(*VehicleName) << finalize);
			return Res->deleted_count() > 0;
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::DeleteVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return false;
	});

	return Future.Get();
}

bool FMongoDBGetway::LoadVehiclesList(TArray<FVechicleSaveAddress>& Addresses)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, &Addresses]()
	{
		if (!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadVehiclesList(); MongoDB isn't connected"));
			return false;
		}

		try
		{
			mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_VEHICLES];

			mongocxx::options::find Opts;
			//if(bSortByDateTime) Opts.sort(document{} << "timestamp" << -1 << finalize);
			//Opts.allow_partial_results(true);
			//Opts.projection(document{} << "bin" << 0 << finalize);

			for (auto& It : Collectioin.find({}, Opts))
			{
				Addresses.Add(FVechicleSaveAddress(EVehicleSaveSource::DB, It["_id"].get_string().value.to_string().c_str()));
			}
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadVehiclesList(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

int64 FMongoDBGetway::SaveLevelData(const FLevelStateSlotDescription & Slot, const TArray<uint8> & ObjectBytes)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	int64 ScenarioID = Slot.ScenarioID;

	auto Future = AsyncTask([this, &Slot, &ObjectBytes, &ScenarioID]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::SaveLevelData(); MongoDB isn't connected"));
			return false;
		}

		mongocxx::database DB = (**GetMongoThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
		mongocxx::collection Collectioin = DB[COLLECTION_NAME_SCENARIOS];

		if (ScenarioID < 0)
		{
			try
			{
				mongocxx::options::find Opts;
				Opts.sort(document{} << "_id" << -1 << finalize);
				Opts.limit(1);
				auto Cursor = Collectioin.find({}, Opts);
				if (Cursor.begin() != Cursor.end())
				{
					auto Doc = *Cursor.begin();
					ScenarioID = Doc["_id"].get_int64() + 1;
				}
				else
				{
					ScenarioID = 0;
				}
			}
			catch (const std::system_error& e)
			{
				UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::SaveLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
				return false;
			}
		}

		try
		{
			mongocxx::options::replace Opts;
			Opts.upsert(true);
			Collectioin.replace_one(
				document{} << "_id" << std::int64_t(ScenarioID) << finalize,
				document{} 
					<< "_id" << std::int64_t(ScenarioID)
					<< "timestamp" << std::int64_t((Slot.DateTime.GetTicks() - FDateTime(1970, 1, 1).GetTicks()) / ETimespan::TicksPerMicrosecond)
					<< "description" << TCHAR_TO_UTF8(*Slot.Description)
					<< "bin" << bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, (uint32_t)ObjectBytes.Num(), ObjectBytes.GetData()}
					<< finalize,
				Opts
			);
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::SaveLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	Future.Wait();
	return ScenarioID;
}

bool FMongoDBGetway::LoadLevelData(int64 ScenarioID, TArray<uint8>& OutObjectBytes)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, ScenarioID, &OutObjectBytes]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadLevelData(); MongoDB isn't connected"));
			return false;
		}

		mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
		mongocxx::collection Collectioin = DB[COLLECTION_NAME_SCENARIOS];

		try
		{
			auto Value = Collectioin.find_one(document{} << "_id" << std::int64_t(ScenarioID) << finalize);
			if (!Value)
			{
				return false;
			}

			auto Bin = Value->view()["bin"].get_binary();
			OutObjectBytes = TArray<uint8>(Bin.bytes, Bin.size);
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}

		return true;
	});

	return Future.Get();
}

bool FMongoDBGetway::DeleteLevelData(int64 ScenarioID)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	if (ScenarioID < 0)
	{
		return false;
	}

	auto Future = AsyncTask([this, ScenarioID]()
	{
		try
		{
			mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_SCENARIOS];

			auto Res = Collectioin.delete_one(document{} << "_id" << std::int64_t(ScenarioID) << finalize);
			return Res->deleted_count() > 0;
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::DeleteLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return false;
	});

	return Future.Get();
}

bool FMongoDBGetway::LoadLevelList(const FString& LevelName, bool bSortByDateTime, TArray<FLevelStateSlotDescription>& OutSlots)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	OutSlots.Empty();

	auto Future = AsyncTask([this, &OutSlots, bSortByDateTime, LevelName]()
	{
		if (!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadLevelList(); MongoDB isn't connected"));
			return false;
		}

		try
		{
			mongocxx::database DB = (**GetGameThreadClient())[TCHAR_TO_UTF8(*GetDatabaseName())];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_SCENARIOS];

			mongocxx::options::find Opts;
			if(bSortByDateTime) Opts.sort(document{} << "timestamp" << -1 << finalize);
			Opts.allow_partial_results(true);
			Opts.projection(document{} << "bin" << 0 << finalize);

			for (auto& It : Collectioin.find({}, Opts))
			{
				FLevelStateSlotDescription & SlotRef = OutSlots.Add_GetRef(FLevelStateSlotDescription());
				SlotRef.ScenarioID = It["_id"].get_int64();
				SlotRef.SlotIndex = -1;
				SlotRef.Description = It["description"].get_string().value.to_string().c_str();
				SlotRef.DateTime = FDateTime(1970, 1, 1) + FTimespan(It["timestamp"].get_int64() * ETimespan::TicksPerMicrosecond);
				SlotRef.LevelName = LevelName;
				SlotRef.SlotSource = ELeveSlotSource::Remote;
			}
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBGetway::LoadLevelList(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

} // namespace soda