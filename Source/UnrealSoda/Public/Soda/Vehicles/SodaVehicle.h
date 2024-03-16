// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakInterfacePtr.h"
#include "Soda/Misc/JsonArchive.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/Misc/Extent.h"
#include "Soda/SodaTypes.h"
#include "VehicleBaseTypes.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaVehicleComponent.h"
#include "SodaVehicle.generated.h"

DECLARE_STATS_GROUP(TEXT("SodaVehicle"), STATGROUP_SodaVehicle, STATGROUP_Advanced);

class UCANBusComponent;

UENUM(BlueprintType)
enum class EVehicleSaveSource : uint8
{
	NoSave,

	/** The vehicles is saved to an binary file format in external location. */
	BinExternal,  

	/** The vehicles is saved to an binary file format in the local storage. */
	BinLocal,

	/** The vehicles is saved to an separate save game slot. */
	Slot, 

	/** The vehicle is saved as part of the binary level save game. */
	BinLevel, 

	/** The vehicles is saved to an JSON file format in external location. */
	JsonExternal,

	/** The vehicles is saved to an JSON file format in local storage. */
	JsonLocal, 

	/** The vehicle is saved to the MongoDB. */
	DB,
};

USTRUCT(BlueprintType)
struct FVechicleSaveAddress
{
	GENERATED_BODY()

	FVechicleSaveAddress() {}
	FVechicleSaveAddress(EVehicleSaveSource InSource, const FString& InLocation)
		: Source(InSource)
		, Location(InLocation)
	{}

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = VechicleSaveAddress)
	EVehicleSaveSource Source = EVehicleSaveSource::NoSave;

	/** Location of the save data provided by the Source */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = VechicleSaveAddress)
	FString Location;

	FORCEINLINE bool operator ==(const FVechicleSaveAddress& Other) const
	{
		return Source != EVehicleSaveSource::NoSave && Source != EVehicleSaveSource::BinLevel && Source == Other.Source && Location == Other.Location;
	}

	FString ToVehicleName() const ;

	FORCEINLINE void Set(EVehicleSaveSource InSource, const FString& InLocation)
	{
		Source = InSource;
		Location = InLocation;
	}
	
	bool SetFromUrl(const FString & Url);

	FORCEINLINE void Reset()
	{
		Source = EVehicleSaveSource::NoSave;
		Location.Reset();
	}

	FString ToUrl() const;

};

/**
 * UVehicleWidget is the abstract user widget for ASodaVehicle.
 * Usually using as gauge widget for vehicle.
 */
UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable)
class UNREALSODA_API UVehicleWidget : public UUserWidget
{
	GENERATED_BODY()

	friend class SObjectWidget;

public:
	UVehicleWidget(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{}

	UPROPERTY(Transient, BlueprintReadOnly, Category = VehicleWidget)
	ASodaVehicle* Vehcile;
};

/**
 * ASodaVehicle is the base soda's vehicle pawn actor.
 * ASodaVehicle is abstracted from the  vehicle type. It may be 2WD, 4WD, nWD, flying and other type vehicles.
 * ASodaVehicle povides binary, JSON and YAMAL serialization, vehicle widget, vehicle debug canvas
 * and ability to manage of vehicle's sensors.
 */
UCLASS(abstract, config = Game, BlueprintType)
class UNREALSODA_API ASodaVehicle : 
	public APawn, 
	public ISodaActor
{
	GENERATED_UCLASS_BODY()

	/** Show vehicle widget created form VehicleWidgetClass1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bShowVehicleWidget1 = true;

	/** Show vehicle widget created form VehicleWidgetClass2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bShowVehicleWidget2 = true;

	/** Usually Vehicle Widget Class is the speed gauge widget, but it may be anything. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget)
	TSubclassOf<UVehicleWidget> VehicleWidgetClass1;

	/** Second Vehicle Widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget)
	TSubclassOf<UVehicleWidget> VehicleWidgetClass2;

	/** 
	 * If true then speed units is rpm else km/h.
	 * This is not used in the ASodaVehicle class, but it can be uesed in the UVehicleWidget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bVehicleWidgetUnitsRPM = true;

	/** 
	 * A recommendation for the vehicle UVehicleWidget to have the UVehicleWidget display the required speedometer scale.
	 * This is not used in the ASodaVehicle class, but it can be uesed in the UVehicleWidget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleWidget, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bIsVehicleWidgetHighSpeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vehicle, SaveGame, meta = (EditInRuntime, ReactivateActor))
	ESegmObjectLabel SegmObjectLabel = ESegmObjectLabel::Vehicles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bShowPhysBodyKinematic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bRecordDataset = true;

public:
	/** Widget created form VehicleWidgetClass. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = VehicleWidget)
	UVehicleWidget* VehicleWidget1;

	/** Widget created form VehicleWidgetClass2. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = VehicleWidget)
	UVehicleWidget* VehicleWidget2;

	/** Mutex for vehicle physical thread */
	FCriticalSection PhysicMutex;

public:
	UFUNCTION(Category = "Save & Load")
	const FVechicleSaveAddress& GetSaveAddress() const { return SaveAddress; }

	UFUNCTION(Category = "Save & Load")
	void SetSaveAddress(const FVechicleSaveAddress& Address) { SaveAddress = Address; }

	/** Export snesors to registread ISodaVehicleExporter format */
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual FString ExportTo(const FString & ExporterName);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToJson(const FString& FileName, bool bRebase);
	
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToBin(const FString& FileName, bool bRebase);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToSlot(const FString& SlotName, bool bRebase);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToDB(const FString& VehicleName, bool bRebase);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToAddress(const FVechicleSaveAddress& SaveAddress, bool bRebase);

	/** If vehicle is already saved then will be resaved  */
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool Resave();

	static ASodaVehicle* SpawnVehicleFromJsonArchive(UWorld* World, const TSharedPtr<FJsonActorArchive> & Ar, const FVechicleSaveAddress & Address, const FVector& Location, const FRotator& Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFromJsonFile(const UObject* WorldContextObject, const FString& FileName, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFromDB(const UObject* WorldContextObject, const FString& VehicleName, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFromBin(const UObject* WorldContextObject, const FString& SlotOrFileName, bool IsSlot, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFormAddress(const UObject* WorldContextObject, const FVechicleSaveAddress& Address, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static void GetSavedVehiclesLocal(TArray<FVechicleSaveAddress>& Addresses);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static bool GetSavedVehiclesDB(TArray<FVechicleSaveAddress>& Addresses);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static FString GetDefaultVehiclesFolder();

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle* FindVehicleByAddress(const UWorld* World, const FVechicleSaveAddress & Address);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static bool DeleteVehicleSave(const FVechicleSaveAddress & Address);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* FindVehicleComponentByName(const FString & ComponentName) const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* FindComponentByName(const FString & ComponentName) const;

	/** UActorComponent is ISodaVehicleComponent interface */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* AddVehicleComponent(TSubclassOf<UActorComponent> Class, FName Name);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool RemoveVehicleComponentByName(FName Name);

	/** 
	 * Resapwn this vehicle.
	 * @param[in] IsOffset - Is Location & Rotation world or local space?
	 * @param[in] NewVehicleClass - change vehicle class. The NewVehicleClass must be inherited from ASodaVehicle. 
	 *							    If nullptr then the vehicle class will not be changed and all vehicles params will be moved to the returned vehicle.
	 */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual ASodaVehicle * RespawnVehcile(FVector Location = FVector(0, 0, 20), FRotator Rotation = FRotator(0, 0, 0), bool IsOffset = true, const UClass* NewVehicleClass = nullptr);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual ASodaVehicle* RespawnVehcileFromAddress(const FVechicleSaveAddress & Address, FVector Location = FVector(0, 0, 20), FRotator Rotation = FRotator(0, 0, 0), bool IsOffset = true);


	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual void ReActivateVehicleComponents(bool bOnlyTopologyComponents);

	/** Get vehicle extent calculated and store in the CalculateVehicleExtent(). */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual const FExtent & GetVehicleExtent() const { return VehicelExtent; }

	virtual const FVehicleSimData & GetSimData() const { return FVehicleSimData::Zero; }

	/** Get vehicle forward speed. Must be overridden. */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual float GetForwardSpeed() const { return 0; }

	/** Draw vehicle left debug panel to the Canvas. */
	virtual bool DrawDebug(class UCanvas* Canvas, float& YL, float& YPos);

	template<class T>
	T * FindComponentByName(FName ComponentName) const
	{
		TArray<T*> Components;
		GetComponents<T>(Components);
		for (auto& Component : Components)
		{
			if (Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}
		return nullptr;
	}

	TArray<ISodaVehicleComponent*> GetVehicleComponents() const;

	virtual TArray<ISodaVehicleComponent*> GetVehicleComponentsSorted(const FString& GUICategory = "", bool bNormalizeIfNecessary = true);


	//TODO: Add CanCreateVehicle() logic
	//virtual bool CanCreateVehicle() const;

public:
	/**
	 * Called just before stimulation step. 
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating sub-steps). 
	 */
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/** 
	 * Called just after stimulation step .
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating sub-steps). 
	 */
	virtual void PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/** 
	 * Called just after PostPhysicSimulation().
	 * This function call must correspond to real time.
	 */
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp);

	/**
	 * Called during scenario playing if the datset is recording for this vehicle.
	 */
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const;

	virtual void GenerateDatasetDescription(soda::FBsonDocument& Doc) const;

public:
	/* Override from IEditableObject */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual TSharedPtr<SWidget> GenerateToolBar();

public:
	/* Override from ISodaActor */
	virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	virtual bool IsPinnedActor() const override;
	virtual bool SavePinnedActor() override;
	virtual AActor* LoadPinnedActor(UWorld* World, const FTransform& Transform, const FString& SlotName, bool bForceCreate, FName DesireName = NAME_None) const override;
	virtual FString GetPinnedActorName() const override;
	virtual FString GetPinnedActorSlotName() const override;
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;

public:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void Serialize(FArchive& Ar) override;

protected:
	virtual void ReRegistreVehicleComponents();
	virtual void UpdateProperties();

protected:
	UPROPERTY(SaveGame)
	TArray<FString> VehicleCategoriesSorted;

	UPROPERTY(SaveGame)
	TArray<FString> VehicleComponentsSortedNames;

	UPROPERTY(SaveGame)
	FVechicleSaveAddress SaveAddress;

	TArray<IVehicleTickablObject*> PreTickedVehicleComponens;
	TArray<IVehicleTickablObject*> PostTickedVehicleComponens;
	TArray<IVehicleTickablObject*> PostTickedDeferredVehicleComponens;
	TArray<ISodaVehicleComponent*> VehicleComponensForDataset;

	UPROPERTY(Transient)
	TArray<UCANBusComponent*> CANDevs;
	TArray<FComponentRecord> ComponentRecords;
	TSharedPtr<FJsonActorArchive> JsonAr;
	FExtent VehicelExtent;

	FPhysBodyKinematic PhysBodyKinematicCashed;
	TSharedPtr<soda::FActorDatasetData> Dataset;
};
