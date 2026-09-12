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
