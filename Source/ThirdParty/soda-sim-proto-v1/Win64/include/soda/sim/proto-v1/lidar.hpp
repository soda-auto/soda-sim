#pragma once

#include <soda/sim/proto-v1/pose.hpp>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <stddef.h>

namespace soda
{
namespace sim 
{
namespace proto_v1 
{

#pragma pack(push, 1)

struct LidarScanPoint
{
    static std::uint8_t constexpr MaximumDiffuseReflectivity = 100;
    
    /**
    Properties associated with scan point.
    \warning All the properties represent results of vendor-specific classification.
    */
    enum Properties: std::uint8_t 
    {
        None = 0x0, // No properties associated with this point.
        Valid = 0x1, // This point is valid in some vendor-specific sense. For example, \em invalid points may be the ones that were classfied as dirt on LiDAR's glass surface, any kind of precipitation, or even road surface.
        Transparent = 0x2, // This point is considered to be transparent.
        Reflector = 0x4 // This point is reflective enough to be counted as a reflector.
    };

    struct 
    {
        float x, y, z;
    } coords; // Cartesian coordinates of this point, in meters.
    /**
    Reflectivity of this point.

    Values:
      - 0 to 100 - Diffuse reflection. 0 is for black diffuse surface, and
        100 - is for white diffuse surface.
      - 101 to 255 - Specular reflection. 101 is for the least specular surfaces,
        and 255 is for the most specular ones.
    */
    std::uint8_t reflectivity = MaximumDiffuseReflectivity;
    std::uint8_t layer = 0; // Index of horizontal layer this point belongs to.
    std::uint8_t echo = 0; // Echo index of this point.
    std::uint16_t properties = Properties::Valid; //  ScanPoint::Properties Properties of this point.

    bool operator == (LidarScanPoint const& other) const noexcept 
    {
        return std::tie(layer, echo, properties) ==
            std::tie(other.layer, other.echo, other.properties) &&
            std::abs(coords.x - other.coords.x) <= 1e-5f &&
            std::abs(coords.y - other.coords.y) <= 1e-5f &&
            std::abs(coords.z - other.coords.z) <= 1e-5f;
    }

    bool operator != (LidarScanPoint const& other) const noexcept { return !(*this == other); }
};

#pragma pack(pop)

static_assert(sizeof(LidarScanPoint) == 17, "ScanPoint has unexpected size");

struct LidarScan
{
    std::uint16_t device_id = 0;
    std::int64_t device_timestamp = 0;
    std::uint32_t scan_id;
    std::uint16_t block_count = 0;
    std::uint16_t block_id = 0;
    std::vector<LidarScanPoint> points;

    inline std::chrono::system_clock::time_point get_timepoint() const 
    {
        return chrono_timestamp(device_timestamp);
    }
};
static_assert((sizeof(LidarScan) - offsetof(LidarScan, points)) == sizeof(LidarScan::points), "Scan::points is not the last field");

int SODASIMPROTOV1_API write(std::ostream & out, LidarScan const& scan);
void SODASIMPROTOV1_API read(std::istream & in, LidarScan & scan);

} // namespace sim
} // namespace proto_v1 
} // namespace soda
