// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "WheeledVehicleComponent.generated.h"

class ASodaWheeledVehicle;
class IWheeledVehicleMovementInterface;

/**
 * UWheeledVehicleComponent
 */
UCLASS(Abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UWheeledVehicleComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = VehicleComponent)
	ASodaWheeledVehicle* GetWheeledVehicle() const { return WheeledVehicle; }

	virtual class IWheeledVehicleMovementInterface* GetWheeledComponentInterface() { return WheeledComponentInterface; }
	virtual class IWheeledVehicleMovementInterface* GetWheeledComponentInterface() const { return WheeledComponentInterface; }

public:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const override;

private:
	UPROPERTY()
	ASodaWheeledVehicle* WheeledVehicle = nullptr;
	IWheeledVehicleMovementInterface* WheeledComponentInterface = nullptr;
};
