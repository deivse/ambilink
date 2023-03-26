#include "ObjectDeleted.h"
#include "Subscribed.h"
#include "OfflineRendering.h"
#include "Connected.h"

namespace ambilink::ipc::state {

ObjectDeleted::ObjectDeleted(const Subscribed& prev_state)
  : State(prev_state, SupportedCommands{}),
    SubscribedObjectInfoHolder(prev_state) {
    queuePropUpdate(ids::object_deleted, true);
}

ObjectDeleted::ObjectDeleted(const OfflineRendering& prev_state)
  : State(prev_state, SupportedCommands{}),
    SubscribedObjectInfoHolder(prev_state) {
    queuePropUpdate(ids::object_deleted, true);
    sendSimpleCommand(_reqrep_sock,
                      constants::ReqRepCommand::INFORM_RENDER_FINISHED);
}

std::unique_ptr<StateBase>
  ObjectDeleted::processCommand(ambilink::events::EventBase& command,
                                std::function<bool()>) {
    events::Dispatcher dispatcher(command);
    std::unique_ptr<StateBase> next_state{nullptr};

    dispatchObjListUpdCommand(*this, dispatcher, _reqrep_sock);

    dispatcher.dispatch<commands::SubscribeToObject>(
      [this, &next_state](const commands::SubscribeToObject& cmd) {
          next_state = std::make_unique<Subscribed>(cmd.object_name, *this);
          return true;
      });

    dispatcher.dispatch<commands::Unsubscribe>(
      [this, &next_state](const commands::Unsubscribe&) {
          next_state = std::make_unique<Connected>(*this);
          return true;
      });

    return next_state;
}

void ObjectDeleted::onPubSubCommand(constants::PubSubMsgType, DataReader&) {
    // TODO process object restored msg if implemented
}

} // namespace ambilink::ipc::state
