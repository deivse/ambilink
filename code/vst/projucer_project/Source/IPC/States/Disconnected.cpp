#include "Disconnected.h"
#include "Connected.h"
#include <Events/Consumers.h>
#include <IPC/Client.h>

#include <nngpp/error.h>

namespace ambilink::ipc::state {

std::unique_ptr<StateBase>
  Disconnected::processCommand(ambilink::events::EventBase&,
                               std::function<bool()>) {
    return nullptr;
}

std::unique_ptr<StateBase>
  Disconnected::reqRepThreadIdleUpdate(std::function<bool()> should_stop) {
    while (!should_stop()) {
        try {
            return std::make_unique<Connected>(*this, _pubsub_sock);
        } catch (const nng::exception& e) {
            if (e.get_error() == nng::error::connrefused) {
                /*
                `nng_close` would block indefenitely when an instance of the IPC
                Client was being destroyed, but 1 or more other instances were
                still trying to connect. This is fixed by adding a short delay
                between reconnect attempts.
                */
                std::this_thread::sleep_for(std::chrono::milliseconds{250});
                continue;
            }
            throw;
        }
    }
    return nullptr;
}

void Disconnected::onPubSubCommand(constants::PubSubMsgType, DataReader&) {
}

} // namespace ambilink::ipc::state
