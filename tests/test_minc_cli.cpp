// End-to-end test of mincnlm's MINC I/O: writes a synthetic phantom to a .mnc,
// runs the real mincnlm binary on it, reads the result back and checks both the
// header and the denoising.  This is the only NLM test that touches files or
// runs a program -- it exists because the MINC paths in the three CLI drivers
// are compiled only when NLM_HAVE_MINC is set, and a build that quietly loses
// that define still passes every in-process test.
//
// Built only when NLM_HAVE_MINC (see CMakeLists.txt); the path to the binary
// under test comes in as argv[1] ($<TARGET_FILE:mincnlm>).
#include "test_common.h"

#include <minc_1_simple.h>
#include <minc_1_simple_rw.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
  if(argc < 2)
  {
    std::fprintf(stderr, "Usage: %s <path to mincnlm>\n", argv[0]);
    return 1;
  }
  const std::string mincnlm = argv[1];
  const char *in_file  = "test_minc_cli_in.mnc";
  const char *out_file = "test_minc_cli_out.mnc";

  const int    nx = 32, ny = 32, nz = 32;
  const size_t n  = (size_t)nx * ny * nz;
  const double peak  = 1000.0;
  const double sigma = 40.0;
  // Deliberately anisotropic and offset, so a header that is not carried
  // through from input to output shows up below.
  const double step_x = 1.0, step_y = 1.5, step_z = 2.0;
  const double start_x = -16.0, start_y = -24.0, start_z = -32.0;

  std::vector<float> clean(n), noisy(n);
  make_synthetic_volume<float>(clean.data(), nx, ny, nz, peak);
  noisy = clean;
  add_gaussian_noise<float>(noisy.data(), n, sigma, 42);

  double psnr_before = psnr<float>(clean.data(), noisy.data(), n, peak);

  try
  {
    // File order slowest-first, as real MINC files are written;
    // save_standard_volume() derives its strides from the dimension map, so the
    // x-fastest buffer lands correctly whatever order is declared here.
    minc::minc_info info;
    info.push_back(minc::dim_info(nz, start_z, step_z, minc::dim_info::DIM_Z, false));
    info.push_back(minc::dim_info(ny, start_y, step_y, minc::dim_info::DIM_Y, false));
    info.push_back(minc::dim_info(nx, start_x, step_x, minc::dim_info::DIM_X, false));

    minc::simple_volume<float> in_vol(nx, ny, nz);
    for(size_t i = 0; i < n; i++)
      in_vol.c_buf()[i] = noisy[i];

    minc::minc_1_writer wrt;
    wrt.open(in_file, info, 2, NC_FLOAT);
    minc::save_simple_volume<float>(wrt, in_vol);
    wrt.close();
  }
  catch(const minc::generic_error & err)
  {
    std::fprintf(stderr, "FAIL: writing %s: %s (%s:%d)\n",
                 in_file, err.msg(), err.file(), err.line());
    return 1;
  }

  // Explicit -sigma keeps the run deterministic and skips the power-of-two
  // padding of automatic estimation; -mt 1 is deterministic and still leaves
  // block mode on (nz >= 2*threads).
  std::string cmd = "\"" + mincnlm + "\" -sigma 40 -mt 1 -clobber " +
                    in_file + " " + out_file;
  std::printf("running: %s\n", cmd.c_str());
  int rc = std::system(cmd.c_str());
  if(rc != 0)
  {
    std::fprintf(stderr, "FAIL: mincnlm exited with status %d -- a build without "
                         "MINC support rejects .mnc input\n", rc);
    return 1;
  }

  std::vector<float> denoised(n);
  try
  {
    minc::minc_1_reader rdr;
    rdr.open(out_file);

    if(rdr.ndim(1) != nx || rdr.ndim(2) != ny || rdr.ndim(3) != nz)
    {
      std::fprintf(stderr, "FAIL: output dimensions %dx%dx%d, expected %dx%dx%d\n",
                   rdr.ndim(1), rdr.ndim(2), rdr.ndim(3), nx, ny, nz);
      return 1;
    }
    const double expect_step[3] = { step_x, step_y, step_z };
    for(int i = 0; i < 3; i++)
    {
      if(fabs(rdr.nspacing(i + 1) - expect_step[i]) > 1e-6)
      {
        std::fprintf(stderr, "FAIL: output step[%d]=%g, expected %g\n",
                     i, rdr.nspacing(i + 1), expect_step[i]);
        return 1;
      }
    }

    minc::simple_volume<float> out_vol;
    minc::load_simple_volume<float>(rdr, out_vol);
    for(size_t i = 0; i < n; i++)
      denoised[i] = out_vol.c_buf()[i];
  }
  catch(const minc::generic_error & err)
  {
    std::fprintf(stderr, "FAIL: reading %s: %s (%s:%d)\n",
                 out_file, err.msg(), err.file(), err.line());
    return 1;
  }

  double psnr_after = psnr<float>(clean.data(), denoised.data(), n, peak);

  std::printf("mincnlm MINC round trip: PSNR before=%.2f dB, after=%.2f dB (gain=%.2f dB)\n",
              psnr_before, psnr_after, psnr_after - psnr_before);

  // Same bar as the in-process tests; the MINC round trip is float, so only the
  // filtering matters here.
  bool ok = (psnr_after >= psnr_before + 3.0) && (psnr_after >= 28.0);
  if(!ok)
  {
    std::fprintf(stderr, "FAIL: denoising through MINC files did not improve PSNR "
                         "as expected\n");
    return 1;
  }

  std::remove(in_file);
  std::remove(out_file);
  std::printf("PASS\n");
  return 0;
}
