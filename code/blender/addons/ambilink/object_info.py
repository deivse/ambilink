from typing import Callable, Dict, List, Tuple
import bpy
import mathutils
from ambilink.utils import Memoized
from ambilink.math import get_object_location_camera_space


class ObjectNotFoundError(Exception):
    """Exception used when the requested blender object can not be found."""


class ObjectDeletedError(Exception):
    """Exception used when a blender object with subscribers has been deleted."""


def raise_cb_unassigned_exc(*args, **kwargs):
    """Helper function that just raises an exception."""
    raise RuntimeError("Callback not assigned.")


class ObjectInfo:
    """Helper class to represent a single registered (with at least one VST subscriber) object."""
    AMBILINK_ID_PROP_NAME = "ambilink_id"
    _curr_ambilink_id = 0
    rename_cb: Callable[[int, str], None] = raise_cb_unassigned_exc
    delete_cb: Callable[[int], None] = raise_cb_unassigned_exc
    curr_context = None

    @classmethod
    def _get_next_id(cls) -> int:
        ObjectInfo._curr_ambilink_id += 1
        return ObjectInfo._curr_ambilink_id - 1

    def _subscribe_to_rename_updates(self):
        bpy.msgbus.subscribe_rna(
            key=self._object.path_resolve("name", False),
            args=(self,),
            notify=ObjectInfo.on_rename,
            owner=self,
        )

    def _get_reference_property(self, prop: str):
        """Get a property from the referenced blender object.
        Worst case complexity:
            O(#objects_in_scene)

        Args:
            prop (str): property string - single property name, or sequence (e.g. "location.x")

        Raises:
            ObjectDeletedError: If the referenced object has been deleted
            ValueError: the provided property name was invalid

        Returns:
            [requested property type]: prop value
        """
        try:
            return self._object.path_resolve(prop)
        except ReferenceError as ref_err:
            scene = ObjectInfo.curr_context.scene
            if (scene_obj := scene.objects.get(self.name)) is not None:
                self.reassign_object_ref(scene_obj)
                return self._object.path_resolve(prop)
            for obj in scene.objects:
                if obj.get(ObjectInfo.AMBILINK_ID_PROP_NAME) == self.ambilink_id:
                    self.reassign_object_ref(obj)
                    return self._object.path_resolve(prop)
            ObjectInfo.delete_cb(self.ambilink_id)
            raise ObjectDeletedError from ref_err

    def __init__(self, obj: bpy.types.Object):
        self._object = obj
        self.name = obj.name
        self.ambilink_id = ObjectInfo._get_next_id()
        obj[ObjectInfo.AMBILINK_ID_PROP_NAME] = self.ambilink_id
        self._subscribe_to_rename_updates()

    def unsubscribe_from_rename_updates(self):
        """Unsubscribes from rename updates"""
        bpy.msgbus.clear_by_owner(self)

    def on_rename(self):
        """Called whenever the object is renamed"""
        try:
            ObjectInfo.rename_cb(self.ambilink_id, self._get_reference_property("name"))
            self.name = self._object.name
        except ObjectDeletedError as exc:
            raise ObjectDeletedError(
                "ObjectInfo.on_rename called, but the object has been deleted."
            ) from exc

    def get_location_camera_space(self, camera: bpy.types.Camera):
        """Gets object location in camera space (Z-axis points forward).

        Args:
            camera (bpy.types.Camera): _description_
        Raises:
            ObjectDeletedError: If the referenced object has been deleted

        Returns:
            mathutils.Vector: object location camera space
        """
        return get_object_location_camera_space(
            self._get_reference_property("location"), camera
        )

    def reassign_object_ref(self, obj: bpy.types.Object):
        """Update the stored object reference"""
        self._object = obj

    @classmethod
    def try_get_ambilink_id(cls, obj: bpy.types.Object) -> bool:
        """Check if a blender object has an ambilink_id custom prop.
        Return the value, or None otherwise."""
        return obj.get(ObjectInfo.AMBILINK_ID_PROP_NAME)

    @classmethod
    def clear_ambilink_id(cls, obj: bpy.types.Object):
        """Remove the ambilink ID custom property from `obj`."""
        del obj[ObjectInfo.AMBILINK_ID_PROP_NAME]

    @classmethod
    def clear_ambilink_ids(cls):
        """Removes the ambilink_id custom prop from all objects in scene"""
        for obj in ObjectInfo.curr_context.scene.objects:
            try:
                ObjectInfo.clear_ambilink_id(obj)
            except KeyError:
                pass


class ObjectInfoManager:
    """
    Manages a list of objects that VST instances have subscribed to
    and provides data about the Blender scene.
    """

    class RegisteredObject:
        """ObjectInfo instance paired with a sub count."""

        def __init__(self, obj_info: ObjectInfo, sub_count: int = 1) -> None:
            self.obj_info = obj_info
            self.sub_count = sub_count

        def __iter__(self):
            return iter((self.obj_info, self.sub_count))

    def __init__(self, rename_cb, delete_cb, context) -> None:
        self._registered_objects: Dict[int, ObjectInfoManager.RegisteredObject] = {}
        self.set_context(context)
        ObjectInfo.rename_cb = rename_cb
        ObjectInfo.delete_cb = delete_cb
        ObjectInfo.clear_ambilink_ids()
        bpy.app.handlers.undo_post.append(self._on_undo_redo_post)
        bpy.app.handlers.redo_post.append(self._on_undo_redo_post)

    def __hash__(self) -> int:
        return id(self)

    def set_context(self, context):
        """Set the Blender context that will be used by method calls if required."""
        self._context = context
        ObjectInfo.curr_context = context

    def _on_undo_redo_post(self, scene: bpy.types.Scene, _):
        """
        Updates invalid object references, finds deleted or renamed objects.
        ObjectInfo.delete_cb is called for each deleted object.
        ObjectInfo.rename_cb is called for each renamed object.
        """

        class ObjectFound(Exception):
            """Indicates that the object has been found."""

        # Only one object can be renamed compared to prev state, so this will iterate
        # self._registered_objects once, and scene.objects at most once.
        deleted_object_ids = []
        for ambilink_id, registered_obj in self._registered_objects.items():
            try:
                if (
                    scene_obj := scene.objects.get(registered_obj.obj_info.name)
                ) is not None:
                    registered_obj.obj_info.reassign_object_ref(scene_obj)
                    raise ObjectFound
                for scene_obj in scene.objects:
                    if ObjectInfo.try_get_ambilink_id(scene_obj) == ambilink_id:
                        registered_obj.obj_info.reassign_object_ref(scene_obj)
                        registered_obj.obj_info.on_rename()
                        raise ObjectFound
                ObjectInfo.delete_cb(ambilink_id)
                deleted_object_ids.append(ambilink_id)
            except ObjectFound:
                continue
        for ambilink_id in deleted_object_ids:
            self._registered_objects.pop(ambilink_id)

    def get_object_name_list(self) -> List[str]:
        """Get a list of names of all objects in the scene"""
        return [obj.name for obj in self._context.scene.objects]

    def register_sub(self, object_name: str) -> int:
        """Adds an object to the list of object for which updates are published.
        Raises:
            ObjectNotFoundError: object with the given name could not be found.
        Returns:
            int ambilink_id of the object.
        """
        obj = self._context.scene.objects.get(object_name)
        if obj is None:
            raise ObjectNotFoundError(
                f"Object with name {object_name} not found in object list."
            )
        if (ambilink_id := ObjectInfo.try_get_ambilink_id(obj)) is not None:
            if self._registered_objects.get(ambilink_id) is None:
                ObjectInfo.clear_ambilink_id(obj)
            else:
                self._registered_objects[ambilink_id].sub_count += 1
                return ambilink_id
        # object doesn't have any subscribers yet
        obj_info = ObjectInfo(obj)
        self._registered_objects[
            obj_info.ambilink_id
        ] = ObjectInfoManager.RegisteredObject(obj_info, 1)
        self.invalidate_rendering_location_data_cache()
        return obj_info.ambilink_id

    def unregister_sub(self, ambilink_id: int) -> bool:
        """Decreases object subscriber count, if sub count reaches 0,
        the object is removed from the list of object for which updates are published.

        Returns:
            bool: true if subscribed object was found, false otherwise
        """
        try:
            self._registered_objects[ambilink_id].sub_count -= 1
            if self._registered_objects[ambilink_id].sub_count == 0:
                self._registered_objects[
                    ambilink_id
                ].obj_info.unsubscribe_from_rename_updates()
                self._registered_objects.pop(ambilink_id)
                self.invalidate_rendering_location_data_cache()
            return True
        except KeyError:
            return False

    @Memoized
    def get_rendering_location_data_for_registered_objects(
        self, start_frame: int, end_frame: int
    ) -> Dict[int, List[mathutils.Vector]]:
        """Gets the location of an object at the specified time in the animation.

        Raises:
            ObjectNotFoundError: if `ambilink_id` doesn't correspond to a registered object
            ValueError: start_frame or end_frame were invalid

        Returns:
            Dict[int, List[mathutils.Vector]]: locations for all registered objects
                                               for each frame in specified interval.
        """
        scene = self._context.scene
        scene_frame_count, _ = self.get_animation_info()
        initial_frame = scene.frame_current

        if start_frame < 0 or end_frame > scene_frame_count or end_frame < start_frame:
            raise ValueError("Invalid start or end frame")

        camera = scene.camera
        if camera is None:
            return [
                mathutils.Vector((0, 0, 0)) for _ in range(start_frame, end_frame + 1)
            ]

        retval: Dict[int, List(mathutils.Vector)] = {
            ambilink_id: [] for ambilink_id in self._registered_objects
        }

        real_start_frame = scene.frame_start + start_frame * scene.frame_step
        real_end_frame = scene.frame_start + end_frame * scene.frame_step

        for frame in range(
            real_start_frame, real_end_frame + 1, scene.frame_step
        ):
            scene.frame_set(frame)
            for ambilink_id, reg_obj in self._registered_objects.items():
                retval[ambilink_id].append(reg_obj.obj_info.get_location_camera_space(camera))

        scene.frame_set(initial_frame)
        return retval

    def invalidate_rendering_location_data_cache(self):
        """Invalidates the rendering data cache, should be called before new render is started."""
        ObjectInfoManager.get_rendering_location_data_for_registered_objects.invalidate_cache() # pylint: disable=no-member

    def get_animation_info(self) -> Tuple[int, float]:
        """Returns a tuple of (frame_count, fps) for the curr scene."""
        scene = self._context.scene
        frame_count = (scene.frame_end - scene.frame_start + 1) // scene.frame_step
        fps = scene.render.fps / scene.render.fps_base
        return (frame_count, fps)

    def get_updated_object_locations(self) -> List[Tuple[int, mathutils.Vector]]:
        """Get object locations for all registered objects"""
        camera = self._context.scene.camera
        if camera is None:
            return []

        retval = []
        deleted_object_ids = []
        for ambilink_id, [obj_info, _] in self._registered_objects.items():
            try:
                retval.append((ambilink_id, obj_info.get_location_camera_space(camera)))
            except ObjectDeletedError:
                deleted_object_ids.append(ambilink_id)
        for ambilink_id in deleted_object_ids:
            self._registered_objects.pop(ambilink_id)
        return retval


### When are objects invalidated?
#
# - Undo/Redo
# - File reload
# - Object deletion (different subcase)
#
###

### Known issues
#
# - Object Duplication results in duplicate ambilink IDs.
#   Possible solution - override all operators that can result in object duplication...
#
###
