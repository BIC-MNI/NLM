PROJECT(NLM CXX C)

CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT MINC_TOOLKIT_BUILD)
  SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")

  OPTION(NLM_USE_MINC "Build with MINC (.mnc) I/O support if available" ON)

  IF(NLM_USE_MINC)
    FIND_PACKAGE(LIBMINC QUIET)
  ENDIF(NLM_USE_MINC)

  IF(LIBMINC_FOUND AND NLM_USE_MINC)
    INCLUDE( ${LIBMINC_USE_FILE} )
    SET(HAVE_MINC1 ON)
  ELSE()
    SET(HAVE_MINC1 OFF)
    MESSAGE(STATUS "NLM: building NIfTI-only (MINC not found or disabled via NLM_USE_MINC=OFF)")
  ENDIF()

  OPTION(NLM_USE_SYSTEM_ZLIB   "Use system-installed ZLIB instead of downloading+building a static copy"   OFF)
  OPTION(NLM_USE_SYSTEM_GSL    "Use system-installed GSL instead of downloading+building a static copy"    OFF)
  OPTION(NLM_USE_SYSTEM_FFTW3F "Use system-installed FFTW3F instead of downloading+building a static copy" OFF)
  OPTION(NLM_USE_SYSTEM_NIFTI  "Use system-installed NIFTI instead of downloading+building a static copy"  OFF)

  INCLUDE(ExternalProject)

  # ZLIB is resolved first (system or built) since build_nifti() below needs
  # ZLIB_INCLUDE_DIR/ZLIB_LIBRARY already set, whichever source they came from.
  IF(NLM_USE_SYSTEM_ZLIB)
    FIND_PACKAGE(ZLIB REQUIRED)
    SET(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
  ELSE()
    INCLUDE(BuildZLIB)
    build_zlib(${CMAKE_BINARY_DIR}/nlm-deps/install)
  ENDIF()

  IF(NLM_USE_SYSTEM_GSL)
    FIND_PACKAGE(GSL REQUIRED)
  ELSE()
    INCLUDE(BuildGSL)
    build_gsl(${CMAKE_BINARY_DIR}/nlm-deps/install)
  ENDIF()

  IF(NLM_USE_SYSTEM_FFTW3F)
    FIND_PACKAGE(FFTW3F REQUIRED)
  ELSE()
    INCLUDE(BuildFFTW3F)
    build_fftw3f(${CMAKE_BINARY_DIR}/nlm-deps/install)
  ENDIF()

  IF(NLM_USE_SYSTEM_NIFTI)
    FIND_PACKAGE(NIFTI REQUIRED)
  ELSE()
    INCLUDE(BuildNIFTI)
    build_nifti(${CMAKE_BINARY_DIR}/nlm-deps/install)
  ENDIF()

  # The pure, MINC-independent volume container (minc_io_simple_volume.h et al.) is
  # needed regardless of HAVE_MINC1, but only needs to be located explicitly when
  # MINC itself isn't available -- when HAVE_MINC1 is ON, LIBMINC_USE_FILE already
  # propagated the real include dir above.
  IF(NOT HAVE_MINC1)
    IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libminc/ezminc")
      # Single source of truth when this checkout has the sibling libminc submodule
      # (the superbuild layout) -- avoid drifting from a second vendored copy.
      INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../libminc/ezminc")
    ELSE()
      # Standalone NLM checkout with no sibling libminc at all -- use NLM's own
      # vendored fallback copy of the same pure, MINC-independent headers.
      INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ezminc")
    ENDIF()
  ENDIF()

  ENABLE_TESTING()
  INCLUDE(CTest)
ENDIF(NOT MINC_TOOLKIT_BUILD)

INCLUDE_DIRECTORIES( ${GSL_INCLUDE_DIR} ${FFTW3F_INCLUDE_DIRS} ${NIFTI_INCLUDE_DIR} ${ZNZ_INCLUDE_DIR} )

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)
