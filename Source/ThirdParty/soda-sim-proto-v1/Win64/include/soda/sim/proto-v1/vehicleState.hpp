#pragma once

#include <soda/sim/proto-v1/navigation.hpp>
#include <cstdint>

namespace soda
{
namespace sim
{
namespace proto_v1 
{

enum class EGear : std::uint8_t 
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

#pragma pack(push, 8)

struct GenericVehicleControl
{
	float steer_req; // [rad]
	float acc_decel_req; // [m/s^2]
	EGear gear_req; // 0 - Neutral, 1 - Drive, 2 - Revers, 3 - Parking
};

// State estimated by localization.
struct GenericVehicleState
{
	struct WheelState
	{
		double ang_vel; // [rad/s]
		double brake_torq; // [H/m]
		double torq;  // [H/m]
	};

	double steer; // [rad]
	EGear gear; 
	EControlMode mode;
	NavigationState navigation_state;
	WheelState wheels_state[4]; // FL, FR, RL, RR wheels
};

#pragma pack (pop)

} // namespace sim
} // namespace proto_v1 
} // namespace soda
