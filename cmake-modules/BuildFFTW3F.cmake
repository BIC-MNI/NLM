# Self-contained FFTW3F (single-precision) download+build for standalone NLM builds
# (NLM_USE_SYSTEM_FFTW3F=OFF). See BuildZLIB.cmake for why this doesn't reuse the
# superbuild's own cmake-modules/BuildFFTW3F.cmake.
macro(build_fftw3f install_prefix)

  ExternalProject_Add(FFTW3F
    URL "https://fftw.org/fftw-3.3.11.tar.gz"
    URL_MD5 "40ec8d0447d03b8f01f8c90aa77bd16f"
    UPDATE_COMMAND ""
    SOURCE_DIR "${CMAKE_BINARY_DIR}/nlm-deps/FFTW3F"
    BINARY_DIR "${CMAKE_BINARY_DIR}/nlm-deps/FFTW3F-build"
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DENABLE_FLOAT:BOOL=ON
        -DDISABLE_FORTRAN:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DBUILD_TESTS:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${install_prefix}
        -DCMAKE_INSTALL_LIBDIR:PATH=lib
    INSTALL_DIR "${install_prefix}"
  )

  SET(FFTW3F_INCLUDE_DIR  "${install_prefix}/include")
  SET(FFTW3F_INCLUDE_DIRS "${install_prefix}/include")
  SET(FFTW3F_LIBRARY      "${install_prefix}/lib/libfftw3f.a")
  SET(FFTW3F_FOUND ON)

endmacro(build_fftw3f)
