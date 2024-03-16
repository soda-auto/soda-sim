// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/SodaTypes.h"
#include "SodaActorFactory.generated.h"

struct FPinnedActorRecord
{
	TSoftClassPtr<AActor> Class;
	FString SlotName;
	FTransform Transform;
	FName DesireName;

	friend FArchive& operator<<(FArchive& Ar, FPinnedActorRecord& Record)
	{
		Ar << Record.Class;
		Ar << Record.SlotName;
		Ar << Record.Transform;
		Ar << Record.DesireName;
		return Ar;
	}

	inline FString ToString() const
	{
		return FString::Printf(TEXT("SlotName=(%s), DesireName=(%s), Class=(%s)"), *SlotName, *DesireName.ToString(), *Class.ToString());
	}
};

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ASodaActorFactory : public AActor
{
	GENERATED_BODY()

public:
	/** Called if any actor was addwd or removed or renamed */
	DECLARE_MULTICAST_DELEGATE(FInvalidateEvent);
	FInvalidateEvent OnInvalidateDelegate;

public:
	/** ActorClass must be ISodaActor interface */
	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	AActor * SpawnActor(TSubclassOf<AActor> ActorClass , FTransform Transform);

	/** Actor must be ISodaActor interface */
	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	void AddActor(AActor * Actor);

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	bool CheckActorIsExist(const AActor* Actor) const { return SpawnedActors.Find(const_cast<AActor*>(Actor)) != INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	const TArray<AActor*> & GetActors() const { return SpawnedActors; }

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	void RemoveAllActors();

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	void SpawnAllSavedActors(bool bSaved=true, bool bPinned=true);

	/** NewActor and PreviewActor must be ISodaActor interface */
	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	bool ReplaceActor(AActor * NewActor, AActor * PreviewActor, bool bPushActorIfNotFound);

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	bool RenameActor(AActor* Actor, const FString& NewName);

	UFUNCTION(BlueprintCallable, Category = ActorFactory)
	bool RemoveActor(AActor* Actor, bool bAndDestroy = true);

	void SetSerializePinnedActorsAsUnpinned(bool bVal) { bSerializePinnedActorsAsUnpinned = bVal; }

public:
	virtual void Serialize(FArchive& Ar) override;

protected:
	TArray<FActorRecord> LoadedActorRecords;
	TArray<FPinnedActorRecord> LoadedPinnedActors;

	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	bool bSerializePinnedActorsAsUnpinned = false;
};
