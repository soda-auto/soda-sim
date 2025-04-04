// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/MongoDB/MongoDBGateway.h"
#include "Soda/UnrealSoda.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/Vehicles/SodaVehicle.h"
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

} // namespace soda