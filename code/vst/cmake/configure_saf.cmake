set(SAF_ENABLE_SOFA_READER_MODULE 1)
if (APPLE)
    set(SAF_PERFORMANCE_LIB SAF_USE_APPLE_ACCELERATE)
    #TODO
else()
    set(SAF_PERFORMANCE_LIB SAF_USE_OPEN_BLAS_AND_LAPACKE)
    set(OPENBLAS_LIBRARY ${CONAN_LIBS_OPENBLAS})
    set(LAPACKE_LIBRARY ${CONAN_LIBS_OPENBLAS}) #OpenBLAS package includes LAPACKE
    set(FFTW_LIBRARY ${CONAN_LIBS_FFTW})
    set(SAF_ENABLE_SIMD 1)
    set(SAF_USE_FFTW 1)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/third-party/Spatial_Audio_Framework)
target_compile_options(saf PUBLIC -mavx2)
target_include_directories(saf PUBLIC ${CONAN_INCLUDE_DIRS})