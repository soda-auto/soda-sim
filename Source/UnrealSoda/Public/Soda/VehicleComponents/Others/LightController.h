// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "LightController.generated.h"

class ULightSetComponent;
class UVehicleLightItem;

USTRUCT(BlueprintType)
struct FLightTesting
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTurnL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTurnR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestBrakeL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestBrakeR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestHighBeamL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestHighBeamR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestLowBeamL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestLowBeamR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTailFL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTailFR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTailRL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestTailRR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestReverseL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
		bool TestReverseR = false;
};



/**
 * ULightController
 * Vehicle component that controls the activation of vehicle's lights if vehicle model supports it. Requires configurated LightSet component to work
 */

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULightController : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()
	
public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.LightSet"))
	FSubobjectReference LinkToLightSet;

	/** True to allow side lights activation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	bool bAllowSideLights = true;

	/** True to allow turning lights activation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	bool bAllowTurnLights = true;

	/** Time between blinks of turning lights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	float TimeBetweenTurnLightActivation = 0.2;	

	/** Braking pedal position threshold to activate braking lights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	float BrakeLightPdlPosnThd = 0.05;

	/** True to enable high beam lights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	bool bEnableHighBeam = false;

	/** True to enable low beam lights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Setup", meta = (EditInRuntime))
	bool bEnableLowBeam = false;

	/** True to enable testing configuration, requests from InputComponent will be ignored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
	bool bDoTesting = false;
	
	/** Testing configuration parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime))
	FLightTesting LigthTestingPrms;
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
//	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;


	UPROPERTY()
	ULightSetComponent* LightSet;

	float LastTimeBlinked = 0;
	float bBlinkAllwd = true;


	void EnableLightByName(const TMap<FName, UVehicleLightItem*>& AllLights, FName LightName, bool bNewActivation);

};