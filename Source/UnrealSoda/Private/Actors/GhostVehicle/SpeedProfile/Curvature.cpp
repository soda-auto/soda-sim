// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Actors/GhostVehicle/SpeedProfile/Curvature.h"

namespace SpeedProfile 
{

double calculateCurvature(
	const Eigen::Vector2d& p1,
	const Eigen::Vector2d& p2,
	const Eigen::Vector2d& p3) 
{
	Eigen::Vector2d a = p2 - p1;
	Eigen::Vector2d b = p3 - p2;
	Eigen::Vector2d c = p1 - p3;

	double aNorm2 = a.squaredNorm();
	double bNorm2 = b.squaredNorm();
	double cNorm2 = c.squaredNorm();

	auto normMinMax = std::minmax<double>({aNorm2, bNorm2, cNorm2});

	if (normMinMax.first < 1e-6 || 1e4 * normMinMax.first < normMinMax.second)
		return 0;

	double vdot = a.x() * b.y() - a.y() * b.x();
	return 2.0 * vdot / std::sqrt(aNorm2 * bNorm2 * cNorm2);
}

FCurvatureInfo calculateCurvatureInfo(
	const Eigen::Vector2d& p1,
	const Eigen::Vector2d& p2,
	const Eigen::Vector2d& p3) {
	Eigen::Vector2d a = p2 - p1;
	Eigen::Vector2d b = p3 - p2;
	Eigen::Vector2d c = p1 - p3;

	double aNorm2 = a.squaredNorm();
	double bNorm2 = b.squaredNorm();
	double cNorm2 = c.squaredNorm();

	auto normMinMax = std::minmax<double>({aNorm2, bNorm2, cNorm2});

	if (normMinMax.first < 1e-6 || 1e4 * normMinMax.first < normMinMax.second) 
	{
		return {0.0, std::sqrt(cNorm2)};
	}

	double vdot = a.x() * b.y() - a.y() * b.x();
	double sdot = a.x() * b.x() + a.y() * b.y();
	double curvature = 2.0 * vdot / std::sqrt(aNorm2 * bNorm2 * cNorm2);
	double arc_len = std::abs(curvature) > 1e-9 ? 2.0 * std::abs(std::atan2(vdot, sdot) / curvature) : std::sqrt(cNorm2);

	return {curvature, arc_len};
}

FCurvatureInfo calculateSimpleCurvatureInfo(
	const Eigen::Vector2d& p1,
	const Eigen::Vector2d& p2,
	const Eigen::Vector2d& p3) 
{
	return {calculateCurvature(p1, p2, p3), (p3 - p1).norm()};
}

} // namespace SpeedProfile