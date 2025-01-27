// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/Actors/OpenDriveTool.h"

#include "opendrive/OpenDrive.hpp"
#include "opendrive/geometry/CenterLine.hpp"
#include "opendrive/geometry/GeometryGenerator.hpp"

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

	class FOpenDriveTool_Handler : public FObjectDatasetMongDBHandler
	{
	public:
		FOpenDriveTool_Handler(AOpenDriveTool* Outer)
			: FObjectDatasetMongDBHandler(Outer)
			, Outer(Outer)
		{
			check(Outer);
		}

		virtual void CreateObjectDescription(bsoncxx::builder::stream::document& Description) override
		{
			FObjectDatasetMongDBHandler::CreateObjectDescription(Description);

			if (!Outer .IsValid() || !Outer->GetOpenDriveData().IsValid())
			{
				return;
			}

			const auto &  OpenDriveData = Outer->GetOpenDriveData();

			// Store Lanes Marks
			auto laneArray = Description << "road_lanes_marks" << open_array;
			for (int road_id = 0; road_id < Outer->GetOpenDriveData()->roads.size(); ++road_id)
			{
				std::vector<opendrive::LaneMark> LaneMarkers;
				opendrive::geometry::GenerateRoadMarkLines(OpenDriveData->roads[road_id], LaneMarkers, Outer->MarkAccuracy / 100.f);

				auto laneDoc = laneArray << open_document;
				for (auto& MarkLine : LaneMarkers)
				{
					laneDoc << "type" << MarkLine.type;
					auto ptArray = laneDoc << "points" << open_array;
					for (auto& Pt : MarkLine.Edge)
					{
						FVector Pt2{ Pt.x * 100, -Pt.y * 100, Pt.z * 100 };
						ptArray << open_array << Pt2.X << Pt2.Y << Pt2.Z << close_array;
					}
					ptArray << close_array;
				}
				laneDoc << close_document;
			}
			laneArray << close_array;

			// Store XDOR
			FString FileName = Outer->FindXDORFile();
			FString FileContent;
			if (!FileName.IsEmpty())
			{
				if (FFileHelper::LoadFileToString(FileContent, *FileName))
				{
					Description << "xdor" << TCHAR_TO_UTF8(*FileContent);
				}
				else
				{
					UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::ScenarioBegin(); Can't load %s"), *FileName);
				}
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::ScenarioBegin(); Can't find XDOR file"));
			}
		}

		TWeakObjectPtr<AOpenDriveTool> Outer;
	};

} // namespace mongodb
} // namespace soda