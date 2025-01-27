// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/MongoDB/MongoDBGateway.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/UnrealSoda.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/Misc/Time.h"
#include "SodaStyleSet.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FMongoDBDatasetManager"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

#define COLLECTION_NAME_DATASETE_PREFIX TEXT("dataset_")


namespace soda
{
namespace mongodb 
{

FString ScenarioStopReasonToString(EScenarioStopReason Reason)
{
	switch (Reason)
	{
	case EScenarioStopReason::UserRequest: return "user_request";
	case EScenarioStopReason::ScenarioStopTrigger: return "scenario_stop_trigger";
	case EScenarioStopReason::InnerError: return "inner_error";
	case EScenarioStopReason::QuitApplication: return "quit_application";
	default: return "undefined";
	}
}

FMongoDBDatasetManager::FMongoDBDatasetManager()
{
}

bool FMongoDBDatasetManager::BeginRecording()
{
	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(true);

	if (!MongoTask)
	{
		MongoTask = soda::AsyncTaskLoop(MongoDBGetway.GetTaskManager(), [this]() { Tick_MongoThread(); });
	}

	if (IsDatasetWriting())
	{
		OnFaild(TEXT("There is an unfinished session from the previous dataset recording"), ANSI_TO_TCHAR(__FUNCTION__), true);
		return false;
	}

	bWasFaild = false;

	UWorld * World = SodaApp.GetSodaSubsystemChecked()->GetWorld();

	TArray<TSharedPtr<FObjectDatasetMongDBHandler>> DatasetHandlersTmp;
	for (TObjectIterator<UObject> It; It; ++It)
	{
		if (It->GetWorld() == World && !It->HasAnyFlags(RF_ClassDefaultObject) && !It->GetName().StartsWith(TEXT("SKEL_")) && !It->GetName().StartsWith(TEXT("REINST_")))
		{
			if (IObjectDataset* DatasetObject = Cast<IObjectDataset>(*It))
			{
				if (DatasetObject->ShouldRecordDataset())
				{
					TArray<UClass*> FoundClasses;
					for (auto& Handler : RegistredHandlers)
					{
						if (It->IsA(Handler.Key))
						{
							FoundClasses.Add(Handler.Key);
						}
					}

					if (FoundClasses.Num())
					{
						FoundClasses.Sort([](const auto& A, const auto& B) { return A.IsChildOf(&B); });
						auto Handler = RegistredHandlers[FoundClasses[0]](*It);
						DatasetHandlersTmp.Add(Handler);
						DatasetObject->AddDatasetHandler(Handler);
					}
				}
			}
		}
	}

	auto Future = MongoDBGetway.AsyncTask([this, &MongoDBGetway, DatasetHandlersTmp=MoveTemp(DatasetHandlersTmp)]() mutable
	{
		if (MongoDBGetway.GetStatus() != EMongoDBGatewayStatus::Connected)
		{
			OnFaild(TEXT("The MongoDB isn't connected"), ANSI_TO_TCHAR(__FUNCTION__), true);
			return false;
		}

		DatasetHandlers = MoveTemp(DatasetHandlersTmp);

		auto CollectionListNames = MongoDBGetway.GetDB().list_collection_names();

		DatasetID = -1;
		for (const auto& It : CollectionListNames)
		{
			FString CollectionName = UTF8_TO_TCHAR(It.c_str());
			if (CollectionName.StartsWith(COLLECTION_NAME_DATASETE_PREFIX))
			{
				int32 CollectionID = FCString::Atoi64(*CollectionName.RightChop(FString(COLLECTION_NAME_DATASETE_PREFIX).Len()));
				FString CollectionNameChecked = FString(COLLECTION_NAME_DATASETE_PREFIX) + FString::FromInt(CollectionID);
				if (std::find(CollectionListNames.begin(), CollectionListNames.end(), TCHAR_TO_UTF8(*CollectionNameChecked)) != CollectionListNames.end())
				{
					DatasetID = FMath::Max(DatasetID, (int64)CollectionID);
				}
			}
		}
		++DatasetID;

		CollectionName = FString(COLLECTION_NAME_DATASETE_PREFIX) + FString::FromInt(DatasetID);
		Collectioin = MongoDBGetway.GetDB()[TCHAR_TO_UTF8(*CollectionName)];

		try
		{
			auto RetVal = Collectioin.insert_one((document{}
				<< "begin_ts" << std::int64_t(soda::NowRaw< std::chrono::system_clock, std::chrono::microseconds>())
				<< "unreal_soda_ver" << TCHAR_TO_UTF8(UNREALSODA_VERSION_STRING)
				<< "objects" << open_array << close_array
				<< finalize
			).view());

			DecriptorOID = RetVal->inserted_id().get_oid().value;
		}
		catch (const std::system_error& e)
		{
			OnFaild(FString::Printf(TEXT("The \"find\" request failed on collection \"%s\"; MongoDB error: %s"), *CollectionName, UTF8_TO_TCHAR(e.what())), ANSI_TO_TCHAR(__FUNCTION__), true);
			return false;
		}

		for (auto& Handler : DatasetHandlers)
		{
			document Description{};
			Handler->CreateObjectDescription(Description);
			try
			{
				Collectioin.update_one(
					document{} << "_id" << DecriptorOID << finalize,
					document{} << "$push"
					<< open_document << "objects"
					<< Description
					<< close_document
					<< finalize
				);
			}
			catch (const std::system_error& e)
			{
				OnFaild(FString::Printf(TEXT("Can't update_one() document to \"%s\" collection; MongoDB error:"), *CollectionName, UTF8_TO_TCHAR(e.what())), ANSI_TO_TCHAR(__FUNCTION__), true);
				return false;
			}
		}

		UE_LOG(LogSoda, Log, TEXT("FMongoDBDatasetManager::BeginRecording(); Begin recording dataset; ID: %i"), DatasetID);

		bIsRecordingStarted = true;

		return true;
	});

	if (!Future.Get())
	{
		return false;
	}

	return true;
}

void FMongoDBDatasetManager::EndRecording(EScenarioStopReason Reasone, bool bImmediately)
{
	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);
	MongoDBGetway.AsyncTask([this, &MongoDBGetway, Reasone, bImmediately]()
	{
		if(bIsRecordingStarted)
		{
			if (MongoDBGetway.GetStatus() == EMongoDBGatewayStatus::Connected)
			{
				for (auto& Handler : DatasetHandlers)
				{
					Handler->FinalizeBatch();
				}

				mongocxx::database DB = (**MongoDBGetway.GetMongoThreadClient())[TCHAR_TO_UTF8(*MongoDBGetway.GetDatabaseName())];
				mongocxx::collection Collectioin = DB[TCHAR_TO_UTF8(*CollectionName)];
				try
				{
					Collectioin.update_one(
						document{} << "_id" << DecriptorOID << finalize,
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
					OnFaild(FString::Printf(TEXT("\"update_one()\" failed on collection \"%s\";  MongoDB error: %s"), *CollectionName, UTF8_TO_TCHAR(e.what())), ANSI_TO_TCHAR(__FUNCTION__), true);
				}
			}
			else
			{
				OnFaild(TEXT("The MongoDB isn't connected"), ANSI_TO_TCHAR(__FUNCTION__), true);
			}
		}

		bIsRecordingStarted = false;

		if (bImmediately)
		{
			ReleaseRecording_MongoThread();
		}

		return true;

	}).Get();
}

void FMongoDBDatasetManager::ReleaseRecording_MongoThread()
{
	DatasetHandlers.Empty(); 
	DatasetID = -1;
	CollectionName.Empty();
	Collectioin = {};
	DecriptorOID = {};
}

void FMongoDBDatasetManager::Tick(float DeltaSeconds)
{
	if (IsDatasetWriting())
	{
		if (!ProgressNotification)
		{
			FNotificationInfo Info(FText::FromString(TEXT("Recording MongoDB Dataset: #") + FString::FromInt(DatasetID)));//(LOCTEXT("RecordingNotificationInfo", );
			Info.bFireAndForget = false;
			Info.FadeOutDuration = 3.0f;
			Info.ExpireDuration = 0.0f;
			Info.bUseThrobber = true;
			Info.Image = FSodaStyle::Get().GetBrush(TEXT("SodaIcons.DB.UpDownFull"));
			Info.SubText = MakeAttributeLambda([this]() { return FText::FromString(TEXT("Num objects: ") + FString::FromInt(DatasetHandlers.Num())); });
			FNotificationButtonInfo CancleButton(
				LOCTEXT("DSNotification_CancleButtonText", "Cancle"), 
				LOCTEXT("DSNotification_CancleButton_ToolTip", "Cancle record MongoDB dataset"), 
				FSimpleDelegate::CreateLambda([this]()
				{
					FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);
					EndRecording(EScenarioStopReason::UserRequest, true);
				}));
			Info.ButtonDetails.Add(MoveTemp(CancleButton));
			ProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
			if (ProgressNotification)
			{
				ProgressNotification->SetCompletionState(SNotificationItem::CS_Pending);
			}
		}
	}
	else
	{
		if (ProgressNotification)
		{
			ProgressNotification->SetCompletionState(bWasFaild ? SNotificationItem::CS_Fail : SNotificationItem::CS_Success);
			ProgressNotification->ExpireAndFadeout();
			ProgressNotification.Reset();
		}
	}
}

void FMongoDBDatasetManager::Tick_MongoThread()
{
	if (!bIsRecordingStarted && IsDatasetWriting()) // Flushing
	{
		for (auto It = DatasetHandlers.CreateIterator(); It; ++It)
		{
			if ((*It)->QueueIsEmpty())
			{
				It.RemoveCurrent();
			}
		}
	}

	if (IsDatasetWriting())
	{
		FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);

		if (MongoDBGetway.GetStatus() != EMongoDBGatewayStatus::Connected)
		{
			OnFaild(TEXT("The MongoDB isn't connected"), ANSI_TO_TCHAR(__FUNCTION__), true);
			return;
		}

		for (auto& It : DatasetHandlers)
		{
			if (TSharedPtr<bsoncxx::builder::stream::document> Doc = It->Dequeue())
			{
				try
				{
					Collectioin.insert_one((*Doc << finalize).view());
				}
				catch (const std::system_error& e)
				{
					OnFaild(FString::Printf(TEXT("Can't insert document to \"%s\" collection; MongoDB error:"), *CollectionName, UTF8_TO_TCHAR(e.what())), ANSI_TO_TCHAR(__FUNCTION__), true);
				}
			}
		}
	}
}

FText FMongoDBDatasetManager::GetToolTip() const
{
	return LOCTEXT("Menu_ToolTip", "Default SODA dataset recorder to MongoDB database");
}

FText FMongoDBDatasetManager::GetDisplayName() const
{
	return LOCTEXT("Menu_DisplayName", "MongoDB");
}

FName FMongoDBDatasetManager::GetIconName() const
{
	static FName IconName = "SodaIcons.MongoDB";
	return IconName;
}

EDatasetManagerStatus FMongoDBDatasetManager::GetStatus() const
{
	if (bWasFaild)
	{
		return EDatasetManagerStatus::Faild;
	}

	if (bIsRecordingStarted)
	{
		return EDatasetManagerStatus::Recording;
	}

	if (IsDatasetWriting())
	{
		return EDatasetManagerStatus::Fluishing;
	}

	return EDatasetManagerStatus::Standby;
}

void FMongoDBDatasetManager::ClearDatasetsQueue()
{
	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(true);
	MongoDBGetway.AsyncTask([this]() { DatasetHandlers.Empty(); return true; }).Get();
}

/*
int FMongoDBDatasetManager::GetDasetesQueueNum() const
{
	int Res = 0;
	for (auto& It : DatasetHandlers)
	{
		Res += It->GetQueueNum();
	}
	return Res;
}
*/

void FMongoDBDatasetManager::OnFaild(const FString& What, const FString& FunctionName, bool bShowMessageBox)
{
	UE_LOG(LogSoda, Error, TEXT("%s; %s"), *FunctionName, *What);

	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);
	MongoDBGetway.AsyncTask([this]() { ReleaseRecording_MongoThread(); return true; }).Get();
	bWasFaild = true;

	if (bShowMessageBox)
	{
		::AsyncTask(ENamedThreads::GameThread, [What=What]()
		{
			FNotificationInfo Msg(FText::FromString(What));
			Msg.ExpireDuration = 5.0f;
			Msg.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
			FSlateNotificationManager::Get().AddNotification(Msg);
		});
	}

}

//---------------------------------------------------------------------------------------
FObjectDatasetMongDBHandler::FObjectDatasetMongDBHandler()
	:ObjectName (TEXT("undefined"))
{
}

FObjectDatasetMongDBHandler::FObjectDatasetMongDBHandler(const UObject* Object)
{
	check(Object);

	if (Cast<AActor>(Object))
	{
		ObjectName = Object->GetName();
	}
	else if (const UActorComponent * ActorComponent = Cast<UActorComponent>(Object))
	{
		if (AActor* Owner = ActorComponent->GetOwner())
		{
			ObjectName = Owner->GetName() + TEXT(".") + ActorComponent->GetName();
		}
		else
		{
			ObjectName = ActorComponent->GetName();
		}
	}
	else
	{
		ObjectName = Object->GetPathName();
	}

	UClass* CppClass = Object->GetClass();
	while (CppClass->IsInBlueprint())
	{
		CppClass = CppClass->GetSuperClass();
	}

	ObjectClass = CppClass->GetName();
}

FObjectDatasetMongDBHandler::~FObjectDatasetMongDBHandler()
{
}

bool FObjectDatasetMongDBHandler::Sync()
{
	bool bRet = Sync(BeginBatchItem());
	EndBatchItem();
	return bRet;
}

void FObjectDatasetMongDBHandler::CreateObjectDescription(bsoncxx::builder::stream::document& Description)
{
	Description
		<< "name" << TCHAR_TO_UTF8(*ObjectName)
		//<< "type" << TCHAR_TO_UTF8(*ObjectType)
		<< "class" << TCHAR_TO_UTF8(*ObjectClass);
}

void FObjectDatasetMongDBHandler::FinalizeBatch()
{
	TSharedPtr<bsoncxx::builder::stream::document> Doc = MakeShared<bsoncxx::builder::stream::document>();
	*Doc 
		<< "name" << TCHAR_TO_UTF8(*ObjectName)
		<< "part" << DocPart
		<< "batch" << std::move(BatchArray);
	DocQueue.Enqueue(Doc);
	BatchArray.clear();
	++DocPart;
	DocsPerPart = 0;
	++QueueNum;
}

bsoncxx::builder::stream::document& FObjectDatasetMongDBHandler::BeginBatchItem()
{
	BatchItemDoc.clear();
	return BatchItemDoc;
}

void FObjectDatasetMongDBHandler::EndBatchItem()
{
	//if (IsValid())
	{
		BatchArray << BatchItemDoc;
		if (++DocsPerPart >= GetMaxBatchSize())
		{
			FinalizeBatch();
		}
	}
}

TSharedPtr<bsoncxx::builder::stream::document> FObjectDatasetMongDBHandler::Dequeue()
{

	TSharedPtr<bsoncxx::builder::stream::document> Doc;
	if (DocQueue.Dequeue(Doc))
	{
		--QueueNum;
		return Doc;
	}

	return TSharedPtr<bsoncxx::builder::stream::document>();
}


} // namespace mongodb
} // namespace sodaoda

#undef LOCTEXT_NAMESPACE