// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/Actors/GhostPedestrian.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaApp.h"
#include "Soda/Actors/NavigationRoute.h"

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

class FGhostPedestrian_Handler : public FObjectDatasetMongDBHandler
{
public:
	FGhostPedestrian_Handler(AGhostPedestrian* Outer)
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

		FExtent Extent = USodaStatics::CalculateActorExtent(Outer.Get());
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

		FVector Location = Outer->GetActorLocation();
		FRotator Rotation = Outer->GetActorRotation();
		FVector Vel = FVector(Outer->GetCurrentVelocity(), 0, 0);
		//FVector AngVel = FVector(0, 0, 0);

		Doc
			<< "Ts" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SodaApp.GetSimulationTimestamp()))
			<< "Loc" << open_array << Location.X << Location.Y << Location.Z << close_array
			<< "Rot" << open_array << Rotation.Pitch << Rotation.Yaw << Rotation.Roll << close_array
			<< "Vel" << open_array << Vel.X << Vel.Y << Vel.Z << close_array;

		auto RouteDoc = Doc << "route" << open_document;
		if (const FTrajectoryPlaner::FWayPoint* WayPoint = Outer->TrajectoryPlaner.GetCurrentWayPoint())
		{
			if (WayPoint->OwndedRoute.IsValid())
			{
				RouteDoc << "name" << TCHAR_TO_UTF8(*WayPoint->OwndedRoute->GetName());
			}
			else
			{
				RouteDoc << "name" << "";
			}
		}
		else
		{
			RouteDoc << "name" << "";
		}
		RouteDoc << close_document;

		return true;
	}

	TWeakObjectPtr<AGhostPedestrian> Outer;
};

} // namespace mongodb
} // namespace soda