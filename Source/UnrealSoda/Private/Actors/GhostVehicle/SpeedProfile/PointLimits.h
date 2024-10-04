// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <Eigen/Core>
#include <vector>


namespace SpeedProfile 
{

struct FPointLimits 
{
	double maxVelocity = 42;
	double mu = 0.85;
	double maxAcceleration = 11;
	double maxDeceleration = 11;
	// default devbot values
};

using FTrajectoryLimits = std::vector<FPointLimits>;

} // namespace SpeedProfile
