// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "SodaVehicle_Handler.hpp"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

namespace soda
{
namespace mongodb
{

class FSodaWheeledVehicle_Handler : public FSodaVehicle_Handler
{
public:
	FSodaWheeledVehicle_Handler(ASodaWheeledVehicle* Outer)
		: FSodaVehicle_Handler(Outer)
	{
	}

	virtual void CreateObjectDescription(bsoncxx::builder::stream::document& Description) override
	{
		FSodaVehicle_Handler::CreateObjectDescription(Description);

		ASodaWheeledVehicle* WheeledVehicle = Cast<ASodaWheeledVehicle>(Outer);
		if (!IsValid(WheeledVehicle))
		{
			return;
		}

		if (WheeledVehicle->GetWheeledComponentInterface())
		{
			Description
				<< "TrackWidth" << WheeledVehicle->GetWheeledComponentInterface()->GetTrackWidth()
				<< "WheelBaseWidth" << WheeledVehicle->GetWheeledComponentInterface()->GetWheelBaseWidth()
				<< "Mass" << WheeledVehicle->GetWheeledComponentInterface()->GetVehicleMass();

			bsoncxx::builder::stream::array WheelsArray;
			for (int i = 0; i < WheeledVehicle->GetWheelsSorted().Num(); ++i)
			{
				const auto& Wheel = WheeledVehicle->GetWheelsSorted()[i];
				WheelsArray << open_document
					//<< "4wdInd" << int(Wheel->WheelIndex)
					<< "Radius" << Wheel->Radius
					<< "Location" << open_array << Wheel->RestingLocation.X << Wheel->RestingLocation.Y << Wheel->RestingLocation.Z << close_array
					<< close_document;
			}
			Description << "Wheels" << WheelsArray;
		}
	}

	virtual bool Sync(bsoncxx::builder::stream::document& Doc) override
	{
		if (!FSodaVehicle_Handler::Sync(Doc))
		{
			return false;
		}

		ASodaWheeledVehicle* WheeledVehicle = Cast<ASodaWheeledVehicle>(Outer.Get());
		if (!IsValid(WheeledVehicle))
		{
			return false;
		}

		if (UVehicleInputComponent* Input = WheeledVehicle->GetActiveVehicleInput())
		{
			Doc << "Inputs" 
				<< open_document
				<< "Break" << Input->GetInputState().Brake
				<< "Steer" << Input->GetInputState().Steering
				<< "Throttle" << Input->GetInputState().Throttle
				<< "Gear" << int(Input->GetInputState().GearState)
				<< close_document;
		}

		if (WheeledVehicle->GetWheeledComponentInterface())
		{
			bsoncxx::builder::stream::array WheelsArray;
			for (int i = 0; i < WheeledVehicle->GetWheelsSorted().Num(); ++i)
			{
				const auto& Wheel = WheeledVehicle->GetWheelsSorted()[i];
				WheelsArray 
					<< open_document
					<< "Ind" << int(Wheel->GetWheelIndex())
					<< "AngVel" << Wheel->ResolveAngularVelocity()
					<< "ReqTorq" << Wheel->ReqTorq
					<< "ReqBrakeTorq" << Wheel->ReqBrakeTorque
					<< "Steer" << Wheel->Steer
					<< "Sus" << Wheel->SuspensionOffset2.Z
					<< "LongSlip" << Wheel->Slip.X
					<< "LatSlip" << Wheel->Slip.Y
					<< close_document;
			}

			Doc << "Wheels" << WheelsArray;
		}

		return true;
	}
};

} // namespace mongodb
} // namespace soda