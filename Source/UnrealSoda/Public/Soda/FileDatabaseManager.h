// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/SodaTypes.h"
#include "Templates/SharedPointer.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

class FSQLiteDatabase;

namespace soda
{
	class IFileDatabaseSource;
	class FFileDatabaseStatement;

	struct FFileDatabaseImpl
	{
		~FFileDatabaseImpl();
		TUniquePtr<FSQLiteDatabase> Database;
		TUniquePtr<FFileDatabaseStatement> Statements;
	};

	enum class  EFileSlotSourceVersion: uint8
	{
		/** No attempts were made to check the availability of this slot on the source server */
		NotChecked,

		/** An attempt was made to find the slot in the source server, but could not be found. Most likely the slot has been removed from the sources server */
		LocalOnly, 

		/** The slot is on the source server, but not locally */
		RemoteOnly,

		/** The source server has a newer slot */
		LocalIsNewer,

		/** Local slot is newer than on the source server */
		RemoteIsNewer,

		/** Local and remote slot are the same */
		Synchronized,
	};

	enum class EFileSlotType: uint8
	{
		Vehicle = 0,
		VehicleComponent,
		Level,
		Actor,
	};

	struct FFileDatabaseSlotInfo
	{
		FGuid GUID;
		EFileSlotType Type;
		FString Label;
		FString Description; // Optional 
		TObjectPtr<UClass> DataClass; // Optional
		FString DataClassName;
		FString JsonDescription; // Optional
		FDateTime DateTime;
		FGuid DataMD5Hash;

		EFileSlotSourceVersion SourceVersion = EFileSlotSourceVersion::NotChecked;
		//TSharedPtr<FFileDatabaseImpl> Database;

		bool IsEmpty() const { return !DataMD5Hash.IsValid(); }

		inline bool operator==(const FFileDatabaseSlotInfo& Other) const
		{
			return
				GUID == Other.GUID &&
				Type == Other.Type &&
				DataClass == Other.DataClass &&
				Label == Other.Label &&
				Description == Other.Description &&
				DataMD5Hash == Other.DataMD5Hash &&
				DateTime == Other.DateTime;
		}

		inline bool operator!=(const FFileDatabaseSlotInfo& Other) const
		{
			return !(*this == Other);
		}
	};

	class IFileDatabaseSource
	{
	public:
		virtual FName GetSourceName() const = 0;
		virtual TSet<EFileSlotType> SupportedTypes() const = 0;
		virtual bool PullSlotsInfo(TMap<FGuid, FFileDatabaseSlotInfo> & OutSlots, EFileSlotType Type) const = 0;
		virtual bool DeleteSlot(const FGuid& Guid) = 0;
		virtual bool PullSlot(const FGuid& Guid, FFileDatabaseSlotInfo & OutInfo, TArray<uint8>& OutData) = 0;
		virtual bool PushSlot(const FFileDatabaseSlotInfo& Info, const TArray<uint8>& Data) = 0;
	};

	class FFileDatabaseManager : public TSharedFromThis<FFileDatabaseManager, ESPMode::ThreadSafe>
	{
	public:
		FFileDatabaseManager() = default;

		void RegisterSource(TSharedRef<IFileDatabaseSource> Source);
		void UnregisterSource(FName SourceName);
		const TMap<FName, TSharedPtr<IFileDatabaseSource>>& GetSources() const { return Sources; }

		TMap<FGuid, TSharedPtr<FFileDatabaseSlotInfo>> GetSlots(EFileSlotType Type, bool bRescanFiles = true, FName SourceName = NAME_None);
		bool GetSlot(const FGuid& Guid, FFileDatabaseSlotInfo& Info);

		void ScanFiles();

		/**
		 * if Info.GUID is not valid will create new GUID.
		 * Info.DataMD5Hash is ignored
		 * Info.DateTime  is ignored, used now() insted
		 */ 
		bool AddOrUpdateSlotInfo(const FFileDatabaseSlotInfo & Info);
		bool DeleteSlot(const FGuid & Guid);
		bool GetSlotData(const FGuid& Guid, TArray<uint8>& OutData);
		bool UpdateSlotData(const FGuid& Guid, const TArray<uint8>& Data);

	protected:
		TUniquePtr<FSQLiteDatabase> OpenDatabase(const FString & FilePath);

	protected:
		TArray<TSharedPtr<FFileDatabaseImpl>> DatabasesImpl;
		TSharedPtr<FFileDatabaseImpl> DefaultDatabasesImpl;
		//TMap<FGuid, TSharedPtr<const FFileDatabaseSlotInfo>> Slots;
		TMap<FName, TSharedPtr<IFileDatabaseSource>> Sources;
	};

} // namespace soda