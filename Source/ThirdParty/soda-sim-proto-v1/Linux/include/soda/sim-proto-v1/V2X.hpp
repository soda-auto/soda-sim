#pragma once

#include <cstdint>

namespace soda
{
namespace sim_proto_v1 
{
	
#pragma pack(push, 1)
struct V2VSimulatedObject
{
  std::uint16_t id;

  std::int64_t timestamp; // msec

  double bounding_box0_X; // Meter
  double bounding_box0_Y; // Meter
  double bounding_box1_X; // Meter
  double bounding_box1_Y; // Meter
  double bounding_box2_X; // Meter
  double bounding_box2_Y; // Meter
  double bounding_box3_X; // Meter
  double bounding_box3_Y; // Meter

  double bounding_3d_box0_X; // Meter
  double bounding_3d_box0_Y; // Meter
  double bounding_3d_box0_Z; // Meter
  double bounding_3d_box1_X; // Meter
  double bounding_3d_box1_Y; // Meter
  double bounding_3d_box1_Z; // Meter
  double bounding_3d_box2_X; // Meter
  double bounding_3d_box2_Y; // Meter
  double bounding_3d_box2_Z; // Meter
  double bounding_3d_box3_X; // Meter
  double bounding_3d_box3_Y; // Meter
  double bounding_3d_box3_Z; // Meter
  double bounding_3d_box4_X; // Meter
  double bounding_3d_box4_Y; // Meter
  double bounding_3d_box4_Z; // Meter
  double bounding_3d_box5_X; // Meter
  double bounding_3d_box5_Y; // Meter
  double bounding_3d_box5_Z; // Meter
  double bounding_3d_box6_X; // Meter
  double bounding_3d_box6_Y; // Meter
  double bounding_3d_box6_Z; // Meter
  double bounding_3d_box7_X; // Meter
  double bounding_3d_box7_Y; // Meter
  double bounding_3d_box7_Z; // Meter

  double longitude; // Rad
  double latitude;  // Rad
  double altitude;  // Meter
  double roll;      // Rotation along X-axis, [-pi, pi]
  double pitch;     // Rotation along Y-axis, [-pi/2, pi/2]
  double yaw;       // Rotation along Z-axis, [-pi, pi]

  double ang_vel_roll;  // Rad/s
  double ang_vel_pitch; // Rad/s
  double ang_vel_yaw;   // Rad/s

  double vel_forward; // m/s
  double vel_lateral; // m/s
  double vel_up;      // m/s

};
#pragma pack(pop)

enum class V2XRenderObjectType : uint32_t
{
    VEHICLE =     0,
    OBSTACLE =    1,
    COLLECTABLE = 2
};


#pragma pack(push, 1)
struct V2XRenderObject
{
    uint32_t reserved;            /*!< Future feature */
    uint32_t object_type;         /*!< Object type (0 - vehicle, 1 - obstacle, 2 - collectable) */
    uint32_t object_id;           /*!< Object identifier unique for track*/
    double timestamp;             /*!< Timestamp (seconds since epoch) */
    double east;                  /*!< Relative distance projection to east-west axis, meters */
    double north;                 /*!< Relative distance projection to north-south axis, meters */
    double up;                    /*!< Relative distance projection to up-down axis, meters */
    double heading;               /*!< Heading relative to true North, radians */
    double velocity;              /*!< Absolute value of velocity vector projection on XY plane, m/s */
    uint8_t status;               /*!< Object status */
    double position_accuracy;     /*!< Position accuracy, meters */
    double heading_accuracy;      /*!< Heading accuracy, radians */
    double velocity_accuracy;     /*!< Velocity accuracy, m/s */

    // The following fields are filled for virtual objects only
    double vertical_velocity;     /*!< Velocity vector projection on Z axis, m/s */
    double length;                /*!< Object length, m */
    double width;                 /*!< Object width, m */
    double height;                /*!< Object height, m */
    double yaw_angle;             /*!< Object yaw angle, radians */
    double pitch_angle;           /*!< Object pitch angle, radians */
    double roll_angle;            /*!< Object roll angle, radians */
    double orientation_accuracy;  /*!< Orientation accuracy, radians */
};
#pragma pack(pop)

} // namespace sim_proto_v1 
} // namespace soda
