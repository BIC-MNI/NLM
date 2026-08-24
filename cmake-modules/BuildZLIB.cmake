# Self-contained ZLIB download+build for standalone NLM builds (NLM_USE_SYSTEM_ZLIB=OFF).
#
# Deliberately independent of the superbuild's own cmake-modules/BuildZLIB.cmake:
# no MT_RPATH_ORIGIN/LIB_SUFFIX/staging-prefix concepts, no GET_PACKAGE/MT_PACKAGES_PATH
# indirection. Installs a private static copy entirely inside the build tree
# (install_prefix), never CMAKE_INSTALL_PREFIX -- only the NLM executables are ever
# installed there (NLM/src/CMakeLists.txt's INSTALL(TARGETS ...)).
#
# Uses zlib-ng in ZLIB_COMPAT mode (drop-in libz/zlib.h ABI), same source/version as
# the superbuild's own BuildZLIB.cmake, for consistency.
macro(build_zlib install_prefix)

  ExternalProject_Add(ZLIB
    URL "https://github.com/zlib-ng/zlib-ng/archive/refs/tags/2.3.3.tar.gz"
    URL_MD5 "72337e6a7d2662af50a4ed0274c61b7e"
    UPDATE_COMMAND ""
    SOURCE_DIR "${CMAKE_BINARY_DIR}/nlm-deps/ZLIB"
    BINARY_DIR "${CMAKE_BINARY_DIR}/nlm-deps/ZLIB-build"
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DZLIB_COMPAT:BOOL=ON
        -DWITH_GZFILEOP:BOOL=ON
        -DBUILD_TESTING:BOOL=OFF
        -DZLIB_ENABLE_TESTS:BOOL=OFF
        -DWITH_GTEST:BOOL=OFF
        -DWITH_BENCHMARKS:BOOL=OFF
        -DWITH_FUZZERS:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${install_prefix}
        -DCMAKE_INSTALL_LIBDIR:PATH=lib
    INSTALL_DIR "${install_prefix}"
  )

  SET(ZLIB_INCLUDE_DIR "${install_prefix}/include")
  SET(ZLIB_LIBRARY     "${install_prefix}/lib/libz.a")
  SET(ZLIB_FOUND ON)

endmacro(build_zlib)
