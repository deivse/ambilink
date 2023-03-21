#pragma once
#include "../Events/Events.h"

#include <juce_core/juce_core.h>
#include <glm/vec3.hpp>

namespace ambilink::ipc::commands {

struct SubscribeToObject : public events::Event<SubscribeToObject>
{
    juce::String object_name;
    SubscribeToObject(juce::String object_name_)
      : object_name{std::move(object_name_)} {}
};

struct Connect : events::Event<Connect>
{};

struct Unsubscribe : events::Event<Unsubscribe>
{};

struct UpdateObjectList : public events::Event<UpdateObjectList>
{};

struct EnableRenderingMode : public events::Event<EnableRenderingMode>
{};

struct DisableRenderingMode : public events::Event<DisableRenderingMode>
{};

} // namespace ambilink::ipc::commands
