#pragma once
#include "State.h"

#include <IPC/Commands.h>

namespace ambilink::ipc::state {

/**
 * @brief [IPC state]: exception ocuured in IPC communication.
 */
class ErrorState : public State<ErrorState>
{
    using SupportedCommands = utils::TypeList<commands::Connect>;

    std::exception_ptr _exception;
    std::string_view _prev_state_name;

public:
    /**
     * @brief Sets the ids::ipc_error ValueTree property, stops subscriber
     * thread and tries to sedn unsubscribe request if needed.
     *
     * @param error the exception that resulted in transition to error state
     * @param prev_state the previous state
     */
    ErrorState(std::exception_ptr error, const StateBase& prev_state);

    std::unique_ptr<StateBase>
      processCommand(ambilink::events::EventBase& command,
                     std::function<bool()> should_stop) final;
    void onPubSubCommand(constants::PubSubMsgType msg,
                         DataReader& reader) final;

    /**
     * @brief used to update the error message if another exception occurs while
     * already in error state.
     */
    void setException(std::exception_ptr new_error);

    /**
     * @brief get the error description for the exception that led to
     * transitioning to error state, or the last exception that occured while in
     * error state.
     */
    std::string getErrorDescription();

    /**
     * @brief rethrow the exception that led to transitioning to error state, or
     * the last exception that occured while in error state.
     */
    void rethrowException();

    implement_GetStateName(ErrorState);
};

} // namespace ambilink::ipc::state
