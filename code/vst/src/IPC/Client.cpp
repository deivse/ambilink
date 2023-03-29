#include "Client.h"

#include <nngpp/protocol/req0.h>
#include <nngpp/protocol/sub0.h>

#include <ValueIDs.h>
#include <string>
#include "States/Disconnected.h"
#include "States/ErrorState.h"
#include <spdlog/spdlog.h>

namespace ambilink::ipc {

IPCClient::IPCClient(juce::ValueTree& other_state)
  : _other_plugin_state(other_state), _reqrep_sock(nng::req::open()),
    _pubsub_sock(nng::sub::open()),
    _sub_thread_ctrl(makeSubThreadController()) {
    _reqrep_sock.set_opt_ms(NNG_OPT_RECVTIMEO,
                            IPCClient::reqrep_recv_timeout.count());
    _reqrep_sock.set_opt_ms(NNG_OPT_SENDTIMEO,
                            IPCClient::reqrep_send_timeout.count());
    _pubsub_sock.set_opt_ms(NNG_OPT_RECVTIMEO,
                            IPCClient::pubsub_recv_timeout.count());
    _pubsub_sock.set_opt(NNG_OPT_SUB_SUBSCRIBE, {});

    _current_state_id = static_cast<size_t>(state::Disconnected::id);
    _states[_current_state_id] = makeDisconnectedState();
    updateIPCStateValueTreeProp();
    _req_rep_thread = std::thread{[this]() { requestorThreadFunc(); }};
}

IPCClient::~IPCClient() {
    spdlog::debug("In IPCClient destructor.");
    _req_rep_thread_should_stop = true;
    _req_rep_thread_cond_var.notify_one();
    spdlog::debug("About to join reqrep thread.");
    _req_rep_thread.join();

    _sub_thread_should_stop = true;
    if (_sub_thread.joinable()) {
        spdlog::debug("About to join sub thread");
        _sub_thread.join();
    }

    std::lock_guard guard{_state_change_mu};
    try {
        getCurrentStateUnlocked().onShutdown();
    } catch (const std::exception& e) {
        spdlog::error("Exception in onShutDown() for state {}: {}",
                      getCurrentStateUnlocked().getStateName(), e.what());
    }
    _states[_current_state_id].reset(nullptr);

    // nng_close sometimes hangs; suspect this is due to an NNG bug.
    // https://github.com/nanomsg/nng/issues/1543
    // https://github.com/nanomsg/nng/pull/1616
    //
    // Temporary solution - leak memory and let OS cleanup.
    // From my testing, this doesn't affect further communication in any way.
    _reqrep_sock.release();
    _pubsub_sock.release();
}

state::SubThreadController IPCClient::makeSubThreadController() {
    auto start = [this](AmbilinkID id) {
        if (_sub_thread.joinable()) {
            _sub_thread_should_stop = true;
            _sub_thread.join();
        }
        _obj_id = id;
        _sub_thread_should_stop = false;
        _sub_thread = std::thread([this]() { subscriberThreadFunc(); });
    };
    auto stop = [this]() { _sub_thread_should_stop = true; };
    return {std::move(start), std::move(stop)};
}

std::unique_ptr<state::Disconnected> IPCClient::makeDisconnectedState() {
    return std::make_unique<state::Disconnected>(
      _reqrep_sock, _pubsub_sock, _current_direction, _current_distance,
      _other_plugin_state, _sub_thread_ctrl);
}

void IPCClient::transitionToErrorOrDisconnectedState() {
    std::exception_ptr exception{std::current_exception()};
    try {
        std::rethrow_exception(exception);
    } catch (const nng::exception& e) {
        if (reconnectable_ipc_errors.contains(e.get_error())) {
            setNextState(makeDisconnectedState());
            return;
        }
    } catch (...) {
    }
    if (isInState<state::ErrorState>()) {
        getCurrentState<state::ErrorState>().value()->setException(exception);
    } else {
        setNextState(std::make_unique<state::ErrorState>(
          exception, getCurrentStateUnlocked()));
    }
}

void IPCClient::requestorThreadFunc() {
    while (!_req_rep_thread_should_stop) {
        try {
            if (eventQueued()) {
                auto command = getEvent();
                jassert(command);
                setNextState(
                  getCurrentStateUnlocked().processCommand(*command, [this]() {
                      return _req_rep_thread_should_stop.load();
                  }));
            }
            setNextState(getCurrentStateUnlocked().reqRepThreadIdleUpdate(
              [this]() { return _req_rep_thread_should_stop.load(); }));
            getCurrentStateUnlocked().sendPingRequest();

            if (eventQueued()) continue;
            std::unique_lock lock{_req_rep_thread_cond_var_mu};
            _req_rep_thread_cond_var.wait_for(lock,
                                              std::chrono::milliseconds(50));
        } catch (...) {
            transitionToErrorOrDisconnectedState();
        }
    }
    _req_rep_thread_should_stop = false;
}

void IPCClient::subscriberThreadFunc() {
    while (!_sub_thread_should_stop) {
        try {
            auto msg_data_reader = DataReader{_pubsub_sock.recv()};
            assert(_obj_id.has_value());
            if (*_obj_id != msg_data_reader.read<AmbilinkID>()) continue;

            using MsgType = constants::PubSubMsgType;

            auto msg_type = msg_data_reader.read<MsgType>();
            getCurrentStateUnlocked().onPubSubCommand(msg_type,
                                                      msg_data_reader);
        } catch (const nng::exception& err) {
            if (err.get_error() == nng::error::timedout) continue;
            transitionToErrorOrDisconnectedState();
        } catch (...) {
            transitionToErrorOrDisconnectedState();
        }
    }
}

void IPCClient::setNextState(std::unique_ptr<state::StateBase>&& next_state) {
    if (!next_state) return;
    const auto prev_state_id = _current_state_id.load();
    const auto next_state_id = static_cast<size_t>(next_state->getId());
    if (prev_state_id == next_state_id) {
        jassert(false);
        return;
    }

    spdlog::debug("State transition begin: {} -> {}",
                  _states[prev_state_id]->getStateName(),
                  next_state->getStateName());

    _states[next_state_id] = std::move(next_state);

    {
        std::lock_guard guard{_state_change_mu};
        _current_state_id.store(next_state_id);
        _states[prev_state_id].reset(nullptr);
    }

    spdlog::debug("State transition successful.");
    triggerAsyncUpdate();
}

StateBase& IPCClient::getCurrentStateUnlocked() {
    return *_states[_current_state_id];
}

void IPCClient::updateIPCStateValueTreeProp() {
    _other_plugin_state.setProperty(ids::ipc_client_state,
                                    static_cast<int>(_current_state_id.load()),
                                    nullptr);
}
} // namespace ambilink::ipc
