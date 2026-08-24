#ifndef NLM_TEST_COMMON_H
#define NLM_TEST_COMMON_H

#include <cstddef>
#include <cmath>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

// Deterministic smooth synthetic phantom: a ramp along x plus a
// product-of-cosines "bumps" term, scaled into [0.1*peak, peak].
// Flat buffer, x-fastest layout (matches simple_volume<T>::c_buf()).
template<class T>
inline void make_synthetic_volume(T* buf, int nx, int ny, int nz, double peak)
{
  for(int z = 0; z < nz; z++)
  {
    for(int y = 0; y < ny; y++)
    {
      for(int x = 0; x < nx; x++)
      {
        double ramp  = (double)x / (nx - 1);
        double bumps = cos(2.0 * M_PI * 2.0 * x / nx) *
                        cos(2.0 * M_PI * 3.0 * y / ny) *
                        cos(2.0 * M_PI * 2.0 * z / nz);
        double v = peak * (0.55 + 0.35 * ramp + 0.25 * bumps);
        if(v < 0.0) v = 0.0;
        if(v > peak) v = peak;
        buf[(size_t)x + (size_t)y * nx + (size_t)z * nx * ny] = (T)v;
      }
    }
  }
}

template<class T>
inline void add_gaussian_noise(T* buf, size_t n, double sigma, unsigned long seed)
{
  gsl_rng* rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);
  for(size_t i = 0; i < n; i++)
    buf[i] = (T)((double)buf[i] + gsl_ran_gaussian(rng, sigma));
  gsl_rng_free(rng);
}

// Rician noise: sqrt((I+n1)^2 + n2^2), n1,n2 ~ N(0,sigma) -- the standard
// construction for MRI magnitude-image noise.
template<class T>
inline void add_rician_noise(const T* clean, T* buf, size_t n, double sigma, unsigned long seed)
{
  gsl_rng* rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);
  for(size_t i = 0; i < n; i++)
  {
    double n1 = gsl_ran_gaussian(rng, sigma);
    double n2 = gsl_ran_gaussian(rng, sigma);
    double v  = (double)clean[i];
    buf[i] = (T)sqrt((v + n1) * (v + n1) + n2 * n2);
  }
  gsl_rng_free(rng);
}

template<class T>
inline double psnr(const T* clean, const T* test, size_t n, double peak)
{
  double se = 0.0;
  for(size_t i = 0; i < n; i++)
  {
    double d = (double)clean[i] - (double)test[i];
    se += d * d;
  }
  double mse = se / (double)n;
  if(mse <= 0.0) return 1000.0; // identical, effectively infinite PSNR
  return 20.0 * log10(peak) - 10.0 * log10(mse);
}

#endif //NLM_TEST_COMMON_H
