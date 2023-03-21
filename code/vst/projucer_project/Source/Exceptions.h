#pragma once

#include <stdexcept>
#include <string_view>
#include <source_location>

#include <fmt/format.h>

namespace ambilink {


/// @brief Helper for better exception messages.
struct InvalidArgumentError : public std::invalid_argument
{
    template<typename ArgT>
    InvalidArgumentError(std::string_view arg_name,
                         ArgT val,
                         std::string_view reason = "reason unspecified",
                         std::source_location src_loc = std::source_location::current())
      : std::invalid_argument(fmt::format(
        "Invalid argument \"{}\": {} ({}) [in func `{}` called in file `{}` on line {}]",
        arg_name, val, reason, src_loc.function_name(), src_loc.file_name(), src_loc.line())) {}
};
} // namespace ambilink
