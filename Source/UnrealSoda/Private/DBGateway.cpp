// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"

namespace soda
{

FString FDBGateway::ScenarioStopReasonToString(EScenarioStopReason Reason)
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

void FDBGateway::ClearDatasetsQueue()
{
	FScopeLock ScopeLock(&DatasetsLock);
	Datasets.Empty();
}
int FDBGateway::GetDasetesQueueNum() const
{
	int Res = 0;
	FScopeLock ScopeLock(&DatasetsLock);
	for (auto& It : Datasets)
	{
		Res += It->GetQueueNum();
	}
	return Res;
}

void FDBGateway::StallDatasetRecording()
{
	bIsDatasetRecordingStalled = true;
	FScopeLock ScopeLock(&DatasetsLock);
	Datasets.Empty();
}

/*
 * FActorDatasetData
 */

FActorDatasetData::FActorDatasetData(const FString& InActorName) :
	ActorName(InActorName)
{
	DatabaseName = FDBGateway::Instance().GetDatabaseName();
	DatasetCollectionName = FDBGateway::Instance().GetDatasetCollectionName();
}

void FActorDatasetData::PushAsync()
{
	TSharedPtr<bsoncxx::builder::stream::document> Doc = MakeShared<bsoncxx::builder::stream::document>();
	*Doc << "name" << TCHAR_TO_UTF8(*ActorName)
		<< "part" << DocPart
		<< "batch" << std::move(CurrentArray);
	DocQueue.Enqueue(Doc);
	CurrentArray.clear();
	++DocPart;
	DocsPerPart = 0;
	++QueueNum;
}

void FActorDatasetData::BeginRow()
{
	if (!bIsInvalidate)
	{
		RowDoc.clear();
	}
}

void FActorDatasetData::EndRow()
{
	if (!bIsInvalidate)
	{
		CurrentArray << RowDoc;
		if (++DocsPerPart >= MaxDocsPerPart)
		{
			PushAsync();
		}
	}
}

TSharedPtr<bsoncxx::builder::stream::document> FActorDatasetData::Dequeue()
{
	using bsoncxx::builder::stream::finalize;

	if (bIsInvalidate)
	{
		return TSharedPtr<bsoncxx::builder::stream::document>();
	}

	TSharedPtr<bsoncxx::builder::stream::document> Doc;
	if (DocQueue.Dequeue(Doc))
	{
		--QueueNum;
		return Doc;
	}

	return TSharedPtr<bsoncxx::builder::stream::document>();
}

} // namespace soda