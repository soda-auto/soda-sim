// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Actors/GhostVehicle/SpeedProfile/Math.h"
#include "Actors/GhostVehicle/SpeedProfile/VehicleDynamicCalculator.h"


namespace SpeedProfile 
{

double FVehicleDynamicsCalculator::getMaxAcceleration(
	double velocity,
	const FTrajectoryPoint& point,
	const FPointLimits& pointLimits
) const 
{
	double angularVelocity = velocity / vehicleDynamics.r_wheel;
	double torqueOnMaxPower = vehicleDynamics.max_power / angularVelocity;
	double torque = std::min(vehicleDynamics.max_torque, torqueOnMaxPower);

	if (angularVelocity >= vehicleDynamics.omega_nul_torque) 
	{
		torque = 0.0;
	} 
	else if (angularVelocity >= vehicleDynamics.omega_max_power) 
	{
		torque = torqueOnMaxPower * 
				 (vehicleDynamics.omega_nul_torque - angularVelocity) / 
				 (vehicleDynamics.omega_nul_torque - vehicleDynamics.omega_max_power);
	}

	double force = torque / vehicleDynamics.r_wheel;
	double dragForce = vehicleDynamics.drag_force_coeff * math::square(velocity) + vehicleDynamics.rolling_resistance;
	double maxLongitudinalAcceleration = (force - dragForce) / vehicleDynamics.car_mass;

	constexpr double epsilon_ = 0.001;
	if (maxLongitudinalAcceleration < epsilon_) 
	{
		return 0.0;
	}

	double accelerationRadial = std::abs(point.curvature) * math::square(velocity);
	double accelerationFriction = pointLimits.mu * math::grav_acc;
	if (accelerationRadial > accelerationFriction + epsilon_) 
	{
		return 0.0;
	}

	//get the maximum longitudinal acceleration value (kammscher kreis)
	double accelerationLongitudinal =
		std::sqrt(std::max((math::square(accelerationFriction) - math::square(accelerationRadial))
						   * math::square(maxLongitudinalAcceleration) / math::square(accelerationFriction), 0.0));

	accelerationLongitudinal = std::min(accelerationLongitudinal, pointLimits.maxAcceleration);
	accelerationLongitudinal = std::min(accelerationLongitudinal, accelerationFriction);
	return std::max(0.0, accelerationLongitudinal);
}

FVehicleDynamicsCalculator::DecelerationResult FVehicleDynamicsCalculator::getMaxDeceleration(
	double velocity,
	const FTrajectoryPoint& point,
	const FPointLimits& pointLimits
) const 
{
	double accelerationFriction = pointLimits.mu * math::grav_acc;
	double accelerationRadial = std::abs(point.curvature) * math::square(velocity);
	double diff = math::square(accelerationFriction) - math::square(accelerationRadial);
	DecelerationResult result;
	if (diff < 0) 
	{
		result.deceleration = 0.0;
		result.isSafe = false;
	} 
	else 
	{
		result.deceleration = std::min(std::sqrt(diff), pointLimits.maxDeceleration); // TODO: Check formula;
		result.isSafe = true;
	}
	return result;
}

} // namespace SpeedProfile
