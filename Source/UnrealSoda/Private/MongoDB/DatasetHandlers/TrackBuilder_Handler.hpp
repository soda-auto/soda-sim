// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/Actors/TrackBuilder.h"

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

	class FTrackBuilder_Handler : public FObjectDatasetMongDBHandler
	{
	public:
		FTrackBuilder_Handler(ATrackBuilder* Outer)
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

			Description << "json_file" << TCHAR_TO_UTF8(*Outer->LoadedFileName);
			Description << "json_is_valid" << Outer->bJSONLoaded;
			if (Outer->bJSONLoaded)
			{
				auto PointsArray = Description << "outside_points" << open_array;
				for (auto& Pt : Outer->OutsidePoints)
				{
					PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
				}
				PointsArray << close_array;

				PointsArray = Description << "inside_points" << open_array;
				for (auto& Pt : Outer->InsidePoints)
				{
					PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
				}
				PointsArray << close_array;

				PointsArray = Description << "centre_points" << open_array;
				for (auto& Pt : Outer->CentrePoints)
				{
					PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
				}
				PointsArray << close_array;

				Description << "lon" << Outer->RefPointLon;
				Description << "lat" << Outer->RefPointLat;
				Description << "alt" << Outer->RefPointAlt;
			}
		}

		TWeakObjectPtr<ATrackBuilder> Outer;
	};

} // namespace mongodb
} // namespace soda