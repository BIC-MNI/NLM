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

TARGET_LINK_LIBRARIES( mincnlm ${EZMINC_LIBRARIES} ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} ${FFTW3F_LIBRARY} ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} pthread  )

ADD_EXECUTABLE( minc_anlm
                minc_anlm.cpp
                minc_histograms.cpp minc_histograms.h
                minc_io_nifti_volume.h )
TARGET_LINK_LIBRARIES( minc_anlm ${EZMINC_LIBRARIES} ${NIFTI_LIBRARY} ${ZNZ_LIBRARY} ${ZLIB_LIBRARY} pthread  )

INSTALL(TARGETS 
        mincnlm minc_anlm
        RUNTIME DESTINATION bin )

