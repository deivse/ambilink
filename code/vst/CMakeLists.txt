cmake_minimum_required(VERSION 3.25)
project(ambilink VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(NOT UNIX)
    message(FATAL_ERROR "Only Linux is currently supported by the build system.")
endif()

include(cmake/configure_juce.cmake)

set(CONAN_DISABLE_CHECK_COMPILER FALSE)

include(cmake/configure_saf.cmake)

add_subdirectory(${CMAKE_SOURCE_DIR}/third-party/nngpp)

# adds and configures the actual plugin target
include(cmake/ambilink.cmake)
