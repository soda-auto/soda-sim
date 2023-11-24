// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <Eigen/Core>
#include <utility>

namespace SpeedProfile
{

struct FTrajectoryPoint 
{
	FTrajectoryPoint() = default;
	FTrajectoryPoint(
		const Eigen::Vector2d& point_,
		double curvature_,
		double yaw_)
		: point(point_)
		, curvature(curvature_)
		, yaw(yaw_) {}

	Eigen::Vector2d point = Eigen::Vector2d(0.0, 0.0);
	double curvature = 0;
	double yaw = 0;

	inline operator Eigen::Vector2d& () noexcept
	{
		return point;
	}
	inline operator const Eigen::Vector2d& () const noexcept
	{
		return point;
	}
};


inline bool operator==(const FTrajectoryPoint& lhs, const FTrajectoryPoint& rhs) 
{
	return std::tie(lhs.point, lhs.curvature, lhs.yaw) == std::tie(rhs.point, rhs.curvature, rhs.yaw);
}

inline bool operator!=(const FTrajectoryPoint& lhs, const FTrajectoryPoint& rhs) 
{
	return !(rhs == lhs);
}

} // namespace SpeedProfile
