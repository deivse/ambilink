#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <juce_core/juce_core.h>

#include "Constants.h"
#include "ByteIO.h"
#include "Exceptions.h"

namespace ambilink::ipc {

/// @brief concats the command byte and the data
std::vector<uint8_t> encodeReqRepRequest(constants::ReqRepCommand command,
                                         std::vector<uint8_t>&& data);

/// @brief returns a single-byte vector with the command byte
inline std::vector<uint8_t>
  encodeReqRepRequest(constants::ReqRepCommand command) {
    return {static_cast<uint8_t>(command)};
}

/**
 * @brief Creates request data for given command, with command-specific data
 * given by an array of values of type T.
 */
template<typename T>
std::vector<uint8_t> encodeReqRepRequest(constants::ReqRepCommand command,
                                         const T* data = nullptr,
                                         size_t size = 0) {
    DataWriter writer{};
    writer.write(command);

    // important check, because data and size are nullptr and 0, resp. by
    // default.
    if (!data || !size) return std::move(writer).release_data();

    writer.write_bytes(reinterpret_cast<const uint8_t*>(data), size);
    return std::move(writer).release_data();
}

/// @brief encode an object name
std::vector<uint8_t> encodeObjectName(const std::u8string& name);

/// @brief decode an object name
juce::String decodeObjectName(DataReader& reader);
/// @brief decode a list of object names
juce::StringArray decodeObjectList(DataReader& reader);

/**
 * @brief Check the status code of a req/rep reply, throw on error statuses.
 *
 * @param status_code
 * @param throw_if_object_not_found if true, throw if status ==
 * ReqRepStatusCode::OBJECT_NOT_FOUND
 */
void checkReplyStatus(constants::ReqRepStatusCode status_code,
                      bool throw_if_object_not_found = true);

} // namespace ambilink::ipc
