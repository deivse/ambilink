#pragma once

#include <cstdint>
namespace ambilink::ipc::constants {

// Not using std::string_view because of nngpp interface
constexpr auto reqrep_addr = "ipc:///tmp/ambilink_reqrep";
constexpr auto pubsub_addr = "ipc:///tmp/ambilink_pubsub";

enum class ReqRepCommand : uint8_t
{
    OBJ_LIST = 0x01,
    OBJ_SUB = 0x02,
    OBJ_UNSUB = 0x03,
    PREPARE_TO_RENDER = 0x04,
    INFORM_RENDER_FINISHED = 0x05,
    GET_RENDERING_LOCATION_DATA = 0x06,
    GET_ANIMATION_INFO = 0x07,
    PING = 0xFF,
};

enum class ReqRepStatusCode : uint8_t
{
    SUCCESS = 0x00,
    OBJECT_NOT_FOUND = 0x01,
    INVALID_REQUEST_DATA = 0x02,
    INTERNAL_ERROR = 0xFE,
    UNKNOWN_COMMAND = 0xFF,
};

enum class PubSubMsgType : uint8_t
{
    OBJECT_POSITION_UPDATED = 0x00,
    OBJECT_RENAMED = 0x01,
    OBJECT_DELETED = 0x02,
};

} // namespace ambilink::ipc::constants
