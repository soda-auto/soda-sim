// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleMovementBaseComponent.h"
#include "StaticWheeledVehicleMovement.generated.h"

class USodaGameModeComponent;

/**
 * The UStaticWheeledVehicleMovementComponent
 */
UCLASS(ClassGroup = Soda,BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UStaticWheeledVehicleMovementComponent : public UWheeledVehicleMovementBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

protected:
	/* Overrides from  ISodaVehicleComponent */
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPreActivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override {}

public:
	/*  Overrides from  IWheeledVehicleMovementInterface  */
	virtual float GetVehicleMass() const override { return 1500; }
	virtual const FVehicleSimData& GetSimData() const { return VehicleSimData; }
	virtual bool SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation);

protected:
	FVehicleSimData VehicleSimData;
};