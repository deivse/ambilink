#pragma once

#include <array>
#include <utility>

#include "glm/vec3.hpp"
#include "../DataTypes.h"

/// @brief Math utility functions.
namespace ambilink::math {

/**
 * @brief Calculates the direction and distance to an object from the camera
 * from it's camera space coordinates.
 */
DirectionWithDistance
  directionFromCamSpaceLocation(const glm::vec3& location_camera_space);

} // namespace ambilink::math
