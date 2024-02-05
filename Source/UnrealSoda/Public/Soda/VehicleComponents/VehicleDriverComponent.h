// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "VehicleDriverComponent.generated.h"

class ISodaJoystickPlugin;
class IWheeledVehicleMovementInterface;
class ASodaWheeledVehicle;


/**
 * Current vehicle drive mode
 */
UENUM(BlueprintType)
enum class  ESodaVehicleDriveMode : uint8
{
	/** The vehicle is under a driver (human) control. See manual control sourse in the ESodaVehicleInputSource. */
	Manual = 0,

	/** The vehicle is under AD (autonomous driving) control. */
	AD = 1,

	/** The vehicle is under (autonomous driving) control, but something was wrong. */
	SafeStop = 2,

	/** The vehicle is under a driver (human) control, but it is ready to switch to AD control. */
	ReadyToAD = 3,
};


/** 
 * Convert ESodaVehicleDriveMode to string 
 */
FString ToString(ESodaVehicleDriveMode Mode);


/**
 * UVehicleDriverComponent is a prototipe of the Vehicle Driver Unit Controller.
 * UVehicleDriverComponent takes control from the driver's controls (steering wheel, gears, pedals, various buttons, etc.)  - 
 * usually this the VehicleInputComponent, but it can be anything. 
 * And must transfer control to various actuators (engines, steering rack, breaking system and more).
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleDriverComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

	/** if true IsADPing() always equal true. That is, if there is no ping with VAPI, sefstop will not be set*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bIsVehicleDriveDebugMode = false;

public:
	/** This function should be overloaded in inheritors. */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledLeftTurningLights() const { return false; }

	/** This function should be overloaded in inheritors. */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledRightTurningLights() const { return false; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledBrakeLight() const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledHeadlights() const { return false; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledDaytimeRunningLights() const { return false; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledReversLights() const;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsEnabledHorn() const { return false; }

	/** Is there a connection with the AD control unit. */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsADPing() const { return false; }

	/** AD unit issued a warning. */
	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual bool IsADWarning() const { return false; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual ESodaVehicleDriveMode GetDriveMode() const;

	/** Vehicle external LED. The color of the LED indicates the various statuses/warnings/errors of AD unit. */
	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual FColor GetLED() const;

	UFUNCTION(BlueprintCallable, Category = VehicleInput)
	virtual EGearState GetGearState() const { return EGearState::Neutral; }
};
