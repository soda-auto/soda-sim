// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once
#include "Actors/GhostVehicle/SpeedProfile/TrajectoryPoint.h"
#include "Actors/GhostVehicle/SpeedProfile/VehicleParameters.h"

#include <vector>


namespace SpeedProfile 
{

class FSpeedEstimator 
{
public:
	FSpeedEstimator(
		double baseMu,
		double maxMu,
		const FVehicleDynamicParams& vehicleDynamicParams,
		const std::vector<Eigen::Vector2d>& points,
		bool bIsLooped);

   // std::vector<trajectory_msgs::msg::MuEstimationItem> getMuMap() const;
	double estimateTime() const;

private:
	void calculateVelocities();
	double estimateMu(int index) const ;

private:
	const double baseMu;
	const double maxMu;
	const FVehicleDynamicParams& vehicleDynamicParams;

	const std::vector<FTrajectoryPoint> trajectory;
	const bool bIsLooped;

	std::vector<double> velocities;
	std::vector<double> accelerations;
};

} // namespace SpeedProfile
