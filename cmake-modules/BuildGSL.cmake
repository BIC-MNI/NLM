# Self-contained GSL download+build for standalone NLM builds (NLM_USE_SYSTEM_GSL=OFF).
# See BuildZLIB.cmake for why this doesn't reuse the superbuild's own BuildGSL.cmake.
macro(build_gsl install_prefix)

  IF(CMAKE_BUILD_TYPE STREQUAL Release)
    SET(GSL_EXT_C_FLAGS   "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    SET(GSL_EXT_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
  ELSE()
    SET(GSL_EXT_C_FLAGS   "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    SET(GSL_EXT_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
  ENDIF()

  ExternalProject_Add(GSL
    URL "https://ftp.gnu.org/gnu/gsl/gsl-2.4.tar.gz"
    URL_MD5 "dba736f15404807834dc1c7b93e83b92"
    UPDATE_COMMAND ""
    SOURCE_DIR "${CMAKE_BINARY_DIR}/nlm-deps/GSL"
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure --enable-silent-rules --silent
        --prefix=${install_prefix} --libdir=${install_prefix}/lib
        --with-pic --disable-shared --enable-static
        CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
        "CPPFLAGS=${GSL_EXT_C_FLAGS}" "CXXFLAGS=${GSL_EXT_CXX_FLAGS}" "CFLAGS=${GSL_EXT_C_FLAGS}"
    BUILD_COMMAND   $(MAKE) -s V=0
    INSTALL_COMMAND $(MAKE) -s V=0 install
    INSTALL_DIR "${install_prefix}"
  )

  SET(GSL_INCLUDE_DIR   "${install_prefix}/include")
  SET(GSL_LIBRARY       "${install_prefix}/lib/libgsl.a")
  SET(GSL_CBLAS_LIBRARY "${install_prefix}/lib/libgslcblas.a")
  SET(GSL_VERSION "2.4")
  SET(GSL_FOUND ON)

endmacro(build_gsl)
