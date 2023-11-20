// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Actors/GhostVehicle/SpeedProfile/SpeedEstimator.h"
#include "Actors/GhostVehicle/SpeedProfile/UnifiedSpeedProfile.h"
#include "Actors/GhostVehicle/SpeedProfile/Math.h"
#include "Actors/GhostVehicle/SpeedProfile/Curvature.h"

namespace SpeedProfile 
{

std::vector<FTrajectoryPoint> convertToTrajectoryPoints(
	const std::vector<Eigen::Vector2d> &path,
	bool loop ) 
{
	std::vector<FTrajectoryPoint> result;
	result.resize(path.size());
	if (path.size() < 3) {
		for (int i = 0; i < result.size(); ++i)
		{
			result[i].point = path[i];
		}
		if (path.size() == 2) 
		{
			Eigen::Vector2d dir = result[1].point - result[0].point;
			double yaw = std::atan2(dir.y(), dir.x());
			result[0].yaw = yaw;
			result[1].yaw = yaw;
		}
		return result;
	}

	for (int i = loop ? 0 : 1; i + (loop ? 0 : 1) < path.size(); i++) 
	{
		auto &rc = result.at(i);
		auto pp = path.at((i - 1 + path.size()) % path.size());
		auto pn = path.at((i + 1 + path.size()) % path.size());

		rc.point = path.at(i);
		Eigen::Vector2d dir = pn - pp;
		rc.yaw = ::std::atan2(dir.y(), dir.x());
		rc.curvature = calculateCurvature(pp, path.at(i), pn);
	}

	if (!loop)
	{
		result.front().point = path.front();
		result.front().curvature = result.at(1).curvature;
		result.front().yaw = result.at(1).yaw;

		result.back().point = path.back();
		result.back().curvature = result.at(result.size() - 2).curvature;
		result.back().yaw = result.at(result.size() - 2).yaw;
	}

	return result;
}


namespace 
{
	constexpr double epsilon = 1e-6; // m/s
}

double estimateTime(
	const std::vector<FTrajectoryPoint>& trajectory,
	const std::vector<double>& velocities,
	bool bIsLooped)
{
	assert(velocities.size() == trajectory.size());
	double loopTime = 0;
	if (bIsLooped)
	{
		double dist = (trajectory.front().point - trajectory.back().point).norm();
		double averageSpeed = 0.5 * (velocities.front() + velocities.back());
		if (averageSpeed < epsilon)
		{
			loopTime = std::numeric_limits<double>::infinity();
		}
		else 
		{
			loopTime += dist / averageSpeed;
		}
	}
	for (int i = 0; i + 1 < velocities.size(); ++i) 
	{
		double dist = (trajectory.at(i + 1).point - trajectory.at(i).point).norm();
		double averageSpeed = 0.5 * (velocities.at(i + 1) + velocities.at(i));
		if (averageSpeed < epsilon) 
		{
			loopTime = std::numeric_limits<double>::infinity();
		}
		else 
		{
			loopTime += dist / averageSpeed;
		}
	}
	return loopTime;
}

std::vector<double> estimateTimestamps(
	const std::vector<Eigen::Vector2d>& trajectory,
	const std::vector<double>& velocities) 
{
	std::vector<double> result;
	result.reserve(trajectory.size());
	result.emplace_back(0);
	double time = 0;
	for (int i = 0; i + 1 < velocities.size(); ++i) 
	{
		double dist = (trajectory.at(i + 1) - trajectory.at(i)).norm();
		double averageSpeed = 0.5 * (velocities.at(i + 1) + velocities.at(i));
		if (averageSpeed < epsilon) 
		{
			time = std::numeric_limits<double>::infinity();
		}
		else 
		{
			time += dist / averageSpeed;
		}
		result.emplace_back(time);
	}
	return result;
}

FSpeedEstimator::FSpeedEstimator(
	double baseMu,
	double maxMu,
	const FVehicleDynamicParams& vehicleDynamicParams,
	const std::vector<Eigen::Vector2d>& points,
	bool bIsLooped)
	: baseMu(baseMu),
	  maxMu(maxMu),
	  vehicleDynamicParams(vehicleDynamicParams),
	  trajectory(convertToTrajectoryPoints(points, bIsLooped)),
	  bIsLooped(bIsLooped)
{
	calculateVelocities();
}

void FSpeedEstimator::calculateVelocities() 
{
	struct LimitsCalculator 
	{
		inline FTrajectoryLimits getTrajectoryLimits(size_t size) const
		{
			return FTrajectoryLimits(size, pointLimit);
		}
		FPointLimits pointLimit;
	};

	LimitsCalculator limitsCalculator;

	limitsCalculator.pointLimit.maxAcceleration = 10.0;
	limitsCalculator.pointLimit.maxDeceleration = 10.0;
	limitsCalculator.pointLimit.maxVelocity = 100.0;
	limitsCalculator.pointLimit.mu = baseMu;

	auto trajectoryCpy = trajectory;
	int baseSize = trajectory.size();
	if (bIsLooped)
	{
		trajectoryCpy.insert(trajectoryCpy.end(), trajectory.begin(), trajectory.end());
		trajectoryCpy.insert(trajectoryCpy.end(), trajectory.begin(), trajectory.end());
	}
	auto mainLimits = limitsCalculator.getTrajectoryLimits(trajectoryCpy.size());

	SpeedProfile::FSpeedProfile speedProfile(vehicleDynamicParams);
	velocities = speedProfile.calculateVelocities(0.0, trajectoryCpy, mainLimits, true).velocities;
	accelerations = speedProfile.calculateAccelerations(trajectoryCpy, velocities);

	if (bIsLooped)
	{
		velocities.erase(velocities.begin(), velocities.begin() + baseSize);
		velocities.resize(baseSize);

		accelerations.erase(accelerations.begin(), accelerations.begin() + baseSize);
		accelerations.resize(baseSize);
	}
}

double FSpeedEstimator::estimateTime() const 
{
	return SpeedProfile::estimateTime(trajectory, velocities, bIsLooped);
}

double FSpeedEstimator::estimateMu(int index) const 
{
	constexpr double downForceCoefFront = 1.17;
	constexpr double downForceCoefRear = 1.91;
	constexpr double downForceCoef = 0.5 * (downForceCoefFront + downForceCoefRear);


	double velocity = velocities.at(index);
	double acceleration = accelerations.at(index);
	double curvature = trajectory.at(index).curvature;
	if (velocity < 10.0) 
	{
		return baseMu;
	}

	double accelerationLong = acceleration;
	double accelerationLat = curvature * math::square(velocity);

	double accelerationLatAdj =
		(vehicleDynamicParams.car_mass * math::grav_acc + downForceCoef * math::square(velocity)) *
			baseMu / vehicleDynamicParams.car_mass;

	if (acceleration >= 0.0) 
	{
		return std::min(maxMu, accelerationLatAdj / math::grav_acc);
	}

	double accelerationLongAdj = accelerationLatAdj +
		(vehicleDynamicParams.rolling_resistance + vehicleDynamicParams.drag_force_coeff * math::square(velocity)) / vehicleDynamicParams.car_mass;

	double mu = accelerationLatAdj * accelerationLongAdj /
		std::hypot(accelerationLatAdj * accelerationLong, accelerationLat * accelerationLongAdj) *
		std::hypot(accelerationLat, accelerationLong) /
		math::grav_acc;

	return std::min(maxMu, mu);
}

} // namespace SpeedProfile
