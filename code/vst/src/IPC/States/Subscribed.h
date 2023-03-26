#pragma once
#include "Common.h"
#include "State.h"

#include <IPC/Commands.h>

#include <spdlog/spdlog.h>

namespace ambilink::ipc::state {

/**
 * @brief [IPC state]: Subscribed to object, and in real-time rendering mode.
 */
class Subscribed : public State<Subscribed>, public SubscribedObjectInfoHolder
{
    /// @brief subscribes to a new object, must be unsubbed when called.
    void subscribe(const juce::String& object_name);

    std::atomic<bool> _should_switch_to_deleted_state{false};

    /// @brief interval for for direction and distance ValueTree prop updates. 0
    /// means never. (disabled for now since gui is not implemented.)
    constexpr static std::chrono::milliseconds val_tree_upd_interval{0};
    std::chrono::steady_clock::time_point _last_val_tree_dir_update
      = std::chrono::steady_clock::now();
    void updateDirectionValTreeProp(Direction&& new_direction,
                                    Distance new_distance);

public:
    /// @brief subscribes to an object
    Subscribed(juce::String object_name, const Connected& prev_state);
    /// @brief stops the sub thread and subscribes to an object.
    Subscribed(juce::String object_name, const ObjectDeleted& prev_state);
    /// @brief sends an INFORM_RENDER_FINISHED request.
    Subscribed(const OfflineRendering& prev_state);

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;
    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;
    std::unique_ptr<StateBase>
      reqRepThreadIdleUpdate(std::function<bool()> should_stop) final;

    void onShutdown() final {
        sendObjectUnsubRequest(_reqrep_sock, _obj_info.id);
        spdlog::debug("Unsubscribed from object (shutdown): {}({})",
                      _obj_info.name.toStdString(), _obj_info.id);
    }

    implement_GetStateName(Subscribed);
};
} // namespace ambilink::ipc::state
