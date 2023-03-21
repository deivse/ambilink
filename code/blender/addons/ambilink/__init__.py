import subprocess
import logging
import importlib
import logging

bl_info = {
    "name": "Ambilink",
    "author": "Ivan Desiatov",
    "version": (1, 0, 0),
    "blender": (3, 0, 1),
    "description": "Provides data about objects in the Blender scene to the Ambilink ambisonic panner VST.",
    "category": "Import-Export",
}

if "bpy" in locals():
    import importlib
    importlib.reload(deputils)
    importlib.reload(ipc)
    importlib.reload(utils)
else:
    from ambilink import dependency_utils as deputils
    from ambilink import ipc, utils

import bpy

logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

MODULE_DEPENDENCIES = (deputils.Dependency(module="pynng", package=None, name=None),)
ipc.DEPENDENCIES_INSTALLED = False

submodules_to_register = [ipc]


class InstallDependenciesOp(bpy.types.Operator):
    bl_idname = "ambilink.install_dependencies"
    bl_label = "Install Ambilink dependencies"
    bl_description = (
        "Downloads and installs the python modules required by Ambilink. "
        "An internet connection is required."
    )
    bl_options = {"REGISTER", "INTERNAL"}

    @classmethod
    def poll(cls, context):
        return not ipc.DEPENDENCIES_INSTALLED and not ipc.StartServerOp.is_running

    def execute(self, context):
        try:
            deputils.install_pip()
            for dependency in MODULE_DEPENDENCIES:
                deputils.install_and_import_module(
                    module_name=dependency.module,
                    package_name=dependency.package,
                    global_name=dependency.name,
                )
                logging.info("Succesfully installed dependencies.")
        except (subprocess.CalledProcessError, ImportError) as err:
            self.report({"ERROR"}, str(err))
            return {"CANCELLED"}

        importlib.reload(ipc)
        ipc.register()
        ipc.DEPENDENCIES_INSTALLED = True

        return {"FINISHED"}


class UninstallDependenciesOp(bpy.types.Operator):
    bl_idname = "ambilink.uninstall_dependencies"
    bl_label = "Uninstall Ambilink dependencies"
    bl_description = "Uninstalls the python modules required by Ambilink. "
    bl_options = {"REGISTER", "INTERNAL"}

    @classmethod
    def poll(self, context):
        # Deactivate when dependencies have been installed or user is not admin
        return ipc.DEPENDENCIES_INSTALLED and not ipc.StartServerOp.is_running

    def execute(self, context):
        try:
            for dependency in MODULE_DEPENDENCIES:
                deputils.uninstall_module(
                    module_name=dependency.module,
                    package_name=dependency.package,
                    global_name=dependency.name,
                )
            logging.info("Succesfully uninstalled dependencies.")
        except (subprocess.CalledProcessError, ImportError) as err:
            self.report({"ERROR"}, str(err))
            return {"CANCELLED"}

        ipc.DEPENDENCIES_INSTALLED = False
        return {"FINISHED"}

class AddonPreferences(bpy.types.AddonPreferences):
    bl_idname = "ambilink"

    tickrate: bpy.props.IntProperty(
        name = "Server Update Frequency (Hz)",
        default = ipc.IPCServer.DEFAULT_TICKRATE_HZ,
        min = 1,
        max = 120,
        description = ("Controls the interval at which the Ambilink Blender add-on replies"
            "to requests from the VST instances and updates the location of objects."
            "The recomended range is from 15 to 60 Hz."
            "Setting a higher value will reduce latency and increase update frequency, but is hardware-intensive;"
            "a lower value will reduce the frequency at which sound directions are updated,"
            "but may improve performance. \n"),
    )

    def draw(self, context):
        if not ipc.DEPENDENCIES_INSTALLED:
            self.layout.label(text="Please install dependencies before use!", icon="ERROR")
            self.layout.label(text="Current list of module dependencies: " + " ".join([f"'{x.module}'" for x in MODULE_DEPENDENCIES])\
                              + ". Dependencies will be installed with PIP.", icon="INFO")
        for op in [InstallDependenciesOp, ipc.StartServerOp, ipc.StopServerOp, UninstallDependenciesOp]:
            if op.poll(context):
                self.layout.operator(op.bl_idname, icon="CONSOLE")
        self.layout.separator()
        self.layout.label(text='The server needs to be restarted to apply changes.', icon="INFO")
        self.layout.prop(self, "tickrate")


PREFERENCE_CLASSES = (InstallDependenciesOp, UninstallDependenciesOp, AddonPreferences)

def register():
    ipc.DEPENDENCIES_INSTALLED = False

    for cls in PREFERENCE_CLASSES:
        bpy.utils.register_class(cls)

    try:
        for dependency in MODULE_DEPENDENCIES:
            deputils.import_module(module_name=dependency.module, global_name=dependency.name)
        importlib.reload(ipc)
        ipc.DEPENDENCIES_INSTALLED = True
    except ModuleNotFoundError:
        pass
    ipc.register()

def unregister():
    for cls in PREFERENCE_CLASSES:
        bpy.utils.unregister_class(cls)
    ipc.unregister()
