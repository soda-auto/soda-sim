// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"

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

class FVehicleInputAIComponent_Handler : public FObjectDatasetMongDBHandler
{
public:
	FVehicleInputAIComponent_Handler(UVehicleInputAIComponent* Outer)
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
			<< "Throttle" << Outer->GetInputState().Throttle
			<< "Brake" << Outer->GetInputState().Brake
			<< "Steering" << Outer->GetInputState().Steering
			<< "GearState" << int(Outer->GetInputState().GearState)
			<< "GearNum" << Outer->GetInputState().GearNum
			<< "bADModeEnbaled" << Outer->GetInputState().bADModeEnbaled
			<< "bSafeStopEnbaled" << Outer->GetInputState().bSafeStopEnbaled
			//<< "TargetLocationsNum" << Outer->TargetLocations.Num()
			<< "SideError" << Outer->GetSideError()
			<< "ObstacleDistance" << Outer->GetObstacleDistance();
		return true;
	}

	TWeakObjectPtr<UVehicleInputAIComponent> Outer;
};

} // namespace mongodb
} // namespace soda