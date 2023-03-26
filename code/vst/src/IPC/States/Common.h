#pragma once
#include <juce_events/juce_events.h>
#include <juce_data_structures/juce_data_structures.h>
#include <nngpp/socket_view.h>

#include <DataTypes.h>
#include <Events/Consumers.h>
#include <IPC/Constants.h>
#include <IPC/ByteIO.h>

namespace ambilink::ipc::state {
class StateBase;

/// @brief if the dispatcher contains an UpdateObjectList command, requests
/// the object list and schedules a prop update via `curr_state`.
void dispatchObjListUpdCommand(StateBase& curr_state,
                               events::Dispatcher& dispatcher,
                               nng::socket_view& reqrep_sock);

/// @brief sends unsub request, throws if reply status is not SUCCESS.
void sendObjectUnsubRequest(nng::socket_view& reqrep_sock,
                            AmbilinkID object_to_unsub_from);

/// @brief sends a simple (containing only the command id) command, throws if
/// reply status is not SUCCESS. Returns a DataReader with the reply data with
/// the status byte already read.
DataReader sendSimpleCommand(nng::socket_view& reqrep_sock,
                             constants::ReqRepCommand command);

struct SubscribedObjectInfo
{
    AmbilinkID id;
    juce::String name;
};

/**
 *@brief Allows state objects to control the subscriber thread without directly
 *referencing the IPCClient.
 */
struct SubThreadController
{
    std::function<void(AmbilinkID)> start;
    std::function<void()> stop;
};

/// @brief Helper base class for states that hold a SubscribedObjectInfo
class SubscribedObjectInfoHolder
{
protected:
    SubscribedObjectInfo _obj_info{};

public:
    SubscribedObjectInfoHolder() = default;
    SubscribedObjectInfoHolder(const SubscribedObjectInfoHolder& other)
      : _obj_info{other.getObjectInfo()} {}

    SubscribedObjectInfo getObjectInfo() const { return _obj_info; }
};

} // namespace ambilink::ipc::state
