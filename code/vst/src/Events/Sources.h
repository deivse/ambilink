#pragma once

#include "Events.h"
#include "Consumers.h"

namespace ambilink::events {

/**
 * @brief Utility base class for classes that send events to
 * consumers.
 */
class EventSource
{
protected:
    EventConsumer& _event_target;

public:
    /**
     * @param target An EventConsumer that will handle the event -
     * either a final consumer, or a propagator.
     */
    EventSource(EventConsumer& target) : _event_target(target) {}
    template<typename EventT>
    void sendEvent(EventT&& event) {
        _event_target.onEvent(event);
    }
};

} // namespace ambilink::events
