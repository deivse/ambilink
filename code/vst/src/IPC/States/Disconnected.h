#pragma once
#include "State.h"

#include <IPC/Commands.h>

namespace ambilink::ipc::state {

/**
 * @brief [IPC state]: Initial state, constantly tries to 
 * transition to Connected.
 */
class Disconnected : public State<Disconnected>
{
    nng::socket_view _pubsub_sock;

public:
    Disconnected(nng::socket_view reqrep_sock, nng::socket_view pubsub_sock,
                 std::atomic<Direction>& current_direction,
                 std::atomic<Distance>& current_distance,
                   juce::ValueTree& other_plugin_state,
                 const SubThreadController& sub_thread_ctrl)
      : State(reqrep_sock, current_direction, current_distance,
              other_plugin_state, sub_thread_ctrl, utils::TypeList{}),
        _pubsub_sock(pubsub_sock) {}

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;
    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;
    std::unique_ptr<StateBase>
      reqRepThreadIdleUpdate(std::function<bool()> should_stop) final;

    void sendPingRequest() final {} // Do not ping when already disconnected.

    implement_GetStateName(Disconnected);
};
} // namespace ambilink::ipc::state
