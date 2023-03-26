#pragma once
#include <vector>
#include <span>
#include <DataTypes.h>

#include <glm/vec3.hpp>

namespace ambilink {

class TimelineDirectionData
{
    std::vector<DirectionWithDistance> _directions{};
    float _fps = 0;
public:
    TimelineDirectionData() = default;
    TimelineDirectionData(std::span<glm::vec3> location_data, float fps);
    DirectionWithDistance getDirectionAndDistanceAtTime(float time_secs);
    
};

} // namespace ambilink
