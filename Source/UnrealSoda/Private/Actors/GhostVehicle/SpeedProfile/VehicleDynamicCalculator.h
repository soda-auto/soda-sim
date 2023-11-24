// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Actors/GhostVehicle/SpeedProfile/VehicleParameters.h"
#include "Actors/GhostVehicle/SpeedProfile/PointLimits.h"
#include "Actors/GhostVehicle/SpeedProfile/VelocityProfile.h"
#include "Actors/GhostVehicle/SpeedProfile/TrajectoryPoint.h"


namespace SpeedProfile 
{

class FVehicleDynamicsCalculator 
{
public:
	struct DecelerationResult 
	{
		double deceleration;
		bool isSafe = true;
	};

	explicit FVehicleDynamicsCalculator(const FVehicleDynamicParams& dynamics)
		: vehicleDynamics(dynamics)
	{}

	FVehicleDynamicsCalculator() = default;

	const FVehicleDynamicParams& getVehicleDynamics() const noexcept { return vehicleDynamics; }

	double getMaxAcceleration(double velocity,
							  const FTrajectoryPoint& point,
							  const FPointLimits& pointLimits) const;

	DecelerationResult getMaxDeceleration(double velocity,
										  const FTrajectoryPoint& point,
										  const FPointLimits& pointLimits) const;

private:
	FVehicleDynamicParams vehicleDynamics;
};

} // namespace SpeedProfile
