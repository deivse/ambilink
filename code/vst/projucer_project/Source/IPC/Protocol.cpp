#include "Protocol.h"
#include "ByteIO.h"

namespace ambilink::ipc {
std::vector<uint8_t> encodeReqRepRequest(constants::ReqRepCommand command,
                                         std::vector<uint8_t>&& data) {
    DataWriter writer{};
    writer.write(command);
    writer.write_bytes(std::move(data));
    return std::move(writer).release_data();
}

void checkReplyStatus(constants::ReqRepStatusCode status_code,
                      bool throw_if_object_not_found) {
    using status = constants::ReqRepStatusCode;
    switch (status_code) {
        case status::SUCCESS:
            return;
        case status::OBJECT_NOT_FOUND:
            if (throw_if_object_not_found) throw exceptions::ObjectNotFound();
            return;
        case status::UNKNOWN_COMMAND:
            throw exceptions::UnknownCommandResponse();
        case status::INTERNAL_ERROR:
            throw exceptions::InternalErrorResponse();
        case status::INVALID_REQUEST_DATA:
            throw exceptions::InvalidRequestData{};
        default:
            throw exceptions::ProtocolError("Unknown status code: {:#04x}",
                                            static_cast<uint8_t>(status_code));
    }
}

std::vector<uint8_t> encodeObjectName(const std::u8string& name) {
    if (name.size() > std::numeric_limits<uint8_t>::max())
        throw std::invalid_argument(
          "Blender object names may not be more than 63 characters long.");
    DataWriter writer{};

    auto length = name.size();
    if (std::endian::native == std::endian::big) {
        writer.write<uint8_t>(*(reinterpret_cast<uint8_t*>(&length)
                                + (sizeof(decltype(length)) - 1)));
    } else {
        writer.write(*reinterpret_cast<uint8_t*>(&length));
    }
    writer.write_string(name);
    return std::move(writer).release_data();
}

juce::String decodeObjectName(DataReader& reader) {
    auto name_size = reader.read<uint8_t>();
    auto name_data = reader.readBytes(name_size);
    return juce::String::fromUTF8(
      reinterpret_cast<const char*>(name_data.data()), name_data.size());
}

juce::StringArray decodeObjectList(DataReader& reader) {
    juce::StringArray retval{};
    while (reader.remaining() > 0) {
        retval.add(decodeObjectName(reader));
    }
    return retval;
}

} // namespace ambilink::ipc
