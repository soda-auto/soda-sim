#pragma once

#include <soda/sim-proto-v1/pose.hpp>
#include <vector>

namespace soda 
{
namespace sim_proto_v1 
{
	
#pragma pack(push, 1)

struct RadarCluster {
  enum class property_t : std::uint8_t {
    moving = 0x0, // moving
    stationary = 0x1, // stationary
    oncoming = 0x2, // oncoming
    stationary_candidate = 0x3, // stationary candidate
    unknown = 0x4, // unknown
    crossing_stationary = 0x5, // crossing stationary
    crossing_moving = 0x6, // crossing moving
    stopped = 0x7 // stopped
  };

  std::uint8_t id; // Id of the cluster
  struct {
    float x, y, z;
  } coords; // Cartesian coordinates of the cluster, in meters
  struct {
    float lat, lon;
  } velocity; // Velocity described with lateral and longitudinal components, in m/s
  float RCS;
  property_t property;
  std::uint8_t reserved[6];
};

struct RadarObject {
  enum class class_t : std::uint32_t {
    point = 0x00, // point
    car = 0x01, // car
    truck = 0x02, // truck
    pedestrian = 0x03, // pedestrian
    motorcycle = 0x04, // motorcycle
    bicycle = 0x05, // bicycle
    wide = 0x06, // wide object
    reserved = 0x07 // reserved field
  };

  std::uint8_t id; // Id of the cluster
  struct {
    float x, y, z;
  } coords; // Cartesian coordinates of the object, in meters
  struct {
    float lat, lon;
  } velocity; // Velocity described with lateral and longitudinal components, in m/s
  struct {
    std::uint32_t length, width;
  } dimension; // Object's dimensions (length x width), in meters

  class_t classification; // link Object::class_t Classification\endlink of the object
};
#pragma pack(pop)

struct RadarScan {
  std::uint16_t device_id = 0;
  std::int64_t device_timestamp = 0;
  std::vector<RadarCluster> items;

  inline std::chrono::system_clock::time_point get_timepoint() const {
      return chrono_timestamp(device_timestamp);
  }
};

int SODASIMPROTOV1_API write(std::ostream & out, RadarScan const& scan);
void SODASIMPROTOV1_API read(std::istream & in, RadarScan & scan);

} // namespace sim_proto_v1 
} // namespace soda
