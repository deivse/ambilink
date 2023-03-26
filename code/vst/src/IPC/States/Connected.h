#pragma once

#include "State.h"

#include <mutex>
#include <juce_core/juce_core.h>

#include "Common.h"
#include <IPC/Commands.h>

namespace ambilink::ipc::state {

/**
 * @brief [IPC state]: IPC connected, but not subscribed to an object.
 */
class Connected : public State<Connected>
{
    using SupportedCommands = utils::TypeList<commands::UpdateObjectList,
                                              commands::SubscribeToObject>;
public:
    /**
     * @brief Tries to initialise communication with a blender plugin instance.
     * @throws nng::exception if connection unsuccesful
     */
    Connected(const Disconnected& prev_state, nng::socket_view pubsub_sock);

    /// @brief Stops the sub thread and unsubscribes.
    Connected(const Subscribed& prev_state);

    /// @brief Stops the sub thread and unsubscribes.
    Connected(const ObjectDeleted& prev_state);

    /// @brief no-op transition.
    Connected(const ErrorState& prev_state);

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;

    std::unique_ptr<StateBase>
      reqRepThreadIdleUpdate(std::function<bool()>) final;

    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;

    implement_GetStateName(Connected);
};
} // namespace ambilink::ipc::state
