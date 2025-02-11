// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBSource.h"
#include "Soda/MongoDB/MongoDBGateway.h"
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

#define COLLECTION_NAME "files"


namespace soda
{

static UClass* GetClassFromString(const FString& ClassName)
{
	if (ClassName.IsEmpty() || ClassName == TEXT("None"))
	{
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!FPackageName::IsShortPackageName(ClassName))
	{
		Class = FindObject<UClass>(nullptr, *ClassName);
	}
	else
	{
		Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None, ELogVerbosity::Warning, TEXT("GetClassFromString()"));
	}
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}

bsoncxx::types::b_binary GuidToBin(const FGuid & Guid)
{
	return bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, static_cast<std::uint32_t>(16), reinterpret_cast<const std::uint8_t*>(&Guid) };
}

FGuid BinToGuid(const bsoncxx::types::b_binary& Bin)
{
	FGuid Guid;
	if (Bin.size != sizeof(FGuid))
	{
		throw std::runtime_error("Guid != 16");
	}
	FMemory::Memcpy((void*)&Guid, (void*)Bin.bytes, sizeof(FGuid));
	return Guid;
}

FFileDatabaseSlotInfo ToSlotInfo(const bsoncxx::v_noabi::document::view & DocView)
{
	FFileDatabaseSlotInfo Slot;

	Slot.GUID = BinToGuid(DocView["_id"].get_binary());
	Slot.Type = static_cast<EFileSlotType>(DocView["type"].get_int32().value);
	Slot.Lable = UTF8_TO_TCHAR(DocView["lable"].get_string().value.to_string().c_str());
	Slot.Description = UTF8_TO_TCHAR(DocView["description"].get_string().value.to_string().c_str());
	Slot.DataClass = GetClassFromString(UTF8_TO_TCHAR(DocView["class"].get_string().value.to_string().c_str()));
	Slot.DateTime = FDateTime::FromUnixTimestamp(DocView["last_modified"].get_int64().value);
	Slot.DataMD5Hash = BinToGuid(DocView["hash"].get_binary());
	Slot.SourceVersion = EFileSlotSourceVersion::RemoteOnly;
	return Slot;
}

FName FMongoDBSource::GetSourceName() const
{
	static FName SourceName("ManogDB");
	return SourceName;
}

TSet<EFileSlotType> FMongoDBSource::SupportedTypes() const
{
	static TSet<EFileSlotType> Types = { EFileSlotType::Actor, EFileSlotType::Level, EFileSlotType::Vehicle, EFileSlotType::VehicleComponent };
	return Types;
}

bool FMongoDBSource::PullSlotsInfo(TMap<FGuid, FFileDatabaseSlotInfo>& OutSlots, EFileSlotType Type) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(true);

	auto Future = MongoDBGetway.AsyncTask([this, &MongoDBGetway, &OutSlots, &Type]()
	{
		if (!MongoDBGetway.IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PullSlotsInfo(); MongoDB isn't connected"));
			return false;
		}

		try
		{
			mongocxx::collection Collectioin = MongoDBGetway.GetDB()[COLLECTION_NAME];
			//mongocxx::options::find Opts;
			//if(bSortByDateTime) Opts.sort(document{} << "timestamp" << -1 << finalize);
			//Opts.allow_partial_results(true);
			//Opts.projection(document{} << "bin" << 0 << finalize);

			for (auto& It : Collectioin.find(document{} << "type" << static_cast<int>(Type) << finalize))
			{
				FFileDatabaseSlotInfo Slot = ToSlotInfo(It);
				OutSlots.Emplace(Slot.GUID, MoveTemp(Slot));
			}
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PullSlotsInfo(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

bool FMongoDBSource::DeleteSlot(const FGuid& Guid)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);

	auto Future = MongoDBGetway.AsyncTask([this, &MongoDBGetway, &Guid]()
	{
		try
		{
			mongocxx::collection Collectioin = MongoDBGetway.GetDB()[COLLECTION_NAME];
			auto Res = Collectioin.delete_one(document{} << "_id" << GuidToBin(Guid) << finalize);
			return Res->deleted_count() > 0;
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::DeleteSlot(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return false;
	});

	return Future.Get();
}

bool FMongoDBSource::PullSlot(const FGuid& Guid, FFileDatabaseSlotInfo& OutInfo, TArray<uint8>& OutData)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);

	auto Future = MongoDBGetway.AsyncTask([this, &MongoDBGetway, &Guid, &OutInfo, &OutData]()
	{
		if(!MongoDBGetway.IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PullSlot(); MongoDB isn't connected"));
			return false;
		}

		try
		{
			mongocxx::collection Collectioin = MongoDBGetway.GetDB()[COLLECTION_NAME];
			auto Value = Collectioin.find_one(document{} << "_id" << GuidToBin(Guid) << finalize);
			if (!Value)
			{
				UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PullSlot();  Slot with given GUID isn't found"));
				return false;
			}

			OutInfo = ToSlotInfo(*Value);
			OutData.SetNum(Value->view()["data"].get_binary().size);
			FMemory::Memcpy(OutData.GetData(), Value->view()["data"].get_binary().bytes, OutData.Num());
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PullSlot(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}

		return true;
	});

	return Future.Get();
}

bool FMongoDBSource::PushSlot(const FFileDatabaseSlotInfo& Info, const TArray<uint8>& Data)
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;

	FMongoDBGetway& MongoDBGetway = FMongoDBGetway::Get(false);

	auto Future = MongoDBGetway.AsyncTask([this, &MongoDBGetway, &Info, &Data]()
	{
		if (!MongoDBGetway.IsConnected())
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PushSlot(); MongoDB isn't connected"));
			return false;
		}

		mongocxx::collection Collectioin = MongoDBGetway.GetDB()[COLLECTION_NAME];

		try
		{
			auto Id = GuidToBin(Info.GUID);
			mongocxx::options::replace Opts;
			Opts.upsert(true);
			Collectioin.replace_one(
				document{} << "_id" << Id << finalize,
				document{}
					<< "_id" << Id
				    << "type" << static_cast<int>(Info.Type)
					<< "lable" << TCHAR_TO_UTF8(*Info.Lable)
					<< "description" << TCHAR_TO_UTF8(*Info.Description)
					<< "class" << TCHAR_TO_UTF8(Info.DataClass ? *Info.DataClass->GetClassPathName().ToString() : TEXT(""))
					<< "last_modified" << Info.DateTime.ToUnixTimestamp()
					<< "hash" << GuidToBin(Info.DataMD5Hash)
					<< "data" << bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, static_cast<std::uint32_t>(Data.Num()), reinterpret_cast<const std::uint8_t*>(Data.GetData()) }
				<< finalize,
				Opts
			);
		}
		catch (const std::system_error& e)
		{
			UE_LOG(LogSoda, Error, TEXT("FMongoDBSource::PushSlot(); MongoDB error: \"%s\""), UTF8_TO_TCHAR(e.what()));
			return false;
		}
		return true;
	});

	return Future.Get();
}

} // namespace soda