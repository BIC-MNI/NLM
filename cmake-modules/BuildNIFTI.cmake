# Self-contained NIFTI (nifti_clib) download+build for standalone NLM builds
# (NLM_USE_SYSTEM_NIFTI=OFF). See BuildZLIB.cmake for why this doesn't reuse the
# superbuild's own cmake-modules/BuildNIFTI.cmake -- notably, that one builds a
# *symbol-mangled* copy (minc_nifti_* prefixes, see libminc/cmake-modules/nifti_mangle.h)
# so it can coexist with ITK's own bundled niftiio inside the superbuild. NLM has no such
# conflict standalone, so this builds plain, unmangled nifti_clib/znzlib symbols, which is
# what NLM's own sources (minc_io_nifti_volume.h et al.) call via unmangled headers.
#
# Requires ZLIB_INCLUDE_DIR/ZLIB_LIBRARY to already be set (system or NLM-built) before
# this macro is invoked -- see NLM/CMakeLists.txt, which resolves ZLIB first.
macro(build_nifti install_prefix)

  # Same pin as the superbuild's own BuildNIFTI.cmake -- the ITK-maintained nifti_clib
  # fork (upstream NIFTI-Imaging/nifti_clib has been dormant since v3.0.0/2020).
  SET(NLM_NIFTI_GIT_SHA f24a607843c1fe4726ad774d0476bdf36bc11f2a)

  ExternalProject_Add(NIFTI
    URL "https://github.com/InsightSoftwareConsortium/nifti_clib/archive/${NLM_NIFTI_GIT_SHA}.tar.gz"
    URL_HASH SHA256=1829700c16ac0679487d9b7eb1d8d63a5d8a045e990d4b37b38f4760499aea16
    UPDATE_COMMAND ""
    SOURCE_DIR "${CMAKE_BINARY_DIR}/nlm-deps/NIFTI"
    BINARY_DIR "${CMAKE_BINARY_DIR}/nlm-deps/NIFTI-build"
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${install_prefix}
        -DCMAKE_INSTALL_LIBDIR:PATH=lib
        -DNIFTI_INSTALL_LIBRARY_DIR:PATH=lib
        -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
        -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}
        -DGIT_REPO_VERSION:STRING=3.0.0
        -DNIFTI_BUILD_APPLICATIONS:BOOL=OFF
        -DNIFTI_BUILD_TESTING:BOOL=OFF
        -DBUILD_TESTING:BOOL=OFF
        -DUSE_NIFTI2_CODE:BOOL=OFF
        -DUSE_NIFTICDF_CODE:BOOL=OFF
        -DNIFTI_INSTALL_NO_DOCS:BOOL=ON
    INSTALL_DIR "${install_prefix}"
  )

  # If ZLIB is itself an NLM-built ExternalProject target (NLM_USE_SYSTEM_ZLIB=OFF),
  # NIFTI's own build links against it at build time -- order the two explicitly.
  IF(TARGET ZLIB)
    ADD_DEPENDENCIES(NIFTI ZLIB)
  ENDIF()

  SET(NIFTI_INCLUDE_DIR "${install_prefix}/include/nifti")
  SET(NIFTI_LIBRARY     "${install_prefix}/lib/libniftiio.a")
  SET(ZNZ_INCLUDE_DIR   "${install_prefix}/include/nifti")
  SET(ZNZ_LIBRARY       "${install_prefix}/lib/libznz.a")
  SET(NIFTI_FOUND ON)

endmacro(build_nifti)
