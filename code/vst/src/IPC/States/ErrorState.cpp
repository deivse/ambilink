#include "ErrorState.h"

#include <string_view>
#include <fmt/format.h>

#include "Subscribed.h"
#include "OfflineRendering.h"
#include "Connected.h"

#include <spdlog/spdlog.h>

namespace ambilink::ipc::state {
using namespace std::literals;

ErrorState::ErrorState(std::exception_ptr error, const StateBase& prev_state)
  : State(prev_state, SupportedCommands{}),
    _exception(std::move(error)), _prev_state_name{prev_state.getStateName()} {
    spdlog::error("Transitioned to error state from state {}. Error: {}",
                  prev_state.getStateName(), getErrorDescription());
    const auto prev_state_id = prev_state.getId();
    if (prev_state_id == OfflineRendering::id
        || prev_state_id == Subscribed::id) {
        _sub_thread_ctrl.stop();
        try {
            auto object_info
              = dynamic_cast<const SubscribedObjectInfoHolder&>(prev_state)
                  .getObjectInfo();
            spdlog::debug("Unsubscribing from object {}({}).", object_info.name.toStdString(), object_info.id);
            sendObjectUnsubRequest(_reqrep_sock, object_info.id);
        } catch (const std::exception& e) {
            spdlog::error("Error while unsubscribing from object: {}", e.what());
        }
    }
    queuePropUpdate(ids::ipc_error, juce::String{getErrorDescription()});
}

std::unique_ptr<StateBase>
  ErrorState::processCommand(ambilink::events::EventBase& command,
                             std::function<bool()>) {
    std::unique_ptr<StateBase> next_state{nullptr};

    events::Dispatcher dispatcher{command};

    dispatcher.dispatch<commands::Connect>(
      [this, &next_state](const commands::Connect&) {
          next_state = std::make_unique<state::Connected>(*this);
          return true;
      });
    return next_state;
}

void ErrorState::onPubSubCommand(constants::PubSubMsgType, DataReader&) {}

void ErrorState::setException(std::exception_ptr new_error) {
    _exception = new_error;
    queuePropUpdate(ids::ipc_error, juce::String{getErrorDescription()});
}

std::string ErrorState::getErrorDescription() {
    try {
        std::rethrow_exception(_exception);
    } catch (const nng::exception& e) {
        return fmt::format(
          "IPC Communication error in state {}: {}[NNG error code: {}]",
          _prev_state_name, e.what(), static_cast<int>(e.get_error()));
    } catch (const std::runtime_error& e) {
        return fmt::format("Error occured in state {}: {}", _prev_state_name,
                           e.what());
    } catch (...) {
        return fmt::format("Unknown error occured in state {}",
                           _prev_state_name);
    }
}

void ErrorState::rethrowException() { std::rethrow_exception(_exception); }

} // namespace ambilink::ipc::state
