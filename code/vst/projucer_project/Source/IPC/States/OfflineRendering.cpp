#include "OfflineRendering.h"

#include <ValueIDs.h>
#include <Math/Math.h>
#include <IPC/Exceptions.h>
#include <IPC/Protocol.h>

#include "Subscribed.h"
#include "ObjectDeleted.h"

namespace ambilink::ipc::state {

OfflineRendering::OfflineRendering(const Subscribed& prev_state)
  : State(prev_state, SupportedCommands{}),
    SubscribedObjectInfoHolder(prev_state) {
    sendSimpleCommand(_reqrep_sock,
                      constants::ReqRepCommand::PREPARE_TO_RENDER);
    auto reply_data_reader = sendSimpleCommand(
      _reqrep_sock, constants::ReqRepCommand::GET_ANIMATION_INFO);

    _frame_count = reply_data_reader.read<size_t>();
    _fps = reply_data_reader.read<float>();
    _animation_length_seconds = _frame_count / _fps;

    _num_slices
      = std::ceil(static_cast<double>(_frame_count) / max_frames_per_slice);
    _last_slice_frame_count = _frame_count % max_frames_per_slice;
    if (_last_slice_frame_count == 0)
        _last_slice_frame_count = max_frames_per_slice;

    _slices.resize(_num_slices);

    _max_slice_length_seconds = static_cast<float>(max_frames_per_slice) / _fps;

    _first_fetch_time = std::chrono::steady_clock::now() + first_fetch_delay;
}

DirectionWithDistance
  OfflineRendering::getDirectionAndDistanceAtTime(float time_secs) {
    jassert(time_secs >= 0);
    size_t target_slice = std::floor(time_secs / _max_slice_length_seconds);
    size_t target_frame
      = std::floor(std::fmod(time_secs * _fps, max_frames_per_slice));

    if (target_slice >= _num_slices) {
        target_slice = _num_slices - 1;
        target_frame = _last_slice_frame_count - 1;
    } else if (target_slice == _num_slices - 1) {
        target_frame = std::min(_last_slice_frame_count - 1, target_frame);
    }

    while (target_slice >= _slice_to_fetch) {
        // error occured or object deleted, return ASAP to avoid blocking state
        // transition due to state change mutex locked by ScopedThreadAccess.
        if (_rendering_mode_aborted) return {};

        // Busy wait for req/rep thread to fetch data.
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    _slice_being_read = target_slice;
    return _slices[target_slice].at(target_frame);
}

void OfflineRendering::fetchSlice(size_t slice_to_fetch) {
    DataWriter request_data_writer{};
    request_data_writer.write(
      constants::ReqRepCommand::GET_RENDERING_LOCATION_DATA);
    request_data_writer.write(_obj_info.id);

    const size_t start_frame = slice_to_fetch * max_frames_per_slice;
    const size_t end_frame
      = std::min((slice_to_fetch + 1) * max_frames_per_slice, _frame_count) - 1;
    const auto slice_frame_count = end_frame - start_frame + 1;

    request_data_writer.write(start_frame);
    request_data_writer.write(end_frame);

    auto request_data = std::move(request_data_writer).release_data();

    _reqrep_sock.send(nng::view{request_data.data(), request_data.size()});
    auto reply_data_reader = DataReader{_reqrep_sock.recv()};

    checkReplyStatus(reply_data_reader.read<constants::ReqRepStatusCode>());

    auto bytes
      = reply_data_reader.readBytes(slice_frame_count * sizeof(glm::vec3));
    auto locations = std::span<glm::vec3>(
      reinterpret_cast<glm::vec3*>(bytes.data()), slice_frame_count);

    jassert(_slices[slice_to_fetch].empty());
    _slices[slice_to_fetch].reserve(slice_frame_count);
    for (auto&& location : locations) {
        _slices[slice_to_fetch].emplace_back(
          math::directionFromCamSpaceLocation(location));
    }
}

std::unique_ptr<StateBase>
  OfflineRendering::reqRepThreadIdleUpdate(std::function<bool()>) {
    try {
        if (_should_switch_to_deleted_state) {
            return std::make_unique<ObjectDeleted>(*this);
        }

        // delay first fetch, so other plugin instances can transition to
        // OfflineRendering state faster
        if (!_first_fetch_done) {
            if (std::chrono::steady_clock::now() < _first_fetch_time)
                return nullptr;
            else
                _first_fetch_done = true;
        }

        while (_slice_to_fetch < _num_slices
               && _slice_to_fetch - _slice_being_read + 1
                    <= max_cached_slices) {
            fetchSlice(_slice_to_fetch);
            _slice_to_fetch++;
        }

        if (_num_slices <= max_cached_slices) return nullptr;

        size_t slice_to_clear;
        for (slice_to_clear = 0;
             slice_to_clear
             < std::min(_last_uncleared_slice + 1, _slice_being_read.load());
             slice_to_clear++) {
            _slices[slice_to_clear].clear();
        }
        _last_uncleared_slice = slice_to_clear;
        return nullptr;
    } catch (...) {
        _rendering_mode_aborted.store(true);
        throw;
    }
}

std::unique_ptr<StateBase>
  OfflineRendering::processCommand(ambilink::events::EventBase& command,
                                   std::function<bool()>) {
    try {
        events::Dispatcher dispatcher(command);
        std::unique_ptr<StateBase> next_state{nullptr};

        dispatchObjListUpdCommand(*this, dispatcher, _reqrep_sock);

        dispatcher.dispatch<commands::DisableRenderingMode>(
          [this, &next_state](const commands::DisableRenderingMode&) {
              _rendering_mode_aborted.store(true);            
              next_state = std::make_unique<Subscribed>(*this);
              return true;
          });

        return next_state;
    } catch (...) {
        _rendering_mode_aborted.store(true);
        throw;
    }
}

void OfflineRendering::onPubSubCommand(constants::PubSubMsgType msg,
                                       DataReader& reader) {
    using MsgType = constants::PubSubMsgType;
    try {
        switch (msg) {
            case MsgType::OBJECT_DELETED:
                _rendering_mode_aborted.store(true);
                _should_switch_to_deleted_state.store(true);
            case MsgType::OBJECT_RENAMED:
                _obj_info.name = decodeObjectName(reader);
                queuePropUpdate(ids::object_name, _obj_info.name);
                break;
            case MsgType::OBJECT_POSITION_UPDATED:
                break;
        }
    } catch (...) {
        _rendering_mode_aborted.store(true);
        throw;
    }
}

} // namespace ambilink::ipc::state
