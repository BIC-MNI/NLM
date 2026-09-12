IF(NLM_HAVE_MINC)
  ADD_DEFINITIONS(-DNLM_HAVE_MINC)
ENDIF(NLM_HAVE_MINC)

# The algorithms, with no image I/O of any kind: pthread, GSL, FFTW3F and the
# header-only minc::simple_volume container, nothing more.  MINC/NIfTI reading and
# writing belongs to the three CLI drivers below, which is what lets an unrelated
# C++ tool (N3's nu_correct_cxx, say) link this without inheriting a file format.
ADD_LIBRARY( nlm
             nl_means.cpp         nl_means.h
             nl_means_block.cpp   nl_means_block.h
             nl_means_utils.cpp   nl_means_utils.h
             anlm_proc.cpp        anlm_proc.h
             noise_estimate.cpp   noise_estimate.h
             minc_histograms.cpp  minc_histograms.h
             fftw_blur.cpp        fftw_blur.h
             dwt.cpp
             dwt_utils.cpp        dwt_utils.h  volume_dwt.h
             nlm_globals.cpp      nlm_globals.h
             nlm_denoise.cpp      nlm_denoise.h nlm_denoise_volume.h )

# PUBLIC: a consumer that includes nlm_denoise.h needs neither of these, but one
# that includes nlm_denoise_volume.h or noise_estimate.h needs simple_volume.
TARGET_INCLUDE_DIRECTORIES( nlm PUBLIC
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${NLM_EZMINC_INCLUDE_DIR} )

TARGET_LINK_LIBRARIES( nlm PUBLIC
                       ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} ${FFTW3F_LIBRARY} pthread )

# A static nlm may still end up inside a shared consumer.
IF(NOT BUILD_SHARED_LIBS)
  SET_TARGET_PROPERTIES( nlm PROPERTIES POSITION_INDEPENDENT_CODE ON )
ENDIF()

ADD_EXECUTABLE( mincnlm
                mincnlm.cpp
                minc_io_nifti_volume.h )

TARGET_LINK_LIBRARIES( mincnlm nlm ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} )
IF(NLM_HAVE_MINC)
  TARGET_LINK_LIBRARIES( mincnlm ${EZMINC_LIBRARIES} )
ENDIF(NLM_HAVE_MINC)

ADD_EXECUTABLE( minc_anlm
                minc_anlm.cpp
                minc_io_nifti_volume.h )
TARGET_LINK_LIBRARIES( minc_anlm nlm ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} )
IF(NLM_HAVE_MINC)
  TARGET_LINK_LIBRARIES( minc_anlm ${EZMINC_LIBRARIES} )
ENDIF(NLM_HAVE_MINC)

ADD_EXECUTABLE( noise_estimate
                noise_estimate_tool.cpp
                minc_io_nifti_volume.h )
TARGET_LINK_LIBRARIES( noise_estimate nlm ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} )
IF(NLM_HAVE_MINC)
  TARGET_LINK_LIBRARIES( noise_estimate ${EZMINC_LIBRARIES} )
ENDIF(NLM_HAVE_MINC)

# In-build target only: no INSTALL() of nlm or its headers, no NLMConfig.cmake.
# In-tree consumers link the target directly; only the tools are installed.
INSTALL(TARGETS
        mincnlm minc_anlm noise_estimate
        RUNTIME DESTINATION bin )

# When NLM built its own copies of these libraries (standalone,
# NLM_USE_SYSTEM_*=OFF, the default), they're ExternalProject targets that build at
# *build time* -- order the consumers after them explicitly. Inside the superbuild
# GSL/FFTW3F/NIFTI/ZLIB are already-resolved path variables, not targets by these
# names in this scope, so this must not run there.
IF(NOT MINC_TOOLKIT_BUILD)
  IF(NOT NLM_USE_SYSTEM_ZLIB)
    ADD_DEPENDENCIES(mincnlm ZLIB)
    ADD_DEPENDENCIES(minc_anlm ZLIB)
    ADD_DEPENDENCIES(noise_estimate ZLIB)
  ENDIF()
  # GSL/FFTW3F are compiled into nlm itself, so the ordering belongs on the library;
  # the executables inherit it through their link to nlm.
  IF(NOT NLM_USE_SYSTEM_GSL)
    ADD_DEPENDENCIES(nlm GSL)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_FFTW3F)
    ADD_DEPENDENCIES(nlm FFTW3F)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_NIFTI)
    ADD_DEPENDENCIES(mincnlm NIFTI)
    ADD_DEPENDENCIES(minc_anlm NIFTI)
    ADD_DEPENDENCIES(noise_estimate NIFTI)
  ENDIF()
ENDIF()
