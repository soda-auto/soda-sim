// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleDifferentialComponent.generated.h"


/**
 * UVehicleDifferentialBaseComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleDifferentialBaseComponent  : public UWheeledVehicleComponent, public ITorqueTransmission
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PassTorque(float InTorque) {}
	virtual float ResolveAngularVelocity() const { return 0; }
	virtual bool FindWheelRadius(float& OutRadius) const override { return false; }
	virtual bool FindToWheelRatio(float& OutRatio) const override { return false; }
};

/**
 * EVehicleDifferentialType
 */
UENUM(BlueprintType)
enum class  EVehicleDifferentialType : uint8
{
	/** Connect engines to front wheels */
	Open_FrontDrive,

	/** Connect engines to rear wheels */
	Open_RearDrive,
};

/**
 * UVehicleDifferentialSimpleComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleDifferentialSimpleComponent : public UVehicleDifferentialBaseComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Differential)
	TScriptInterface<ITorqueTransmission> OutputTorqueTransmission1;

	UPROPERTY(BlueprintReadOnly, Category = Differential)
	TScriptInterface<ITorqueTransmission> OutputTorqueTransmission2;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Differential, SaveGame, meta = (EditInRuntime))
	EVehicleDifferentialType DifferentialType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "TorqueTransmission"))
	FSubobjectReference LinkToTorqueTransmission1 { TEXT("WheelFL") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "TorqueTransmission"))
	FSubobjectReference LinkToTorqueTransmission2 { TEXT("WheelFR") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Differential, SaveGame, meta = (EditInRuntime))
	float Ratio = 1.0;

	float GetInTorq() const { return InTorq; }
	float GetOutTorq() const{ return OutTorq; }
	float GetInAngularVelocity() const{ return InAngularVelocity; }
	float GetOutAngularVelocity() const{ return OutAngularVelocity; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

public:
	virtual void PassTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override;
	virtual bool FindWheelRadius(float& OutRadius) const override;
	virtual bool FindToWheelRatio(float& OutRatio) const override { OutRatio = Ratio; return true; }

protected:
	mutable float InTorq = 0;
	mutable float OutTorq = 0;
	mutable float InAngularVelocity = 0;
	mutable float OutAngularVelocity = 0;
};

