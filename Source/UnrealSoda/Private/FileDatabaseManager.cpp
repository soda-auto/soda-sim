// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/FileDatabaseManager.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "SQLiteDatabase.h"
#include "Misc/Paths.h"

namespace soda
{
static UClass* GetClassFromString(const FString& ClassName)
{
	if(ClassName.IsEmpty() || ClassName == TEXT("None"))
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
	if(!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}


class FFileDatabaseStatement
{
private:
	FSQLiteDatabase& Database;

public:
	explicit FFileDatabaseStatement(FSQLiteDatabase& InDatabase)
		: Database(InDatabase)
	{
		check(Database.IsValid());
	}

	bool CreatePreparedStatements()
	{
		check(Database.IsValid());

		#define PREPARE_STATEMENT(VAR)																				\
			(VAR) = Database.PrepareStatement<decltype(VAR)>(ESQLitePreparedStatementFlags::Persistent);	\
			if (!(VAR).IsValid()) { return false; }

		PREPARE_STATEMENT(Statement_GetSlotsInfo);
		PREPARE_STATEMENT(Statement_GetSlotInfo);
		PREPARE_STATEMENT(Statement_GetSlotData);
		PREPARE_STATEMENT(Statement_UpdateSlotInfo);
		PREPARE_STATEMENT(Statement_UpdateSlotData);
		PREPARE_STATEMENT(Statement_AddSlot);
		PREPARE_STATEMENT(Statement_DeleteSlot);

		#undef PREPARE_STATEMENT

		return true;
	}

	SQLITE_PREPARED_STATEMENT(FGetSlotsInfo,
		"SELECT guid, type, lable, description, class, json_description, last_modified, hash FROM table_files WHERE type = ?1;",
		SQLITE_PREPARED_STATEMENT_COLUMNS(FGuid, EFileSlotType, FString, FString, FString, FString, FDateTime, FGuid),
		SQLITE_PREPARED_STATEMENT_BINDINGS(EFileSlotType)
	);

	private: FGetSlotsInfo Statement_GetSlotsInfo;
	public: bool GetSlotsInfo(EFileSlotType Type, TFunctionRef<ESQLitePreparedStatementExecuteRowResult(FFileDatabaseSlotInfo&&)> InCallback)
	{
		return Statement_GetSlotsInfo.BindAndExecute(Type, [&InCallback](const FGetSlotsInfo& InStatement) //-V562
		{
			FFileDatabaseSlotInfo Slot;
			FString DataClassName;
			if (InStatement.GetColumnValues(Slot.GUID, Slot.Type, Slot.Lable,  Slot.Description, DataClassName, Slot.JsonDescription, Slot.DateTime, Slot.DataMD5Hash))
			{
				Slot.DataClass = GetClassFromString(DataClassName);
				return InCallback(MoveTemp(Slot));
			}
			return ESQLitePreparedStatementExecuteRowResult::Error;
		}) != INDEX_NONE;
	}

	SQLITE_PREPARED_STATEMENT(FGetSlotInfo,
		"SELECT type, lable, description, class, json_description, last_modified, hash FROM table_files WHERE guid = ?1;",
		SQLITE_PREPARED_STATEMENT_COLUMNS(EFileSlotType, FString, FString, FString, FString, FDateTime, FGuid),
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid)
	);

	private: FGetSlotInfo Statement_GetSlotInfo;
	public: bool GetSlotInfo(const FGuid & Guid, FFileDatabaseSlotInfo & Slot)
	{
		FString JsonDescription_Dummy;
		FString DataClassName;
		if (Statement_GetSlotInfo.BindAndExecuteSingle(Guid, Slot.Type, Slot.Lable, Slot.Description, DataClassName, Slot.JsonDescription, Slot.DateTime, Slot.DataMD5Hash))
		{
			Slot.GUID = Guid;
			Slot.DataClass = GetClassFromString(DataClassName);
			return true;
		}
		return false;
	}

	SQLITE_PREPARED_STATEMENT(FGetSlotData,
		"SELECT data FROM table_files WHERE guid = ?1;",
		SQLITE_PREPARED_STATEMENT_COLUMNS(TArray<uint8>),
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid)
	);
	private: FGetSlotData Statement_GetSlotData;
	public: bool GetSlotData(const FGuid& Guid, TArray<uint8> & SlotData)
	{
		if (Statement_GetSlotData.BindAndExecuteSingle( Guid, SlotData))
		{
			return true;
		}
		return false;
	}

	SQLITE_PREPARED_STATEMENT_BINDINGS_ONLY(
		FUpdateSlotInfo,
		" UPDATE table_files SET type = ?2, lable = ?3, description = ?4, class = ?5, json_description = ?6, last_modified = ?7 WHERE guid = ?1;",
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid, EFileSlotType, FString, FString, FString, FString, FDateTime)
	);
	private: FUpdateSlotInfo Statement_UpdateSlotInfo;

	SQLITE_PREPARED_STATEMENT_BINDINGS_ONLY(
		FUpdateSlotData,
		" UPDATE table_files SET hash = ?2, data = ?3, last_modified = ?4 WHERE guid = ?1;",
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid, FGuid, TArray<uint8>, FDateTime)
	);
	private: FUpdateSlotData Statement_UpdateSlotData;

	public: bool UpdateSlotData(const FGuid & Guid, const FDateTime & UpdateTime, const FGuid& MD5Hash, const TArray<uint8> & Data)
	{
		return Statement_UpdateSlotData.BindAndExecute(Guid, MD5Hash, Data, UpdateTime);
	}

	SQLITE_PREPARED_STATEMENT_BINDINGS_ONLY(
		FAddSlot,
		" INSERT INTO table_files(guid, type, lable, description, class, json_description, last_modified, hash)"
		" VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);",
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid, EFileSlotType, FString, FString, FString, FString, FDateTime, FGuid)
	);
	private: FAddSlot Statement_AddSlot;

	public: bool AddOrUpdateSlotInfo(const FFileDatabaseSlotInfo& Slot)
	{
		FFileDatabaseSlotInfo FoundSlot;
		if (GetSlotInfo(Slot.GUID, FoundSlot))
		{
			if (!Statement_UpdateSlotInfo.BindAndExecute(Slot.GUID, Slot.Type, Slot.Lable, Slot.Description, Slot.DataClass ? *Slot.DataClass->GetClassPathName().ToString() : TEXT(""), Slot.JsonDescription, Slot.DateTime))
			{
				return false;
			}
		}
		else
		{
			if (!Statement_AddSlot.BindAndExecute(Slot.GUID, Slot.Type, Slot.Lable, Slot.Description, Slot.DataClass ? *Slot.DataClass->GetClassPathName().ToString() : TEXT(""), Slot.JsonDescription, Slot.DateTime, FGuid{}))
			{
				return false;
			}
		}
		return true;
	}

	SQLITE_PREPARED_STATEMENT_BINDINGS_ONLY(
		FDeleteSlot,
		" DELETE FROM table_files WHERE guid = ?1;",
		SQLITE_PREPARED_STATEMENT_BINDINGS(FGuid)
	);
	private: FDeleteSlot Statement_DeleteSlot;

	public: bool DeleteSlot(const FGuid & Guid)
	{
		return Statement_DeleteSlot.BindAndExecute(Guid);
	}
};

// ---------------------------------------------------------------------------------------------------------------

FFileDatabaseImpl::~FFileDatabaseImpl()
{
	Statements.Reset();
	if (Database.IsValid())
	{
		Database->Close();
	}
	Database.Reset();
}

// ---------------------------------------------------------------------------------------------------------------

void FFileDatabaseManager::RegisterSource(TSharedRef<IFileDatabaseSource> Source)
{
	Sources.Add(Source->GetSourceName(), Source);
}

void FFileDatabaseManager::UnregisterSource(FName SourceName)
{
	Sources.Remove(SourceName);
}

void FFileDatabaseManager::ScanFiles()
{
	static const FString DBDir = FPaths::ProjectSavedDir() / TEXT("SODA");
	static const FString DBExtension = TEXT(".ssdb");
	static const FString DefaultDB = TEXT("Default") + DBExtension;

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *DBDir, *DBExtension);

	if (!Files.FindByPredicate([](const auto& It) { return FPaths::IsSamePath(It, DefaultDB); }))
	{
		Files.Add(DefaultDB);
	}

	for (auto& File : Files)
	{
		auto FullFilePath = DBDir / File;

		TSharedPtr<FFileDatabaseImpl> DatabaseImpl;

		if (auto* FoundDatabaseImpl = DatabasesImpl.FindByPredicate([&FullFilePath](const auto& It) { return FPaths::IsSamePath(It->Database->GetFilename(), FullFilePath); }))
		{
			DatabaseImpl = (*FoundDatabaseImpl);
		}
		else
		{
			DatabaseImpl = MakeShared<FFileDatabaseImpl>();
			DatabaseImpl->Database = OpenDatabase(FullFilePath);

			if (!DatabaseImpl->Database)
			{
				continue;
			}

			DatabaseImpl->Statements = MakeUnique<FFileDatabaseStatement>(*DatabaseImpl->Database.Get());
			if (!DatabaseImpl->Statements->CreatePreparedStatements())
			{
				soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), TEXT("Can't create statements"));
				continue;
			}
		}

		if (!DatabaseImpl.IsValid() || !DatabaseImpl->Database->IsValid())
		{
			continue;
		}

		DatabasesImpl.Add(DatabaseImpl);

		if (FPaths::IsSamePath(File, DefaultDB))
		{
			DefaultDatabasesImpl = DatabaseImpl;
		}
	}
}

TMap<FGuid, TSharedPtr<FFileDatabaseSlotInfo>> FFileDatabaseManager::GetSlots(EFileSlotType Type, bool bRescanFiles, FName SourceName)
{
	TMap<FGuid, TSharedPtr<FFileDatabaseSlotInfo>> OutSlots;

	if (bRescanFiles)
	{
		ScanFiles();
	}

	for (auto& It : DatabasesImpl)
	{
		bool bRes = It->Statements->GetSlotsInfo(Type, [this, &OutSlots](FFileDatabaseSlotInfo&& InSlot)
		{
			auto Slot = MakeShared<FFileDatabaseSlotInfo>(MoveTemp(InSlot));
			Slot->SourceVersion = EFileSlotSourceVersion::LocalOnly;
			OutSlots.Emplace(InSlot.GUID, MoveTemp(Slot));
			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});

		if (!bRes)
		{
			soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), *It->Database->GetLastError());
			continue;
		}
	}

	TMap<FGuid, FFileDatabaseSlotInfo> SourceSlots;

	if (!SourceName.IsNone())
	{
		if (auto* FoundSource = Sources.Find(SourceName))
		{
			if ((*FoundSource)->SupportedTypes().Contains(Type))
			{
				if (!(*FoundSource)->PullSlotsInfo(SourceSlots, Type))
				{
					soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), TEXT("Can't pull slots from\"%s\""), *SourceName.ToString());
				}
			}
		}
	}

	for (auto& RemoteSlot : SourceSlots)
	{
		auto* LocalSlotIt = OutSlots.Find(RemoteSlot.Key);

		if (LocalSlotIt)
		{
			FFileDatabaseSlotInfo* LocalSlot = const_cast<FFileDatabaseSlotInfo*>((*LocalSlotIt).Get());

			if (*LocalSlot == RemoteSlot.Value)
			{
				LocalSlot->SourceVersion = EFileSlotSourceVersion::Synchronized;
			}
			else if (RemoteSlot.Value.DateTime > LocalSlot->DateTime)
			{
				LocalSlot->SourceVersion = EFileSlotSourceVersion::RemoteIsNewer;
			}
			else
			{
				LocalSlot->SourceVersion = EFileSlotSourceVersion::LocalIsNewer;
			}
		}
		else
		{
			RemoteSlot.Value.SourceVersion = EFileSlotSourceVersion::RemoteOnly;
			OutSlots.Add(RemoteSlot.Key, MakeShared<FFileDatabaseSlotInfo>(RemoteSlot.Value));
		}
	}

	return OutSlots;
}

bool FFileDatabaseManager::GetSlot(const FGuid& Guid, FFileDatabaseSlotInfo & Slot)
{
	for (auto& It : DatabasesImpl)
	{
		if (It->Statements->GetSlotInfo(Guid, Slot))
		{
			return true;
		}
	}
	return false;
}

TUniquePtr<FSQLiteDatabase> FFileDatabaseManager::OpenDatabase(const FString& FilePath)
{
	TUniquePtr<FSQLiteDatabase> Database = MakeUnique<FSQLiteDatabase>();

	if (!Database->Open(*FilePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), TEXT("Can't open %s database"), *FilePath);
		return {};
	}

	//Database->Execute(TEXT("PRAGMA journal_mode=WAL;"));
	//Database->Execute(TEXT("PRAGMA synchronous=FULL;"));
	//Database->Execute(TEXT("PRAGMA cache_size=1000;"));
	//Database->Execute(TEXT("PRAGMA page_size=65535;"));
	//Database->Execute(TEXT("PRAGMA locking_mode=EXCLUSIVE;"));

	if (!Database->PerformQuickIntegrityCheck())
	{
		soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), TEXT("Database failed integrity check."));
		Database->Close();
		return {};
	}

	if (!ensure(Database->Execute(TEXT("CREATE TABLE IF NOT EXISTS table_files(guid BLOB PRIMARY KEY, type INT, lable TEXT, description TEXT, class TEXT, json_description TEXT, last_modified INTEGER, hash BLOB NOT NULL, data BLOB);"))))
	{
		soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), *Database->GetLastError());
		Database->Close();
		return {};
	}

	if (!ensure(Database->Execute(TEXT("CREATE INDEX IF NOT EXISTS type_index ON table_files(type);"))))
	{
		soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), *Database->GetLastError());
		return {};
	}

	return Database;
}

bool FFileDatabaseManager::AddOrUpdateSlotInfo(const FFileDatabaseSlotInfo& InSlot)
{
	if (!DefaultDatabasesImpl)
	{
		return false;
	}

	FFileDatabaseSlotInfo Slot = InSlot;
	Slot.GUID = Slot.GUID.IsValid() ? Slot.GUID : FGuid::NewGuid();
	Slot.DateTime = FDateTime::Now();

	if (!DefaultDatabasesImpl->Statements->AddOrUpdateSlotInfo(Slot))
	{
		soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), *DefaultDatabasesImpl->Database->GetLastError());
		return false;
	}

	return true;
}

bool FFileDatabaseManager::DeleteSlot(const FGuid & Guid)
{
	for (auto& It : DatabasesImpl)
	{
		FFileDatabaseSlotInfo DummySlot;
		if (It->Statements->GetSlotInfo(Guid, DummySlot))
		{
			return It->Statements->DeleteSlot(Guid);
		}
	}
	return false;
}

bool FFileDatabaseManager::GetSlotData(const FGuid& Guid, TArray<uint8>& OutData)
{
	for (auto& It : DatabasesImpl)
	{
		if (It->Statements->GetSlotData(Guid, OutData))
		{
			return true;
		}
	}
	return false;
}

bool FFileDatabaseManager::UpdateSlotData(const FGuid& Guid, const TArray<uint8>& Data)
{
	bool bRes = false;
	for (auto& It : DatabasesImpl)
	{
		FFileDatabaseSlotInfo DummySlot;
		if (It->Statements->GetSlotInfo(Guid, DummySlot))
		{
			FMD5 MD5;
			MD5.Update(Data.GetData(), Data.Num());
			FMD5Hash Hash;
			Hash.Set(MD5);
			if (!It->Statements->UpdateSlotData(Guid, FDateTime::Now(), MD5HashToGuid(Hash), Data))
			{
				soda::ShowNotificationAndLog(ENotificationLevel::Error, 5.0, ANSI_TO_TCHAR(__FUNCTION__), *DefaultDatabasesImpl->Database->GetLastError());
				return false;
			}
			return true;
		}
	}

	return false;
}



} // namespace soda