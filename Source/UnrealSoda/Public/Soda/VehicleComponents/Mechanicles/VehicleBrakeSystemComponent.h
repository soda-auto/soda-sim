// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "VehicleBrakeSystemComponent.generated.h"

class USodaVehicleWheelComponent;

/**
 * UWheelBrake
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable)
class UWheelBrake: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Transient)
	USodaVehicleWheelComponent * ConnectedWheel = nullptr;

	/** [Bar] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual void RequestByPressure(float InBar, double DeltaTime) {};

	/** [H/m] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual void RequestByTorque(float InTorque, double DeltaTime) {};

	/** [0..1] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual void RequestByRatio(float InRatio, double DeltaTime) {}

	/** [H] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual float GetTorque() const { return 0; }

	/** [Bar] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual float GetPressure() const { return 0; }

	/** [0..1] */
	UFUNCTION(BlueprintCallable, Category = WheelBrake)
	virtual float GetLoad() const { return 0; }
};

/**
 * UVehicleBrakeSystemBaseComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleBrakeSystemBaseComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Acceleratio in [m/c^2] */
	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual void RequestByAcceleration(float InAcceleration, double DeltaTime) {}

	/** Force in [H] */
	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual void RequestByForce(float InForce, double DeltaTime) {}

	/** Torque in [H/m] */
	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual void RequestByTorque(float InTorque, double DeltaTime) {}

	/** [0..1] */
	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual void RequestByRatio(float InRatio, double DeltaTime) {}

	/** [Bar] */
	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual void RequestByPressure(float InBar, double DeltaTime) {}

	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual UWheelBrake* GetWheel(int Ind) const { return nullptr; }

	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual UWheelBrake* GetWheel4WD(E4WDWheelIndex Ind) const { return nullptr; }

	UFUNCTION(BlueprintCallable, Category = BrakeSystem)
	virtual float ComputeFullTorqByRatio(float InRatio) { return 0; }
};

/**
 * FWheelBrakeSetup
 */
USTRUCT(BlueprintType, Blueprintable)
struct FWheelBrakeSetup
{
	GENERATED_BODY();

	/** Coefficient of adhesion of a pad to a brake disc */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	float PadFricCoeff = 0.34125; 
	
	/** Number of pistons */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	int ClpPistNo = 1; 

	/** Applied piston surface area */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	float PistAr = 0.005002; 

	/** Effective brake disc radius */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	float EfcRd = 0.175; 

	/** What part of the total braking request will be applied  on this wheel  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	float BrakeDist = 0.25;

	/** Possible max brake torque [H/m] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BrakeSetup, SaveGame, meta = (EditInRuntime))
	float MaxTorque = 1500.0f;

	UPROPERTY(EditAnywhere, Category = BrakeSetup, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/SodaSim.SodaVehicleWheelComponent"))
	FSubobjectReference ConnectedWheel;
};

/**
 * UWheelBrakeSimple
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UWheelBrakeSimple: public UWheelBrake
{
	friend UVehicleBrakeSystemSimpleComponent;

	GENERATED_UCLASS_BODY()

public:
	virtual void RequestByPressure(float InBar, double DeltaTime) override;
	virtual void RequestByTorque(float InTorque, double DeltaTime) override;
	virtual void RequestByRatio(float InRatio, double DeltaTime) override;
	virtual float GetTorque() const override;
	virtual float GetPressure() const override;
	virtual float GetLoad() const override;

protected:
	UPROPERTY()
	UVehicleBrakeSystemSimpleComponent* BrakeSystem = nullptr;
	FWheelBrakeSetup BrakeSetup;

	float CurrentBar = 0;
	float CurrentTorque = 0;
};


/**
 * UVehicleBrakeSystemBaseComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleBrakeSystemSimpleComponent : public UVehicleBrakeSystemBaseComponent
{
	GENERATED_UCLASS_BODY()

	/** [Nm/s] */
	UPROPERTY(EditAnywhere, Category = BrakeSystem, SaveGame, meta = (EditInRuntime))
	FInputRate MechanicalBrakeRate {3000, 3000};

	UPROPERTY(EditAnywhere, Category = BrakeSystem, SaveGame, meta = (EditInRuntime))
	TArray<FWheelBrakeSetup> WheelBrakesSetup;

	/** Allow set brake from default the UVhicleInputComponent */
	UPROPERTY(EditAnywhere, Category = BrakeSystem, SaveGame, meta = (EditInRuntime))
	bool bAcceptPedalFromVehicleInput = true;

public:
	virtual void RequestByAcceleration(float InAcceleration, double DeltaTime) override;
	virtual void RequestByForce(float InForce, double DeltaTime) override;
	virtual void RequestByRatio(float InRatio, double DeltaTime) override;
	virtual void RequestByPressure(float InBar, double DeltaTime) override;
	virtual UWheelBrake* GetWheel(int Ind) const override { return WheelBrakes[Ind]; }
	virtual UWheelBrake* GetWheel4WD(E4WDWheelIndex Ind) const override;
	virtual float ComputeFullTorqByRatio(float InRatio) override;

public:
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;


protected:
	UPROPERTY()
	TArray<UWheelBrakeSimple*> WheelBrakes;

	UPROPERTY()
	TArray<TWeakObjectPtr<UWheelBrakeSimple>> WheelBrakes4WD;

	float PedalPos = 0;
};
