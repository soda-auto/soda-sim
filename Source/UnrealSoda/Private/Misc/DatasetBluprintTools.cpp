// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/DatasetBluprintTools.h"
#include "Soda/UnrealSoda.h"
#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"


bool UDatasetRecorder::BeginRecord(const FString& ActorDatasetName, const FString& DatasetType, const FJsonObjectWrapper& Descriptor)
{
	Dataset.Reset();

	if (soda::FDBGateway::Instance().GetStatus() != soda::EDBGatewayStatus::Connected)
	{
		return false;
	}

	FString JsonString;
	if (!Descriptor.JsonObjectToString(JsonString))
	{
		return false;
	}

	soda::FBsonDocument Doc;
	try
	{
		Doc.Document.view() = bsoncxx::from_json(bsoncxx::stdx::string_view{TCHAR_TO_UTF8(*JsonString)});
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("UDatasetRecorder::BeginRecord(); %s"), UTF8_TO_TCHAR(e.what()));
		return false;
	}

	Dataset = soda::FDBGateway::Instance().CreateActorDataset(ActorDatasetName, DatasetType, GetClass()->GetName(), *Doc);
	if (!Dataset)
	{
		return false;
	}

	return true;
}

void UDatasetRecorder::EndRecord()
{
	if (Dataset)
	{
		Dataset->PushAsync();
		Dataset.Reset();
	}
}

bool UDatasetRecorder::TickDataset(const FJsonObjectWrapper& DatasetTickData)
{
	if (!Dataset)
	{
		return false;
	}

	// TODO: dont convert DatasetTickData to string. Parse JsonObject insted
	FString JsonString;
	if (!DatasetTickData.JsonObjectToString(JsonString))
	{
		return false;
	}

	Dataset->BeginRow();

	try
	{
		Dataset->GetRowDoc().view() = bsoncxx::from_json(bsoncxx::stdx::string_view{TCHAR_TO_UTF8(*JsonString)});
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("UDatasetRecorder::TickDataset(); %s"), UTF8_TO_TCHAR(e.what()));
		Dataset->EndRow();
		return false;
	}
	Dataset->EndRow();
	return true;
}