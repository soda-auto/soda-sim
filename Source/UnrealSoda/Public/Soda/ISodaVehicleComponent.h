// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Templates/SubclassOf.h"
#include "Soda/SodaTypes.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Misc/Time.h"
#include "IEditableObject.h"
#include "ISodaVehicleComponent.generated.h"

class SWidget;
class FSceneView;
class FPrimitiveDrawInterface;
class UCanvas;
class ASodaVehicle;
class UActorComponent;

namespace soda
{
	class FActorDatasetData;
	struct FBsonDocument;
}

enum class EVehicleComponentDeferredTask
{
	None,
	Activate,
	Deactivate
};

UENUM(BlueprintType)
enum class EVehicleComponentActivation : uint8
{
	None,
	OnBeginPlay,
	OnStartScenario
};

/**
 * FVehicleComponentGUI
 * Setting for generate the GUI
 */
USTRUCT(BlueprintType)
struct FVehicleComponentGUI
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GUI)
	FString Category = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GUI)
	FString ComponentNameOverride = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GUI)
	bool bIsPresentInAddMenu = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GUI)
	bool bIsSubComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GUI)
	FName IcanName = TEXT("SodaIcons.Soda");

	UPROPERTY(BlueprintReadOnly, Category = GUI, SaveGame)
	int Order = 0;

	UPROPERTY(BlueprintReadOnly, Category = GUI, SaveGame)
	bool bIsDeleted = false;
};

/**
 * FVehicleComponentCommon
 */
USTRUCT(BlueprintType)
struct FVehicleComponentCommon
{
	GENERATED_USTRUCT_BODY()

	/** if true component will activated after BeginPlay() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleComponent, SaveGame)
	EVehicleComponentActivation Activation = EVehicleComponentActivation::None;

	/**
	  * The Topology Component means that other vehicle components may depend on this component.
	  * This means that if this component is added, removed, renamed or activated, all other components of the vehicle will be reactivated.
	  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	bool bIsTopologyComponent = false;

	/** 
	 * Component will update on frequency FPS/Divider.
	 * FPSDivider:
	 * 1 - process every tick
	 * 2 - process 1st tick; 2nd - skip
	 * 3 - process 1st tick; 2nd, 3rd - skip
	 * 4 - process 1st tick; 2nd, 3rd, 4th - skip
	 * ...
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleComponent, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int RealtimeTickDivider = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleComponent, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int NonRealtimeTickDivider = 3;

	/** 
	  * Whether to write a dataset for this component. Valid only if this component supports dataset writing.
	  * See USodaVehicleComponent::OnPushDataset().
	  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = VehicleComponent)
	bool bWriteDataset = false;

	/** Whether to allow this sensor to display any debug information. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugCanvas = false;

	/** Only one component of this class can be active */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = VehicleComponent)
	UClass * UniqueVehiceComponentClass = nullptr;

	/** Any parent allowed (not only ASodaVehicle)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = VehicleComponent)
	bool bAnyParentAllowed = false;
};

/**
 * FVehicleComponentTick
 */
USTRUCT(BlueprintType)
struct FVehicleComponentTick
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	bool bAllowVehiclePrePhysTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	bool bAllowVehiclePostPhysTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	bool bAllowVehiclePostDeferredPhysTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	EVehicleComponentPrePhysTickGroup PrePhysTickGroup = EVehicleComponentPrePhysTickGroup::TickGroup0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	EVehicleComponentPostPhysTickGroup PostPhysTickGroup = EVehicleComponentPostPhysTickGroup::TickGroup0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent)
	EVehicleComponentPostDeferredPhysTickGroup PostPhysDeferredTickGroup = EVehicleComponentPostDeferredPhysTickGroup::TickGroup0;
};

/**
 * USodaVehicleComponent
 */
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint, RuntimeMetaData))
class UNREALSODA_API USodaVehicleComponent : public UEditableObject
{
	GENERATED_BODY()
};

class UNREALSODA_API ISodaVehicleComponent : public IEditableObject
{
	friend ASodaVehicle;

	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual UActorComponent* AsActorComponent() = 0;

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual FVehicleComponentGUI & GetVehicleComponentGUI() = 0;

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual FVehicleComponentCommon& GetVehicleComponentCommon() = 0;

	virtual const FVehicleComponentGUI & GetVehicleComponentGUI() const = 0;
	virtual const FVehicleComponentCommon& GetVehicleComponentCommon() const = 0;

	/** Get a pointer to the vehicle on which this component is located. */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual ASodaVehicle * GetVehicle() const = 0;

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool DoesVehicleSupport(TSubclassOf<ASodaVehicle> VehicleClass) const { return true; }

	/** Returns whether the VehicleComponent is active or not */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool IsVehicleComponentActiveted() const { return bIsVehicleComponentActiveted; }

	/** The component can be activated only after BeginPlay() */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent, meta=(ScenarioAction))
	virtual void ActivateVehicleComponent();

	/** If component is activated then it will be automatically deactivated before EndPlay() */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent, meta = (ScenarioAction))
	virtual void DeactivateVehicleComponent();

	UFUNCTION(BlueprintCallable, Category = VehicleComponent, meta = (ScenarioAction))
	virtual void Toggle();

	/** This description will be displayed in the UI (Vehicle Components List) */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual void GetRemark(FString & Info) const { Info = ""; }

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual EVehicleComponentHealth GetHealth() const { return  Health; }

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual void SetHealth(EVehicleComponentHealth NewHealth, const FString & AddMessage="");

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool IsVehicleComponentInitializing() const { return false; }

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual void AddDebugMessage(EVehicleComponentHealth MessageHealth, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool HealthIsWorkable() const { return !IsVehicleComponentInitializing() && (GetHealth() == EVehicleComponentHealth::Ok || GetHealth() == EVehicleComponentHealth::Warning); }

	/** Returns whether this component was instanced from a component/subobject template */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool IsDefaultComponent();

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual void RemoveVehicleComponent();

	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool RenameVehicleComponent(const FString & NewName);

	/** Is component should tick on current frame according to FpsDivider */
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	virtual bool IsTickOnCurrentFrame() const;

	virtual void MarkAsDirty();

	/**
	 * Draw on the debug canvase
	 */
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos);

	virtual void ScenarioBegin() {}

	virtual void ScenarioEnd() {}

	virtual void DrawSelection(const FSceneView* View, FPrimitiveDrawInterface* PDI);

	EVehicleComponentDeferredTask GetDeferredTask() const { return DeferredTask; }
	void SetDeferredTask(EVehicleComponentDeferredTask Task) { DeferredTask = Task; }

	bool IsVehicleComponentSelected() const;

protected:
	/* Override from IEditableObject */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

protected:
	virtual void OnRegistreVehicleComponent() {}
	virtual void OnPreActivateVehicleComponent() {}
	virtual bool OnActivateVehicleComponent();
	virtual void OnDeactivateVehicleComponent();

	/** 
	 * Callen during scenario playing if the datset is recording for this vehicle.
	 * This function called in the vehicle physic thread.
	 */
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const {}

	/**
	 * Callen after ScenarioPlay()
	 * Here you can fill in the descriptor that will be written to the database
	 */
	virtual void GenerateDatasetDescription(soda::FBsonDocument& Doc) const {}


private:
	EVehicleComponentHealth Health = EVehicleComponentHealth::Disabled;
	EVehicleComponentDeferredTask DeferredTask = EVehicleComponentDeferredTask::None;
	bool bIsVehicleComponentActiveted = false;
	int TickDivider = 1;
	TArray <FString> DebugCanvasErrorMessages;
	TArray <FString> DebugCanvasWarningMessages;
};


/**
 * UVehicleTickablObject
 */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint, RuntimeMetaData))
class UNREALSODA_API UVehicleTickablObject : public UInterface
{
	GENERATED_BODY()
};

class UNREALSODA_API IVehicleTickablObject
{
	GENERATED_BODY()

public:

	virtual FVehicleComponentTick& GetVehicleComponentTick() = 0;

	const virtual FVehicleComponentTick& GetVehicleComponentTick() const = 0;

	virtual bool IsTickAllowed() const { return true; }


	/**
	 * Called just before stimulation step.
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating sub-steps).
	 */
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) {}

	/**
	 * Called just after stimulation step .
	 * For some implementations it can be called not in real time (For example, for PhysX Vehicle model when calculating sub-steps).
	 */
	virtual void PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) {}

	/**
	  * Called just after PostPhysicSimulation().
	  * This function call must correspond to real time.
	  */
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) {}
};

