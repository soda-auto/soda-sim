#pragma once

#include <soda/sim/proto-v1/navigation.hpp>
#include <cstdint>

namespace soda
{
namespace sim
{
namespace proto_v1 
{

enum class EGearState : std::uint8_t 
{ 
	Neutral = 0, 
	Drive, 
	Revers, 
	Parking
};

enum class EControlMode : std::uint8_t 
{ 
	/** The vehicle is under a driver (human) control. See manual control sourse in the ESodaVehicleInputSource. */
	Manual = 0,

	/** The vehicle is under AD (autonomous driving) control. */
	AD = 1,

	/** The vehicle is under (autonomous driving) control, but something was wrong. */
	SafeStop = 2,

	/** The vehicle is under a driver (human) control, but it is ready to switch to AD control. */
	ReadyToAD = 3,

};

#pragma pack(push, 1)

struct GenericVehicleControlMode1
{
	float steer_req; // [rad]
	float acc_decel_req; // [m/s^2]
	EGearState gear_state_req; 
	
	/** 
	 * Desire gear number for the drive and revers gear;  
	 * Values:
	 *   - 0        - automatic/undefined; 
	 *   - others   - desire gear number;
	 */
	std::int8_t gear_num_req; 
};

// State estimated by localization.
struct GenericVehicleState
{
	struct WheelState
	{
		float ang_vel; // [rad/s]
		float brake_torq; // [H/m]
		float torq;  // [H/m]
	};
	WheelState wheels_state[4]; // FL, FR, RL, RR wheels
	float steer; // [rad]
	EGearState gear_state; 
	std::int8_t gear_num;
	EControlMode mode;
};

static_assert(sizeof(float) == 4);

#pragma pack (pop)

} // namespace sim
} // namespace proto_v1 
} // namespace soda
