#pragma once

#include <chrono>

#define SODASIMPROTOV1_API

namespace soda 
{
namespace sim_proto_v1 
{
	
template <
	typename Clock = std::chrono::system_clock,
	typename Units = std::chrono::nanoseconds
>
inline auto chrono_timestamp(typename Clock::rep u) noexcept
{
	using namespace std::chrono;
	using TPoint = time_point<Clock>;
	using TDuration = typename Clock::duration;
	return TPoint(duration_cast<TDuration>(Units(u)));
}
#pragma pack(push, 8)
struct Vector 
{
	double x;
	double y;
	double z;
};

struct Rotator
{
	double roll;  // Rotation along X-axis, [-pi, pi]
	double pitch;  // Rotation along Y-axis, [-pi/2, pi/2]
	double yaw;  // Rotation along Z-axis, [-pi, pi]
};
#pragma pack (pop)

} // namespace sim_proto_v1 
} // namespace soda
