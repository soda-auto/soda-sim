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
#include "Soda/ISodaDataset.h"
#include "SodaVehicle.generated.h"

DECLARE_STATS_GROUP(TEXT("SodaVehicle"), STATGROUP_SodaVehicle, STATGROUP_Advanced);

class UCANBusComponent;

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
class UNREALSODA_API ASodaVehicle 
	: public APawn
	, public ISodaActor
	, public IObjectDataset

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = scenario, SaveGame, meta = (EditInRuntime))
	bool bPossesWhenScarioPlay = false;

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
	//UFUNCTION(Category = "Save & Load")
	//const FVechicleSaveAddress& GetSaveAddress() const { return SaveAddress; }

	//UFUNCTION(Category = "Save & Load")
	//void SetSaveAddress(const FVechicleSaveAddress& Address) { SaveAddress = Address; }

	/** Export snesors to registread ISodaVehicleExporter format */
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual FString ExportTo(FName ExporterName);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToJsonFile(const FString& FileName);
	
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToBinFile(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool SaveToSlot(const FString& Lable, const FString & Description, const FGuid & Guid = FGuid(), bool bRebase = true);

	/** If vehicle is already saved then will be resaved  */
	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual bool Resave();

	static ASodaVehicle* SpawnVehicleFromJsonArchive(UWorld* World, const TSharedPtr<FJsonActorArchive> & Ar, const FVector& Location, const FRotator& Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFromJsonFile(const UObject* WorldContextObject, const FString& FileName, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFromBinFile(const UObject* WorldContextObject, const FString& FileName, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	static ASodaVehicle * SpawnVehicleFormSlot(const UObject* WorldContextObject, const FGuid& Slot, const FVector& Location, const FRotator & Rotation, bool Posses = true, FName DesireName = NAME_None, bool bApplyOffset = false);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	virtual ASodaVehicle* RespawnVehcile(FVector Location = FVector(0, 0, 20), FRotator Rotation = FRotator(0, 0, 0), bool IsLocalCoordinateSpace = true);

	UFUNCTION(BlueprintCallable, Category = "Save & Load")
	const FGuid& GetSlotGuid() const { return SlotGuid; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* FindVehicleComponentByName(const FString & ComponentName) const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* FindComponentByName(const FString & ComponentName) const;

	/** UActorComponent is ISodaVehicleComponent interface */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UActorComponent* AddVehicleComponent(TSubclassOf<UActorComponent> Class, FName Name);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool RemoveVehicleComponentByName(FName Name);


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

	virtual void DrawVisualization(USodaGameViewportClient* ViewportClient, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

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
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void ReRegistreVehicleComponents();
	virtual void UpdateProperties();

protected:
	UPROPERTY(SaveGame)
	TArray<FString> VehicleCategoriesSorted;

	UPROPERTY(SaveGame)
	TArray<FString> VehicleComponentsSortedNames;

	//UPROPERTY(SaveGame)
	//FVechicleSaveAddress SaveAddress;

	UPROPERTY(SaveGame)
	FGuid SlotGuid;

	UPROPERTY(SaveGame)
	FString SlotLable;

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
};
