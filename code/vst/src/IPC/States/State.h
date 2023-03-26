#pragma once

#include "Common.h"
#include <Events/Events.h>
#include <ValueIDs.h>
#include <Utility/IdGenerator.h>
#include <IPC/Constants.h>
#include <IPC/ByteIO.h>

#include <nngpp/socket_view.h>
#include <DataTypes.h>

namespace ambilink::ipc {
class IPCClient;
}

/**
 * @brief IPCClient states.
 * 
 */
namespace ambilink::ipc::state {
class StateBase;

using IdGenerator = utils::SequentialIdGenerator<StateBase, int16_t, -1>;
using StateID = IdGenerator::IdType;

/**
 * @brief Base class defining the interface for all IPC states and implementing
 * some common functionality.
 */
class StateBase : private juce::AsyncUpdater
{
    /**
     * @brief Updates plugin state based on items popped from the prop update
     * queue. (Since the ValueTree must be updated from the message thread.)
     */
    void handleAsyncUpdate() final;

    /// @brief helper structure to schedule ValueTree updates from IPC threads.
    struct PropUpdate
    {
        juce::Identifier id;
        juce::var new_value;
    };

    /// @brief queue of scheduled ValueTree property updates
    lock_free::Queue<PropUpdate, 100> _prop_update_queue;

    /**
     * @brief Interval at which ping requests are sent to the Blender plugin.
     * (used to ensure connection hasn't been lost)
     */
    constexpr static std::chrono::seconds ping_interval{5};

    /// @brief time at which last ping request has been sent.
    std::chrono::steady_clock::time_point last_ping
      = std::chrono::steady_clock::now();

    juce::ValueTree _other_plugin_state;

protected:
    /// @brief view over the reqrep sock, concrete states should use this to
    /// send requests.
    nng::socket_view _reqrep_sock;
    /// @brief use to update current direction in real-time mode.
    std::atomic<Direction>& _curr_direction;
    /// @brief use to update current distance in real-time mode.
    std::atomic<Distance>& _curr_distance;
    /// @brief used to control
    const SubThreadController& _sub_thread_ctrl;

    /// @brief allows state implementations to read but not set properties.
    const juce::ValueTree& getOtherPluginState() { return _other_plugin_state; }

public:
    /// @brief get the state ID
    virtual StateID getId() const = 0;

    /**
     * @brief Constructs a new StateBase
     *
     * @param reqrep_sock nng socket for states to send requests
     * @param current_direction reference to the atomic Direction held by
     * IPCClient, used for updating in real-time mode.
     * @param current_distance reference to the atomic Distance held by
     * IPCClient, used for updating in real-time mode.
     * @param other_plugin_state non-audio parameters of the plugin
     * @param sub_thread_ctrl for controlling the Subscriber thread managed by
     * IPCClient.
     */
    StateBase(nng::socket_view reqrep_sock,
              std::atomic<Direction>& current_direction,
              std::atomic<Distance>& current_distance,
              juce::ValueTree& other_plugin_state,
              const SubThreadController& sub_thread_ctrl);

    /**
     * @brief Construct a new StateBase from an existing state. Used when
     * transitioning to a new state.
     *
     * @param prev_state the state being transitioned from
     */
    StateBase(const StateBase& prev_state);

    virtual ~StateBase();

    /**
     * @brief Called from reqRepThread when a command (event) is queued.
     *
     * @param command the command to process
     * @param should_stop callable that returns true if the requestor thread is
     * shutting down, and the processCommand implementations should return ASAP.
     * Must be checked during long operations.
     *
     * @return std::unique_ptr<StateBase> nullptr if no transition, next state
     * if transition should occur.
     */
    virtual std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop)
      = 0;

    /**
     * @brief Periodically called from reqRepThread when no events are queued.
     *
     * @return std::unique_ptr<StateBase> nullptr if no transition, next state
     * if transition.
     */
    virtual std::unique_ptr<StateBase>
      reqRepThreadIdleUpdate(std::function<bool()> /*should_stop*/) {
        return nullptr;
    }

    /**
     * @brief Called from the subscriber thread when a message is received for
     * the curr object.
     *
     * @param msg type of pubsub message.
     * @param reader data reader with the AmbilinkID and message type already
     * read. (read pointer at start of message data)
     */
    virtual void onPubSubCommand(constants::PubSubMsgType msg,
                                 DataReader& reader)
      = 0;

    /**
     * @brief Returns true if the event type is supported by the current state.
     * Implemented in the State class.
     */
    virtual bool wantsCommand(events::EventID StateID) = 0;

    /**
     * @brief Uses juce::AsyncUpdater to update the specified property from the
     * message thread. Can be called from any thread.
     */
    void queuePropUpdate(juce::Identifier id, juce::var&& new_val);

    /**
     * @brief Default implementation periodically sends a ping request to ensure
     * connection is still active. The method can be called as often as
     * possible, time since last ping request is tracked by the StateBase
     * object. (Called by IPCClient.)
     */
    virtual void sendPingRequest();

    /**
     * @brief Called on IPC Client shutdown, override to provide
     * deinitialisation logic (e.g. object unsub).
     */
    virtual void onShutdown(){};

    virtual std::string_view getStateName() const = 0;
};

/// @brief macro to help state implementations define getStateName.
#define implement_GetStateName(Derived) \
    std::string_view getStateName() const final { return #Derived; }

/**
 * @brief CRTP-based class that assigns an ID to each state and implements some
 * methods.
 *
 * @tparam DerivedT The final state implementation.
 */
template<typename DerivedT>
class State : public StateBase
{
    /// @brief events supported by this state.
    std::unordered_set<events::EventID> _wanted_events;

public:
    inline static const StateID id = IdGenerator::getNextId();

    /**
     * @brief Passes params to StateBase constructor, initialises set of
     * supported commands.
     *
     * @param reqrep_sock nng socket for states to send requests
     * @param current_direction reference to the atomic Direction held by
     * IPCClient, used for updating in real-time mode.
     * @param current_distance reference to the atomic Distance held by
     * IPCClient, used for updating in real-time mode.
     * @param other_plugin_state non-audio parameters of the plugin
     * @param sub_thread_ctrl for controlling the Subscriber thread managed by
     * IPCClient.
     * @param command_types type list of commands that this state supports
     */
    template<events::IsConcreteEvent... ReqRepCommandTypes>
    State(nng::socket_view reqrep_sock,
          std::atomic<Direction>& current_direction,
          std::atomic<Distance>& current_distance,
          juce::ValueTree& other_plugin_state,
          const SubThreadController& sub_thread_ctrl,
          utils::TypeList<ReqRepCommandTypes...> /*command_types*/)
      : StateBase(reqrep_sock, current_direction, current_distance,
                  other_plugin_state, std::move(sub_thread_ctrl)) {
        (_wanted_events.insert(ReqRepCommandTypes::id), ...);
    }

    /**
     * @brief Used when transitioning to a new state.
     *
     * @param prev_state the state being transitioned from
     * @param command_types type list of commands that this state supports
     */
    template<events::IsConcreteEvent... ReqRepCommandTypes>
    State(const StateBase& prev_state,
          utils::TypeList<ReqRepCommandTypes...> /*command_types*/)
      : StateBase(prev_state) {
        (_wanted_events.insert(ReqRepCommandTypes::id), ...);
    }

    /// @brief Returns true if the event type is supported by the current state.
    bool wantsCommand(events::EventID event_id) final {
        return _wanted_events.contains(event_id);
    }

    /// @brief returns the state's ID.
    StateID getId() const final { return id; }
};

/**
 * @brief Helper class to provides direct access to a state object to classed
 * outside the IPC implementation while keeping a mutex locked.
 *
 * @tparam StateT The state type.
 */
template<typename StateT>
class ScopedStateAccess
{
    using StateType = StateT;
    inline static const StateID id = StateT::id;
    std::mutex& _access_mu;

    StateT* _state;

public:
    /**
     * @brief Construct a new ScopedStateAccess object and lock the mutex.
     * @throws std::runtime_error if `state` is not of type StateT.
     *
     * @param access_mu the mutex that will be locked upon construction and
     * unlocked upon destruction.
     * @param state the state
     */
    ScopedStateAccess(std::mutex& access_mu, StateBase& state)
      : _access_mu(access_mu), _state(dynamic_cast<StateT*>(&state)) {
        if (!_state)
            throw std::runtime_error(
              "State passed to ScopedStateAccess<StateT> is not of type "
              "StateT.");
        _access_mu.lock();
    }
    /// @brief access the state
    StateT& get() { return *_state; }

    /// @brief destoys the object, unlocking the mutex.
    ~ScopedStateAccess() { _access_mu.unlock(); }

    StateT* operator*() { return _state; }
    StateT* operator->() { return _state; }
    operator StateT&() { return *_state; }
};

///// Predeclarations of states.
class Disconnected;
class Connected;
class Subscribed;
class OfflineRendering;
class ObjectDeleted;
class ErrorState;

} // namespace ambilink::ipc::state
