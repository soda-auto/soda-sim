// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"


float IWheeledVehicleMovementInterface::GetWheelBaseWidth() const
{
	ASodaWheeledVehicle* WheeledVehicle = GetWheeledVehicle();

	if (WheeledVehicle->Is4WDVehicle())
	{
		return FMath::Abs(WheeledVehicle->GetWheelByIndex(EWheelIndex::Ind0_FL)->RestingLocation.X - WheeledVehicle->GetWheelByIndex(EWheelIndex::Ind2_RL)->RestingLocation.X);
	}
	else
	{
		checkf(false, TEXT("Nessessory override GetWheelBaseWidth() for no 4WD vehicle"));
		return 100;
	}
}

float IWheeledVehicleMovementInterface::GetTrackWidth() const
{
	ASodaWheeledVehicle* WheeledVehicle = GetWheeledVehicle();

	if (WheeledVehicle->Is4WDVehicle())
	{
		return FMath::Abs(WheeledVehicle->GetWheelByIndex(EWheelIndex::Ind0_FL)->RestingLocation.Y - WheeledVehicle->GetWheelByIndex(EWheelIndex::Ind1_FR)->RestingLocation.Y);
	}
	else
	{
		checkf(false, TEXT("Nessessory override GetTrackWidth() for no 4WD vehicle"));
		return 100;
	}
	
}

void IWheeledVehicleMovementInterface::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	for (auto & Wheel : GetWheeledVehicle()->GetWheels())
	{
		Wheel->ReqTorq = 0;
		Wheel->ReqBrakeTorque = 0;
	}
	GetWheeledVehicle()->PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);
}

void IWheeledVehicleMovementInterface::PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	GetWheeledVehicle()->PostPhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);
}

void IWheeledVehicleMovementInterface::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	GetWheeledVehicle()->PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);
}

void IWheeledVehicleMovementInterface::OnSetActiveMovement()
{
	GetWheeledVehicle()->OnSetActiveMovement(this);
}
