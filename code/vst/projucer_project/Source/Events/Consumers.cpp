#include "Consumers.h"

namespace ambilink::events {

void AsyncEventConsumer::onEvent(EventBase& event) {
    if (wantsEvent(event.getEventTypeID())) {
        event.handled = _event_queue.pushOrFail(event.getCopy());
        onEventAvailable();
    }
}
std::unique_ptr<EventBase> AsyncEventConsumer::getEvent() {
    if (_event_queue.empty()) throw NoQueuedEventError{};
    return _event_queue.pop();
}

bool AsyncEventConsumer::wantsEvent(events::EventID) {
    return true;
}

void EventPropagator::onEvent(EventBase& event) {
    for (auto& consumer : _consumers) {
        consumer.get().onEvent(event);
        if (event.handled && !event.isIntendedForMultipleConsumers())
            break;
    }
}


} // namespace ambilink::events
