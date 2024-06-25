// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "WheeledVehicleMovementBaseComponent.generated.h"

class ASodaWheeledVehicle;

/**
 * UWheeledVehicleMovementBaseComponent
 * This is just template for implement the IWheeledVehicleMovementInterface component
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UWheeledVehicleMovementBaseComponent :
	public UPawnMovementComponent,
	public IWheeledVehicleMovementInterface,
	public ISodaVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame)
	FVehicleComponentGUI GUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleComponent, SaveGame)
	FVehicleComponentCommon Common;

	virtual void InitializeComponent() override;
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

public:
	/* Override IWheeledVehicleMovementInterface */
	virtual ASodaWheeledVehicle* GetWheeledVehicle() const override { return WheeledVehicle; }

protected:
	UPROPERTY()
	ASodaWheeledVehicle* WheeledVehicle = nullptr;
};
