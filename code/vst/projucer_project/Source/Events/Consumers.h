#pragma once
#include "Events.h"

namespace ambilink::events {

template<typename T>
concept IsEventConsumer
  = std::is_base_of_v<EventConsumer, std::remove_reference_t<T>>;

template<typename T, typename EventT>
concept IsEventHandler
  = requires(T t, EventT& evt) {
        std::is_invocable_v<T, std::add_lvalue_reference_t<EventT>>;
        { t(evt) } -> std::same_as<bool>;
    };

/**
 * @brief Defines the event consumer interface
 */
class EventConsumer
{
public:
    virtual ~EventConsumer() = default;
    virtual void onEvent(EventBase& event) = 0;
};

/**
 * @brief An EventConsumer implementation that can be used to delay
 * event processing or offload it to a separate thread.
 */
class AsyncEventConsumer : public EventConsumer
{
    lock_free::Queue<std::unique_ptr<EventBase>, 500> _event_queue{};

    /**
     * @brief Override in derived class to be informed when an event
     * becomes available.
     */
    virtual void onEventAvailable() {}
    /**
     * @brief Default implementaion always returns true. Override to provide custom behaviour.
     */
    virtual bool wantsEvent(events::EventID);

public:
    struct NoQueuedEventError : public std::runtime_error
    {
        NoQueuedEventError() : std::runtime_error("No events to process"){};
    };

    virtual ~AsyncEventConsumer() = default;    
    AsyncEventConsumer() = default;

    /**
     * @brief Adds an event to the internal queue. Called by the event
     * source/propagator.
     *
     * @param event the event.
     */
    void onEvent(EventBase& event) final;

    /**
     * @brief Get the latest event from the queue.
     * @throws NoQueuedEventError if the event queue is empty.
     *
     * @return std::unique_ptr<EventBase> the first unprocessed event
     * in queue.
     */
    std::unique_ptr<EventBase> getEvent();

    /**
     * @brief Check if 1/more events are queued for processing.
     */
    bool eventQueued() { return !_event_queue.empty(); }
};

/**
 * @brief An EventConsumer imlementation that propagates events to a
 * list of other EventConsumers.
 */
class EventPropagator : public EventConsumer
{
    std::vector<std::reference_wrapper<EventConsumer>> _consumers;

public:
    /**
     * @brief Construct a new EventPropagator.
     *
     * @tparam Consumers
     * @param consumers the consumers that this propagator should
     * serve. The consumers are stored as references. The lifetime of
     * the EventPropagator must not surpass the lifetime of the
     * consumer objects.
     */
    template<IsEventConsumer... Consumers>
    EventPropagator(Consumers&... consumers) {
        (_consumers.emplace_back(consumers), ...);
    }
    EventPropagator() = delete;

    void onEvent(EventBase& event) override;
};

/**
 * @brief An EventConsumer implementation that handles events of some
 * types but propagates other events to other consumers.
 *
 * @tparam EventTypes
 */
template<IsConcreteEvent... EventTypes>
class InterceptingPropagator : public EventPropagator
{
    /// @brief Called by InterceptingPropagator::onEvent for events
    /// Whose type matches one of the IDs.
    virtual void onInterceptedEvent(EventBase& event) = 0;

    constexpr static bool shouldIntercept(EventID id) {
        return (... || (id == EventTypes::id));
    }

public:
    using EventPropagator::EventPropagator;

    void onEvent(EventBase& event) final {
        if (shouldIntercept(event.getEventTypeID())) {
            onInterceptedEvent(event);
        } else {
            EventPropagator::onEvent(event);
        }
    }
};

/**
 * @brief Class to dispatch events inside an event handler
 * implementation.
 */
class Dispatcher
{
public:
    /**
     * @param event The dispatcher will hold a non-owning reference to
     * this event.
     */
    explicit Dispatcher(EventBase& event) : _event(event) {}

    /**
     * @brief Call this function for each event type you intend to
     * handle.
     *
     * @tparam EventT The event type to handle
     * @tparam HandlerCallable Type of the callable to handle the
     * event, must accept a single parameter of type `EventT&`.
     * @param func A callable to handle the event.
     * @return bool true if the event's type matched `EventT` and it
     * was handled.
     */
    template<typename EventT, IsEventHandler<EventT> HandlerCallable>
    bool dispatch(const HandlerCallable& func) {
        if (_event.getEventTypeID() == EventT::id) {
            _event.handled |= func(static_cast<EventT&>(_event));
            return true;
        }
        return false;
    }

private:
    EventBase& _event;
};

} // namespace ambilink::events
