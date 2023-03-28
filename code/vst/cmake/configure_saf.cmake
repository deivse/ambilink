set(SAF_ENABLE_SOFA_READER_MODULE 1)

if(WIN32)
# TODO
elseif(APPLE)
    set(SAF_PERFORMANCE_LIB SAF_USE_APPLE_ACCELERATE)
# TODO
else()
    find_package(OpenBLAS REQUIRED CONFIG)
    find_package(FFTW3f REQUIRED CONFIG)

    set(SAF_PERFORMANCE_LIB SAF_USE_OPEN_BLAS_AND_LAPACKE)
    set(OPENBLAS_LIBRARY ${OpenBLAS_LIBRARIES})
    set(LAPACKE_LIBRARY ${OpenBLAS_LIBRARIES}) # OpenBLAS package includes LAPACKE
    set(FFTW_LIBRARY ${FFTW3f_LIBRARIES})
    set(SAF_ENABLE_SIMD True)
    set(SAF_USE_FFTW True)
endif()

set(SAF_BUILD_EXAMPLES OFF)
set(SAF_BUILD_EXTRAS OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/third-party/Spatial_Audio_Framework)
target_link_libraries(saf PRIVATE OpenBLAS::OpenBLAS FFTW3::fftw3f)

