# GSL/FFTW3F (system-found or NLM-built) are already resolved by the parent
# NLM/CMakeLists.txt before ADD_SUBDIRECTORY(tests) -- reuse those, don't re-probe
# (re-probing here previously re-ran FIND_PACKAGE unconditionally, which could pick up
# a different, unrelated system GSL/FFTW3F than what src/ actually built against, and
# fails outright when the system has no fftw3f_threads/fftw3f_omp).

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../src ${GSL_INCLUDE_DIR} )

ADD_EXECUTABLE( test_nlm_denoise
                test_nlm_denoise.cpp
                test_common.h
                ../src/nl_means.cpp        ../src/nl_means.h
                ../src/nl_means_utils.cpp  ../src/nl_means_utils.h )
TARGET_LINK_LIBRARIES( test_nlm_denoise ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} pthread )
ADD_TEST( nlm_denoise_psnr test_nlm_denoise )

ADD_EXECUTABLE( test_anlm_denoise
                test_anlm_denoise.cpp
                test_common.h
                ../src/anlm_proc.cpp ../src/anlm_proc.h )
TARGET_LINK_LIBRARIES( test_anlm_denoise ${GSL_LIBRARY} pthread )
ADD_TEST( anlm_denoise_psnr test_anlm_denoise )

ADD_EXECUTABLE( test_noise_estimate
                test_noise_estimate.cpp
                test_common.h
                ../src/noise_estimate.cpp  ../src/noise_estimate.h
                ../src/minc_histograms.cpp ../src/minc_histograms.h
                ../src/fftw_blur.cpp       ../src/fftw_blur.h
                ../src/dwt.cpp
                ../src/dwt_utils.cpp       ../src/dwt_utils.h ../src/volume_dwt.h )
TARGET_LINK_LIBRARIES( test_noise_estimate ${GSL_LIBRARY} ${GSL_CBLAS_LIBRARY} ${FFTW3F_LIBRARY} pthread )
ADD_TEST( noise_estimate_sigma test_noise_estimate )

# See NLM/src/CMakeLists.txt for why this ordering is needed when NLM built its own
# GSL/FFTW3F (standalone, NLM_USE_SYSTEM_*=OFF, the default).
IF(NOT MINC_TOOLKIT_BUILD)
  IF(NOT NLM_USE_SYSTEM_GSL)
    ADD_DEPENDENCIES(test_nlm_denoise GSL)
    ADD_DEPENDENCIES(test_anlm_denoise GSL)
    ADD_DEPENDENCIES(test_noise_estimate GSL)
  ENDIF()
  IF(NOT NLM_USE_SYSTEM_FFTW3F)
    ADD_DEPENDENCIES(test_noise_estimate FFTW3F)
  ENDIF()
ENDIF()
