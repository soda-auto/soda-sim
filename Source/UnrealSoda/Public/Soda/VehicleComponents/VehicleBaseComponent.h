// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "Soda/ISodaVehicleComponent.h"
#include "VehicleBaseComponent.generated.h"

class ASodaVehicle;
class USodaSubsystem;
class ALevelState;

struct FSensorDataHeader
{
	TTimestamp Timestamp;
	int64 FrameIndex;
	//FString FrameName;
};

/**
 * UVehicleBaseComponent
 */
UCLASS(Abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleBaseComponent : 
	public UPrimitiveComponent,
	public ISodaVehicleComponent,
	public IVehicleTickablObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Setting for generate the GUI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame)
	FVehicleComponentGUI GUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame, meta = (EditInRuntime))
	FVehicleComponentCommon Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame)
	FVehicleComponentTick TickData;

	UPROPERTY(BlueprintAssignable, Category = VehicleComponent)
	FVehicleComponentActivatedDelegate OnVehicleComponentActivated;

	UPROPERTY(BlueprintAssignable, Category = VehicleComponent)
	FVehicleComponentDeactivateDelegate OnVehicleComponentDeactivated;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = VehicleComponent, meta = (DisplayName = "ActivateVehicleComponen"))
	void ReceiveActivateVehicleComponent();

	UFUNCTION(BlueprintImplementableEvent, Category = VehicleComponent, meta = (DisplayName = "DeactivateVehicleComponent"))
	void ReceiveDeactivateVehicleComponent();

public:
	/* Override ISodaVehicleComponent */
	virtual UActorComponent* AsActorComponent() override { return this; }
	virtual ASodaVehicle* GetVehicle() const override { return Vehicle; }
	virtual bool IsTickAllowed() const override { return HealthIsWorkable(); }

	virtual FVehicleComponentGUI& GetVehicleComponentGUI() override { return GUI; }
	virtual FVehicleComponentCommon& GetVehicleComponentCommon() override { return Common; }
	virtual FVehicleComponentTick& GetVehicleComponentTick() override { return TickData; }

	virtual const FVehicleComponentGUI& GetVehicleComponentGUI() const override { return GUI; }
	virtual const FVehicleComponentCommon& GetVehicleComponentCommon() const override { return Common; }
	virtual const FVehicleComponentTick& GetVehicleComponentTick() const override { return TickData; }

	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

public:
	/* Override AActorComponent */
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	USodaSubsystem* GetSodaSubsystem() const { return SodaSubsystem; }
	ALevelState* GetLevelState() const { return LevelState; }

	FSensorDataHeader GetHeaderGameThread() const;
	FSensorDataHeader GetHeaderVehicleThread() const;

private:
	UPROPERTY()
	ASodaVehicle* Vehicle = nullptr;

	UPROPERTY()
	USodaSubsystem* SodaSubsystem = nullptr;

	UPROPERTY()
	ALevelState* LevelState = nullptr;
};
