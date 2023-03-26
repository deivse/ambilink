#include "./Math.h"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <glm/trigonometric.hpp>
#include <glm/gtx/compatibility.hpp>

namespace ambilink::math {
DirectionWithDistance
  directionFromCamSpaceLocation(const glm::vec3& location_camera_space) {
    const glm::vec3 xz_projected
      = {location_camera_space.x, 0, location_camera_space.z};

    // negate because flipped otherwise
    auto azimuth = -atan2(location_camera_space.x, location_camera_space.z);
    auto elevation = glm::acos(glm::dot(glm::normalize(location_camera_space),
                                        glm::normalize(xz_projected)));

    if (location_camera_space.y < 0) elevation = -elevation;

    return {Direction{glm::degrees(static_cast<float>(azimuth)),
                      glm::degrees(static_cast<float>(elevation))},
            glm::length(location_camera_space)};
}
} // namespace ambilink::math
