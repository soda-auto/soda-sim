// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/UnrealSoda.h"
#include "Engine/World.h"
#include "SerializationHelpers.generated.h"

USTRUCT()
struct UNREALSODA_API FBaseRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName Name;

	virtual void Serialize(FArchive &Ar)
	{
		Ar << Name;
	}

	friend FArchive &operator<<(FArchive &Ar, FBaseRecord &Record) 
	{
		Record.Serialize(Ar);
		return Ar;
	}

	virtual FString ToString() const
	{
		return FString::Printf(TEXT("RecordName=(%s)"), *Name.ToString());
	}

	virtual ~FBaseRecord() {}
};

USTRUCT()
struct UNREALSODA_API FObjectRecord : public FBaseRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSoftClassPtr<UObject> Class;

	UPROPERTY()
	TArray<uint8> Data;

	virtual void Serialize(FArchive &Ar) override;

	bool IsRecordValid() const { return !Name.IsNone() && !Class.IsNull() && Data.Num() > 0; }

	FORCEINLINE bool operator == (const UObject *Other) const 
	{
		return Name == Other->GetFName() && Class == Other->GetClass();
	}

	virtual bool SerializeObject(UObject* Object);
	virtual bool DeserializeObject(UObject* Object);

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("RecordName=(%s), Class=(%s)"), *Name.ToString(), *Class.ToString());
	}
};

USTRUCT()
struct UNREALSODA_API FComponentRecord : public FObjectRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	bool SerializeComponent(UActorComponent* Component);
	bool DeserializeComponent(UActorComponent* Component);

	virtual void Serialize(FArchive &Ar) override;
};

USTRUCT()
struct UNREALSODA_API FActorRecord : public FObjectRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	TArray<FComponentRecord> ComponentRecords;

	bool SerializeComponent(UActorComponent* Component);
	bool DeserializeComponent(UActorComponent* Component);

	bool SerializeActor(AActor* Actor, bool bWithComponents);
	bool DeserializeActor(AActor* Actor, bool bWithComponents, bool bSetTransform = false);

	virtual void Serialize(FArchive &Ar) override;

	template<typename ActorType>
	ActorType* SpawnActor(UWorld* World, bool WithComponents, const FTransform & InTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding)
	{
		if (!IsRecordValid()) return nullptr;

		UClass* ActorClass = Class.Get();
		if (!ActorClass)
		{
			ActorClass = LoadObject<UClass>(nullptr, *Class.ToString());
		}

		if (!ActorClass)
		{
			UE_LOG(LogSoda, Error, TEXT("FActorRecord::SpawnActor(); Can't find \"%s\" class"), *Class.ToString());
			return nullptr;
		}

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = CollisionHandlingOverride;
		SpawnInfo.bDeferConstruction = true;
		SpawnInfo.Name = Name.IsNone() ? NAME_None : FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), ActorClass, Name);
		ActorType * NewActor =  Cast<ActorType>(World->SpawnActor(ActorClass, &InTransform, SpawnInfo));
		if (!IsValid(NewActor))
		{
			return nullptr;
		}

		if (!DeserializeActor(NewActor, WithComponents))
		{
			NewActor->Destroy();
			return nullptr;
		}
		NewActor->FinishSpawning(InTransform);
		return IsValid(NewActor) ? NewActor : nullptr;
	}

	template<typename ActorType>
	ActorType* SpawnActor(UWorld* World, bool WithComponents, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding)
	{
		return SpawnActor<ActorType>(World, WithComponents, Transform, CollisionHandlingOverride);
	}
};

UCLASS(ClassGroup = Soda, hideCategories = ("Activation", "Actor Tick", "Actor", "Input", "Rendering", "Replication", "Socket", "Thumbnail"))
class UNREALSODA_API USaveGameActor : public USaveGame 
{
	GENERATED_BODY()

public:
	USaveGameActor() : Super() {}

	UFUNCTION(BlueprintCallable, Category = SaveActor)
	bool SerializeActor(AActor* Actor, bool bWithComponents);

	UFUNCTION(BlueprintCallable, Category = SaveActor)
	bool DeserializeActor(AActor* Actor, bool bWithComponents);

	UFUNCTION(BlueprintCallable, Category = SaveActor)
	bool SerializeComponent(UActorComponent* Component);

	UFUNCTION(BlueprintCallable, Category = SaveActor)
	bool DeserializeComponent(UActorComponent* Component);

	FActorRecord& GetActorRecord() { return ActorRecord; }

	virtual void Serialize(FArchive &Ar) override;

	FActorRecord ActorRecord;
};
