// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/LevelState.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/FileHelper.h"
#include "EngineUtils.h"
#include "GameFramework/SpectatorPawn.h"
#include "TimerManager.h"
#include "Engine/LevelStreamingPersistent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaSpectator.h"
#include "Soda/Actors/RefPoint.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/FileDatabaseManager.h"
#include "Soda/SodaDelegates.h"


void ULevelSaveGame::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.GetError())
	{
		UE_LOG(LogSoda, Error, TEXT("ULevelSaveGame::Serialize(); FArchive is in error state: %i"), Ar.IsCriticalError());
	}

	Ar << LevelDataRecord;
}

// ----------------------------------------------------------------------------------------------------------------

ALevelState* ALevelState::Get()
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		return  SodaSubsystem->LevelState;
	}
	return nullptr;
}

ALevelState* ALevelState::GetChecked()
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	check(SodaSubsystem);
	check(SodaSubsystem->LevelState);
	return SodaSubsystem->LevelState;
}

ALevelState::ALevelState()
{
	DefaultActorFactoryClass = ASodaActorFactory::StaticClass();
}

void ALevelState::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.GetError())
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::Serialize(); FArchive is in error state; IsCriticalError: %i"), Ar.IsCriticalError());
	}

	if (Ar.IsSaveGame())
	{
		Ar << ActorFactoryRecord;
		Ar << SpectatorActorRecord;

		if (Ar.IsSaving())
		{
			AdditionalActorRecords.Empty();
			for (auto& Actor : AdditionalActorsForSerialize)
			{
				FActorRecord& Record = AdditionalActorRecords.Add_GetRef(FActorRecord());
				Record.SerializeActor(Actor, true);

			}
			Ar << AdditionalActorRecords;
		}
		else if (Ar.IsLoading())
		{
			AdditionalActorRecords.Empty();
			Ar << AdditionalActorRecords;
			if (AdditionalActorRecords.Num() != AdditionalActorsForSerialize.Num())
			{
				if (AdditionalActorRecords.Num() != 0)
				{
					UE_LOG(LogSoda, Error, TEXT("ALevelState::Serialize() Additional actor records num is mismatched"));
				}
			}
			else
			{
				for (int i = 0; i < AdditionalActorRecords.Num(); ++i)
				{
					AdditionalActorRecords[i].DeserializeActor(AdditionalActorsForSerialize[i], true, true);
				}
			}
		}
	}
}

ULevelSaveGame * ALevelState::CreateSaveGame(bool bSerializePinnedActorsAsUnpinned)
{
	ASodalSpectator* SpectatorActor = nullptr;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (SodaSubsystem)
	{
		SpectatorActor = SodaSubsystem->GetSpectatorActor();
	}

	if (IsValid(SpectatorActor))
	{
		SpectatorActorRecord.SerializeActor(SpectatorActor, false);
	}

	if (IsValid(ActorFactory))
	{
		ActorFactory->SetSerializePinnedActorsAsUnpinned(bSerializePinnedActorsAsUnpinned);
		ActorFactoryRecord.SerializeActor(ActorFactory, false);
		ActorFactory->SetSerializePinnedActorsAsUnpinned(false);
	}

	ULevelSaveGame* SaveGame = NewObject<ULevelSaveGame>(this);
	check(SaveGame);
	//SaveGame->Description = Slot.Description;
	//SaveGame->DateTime = Slot.DateTime;
	SaveGame->LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	SaveGame->LevelDataRecord.SerializeActor(this, false);

	return SaveGame;
}

bool ALevelState::SaveToToMemory(TArray<uint8>& OutSaveData)
{
	//Slot.LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

	ULevelSaveGame* SaveGame = CreateSaveGame(false);
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToTransientSlot(); Can't CreateSaveGame()"));
		return false;
	}

	if (!UGameplayStatics::SaveGameToMemory(SaveGame, OutSaveData))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToTransientSlot(); Can't SaveGameToSlot()"));
		return false;
	}

	return true;
}

bool ALevelState::SaveToSlot(const FString& Label, const FString& Description, const FGuid& Guid)
{
	soda::FFileDatabaseSlotInfo SlotInfo{};
	SlotInfo.GUID = Guid.IsValid() ? Guid : FGuid::NewGuid();
	SlotInfo.Type = soda::EFileSlotType::Level;
	SlotInfo.Label = Label;
	SlotInfo.Description = Description;
	SlotInfo.DataClass = GetClass();
	if (!SerializeSlotDescriptor(UGameplayStatics::GetCurrentLevelName(GetWorld(), true), SlotInfo.JsonDescription))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToSlot(); Can't SerializeSlotDescriptor()"));
		return false;
	}

	SlotGuid = SlotInfo.GUID;
	SlotLabel = SlotInfo.Label;

	ULevelSaveGame* SaveGame = CreateSaveGame(false);
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToSlot(); Can't CreateSaveGame()"));
		return false;
	}

	TArray<uint8> SaveData;
	if (!UGameplayStatics::SaveGameToMemory(SaveGame, SaveData))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToSlot(); Can't UGameplayStatics::SaveGameToMemory()"));
		return false;
	}

	auto& Database = SodaApp.GetFileDatabaseManager();

	if (!Database.AddOrUpdateSlotInfo(SlotInfo))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToSlot(); Can't AddOrUpdateSlotInfo() "));
		return false;
	}

	if (!Database.UpdateSlotData(SlotInfo.GUID, SaveData))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveToSlot(); Can't UpdateSlotData() "));
		return false;
	}

	bIsDirty = false;

	return true;
}

bool ALevelState::Resave()
{
	if (!SlotGuid.IsValid())
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::Resave(); There is no slot for save"));
		return false;
	}

	soda::FFileDatabaseSlotInfo Slot;
	if (!SodaApp.GetFileDatabaseManager().GetSlot(SlotGuid, Slot))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::Resave(); Can't find slot "));
		return false;
	}

	return SaveToSlot(Slot.Label, Slot.Description, Slot.GUID);
}

ULevelSaveGame* ALevelState::LoadSaveGameFromSlot(const FGuid& Guid, soda::FFileDatabaseSlotInfo& SlotInfo)
{
	auto& Database = SodaApp.GetFileDatabaseManager();

	if (!Database.GetSlot(Guid, SlotInfo))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::LoadSaveGameFromSlot(); Can't find slot "));
		return nullptr;
	}

	FString LevelName;
	if (!DeserializeSlotDescriptor(SlotInfo.JsonDescription, LevelName))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::LoadSaveGameFromSlot(); DeserializeSlotDescriptor() faild"));
		return nullptr;
	}

	TArray<uint8> SavedData;
	if (!Database.GetSlotData(Guid, SavedData))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::LoadSaveGameFromSlot(); GetSlotData() faild"));
		return nullptr;
	}

	ULevelSaveGame* SaveGame = Cast<ULevelSaveGame>(UGameplayStatics::LoadGameFromMemory(SavedData));
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::LoadSaveGameFromSlot(); UGameplayStatics::LoadGameFromMemory() faild"));
		return nullptr;
	}

	return SaveGame;
}

void ALevelState::SpawnSavedActors()
{
	UWorld* World = GetWorld();
	check(World);

	/* Initialize ActorFactory*/
	if (!TActorIterator<ASodaActorFactory>(World))
	{
		ActorFactory = ActorFactoryRecord.SpawnActor<ASodaActorFactory>(World, false);
		if (ActorFactory == nullptr && DefaultActorFactoryClass)
		{
			ActorFactory = World->SpawnActor<ASodaActorFactory>(DefaultActorFactoryClass);
		}
	}
	else
	{
		ActorFactory = *TActorIterator<ASodaActorFactory>(World);
		ActorFactoryRecord.DeserializeActor(ActorFactory, false);
	}
	if (ActorFactory)
	{
		ActorFactory->SpawnAllSavedActors();
	}
}

void ALevelState::BeginPlay()
{
	Super::BeginPlay();

	ActorsMapChenagedHandle = FSodaDelegates::ActorsMapChenaged.AddLambda([this]()
	{
		MarkAsDirty();
	});
}

void ALevelState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ActorsMapChenagedHandle.IsValid())
	{
		FSodaDelegates::ActorsMapChenaged.Remove(ActorsMapChenagedHandle);
	}
}

void ALevelState::SetGeoReference(double Lat, double Lon, double Alt, const FVector& OrignShift, float OrignDYaw)
{
	LLConverter.OrignLat = Lat;
	LLConverter.OrignLon = Lon;
	LLConverter.OrignAltitude = Alt;
	LLConverter.OrignShift = OrignShift;
	LLConverter.OrignDYaw = OrignDYaw;
	LLConverter.Init();

	for (TActorIterator<ARefPoint> It(GetWorld()); It; ++It)
	{
		It->SetActorLocation(LLConverter.OrignShift);
		It->SetActorRotation(FRotator(0, LLConverter.OrignDYaw, 0));
		It->Longitude = Lon;
		It->Latitude = Lat;
		It->Altitude = Alt;
	}
}

void ALevelState::ClearLevel()
{
	if (ActorFactory)
	{
		ActorFactory->RemoveAllActors();
	}
}

bool ALevelState::SerializeSlotDescriptor(const FString& LevelName, FString& OutJsonString)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("level_name"), LevelName);
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	return FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
}

bool ALevelState::DeserializeSlotDescriptor(const FString& InJsonString, FString& OutLevelName)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InJsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("level_name"), OutLevelName))
	{
		return false;
	}

	return true;
}