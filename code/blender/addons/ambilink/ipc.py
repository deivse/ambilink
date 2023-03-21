import logging
import sys
import struct
from io import BytesIO
from enum import IntEnum
from queue import Queue
import time
from typing import Tuple
import mathutils
import bpy
from ambilink.object_info import ObjectInfoManager, ObjectNotFoundError

OBJECT_ID_LENGTH_BYTES = 2
BYTE_ORDER = sys.byteorder
DEPENDENCIES_INSTALLED = False

# pynng may not be installed when blender launches
# so this is allowed to fail on initial module load
# after dependencies are installed, ambilink.ipc
# will be reloaded, and import will succeed.
try:
    import pynng
    from pynng import Rep0, Pub0
    logging.debug("pynng import success")
except ModuleNotFoundError:
    logging.debug("pynng import failed")


class PubSubMsgType(IntEnum):
    OBJ_POSITION_UPDATED = 0x00
    OBJ_RENAMED = 0x01
    OBJ_DELETED = 0x02


class ReqRepCommand(IntEnum):
    OBJ_LIST = 0x01
    OBJ_SUB = 0x02
    OBJ_UNSUB = 0x03
    PREPARE_TO_RENDER = 0x04
    INFORM_RENDER_FINISHED = 0x05
    GET_RENDERING_LOCATION_DATA = 0x06
    GET_ANIMATION_INFO = 0x07
    PING = 0xFF


class ReqRepStatusCode(IntEnum):
    SUCCESS = 0x00
    OBJECT_NOT_FOUND = 0x01
    INVALID_REQUEST_DATA = 0x02
    INTERNAL_ERROR = 0xFE
    UNKNOWN_COMMAND = 0xFF


def encode_pubsub_msg(
    ambilink_id: int, msg_type_byte: PubSubMsgType, msg=None
) -> bytes:
    """Encode message to publish via the PUB/SUB part of the protocol."""
    data = msg_type_byte.value.to_bytes(1, BYTE_ORDER)
    if isinstance(msg, bytes):
        data += msg
    elif isinstance(msg, str):
        data += msg.encode("utf8")
    elif msg is not None:
        raise TypeError(
            f"msg argument must be of type bytes or str (got {type(msg)})")

    return ambilink_id.to_bytes(OBJECT_ID_LENGTH_BYTES, BYTE_ORDER) + data


def encode_reqrep_reply(
    status_code: ReqRepStatusCode, reply_data: bytes = bytes()
) -> bytes:
    """Encode reply to a request for the REQ/REP part of the protocol."""
    return status_code.to_bytes(1, BYTE_ORDER) + reply_data


def encode_object_name(name: str) -> bytes:
    """Encode object name as [1 byte|`obj_name_length`] [`obj_name_length` bytes|`obj_name[]`]"""
    encoded_name = name.encode("utf8")
    # Max Blender object name length is 63 chars, so 1 byte is enough
    # even if all symbols take up 4 bytes in utf8 (max byte count per code point).
    return len(encoded_name).to_bytes(1, BYTE_ORDER) + encoded_name


def decode_object_name(request_data: BytesIO) -> str:
    """Decode object name encoded as
    [1 byte|`obj_name_length`] [`obj_name_length` bytes|`obj_name[]`]
    from request data BytesIO
    """
    length = int.from_bytes(request_data.read(1), BYTE_ORDER)
    return request_data.read(length).decode("utf8")


def decode_ambilink_id(request_data: BytesIO) -> int:
    """Decode ambilink id - 2 byte unsigned int"""
    return int.from_bytes(request_data.read(2), BYTE_ORDER)


def encode_location(loc: mathutils.Vector):
    """Encode object location as 3 4-byte (standard size) floats"""
    return struct.pack("=fff", loc.x, loc.y, loc.z)


class IPCServer:
    """Handles communication with the VST instances."""

    REQREP_ADDRESS = "ipc:///tmp/ambilink_reqrep"
    PUBSUB_ADDRESS = "ipc:///tmp/ambilink_pubsub"
    DEFAULT_TICKRATE_HZ = 30

    def __init__(self, context) -> None:
        self._rep_sock: Rep0 = Rep0(listen=IPCServer.REQREP_ADDRESS)
        self._pub_sock: Pub0 = Pub0(listen=IPCServer.PUBSUB_ADDRESS)
        self._msg_queue: Queue = Queue()
        self._rendering = False
        self._obj_info_manager = ObjectInfoManager(
            rename_cb=self._queue_rename_msg, delete_cb=self._queue_delete_msg,
            context=context
        )

    def close_sockets(self):
        """Close sockets. Should be called before stopping the server."""
        self._rep_sock.close()
        self._pub_sock.close()

    def serve(self, context):
        """Scheduled via blender window manager event timers.
        Replies to request and publishes updates."""

        self._obj_info_manager.set_context(context)
        self._reply()
        if not self._rendering:
            pos_list = self._obj_info_manager.get_updated_object_locations()
            for id_pos in pos_list:
                self._queue_location_update_msg(id_pos)
        self._publish()

    def _queue_rename_msg(self, ambilink_id: int, new_name: str):
        """Put a rename msg into the message queue.

        Args:
            ambilink_id (int): ambilink-specific object id.
            new_name (str): new object name to display in VST UI.
        """
        self._msg_queue.put(
            encode_pubsub_msg(
                ambilink_id, PubSubMsgType.OBJ_RENAMED, encode_object_name(
                    new_name)
            )
        )

    def _queue_delete_msg(self, ambilink_id: int):
        """Add a delete msg to the message queue.

        Args:
            ambilink_id (int): ambilink-specific object id.
        """
        self._msg_queue.put(encode_pubsub_msg(
            ambilink_id, PubSubMsgType.OBJ_DELETED))

    def _queue_location_update_msg(self, id_pos: Tuple[int, mathutils.Vector]):
        """Add an object pos update msg to the message queue.

        Args:
            ambilink_id (int): ambilink-specific object id.
            pos (mathutils.Vector): object position in camera space.
        """
        self._msg_queue.put(
            encode_pubsub_msg(
                id_pos[0],
                PubSubMsgType.OBJ_POSITION_UPDATED,
                encode_location(id_pos[1]),
            )
        )

    def _reply(self):
        """Reply to all incoming requests using the Rep0 socket."""

        def match_command(command_byte: bytes, enum_value: ReqRepCommand):
            return command_byte == enum_value.to_bytes(1, BYTE_ORDER)

        try:
            while True:
                request_data = BytesIO(self._rep_sock.recv(block=False))
                command = request_data.read(1)

                logging.debug("received command %s", ReqRepCommand(
                    int.from_bytes(command, byteorder=BYTE_ORDER, signed=False)))

                if match_command(command, ReqRepCommand.OBJ_LIST):
                    self._rep_sock.send(self._process_object_list_request())
                elif match_command(command, ReqRepCommand.OBJ_SUB):
                    self._rep_sock.send(
                        self._process_object_sub_request(request_data))
                elif match_command(command, ReqRepCommand.OBJ_UNSUB):
                    self._rep_sock.send(
                        self._process_object_unsub_request(request_data)
                    )
                elif match_command(command, ReqRepCommand.GET_ANIMATION_INFO):
                    self._rep_sock.send(self._process_animation_info_request())
                elif match_command(command, ReqRepCommand.PREPARE_TO_RENDER):
                    self._rep_sock.send(
                        self._process_prepare_to_render_request())
                elif match_command(command, ReqRepCommand.INFORM_RENDER_FINISHED):
                    self._rep_sock.send(
                        self._process_inform_render_finished_request())
                elif match_command(command, ReqRepCommand.GET_RENDERING_LOCATION_DATA):
                    self._rep_sock.send(
                        self._process_rendering_location_data_request(
                            request_data)
                    )
                elif match_command(command, ReqRepCommand.PING):
                    self._rep_sock.send(
                        encode_reqrep_reply(ReqRepStatusCode.SUCCESS))
                else:
                    self._rep_sock.send(
                        encode_reqrep_reply(ReqRepStatusCode.UNKNOWN_COMMAND)
                    )
        except pynng.TryAgain:
            pass

    def _process_animation_info_request(self):
        frame_count, fps = self._obj_info_manager.get_animation_info()
        return encode_reqrep_reply(
            ReqRepStatusCode.SUCCESS, struct.pack("=Qf", frame_count, fps)
        )

    def _process_rendering_location_data_request(self, request_data: BytesIO):
        ambilink_id = decode_ambilink_id(request_data)
        start_frame = int.from_bytes(
            request_data.read(8), BYTE_ORDER, signed=False)
        end_frame = int.from_bytes(
            request_data.read(8), BYTE_ORDER, signed=False)

        locs_per_object = (
            ObjectInfoManager.get_rendering_location_data_for_registered_objects(
                self._obj_info_manager, start_frame, end_frame
            )
        )

        if ambilink_id not in locs_per_object:
            return encode_reqrep_reply(ReqRepStatusCode.OBJECT_NOT_FOUND)

        reply_data = BytesIO()
        for location in locs_per_object[ambilink_id]:
            reply_data.write(encode_location(location))

        return encode_reqrep_reply(ReqRepStatusCode.SUCCESS, reply_data.getvalue())

    def _process_prepare_to_render_request(self):
        if not self._rendering:
            # First call after previous render, reset location data cache
            self._obj_info_manager.invalidate_rendering_location_data_cache()
            self._rendering = True
        return encode_reqrep_reply(ReqRepStatusCode.SUCCESS)

    def _process_inform_render_finished_request(self):
        self._rendering = False
        return encode_reqrep_reply(ReqRepStatusCode.SUCCESS)

    def _process_object_sub_request(self, request_data: BytesIO) -> bytes:
        name = decode_object_name(request_data)
        try:
            ambilink_id = self._obj_info_manager.register_sub(name)
            return encode_reqrep_reply(
                ReqRepStatusCode.SUCCESS, ambilink_id.to_bytes(2, BYTE_ORDER)
            )
        except ObjectNotFoundError:
            return encode_reqrep_reply(ReqRepStatusCode.OBJECT_NOT_FOUND)

    def _process_object_unsub_request(self, request_data: BytesIO) -> bytes:
        ambilink_id = decode_ambilink_id(request_data)
        return encode_reqrep_reply(
            ReqRepStatusCode.SUCCESS
            if self._obj_info_manager.unregister_sub(ambilink_id)
            else ReqRepStatusCode.OBJECT_NOT_FOUND
        )

    def _process_object_list_request(self) -> bytes:
        """Build a reply to an object list request."""
        obj_list = self._obj_info_manager.get_object_name_list()
        encoded_list = bytes()
        for obj_name in obj_list:
            encoded_list += encode_object_name(obj_name)

        return encode_reqrep_reply(ReqRepStatusCode.SUCCESS, encoded_list)

    def _publish(self):
        """Publish all queued messages using the Pub0 socket."""
        while not self._msg_queue.empty():
            msg = self._msg_queue.get_nowait()
            self._pub_sock.send(msg)


class StartServerOp(bpy.types.Operator):
    """Starts"""

    bl_idname = "ambilink.start_server"
    bl_label = "Start Server For Ambilink VST."
    bl_description = (
        "Starts the server that Ambilink VST plugins use to retrieve object locations."
    )
    bl_options = {"REGISTER"}

    _timer = None
    _ipc_bridge = None
    _error_last_shown = None
    should_stop = False
    is_running = False

    def _show_missing_camera_error(self):
        if (self._error_last_shown is None) or (
            time.time() - self._error_last_shown >= 5
        ):
            self._error_last_shown = time.time()
            self.report(
                {"ERROR"},
                "Ambilink Server Error: The scene must have an active camera.",
            )

    def modal(self, context, event):
        if self.should_stop:
            self.cancel(context)
            return {"CANCELLED"}

        if event.type == "TIMER":
            if context.scene.camera is None:
                self._show_missing_camera_error()
            self._ipc_bridge.serve(context)

        return {"PASS_THROUGH"}

    @classmethod
    def poll(cls, context):
        return not StartServerOp.is_running

    def execute(self, context):
        if not DEPENDENCIES_INSTALLED:
            self.report(
                {"ERROR"}, "Please install dependencies in add-on settings.")
            return {"CANCELLED"}

        tickrate = IPCServer.DEFAULT_TICKRATE_HZ
        try:
            tickrate = context.preferences.addons['ambilink'].preferences['tickrate']
            logging.info(
                "StartServerOp: using tickrate from preferences: %d Hz", tickrate)
        except KeyError:
            logging.info(
                "StartServerOp: using default tickrate (%d Hz)", tickrate)

        self._timer = context.window_manager.event_timer_add(
            1.0 / tickrate, window=context.window
        )
        self._ipc_bridge = IPCServer(context)
        StartServerOp.is_running = True
        context.window_manager.modal_handler_add(self)
        self.report({"INFO"}, "Started Ambilink server.")
        return {"RUNNING_MODAL"}

    def cancel(self, context):
        context.window_manager.event_timer_remove(self._timer)
        self._ipc_bridge.close_sockets()
        self._ipc_bridge = None
        StartServerOp.is_running = False
        StartServerOp.should_stop = False
        self.report({"INFO"}, "Stopped Ambilink server.")


class StopServerOp(bpy.types.Operator):
    bl_idname = "ambilink.stop_server"
    bl_label = "Stop Server For Ambilink VST."
    bl_description = (
        "Do not stop while rendering audio! Stops the server for Ambilink VST"
    )
    bl_options = {"REGISTER"}


    @classmethod
    def poll(cls, context):
        return bool(StartServerOp.is_running)

    def execute(self, context):
        StartServerOp.should_stop = True
        return {"FINISHED"}

    def invoke(self, context, event):
        return context.window_manager.invoke_confirm(self, event)


SERVER_OPERATORS = [StartServerOp, StopServerOp]

def server_menu_draw(self, context):
    """
    Draw function to add Ambilink operators to Blender's File > Export menu.
    """
    for op in SERVER_OPERATORS:
        if op.poll(context):
            self.layout.operator(op.bl_idname)

def register():
    for cls in SERVER_OPERATORS:
        bpy.utils.register_class(cls)
    bpy.types.TOPBAR_MT_file_export.append(server_menu_draw)


def unregister():
    for cls in SERVER_OPERATORS:
        bpy.utils.unregister_class(cls)
    bpy.types.TOPBAR_MT_file_export.remove(server_menu_draw)
