// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/SaveGame.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/Misc/LLConverter.h"
#include "IEditableObject.h"
#include "LevelState.generated.h"

class ASodaActorFactory;
class ASodaVehicle;

UENUM(BlueprintType)
enum class ELeveSlotSource : uint8
{
	NoSlot,
	Local,
	Remote,
	NewSlot,
};

/*
 * FLevelStateSlotDescription
 */
USTRUCT(BlueprintType)
struct FLevelStateSlotDescription
{
	GENERATED_BODY()

	/** Valid only if SlotSource ==  Local */
	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	int SlotIndex = -1;

	/** Valid only if SlotSource ==  Remote */
	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	int64 ScenarioID = -1;

	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	FDateTime DateTime;

	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	FString LevelName;

	UPROPERTY(BlueprintReadOnly, Category = LevelStateSlotDescription)
	ELeveSlotSource SlotSource = ELeveSlotSource::NoSlot;
};

/*
 * ULevelSaveGame
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULevelSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = LevelSaveGame, SaveGame)
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = LevelSaveGame, SaveGame)
	FDateTime DateTime;

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

	UPROPERTY(BlueprintReadOnly, Category = LevelSaveGame)
	FLevelStateSlotDescription Slot;

	UPROPERTY(BlueprintReadOnly, Category = LevelState)
	ASodaActorFactory * ActorFactory = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = LevelState, meta = (EditInRuntime))
	FLLConverter LLConverter;

public:
	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool SaveLevelRemotlyAs(int64 ScenarioID, const FString& Description);

	/** if SlotIndex < 0, SlotIndex will detect automaticly */
	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool SaveLevelLocallyAs(int SlotIndex, const FString& Description);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool SaveLevelToTransientSlot();

	UFUNCTION(BlueprintCallable, Category = LevelState)
	bool ReSaveLevel();

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool ReloadLevelEmpty(const UObject* WorldContextObject);

	/**
	 * SlotIndex == -1 -  Find & open last saved slot
	 */
	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool ReloadLevelFromSlotLocally(const UObject* WorldContextObject, int SlotIndex = -1); 

	/**
	 * SlotIndex == -1 -  Find & open last saved slot
	 */
	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool ReloadLevelFromSlotRemotly(const UObject* WorldContextObject, int64 ScenarioID = -1);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static ALevelState* CreateOrLoad(const UObject* WorldContextObject, UClass * DefaultClass, bool bFromTransientSlot = false);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool DeleteLevelLocally(const UObject* WorldContextObject, int SlotIndex);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool DeleteLevelRemotly(const UObject* WorldContextObject, int64 ScenarioID);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool GetLevelSlotsLocally(const UObject* WorldContextObject, TArray<FLevelStateSlotDescription> & Slots, bool bSortByDateTime = false);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	static bool GetLevelSlotsRemotly(const UObject* WorldContextObject, TArray<FLevelStateSlotDescription> & Slots, bool bSortByDateTime = false);

	UFUNCTION(BlueprintCallable, Category = LevelState)
	void FinishLoadLevel();

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

	virtual ULevelSaveGame* CreateSaveGame(bool bSerializePinnedActorsAsUnpinned);

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
	static FString GetLocalSaveSlotName(const UObject* WorldContextObject, int SlotIndex);
	/* SlotIndex == -1 - load last saved slot */
	static ULevelSaveGame* LoadSaveGameLocally(const UObject* WorldContextObject, int & SlotIndex);
	/* SlotIndex == -1 - load last saved slot */
	static ULevelSaveGame* LoadSaveGameRemotly(const UObject* WorldContextObject, int64 & ScenarioID);

	static ULevelSaveGame* LoadSaveGameTransient(const UObject* WorldContextObject);

	static FLevelStateSlotDescription StaticSlot;

protected:
	bool bIsDirty = false;
};
