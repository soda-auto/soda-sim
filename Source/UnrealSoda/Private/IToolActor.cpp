// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/IToolActor.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/UnrealSoda.h"
#include "UI/Wnds/SPinToolActorWindow.h"
#include "Soda/SodaSubsystem.h"

#define TOOL_ACTOR_SLOT_PREFIX "Pinned"

bool IToolActor::PinActor(const FString& SlotName)
{
	if (FindPinnedActorBySlotName(AsActor()->GetWorld(), SlotName))
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::PinActor(); Actor with same SloatName is already exist"));
		return false;
	}
	ToolActorSlotName = SlotName;
	return true;
}

void IToolActor::UnpinActor()
{
	ToolActorSlotName = "";
}

bool IToolActor::IsPinnedActor() const
{
	return ToolActorSlotName.Len() > 0;
}

FString IToolActor::GetPinnedActorSlotName() const
{
	return ToolActorSlotName;
}

FString IToolActor::GetPinnedActorName() const
{
	return ToolActorSlotName;
}

bool IToolActor::OnSetPinnedActor(bool bIsPinnedActor)
{
	if (bIsPinnedActor)
	{
		if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
		{
			SodaSubsystem->OpenWindow(FString::Printf(TEXT("Pin \"%s\" Actor"), *AsActor()->GetName()), SNew(soda::SPinToolActorWindow, this));
		}
	}
	else
	{
		UnpinActor();
	}
	return true;
}

bool IToolActor::SavePinnedActor() 
{ 
	if (!AsActor())
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::SavePinnedActor(); Forgot implement AsActor()?"));
		return false;
	}

	if (!IsPinnedActor())
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::SavePinnedActor(); IsPinnedActor() == false"));
		return false;
	}

	UPinnedToolActorsSaveGame* SaveGame = GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::SavePinnedActor(); Can't load SaveGame"));
		return false;
	}

	FActorRecord & ActorRecord = SaveGame->ActorRecords.FindOrAdd(ToolActorSlotName);
	if (!ActorRecord.SerializeActor(AsActor(), false))
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::SavePinnedActor(); Serialize actor"));
		return false;
	}

	if (SaveAll())
	{
		ClearDirty();
		return true;
	}

	return false;
}

bool IToolActor::DeserializePinnedActor(const FString& SlotName)
{
	if (!AsActor())
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::DeserializePinnedActor(); Forgot implement AsActor()?"));
		return false;
	}

	UPinnedToolActorsSaveGame* SaveGame = GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::DeserializePinnedActor(); Can't load SaveGame"));
		return false;
	}

	if (FindPinnedActorBySlotName(AsActor()->GetWorld(), SlotName))
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::DeserializePinnedActor(); Actor with same SloatName is already exist"));
		return false;
	}

	for (auto& It : SaveGame->ActorRecords)
	{
		if (It.Key == SlotName)
		{
			if (It.Value.Class != AsActor()->GetClass())
			{
				UE_LOG(LogSoda, Error, TEXT("IToolActor::DeserializePinnedActor(); Slot class mismatch"));
				return false;
			}
			if (It.Value.DeserializeActor(AsActor(), false, true))
			{
				ToolActorSlotName = SlotName;
				return true;
			}
			return false;
		}
	}
	return false;
}

AActor* IToolActor::LoadPinnedActor(UWorld* World, const FTransform& Transform, const FString& SlotName, bool bForceCreate, FName DesireName) const
{
	check(World);

	if (!AsActor())
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::LoadPinnedActor(); Forgot implement AsActor()?"));
		return nullptr;
	}

	if (FindPinnedActorBySlotName(AsActor()->GetWorld(), SlotName))
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::LoadPinnedActor(); Actor with same SloatName is already exist"));
		return nullptr;
	}

	UPinnedToolActorsSaveGame* SaveGame = GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::LoadPinnedActor(); Can't load SaveGame"));
		return nullptr;
	}

	AActor* Actor = nullptr;
	for (auto& It : SaveGame->ActorRecords)
	{
		if (It.Key == SlotName)
		{
			if (It.Value.Class != AsActor()->GetClass())
			{
				UE_LOG(LogSoda, Error, TEXT("IToolActor::LoadPinnedActor(); Slot class mismatch"));
				return nullptr;
			}
			It.Value.Name = DesireName;
			Actor = It.Value.SpawnActor<AActor>(World, false);
		}
	}

	if (!Actor)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::LoadPinnedActor(); Can't find slot '%s' for actor class '%s'"), *SlotName, *AsActor()->GetClass()->GetName());
		return nullptr;
	}

	if (!Actor && bForceCreate)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnInfo.Name = DesireName.IsNone() ? NAME_None : FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), AsActor()->GetClass(), DesireName);
		Actor = World->SpawnActor(AsActor()->GetClass(), &Transform, SpawnInfo);
	}

	if (Actor)
	{
		if (IToolActor* ActorTool = Cast<IToolActor>(Actor))
		{
			ActorTool->ToolActorSlotName = SlotName;
		}
	}

	return Actor;
}

UPinnedToolActorsSaveGame* IToolActor::GetSaveGame() const
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		// Try find savegamae
		UPinnedToolActorsSaveGame *& SaveGame = SodaSubsystem->PinnedToolActorsSaveGame.FindOrAdd(AsActor()->GetClass());
		if (SaveGame)
		{
			if (SaveGame->Class != AsActor()->GetClass())
			{
				SaveGame->Class = AsActor()->GetClass();
				UE_LOG(LogSoda, Warning, TEXT("IToolActor::GetSaveGame(); Found slot class mismatch"));
			}
			return SaveGame;
		} 

		// Try load savagame
		const FString SlotName = FString(TOOL_ACTOR_SLOT_PREFIX) + "_" + AsActor()->GetClass()->GetName();
		SaveGame = Cast<UPinnedToolActorsSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (SaveGame)
		{
			if (SaveGame->Class != AsActor()->GetClass())
			{
				SaveGame->Class = AsActor()->GetClass();
				UE_LOG(LogSoda, Warning, TEXT("IToolActor::GetSaveGame(); Loaded slot class mismatch"));
			}
			return SaveGame;
		}

		// Create savegame
		SaveGame = NewObject< UPinnedToolActorsSaveGame>(SodaSubsystem);
		SaveGame->Class = AsActor()->GetClass();
		return SaveGame;
	}
	return nullptr;
}

IToolActor* IToolActor::FindPinnedActorBySlotName(const UWorld* World, const FString& SlotName) const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, AsActor()->GetClass(), Actors);
	for (auto& It : Actors)
	{
		if (IToolActor* ActorTool = Cast<IToolActor>(It))
		{
			if (ActorTool->IsPinnedActor() && ActorTool->GetPinnedActorSlotName() == SlotName)
			{
				return ActorTool;
			}
		}
	}
	return nullptr;
}

bool IToolActor::DeleteSlot(const UWorld* World, const FString& SlotName) const
{
	UPinnedToolActorsSaveGame* SaveGame = GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::DeleteSlot(); Can't load SaveGame"));
		return false;
	}

	if (SaveGame->ActorRecords.Remove(SlotName) <= 0)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::DeleteSlot(); Can't can't find '%s' slot name"), *SlotName);
		return false;
	}

	SaveAll();

	return true;
}

bool IToolActor::IToolActor::SaveAll() const
{
	UPinnedToolActorsSaveGame* SaveGame = GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogSoda, Error, TEXT("IToolActor::SaveAll(); Can't load SaveGame"));
		return false;
	}

	const FString SlotName = FString(TOOL_ACTOR_SLOT_PREFIX) + "_" + AsActor()->GetClass()->GetName();
	return UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);
}

void IToolActor::ToolActorSerialize(FArchive& Ar)
{  
	if (Ar.IsSaveGame())
	{
		Ar << ToolActorSlotName;
	}
}