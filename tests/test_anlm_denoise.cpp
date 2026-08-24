// Unit test for the core adaptive NL-means denoising algorithm used by
// minc_anlm (anlm_proc), exercised in-process on a synthetic phantom --
// no CLI parsing, no file I/O.
#include "test_common.h"
#include "anlm_proc.h"

#include <cstdio>
#include <vector>

int main()
{
  const int nx = 32, ny = 32, nz = 32;
  const size_t n = (size_t)nx * ny * nz;
  const double peak  = 1000.0;
  const double sigma = 40.0;

  std::vector<double> clean(n);
  make_synthetic_volume<double>(clean.data(), nx, ny, nz, peak);

  minc::simple_volume<double> noisy(nx, ny, nz);
  for(size_t i = 0; i < n; i++)
    noisy.c_buf()[i] = clean[i];
  add_gaussian_noise<double>(noisy.c_buf(), n, sigma, 42);

  double psnr_before = psnr<double>(clean.data(), noisy.c_buf(), n, peak);

  anlm_proc anlm(noisy, /*search*/2, /*patch*/1, /*rician*/false, /*beta*/1.0);
  anlm.exec(1);

  double psnr_after = psnr<double>(clean.data(), anlm.fima.c_buf(), n, peak);

  std::printf("minc_anlm core: PSNR before=%.2f dB, after=%.2f dB (gain=%.2f dB)\n",
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
