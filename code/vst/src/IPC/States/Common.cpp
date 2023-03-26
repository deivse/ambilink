#include "Common.h"

#include <IPC/Commands.h>
#include <IPC/Protocol.h>
#include <ValueIDs.h>
#include "State.h"

namespace ambilink::ipc::state {

juce::StringArray getCurrentObjectList(nng::socket_view& reqrep_sock) {
    auto request_data = encodeReqRepRequest(constants::ReqRepCommand::OBJ_LIST);
    reqrep_sock.send(nng::view{request_data.data(), request_data.size()});

    auto reply_data_reader = DataReader{reqrep_sock.recv()};
    checkReplyStatus(reply_data_reader.read<constants::ReqRepStatusCode>());

    return decodeObjectList(reply_data_reader);
}

void dispatchObjListUpdCommand(StateBase& curr_state,
                               events::Dispatcher& dispatcher,
                               nng::socket_view& reqrep_sock) {
    dispatcher.dispatch<commands::UpdateObjectList>(
      [&reqrep_sock, &curr_state](const commands::UpdateObjectList&) {
          curr_state.queuePropUpdate(ids::object_list,
                                     getCurrentObjectList(reqrep_sock));
          return true;
      });
}

void sendObjectUnsubRequest(nng::socket_view& reqrep_sock,
                            AmbilinkID object_to_unsub_from) {
    auto request_data
      = encodeReqRepRequest(constants::ReqRepCommand::OBJ_UNSUB,
                            &object_to_unsub_from, sizeof(AmbilinkID));
    reqrep_sock.send(nng::view{request_data.data(), request_data.size()});

    auto reply_data = reqrep_sock.recv();
    checkReplyStatus(
      constants::ReqRepStatusCode{*static_cast<uint8_t*>(reply_data.data())});
}

DataReader sendSimpleCommand(nng::socket_view& reqrep_sock,
                             constants::ReqRepCommand command) {
    auto request_data = encodeReqRepRequest(command);
    reqrep_sock.send(nng::view{request_data.data(), request_data.size()});
    auto reply_data_reader = DataReader{reqrep_sock.recv()};
    checkReplyStatus(reply_data_reader.read<constants::ReqRepStatusCode>());
    return reply_data_reader;
}

} // namespace ambilink::ipc::state
