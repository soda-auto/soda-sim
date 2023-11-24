// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaActorFactory.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "UObject/UObjectHash.h"
#include "Blueprint/WidgetTree.h"
#include "Soda/SodaGameMode.h"
#include "Soda/ISodaActor.h"
#include "EngineUtils.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/EditorUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

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
					if (!bSerializePinnedActorsAsUnpinned && SodaActor && SodaActor->IsPinnedActor())
					{
						FPinnedActorRecord & Record = PinnedActors.Add_GetRef(FPinnedActorRecord());
						Record.DesireName = Actor->GetFName();
						Record.Class = Actor->GetClass();
						Record.SlotName = SodaActor->GetPinnedActorSlotName();
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

	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		const FSodaActorDescriptor& Desc = GameMode->GetSodaActorDescriptor(ActorClass.Get());
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
		OnInvalidateDelegate.Broadcast();
		return NewActor;
	}

	return nullptr;
}

void ASodaActorFactory::AddActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		OnInvalidateDelegate.Broadcast();
		SpawnedActors.Add(Actor);
	}
}

void ASodaActorFactory::RemoveAllActors()
{
	for (auto& Actor : SpawnedActors) Actor->Destroy();
	SpawnedActors.Empty();
	OnInvalidateDelegate.Broadcast();
}

void ASodaActorFactory::SpawnAllSavedActors(bool bSaved, bool bPinned)
{
	RemoveAllActors();

	UWorld* World = GetWorld();
	check(World);

	FSlateNotificationManager& NotificationManager = FSlateNotificationManager::Get();

	int ErrorNum = 0;
	const int MaxNotifyError = 10;

	auto ShowError = [&ErrorNum, MaxNotifyError, &NotificationManager](const FString & Msg, const auto & Record)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaActorFactory::SpawnAllSavedActors(). %s %s"), *Msg, *Record.ToString());

		if (ErrorNum < MaxNotifyError)
		{
			FNotificationInfo Info(FText::FromString(TEXT("Faild spawn pinned actor.") + Msg + TEXT(" ") + Record.ToString()));
			Info.ExpireDuration = 5.0f;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
			NotificationManager.AddNotification(Info);
		}
		++ErrorNum;
	};

	if (bPinned)
	{
		if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
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
						if (AActor* Actor = SodaActor->LoadPinnedActor(World, Record.Transform, Record.SlotName, false, Record.DesireName))
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
		FNotificationInfo Info(FText::FromString(TEXT("Too many errors. See the log to see all.")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
		NotificationManager.AddNotification(Info);
	}

	OnInvalidateDelegate.Broadcast();
}

bool ASodaActorFactory::ReplaceActor(AActor* NewActor, AActor* PreviewActor, bool bPushActorIfNotFound)
{
	if (SpawnedActors.Find(PreviewActor) != INDEX_NONE)
	{
		SpawnedActors.Remove(PreviewActor);
		SpawnedActors.Add(NewActor);
		OnInvalidateDelegate.Broadcast();
		return true;
	}

	if (bPushActorIfNotFound)
	{
		SpawnedActors.Add(NewActor);
		OnInvalidateDelegate.Broadcast();
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
		OnInvalidateDelegate.Broadcast();
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

	OnInvalidateDelegate.Broadcast();
	return true;
}