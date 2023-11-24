// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/ISodaActor.h"
#include "UObject/Interface.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "IToolActor.generated.h"


/**
 * UPinnedToolActorsSaveGame
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UPinnedToolActorsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = ToolActorsSaveGame, SaveGame)
	UClass* Class;

	TMap<FString, FActorRecord> ActorRecords;

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << ActorRecords;
	}
};

/**
 * IToolActor
 * IToolActor implement the "Pinned Actor" logic inherited from "ISodaActor":
 *   - Using binary serialization 
 *   - UGameplayStatics::SaveGameToSlot() and UGameplayStatics::LoadGameFromSlot() for save/load
 *   - One binary savegame slot (see UPinnedToolActorsSaveGame) for all pinned actor records for one actor class
 *   - SPinToolActorWindow implements UI for IToolActor
 */
UINTERFACE(BlueprintType, Blueprintable, meta = (RuntimeMetaData))
class UNREALSODA_API UToolActor : public USodaActor
{
	GENERATED_BODY()
};

class UNREALSODA_API IToolActor : public ISodaActor
{
	GENERATED_BODY()

public:
	virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	virtual bool IsPinnedActor() const override;
	virtual bool SavePinnedActor() override;
	virtual AActor* LoadPinnedActor(UWorld* World, const FTransform& Transform, const FString& SlotName, bool bForceCreate, FName DesireName = NAME_None) const override;
	virtual FString GetPinnedActorSlotName() const override;
	virtual FString GetPinnedActorName() const override;

	/** It is necessary to call this method in any descendants in the Serialize(FArchive& Ar) method */
	virtual void ToolActorSerialize(FArchive& Ar); 

public:
	virtual bool PinActor(const FString& SlotName);
	virtual void UnpinActor();
	virtual bool DeserializePinnedActor(const FString& SlotName);
	virtual UPinnedToolActorsSaveGame* GetSaveGame() const; //May be called in the CDO object
	virtual IToolActor* FindPinnedActorBySlotName(const UWorld * World, const FString& SlotName) const; // May be called in the CDO object 
	virtual bool DeleteSlot(const UWorld* World, const FString& SlotName) const; // May be called in the CDO object 
	virtual bool SaveAll() const;

protected:
	FString ToolActorSlotName;
};

