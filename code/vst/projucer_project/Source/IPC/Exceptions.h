#pragma once

#include <stdexcept>
#include <fmt/format.h>

namespace ambilink::ipc::exceptions {

/**
 * @brief Base class for all IPC-related exceptions.
 *
 */
struct Base : public std::runtime_error
{
    using std::runtime_error::runtime_error;

    template<typename... Args>
    Base(fmt::format_string<Args...> fmt, Args&&... args)
      : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...)) {}
};

#define DEFINE_SIMPLE_EXCEPTION_CLASS(ClassName) \
    struct ClassName : public Base               \
    {                                            \
        using Base::Base;                        \
                                                 \
        ClassName() : Base{#ClassName} {}        \
    }

/**
 * @brief Unable to establish connection with the blender plugin.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(ConnectionRefused);
/**
 * @brief The VST plugin is not subscribed to any object's updates.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(NoActiveSubscription);
/**
 * @brief The blender plugin reported an object not found error.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(ObjectNotFound);
/**
 * @brief The blender plugin reported an internal error.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(InternalErrorResponse);
/**
 * @brief The blender plugin responded with UNKNOWN_COMMAND.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(UnknownCommandResponse);
/**
 * @brief The VST plugin received an unexpected message from the blender plugin.
 * (e.g. unknown status code, unknown pubsub msg type, etc.)
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(ProtocolError);

/**
 * @brief The VST plugin sent invalid data to the Blender plugin.
 */
DEFINE_SIMPLE_EXCEPTION_CLASS(InvalidRequestData);
} // namespace ambilink::ipc::exceptions
