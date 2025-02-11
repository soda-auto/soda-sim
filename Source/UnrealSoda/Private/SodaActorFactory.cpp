// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaActorFactory.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "UObject/UObjectHash.h"
#include "Blueprint/WidgetTree.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaApp.h"
#include "Soda/ISodaActor.h"
#include "EngineUtils.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/LevelState.h"
#include "Soda/SodaDelegates.h"

void ASodaActorFactory::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(Ar.IsSaveGame())
	{
		if (Ar.IsSaving())
		{
			TArray<FActorRecord> ActorRecords;
			TArray<FPinnedActorRecord> PinnedActors;
			for (auto& Actor : SpawnedActors)
			{
				if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
				{
					ISodaActor* SodaActor = Cast<ISodaActor>(Actor);
					if (!bSerializePinnedActorsAsUnpinned && SodaActor && SodaActor->GetSlotGuid().IsValid())
					{
						FPinnedActorRecord & Record = PinnedActors.Add_GetRef(FPinnedActorRecord());
						Record.DesireName = Actor->GetFName();
						Record.Class = Actor->GetClass();
						Record.Slot = SodaActor->GetSlotGuid();
						Record.Transform = Actor->GetActorTransform();
					}
					else
					{
						FActorRecord Record;
						if (Record.SerializeActor(Actor, true))
						{
							ActorRecords.Add(Record);
						}
					}
				}
			}
			Ar << ActorRecords;
			Ar << PinnedActors;
		}
		else if (Ar.IsLoading())
		{
			Ar << LoadedActorRecords;
			Ar << LoadedPinnedActors;
		}
	}
}

AActor* ASodaActorFactory::SpawnActor(TSubclassOf<AActor> ActorClass, FTransform Transform)
{
	UWorld* World = GetWorld();
	check(World);

	if (!ActorClass)
	{
		return nullptr;
	}

	if (!ActorClass->ImplementsInterface(USodaActor::StaticClass()))
	{
		return nullptr;
	}

	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		const FSodaActorDescriptor& Desc = SodaSubsystem->GetSodaActorDescriptor(ActorClass.Get());
		Transform.AddToTranslation(Desc.SpawnOffset);
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.bDeferConstruction = true;
	//SpawnInfo.Name = FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), ActorClass);
	AActor* NewActor = World->SpawnActor(ActorClass, &Transform, SpawnInfo);

	if (IsValid(NewActor))
	{
		NewActor->FinishSpawning(Transform);
		SpawnedActors.Add(NewActor);
		FSodaDelegates::ActorsMapChenaged.Broadcast();
		ALevelState::GetChecked()->MarkAsDirty();
		return NewActor;
	}

	return nullptr;
}

void ASodaActorFactory::AddActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		FSodaDelegates::ActorsMapChenaged.Broadcast();
		SpawnedActors.Add(Actor);
	}
}

void ASodaActorFactory::RemoveAllActors()
{
	for (auto& Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
	FSodaDelegates::ActorsMapChenaged.Broadcast();
}

void ASodaActorFactory::SpawnAllSavedActors(bool bSaved, bool bPinned)
{
	RemoveAllActors();

	UWorld* World = GetWorld();
	check(World);

	int ErrorNum = 0;
	const int MaxNotifyError = 10;

	auto ShowError = [&ErrorNum, MaxNotifyError](const FString & Msg, const auto & Record)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaActorFactory::SpawnAllSavedActors(). %s %s"), *Msg, *Record.ToString());

		if (ErrorNum < MaxNotifyError)
		{
			soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Faild spawn pinned actor \"%s\""), *Record.ToString());
		}
		++ErrorNum;
	};

	if (bPinned)
	{
		if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
		{
			for (auto& Record : LoadedPinnedActors)
			{
				UClass* Class = Record.Class.Get();
				if (!Class)
				{
					Class = LoadObject<UClass>(nullptr, *Record.Class.ToString());
				}
				if (Class)
				{
					if (ISodaActor* SodaActor = Cast<ISodaActor>(Record.Class->GetDefaultObject()))
					{
						if (AActor* Actor = SodaActor->SpawnActorFromSlot(World, Record.Slot, Record.Transform, Record.DesireName))
						{
							SpawnedActors.Add(Actor);
						}
						else
						{
							ShowError(TEXT("Can't load from slot."), Record);
						}
					}
				}
				else
				{
					ShowError(TEXT("Can't find class."), Record);
				}
			}
		}
	}

	if (bSaved)
	{
		TArray<AActor*> NewActors;
		for (int i = 0; i < LoadedActorRecords.Num(); ++i)
		{
			auto& Record = LoadedActorRecords[i];
			if (Record.IsRecordValid())
			{
				UClass* Class = Record.Class.Get();
				if (!Class)
				{
					Class = LoadObject<UClass>(nullptr, *Record.Class.ToString());
				}
				if (Class)
				{
					FActorSpawnParameters SpawnInfo;
					SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					SpawnInfo.bDeferConstruction = true;
					SpawnInfo.Name = Record.Name.IsNone() ? NAME_None : FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), Class, Record.Name);
					NewActors.Add(World->SpawnActor(Class, &Record.Transform, SpawnInfo));
				}
				else
				{
					NewActors.Add(nullptr);
					ShowError(TEXT("Can't find class."), Record);
				}
			}
			else
			{
				NewActors.Add(nullptr);
				ShowError(TEXT("Record is is invalid."), Record);
			}
		}

		for (int i = 0; i < LoadedActorRecords.Num(); ++i)
		{
			auto& Record = LoadedActorRecords[i];
			AActor* NewActor = NewActors[i];
			if (Record.IsRecordValid() && IsValid(NewActor))
			{
				if (Record.DeserializeActor(NewActor, true))
				{
					NewActor->FinishSpawning(Record.Transform);
					SpawnedActors.Add(NewActor);
				}
				else
				{
					NewActor->Destroy();
					ShowError(TEXT("Faild deserialize actor."), Record);
				}
			}
		}
	}

	if (ErrorNum > MaxNotifyError)
	{
		soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Too many errors. See the log to see all"));
	}

	FSodaDelegates::ActorsMapChenaged.Broadcast();
}

bool ASodaActorFactory::ReplaceActor(AActor* NewActor, AActor* PreviewActor, bool bPushActorIfNotFound)
{
	if (SpawnedActors.Find(PreviewActor) != INDEX_NONE)
	{
		SpawnedActors.Remove(PreviewActor);
		SpawnedActors.Add(NewActor);
		FSodaDelegates::ActorsMapChenaged.Broadcast();
		return true;
	}

	if (bPushActorIfNotFound)
	{
		SpawnedActors.Add(NewActor);
		FSodaDelegates::ActorsMapChenaged.Broadcast();
		return true;
	}

	return false;
}

bool ASodaActorFactory::RenameActor(AActor * Actor, const FString & NewName)
{
	if (!CheckActorIsExist(Actor))
	{
		return false;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetName() == NewName) return false;
	}

	if (Actor->Rename(*NewName))
	{
		FSodaDelegates::ActorsMapChenaged.Broadcast();
		return true;
	}

	return false;
}

bool ASodaActorFactory::RemoveActor(AActor* Actor, bool bAndDestroy)
{
	if (SpawnedActors.Remove(Actor) == 0)
	{
		return false;
	}

	if (bAndDestroy && IsValid(Actor))
	{
		Actor->Destroy();
	}

	FSodaDelegates::ActorsMapChenaged.Broadcast();
	return true;
}