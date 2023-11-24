// � 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Engine/Canvas.h"
#include "SodaJoystick.h"
#include "InputCoreTypes.h"
#include "Soda/SodaGameMode.h"

FString ToString(ESodaVehicleDriveMode Mode)
{
	switch(Mode)
	{
		case ESodaVehicleDriveMode::Manual: return "Manual";
		case ESodaVehicleDriveMode::AD: return "AD";
		case ESodaVehicleDriveMode::SafeStop: return "SafeStop";
		case ESodaVehicleDriveMode::ReadyToAD: return "ReadyToAD";	
		default: return "Unknown";
	}
}

UVehicleDriverComponent::UVehicleDriverComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Drivers");
	GUI.IcanName = TEXT("SodaIcons.Device");

	Common.UniqueVehiceComponentClass = UVehicleDriverComponent::StaticClass();
}

bool UVehicleDriverComponent::IsEnabledBrakeLight() const 
{
	for (auto & Wheel : GetWheeledVehicle()->GetWheels())
	{
		if(Wheel->ReqBrakeTorque > 0.1) return true;
	}
	return false;
}

bool UVehicleDriverComponent::IsEnabledReversLights() const 
{
	return (GetGear() == ENGear::Reverse);
}

ESodaVehicleDriveMode UVehicleDriverComponent::GetDriveMode() const 
{
	bool bSafeStopInput = false;
	bool bADModeInput = false;

	if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
	{
		bSafeStopInput = VehicleInput->bSafeStopInput;
		bADModeInput = VehicleInput->bADModeInput;
	}

	if(bSafeStopInput || (bADModeInput && !IsADPing())) return ESodaVehicleDriveMode::SafeStop;
	if(bADModeInput) return ESodaVehicleDriveMode::AD;
	return ESodaVehicleDriveMode::Manual; 	
}

FColor UVehicleDriverComponent::GetLED() const
{
	switch(GetDriveMode())
	{
		case ESodaVehicleDriveMode::Manual: return FColor(0, 255, 0);
		case ESodaVehicleDriveMode::AD: return FColor(0, 0, 255);
		case ESodaVehicleDriveMode::SafeStop: return FColor(255, 0, 0);
		case ESodaVehicleDriveMode::ReadyToAD: return FColor(0, 0, 255);
		default: return FColor(0, 0, 0);
	}
}
