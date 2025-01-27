// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleBrakeSystemComponent.h"

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

class FVehicleBrakeSystemSimpleComponent_Handler : public FObjectDatasetMongDBHandler
{
public:
public:
	FVehicleBrakeSystemSimpleComponent_Handler(UVehicleBrakeSystemSimpleComponent* Outer)
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

		Doc << "PedalPos" << Outer->GetPedalPos();
		auto Array = Doc << "Wheels" << open_array;
		for (auto& Wheel : Outer->GetWheelBrakes())
		{
			Array
				<< open_document
				<< "Torque" << Wheel->GetTorque()
				<< "Pressure" << Wheel->GetPressure()
				<< "Load" << Wheel->GetLoad()
				<< close_document;
		}

		Array << close_array;
		return true;
	}

	TWeakObjectPtr<UVehicleBrakeSystemSimpleComponent> Outer;
};

} // namespace mongodb
} // namespace soda