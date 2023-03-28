# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                     #
#  Thanks to Anthony Alfimov for his JUCE Cmake Plugin Template,      #
#  which was used as a starting point for this file.                  #
#                                                                     #
#  https://github.com/anthonyalfimov/JUCE-CMake-Plugin-Template       #
#                                                                     #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

set(ambilink_target "ambilink")
set(ambilink_source_dir "${CMAKE_SOURCE_DIR}/src")
set(AMBILINK_INSTALL_DIR "${CMAKE_BINARY_DIR}" CACHE STRING "Directory to which the VST3 will be copied after build.")

juce_add_plugin(${ambilink_target}
    FORMATS "VST3"                              # The formats to build. Valid formats: Standalone Unity VST3 AU AUv3 AAX VST LV2.
                                                # AU and AUv3 plugins will only be enabled when building on macOS.
    PRODUCT_NAME "Ambilink"                     # The name of the final executable, which can differ from the target name. 

    # ICON_BIG                                  # ICON_* arguments specify a path to an image file to use as an icon for the Standalone.
    # ICON_SMALL
    COMPANY_NAME "Ivan Desiatov"                # The name of this target's author. The value is inherited from JUCE_COMPANY_NAME.
    COMPANY_WEBSITE "https://github.com/deivse/ambilink"                          

    PLUGIN_MANUFACTURER_CODE "Ivde"             # A four-character manufacturer id with at least one upper-case character.
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case.
    PLUGIN_CODE "Amlk"                          # A unique four-character plugin id with exactly one upper-case character.
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case.

    IS_SYNTH "FALSE"                            # Is this a synth or an effect?
    NEEDS_MIDI_INPUT "FALSE"                    # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT "FALSE"                   # Does the plugin need midi output?
    IS_MIDI_EFFECT "FALSE"                      # Is this plugin a MIDI effect?
    EDITOR_WANTS_KEYBOARD_FOCUS "TRUE"          # Does the editor need keyboard focus?

    VST3_CATEGORIES "Spatial" "Surround" "Up-Downmix"
    COPY_PLUGIN_AFTER_BUILD "FALSE")

# Add sources to target
file(GLOB_RECURSE ambilink_sources CONFIGURE_DEPENDS ${ambilink_source_dir}/*.h ${ambilink_source_dir}/*.cpp)
target_sources(${ambilink_target} PRIVATE ${ambilink_sources})

target_compile_definitions(
    ${ambilink_target} PUBLIC
    JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0) # TODO: replace with conan thing?

target_include_directories(${ambilink_target} PRIVATE ${ambilink_source_dir})

find_package(fmt REQUIRED CONFIG)
find_package(FFTW3f REQUIRED CONFIG)
find_package(OpenBLAS REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

target_link_libraries(${ambilink_target}
    PRIVATE
        juce::juce_audio_utils

        OpenBLAS::OpenBLAS
        FFTW3::fftw3f
        fmt::fmt
        glm::glm
        spdlog::spdlog
        saf
        nngpp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(${ambilink_target} PRIVATE "${CMAKE_SOURCE_DIR}/third-party/nngpp/include")

# Disable warnings from libs consumed as submodules
if (UNIX)
    target_compile_options(nngpp INTERFACE -w)
    target_compile_options(saf PRIVATE -w)
endif()
