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

enum class ESteerReqMode: std::int8_t
{
	ByRatio,
	ByAngle
};

enum class EDriveEffortReqMode : std::int8_t
{
	ByRatio,
	ByAcc
};

#pragma pack(push, 1)

struct GenericVehicleControlMode1
{
	union
	{
		float by_angle; // [rad]
		float by_ratio; // [-1..1]
	} steer_req;
	
	/** Zero value means change speed as quickly as possible to TargetSpeed */
	union
	{
		float by_acc; // [m/s^2]
		float by_ratio; // [-1..1]
	} drive_effort_req;
	
	/** [cm/s]. Zero TargetSpeed means don't used  */
	float target_speed_req;	
	
	EGearState gear_state_req; 
	
	/** 
	 * Desire gear number for the drive and revers gear;  
	 * Values:
	 *   - 0        - automatic/undefined; 
	 *   - others   - desire gear number;
	 */
	std::int8_t gear_num_req; 

	ESteerReqMode steer_req_mode;
	EDriveEffortReqMode drive_effort_req_mode;
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
	
	// [ns]
	std::int64_t timestemp; 
};

static_assert(sizeof(float) == 4);

#pragma pack (pop)

} // namespace sim
} // namespace proto_v1 
} // namespace soda
