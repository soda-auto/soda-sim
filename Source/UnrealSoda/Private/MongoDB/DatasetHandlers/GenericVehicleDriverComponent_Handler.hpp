// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Drivers/GenericVehicleDriverComponent.h"

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

class FGenericVehicleDriverComponent_Handler : public FObjectDatasetMongDBHandler
{
public:
	FGenericVehicleDriverComponent_Handler(UGenericVehicleDriverComponent* Outer)
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
			<< "bVapiPing" << Outer->IsADPing()
			<< "bADModeEnbaled" << Outer->IsADModeEnbaled()
			<< "bSafeStopEnbaled" << Outer->IsSafeStopEnbaled()
			<< "GearState" << int(Outer->GetGearState())
			<< "GearNum" << int(Outer->GetGearNum())
			<< "Control" << open_document
				<< "SteerReq" << Outer->GetControl().SteerReq.ByRatio
				<< "DriveEffortReq" << Outer->GetControl().DriveEffortReq.ByRatio
				<< "TargetSpeedReq" << Outer->GetControl().TargetSpeedReq
				<< "GearStateReq" << int(Outer->GetControl().GearStateReq)
				<< "GearNumReq" << int(Outer->GetControl().GearNumReq)
				<< "SteerReqMode" << int(Outer->GetControl().SteerReqMode)
				<< "DriveEffortReqMode" << int(Outer->GetControl().DriveEffortReqMode)
				<< "TimestampUs" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(Outer->GetControl().Timestamp))
			<< close_document;
		return true;
	}

	TWeakObjectPtr<UGenericVehicleDriverComponent> Outer;
};

} // namespace mongodb
} // namespace soda