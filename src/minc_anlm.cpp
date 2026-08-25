
#include <iostream>
#include <cstdlib>

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>
#include <minc_io_simple_volume.h>
#ifdef NLM_HAVE_MINC
#include <minc_1_simple.h>
#include <minc_1_simple_rw.h>
#include <time_stamp.h>    // for creating minc style history entry
#endif //NLM_HAVE_MINC
#include "minc_io_nifti_volume.h" // direct NIfTI reading & writing, no MINC dependency
#include "minc_histograms.h"
#include "anlm_proc.h"
#include <math.h>
#include <pthread.h>
#include <vector>

using namespace minc;

void show_usage(const char *name)
{
  std::cerr 
      << "This program implements adaptative non-local denoising algorithm published in "<<std::endl
      << "Jose V. Manjon, Pierrick Coupe, Luis Marti-Bonmati, D. Louis Collins, Montserrat Robles \"Adaptive non-local means denoising of MR images with spatially varying noise levels\""
      << " Journal of Magnetic Resonance Imaging Volume 31, Issue 1, pages 192–203, January 2010"<<std::endl
      << " DOI: 10.1002/jmri.22003"<<std::endl
      << std::endl
      << "Usage: "<<name<<" <source> <output_prefix>" << std::endl
      << "\t--rician correct for rician noise (remove bias)"<<std::endl
      << "\t--search <n> search radius"<<std::endl
      << "\t--patch <n> patch radius"<<std::endl
      << "\t--beta <f> adjust weight of smoothing, default  1, <1.0 - less >1.0 - more"<<std::endl
      << "\t--mt <n> use N threads, default is 1, WARNING: currently mutlithreaded version produces different results"<<std::endl
      << "\t--verbose be verbose" << std::endl
      << "\t--double store output as double"<<std::endl
      << "\t--float store output as float"<<std::endl
      << "\t--short store output as short"<<std::endl
      << "\t--byte store output as byte"<<std::endl;
}

int main(int argc,char **argv)
{
  int verbose=0;
  int debug=0;
  int clobber=0;
  int patch_radius=1;
  int search_radius=2;
  int rician=0;
  int store_float=0;
  int store_short=0;
  int store_double=0;
  int store_byte=0;
  int threads=1;
  double beta=1.0;
  
#ifdef NLM_HAVE_MINC
  char *history = time_stamp(argc, argv); //maybe we should free it afterwards
#endif //NLM_HAVE_MINC
  
  static struct option long_options[] =
  {
    {"verbose", no_argument , &verbose,      1},
    {"clobber", no_argument , &clobber,      1},
    {"debug",   no_argument , &debug,        1},
    {"quiet",   no_argument , &verbose,      0},
    {"float",   no_argument , &store_float,  1},
    {"short",   no_argument , &store_short,  1},
    {"byte",    no_argument , &store_byte,   1},
    {"double",  no_argument , &store_double, 1},
    {"rician",  no_argument , &rician,       1},
    {"mt",      required_argument,  0, 't'},
    {"threads", required_argument,  0, 't'},
    {"beta",    required_argument,  0, 'b'},
    {"search",  required_argument,  0, 's'},
    {"patch",    required_argument, 0, 'p'},
    {"help",    no_argument,        0, 'h'},
    {0, 0, 0, 0}
  };

  int c;
  for (;;)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;

    /* getopt_long_only (not getopt_long) so a single-dash "-help" is accepted
       too, matching mincnlm's convention. */
    c = getopt_long_only (argc, argv, "t:b:s:p:h", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
    {
      case 0:
        break;
      case 't':
        threads=atoi(optarg);
        if(threads<1)
        {
          std::cerr<<"Warning! Number of threads should be >= 1!"<<std::endl;
          threads=1;
        }
        break;
      case 'b':
        beta=fabs(atof(optarg));
        break;
      case 's':
        search_radius=atoi(optarg);
        break;
      case 'p':
        patch_radius=atoi(optarg);
        break;
      case 'h':
        show_usage(argv[0]);
        return 0;
      case '?':
        /* getopt_long_only already printed an error message. */
      default:
        show_usage(argv[0]);
        return 1;
    }
  }

  if ((argc - optind) < 2)
  {
    show_usage(argv[0]);
    return 1;
  }
  std::string input_src_f=argv[optind];
  std::string output_prefix=argv[optind+1];
  
  //std::string output_f=output_prefix+"_denoised.mnc";
  std::string output_f=output_prefix;
  
  bool nifti_in  = is_nifti_filename(input_src_f);
  bool nifti_out = is_nifti_filename(output_f);
  if(nifti_in != nifti_out)
  {
    std::cerr << "Mixing MINC and NIfTI between input and output is not supported;"
                  " use nii2mnc/mnc2nii to convert first" << std::endl;
    return 1;
  }

  std::string output_distance=output_prefix+(nifti_out?"_distance.nii.gz":"_distance.mnc");
  std::string output_variances=output_prefix+(nifti_out?"_variances.nii.gz":"_variances.mnc");
  std::string output_means=output_prefix+(nifti_out?"_means.nii.gz":"_means.mnc");
  std::string output_counts=output_prefix+(nifti_out?"_counts.nii.gz":"_counts.mnc");


  if (!clobber && !access(output_f.c_str(), F_OK))
  {
    std::cerr << output_f.c_str () << " Exists!" << std::endl;
    return 1;
  }

  try
  {
    simple_volume<double> src;

#ifdef NLM_HAVE_MINC  
    minc_1_reader rdr1;
    nc_type store_datatype = NC_FLOAT;
#endif //NLM_HAVE_MINC
    nifti_image* nifti_header = NULL;
    int nifti_store_datatype = 0; // 0 = keep same datatype as input

    if(nifti_in)
    {
      double vol_res[3];
      nifti_header = load_nifti_simple_volume<double>(input_src_f, src, vol_res);
      nifti_store_datatype = store_double?DT_FLOAT64:store_float?DT_FLOAT32:store_short?DT_INT16:store_byte?DT_UINT8:0;
    }
    else
    {
#ifdef NLM_HAVE_MINC
      rdr1.open(input_src_f.c_str());
      load_simple_volume<double>(rdr1,src);
      store_datatype= store_double?NC_DOUBLE:store_float?NC_FLOAT:store_short?NC_SHORT:store_byte?NC_BYTE:rdr1.datatype();
#else
      std::cerr << "this build has no MINC support; only .nii/.nii.gz files are"
                    " supported. Rebuild with MINC available to use .mnc files." << std::endl;
      return 1;
#endif //NLM_HAVE_MINC
    }

    if(debug)
      std::cout<<"Image dimensions:"<<src.dim(0)<<","<<src.dim(1)<<","<<src.dim(2)<<std::endl;

    anlm_proc anlm(src,search_radius,patch_radius,rician,beta,debug>0);

    anlm.exec(threads);
    std::cout<<"Done..."<<std::endl;

    if(nifti_out)
    {
      save_nifti_simple_volume<double>(output_f, anlm.fima, nifti_header, nifti_store_datatype);
    }
    else
    {
#ifdef NLM_HAVE_MINC
      minc_1_writer wrt;
      wrt.open(output_f.c_str(),rdr1.info(),2,store_datatype);
      wrt.append_history(history);
      save_simple_volume<double>(wrt,anlm.fima);
#else
      std::cerr << "this build has no MINC support; only .nii/.nii.gz files are"
                    " supported. Rebuild with MINC available to use .mnc files." << std::endl;
      return 1;
#endif //NLM_HAVE_MINC
    }

    if(debug)
    {
      std::cerr<<"Outputting debug information..."<<std::endl;

      if(nifti_out)
      {
        save_nifti_simple_volume<double>(output_distance, anlm.distances, nifti_header, nifti_store_datatype);
        save_nifti_simple_volume<int>(output_counts, anlm.Label, nifti_header, DT_INT32, /*no_rescale=*/true);
      }
      else
      {
#ifdef NLM_HAVE_MINC
        minc_1_writer wrt2;
        wrt2.open(output_distance.c_str(),rdr1.info(),2,store_datatype);
        wrt2.append_history(history);
        save_simple_volume<double>(wrt2,anlm.distances);


        minc_1_writer wrt3;
        wrt3.open(output_counts.c_str(),rdr1.info(),2,store_datatype);
        wrt3.append_history(history);
        save_simple_volume<int>(wrt3,anlm.Label);
#else
        std::cerr << "this build has no MINC support; only .nii/.nii.gz files are"
                      " supported. Rebuild with MINC available to use .mnc files." << std::endl;
        return 1;
#endif //NLM_HAVE_MINC
      }
    }

    if(nifti_header) nifti_image_free(nifti_header);

    return 0;
  } catch (const minc::generic_error & err) {
    std::cerr << "Got an error at:" << err.file () << ":" << err.line () << std::endl;
    std::cerr << err.msg()<<std::endl;
    return 1;
  }
}
