// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleHandBrakeComponent.generated.h"

/**
 * UVehicleHandBrakeBaseComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleHandBrakeBaseComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void RequestByRatio(float InRatio) {}
};


UENUM(BlueprintType)
enum class EHandBrakeMode : uint8
{
	FrontWheels,
	RearWheels,
	FourWheel
};

/**
 * UVehicleHandBrakeSimpleComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleHandBrakeSimpleComponent : public UVehicleHandBrakeBaseComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = HandBrake)
	FInputRate MechanicalBrakeRate;

	UPROPERTY(EditAnywhere, Category = HandBrake)
	EHandBrakeMode HandBrakeMode = EHandBrakeMode::RearWheels;

	UPROPERTY(EditAnywhere, Category = HandBrake)
	float MaxHandBrakeTorque = 3000;

public:
	virtual void RequestByRatio(float InRatio) override;

protected:
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	float CurrentRatio = 0;
};