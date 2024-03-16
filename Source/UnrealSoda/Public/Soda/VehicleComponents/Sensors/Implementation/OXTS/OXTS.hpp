#pragma once

#include <stdint.h>


namespace soda
{

/**
 * GNSS/IMU message (sends via UDP). 
 * This is original NCOM protocol from OXTS.
 * (For more details see: https://support.oxts.com/hc/en-us/articles/115002163985-Decoding-OxTS-navigation-outputs)
 * Note: The OxtsChannel_t structure is not packed tightly under the Visual Studio compiler. 
         Therefore, it is necessary to sequentially pack data manually for the Visual Studio compiler.
 */
#pragma pack(push, 1)
typedef union {
  uint8_t bytes[8];
  struct {
    uint32_t gps_minutes;
    uint8_t num_sats;
    uint8_t position_mode;
    uint8_t velocity_mode;
    uint8_t orientation_mode;
  } chan0;
  struct {
    uint16_t acc_position_north;  // 1e-3 m
    uint16_t acc_position_east;
    uint16_t acc_position_down;
    uint8_t age;
  } chan3;
  struct {
    uint16_t acc_velocity_north;  // 1e-3 m/s
    uint16_t acc_velocity_east;
    uint16_t acc_velocity_down;
    uint8_t age;
  } chan4;
  struct {
    uint16_t acc_heading;  // 1e-5 rad
    uint16_t acc_pitch;
    uint16_t acc_roll;
    uint8_t age;
  } chan5;
  struct {
    int16_t base_station_age;
    int8_t base_station_id[4];
  } chan20;
  struct {
    uint16_t delay_ms;
  } chan23;
  struct {
    uint8_t heading_quality;  // 0:None 1:Poor 2:RTK Float 3:RTK Integer
  } chan27;
  struct {
    int16_t heading_misalignment_angle;      // 1e-4 rad
    uint16_t heading_misalignment_accuracy;  // 1e-4 rad
    uint16_t : 16;
    uint8_t valid;
  } chan37;
  struct {
    int16_t undulation;  // 5e-3 m
    uint8_t HDOP;        // 1e-1
    uint8_t PDOP;        // 1e-1
  } chan48;
} OxtsChannel_t;

typedef struct {
  uint8_t sync;
  uint16_t time; // ms of current GPS minute
  int32_t accel_x : 24;
  int32_t accel_y : 24;
  int32_t accel_z : 24;
  int32_t gyro_x : 24;
  int32_t gyro_y : 24;
  int32_t gyro_z : 24;
  uint8_t nav_status;
  uint8_t chksum1;
  union {
    double latitude; // deg
    uint8_t c[8];
  } _lat;
  union {
    double longitude; // deg
    uint8_t c[8];
  } _lon;
  union {
    float altitude; // m
    uint8_t c[4];
  } _alt;
  int32_t vel_north : 24;
  int32_t vel_east : 24;
  int32_t vel_down : 24;
  int32_t heading : 24;
  int32_t pitch : 24;
  int32_t roll : 24;
  uint8_t chksum2;
  uint8_t channel;
  OxtsChannel_t chan;
  uint8_t chksum3;
} OxtsPacket;
#pragma pack(pop)




} // namespace soda
