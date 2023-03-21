# IPC Protocol
Two protocols from the NNG library are in use - `req/rep` and `pub/sub`, `ipc` is the transport.

- `req/rep` used for all the communication initiated by the VST instance - e.g. getting list of objects in scene, subscribing/unsubscribing to an object, etc.

- `pub/sub` is used once a VST instance establishes an object subscription. The blender plugin sends updates for each object with at least one
subscriber to the `pub` socket, each VST instance filters out messages that don't refer to it's active object.

The following socket addresses in the NNG format are used for the `pub/sub` and `req/rep` sockets respectively:
- `ipc:///tmp/ambilink_pubsub`
- `ipc:///tmp/ambilink_reqrep`.

# REQ/REP messages

- `0x01` OBJ_LIST - Used to get the object list to display in VST GUI.
- `0x02` OBJ_SUB - Used to (re)subscribe to object updates.
- `0x03` OBJ_UNSUB - Used to indicate unsubscription if object selection in VST has changed, the VST is being closed, etc.
- `0x04` PREPARE_TO_RENDER - Requests the blender plugin to stop sending location updates without unsubscribing.
- `0x05` INFORM_RENDER_FINISHED - Requests the blender plugin to resume sending location updates after a PREPARE_TO_RENDER command has been received.
- `0x06` GET_RENDERING_LOCATION_DATA - Requests a "vector" of camera space locations for the specified frame interval.
- `0x07` GET_ANIMATION_INFO - Requests the animation length in frames and the fps.
- `0xFF` PING - Check blender plugin is still alive.

## Common Request Structure
[ **1 byte** | `command_id` ] [ **X bytes** | *Request Data* ]
## Common Reply Structure
[ **1 byte** | `status` ] [ **X bytes** | *Reply Data* ]

## Reply status codes:
- `0x00` SUCCESS
- `0x01` OBJECT_NOT_FOUND
- `0x02` INVALID_REQUEST_DATA
- `0xFE` INTERNAL_ERROR
- `0xFF` UNKNOWN_COMMAND

*Some status codes are command-specific.

## OBJ_LIST
### Request
[ **1 byte** | `command_id` ]
### Reply
[ **1 byte** | `status`]

[[ **1 byte** | `obj_name_length` ] [ `obj_name_length` bytes | `obj_name_utf8[]` ]]

... (for each object in scene)

[[ **1 byte** | `obj_name_length` ] [ `obj_name_length` bytes | `obj_name_utf8[]` ]]


## OBJ_SUB
### Request
[ **1 byte** | `command_id` ] [ **1 byte** | `obj_name_length` ]
[ `obj_name_length` bytes | `obj_name_utf8[]` ]
### Reply

[ **1 byte** | `status`] [...]

If `status` == `SUCCESS` then 

[ **2 bytes** | `ambilink_id`] 

follows the `status`, otherwise the message ends.


## OBJ_UNSUB
### Request
[ **1 byte** | `command_id` ] [ **2 bytes** | `ambilink_id`]
### Reply

[ **1 byte** | `status`] 

## PREPARE_TO_RENDER
### Request
[ **1 byte** | `command_id` ]
### Reply
[ **1 byte** | `status`] 

## INFORM_RENDER_FINISHED
### Request
[ **1 byte** | `command_id` ]
### Reply
[ **1 byte** | `status`] 

## GET_RENDERING_LOCATION_DATA
### Request
[ **1 byte** | `command_id` ] [ **2 bytes** | `ambilink_id`] [ `size_t`(8 bytes) | `start_frame`] [ `size_t`(8 bytes) | `end_frame`]
### Reply
[ **1 byte** | `status`] [ ... ]

If `status` == `SUCCESS` then 

[ [ `float`(4 bytes) * 3 | `camera_space_location` ] **x** `end_frame - start_frame + 1` ]

follows the data, otherwise the message ends.

## GET_ANIMATION_INFO
### Request
[ **1 byte** | `command_id` ]

### Reply
[ **1 byte** | `status`] [ ... ]

If `status` == `SUCCESS` then 

[ `size_t`(8 bytes) | `frame_count` ] [ `float`(4 bytes) | `fps` ] 

follows the data, otherwise the message ends.

# PUB/SUB messages
- `0x00` OBJ_POSITION_UPDATED
- `0x01` OBJ_RENAMED
- `0x02` OBJ_DELETED - When object is deleted, or obj. creation is UNDOne

## Common message structure

All messages are prefixed with the Ambilink ID of the blender object they relate to. (The same ID sent in reply to an **OBJ_SUB** request)

[ **2 bytes** | `ambilink_id` ] [ **1 byte** | `msg_type` ] [ **X bytes** | *Msg Data* ]

## OBJ_POSITION_UPDATED

[ **2 bytes** | `ambilink_id` ] [ **1 byte** | `msg_type` ] [ [ `float`(4 bytes) * 3 | `camera_space_location` ]

## OBJ_RENAMED

[ **2 bytes** | `ambilink_id` ] [ **1 byte** | `msg_type` ] [ **1 byte** | `new_obj_name_length` ] [ `obj_name_length` bytes | `new_obj_name_utf8[]` ]

## OBJ_DELETED

[ **2 bytes** | `ambilink_id` ] [ **1 byte** | `msg_type` ]
