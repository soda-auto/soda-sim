// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/VehicleComponents/SodaVehicleWheel.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SodaWheeledVehicle.generated.h"

class ASodaWheeledVehicle;
class IWheeledVehicleMovementInterface;
class UVehicleDriverComponent;
class UVehicleBrakeSystemBaseComponent;
class UVehicleEngineBaseComponent;
class UVehicleSteeringRackBaseComponent;
class UVehicleHandBrakeBaseComponent;
class UVehicleDifferentialBaseComponent;
class UVehicleGearBoxBaseComponent;

/**
 * The ASodaWheeledVehicle Vehicle provides the template to implement a 4WD vehicle model.
 * ISodaWheeledVehicleSimulationInterface is responsible for tire and suspension modeling.
 */
UCLASS(abstract, config = Game, BlueprintType, Blueprintable)
class UNREALSODA_API ASodaWheeledVehicle : public ASodaVehicle
{
	GENERATED_UCLASS_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Scenario, SaveGame, meta = (EditInRuntime))
	//bool bReinitPositionIfStop = false;

	/** Vehicle body mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	class USkeletalMeshComponent* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	class USpringArmComponent* SpringArm = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	class UCameraComponent* FollowCamera = nullptr;

	/**  Pointer to VehicleMovement created from VehicleMovementClass. */
	UPROPERTY(BlueprintReadOnly, Category = Vehicle, meta = (AllowPrivateAccess = "true"))
	class UPawnMovementComponent *VehicleMovement = nullptr;


public:
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UVehicleInputComponent* GetActiveVehicleInput() const { return ActiveVehicleInput; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual void SetActiveVehicleInput(UVehicleInputComponent* InVehicleInput);

	UFUNCTION(BlueprintCallable, Category = Vehicle, meta = (ScenarioAction))
	virtual UVehicleInputComponent* SetActiveVehicleInputByName(FName VehicleInputName);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual UVehicleInputComponent* SetActiveVehicleInputByType(EVehicleInputType Type);

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual class UVehicleDriverComponent* GetVehicleDriver() { return VehicleDrivers.Num() ? VehicleDrivers[0] : nullptr; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual class UVehicleGearBoxBaseComponent* GetGearBoxe() { return GearBoxes.Num() ? GearBoxes[0] : nullptr; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual float GetEnginesLoad() const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual FWheeledVehicleState GetVehicleState() const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual USodaVehicleWheelComponent* GetWheel4WD(E4WDWheelIndex Ind) const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual const TArray<USodaVehicleWheelComponent*>& GetWheels() const { return Wheels; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual const TArray<USodaVehicleWheelComponent*>& GetWheels4WD() const { return Wheels4WD; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	bool Is4WDVehicle() const { return Wheels4WD.Num() == 4; }

	/** Compute the steering request as the arithmetic average of the steering request of the two front wheels. */
	virtual float GetReqSteer() const;

	/** Compute the actual steering as the arithmetic average of the actual steering of the two front wheels. */
	virtual float GetSteer() const;

public:
	virtual class USkeletalMeshComponent* GetMesh() const { return Mesh; }
	virtual IWheeledVehicleMovementInterface* GetWheeledComponentInterface() { return WheeledComponentInterface; }
	virtual IWheeledVehicleMovementInterface* GetWheeledComponentInterface() const { return WheeledComponentInterface; }

public:
	virtual bool DrawDebug(class UCanvas* Canvas, float& YL, float& YPos) override;
	virtual const FVehicleSimData& GetSimData() const override;
	virtual float GetForwardSpeed() const override;
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const override;
	virtual void GenerateDatasetDescription(soda::FBsonDocument& Doc) const override;

public:
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual UPawnMovementComponent* GetMovementComponent() const;
	virtual FVector GetVelocity() const override;

public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;

protected:
	virtual void ReRegistreVehicleComponents() override;
	virtual void SetDefaultMeshProfile();

protected:
	friend IWheeledVehicleMovementInterface;
	virtual void OnSetActiveMovement(IWheeledVehicleMovementInterface* InWheeledComponentInterface) { WheeledComponentInterface = InWheeledComponentInterface;}

	friend UVehicleInputComponent;
	void OnSetActiveVehicleInput(UVehicleInputComponent* InVehicleInput) { ActiveVehicleInput = InVehicleInput; }

protected:
	IWheeledVehicleMovementInterface* WheeledComponentInterface = nullptr;
	float SavedSpringArmLength = 0;

	UPROPERTY(Transient)
	UVehicleInputComponent * ActiveVehicleInput = nullptr;

	UPROPERTY(Transient)
	TArray<UVehicleInputComponent*> VehicleInputs;

	UPROPERTY(Transient)
	TArray<UVehicleDriverComponent*> VehicleDrivers;

	UPROPERTY(Transient)
	TArray<UVehicleGearBoxBaseComponent*> GearBoxes;

	UPROPERTY(Transient)
	TArray<UVehicleBrakeSystemBaseComponent*> VehicleBrakeSystems;

	UPROPERTY(Transient)
	TArray<UVehicleEngineBaseComponent*> VehicleEngines;

	UPROPERTY(Transient)
	TArray<UVehicleSteeringRackBaseComponent*> VehicleSteeringRacks;

	UPROPERTY(Transient)
	TArray<UVehicleHandBrakeBaseComponent*> VehicleHandBrakes;

	UPROPERTY(Transient)
	TArray<UVehicleDifferentialBaseComponent*> VehicleDifferentials;

	UPROPERTY(Transient)
	TArray<USodaVehicleWheelComponent*> Wheels;

	UPROPERTY(Transient)
	TArray<USodaVehicleWheelComponent*> Wheels4WD;

	int Zoom = 0;

	FCollisionResponseContainer DefCollisionResponseToChannel;
	ECollisionChannel DefCollisionObjectType;
};
