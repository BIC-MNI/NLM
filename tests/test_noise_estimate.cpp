// Unit test for the noise_estimate algorithm (used internally by mincnlm
// for automatic sigma estimation), exercised in-process on a synthetic
// phantom -- no CLI parsing, no file I/O.
#include "test_common.h"
#include <minc_io_simple_volume.h>
#include "noise_estimate.h"

#include <cstdio>

int main()
{
  const int nx = 32, ny = 32, nz = 32;
  const size_t n = (size_t)nx * ny * nz;
  const double peak  = 1000.0;
  const double sigma = 40.0;

  minc::simple_volume<float> noisy(nx, ny, nz);
  make_synthetic_volume<float>(noisy.c_buf(), nx, ny, nz, peak);
  add_gaussian_noise<float>(noisy.c_buf(), n, sigma, 42);

  double mean_signal = 0.0;
  double estimated_sigma = minc::noise_estimate(noisy, mean_signal, /*gaussian=*/true, /*verbose=*/true);

  std::printf("noise_estimate: true sigma=%.2f, estimated sigma=%.2f, mean_signal=%.2f\n",
              sigma, estimated_sigma, mean_signal);

  bool ok = (estimated_sigma >= 0.9 * sigma) && (estimated_sigma <= 1.1 * sigma);
  if(!ok)
  {
    std::fprintf(stderr, "FAIL: estimated noise sigma is not within a reasonable range of the true value\n");
    return 1;
  }
  std::printf("PASS\n");
  return 0;
}
