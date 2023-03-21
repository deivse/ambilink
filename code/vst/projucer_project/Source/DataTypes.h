#pragma once

#include <cstdint>
namespace ambilink {
using AmbilinkID = uint16_t;

/// @brief Represents direction to object as a point in spherical coordinates.
struct Direction
{
    float azimuth_deg{0};
    float elevation_deg{0};
};

using Distance = float;

/// @brief Contains all data the encoder needs.
struct DirectionWithDistance
{
    Direction direction{};
    Distance distance{0};
};

} // namespace ambilink
