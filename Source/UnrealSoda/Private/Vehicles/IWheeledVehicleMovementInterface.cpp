// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"


float IWheeledVehicleMovementInterface::GetWheelBaseWidth() const
{
	ASodaWheeledVehicle* WheeledVehicle = GetWheeledVehicle();

	if (WheeledVehicle->IsXWDVehicle(4))
	{
		return FMath::Abs(WheeledVehicle->GetWheelByIndex(EWheelIndex::FL)->RestingLocation.X - WheeledVehicle->GetWheelByIndex(EWheelIndex::RL)->RestingLocation.X);
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

	if (WheeledVehicle->IsXWDVehicle(4))
	{
		return FMath::Abs(WheeledVehicle->GetWheelByIndex(EWheelIndex::FL)->RestingLocation.Y - WheeledVehicle->GetWheelByIndex(EWheelIndex::FR)->RestingLocation.Y);
	}
	else
	{
		checkf(false, TEXT("Nessessory override GetTrackWidth() for not 4WD vehicle"));
		return 100;
	}
	
}

void IWheeledVehicleMovementInterface::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	for (auto & Wheel : GetWheeledVehicle()->GetWheelsSorted())
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
