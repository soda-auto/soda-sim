// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "MassAgentComponent.h"
#include "MassEntityTypes.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/ISodaDataset.h"
#include "VehicleMassAgentComponent.generated.h"

class ASodaWheeledVehicle;

/**
 * UVehicleMassAgentComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleMassAgentComponent 
	: public UMassAgentComponent
	, public ISodaVehicleComponent
	, public IObjectDataset
{
	GENERATED_BODY()

public:
	UVehicleMassAgentComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MassAgent, SaveGame)
	FVehicleComponentGUI GUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MassAgent, SaveGame)
	FVehicleComponentCommon Common;

	UPROPERTY(EditAnywhere, SaveGame, Instanced, Category = MassAgent, meta = (EditInRuntime, ReactivateComponent))
	TArray<TObjectPtr<UMassEntityTraitBase>> Traits;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

public:
	/* Override ISodaVehicleComponent */
	virtual UActorComponent* AsActorComponent() override { return this; }
	virtual ASodaVehicle* GetVehicle() const override;

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

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	void OnMassAgentComponentEntityAssociated(const UMassAgentComponent& AgentComponent);

	UPROPERTY()
	FMassEntityConfig OriginEntityConfig;
};
