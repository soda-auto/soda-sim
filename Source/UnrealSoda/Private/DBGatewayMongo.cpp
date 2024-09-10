// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/DBGatewayMongo.h"
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

#define COLLECTION_NAME_VEHICLES "vehicles"
#define COLLECTION_NAME_DATASETES "datasets"
#define COLLECTION_NAME_SCENARIOS "scenarios"
#define COLLECTION_NAME_DATASETE_PREFIX "dataset"

namespace soda
{

FDBGateway& FDBGateway::Instance()
{
	static FDBGatewayMongo Instance;
	return Instance;
}

FDBGatewayMongo::FDBGatewayMongo()
{
}

FDBGatewayMongo::~FDBGatewayMongo()
{
	if (MongoTaskManagerThread)
	{
		MongoTaskManagerThread->Kill();
		delete MongoTaskManagerThread;
	}
	MongoTaskManagerThread = nullptr;
}

TFuture<bool> FDBGatewayMongo::AsyncTask(TUniqueFunction < bool()> Function, bool bSync)
{
	if (!MongoTaskManagerThread)
	{
		UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::AsyncTask(); FDBGatewayMongo isn't configured"));
		SetLastError("FDBGatewayMongo isn't configured");
		TPromise<bool> Promise;
		Promise.SetValue(false);
		return Promise.GetFuture();
	}
	return soda::AsyncTask(MongoTaskManager, MoveTemp(Function), bSync);
}

void FDBGatewayMongo::Configure(const FString& URL, const FString& InDatabaseName, bool bSync)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	if (Status == EDBGatewayStatus::Connecting)
	{
		return;
	}

	Status = EDBGatewayStatus::Connecting;

	if (!Inst)
	{
		Inst = MakeUnique<mongocxx::instance>();
	}

	if (!MongoTaskManagerThread)
	{
		MongoTaskManagerThread = FRunnableThread::Create(&MongoTaskManager, TEXT("MongoTaskManager"));
		if (MongoTaskManagerThread == nullptr)
		{
			UE_LOG(LogSoda, Fatal, TEXT("FDBGatewayMongo::AsyncTask(); Can't create MongoTaskManager"));
		}
	}

	MongoTaskManager.ClearQueue();
	soda::AsyncTaskLoop(MongoTaskManager, [this]() { Tick_MongoThread(); });

	auto Future = AsyncTask([this, InDatabaseName, URL]()
	{
		Disable_MongoThread();
		Pool = MakeUnique<mongocxx::pool>(mongocxx::uri{ TCHAR_TO_UTF8(*URL) });
		GameThreadClient = Pool->try_acquire();
		MongoThreadClient = Pool->try_acquire();
		DatabaseName = InDatabaseName;

		try
		{
			if (MongoThreadClient && *MongoThreadClient && **MongoThreadClient)
			{
				mongocxx::database DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
				bsoncxx::document::value RetVal = DB.run_command(document{} << "ping" << 1 << finalize);
				Status = EDBGatewayStatus::Connected;
				return true;
			}
			else
			{
				Status = EDBGatewayStatus::Faild;
				return false;
			}
		}
		catch (const std::system_error& e)
		{
			LastError = FString("Can't connect to server; MongoDb error: ") + UTF8_TO_TCHAR(e.what());
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::Configure(); %s"), *LastError);
			Status = EDBGatewayStatus::Faild;
			::AsyncTask(ENamedThreads::GameThread, [LastError = LastError]() 
			{
				USodaSubsystem::GetChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "MongoDB Faild", *LastError);
			});
			return false;
		}
	});

	if (bSync)
	{
		Future.Get();
	}
}

void FDBGatewayMongo::Disable_MongoThread()
{
	FScopeLock ScopeLock(&DatasetsLock);
	Datasets.Empty();
	GameThreadClient.reset();
	MongoThreadClient.reset();
	Pool.Reset();
	DatabaseName = "";
	bIsDatasetRecording = false;
	DatasetID = -1;
	DatasetCollectionName = "";
	LastError = "";
	bIsDatasetRecordingStalled = false;
}

void FDBGatewayMongo::Disable()
{
	auto Future = AsyncTask([this]()
	{
		Status = EDBGatewayStatus::Disabled;
		Disable_MongoThread();
		return true;
	});

	Future.Get();
}

mongocxx::pool::entry FDBGatewayMongo::GetConnection()
{
	return Pool->acquire();
}

bsoncxx::stdx::optional<mongocxx::pool::entry> FDBGatewayMongo::TryGetConnection()
{
	return Pool->try_acquire();
}

bool FDBGatewayMongo::BeginRecordDataset()
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	auto Future = AsyncTask([this]()
	{
		LastError = "";
		bIsDatasetRecordingStalled = false;

		if (Status != EDBGatewayStatus::Connected)
		{
			LastError = "The MongoDB isn't connected";
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::BeginRecordDataset(); %s"), *LastError);
			return false;
		}

		{
			FScopeLock ScopeLock(&DatasetsLock);
			if (Datasets.Num())
			{
				LastError = "There is an unfinished session(s) from the previous dataset recording";
				UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::BeginRecordDataset(); %s"), *LastError);
				return false;
			}
		}

		mongocxx::database DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
		mongocxx::collection Collectioin = DB[COLLECTION_NAME_DATASETES];

		DatasetID = 0;
		try
		{
			mongocxx::options::find Opts;
			Opts.sort(document{} << "_id" << -1 << finalize);
			Opts.limit(1);
			auto Cursor = Collectioin.find({}, Opts);
			if (Cursor.begin() != Cursor.end())
			{
				auto Doc = *Cursor.begin();
				DatasetID = Doc["_id"].get_int64() + 1;
			}

			Collectioin.insert_one((document{}
				<< "_id" << std::int64_t(DatasetID)
				<< "begin_ts" << std::int64_t(soda::NowRaw< std::chrono::system_clock, std::chrono::microseconds>())
				<< "unreal_soda_ver" << TCHAR_TO_UTF8(UNREALSODA_VERSION_STRING)
				<< "actors" << open_array << close_array
				<< finalize
			).view());
		}
		catch (const std::system_error& e)
		{
			LastError = FString("The \"find\" request failed on collection \"datasets\";  MongoDB error: ") + UTF8_TO_TCHAR(e.what());
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::BeginRecordDataset(); %s"), *LastError);
			return false;
		}

		DatasetCollectionName = FString(COLLECTION_NAME_DATASETE_PREFIX) + FString::FromInt(DatasetID);

		UE_LOG(LogSoda, Log, TEXT("FDBGatewayMongo::BeginRecordDataset(); Begin recording dataset; ID: %i"), DatasetID);

		bIsDatasetRecording = true;
		return true;
	});

	return Future.Get();
}

void FDBGatewayMongo::EndRecordDataset(EScenarioStopReason Reasone, bool bImmediately)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, Reasone, bImmediately]()
	{
		if(DatasetID >= 0 && bIsDatasetRecording)
		{
			mongocxx::database DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_DATASETES];
			try
			{
				Collectioin.update_one(
					document{} << "_id" << std::int64_t(DatasetID) << finalize,
					document{} << "$set" 
						<< open_document
						<< "end_ts" << std::int64_t(soda::NowRaw< std::chrono::system_clock, std::chrono::microseconds>())
						<< "end_reason" << TCHAR_TO_UTF8 (*ScenarioStopReasonToString(Reasone))
						<< close_document
					<< finalize
				);
			}
			catch (const std::system_error& e)
			{
				SetLastError(FString("The \"update_one\" request failed on collection \"datasets\";  MongoDB error: ") + UTF8_TO_TCHAR(e.what()));
				UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::EndRecordDataset(); %s"), *GetLastError());
			}
		}

		bIsDatasetRecording = false;
		DatasetID = -1;
		DatasetCollectionName.Empty();

		if (bImmediately || IsDatasetRecordingStalled())
		{
			ClearDatasetsQueue();
		}

		return true;
	});

	Future.Get();
}

void FDBGatewayMongo::Tick_MongoThread()
{
	using bsoncxx::builder::stream::finalize;

	TArray<TSharedPtr<FActorDatasetData>> CopyDatasets;
	{
		FScopeLock ScopeLock(&DatasetsLock);
		CopyDatasets = Datasets;
	}

	if (CopyDatasets.Num())
	{
		for (auto& It : CopyDatasets)
		{
			if (!It->IsInvalidate())
			{
				if (TSharedPtr<bsoncxx::builder::stream::document> Doc = It->Dequeue())
				{
					mongocxx::database DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
					mongocxx::collection Collectioin = DB[TCHAR_TO_UTF8(*DatasetCollectionName)];

					try
					{
						Collectioin.insert_one((*Doc << finalize).view());
					}
					catch (const std::system_error& e)
					{
						SetLastError(FString("Can't insert dociment to \"") + DatasetCollectionName + "\" collection; MongoDB error: " + UTF8_TO_TCHAR(e.what()));
						UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::PushAsync(); %s"), *GetLastError());
						It->SetIsInvalidate();
					}
				}
			}

			if (It->IsInvalidate())
			{
				StallDatasetRecording();
			}

			if (It->IsInvalidate() || !IsDatasetRecording())
			{
				FScopeLock ScopeLock(&DatasetsLock);
				Datasets.Remove(It);
			}
		}
	}
}

TSharedPtr<FActorDatasetData> FDBGatewayMongo::CreateActorDataset(const FString& ActorName, const FString& ActorType, const FString& ActorClassName, const bsoncxx::builder::stream::document& Description)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	TSharedPtr<FActorDatasetData> Dataset;
	auto Future = AsyncTask([this, ActorName, ActorType, ActorClassName, &Description, &Dataset]()
	{
		if (GetStatus() == EDBGatewayStatus::Connected && IsDatasetRecording() && !bIsDatasetRecordingStalled)
		{
			Dataset = MakeShared<FActorDatasetData>(ActorName);
			FScopeLock ScopeLock(&DatasetsLock);
			Datasets.Add(Dataset);

			mongocxx::database DB = (**MongoThreadClient)[TCHAR_TO_UTF8(*DatabaseName)];
			mongocxx::collection Collectioin = DB[COLLECTION_NAME_DATASETES];

			try
			{
				Collectioin.update_one(
					document{} << "_id" << std::int64_t(DatasetID) << finalize,
					document{} << "$push" 
					<< open_document << "actors"
					<< open_document 
					<< "name" << TCHAR_TO_UTF8(*ActorName)
					<< "type" << TCHAR_TO_UTF8(*ActorType)
					<< "class" << TCHAR_TO_UTF8(*ActorClassName)
					<< "description" << Description
					<< close_document
					<< close_document
					<< finalize
				);
			}
			catch (const std::system_error& e)
			{
				SetLastError(FString("Can't insert dociment to \"") + DatasetCollectionName + "\" collection; MongoDB error: " + UTF8_TO_TCHAR(e.what()));
				UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::CreateActorDataset(); %s"), *GetLastError());
				return false;
			}
			return true;

		}
		return false;
	});

	Future.Get();
	return Dataset;
}

bool FDBGatewayMongo::SaveVehicleData(const FString& VehicleName, const bsoncxx::document::value& Data)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, VehicleName, &Data]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::SaveVehicleData(); MongoDB isn't connected"));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::SaveVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

bsoncxx::document::view FDBGatewayMongo::LoadVehicleData(const FString& VehicleName)
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadVehicleData(); MongoDB isn't connected"));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}

		return true;
	});

	Future.Wait();// Get();
	return View;
}

bool FDBGatewayMongo::DeleteVehicleData(const FString& VehicleName)
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::DeleteVehicleData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return false;
	});

	return Future.Get();
}

bool FDBGatewayMongo::LoadVehiclesList(TArray<FVechicleSaveAddress>& Addresses)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, &Addresses]()
	{
		if (!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadVehiclesList(); MongoDB isn't connected"));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadVehiclesList(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

int64 FDBGatewayMongo::SaveLevelData(const FLevelStateSlotDescription & Slot, const TArray<uint8> & ObjectBytes)
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::SaveLevelData(); MongoDB isn't connected"));
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
				UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::SaveLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::SaveLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	Future.Wait();
	return ScenarioID;
}

bool FDBGatewayMongo::LoadLevelData(int64 ScenarioID, TArray<uint8>& OutObjectBytes)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	auto Future = AsyncTask([this, ScenarioID, &OutObjectBytes]()
	{
		if(!IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadLevelData(); MongoDB isn't connected"));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}

		return true;
	});

	return Future.Get();
}

bool FDBGatewayMongo::DeleteLevelData(int64 ScenarioID)
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::DeleteLevelData(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return false;
	});

	return Future.Get();
}

bool FDBGatewayMongo::LoadLevelList(const FString& LevelName, bool bSortByDateTime, TArray<FLevelStateSlotDescription>& OutSlots)
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadLevelList(); MongoDB isn't connected"));
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
			UE_LOG(LogSoda, Error, TEXT("FDBGatewayMongo::LoadLevelList(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

} // namespace soda