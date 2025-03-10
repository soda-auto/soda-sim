// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.
#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Sensors/Generic/GenericWheeledVehicleSensor.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"

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

class FGenericWheeledVehicleSensor_Handler : public FObjectDatasetMongDBHandler
{
public:
	FGenericWheeledVehicleSensor_Handler(UGenericWheeledVehicleSensor* Outer)
		: FObjectDatasetMongDBHandler(Outer)
		, Outer(Outer)
	{
		check(Outer);
	}

	virtual void CreateObjectDescription(bsoncxx::builder::stream::document& Description) override
	{
		FObjectDatasetMongDBHandler::CreateObjectDescription(Description);
	}

	virtual bool Sync(bsoncxx::builder::stream::document& Doc) override
	{
		if (!Outer.IsValid())
		{
			return false;
		}

		auto DriveMode = Outer->GetVehicleDriver() ? Outer->GetVehicleDriver()->GetDriveMode() : ESodaVehicleDriveMode::Manual;
		auto GearState = Outer->GetGearBox() ? Outer->GetGearBox()->GetGearState() : EGearState::Neutral;
		int GearNum = Outer->GetGearBox() ? Outer->GetGearBox()->GetGearNum() : 0;
		//float Steer = WheeledVehicle->IsXWDVehicle(4)
		//	? (WheeledVehicle->GetWheelByIndex(EWheelIndex::FL)->Steer + WheeledVehicle->GetWheelByIndex(EWheelIndex::FR)->Steer) / 2 
		//	: 0;

		Doc
			<< "DriveMode" << int(DriveMode)
			<< "GearState" << int(GearState)
			<< "GearNum" << GearNum;
		//	<< "bIs4WDVehicle" << WheeledVehicle->Is4WDVehicle()
		//	<< "Steer" << Steer;
		auto WheelsArray = Doc << "Wheels" << open_array;
		for (auto& Wheel : Outer->GetWheeledVehicle()->GetWheelsSorted())
		{
			WheelsArray
				<< open_document
				<< "WheelIndex" << int(Wheel->GetWheelIndex())
				<< "ReqTorq" << Wheel->ReqTorq
				<< "ReqBrakeTorque" << Wheel->ReqBrakeTorque
				<< "ReqSteer" << Wheel->ReqSteer
				<< "Steer" << Wheel->Steer
				<< "Pitch" << Wheel->Pitch
				<< "AngularVelocity" << Wheel->AngularVelocity
				<< "SuspensionOffset" << open_array << Wheel->SuspensionOffset2.X << Wheel->SuspensionOffset2.Y << Wheel->SuspensionOffset2.Z << close_array
				<< "Slip" << open_array << Wheel->Slip.X << Wheel->Slip.Y << close_array
				<< close_document;
		}
		WheelsArray << close_array;
		return true;
	}

	TWeakObjectPtr<UGenericWheeledVehicleSensor> Outer;
};

} // namespace mongodb
} // namespace soda