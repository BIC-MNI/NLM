#ifndef ANLM_PROC_H
#define ANLM_PROC_H

#include <minc_io_simple_volume.h>
#include <vector>

// Adaptive non-local means denoising algorithm, published in:
// Jose V. Manjon, Pierrick Coupe, Luis Marti-Bonmati, D. Louis Collins,
// Montserrat Robles "Adaptive non-local means denoising of MR images with
// spatially varying noise levels" Journal of Magnetic Resonance Imaging
// Volume 31, Issue 1, pages 192-203, January 2010. DOI: 10.1002/jmri.22003
class anlm_proc
{
  protected:

    struct thread_parameters{
      int ini;
      int fin;
      anlm_proc* _this;
    };

  //thread running stuff
  static void* worker_thread(void *arg);
  void* thread_process(int ini,int fin);

  static double bessi0(double x);
  static double bessi1(double x);
  static double Epsi(double snr);

  static void Regularize(const minc::simple_volume<double>& in,
                  minc::simple_volume<double>& out,int r);

  static double distance(const minc::simple_volume<double> & ima,
                          int x,int y,int z,
                          int nx,int ny,int nz,
                          int f);

  static double distance2(const minc::simple_volume<double> & ima,
                   const minc::simple_volume<double> & medias,
                   int x,int y,int z,
                   int nx,int ny,int nz,
                   int f);

  static void Average_block(const minc::simple_volume<double>& ima,
                     int x,int y,int z,
                     int neighborhoodsize,
                     std::vector<double> &average,
                     double weight,bool rician);

  static void Value_block(minc::simple_volume<double> &Estimate,
                   minc::simple_volume<int> &Label,
                   int x,int y,int z,
                   int neighborhoodsize,
                   const std::vector<double>  &average,
                   double global_sum);


  public:

  const minc::simple_volume<double>& ima;
  minc::simple_volume<double> fima;
  minc::simple_volume<double> means;
  minc::simple_volume<double> variances;

  minc::simple_volume<int> Label;
  minc::simple_volume<double> bias;
  minc::simple_volume<double> Estimate;
  minc::simple_volume<double> distances;

  int search_radius;
  int patch_radius;
  double imax;

  bool rician;
  double beta;
  bool debug;

  anlm_proc(const minc::simple_volume<double>&img,int search,int patch,bool rician,double beta,bool _debug=false);

  void exec(int Nthreads);
};

#endif //ANLM_PROC_H
