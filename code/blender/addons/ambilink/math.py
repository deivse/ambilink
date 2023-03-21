import bpy
import mathutils

def get_object_location_camera_space(
    object_location: mathutils.Vector,
    camera: bpy.types.Object,
) -> mathutils.Vector:
    """Calc camera space position for an object and a camera (Z-axis points forward)"""
    view_m = camera.matrix_world.inverted()
    # TODO: camera scale messes things up

    loc = view_m @ object_location
    loc.z = (
        -loc.z
    )  # invert the Z-axis to point forward (a matter of personal preference)

    return loc
