// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <Eigen/Core>
#include <vector>


namespace SpeedProfile 
{

struct FVelocityProfile 
{
	std::vector<double> velocities;
	bool isSafe = true;
	double inconsistency = 0;
};

} // namespace SpeedProfile
