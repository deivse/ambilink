#include "Connected.h"

#include <Events/Consumers.h>
#include <IPC/Client.h>
#include <IPC/Protocol.h>
#include <ValueIDs.h>

#include "Subscribed.h"
#include "Disconnected.h"
#include "ObjectDeleted.h"
#include "ErrorState.h"

#include <nngpp/socket.h>

namespace ambilink::ipc::state {

Connected::Connected(const Disconnected& prev_state,
                     nng::socket_view pubsub_sock)
  : State(prev_state, SupportedCommands{}) {
    _reqrep_sock.dial(constants::reqrep_addr);
    pubsub_sock.dial(constants::pubsub_addr);
}

Connected::Connected(const Subscribed& prev_state)
  : State(prev_state, SupportedCommands{}) {
    _sub_thread_ctrl.stop();
    sendObjectUnsubRequest(_reqrep_sock, prev_state.getObjectInfo().id);
    queuePropUpdate(ids::object_name, {});
}

Connected::Connected(const ObjectDeleted& prev_state)
  : State(prev_state, SupportedCommands{}) {
    // TODO: when adding obj restore support, unsub here
    _sub_thread_ctrl.stop();
    queuePropUpdate(ids::object_name, {});
}

Connected::Connected(const ErrorState& prev_state)
  : State(prev_state, SupportedCommands{}) {}

std::unique_ptr<StateBase>
  Connected::processCommand(ambilink::events::EventBase& command,
                            std::function<bool()>) {
    events::Dispatcher dispatcher(command);

    dispatchObjListUpdCommand(*this, dispatcher, _reqrep_sock);

    std::unique_ptr<StateBase> next_state{nullptr};
    dispatcher.dispatch<commands::SubscribeToObject>(
      [this, &next_state](const commands::SubscribeToObject& cmd) {
          next_state = std::make_unique<Subscribed>(cmd.object_name, *this);
          return true;
      });

    return next_state;
}

std::unique_ptr<StateBase> Connected::reqRepThreadIdleUpdate(std::function<bool()>) {
    const auto& other_state = getOtherPluginState();
    if (other_state[ids::object_name].isString()) {
        return std::make_unique<Subscribed>(other_state[ids::object_name], *this);
    }
    return nullptr;
}

void Connected::onPubSubCommand(constants::PubSubMsgType, DataReader&) {}

} // namespace ambilink::ipc::state
