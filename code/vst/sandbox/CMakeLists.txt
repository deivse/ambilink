add_executable(sandbox sandbox.cpp)

target_link_libraries(sandbox PUBLIC ${CONAN_LIBS})
target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/third-party/nngpp/include)
target_link_libraries(sandbox PRIVATE saf nngpp)