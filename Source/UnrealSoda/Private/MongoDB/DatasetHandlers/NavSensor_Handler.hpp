// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"

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

class FNavSensor_Handler : public FObjectDatasetMongDBHandler
{
public:
	FNavSensor_Handler(UNavSensor* Outer)
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
		
		FTransform WorldPose;
		FVector WorldVel;
		FVector LocalAcc;
		FVector Gyro;
		Outer->GetStoredBodyKinematic().CalcIMU(Outer->GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
		FRotator WorldRot = WorldPose.Rotator();
		FVector WorldLoc = WorldPose.GetTranslation();
		FVector LocVel = WorldRot.UnrotateVector(WorldVel);

		double Longitude = 0;
		double Latitude = 0;
		double Altitude = 0;

		const auto& LevelState = Outer->GetLevelState();
		if (LevelState)
		{
			Outer->GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Longitude, Latitude, Altitude);
			WorldRot = LevelState->GetLLConverter().ConvertRotationForward(WorldRot);
			WorldVel = LevelState->GetLLConverter().ConvertDirForward(WorldVel);
		}

		Doc
			<< "Position" << open_array << WorldLoc.X << WorldLoc.Y << WorldLoc.Z << close_array
			<< "Rotation" << open_array << WorldRot.Roll << WorldRot.Pitch << WorldRot.Yaw << close_array
			<< "LocVel" << open_array << LocVel.X << LocVel.Y << LocVel.Z << close_array
			<< "WorldVel" << open_array << WorldVel.X << WorldVel.Y << WorldVel.Z << close_array
			<< "AngularVel" << open_array << Gyro.X << Gyro.Y << Gyro.Z << close_array
			<< "Acc" << open_array << LocalAcc.X << LocalAcc.Y << LocalAcc.Z << close_array
			<< "Lon" << Longitude
			<< "Lat" << Latitude
			<< "Alt" << Altitude;
			
		return true;
	}

	TWeakObjectPtr<UNavSensor> Outer;
};

} // namespace mongodb
} // namespace soda