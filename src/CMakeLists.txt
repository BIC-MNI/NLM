IF(HAVE_MINC1)
  ADD_DEFINITIONS(-DHAVE_MINC1)
ENDIF(HAVE_MINC1)

ADD_EXECUTABLE( mincnlm
                mincnlm.cpp
                nl_means.cpp
                nl_means_block.cpp
                nl_means_utils.cpp
                nl_means.h
                nl_means_block.h
                nl_means_utils.h
                noise_estimate.cpp   noise_estimate.h
                minc_histograms.cpp  minc_histograms.h
                fftw_blur.cpp        fftw_blur.h
                dwt.cpp
                dwt_utils.cpp        dwt_utils.h  volume_dwt.h
                minc_io_nifti_volume.h )

TARGET_LINK_LIBRARIES( mincnlm ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} ${FFTW3F_LIBRARY} ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} pthread  )
IF(HAVE_MINC1)
  TARGET_LINK_LIBRARIES( mincnlm ${EZMINC_LIBRARIES} )
ENDIF(HAVE_MINC1)

ADD_EXECUTABLE( minc_anlm
                minc_anlm.cpp
                anlm_proc.cpp        anlm_proc.h
                minc_histograms.cpp minc_histograms.h
                minc_io_nifti_volume.h )
TARGET_LINK_LIBRARIES( minc_anlm ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} pthread  )
IF(HAVE_MINC1)
  TARGET_LINK_LIBRARIES( minc_anlm ${EZMINC_LIBRARIES} )
ENDIF(HAVE_MINC1)

ADD_EXECUTABLE( noise_estimate
                noise_estimate_tool.cpp
                noise_estimate.cpp   noise_estimate.h
                minc_histograms.cpp  minc_histograms.h
                fftw_blur.cpp        fftw_blur.h
                dwt.cpp
                dwt_utils.cpp        dwt_utils.h  volume_dwt.h
                minc_io_nifti_volume.h )
TARGET_LINK_LIBRARIES( noise_estimate ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} ${FFTW3F_LIBRARY} ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} )
IF(HAVE_MINC1)
  TARGET_LINK_LIBRARIES( noise_estimate ${EZMINC_LIBRARIES} )
ENDIF(HAVE_MINC1)

INSTALL(TARGETS
        mincnlm minc_anlm noise_estimate
        RUNTIME DESTINATION bin )

# When NLM built its own copies of these libraries (standalone,
# NLM_USE_SYSTEM_*=OFF, the default), they're ExternalProject targets that build at
# *build time* -- order the executables after them explicitly. Inside the superbuild
# GSL/FFTW3F/NIFTI/ZLIB are already-resolved path variables, not targets by these
# names in this scope, so this must not run there.
IF(NOT MINC_TOOLKIT_BUILD)
  IF(NOT NLM_USE_SYSTEM_ZLIB)
    ADD_DEPENDENCIES(mincnlm ZLIB)
    ADD_DEPENDENCIES(minc_anlm ZLIB)
    ADD_DEPENDENCIES(noise_estimate ZLIB)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_GSL)
    ADD_DEPENDENCIES(mincnlm GSL)
    ADD_DEPENDENCIES(noise_estimate GSL)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_FFTW3F)
    ADD_DEPENDENCIES(mincnlm FFTW3F)
    ADD_DEPENDENCIES(noise_estimate FFTW3F)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_NIFTI)
    ADD_DEPENDENCIES(mincnlm NIFTI)
    ADD_DEPENDENCIES(minc_anlm NIFTI)
    ADD_DEPENDENCIES(noise_estimate NIFTI)
  ENDIF()
ENDIF()

