/* Library entry points for the NLM denoisers -- see nlm_denoise.h.
 *
 * denoise() is mincnlm's Exec() (mincnlm.cpp) lifted out of the CLI, with
 * three changes and no others, so that the filtered result is unchanged:
 *   - parameters come from denoise_params instead of file-scope globals;
 *   - the two exit(0) calls and the speckle/auto-sigma bail-out become
 *     exceptions (exit(0) reported success on a fatal misconfiguration, and
 *     the bail-out returned leaving the output buffer untouched);
 *   - stdout chatter keeps exactly the gating Exec had.
 *
 * The legacy globals in nlm_globals.h are still set here, because nl_means.cpp
 * and nl_means_block.cpp read them directly rather than taking them as
 * arguments.
 */

#include "nlm_denoise.h"
#include "nlm_globals.h"

#include "nl_means_utils.h"
#include "nl_means_block.h"
#include "noise_estimate.h"
#include "anlm_proc.h"

#include <minc_io_simple_volume.h>

#include <iostream>
#include <stdexcept>
#include <vector>

namespace nlm
{

  double denoise(const float *in, float *out, const int sizes[3],
                 const double steps[3], const denoise_params &p,
                 const float *hallucinate)
  {
    if(!in || !out || !sizes)
      throw std::runtime_error("nlm::denoise: null buffer");
    if(sizes[0] < 1 || sizes[1] < 1 || sizes[2] < 1)
      throw std::runtime_error("nlm::denoise: empty volume");
    if(p.threads < 1)
      throw std::runtime_error("nlm::denoise: threads must be >= 1");

    // The algorithm routines read these rather than taking them as arguments.
    verbose   = p.verbose;
    debug     = p.debug;
    nb_thread = p.threads;
    testmean  = p.test_mean ? 1 : 0;
    testvar   = p.test_var  ? 1 : 0;
    block     = p.block     ? 1 : 0;

    int vol_size[3] = { sizes[0], sizes[1], sizes[2] };

    // noise_estimate() takes a simple_volume, and the denoisers take a
    // non-const float*, so the input is copied once and left untouched.
    minc::simple_volume<float> work(vol_size[0], vol_size[1], vol_size[2]);
    const size_t count = (size_t)vol_size[0] * vol_size[1] * vol_size[2];
    float *ima_in = work.c_buf();
    for(size_t i = 0; i < count; i++)
      ima_in[i] = in[i];

    double vol_res[3] = { 1.0, 1.0, 1.0 };
    if(steps)
    {
      vol_res[0] = steps[0];
      vol_res[1] = steps[1];
      vol_res[2] = steps[2];
    }

    double filtering_param = p.sigma;
    int neighborhoodsize[3];
    int searching[3];

    if(vol_size[2] < 2 * nb_thread)
    {
      std::cout << "\n------------------------------------------------" << std::endl;
      std::cout << "!The number of slices is too small (<nb_thread)!" << std::endl;
      std::cout << "!!    => Set voxelwise mode (block = 0)       !!" << std::endl;
      std::cout << "------------------------------------------------" << std::endl;
      block = 0;
    }

    if(!p.anisotropic)
    {
      neighborhoodsize[0] = (int) p.patch_radius;
      neighborhoodsize[1] = (int) p.patch_radius;
      neighborhoodsize[2] = (int) p.patch_radius;
      searching[0] = (int) p.search_radius;
      searching[1] = (int) p.search_radius;
      searching[2] = (int) p.search_radius;
    }
    else
    {
      if(verbose) {
        std::cout << "\n------------------------------------------------" << std::endl;
        std::cout << "                 Anisotropy mode                 " << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << " The neighborhood size is adapted to the anisotropy of the image" << std::endl;
      }

      // set default parameters
      neighborhoodsize[0] = (int) p.patch_radius;
      neighborhoodsize[1] = (int) p.patch_radius;
      neighborhoodsize[2] = (int) p.patch_radius;
      searching[0] = (int) p.search_radius;
      searching[1] = (int) p.search_radius;
      searching[2] = (int) p.search_radius;

      std::cout<<" -- Neighborhoodsize: "<<neighborhoodsize[0]<<","<<neighborhoodsize[1]<<","<<neighborhoodsize[2]<<std::endl;
      std::cout<<" -- Searching: "<<searching[0]<<","<<searching[1]<<","<<searching[2]<<std::endl;

      //check 2D dimension
      if (vol_res[0]>vol_res[1] && vol_res[0]>vol_res[2])
      {
        neighborhoodsize[0] = (int) 0;
        searching[0] = (int) 1;
      }
      else if (vol_res[1]>vol_res[2] && vol_res[1]>vol_res[0])
      {
        neighborhoodsize[1] = (int) 0;
        searching[1] = (int) 1;
      }
      else if (vol_res[2]>vol_res[1] && vol_res[2]>vol_res[0])
      {
        neighborhoodsize[2] = (int)0;
        searching[2] = (int) 1;
      }

      std::cout<<" -- Resolution: "<<vol_res[0]<<","<<vol_res[1]<<","<<vol_res[2]<<std::endl;
      std::cout<<" -- Neighborhoodsize: "<<neighborhoodsize[0]<<","<<neighborhoodsize[1]<<","<<neighborhoodsize[2]<<std::endl;
      std::cout<<" -- Searching: "<<searching[0]<<","<<searching[1]<<","<<searching[2]<<std::endl;
    }

    if(verbose) {
      std::cout <<"\n--------------------------------------------------"<< std::endl;
      std::cout <<    "                   Parameters                     " << std::endl;
      std::cout <<"--------------------------------------------------\n"<< std::endl;

      std::cout <<      "Sigma           : ";
      if (filtering_param == 0)
        std::cout << "Automatic: only Rician or Gaussian noise" << std::endl;
      else
        std::cout << filtering_param << std::endl;
      if (p.beta!=1.0)
        std::cout << "Weighting parameter " << p.beta << std::endl;

      std::cout <<      "Ni              : " << 2*neighborhoodsize[0]+1 << " x " << 2*neighborhoodsize[1]+1 << " x " << 2*neighborhoodsize[2]+1 << std::endl;
      std::cout <<      "Vi              : " << 2*searching[0]+1 << " x " << 2*searching[1]+1 << " x " << 2*searching[2]+1 << std::endl;

      if (testmean)
        std::cout <<    "Mean Test       : Yes, " << p.m_min << " < X < " << 1/p.m_min << std::endl;
      else
        std::cout << "Mean Test       : No" << std::endl;

      if (testvar)
        std::cout <<    "Variance Test   : Yes, " << p.v_min << " < X < " << 1/p.v_min << std::endl;
      else
        std::cout << "Variance Test   : No" << std::endl;

      std::cout << std::endl;

      if (p.anisotropic)
        std::cout <<  "Anisotropic mode activated       : Yes" << std::endl;
      else
        std::cout <<  "Anisotropic mode activated       : No" << std::endl;
    }

    if (block == 1)
    {
      if(verbose) {
        std::cout << "Block Implementaiton of NL-means : Yes" << std::endl;
        std::cout << "--> Distance between blocks      : " << p.b_space << std::endl;
      }
      if (neighborhoodsize[0] < (p.b_space/2))
        throw std::runtime_error("nlm::denoise: we must have Ni < (D_block)/2");

      if (p.anisotropic)
        throw std::runtime_error("nlm::denoise: can't activate anisotropic mode in block version");
    }

    if(verbose) {
      if (block == 0)
        std::cout << "Block Implementation of NL-means : No " << std::endl;
      if (p.weight_method == 0)
        std::cout << "Weighting Method                 : L2-norm (Gaussian noise) " << std::endl;
      if (p.weight_method == 1)
        std::cout << "Weighting Method                 : Pearson Divergence (Speckle)" << std::endl;
      if (p.weight_method == 2)
        std::cout << "Weighting Method                 : L2-norm + Bais correction (Rician noise) " << std::endl;

      std::cout<< "\n" << std::endl;
    }

    /* local mean and local variance storage */
    std::vector<float> mean_map(count, 0.0f);
    std::vector<float> var_map(count, 0.0f);

    if(verbose) {
      std::cout <<"\n--------------------------------------------------"<< std::endl;
      std::cout <<    "                  Preprocessing                      " << std::endl;
      std::cout <<"--------------------------------------------------\n"<< std::endl;
    }

    // Computation of the local means
    Preprocessing(ima_in, mean_map.data(), neighborhoodsize, vol_size);

    if (testvar == 1)
      // Computation of the local variances
      Preprocessing2(ima_in, mean_map.data(), var_map.data(), neighborhoodsize, vol_size);

    if (filtering_param == 0)
    {
      if(p.weight_method == 1)
        throw std::runtime_error("nlm::denoise: automatic variance estimation is not "
                                 "available for speckle noise (weight_method 1); "
                                 "supply sigma explicitly");

      double mean_val = 0.0;
      //use gaussian estimate appropriately
      filtering_param = minc::noise_estimate(work, mean_val, p.weight_method == 0, verbose);
    }

    if(verbose) {
      std::cout <<"\n--------------------------------------------------"<< std::endl;
      std::cout <<    "                  Denoising                      " << std::endl;
      std::cout <<"--------------------------------------------------\n"<< std::endl;
    }

    // The denoisers declare hallucinate as float* but only ever read it.
    float *hall = const_cast<float *>(hallucinate);

    if (block == 0)
      denoise_mt(ima_in, out, mean_map.data(), var_map.data(), filtering_param, p.beta,
                 neighborhoodsize, searching,
                 testmean, testvar, p.m_min, p.v_min, p.weight_method, vol_size, hall);

    if (block == 1)
      denoise_block_mt(ima_in, out, mean_map.data(), var_map.data(), filtering_param, p.beta,
                       neighborhoodsize, searching,
                       testmean, testvar, p.m_min, p.v_min, p.weight_method, p.b_space,
                       vol_size, hall);

    return filtering_param;
  }


  void denoise_adaptive(const double *in, double *out, const int sizes[3],
                        const anlm_params &p)
  {
    if(!in || !out || !sizes)
      throw std::runtime_error("nlm::denoise_adaptive: null buffer");
    if(sizes[0] < 1 || sizes[1] < 1 || sizes[2] < 1)
      throw std::runtime_error("nlm::denoise_adaptive: empty volume");
    if(p.threads < 1)
      throw std::runtime_error("nlm::denoise_adaptive: threads must be >= 1");

    minc::simple_volume<double> src(sizes[0], sizes[1], sizes[2]);
    const size_t count = (size_t)sizes[0] * sizes[1] * sizes[2];
    for(size_t i = 0; i < count; i++)
      src.c_buf()[i] = in[i];

    // anlm_proc holds its input by const reference, so src has to outlive exec().
    anlm_proc anlm(src, p.search_radius, p.patch_radius, p.rician, p.beta, p.debug);
    anlm.exec(p.threads);

    for(size_t i = 0; i < count; i++)
      out[i] = anlm.fima.c_buf()[i];
  }

}
