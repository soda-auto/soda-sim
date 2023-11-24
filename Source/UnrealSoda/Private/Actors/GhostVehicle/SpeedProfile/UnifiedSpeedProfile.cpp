// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Actors/GhostVehicle/SpeedProfile/UnifiedSpeedProfile.h"
#include "Actors/GhostVehicle/SpeedProfile/Math.h"

namespace SpeedProfile 
{

double FSpeedProfile::getMaxPossibleCurvature(double velocity, double friction) 
{
	return friction * math::grav_acc / std::max(velocity * velocity, 1e-8);
}

double FSpeedProfile::getHighestPossibleVelocity(double curvature, double friction) 
{
	return std::sqrt(std::max(friction * math::grav_acc, 0.0) /
		std::max(std::abs(curvature), 1e-8));
}

std::vector<double> FSpeedProfile::getMaxVelocities(
	const FSpeedProfile::Trajectory &trajectory,
	const FTrajectoryLimits& trajectoryLimits
) const 
{
	assert(!trajectory.empty());
	assert(trajectory.size() == trajectoryLimits.size());

	std::vector<double> velocitites(trajectory.size(), vehicleDynamicsCalculator.getVehicleDynamics().max_velocity);

	for (size_t i = 0; i < trajectory.size(); ++i) {
		double possible = FSpeedProfile::getHighestPossibleVelocity(trajectory[i].curvature,
																   trajectoryLimits[i].mu);
		double limited = trajectoryLimits[i].maxVelocity;
		velocitites[i] = std::min(limited, possible);
	}

	return velocitites;
}

double FSpeedProfile::velocityChange(
	const FTrajectoryPoint& start,
	const FTrajectoryPoint& finish,
	double velocity,
	double acceleration
) const noexcept 
{
	double distance = (start.point - finish.point).norm();
	return std::sqrt(math::square(velocity) + 2 * acceleration * distance) - velocity;
}

FVelocityProfile FSpeedProfile::immediateBraking(
	double initialVelocity,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& trajectoryLimits
) const noexcept 
{
	FVelocityProfile profile;
	profile.velocities = getMaxVelocities(trajectory, trajectoryLimits);

	profile.velocities.front() = initialVelocity;

	for (size_t i = 0; i + 1 < profile.velocities.size(); ++i) 
	{
		FVehicleDynamicsCalculator::DecelerationResult decelerationResult =
			vehicleDynamicsCalculator.getMaxDeceleration(profile.velocities[i],
														  trajectory[i],
														  trajectoryLimits[i]);

		double deceleration = -1 * decelerationResult.deceleration;
		double delta = velocityChange(trajectory[i],
									  trajectory[i + 1],
									  profile.velocities[i],
									  deceleration);

		double nextVelocity = std::max(0.0, profile.velocities[i] + delta);

		double nextVelocityLimit = profile.velocities[i + 1];
		// TODO the last condition is used in city scenario only - need refactoring
		if (nextVelocity > std::max(0.0, nextVelocityLimit)) {
			profile.isSafe = false;
		}

		profile.velocities[i + 1] = nextVelocity;
	}

	return profile;
}

std::vector<double> FSpeedProfile::reverseBraking(
	const std::vector<double>& acceleratedVelocities,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& trajectoryLimits
) const 
{
	auto velocities = acceleratedVelocities;

	for (int i = static_cast<int>(velocities.size()) - 2; i >= 0; --i) {
		FVehicleDynamicsCalculator::DecelerationResult decelerationResult =
			vehicleDynamicsCalculator.getMaxDeceleration(velocities[i + 1], trajectory[i + 1], trajectoryLimits[i + 1]);
		double deceleration = decelerationResult.deceleration;
		double delta = velocityChange(trajectory[i], trajectory[i + 1], velocities[i + 1], deceleration);
		double prevVelocity = velocities[i + 1] + delta;

		velocities[i] = std::min(prevVelocity, velocities[i]);
	}
	return velocities;
}

std::vector<double> FSpeedProfile::immediateAcceleration(
	double initialVelocity,
	const std::vector<double>& brakingVelocities,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& trajectoryLimits
) const 
{
	std::vector<double> velocities = brakingVelocities;
	velocities.front() = initialVelocity;

	for (size_t i = 0; i < velocities.size() - 1; ++i) 
	{
		double acceleration = vehicleDynamicsCalculator.getMaxAcceleration(velocities[i],
																			trajectory[i],
																			trajectoryLimits[i]);

		double delta = velocityChange(trajectory[i],
									  trajectory[i + 1],
									  velocities[i],
									  acceleration);

		double nextVelocity = velocities[i] + delta;

		velocities[i + 1] = std::min(nextVelocity, velocities[i + 1]);
	}
	return velocities;
}

FVelocityProfile FSpeedProfile::getAgressiveSpeedProfile(
	double initialVelocity,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& mainLimits
) const noexcept 
{
	FVelocityProfile profile;
	profile.isSafe = true;

	std::vector<double> velocities = getMaxVelocities(trajectory, mainLimits);

	std::vector<double> forwardProfile = immediateAcceleration(initialVelocity, velocities, trajectory, mainLimits);
	std::vector<double> backwardProfile = reverseBraking(forwardProfile, trajectory, mainLimits);

	if (!forwardProfile.empty() && !backwardProfile.empty()) 
	{
		double diff = forwardProfile.front() - backwardProfile.front();
		constexpr double minVelocityDiff = 0.01;
		if (diff  > minVelocityDiff) 
		{
			profile.isSafe = false;
			profile.inconsistency = diff;
		}
	}

	profile.velocities.resize(backwardProfile.size());
	for (size_t i = 0; i < backwardProfile.size(); ++i) {
		profile.velocities[i] = std::min(forwardProfile[i], backwardProfile[i]);
	}

	return profile;
}

FVelocityProfile FSpeedProfile::getAdaptiveSpeedProfile(
	double initialVelocity,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& mainLimits
) const noexcept 
{
	FVelocityProfile profile;
	profile.isSafe = true;

	auto brakingProfile = adaptiveBraking(initialVelocity, trajectory, mainLimits);

	if (!brakingProfile.isSafe) 
	{
		profile.isSafe = false;
	}

	auto reverseVelocities = reverseBraking(getMaxVelocities(trajectory, mainLimits), trajectory, mainLimits);
	for (size_t i = 0; i < brakingProfile.velocities.size(); ++i)
	{
		brakingProfile.velocities[i] = std::max(brakingProfile.velocities[i],
												reverseVelocities[i]);
	}

	profile.velocities = immediateAcceleration(initialVelocity,
											   brakingProfile.velocities,
											   trajectory,
											   mainLimits);
	return profile;
}

static FTrajectoryLimits getProportionalLimits(double ratio, const FTrajectoryLimits& mainLimits) 
{
	assert(ratio <= 1.0);
	FTrajectoryLimits averagedLimits = mainLimits;
	for (std::size_t i = 0; i < mainLimits.size(); i++) 
	{
		averagedLimits[i].maxDeceleration = ratio * mainLimits[i].maxDeceleration;
	}
	return averagedLimits;
}

FVelocityProfile FSpeedProfile::adaptiveBraking(
	double velocity,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& mainLimits
) const 
{
	double ratio = 0.0;
	for (int i = 0; i < 10; i++) 
	{
		FTrajectoryLimits limits = getProportionalLimits(ratio, mainLimits);
		FVelocityProfile profile = immediateBraking(velocity, trajectory, limits);
		if (profile.isSafe) 
		{
			return profile;
		}
		ratio += 0.1;
	}

	return immediateBraking(velocity, trajectory, mainLimits);
}

FVelocityProfile FSpeedProfile::calculateVelocities(
	double initialVelocity,
	const FSpeedProfile::Trajectory& trajectory,
	const FTrajectoryLimits& mainLimits,
	bool isAggressive
) const 
{
	if (trajectory.empty()) 
	{
		return {};
	}
	assert(mainLimits.size() == trajectory.size());

	if (trajectory.size() == 1) 
	{
		bool isSafe = math::approx_cmp(initialVelocity, 0.0) == 0;
		return {{0.0}, isSafe};
	}

	return isAggressive
		? getAgressiveSpeedProfile(initialVelocity, trajectory, mainLimits)
		: getAdaptiveSpeedProfile(initialVelocity, trajectory, mainLimits);
}

std::vector<double> FSpeedProfile::calculateAccelerations(
	const FSpeedProfile::Trajectory& trajectory,
	const std::vector<double>& velocities
)const {
	assert(velocities.size() == trajectory.size());

	std::vector<double> accelerations(trajectory.size());

	double acceleration = 0.0;
	for (size_t i = 0; i < accelerations.size() - 1; ++i)
	{
		constexpr double epsilon_ = 0.0005;

		double distance = (trajectory[i].point - trajectory[i + 1].point).norm();
		if (distance > epsilon_) 
		{
			acceleration = (math::square(velocities[i + 1]) - math::square(velocities[i])) / (2.0 * distance);
		}
		accelerations[i + 1] = acceleration;
	}

	//TODO Check all the logic.
	if (accelerations.size() > 1) 
	{
		accelerations[0] = accelerations[1];
	}

	return accelerations;
}

} // namespace SpeedProfile
