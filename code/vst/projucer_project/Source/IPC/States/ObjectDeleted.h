#pragma once
#include "State.h"

#include <IPC/Commands.h>
#include "Common.h"

namespace ambilink::ipc::state {

/**
 * @brief [IPC state]: The that the VST was subscribed to was deleted.
 */
class ObjectDeleted : public State<ObjectDeleted>,
                      public SubscribedObjectInfoHolder
{
    using SupportedCommands
      = utils::TypeList<commands::SubscribeToObject, commands::Unsubscribe>;

public:
    /// @brief updates the ids::object_deleted ValueTree property.
    ObjectDeleted(const Subscribed& prev_state);

    /// @brief updates the ids::object_deleted ValueTree property.
    ObjectDeleted(const OfflineRendering& prev_state);

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;
    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;

    implement_GetStateName(ObjectDeleted);
};
} // namespace ambilink::ipc::state
