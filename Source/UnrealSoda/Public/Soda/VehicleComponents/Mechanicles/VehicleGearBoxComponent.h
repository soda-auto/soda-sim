// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleGearBoxComponent.generated.h"

class UVehicleInputComponent;

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
	virtual bool SetGearByState(EGearState GearState) { return false; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual bool SetGearByNum(int GearNum) { return false; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual EGearState GetGearState() const { return EGearState::Neutral; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual int GetGearNum() const { return 0; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual float GetGearRatio() const { return 0; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual FString GetGearChar() const;

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual int GetForwardGearsCount() const { return 0; }

	UFUNCTION(Category = GearBox, BlueprintCallable)
	virtual int GetReversGearsCount() const { return 0; }

	virtual bool AcceptGearFromVehicleInput(UVehicleInputComponent* VehicleInput);

public:
	virtual void PassTorque(float InTorque) override {}
	virtual float ResolveAngularVelocity() const override { return 0; }
	virtual bool FindWheelRadius(float& OutRadius) const override { return false; }
	virtual bool FindToWheelRatio(float& OutRatio) const override { return false; }
};

/**
 * UVehicleGearBoxSimpleComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleGearBoxSimpleComponent : public UVehicleGearBoxBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.TorqueTransmission"))
	FSubobjectReference LinkToTorqueTransmission { TEXT("Differential") };

	/** Allow set brake from default the UVhicleInputComponent */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime))
	bool bAcceptGearFromVehicleInput = true;

	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bUseAutomaticGears = false;

	//UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ReactivateActor))
	//bool bUseAutoReverse;

	/** Forward gear ratios */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TArray<float> ForwardGearRatios;

	/** Reverse gear ratio(s) */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TArray<float> ReverseGearRatios;

	/** Engine Revs at which gear up change ocurrs */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ClampMin = "0.0", UIMin = "0.0", ClampMax = "50000.0", UIMax = "50000.0"))
	float ChangeUpRPM = 4500.0f;

	/** Engine Revs at which gear down change ocurrs */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ClampMin = "0.0", UIMin = "0.0", ClampMax = "50000.0", UIMax = "50000.0"))
	float ChangeDownRPM = 2000.0f;

	/** Time it takes to switch gears (seconds) */
	UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ClampMin = "0.0", UIMin = "0.0"))
	float GearChangeTime = 0.2;

	/** Mechanical frictional losses mean transmission might operate at 0.94 (94% efficiency) */
	//UPROPERTY(EditAnywhere, Category = GearBox, SaveGame, meta = (EditInRuntime, ReactivateActor))
	//float TransmissionEfficiency = 1.0;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

public:
	virtual void PassTorque(float InTorque) override;
	virtual float ResolveAngularVelocity() const override;
	virtual bool FindWheelRadius(float& OutRadius) const override;
	virtual bool FindToWheelRatio(float& OutRatio) const override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	virtual bool SetGearByState(EGearState GearState) override;
	virtual bool SetGearByNum(int GearNum) override;
	virtual EGearState GetGearState() const override { return CurrentGearState; };
	virtual int GetGearNum() const override { return CurrentGearNum; }
	virtual float GetGearRatio() const override { return Ratio; }
	virtual int GetForwardGearsCount() const { return ForwardGearRatios.Num(); }
	virtual int GetReversGearsCount() const { return ReverseGearRatios.Num(); }
	float GetInTorq() const { return InTorq; }
	float GetOutTorq() const { return OutTorq; }
	float GetInAngularVelocity() const { return InAngularVelocity; }
	float GetOutAngularVelocity() const { return OutAngularVelocity; }

protected:
	float Ratio = 0;
	EGearState CurrentGearState = EGearState::Neutral;
	EGearState TargetGearState = EGearState::Neutral;
	int CurrentGearNum = 0;
	int TargetGearNum = 0;
	float CurrentGearChangeTime = 0; // Time to change gear, no power transmitted to the wheels during change

	UPROPERTY()
	TScriptInterface<ITorqueTransmission> OutputTorqueTransmission;

	mutable float InTorq = 0;
	mutable float OutTorq = 0;
	mutable float InAngularVelocity = 0;
	mutable float OutAngularVelocity = 0;
};

