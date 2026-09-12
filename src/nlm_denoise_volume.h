/* minc::simple_volume<T> overloads of the nlm_denoise.h entry points, for
 * callers inside NLM that already hold one.  Kept separate so that
 * nlm_denoise.h stays free of ezminc headers and a consumer such as N3 needs
 * nothing on its include path but NLM/src.
 */

#ifndef NLM_DENOISE_VOLUME_H
#define NLM_DENOISE_VOLUME_H

#include "nlm_denoise.h"

#include <minc_io_simple_volume.h>

namespace nlm
{

  //! \see nlm::denoise. `out` is resized to match `in`.
  inline double denoise(const minc::simple_volume<float> &in,
                        minc::simple_volume<float> &out,
                        const denoise_params &p,
                        const double steps[3] = NULL,
                        const minc::simple_volume<float> *hallucinate = NULL)
  {
    const int sizes[3] = { (int)in.dim(0), (int)in.dim(1), (int)in.dim(2) };
    out.resize(in.dim(0), in.dim(1), in.dim(2));
    return denoise(in.c_buf(), out.c_buf(), sizes, steps, p,
                   hallucinate ? hallucinate->c_buf() : NULL);
  }

  //! \see nlm::denoise_adaptive. `out` is resized to match `in`.
  inline void denoise_adaptive(const minc::simple_volume<double> &in,
                               minc::simple_volume<double> &out,
                               const anlm_params &p)
  {
    const int sizes[3] = { (int)in.dim(0), (int)in.dim(1), (int)in.dim(2) };
    out.resize(in.dim(0), in.dim(1), in.dim(2));
    denoise_adaptive(in.c_buf(), out.c_buf(), sizes, p);
  }

}

#endif //NLM_DENOISE_VOLUME_H
