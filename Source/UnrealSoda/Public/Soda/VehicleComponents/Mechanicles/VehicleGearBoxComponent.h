// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleGearBoxComponent.generated.h"



/**
 * UVehicleGearBoxBaseComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleGearBoxBaseComponent  : 
	public UWheeledVehicleComponent, 
	public ITorqueTransmission
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual FString GetGearChar() const { return TEXT("?"); }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual void SetGear(ENGear Gear) {}

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual ENGear GetGear() const { return ENGear::Neutral; }

public:
	virtual void PassTorque(float InTorque) {}
	virtual float ResolveAngularVelocity() const { return 0; }
	virtual float FindWheelRadius() const { return 0; }
	virtual float FindToWheelRatio() const { return 1; }
};


/**
 * UVehicleGearBoxSimpleComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleGearBoxSimpleComponent : public UVehicleGearBoxBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/SodaSim.TorqueTransmission"))
	FSubobjectReference LinkToTorqueTransmission { TEXT("Differential") };

	/** Allow set brake from default the UVhicleInputComponent */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime))
	bool bAcceptGearFromVehicleInput = true;

	/** Drive gear ratio */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime))
	float DGearRatio = 1;

	/** Reverse gear ratio */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime))
	float RGearRatio = 1;


protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

public:
	virtual void PassTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override;
	virtual float FindWheelRadius() const override;
	virtual float FindToWheelRatio() const override;

public:
	virtual FString GetGearChar() const override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual void SetGear(ENGear Gear) override;
	virtual ENGear GetGear() const override { return Gear; };

protected:
	float Ratio = 0;
	ENGear Gear = ENGear::Park;

	UPROPERTY()
	TScriptInterface<ITorqueTransmission> OutputTorqueTransmission;

	mutable float DebugInTorq = 0;
	mutable float DebugOutTorq = 0;
	mutable float DebugInAngularVelocity = 0;
	mutable float DebugOutAngularVelocity = 0;
};

