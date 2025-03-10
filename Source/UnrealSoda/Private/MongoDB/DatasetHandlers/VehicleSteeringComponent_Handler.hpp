// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"

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

class FVehicleSteeringRackSimpleComponent_Handler : public FObjectDatasetMongDBHandler
{
public:
	FVehicleSteeringRackSimpleComponent_Handler(UVehicleSteeringRackSimpleComponent* Outer)
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

		Doc
			<< "CurrentSteer" << Outer->GetCurrentSteer()
			<< "TargetSteer" << Outer->GetTargetSteerAng();
		return true;
	}

	TWeakObjectPtr<UVehicleSteeringRackSimpleComponent> Outer;
};

} // namespace mongodb
} // namespace soda


