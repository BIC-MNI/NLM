/* ----------------------------- MNI Header -----------------------------------
@NAME       : nlm_denoise.h
@DESCRIPTION: Library entry points for the NLM denoisers.

              These are the whole pipeline -- parameter derivation, the local
              mean/variance preprocessing, automatic noise estimation and the
              multithreaded denoise itself -- behind one call, so a caller does
              not have to reproduce the sequence that mincnlm's Exec() performs
              nor define the globals in nlm_globals.h.

              This header deliberately includes nothing but <cstddef>: volumes
              cross the boundary as flat buffers so that a consumer needs no
              ezminc headers on its include path.  Callers already holding a
              minc::simple_volume<T> can use nlm_denoise_volume.h instead.

@COPYRIGHT  :
              Copyright 2008 Pierrick Coupe, Vladimir Fonov,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#ifndef NLM_DENOISE_H
#define NLM_DENOISE_H

#include <cstddef>

namespace nlm
{

  //! Parameters for the classic (Coupe 2008) NL-means denoiser.
  //! Defaults are mincnlm's defaults, so a default-constructed struct
  //! reproduces `mincnlm in out`.
  struct denoise_params
  {
    //! Noise standard deviation. 0 => estimate it automatically with
    //! minc::noise_estimate (not available for weight_method 1).
    double sigma = 0.0;
    //! Smoothing strength; 1.0 is the published setting.
    double beta = 1.0;

    //! Patch (neighbourhood) radius in voxels; patches are 2r+1 cubed.
    //! Double rather than int because mincnlm's -v is a float option and
    //! truncates: keeping the type preserves `-v 1.5` exactly.
    double patch_radius = 1.0;
    //! Search-window radius in voxels; windows are 2r+1 cubed. mincnlm -d.
    double search_radius = 5.0;

    //! 0 = L2 norm (Gaussian noise), 1 = Pearson divergence (speckle),
    //! 2 = L2 norm with Rician bias correction.
    int weight_method = 0;

    //! true => block-wise NL-means, false => voxel-wise.
    bool block = true;
    //! Distance between blocks, block-wise mode only.
    int b_space = 2;

    //! Patch preselection tests, and the ratio bounds they accept.
    bool   test_mean = true;
    bool   test_var  = true;
    double m_min     = 0.95;
    double v_min     = 0.5;

    //! Adapt the patch/search shape to anisotropic voxels, using `steps`.
    //! Cannot be combined with block mode (mincnlm rejects that too).
    bool anisotropic = false;

    //! pthread count.
    int threads = 4;

    //! Chatter on stdout; debug additionally reports per-thread progress.
    int verbose = 0;
    int debug   = 0;
  };

  //! Denoise a volume.
  //!
  //! \param in     input, x-fastest, nx*ny*nz floats, not modified.
  //! \param out    output, caller-allocated, nx*ny*nz floats.
  //! \param sizes  {nx, ny, nz}; nx varies fastest.
  //! \param steps  voxel dimensions, same axis order as \p sizes. Consulted
  //!               only when p.anisotropic; may be NULL otherwise.
  //! \returns the noise sigma actually used -- the estimate, when p.sigma was 0.
  //! \throws std::runtime_error on an unusable parameter combination.
  //!
  //! \param hallucinate optional same-sized volume to draw patch intensities
  //!               from instead of \p in, weights still coming from \p in
  //!               (mincnlm -hallucinate). Read only; NULL for normal use.
  //!
  //! Note: with fewer than 2*p.threads slices along z, block mode is not
  //! usable and the voxel-wise path is taken instead, as mincnlm does.
  double denoise(const float *in, float *out, const int sizes[3],
                 const double steps[3], const denoise_params &p,
                 const float *hallucinate = NULL);

  //! Parameters for the adaptive (Manjon 2010) NL-means denoiser.
  //! Defaults are minc_anlm's defaults.
  struct anlm_params
  {
    int    search_radius = 2;
    int    patch_radius  = 1;
    bool   rician        = false;
    double beta          = 1.0;
    int    threads       = 1;
    bool   debug         = false;
  };

  //! Adaptive NL-means, which estimates a spatially varying noise level
  //! rather than taking a single sigma. Buffer conventions as for denoise().
  void denoise_adaptive(const double *in, double *out, const int sizes[3],
                        const anlm_params &p);

}

#endif //NLM_DENOISE_H
