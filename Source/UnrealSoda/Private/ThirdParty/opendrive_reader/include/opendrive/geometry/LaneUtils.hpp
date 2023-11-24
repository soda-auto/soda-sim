/*
 * ----------------- BEGIN LICENSE BLOCK ---------------------------------
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * ----------------- END LICENSE BLOCK -----------------------------------
 */

#pragma once

#include <cmath>
#include "opendrive/types.hpp"

namespace opendrive {
namespace geometry {

enum class ContactPlace
{
  Overlap,
  LeftLeft,
  RightLeft,
  LeftRight,
  RightRight,
  None
};

ContactPlace contactPlace(Lane const &leftLane, Lane const &rightLane);

//bool near(const opendrive::Point & left, const opendrive::Point & right, double resolution = 1e-3);
//bool lanesOverlap(Lane const &leftLane, Lane const &rightLane, double const overlapMargin);
//void invertLaneAndNeighbors(LaneMap &laneMap, Lane &lane);
void checkAddSuccessor(Lane &lane, Lane const &otherLane);
void checkAddPredecessor(Lane &lane, Lane const &otherLane);

Id laneId(int roadId, int laneSectionIndex, int laneIndex);
}
}
