// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Sensors/Base/RacingSensor.h"

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

	class FRacingSensor_Handler : public FObjectDatasetMongDBHandler
	{
	public:
		FRacingSensor_Handler(URacingSensor* Outer)
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
				<< "bBorderIsValid" << Outer->GetSensorData().bBorderIsValid
				<< "bLapCounterIsValid" << Outer->GetSensorData().bLapCounterIsValid
				<< "CenterLineYaw" << Outer->GetSensorData().CenterLineYaw / M_PI * 180
				<< "CoveredDistanceCurrentLap" << Outer->GetSensorData().CoveredDistanceCurrentLap
				<< "CoveredDistanceFull" << Outer->GetSensorData().CoveredDistanceFull
				<< "LapCaunter" << Outer->GetSensorData().LapCaunter
				<< "LeftBorderOffset" << Outer->GetSensorData().LeftBorderOffset
				<< "RightBorderOffset" << Outer->GetSensorData().RightBorderOffset;
			return true;
		}

		TWeakObjectPtr<URacingSensor> Outer;
	};

} // namespace mongodb
} // namespace soda