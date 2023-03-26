#pragma once
#include "State.h"

#include <IPC/Commands.h>

namespace ambilink::ipc::state {

static_assert(std::atomic<size_t>::is_always_lock_free);

/**
 * @brief [IPC state]: Subscribed to object, and in offline rendering mode.
 */
class OfflineRendering : public State<OfflineRendering>,
                         public SubscribedObjectInfoHolder
{
    using SupportedCommands = utils::TypeList<commands::DisableRenderingMode,
                                              commands::UpdateObjectList>;

    /// @brief number of rendering data frames requested at once.
    constexpr static size_t max_frames_per_slice = 250;
    /// @brief memory limit for cached rendering location data.
    constexpr static size_t max_cached_bytes = 36000;

    constexpr static size_t max_cached_frames
      = max_cached_bytes / sizeof(DirectionWithDistance);
    constexpr static size_t max_cached_slices
      = std::max<size_t>(max_cached_frames / max_frames_per_slice, 1);

    // First rendering data request is delayed, so other plugin instances can
    // transition to OfflineRendering state faster
    constexpr static std::chrono::milliseconds first_fetch_delay{300};
    std::chrono::steady_clock::time_point _first_fetch_time;
    bool _first_fetch_done{false};

    float _fps;
    size_t _frame_count;
    float _animation_length_seconds;
    size_t _num_slices;
    size_t _last_slice_frame_count;
    float _max_slice_length_seconds;

    size_t _last_uncleared_slice = 0;
    std::atomic<size_t> _slice_being_read = 0;
    std::atomic<size_t> _slice_to_fetch = 0;

    std::vector<std::vector<DirectionWithDistance>> _slices{};

    // flag indicating that getDirectionAndDistanceAtTime should return ASAP
    // (possibly with incorrect data).
    std::atomic<bool> _rendering_mode_aborted{false};
    std::atomic<bool> _should_switch_to_deleted_state{false};

    /**
     * @brief requests the specified slice of rendering data, calculated
     * direction and distance from camera space coordinates, and stores the
     * result in `_slices`.
     */
    void fetchSlice(size_t slice_to_fetch);

public:
    /**
     * @brief Sends PREPARE_TO_RENDER request, gets animation info, calculates
     * internal variables.
     */
    OfflineRendering(const Subscribed& prev_state);

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;

    /**
     * @brief Get the direction and distance to the subscribed object at the
     * specified time in the animation. May block until the required data is
     * received.
     *
     * @param time_secs time in the animation
     * @return DirectionWithDistance direction and distance to object
     */
    DirectionWithDistance getDirectionAndDistanceAtTime(float time_secs);

    /**
     * @brief Continuously fetches rendering data from the Blender plugin and
     * cleans up data that's no longer needed. Pauses if memory limit is
     * reached.
     */
    std::unique_ptr<StateBase>
      reqRepThreadIdleUpdate(std::function<bool()> should_stop) final;

    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;

    void onShutdown() final {
        sendObjectUnsubRequest(_reqrep_sock, _obj_info.id);
    }

    implement_GetStateName(OfflineRendering);
};

} // namespace ambilink::ipc::state
