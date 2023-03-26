#include "State.h"

#include <IPC/Client.h>
#include "Common.h"

#include <spdlog/spdlog.h>

namespace ambilink::ipc::state {
StateBase::StateBase(nng::socket_view reqrep_sock,
                     std::atomic<Direction>& current_direction,
                     std::atomic<Distance>& current_distance,
                     juce::ValueTree& other_plugin_state,
                     const SubThreadController& sub_thread_ctrl)
  : _other_plugin_state(other_plugin_state), _reqrep_sock(reqrep_sock),
    _curr_direction(current_direction), _curr_distance(current_distance),
    _sub_thread_ctrl(sub_thread_ctrl) {}

StateBase::StateBase(const StateBase& other)
  : _other_plugin_state(other._other_plugin_state), _reqrep_sock(other._reqrep_sock),
    _curr_direction(other._curr_direction),
    _curr_distance(other._curr_distance),
    _sub_thread_ctrl(other._sub_thread_ctrl) {
    _curr_direction = Direction{0,0};
    _curr_distance = 0;
}

StateBase::~StateBase() {
    cancelPendingUpdate();
    while (!_prop_update_queue.empty()) {
        _prop_update_queue.pop();
    };
}

void StateBase::queuePropUpdate(juce::Identifier id, juce::var&& new_val) {
    // Shouldn't ever fail, but add assert to increase queue size if needed.
    if (!_prop_update_queue.pushOrFail({std::move(id), std::move(new_val)})) {
        spdlog::error("Failed to queue prop update - queue full: {}", id.toString().toStdString());
    }
    triggerAsyncUpdate();
}

void StateBase::handleAsyncUpdate() {
    while (!_prop_update_queue.empty()) {
        auto item = _prop_update_queue.pop();
        _other_plugin_state.setProperty(std::move(item.id),
                                        std::move(item.new_value), nullptr);
    }
}

void StateBase::sendPingRequest() {
    if (auto now = std::chrono::steady_clock::now();
        now - last_ping > ping_interval) {
        sendSimpleCommand(_reqrep_sock, constants::ReqRepCommand::PING);
        last_ping = now;
    }
}

} // namespace ambilink::ipc::state
