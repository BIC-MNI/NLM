# GSL/FFTW3F (system-found or NLM-built) are already resolved by the parent
# NLM/CMakeLists.txt before ADD_SUBDIRECTORY(tests) -- reuse those, don't re-probe
# (re-probing here previously re-ran FIND_PACKAGE unconditionally, which could pick up
# a different, unrelated system GSL/FFTW3F than what src/ actually built against, and
# fails outright when the system has no fftw3f_threads/fftw3f_omp).
#
# The tests link the `nlm` library rather than recompiling ../src/*.cpp, so they
# exercise exactly the objects a consumer gets.  nlm carries its include directories
# and its GSL/FFTW3F/pthread link as usage requirements, and the standalone
# ExternalProject ordering with them.

ADD_EXECUTABLE( test_nlm_denoise test_nlm_denoise.cpp test_common.h )
TARGET_LINK_LIBRARIES( test_nlm_denoise nlm )
ADD_TEST( nlm_denoise_psnr test_nlm_denoise )

ADD_EXECUTABLE( test_anlm_denoise test_anlm_denoise.cpp test_common.h )
TARGET_LINK_LIBRARIES( test_anlm_denoise nlm )
ADD_TEST( anlm_denoise_psnr test_anlm_denoise )

ADD_EXECUTABLE( test_noise_estimate test_noise_estimate.cpp test_common.h )
TARGET_LINK_LIBRARIES( test_noise_estimate nlm )
ADD_TEST( noise_estimate_sigma test_noise_estimate )

# The one test that runs a program and touches files: the MINC readers/writers in
# the CLI drivers are compiled only under NLM_HAVE_MINC, so a build that loses
# that define still passes everything above.  Skipped in a NIfTI-only build.
IF(NLM_HAVE_MINC)
  ADD_EXECUTABLE( test_minc_cli test_minc_cli.cpp test_common.h )
  TARGET_LINK_LIBRARIES( test_minc_cli nlm ${EZMINC_LIBRARIES} )
  ADD_TEST( NAME nlm_minc_cli COMMAND test_minc_cli $<TARGET_FILE:mincnlm> )
ENDIF(NLM_HAVE_MINC)
