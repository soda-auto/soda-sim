#pragma once

#include <cstdint>

namespace soda
{
namespace sim
{
namespace proto_v1 
{

#pragma pack(push, 1)

struct RacingSensor
{
	// Valid only if border_is_valid [m]
	float left_border_offset;

	// Valid only if border_is_valid [m]
	float right_border_offset;

	// Valid only if border_is_valid [rad]
	float center_line_yaw;

	// Valid only if lap_counter_is_valid
	std::int32_t lap_caunter;

	// Valid only if lap_counter_is_valid [m]
	float covered_distance_current_lap;

	// [m]
	float covered_distance_full;

	bool border_is_valid;
	bool lap_counter_is_valid;

	// [ns]
	std::int64_t timestemp; 

	// [ns]
	std::int64_t start_timestemp; 

	// Valid only if lap_counter_is_valid [ns]
	std::int64_t lap_timestemp;
};

static_assert(sizeof(float) == 4);

#pragma pack (pop)

} // namespace sim
} // namespace proto_v1 
} // namespace soda
