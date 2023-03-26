#include "TimelineDirectionData.h"

#include <algorithm>
#include <cmath>

#include <Math/Math.h>

namespace ambilink {
TimelineDirectionData::TimelineDirectionData(std::span<glm::vec3> location_data,
                                             float fps)
  : _fps(fps) {
    std::transform(location_data.begin(), location_data.end(),
                   std::back_inserter(_directions),
                   [](const glm::vec3& location) {
                       return math::directionFromCamSpaceLocation(location);
                   });
}

DirectionWithDistance
  TimelineDirectionData::getDirectionAndDistanceAtTime(float time_secs) {
    const auto frame = std::floor(time_secs * _fps);
    return _directions[std::min(std::max<size_t>(frame, 0),
                                _directions.size() - 1)];
}
} // namespace ambilink
