#pragma once

#include <soda/sim-proto-v1/common.hpp>
#include <array>
#include <tuple>
#include <ostream>
#include <sstream>

namespace soda 
{
namespace sim_proto_v1 
{
	
class Pose2d final
{
public:
  Pose2d() noexcept = default;

  Pose2d(float x, float y, float yaw) noexcept
    : m_x(x)
    , m_y(y)
    , m_yaw(yaw)
  {}

  float x() const noexcept { return m_x; }
  void setX(float x) noexcept { m_x = x; }
  float y() const noexcept { return m_y; }
  void setY(float y) noexcept { m_y = y; }
  float yaw() const noexcept { return m_yaw; }
  void setYaw(float yaw) noexcept { m_yaw = yaw; }

  bool operator == (Pose2d const & other) const noexcept {
    return std::tie(m_x, m_y, m_yaw) == std::tie(other.m_x, other.m_y, other.m_yaw);
  }

  bool operator != (Pose2d const & other) const noexcept { return !(*this == other); }

private:
  float m_x = 0, m_y = 0;
  float m_yaw = 0;
};

inline std::ostream &operator<<(std::ostream & out, Pose2d const& pose) {
  return out << "(" << pose.x() << ", " << pose.y() << "; yaw: " << pose.yaw()
             << ")";
}


class Pose3d final
{
public:
  Pose3d() noexcept = default;

  Pose3d(Pose2d const & other) noexcept
    : m_x(other.x())
    , m_y(other.y())
    , m_z(0)
    , m_roll(0)
    , m_pitch(0)
    , m_yaw(other.yaw())
  {}

  Pose3d(float x, float y, float z, float roll, float pitch, float yaw) noexcept
    : m_x(x)
    , m_y(y)
    , m_z(z)
    , m_roll(roll)
    , m_pitch(pitch)
    , m_yaw(yaw)
  {}

  float x() const noexcept { return m_x; }
  void setX(float x) noexcept { m_x = x; }
  float y() const noexcept { return m_y; }
  void setY(float y) noexcept { m_y = y; }
  float z() const noexcept { return m_z; }
  void setZ(float z) noexcept { m_z = z; }
  float roll() const noexcept { return m_roll; }
  void setRoll(float roll) noexcept { m_roll = roll; }
  float pitch() const noexcept { return m_pitch; }
  void setPitch(float pitch) noexcept { m_pitch = pitch; }
  float yaw() const noexcept { return m_yaw; }
  void setYaw(float yaw) noexcept { m_yaw = yaw; }

  bool operator == (Pose3d const & other) const noexcept {
    return std::tie(m_x, m_y, m_z, m_roll, m_pitch, m_yaw) == std::tie(other.m_x, other.m_y, other.m_z, other.m_roll, other.m_pitch, other.m_yaw);
  }

  bool operator != (Pose3d const & other) const noexcept { return !(*this == other); }

  bool is2d() const noexcept {
    return std::tie(m_z, m_roll, m_pitch) == std::make_tuple<float>(0, 0, 0);
  }

  Pose2d project() const noexcept {
    return Pose2d(m_x, m_y, m_yaw);
  }

private:
  float m_x = 0, m_y = 0, m_z = 0;
  float m_roll = 0, m_pitch = 0, m_yaw = 0;
};

inline std::ostream & operator<<(std::ostream & out, Pose3d const& pose) {
  return out << "(xyz: " << pose.x() << ", " << pose.y() << ", " << pose.z() << "; "
             << "rpy: " << pose.roll() << ", " << pose.pitch() << ", " << pose.yaw()
             << ")";
}

struct Transform4x4 {
  std::array<float, 16> data;

  Transform4x4(std::array<float, 16> m) noexcept;

  static Transform4x4 from_pose(Pose3d const& pose) noexcept;

  static Transform4x4 inverse(Pose3d const& pose) noexcept;
};

#define TRANSFORM_HELPERS(tr) \
  auto & tr00 = (tr).data[0 * 4 + 0]; \
  auto & tr01 = (tr).data[0 * 4 + 1]; \
  auto & tr02 = (tr).data[0 * 4 + 2]; \
  auto & tr03 = (tr).data[0 * 4 + 3]; \
  auto & tr10 = (tr).data[1 * 4 + 0]; \
  auto & tr11 = (tr).data[1 * 4 + 1]; \
  auto & tr12 = (tr).data[1 * 4 + 2]; \
  auto & tr13 = (tr).data[1 * 4 + 3]; \
  auto & tr20 = (tr).data[2 * 4 + 0]; \
  auto & tr21 = (tr).data[2 * 4 + 1]; \
  auto & tr22 = (tr).data[2 * 4 + 2]; \
  auto & tr23 = (tr).data[2 * 4 + 3]

inline void write(std::ostream& out, void const* data, std::size_t size)
{
    out.write(reinterpret_cast< const char* >(data), static_cast< std::streamsize >(size));
}

inline void read(std::istream& in, void* data, std::size_t size)
{
    in.read(reinterpret_cast< char* >(data), static_cast< std::streamsize >(size));
}

void SODASIMPROTOV1_API write(std::ostream & out, Pose3d const& pose);
void SODASIMPROTOV1_API read(std::istream & in, Pose3d & pose);
void SODASIMPROTOV1_API write(std::ostream & out, Pose2d const& pose);
void SODASIMPROTOV1_API read(std::istream & in, Pose2d & pose);

} // namespace sim_proto_v1 
} // namespace soda

