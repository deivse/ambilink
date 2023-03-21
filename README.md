# Ambilink
Ambilink enables the use of Blender for
controlling the direction of ambisonic panning in a DAW.
It consists of two plugins connected via IPC (using the [nng](https://github.com/nanomsg/nng) library):
- **VST3 Plugin** - performs ambisonic panning up to fifth order (ACN channel ordering, N3D/SN3D normalisation). An object from the Blender scene can be picked in the GUI, the panning direction during real-time playback is then given by the current position of the object relative to the active camera. When rendering audio to a file, the plugin's output is synchronised to the animation in Blender.
- **Blender add-on** - provides data about the Blender scene to VST instances via IPC.
In real-time mode continuously publishes updated camera space coordinates of objects with at least one subscribed VST instance, in offline rendering mode sends coordinates for each frame in the animation .

This allows to tie any instance of the VST3 ambisonic panner to any
Blender object, the position of which can be controlled by any means
available in Blender - keyframe animation, drivers, scripts, etc.

## VST Plugin

### Building

> **Warning**
> The build system doesn't currently work on Windows and macOS (see next section).

The VST is developed using C++20 and the [JUCE framework](https://github.com/juce-framework/JUCE). Some functions from [SAF](https://github.com/leomccormack/Spatial_Audio_Framework) and [GLM](https://github.com/g-truc/glm) are used.
The build system uses CMake and Projucer-generated Makefiles. (Projucer is a utility included with JUCE.)
To build and install the plugin, run the following commands:
```bash
> ./build_vst3_linux.sh # clones git submodules, installs .deb and conan packages, builds dependencies and the plugin itself.
> ./install_vst3_linux.sh CONFIG=Release # builds the vst, generates compile_commands.json, copies to ~/.vst3/
```

> **Warning**
> the install script currently only supports Debian-based systems.
 
#### Non-linux builds
The C++ source itself is multiplatform (although some minor changes might be required for compiling with MSVC or Apple-Clang).

However, the build system is not currently fully set up for anything except linux.
The [SAF](https://github.com/leomccormack/Spatial_Audio_Framework) library requires to be linked with a library compatible with the CBLAS and LAPACK standards. On Linux, OPENBLAS is used, but this part of the build system is not set up for Windows or macOS, where other libraries, providing better performance should probably be used (see [this section](https://github.com/leomccormack/Spatial_Audio_Framework/blob/master/docs/PERFORMANCE_LIBRARY_INSTRUCTIONS.md) of SAF docs). Pull requests are of course welcome.

## Blender add-on

### Installation

The Blender `code/blender/addons/ambilink` directory may either be directly copied to Blender's add-on directory, or added to a ZIP file using the `code/blender/zip_up.sh` script, and installed via Blender's add-on settings menu.

## License 
The software is licensed under [GPLv3](./LICENSE).

The [JUCE Framework](https://github.com/juce-framework/JUCE) is dual-licensed under GPLv3 and a proprietary license. The framework itself is not distributed with the project and is downloaded using CPM.

Other third-party libraries are used either as GIT submodules, or conan packages. For git submodules, the licenses can be found in each submodule's respective repository. For conan packages, the licenses are copied to `code/vst/third-party/licences` when dependencies are installed.

