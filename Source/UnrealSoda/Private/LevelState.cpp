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

#define TRANSIENT_SLOT TEXT("LevelState_Transient")

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

void ULevelSaveGame::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.GetError())
	{
		UE_LOG(LogSoda, Error, TEXT("ULevelSaveGame::Serialize(); FArchive is in error state: %i"), Ar.IsCriticalError());
	}

	Ar << LevelDataRecord;
}

FLevelStateSlotDescription ALevelState::StaticSlot{};

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
	SaveGame->Description = Slot.Description;
	SaveGame->DateTime = Slot.DateTime;
	SaveGame->LevelName = Slot.LevelName;
	SaveGame->LevelDataRecord.SerializeActor(this, false);

	return SaveGame;
}

bool ALevelState::SaveLevelToTransientSlot()
{
	Slot.LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

	ULevelSaveGame* SaveGame = CreateSaveGame(false);
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelToTransientSlot(); Can't CreateSaveGame()"));
		return false;
	}

	if (!UGameplayStatics::SaveGameToSlot(SaveGame, TRANSIENT_SLOT, 0))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelToTransientSlot(); Can't SaveGameToSlot()"));
		return false;
	}

	return true;
}

bool ALevelState::SaveLevelLocallyAs(int SlotIndex, const FString& Description)
{
	if (SlotIndex > 255)
	{
		return false;
	}

	if (SlotIndex < 0)
	{
		for (int i = 0; i <= 255; ++i)
		{
			if (!UGameplayStatics::DoesSaveGameExist(GetLocalSaveSlotName(this, i), 0))
			{
				SlotIndex = i;
				break;
			}
		}

		if (SlotIndex < 0)
		{
			UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelLocallyAs(); Run out of free slots"));
			return false;
		}
	}

	Slot.Description = Description;
	Slot.SlotIndex = SlotIndex;
	Slot.ScenarioID = -1;
	Slot.DateTime = FDateTime::Now();
	Slot.LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	Slot.SlotSource = ELeveSlotSource::Local;

	ULevelSaveGame* SaveGame = CreateSaveGame(false);
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelLocallyAs(); Can't CreateSaveGame()"));
		return false;
	}

	if (!UGameplayStatics::SaveGameToSlot(SaveGame, GetLocalSaveSlotName(this, SlotIndex), 0))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelLocallyAs(); Can't SaveGameToSlot()"));
		return false;
	}

	bIsDirty = false;
	StaticSlot = Slot;
	return true;
}

bool ALevelState::SaveLevelRemotlyAs(int64 ScenarioID, const FString& Description)
{
	/*
	ULevelSaveGame* SaveGame = CreateSaveGame(false);
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelRemotlyAs(); Can't CreateSaveGame()"));
		return false;
	}

	TArray<uint8> ObjectBytes;
	if (!UGameplayStatics::SaveGameToMemory(SaveGame, ObjectBytes))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::SaveLevelRemotlyAs(); Can't SaveGameToMemory()"));
		return false;
	}

	FLevelStateSlotDescription NewSlot;
	NewSlot.Description = Description;
	NewSlot.ScenarioID = ScenarioID;
	NewSlot.SlotIndex = -1;
	NewSlot.DateTime = FDateTime::Now();
	NewSlot.LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);;
	NewSlot.SlotSource = ELeveSlotSource::Remote;

	ScenarioID = soda::FDBGateway::Instance().SaveLevelData(NewSlot, ObjectBytes);

	if (ScenarioID >=0)
	{
		NewSlot.ScenarioID = ScenarioID;
		StaticSlot = Slot = NewSlot;
		bIsDirty = false;
		return true;
	}
	else
	{
		return false;
	}
	*/ 
	return false;
}

bool ALevelState::ReSaveLevel()
{
	switch (Slot.SlotSource)
	{
	case ELeveSlotSource::Local:
		return Slot.SlotIndex >= 0 ? SaveLevelLocallyAs(Slot.SlotIndex, Slot.Description) : false;
	case ELeveSlotSource::Remote:
		return Slot.ScenarioID >= 0 ? SaveLevelRemotlyAs(Slot.ScenarioID, Slot.Description) : false;
	default:
		return false;
	}
}

ULevelSaveGame* ALevelState::LoadSaveGameLocally(const UObject* WorldContextObject, int & SlotIndex, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;

	if (SlotIndex < 0)
	{
		TArray<FLevelStateSlotDescription> Slots;
		GetLevelSlotsLocally(WorldContextObject, Slots, true, LevelName);
		if (Slots.Num())
		{
			SlotIndex = Slots[0].SlotIndex;
		}
	}
	if (SlotIndex >= 0)
	{
		return Cast<ULevelSaveGame>(UGameplayStatics::LoadGameFromSlot(GetLocalSaveSlotName(WorldContextObject, SlotIndex, LevelName), 0));
	}
	else
	{
		return nullptr;
	}
}

ULevelSaveGame* ALevelState::LoadSaveGameTransient(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	return Cast<ULevelSaveGame>(UGameplayStatics::LoadGameFromSlot(TRANSIENT_SLOT, 0));
}

ULevelSaveGame* ALevelState::LoadSaveGameRemotly(const UObject* WorldContextObject, int64 & ScenarioID, const FString& InLevelName)
{
	/*
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;

	if (ScenarioID < 0)
	{
		TArray<FLevelStateSlotDescription> Slots;
		GetLevelSlotsRemotly(WorldContextObject, Slots, true, LevelName);
		if (Slots.Num())
		{
			ScenarioID = Slots[0].SlotIndex;
		}
		else
		{
			return nullptr;
		}
	}

	TArray<uint8> ObjectBytes;
	if (!soda::FDBGateway::Instance().LoadLevelData(ScenarioID, ObjectBytes))
	{
		return nullptr;
	}

	return Cast<ULevelSaveGame>(UGameplayStatics::LoadGameFromMemory(ObjectBytes));
	*/
	return nullptr;
}

FString ALevelState::GetLocalSaveSlotName(const UObject* WorldContextObject, int SlotIndex, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;
	//LevelName.ReplaceCharInline('/', '_');
	//LevelName.ReplaceCharInline('\\', '_');
	return FString(TEXT("LevelState")) + TEXT("_") + LevelName + TEXT("_") + FString::FromInt(SlotIndex);
}


bool ALevelState::DeleteLevelLocally(const UObject* WorldContextObject, int SlotIndex, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;

	return UGameplayStatics::DeleteGameInSlot(GetLocalSaveSlotName(WorldContextObject, SlotIndex, LevelName), 0);
}

bool ALevelState::DeleteLevelRemotly(const UObject* /*WorldContextObject*/, int64 ScenarioID)
{
	//return soda::FDBGateway::Instance().DeleteLevelData(ScenarioID);
	return false;
}

bool ALevelState::GetLevelSlotsLocally(const UObject* WorldContextObject, TArray<FLevelStateSlotDescription>& Slots, bool bSortByDateTime, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	Slots.Empty();

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;

	for (int i = 0; i < 255; ++i)
	{
		if (UGameplayStatics::DoesSaveGameExist(GetLocalSaveSlotName(WorldContextObject, i, LevelName), 0))
		{
			if (ULevelSaveGame* SaveGameSlot = LoadSaveGameLocally(WorldContextObject, i, LevelName))
			{
				FLevelStateSlotDescription Slot;
				Slot.SlotIndex = i;
				Slot.ScenarioID = -1;
				Slot.Description = SaveGameSlot->Description;
				Slot.DateTime = SaveGameSlot->DateTime;
				Slot.LevelName = LevelName;
				Slot.SlotSource = ELeveSlotSource::Local;
				Slots.Add(Slot);
			}
		}
	}

	if (bSortByDateTime)
	{
		Slots.Sort([](const FLevelStateSlotDescription& LHS, const FLevelStateSlotDescription& RHS) { return LHS.DateTime > RHS.DateTime; });
	}

	return true;
}

bool ALevelState::GetLevelSlotsRemotly(const UObject* WorldContextObject, TArray<FLevelStateSlotDescription>& Slots, bool bSortByDateTime, const FString& InLevelName)
{
	/*
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}
	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;
	return soda::FDBGateway::Instance().LoadLevelList(LevelName, bSortByDateTime, Slots);
	*/
	return false;
}

bool ALevelState::ReloadLevelEmpty(const UObject* WorldContextObject, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;
	StaticSlot.LevelName = LevelName;
	StaticSlot.SlotIndex = -1;
	StaticSlot.ScenarioID = -1;
	StaticSlot.SlotSource = ELeveSlotSource::NewSlot;
	UGameplayStatics::OpenLevel(WorldContextObject, *LevelName, false);
	return true;
}


bool ALevelState::ReloadLevelFromSlotLocally(const UObject* WorldContextObject, int SlotIndex, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;
	StaticSlot.LevelName = LevelName;
	StaticSlot.SlotIndex = SlotIndex;
	StaticSlot.ScenarioID = -1;
	StaticSlot.SlotSource = ELeveSlotSource::Local;
	UGameplayStatics::OpenLevel(WorldContextObject, *LevelName, false);
	return true;
}

bool ALevelState::ReloadLevelFromSlotRemotly(const UObject* WorldContextObject, int64 ScenarioID, const FString& InLevelName)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(WorldContextObject, true) : InLevelName;
	StaticSlot.LevelName = LevelName;
	StaticSlot.SlotIndex = -1;
	StaticSlot.ScenarioID = ScenarioID;
	StaticSlot.SlotSource = ELeveSlotSource::Remote;
	UGameplayStatics::OpenLevel(WorldContextObject, *LevelName, false);
	return true;
}

ALevelState* ALevelState::CreateOrLoad(const UObject* WorldContextObject, UClass* DefaultClass, bool bFromTransientSlot)
{
	if (!WorldContextObject)
	{
		WorldContextObject = USodaStatics::GetGameWorld();
	}

	UWorld* World = WorldContextObject->GetWorld();
	check(World);

	ULevelSaveGame* LevelSaveGame = nullptr;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(WorldContextObject, true);

	if (StaticSlot.LevelName != CurrentLevelName)
	{
		StaticSlot.SlotIndex = -1;
	}

	if (bFromTransientSlot)
	{
		LevelSaveGame = LoadSaveGameTransient(WorldContextObject);
		if (IsValid(LevelSaveGame))
		{
			if (LevelSaveGame->LevelName != CurrentLevelName)
			{
				UE_LOG(LogSoda, Error, TEXT("ALevelState::CreateOrLoad(); Transient slot LevelName mismatch, loaded name \"%s\", current name \"%s\""), *LevelSaveGame->LevelName, *CurrentLevelName);
				LevelSaveGame = nullptr;
			}
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ALevelState::CreateOrLoad(); Can't load SavaGame for TransientSlot"));
		}
	}
	else
	{
		switch (StaticSlot.SlotSource)
		{
		case ELeveSlotSource::NoSlot:
			StaticSlot.SlotIndex = -1;
			LevelSaveGame = LoadSaveGameLocally(WorldContextObject, StaticSlot.SlotIndex);
			StaticSlot.SlotSource = ELeveSlotSource::Local;
			break;
		case ELeveSlotSource::NewSlot:
			StaticSlot.SlotIndex = -1;
			StaticSlot.SlotSource = ELeveSlotSource::NoSlot;
			break;
		case ELeveSlotSource::Local:
			LevelSaveGame = LoadSaveGameLocally(WorldContextObject, StaticSlot.SlotIndex);
			StaticSlot.SlotSource = ELeveSlotSource::Local;
			break;
		case ELeveSlotSource::Remote:
			LevelSaveGame = LoadSaveGameRemotly(WorldContextObject, StaticSlot.ScenarioID);
			StaticSlot.SlotSource = ELeveSlotSource::Remote;
			break;
		}
	}

	auto It = TActorIterator<ALevelState>(World);
	ALevelState* LevelState = It ? *It : nullptr;

	if (LevelSaveGame)
	{
		if (LevelState)
		{
			LevelSaveGame->LevelDataRecord.DeserializeActor(LevelState, false);
		}
		else
		{
			LevelState = LevelSaveGame->LevelDataRecord.SpawnActor<ALevelState>(World, false);
		}
	}

	if (!LevelState)
	{
		if (DefaultClass)
		{
			LevelState = World->SpawnActor<ALevelState>(DefaultClass);
		}
		else
		{
			LevelState = World->SpawnActor<ALevelState>();
		}
	}

	check(LevelState)

	if (LevelSaveGame)
	{
		LevelState->Slot.Description = LevelSaveGame->Description;
		LevelState->Slot.DateTime = LevelSaveGame->DateTime;
		LevelState->Slot.LevelName = LevelSaveGame->LevelName;
		LevelState->Slot.SlotIndex = StaticSlot.SlotIndex;
		LevelState->Slot.ScenarioID = StaticSlot.ScenarioID;
		LevelState->Slot.SlotSource = StaticSlot.SlotSource;
	}
	else
	{
		StaticSlot = LevelState->Slot = FLevelStateSlotDescription();
	}

	StaticSlot = LevelState->Slot;
	return LevelState;
}

void ALevelState::FinishLoadLevel()
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
}

void ALevelState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
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