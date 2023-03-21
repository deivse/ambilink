cmake_minimum_required(VERSION 3.16)
project(ambilink)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(cmake/configure_juce.cmake)

set(CONAN_DISABLE_CHECK_COMPILER FALSE)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(cmake/configure_saf.cmake)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/third-party/nngpp)

add_subdirectory(sandbox)
