// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
//#include "Soda/VehicleComponents/Sensors/ImuSensor.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "VehicleAnimationInstance.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "SodaChaosWheeledVehicleMovement.generated.h"

class USodaChaosWheeledVehicleMovementComponent;


USTRUCT(BlueprintType)
struct UNREALSODA_API FSodaChaosWheelSetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime, AllowedClasses = "/Script/UnrealSoda.SodaVehicleWheelComponent"))
	FSubobjectReference ConnectedSodaWheel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditCondition = "bOverrideRadius", EditInRuntime))
	float OverrideRadius = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bOverrideRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditCondition = "bOverrideFrictionMultiplier", EditInRuntime))
	float OverrideFrictionMultiplier = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WheelSetup, SaveGame, meta = (EditInRuntime))
	bool bOverrideFrictionMultiplier = false;


	UPROPERTY(BlueprintReadWrite)
	USodaVehicleWheelComponent* SodaWheel = nullptr;

	FSodaChaosWheelSetup() {}
};

/**
 *  USodaChaosWheeledVehicleSimulation
 */
class UNREALSODA_API USodaChaosWheeledVehicleSimulation : public UChaosWheeledVehicleSimulation
{
	friend USodaChaosWheeledVehicleMovementComponent;

public:
	USodaChaosWheeledVehicleSimulation(ASodaWheeledVehicle* WheeledVehicleIn, USodaChaosWheeledVehicleMovementComponent* InWheeledVehicleComponent);
	
	virtual void Init(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicleIn) override;
	virtual void UpdateSimulation(float DeltaTime, const FChaosVehicleAsyncInput& InputData, Chaos::FRigidBodyHandle_Internal* Handle) override;
	virtual void ProcessSteering(const FControlInputs& ControlInputs) override {}
	virtual void ProcessMechanicalSimulation(float DeltaTime)  override {}
	virtual void ApplyInput(const FControlInputs& ControlInputs, float DeltaTime) override { UChaosVehicleSimulation::ApplyInput(ControlInputs, DeltaTime); }

protected:
	ASodaWheeledVehicle* WheeledVehicle = nullptr;
	USodaChaosWheeledVehicleMovementComponent* WheeledVehicleComponent = nullptr;

	FVehicleSimData VehicleSimData;
};

/**
 *  USodaChaosWheeledVehicleMovementComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), hidecategories = (MechanicalSetup, SteeringSetup))
class UNREALSODA_API USodaChaosWheeledVehicleMovementComponent : 
	public UChaosWheeledVehicleMovementComponent, 
	public IWheeledVehicleMovementInterface,
	public ISodaVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bShowWheelSates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bShowWheelGraph;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugLines;

	/** Login some debug states to the console. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bLogPhysStemp = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WheelSetup, EditFixedSize, SaveGame, meta = (EditInRuntime))
	TArray<FSodaChaosWheelSetup> SodaWheelSetups;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame)
	FVehicleComponentGUI GUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame, meta = (EditInRuntime))
	FVehicleComponentCommon Common;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual void PostInitProperties() override;
#endif

public:
	/** Override ISodaVehicleComponent */
	virtual UActorComponent* AsActorComponent() override { return this; }
	virtual ASodaVehicle* GetVehicle() const override { return WheeledVehicle; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

	virtual FVehicleComponentGUI& GetVehicleComponentGUI() override { return GUI; }
	virtual FVehicleComponentCommon& GetVehicleComponentCommon() override { return Common; }

	virtual const FVehicleComponentGUI& GetVehicleComponentGUI() const override { return GUI; }
	virtual const FVehicleComponentCommon& GetVehicleComponentCommon() const override { return Common; }

	UFUNCTION(BlueprintImplementableEvent, Category = VehicleComponent, meta = (DisplayName = "ActivateVehicleComponen"))
	void ReceiveActivateVehicleComponent();

	UFUNCTION(BlueprintImplementableEvent, Category = VehicleComponent, meta = (DisplayName = "DeactivateVehicleComponent"))
	void ReceiveDeactivateVehicleComponent();

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPreActivateVehicleComponent() override;
	virtual void OnPreDeactivateVehicleComponent() override;

public:
	/** Override IWheeledVehicleMovementInterface */
	virtual const FVehicleSimData& GetSimData() const;
	virtual bool SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation);
	virtual ASodaWheeledVehicle* GetWheeledVehicle() const override { return WheeledVehicle; }
	virtual float GetVehicleMass() const  override { return Mass; }

public:
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual bool ShouldCreatePhysicsState() const override;

public:
	virtual TUniquePtr<Chaos::FSimpleWheeledVehicle> CreatePhysicsVehicle() override;
	virtual void ProcessSleeping(const FControlInputs& ControlInputs) override;

public:
	UPROPERTY()
	ASodaWheeledVehicle* WheeledVehicle = nullptr;
	bool bSynchronousMode = false;

protected:
	TWeakPtr<USodaChaosWheeledVehicleSimulation> ArrialVehicleSimulationPT;
	bool bAllowCreatePhysicsState = false;
};