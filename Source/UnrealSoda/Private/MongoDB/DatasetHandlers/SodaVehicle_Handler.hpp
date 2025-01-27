// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/Vehicles/SodaVehicle.h"

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

class FSodaVehicle_Handler : public FObjectDatasetMongDBHandler
{
public:
	FSodaVehicle_Handler(ASodaVehicle* Outer)
		: FObjectDatasetMongDBHandler(Outer)
		, Outer(Outer)
	{
		check(Outer);
	}

	virtual void CreateObjectDescription(bsoncxx::builder::stream::document& Description) override
	{
		FObjectDatasetMongDBHandler::CreateObjectDescription(Description);

		if (!Outer.IsValid())
		{
			return;
		}

		const FExtent& Extent = Outer->GetVehicleExtent();
		Description
			<< "Extents" 
			<< open_document
			<< "Forward" << Extent.Forward
			<< "Backward" << Extent.Backward
			<< "Left" << Extent.Left
			<< "Right" << Extent.Right
			<< "Up" << Extent.Up
			<< "Down" << Extent.Down
			<< close_document;
	}

	virtual bool Sync(bsoncxx::builder::stream::document& Doc) override
	{
		if (!Outer.IsValid())
		{
			return false;
		}

		const FVehicleSimData& SimData = Outer->GetSimData();
		FVector Location = SimData.VehicleKinematic.Curr.GlobalPose.GetLocation();
		FRotator Rotation = SimData.VehicleKinematic.Curr.GlobalPose.GetRotation().Rotator();
		FVector Vel = SimData.VehicleKinematic.Curr.GetLocalVelocity();
		FVector Acc = SimData.VehicleKinematic.Curr.GetLocalAcceleration();
		FVector AngVel = SimData.VehicleKinematic.Curr.AngularVelocity;

		Doc
			<< "VehicleData" 
			<< open_document
			<< "SimTsUs" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SimData.SimulatedTimestamp))
			<< "RenderTsUs" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SimData.RenderTimestamp))
			<< "Step" << SimData.SimulatedStep
			<< "Loc" << open_array << Location.X << Location.Y << Location.Z << close_array
			<< "Rot" << open_array << Rotation.Roll << Rotation.Pitch << Rotation.Yaw << close_array
			<< "Bel" << open_array << Vel.X << Vel.Y << Vel.Z << close_array
			<< "Acc" << open_array << Acc.X << Acc.Y << Acc.Z << close_array
			<< "AngVel" << open_array << AngVel.X << AngVel.Y << AngVel.Z << close_array
			<< close_document;

		return true;
	}

	TWeakObjectPtr<ASodaVehicle> Outer;
};

} // namespace mongodb
} // namespace soda