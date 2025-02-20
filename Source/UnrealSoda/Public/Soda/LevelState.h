// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/SaveGame.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/Misc/LLConverter.h"
#include "IEditableObject.h"
#include "LevelState.generated.h"

class ASodaActorFactory;
class ASodaVehicle;

namespace soda
{
	struct FFileDatabaseSlotInfo;
}


/*
 * ULevelSaveGame
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULevelSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = LevelSaveGame, SaveGame)
	FString LevelName;

	FActorRecord LevelDataRecord;

public:
	virtual void Serialize(FArchive& Ar) override;
};

/*
 * ALevelState
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ALevelState : 
	public AActor,
	public IEditableObject
{
	GENERATED_BODY()

public:
	ALevelState();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelState)
	TArray<AActor*> AdditionalActorsForSerialize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelState)
	TSubclassOf<ASodaActorFactory> DefaultActorFactoryClass;

	UFUNCTION(BlueprintCallable, Category = LevelState)
	const FGuid& GetSlotGuid() const { return SlotGuid; }

	UFUNCTION(BlueprintCallable, Category = LevelState)
	const FString& GetSlotLabel() const { return SlotLabel; }
	
	UPROPERTY(BlueprintReadOnly, Category = LevelState)
	ASodaActorFactory * ActorFactory = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = LevelState, meta = (EditInRuntime))
	FLLConverter LLConverter;

public:
	/**  if Guid is empty - create new slot */
	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool SaveToSlot(const FString& Label, const FString& Description, const FGuid& Guid = FGuid());

	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool SaveToToMemory(TArray<uint8>& OutSaveData);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool Resave();

	UFUNCTION(BlueprintCallable, Category = LevelState)
	void SpawnSavedActors();

	UFUNCTION(BlueprintCallable, Category = LevelState)
	virtual bool IsDirty() { return bIsDirty; }

	UFUNCTION(BlueprintCallable, Category = LevelState)
	virtual void MarkAsDirty() { bIsDirty = true; }

	UFUNCTION(BlueprintCallable, Category = LevelState)
	virtual void ClearLevel();

	const FLLConverter& GetLLConverter() const { return LLConverter; }
	void SetGeoReference(double Lat, double Lon, double Alt, const FVector& OrignShift = FVector{ 0, 0, 0 }, float OrignDYaw = 0);

	static ALevelState* Get();
	static ALevelState* GetChecked();
	static ULevelSaveGame* LoadSaveGameFromSlot(const FGuid& Guid, soda::FFileDatabaseSlotInfo& OutSlotInfo);

	virtual ULevelSaveGame* CreateSaveGame(bool bSerializePinnedActorsAsUnpinned);

	static bool SerializeSlotDescriptor(const FString& LevelName, FString& OutJsonString);
	static bool DeserializeSlotDescriptor(const FString& InJsonString, FString& OutLevelName);

public:
	virtual void Serialize(FArchive& Ar) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

public:
	FActorRecord SpectatorActorRecord;
	FActorRecord ActorFactoryRecord;
	TArray<FActorRecord> AdditionalActorRecords;


protected:
	bool bIsDirty = false;

	UPROPERTY(SaveGame)
	FGuid SlotGuid;

	UPROPERTY(SaveGame)
	FString SlotLabel;

	FDelegateHandle ActorsMapChenagedHandle;
};
