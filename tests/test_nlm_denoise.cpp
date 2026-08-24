// Unit test for the core block-wise NL-means denoising algorithm used by
// mincnlm (nl_means.cpp/nl_means_utils.cpp), exercised in-process on a
// synthetic phantom -- no CLI parsing, no file I/O.
#include "test_common.h"
#include "nl_means.h"
#include "nl_means_utils.h"

#include <cstdio>
#include <vector>

// globals required by nl_means.cpp / nl_means_utils.cpp
int verbose   = 0;
int debug     = 0;
int nb_thread = 4;
int testmean  = 1;
int testvar   = 1;
int block     = 0;

int main()
{
  const int nx = 32, ny = 32, nz = 32;
  const size_t n = (size_t)nx * ny * nz;
  const double peak  = 1000.0;
  const double sigma = 40.0;

  std::vector<float> clean(n), noisy(n), denoised(n), mean_map(n, 0.0f), var_map(n, 0.0f);

  make_synthetic_volume<float>(clean.data(), nx, ny, nz, peak);
  noisy = clean;
  add_gaussian_noise<float>(noisy.data(), n, sigma, 42);

  double psnr_before = psnr<float>(clean.data(), noisy.data(), n, peak);

  int vol_size[3] = {nx, ny, nz};
  int neighborhoodsize[3] = {1, 1, 1};
  int search[3] = {3, 3, 3};
  double m_min = 0.95, v_min = 0.5;
  int weight_method = 0; // L2-norm / Gaussian noise

  Preprocessing(noisy.data(), mean_map.data(), neighborhoodsize, vol_size);
  Preprocessing2(noisy.data(), mean_map.data(), var_map.data(), neighborhoodsize, vol_size);

  denoise_mt(noisy.data(), denoised.data(), mean_map.data(), var_map.data(),
             /*filtering_param=*/sigma, /*beta=*/1.0,
             neighborhoodsize, search,
             testmean, testvar, m_min, v_min, weight_method,
             vol_size, /*hallucinate=*/nullptr);

  double psnr_after = psnr<float>(clean.data(), denoised.data(), n, peak);

  std::printf("mincnlm core: PSNR before=%.2f dB, after=%.2f dB (gain=%.2f dB)\n",
              psnr_before, psnr_after, psnr_after - psnr_before);

  bool ok = (psnr_after >= psnr_before + 3.0) && (psnr_after >= 28.0);
  if(!ok)
  {
    std::fprintf(stderr, "FAIL: denoising did not improve PSNR as expected\n");
    return 1;
  }
  std::printf("PASS\n");
  return 0;
}
