// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <Eigen/Core>

#include "Actors/GhostVehicle/SpeedProfile/VehicleParameters.h"
#include "Actors/GhostVehicle/SpeedProfile/PointLimits.h"
#include "Actors/GhostVehicle/SpeedProfile/VelocityProfile.h"
#include "Actors/GhostVehicle/SpeedProfile/VehicleDynamicCalculator.h"
#include "Actors/GhostVehicle/SpeedProfile/TrajectoryPoint.h"


namespace SpeedProfile 
{

class FSpeedProfile 
{
private:
	using Trajectory = std::vector<FTrajectoryPoint>;

public:
	explicit FSpeedProfile(const FVehicleDynamicParams& dynamics)
		: vehicleDynamicsCalculator(dynamics)
	{}

	FSpeedProfile() = default;

	static double getMaxPossibleCurvature(double velocity, double friction);
	static double getHighestPossibleVelocity(double curvature, double friction);

	std::vector<double> getMaxVelocities(const FSpeedProfile::Trajectory& trajectory,
										 const FTrajectoryLimits& trajectoryLimits) const;

	FVelocityProfile calculateVelocities(double initialVelocity,
										const Trajectory& trajectory,
										const FTrajectoryLimits& mainLimits,
										bool isAggressive = true) const;

	std::vector<double> calculateAccelerations(const Trajectory& trajectory,
											   const std::vector<double>& velocity) const;

	FVelocityProfile immediateBraking(
		double initialVelocity,
		const FSpeedProfile::Trajectory& trajectory,
		const FTrajectoryLimits& trajectoryLimits
	) const noexcept;

private:
	FVelocityProfile getAdaptiveSpeedProfile(
		double initialVelocity,
		const FSpeedProfile::Trajectory& trajectory,
		const FTrajectoryLimits& mainLimits) const noexcept;

	FVelocityProfile getAgressiveSpeedProfile(
		double initialVelocity,
		const FSpeedProfile::Trajectory& trajectory,
		const FTrajectoryLimits& mainLimits
	) const noexcept;

	double velocityChange(const FTrajectoryPoint& start,
						  const FTrajectoryPoint& finish,
						  double velocity,
						  double acceleration) const noexcept;

	FVelocityProfile adaptiveBraking(
		double initialVelocity,
		const FSpeedProfile::Trajectory& trajectory,
		const FTrajectoryLimits& mainLimits
	) const;

	std::vector<double> reverseBraking(const std::vector<double>& acceleratedVelocities,
									   const Trajectory& trajectory,
									   const FTrajectoryLimits& trajectoryLimits) const;

	std::vector<double> immediateAcceleration(double initialVelocity,
											  const std::vector<double>& brakingVelocities,
											  const Trajectory& trajectory,
											  const FTrajectoryLimits& trajectoryLimits) const;

private:
	FVehicleDynamicsCalculator vehicleDynamicsCalculator;
};

} // namespace SpeedProfile
