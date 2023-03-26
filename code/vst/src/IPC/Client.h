#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <stdexcept>

#include <nngpp/socket.h>
#include <fmt/format.h>
#include <glm/vec3.hpp>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <DataTypes.h>
#include <Events/Consumers.h>

#include "ByteIO.h"
#include "Constants.h"
#include "Exceptions.h"

#include "States/State.h"

/**
 * @brief Everything IPC-communication related.
 * 
 */
namespace ambilink::ipc {

static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<Direction>::is_always_lock_free);
static_assert(std::atomic<Distance>::is_always_lock_free);

using StateBase = state::StateBase;
using StateID = state::StateID;

template<typename StateT>
using ScopedStateAccess = state::ScopedStateAccess<StateT>;

/**
 * @brief Handles all IPC communication with the Blender plugin.
 */
class IPCClient : public events::AsyncEventConsumer, public juce::AsyncUpdater
{
    constexpr static std::chrono::milliseconds reqrep_recv_timeout{10000};
    constexpr static std::chrono::milliseconds reqrep_send_timeout{500};
    constexpr static std::chrono::milliseconds pubsub_recv_timeout{0};

    /// @brief nng errors that result from connection loss
    inline static const std::set<nng::error> reconnectable_ipc_errors{
      nng::error::connrefused, nng::error::timedout, nng::error::connshut};

    juce::ValueTree _other_plugin_state;

    nng::socket _reqrep_sock;
    nng::socket _pubsub_sock;

    std::mutex _state_change_mu;
    std::atomic<size_t> _current_state_id = std::numeric_limits<size_t>::max();
    std::array<std::unique_ptr<state::StateBase>, 6> _states{};

    std::atomic<bool> _sub_thread_should_stop{false};
    std::thread _sub_thread{};
    std::optional<AmbilinkID> _obj_id{};

    std::atomic<bool> _req_rep_thread_should_stop{false};
    std::mutex _req_rep_thread_cond_var_mu{};
    std::condition_variable _req_rep_thread_cond_var{};
    std::thread _req_rep_thread{};

    std::atomic<Direction> _current_direction{};
    std::atomic<Distance> _current_distance{};
    state::SubThreadController _sub_thread_ctrl;

    /// @brief implementation of AsyncEventConsumer method informing reqrep
    /// thread of new event
    void onEventAvailable() final { _req_rep_thread_cond_var.notify_one(); }

    /// @brief thread func for thread handling Req/Rep communication
    void requestorThreadFunc();
    /// @brief thread func for thread handling Pub/Sub communication
    void subscriberThreadFunc();

    /// @brief get reference to current state without locking state change mutex
    StateBase& getCurrentStateUnlocked();
    /// @brief sets the current state to next_state, locking state change mutex
    void setNextState(std::unique_ptr<state::StateBase>&& next_state);
    /**
     * @brief Call from catch block (!) to transition to ErrorState or
     * Disconnected, depending on exception type.
     */
    void transitionToErrorOrDisconnectedState();

    /// @brief Sets the `ipc_client_state` value tree prop to
    /// `_current_state_id`.
    void updateIPCStateValueTreeProp();
    void handleAsyncUpdate() final { updateIPCStateValueTreeProp(); }

    state::SubThreadController makeSubThreadController();
    std::unique_ptr<state::Disconnected> makeDisconnectedState();

public:
    IPCClient(juce::ValueTree& other_state);
    ~IPCClient();
    IPCClient(IPCClient&&) = delete;
    IPCClient(const IPCClient&) = delete;
    IPCClient& operator=(IPCClient&&) = delete;
    IPCClient& operator=(const IPCClient&) = delete;

    StateID getCurrentStateID() { return _current_state_id; }

    /**
     * @brief Returns `true` if the current states matches one of the states
     * specified as template parameters (logical OR)
     */
    template<typename... StateT>
    bool isInState() {
        return ((getCurrentStateID() == StateT::id) || ...);
    }

    /**
     * @brief If subscribed, returns the last direction received from the
     * Blender plugin, if not, returns Direction{0,0}. Suitable for real-time
     * thread usage.
     */
    Direction getCurrentDirection_rt() { return _current_direction; }
    /**
     * @brief If subscribed, returns the last distance received from the
     * Blender plugin, if not, returns 0. Suitable for real-time
     * thread usage.
     */
    Distance getCurrentDistance_rt() { return _current_distance; }

    /**
     * @brief If IPC is in state StateT, returns a state::ScopedStateAccess<StateT>,
     * otherwise return std::nullopt. Not suitable for use in online rendering
     * mode.
     */
    template<typename StateT>
    std::optional<ScopedStateAccess<StateT>> getCurrentState() {
        if (!isInState<StateT>()) return std::nullopt;
        return ScopedStateAccess<StateT>{_state_change_mu,
                                         getCurrentStateUnlocked()};
    }

    friend class state::StateBase;
    friend class state::Disconnected;
    friend class state::Connected;
    friend class state::Subscribed;
    friend class state::OfflineRendering;
    friend class state::ObjectDeleted;
    friend class state::ErrorState;
};

} // namespace ambilink::ipc
