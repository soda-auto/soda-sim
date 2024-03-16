#pragma once

#include <soda/sim-proto-v1/pose.hpp>
#include <vector>

namespace soda 
{
namespace sim_proto_v1 
{

struct Ultrasonic {
  enum class Properties : std::uint8_t {
	none = 0x0,
    transmitter = 0x1
  };
  
  float hfov; 
  float vfov; 
  Properties properties;
  std::vector<float> echoes; // size = 4 * num_of_echoes
};

struct UltrasonicsHub {
  std::int64_t device_timestamp;
  std::vector<Ultrasonic> ultrasonics;

  inline std::chrono::time_point<std::chrono::system_clock> get_timepoint() const {
      return chrono_timestamp(device_timestamp);
  }
};

int SODASIMPROTOV1_API write(std::ostream & out, UltrasonicsHub const& scan);
void SODASIMPROTOV1_API read(std::istream & in, UltrasonicsHub & scan);

} // namespace sim_proto_v1 
} // namespace soda
