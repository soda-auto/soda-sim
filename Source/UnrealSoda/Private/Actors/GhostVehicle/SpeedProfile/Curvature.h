// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <vector>
#include <type_traits>
#include <iterator>
#include <Eigen/Core>


namespace SpeedProfile
{

struct FCurvatureInfo 
{
	double curve = 0;
	double arc_length = 0;
};

double calculateCurvature(const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3);
FCurvatureInfo calculateCurvatureInfo(const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3);
FCurvatureInfo calculateSimpleCurvatureInfo(const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3);

} // namespace SpeedProfile
