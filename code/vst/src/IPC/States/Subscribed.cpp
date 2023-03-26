#include "Subscribed.h"
#include <IPC/Protocol.h>
#include <Events/Consumers.h>
#include <Math/Math.h>

#include <ValueIDs.h>

#include "Connected.h"
#include "OfflineRendering.h"
#include "ObjectDeleted.h"

namespace ambilink::ipc::state {

using SupportedCommands
  = utils::TypeList<commands::Unsubscribe, commands::EnableRenderingMode,
                    commands::UpdateObjectList, commands::SubscribeToObject>;

Subscribed::Subscribed(juce::String object_name, const Connected& prev_state)
  : State(prev_state, SupportedCommands{}) {
    subscribe(std::move(object_name));
}

Subscribed::Subscribed(juce::String object_name,
                       const ObjectDeleted& prev_state)
  : State(prev_state, SupportedCommands{}) {
    _sub_thread_ctrl.stop();
    subscribe(std::move(object_name));
}

Subscribed::Subscribed(const OfflineRendering& prev_state)
  : State(prev_state, SupportedCommands{}),
    SubscribedObjectInfoHolder(prev_state) {
    sendSimpleCommand(_reqrep_sock,
                      constants::ReqRepCommand::INFORM_RENDER_FINISHED);
}

void Subscribed::subscribe(const juce::String& object_name) {
    auto request_data = encodeReqRepRequest(
      constants::ReqRepCommand::OBJ_SUB,
      encodeObjectName(utils::u8stringFromJuceString(object_name)));
    _reqrep_sock.send(nng::view{request_data.data(), request_data.size()});

    auto reply_data_reader = DataReader{_reqrep_sock.recv()};

    auto status_code = reply_data_reader.read<constants::ReqRepStatusCode>();

    try {
        checkReplyStatus(status_code);
    } catch (const exceptions::ObjectNotFound&) {
        _should_switch_to_deleted_state = true;
        _obj_info = {0, std::move(object_name)};
        queuePropUpdate(ids::object_name, _obj_info.name);
        return;
    }

    _obj_info = {reply_data_reader.read<AmbilinkID>(), std::move(object_name)};

    queuePropUpdate(ids::object_name, _obj_info.name);
    queuePropUpdate(ids::object_deleted, false);

    _sub_thread_ctrl.start(_obj_info.id);
}

std::unique_ptr<StateBase>
  Subscribed::processCommand(ambilink::events::EventBase& command,
                             std::function<bool()>) {
    events::Dispatcher dispatcher(command);
    std::unique_ptr<StateBase> next_state{nullptr};

    dispatchObjListUpdCommand(*this, dispatcher, _reqrep_sock);

    dispatcher.dispatch<commands::Unsubscribe>(
      [this, &next_state](const commands::Unsubscribe&) {
          next_state = std::make_unique<Connected>(*this);
          return true;
      });

    dispatcher.dispatch<commands::SubscribeToObject>(
      [this](const commands::SubscribeToObject& sub_cmd) {
          _sub_thread_ctrl.stop();
          sendObjectUnsubRequest(_reqrep_sock, _obj_info.id);
          subscribe(sub_cmd.object_name);
          return true;
      });

    dispatcher.dispatch<commands::EnableRenderingMode>(
      [this, &next_state](const commands::EnableRenderingMode&) {
          next_state = std::make_unique<OfflineRendering>(*this);
          return true;
      });

    return next_state;
}

void Subscribed::onPubSubCommand(constants::PubSubMsgType msg,
                                 DataReader& reader) {
    using MsgType = constants::PubSubMsgType;
    switch (msg) {
        case MsgType::OBJECT_DELETED:
            _should_switch_to_deleted_state.store(true);
            break;
        case MsgType::OBJECT_RENAMED:
            _obj_info.name = decodeObjectName(reader);
            queuePropUpdate(ids::object_name, _obj_info.name);
            break;
        case MsgType::OBJECT_POSITION_UPDATED:
            static_assert(sizeof(glm::vec3) == 3 * sizeof(float),
                          "Ensure that glm::vec3 is just a float[3]");
            static_assert(sizeof(float) == 4,
                          "float must be 4 bytes for IPC to work correctly.");
            auto&& [direction, distance]
              = math::directionFromCamSpaceLocation(reader.read<glm::vec3>());

            _curr_direction.store(direction);
            _curr_distance.store(distance);
            updateDirectionValTreeProp(std::move(direction),
                                       std::move(distance));
            break;
    }
}

void Subscribed::updateDirectionValTreeProp(Direction&& new_direction,
                                            Distance new_distance) {
    if constexpr (val_tree_upd_interval == std::chrono::milliseconds::zero()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - _last_val_tree_dir_update > val_tree_upd_interval) {
        queuePropUpdate(ids::curr_direction_azimuth_deg,
                        new_direction.azimuth_deg);
        queuePropUpdate(ids::curr_direction_elevation_deg,
                        new_direction.elevation_deg);
        queuePropUpdate(ids::curr_distance, new_distance);

        _last_val_tree_dir_update = std::move(now);
    }
}

std::unique_ptr<StateBase>
  Subscribed::reqRepThreadIdleUpdate(std::function<bool()>) {
    if (_should_switch_to_deleted_state) {
        return std::make_unique<ObjectDeleted>(*this);
    }
    return nullptr;
}

} // namespace ambilink::ipc::state
