#pragma once

#include <condition_variable>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <concepts>

#include <LockFree/Queue.h>
#include <Utility/IdGenerator.h>

namespace ambilink::events {

class EventConsumer;
template<typename, bool>
struct Event;

struct EventBase;

using IdGenerator = utils::SequentialIdGenerator<EventBase>;
using EventID = IdGenerator::IdType;

template<typename T>
concept IsConcreteEvent = std::is_base_of_v<Event<T, true>, T>
                          || std::is_base_of_v<Event<T, false>, T>;

struct EventBase
{
    /// @brief indicates if the event has been handled by at least one
    /// consumer.
    bool handled = false;

    EventBase() = default;
    virtual ~EventBase() = default;
    [[nodiscard]] virtual EventID getEventTypeID() const = 0;
    virtual std::unique_ptr<EventBase> getCopy() const = 0;
    virtual bool isIntendedForMultipleConsumers() const = 0;
};

/**
 * @brief Derive event classes from this class.
 *
 * @tparam DerivedT the derived class (CRTP)
 * @tparam MultipleConsumers indicates if the event is intended to be
 * handled by multiple consumers. If this is true, propagation will
 * not stop after a consumer handles an event of this type.
 */
template<typename DerivedT, bool MultipleConsumers = false>
struct Event : public EventBase
{
    Event() = default;
    inline static const EventID id = IdGenerator::getNextId();

    EventID getEventTypeID() const final { return id; }

    /// @brief Get a copy of the event.
    std::unique_ptr<EventBase> getCopy() const final {
        return std::make_unique<DerivedT>(static_cast<const DerivedT&>(*this));
    }

    /**
     * @brief used by consumers to check if event should be propagated after
     * it's already been handled once.
     */
    bool isIntendedForMultipleConsumers() const final {
        return MultipleConsumers;
    }
};

} // namespace ambilink::events
