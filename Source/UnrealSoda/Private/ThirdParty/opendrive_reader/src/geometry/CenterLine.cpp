/*
 * ----------------- BEGIN LICENSE BLOCK ---------------------------------
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * ----------------- END LICENSE BLOCK -----------------------------------
 */

#include "opendrive/geometry/CenterLine.hpp"

//#include <boost/array.hpp>
//#include <boost/math/tools/rational.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <float.h>

namespace opendrive {
namespace geometry {

geometry::DirectedPoint CenterLine::eval(double s, bool applyLateralOffset) const
{
  if (s < 0.)
  {
    // as the curve is only defined betwen the range [0 , length], we return the line evaluated at s = 0
    return eval(0.);
  }

  for (auto it = geometry.rbegin(); it != geometry.rend(); it++)
  {
    if (s >= (*it)->GetStartOffset())
    {
      auto directedPoint = (*it)->PosFromDist(s - (*it)->GetStartOffset());

      // This is parametrized as the landmarks positions do not take into account the lateral offset
      if (applyLateralOffset)
      {
        directedPoint.ApplyLateralOffset(calculateOffset(s));
      }
      return directedPoint;
    }
  }

  auto &lastGeometry = geometry.back();
  return lastGeometry->PosFromDist(s - lastGeometry->GetStartOffset());
}

double  CenterLine::evalElevation(double s) const
{
    for (int i = 0; i < roadProfiles.elevation_profile.size(); ++i)
    {
        auto & el = roadProfiles.elevation_profile[i];
        double s0 = el.start_position;
        double s1 = (i < (roadProfiles.elevation_profile.size() - 1)) ? roadProfiles.elevation_profile[i + 1].start_position : FLT_MAX;
        if (s < s1)
        {
            return evaluate_polynomial(el.elevation, el.slope, el.vertical_curvature, el.curvature_change, s - s0);
        }
    }
    return 0;
}


std::vector<double> CenterLine::samplingPoints() const
{
  std::vector<double> points;

  points.push_back(length);
  double const maxError = 1e-2; // max curve sampling error expected

  for (auto it = geometry.rbegin(); it != geometry.rend(); it++)
  {
    if ((*it)->GetType() == ::opendrive::GeometryType::LINE)
    {
      // only start and end point
    }
    else if ((*it)->GetType() == ::opendrive::GeometryType::ARC)
    {
      auto arc = static_cast<::opendrive::geometry::GeometryArc *>((*it).get());
      double theta = fabs(arc->GetCurvature()) * (*it)->GetLength();

      if (theta < 1e-2)
      {
        points.insert(points.begin(), (*it)->GetStartOffset() + 0.5 * (*it)->GetLength());
      }
      else
      {
        double c = fabs(arc->GetCurvature());
        double dsmax = 2.0 / c * acos(1.0 - c * maxError);
        int segments = static_cast<int>((*it)->GetLength() / dsmax);
        for (auto k = segments - 1; k > 0; --k)
        {
          double r = static_cast<double>(k) / static_cast<double>(segments);
          points.insert(points.begin(), (*it)->GetStartOffset() + r * (*it)->GetLength());
        }
      }
    }
    else if ((*it)->GetType() == ::opendrive::GeometryType::POLY3)
    {
      int segments = 10;
      for (auto k = segments - 1; k > 0; --k)
      {
        double r = static_cast<double>(k) / static_cast<double>(segments);
        points.insert(points.begin(), (*it)->GetStartOffset() + r * (*it)->GetLength());
      }
    }
    else if ((*it)->GetType() == ::opendrive::GeometryType::PARAMPOLY3)
    {
      int segments = 10;
      for (auto k = segments - 1; k > 0; --k)
      {
        double r = static_cast<double>(k) / static_cast<double>(segments);
        points.insert(points.begin(), (*it)->GetStartOffset() + r * (*it)->GetLength());
      }
    }
    else
    {
      // nothing to do
    }

    points.insert(points.begin(), (*it)->GetStartOffset());
  }

  return points;
}

double CenterLine::calculateOffset(double s) const
{
  if (s < 0.)
  {
    return calculateOffset(0.);
  }

  for (auto it = offsetVector.rbegin(); it != offsetVector.rend(); it++)
  {
    if (s >= it->s)
    {
      return evaluate_polynomial(it->a, it->b, it->c, it->d, s - it->s);
    }
  }

  return 0.0;
}

bool generateCenterLine(RoadInformation &roadInfo, CenterLine &centerLine)
{
  bool ok = true;

  centerLine.geometry.clear();
  centerLine.length = roadInfo.attributes.length;
  centerLine.offsetVector = roadInfo.lanes.lane_offset;
  centerLine.roadProfiles = roadInfo.road_profiles;

  // Add geometry information
  for (auto &geometry_attribute : roadInfo.geometry_attributes)
  {
    Point start(geometry_attribute->start_position_x, geometry_attribute->start_position_y);

    try
    {
      switch (geometry_attribute->type)
      {
        case ::opendrive::GeometryType::ARC:
        {
          auto arc = static_cast<GeometryAttributesArc *>(geometry_attribute.get());
          centerLine.geometry.emplace_back(std::make_unique<geometry::GeometryArc>(
            arc->start_position, arc->length, arc->heading, start, arc->curvature));
          break;
        }
        case ::opendrive::GeometryType::LINE:
        {
          auto line = static_cast<GeometryAttributesLine *>(geometry_attribute.get());
          centerLine.geometry.emplace_back(
            std::make_unique<geometry::GeometryLine>(line->start_position, line->length, line->heading, start));
          break;
        }
        break;
        case ::opendrive::GeometryType::SPIRAL:
        {
          // currently not yet supported
          std::cerr << "generateCenterLine() spirals are currently not supported yet\n";
          ok = false;
          break;
        }
        break;
        case ::opendrive::GeometryType::POLY3:
        {
          auto poly3 = static_cast<GeometryAttributesPoly3 *>(geometry_attribute.get());
          centerLine.geometry.emplace_back(std::make_unique<geometry::GeometryPoly3>(
            poly3->start_position, poly3->length, poly3->heading, start, poly3->a, poly3->b, poly3->c, poly3->d));
          break;
        }
        break;
        case ::opendrive::GeometryType::PARAMPOLY3:
        {
          auto paramPoly3 = static_cast<GeometryAttributesParamPoly3 *>(geometry_attribute.get());
          centerLine.geometry.emplace_back(std::make_unique<geometry::GeometryParamPoly3>(paramPoly3->start_position,
                                                                                          paramPoly3->length,
                                                                                          paramPoly3->heading,
                                                                                          start,
                                                                                          paramPoly3->aU,
                                                                                          paramPoly3->bU,
                                                                                          paramPoly3->cU,
                                                                                          paramPoly3->dU,
                                                                                          paramPoly3->aV,
                                                                                          paramPoly3->bV,
                                                                                          paramPoly3->cV,
                                                                                          paramPoly3->dV));
          break;
        }
        break;
        default:
          break;
      }
    }
    catch (...)
    {
      std::cerr << "generateCenterLine() Invalid geometry definition\n";
      ok = false;
    }
  }

  return ok;
}
}
}
