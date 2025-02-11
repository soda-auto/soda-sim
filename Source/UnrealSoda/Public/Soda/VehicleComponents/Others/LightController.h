// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "LightController.generated.h"

class ULightSetComponent;
class UVehicleLightItem;



USTRUCT(BlueprintType)
struct FLinkToLight
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LinkToLightComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bIsActive = false;

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
	
	/** Testing all light set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Test", meta = (EditInRuntime, ReactivateComponent))
	TMap<FName, FLinkToLight> LightTestingPrms;



	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TurnFL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TurnFR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TurnRL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TurnRR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_BrakeL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_BrakeR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_HighBeamL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_HighBeamR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_LowBeamL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_LowBeamR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TailFL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TailFR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TailRL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_TailRR;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_ReverseL;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleLightItem> LightItem_ReverseR;





	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
//	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

	void GetLinksToLights();

	UPROPERTY()
	ULightSetComponent* LightSet;

	float LastTimeBlinked = 0;
	float bBlinkAllwd = true;


	void EnableLight(const FLinkToLight& LightItem);
	void EnableLight(TObjectPtr<UVehicleLightItem> LightItem, bool bNewActive);
};