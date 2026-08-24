/* ----------------------------- MNI Header -----------------------------------
@NAME       : minc_io_nifti_volume.h
@DESCRIPTION: Direct NIfTI-1 (.nii/.nii.gz) I/O for simple_volume<T>, with no
              dependency on libminc/NetCDF/HDF5 whatsoever -- only the
              standalone simple_volume<T> container and the superbuild's
              vendored nifti1_io library are used.
@COPYRIGHT  :
              Copyright 2007 Vladimir Fonov, McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */
#ifndef MINC_IO_NIFTI_VOLUME_H
#define MINC_IO_NIFTI_VOLUME_H

#include "minc_io_simple_volume.h"
#include "minc_io_exceptions.h"

extern "C" {
#include <nifti1_io.h>
}

#include <string>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <algorithm>

namespace minc
{
  //! true if path ends in .nii or .nii.gz (case-insensitive)
  inline bool is_nifti_filename(const std::string& path)
  {
    std::string lc(path);
    std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
    if(lc.size() >= 4 && lc.compare(lc.size() - 4, 4, ".nii") == 0)
      return true;
    if(lc.size() >= 7 && lc.compare(lc.size() - 7, 7, ".nii.gz") == 0)
      return true;
    return false;
  }

  namespace _nifti_detail
  {
    //! read a single raw (unscaled) voxel at index i, of the given NIfTI
    //! datatype, as a double
    inline double get_raw_voxel(const void* data, int datatype, size_t i)
    {
      switch(datatype)
      {
        case DT_UINT8:    return (double)((const unsigned char*)data)[i];
        case DT_INT8:     return (double)((const signed char*)data)[i];
        case DT_INT16:    return (double)((const short*)data)[i];
        case DT_UINT16:   return (double)((const unsigned short*)data)[i];
        case DT_INT32:    return (double)((const int*)data)[i];
        case DT_UINT32:   return (double)((const unsigned int*)data)[i];
        case DT_FLOAT32:  return (double)((const float*)data)[i];
        case DT_FLOAT64:  return (double)((const double*)data)[i];
        default:
          REPORT_ERROR("Unsupported NIfTI datatype on read");
      }
      return 0.0;
    }

    //! write a single raw (unscaled) voxel value at index i, into a buffer
    //! of the given NIfTI datatype, rounding to nearest for integer types
    inline void set_raw_voxel(void* data, int datatype, size_t i, double v)
    {
      switch(datatype)
      {
        case DT_UINT8:   ((unsigned char*)data)[i] = (unsigned char)std::lround(v); break;
        case DT_INT8:    ((signed char*)data)[i]   = (signed char)std::lround(v); break;
        case DT_INT16:   ((short*)data)[i]         = (short)std::lround(v); break;
        case DT_UINT16:  ((unsigned short*)data)[i]= (unsigned short)std::lround(v); break;
        case DT_INT32:   ((int*)data)[i]           = (int)std::lround(v); break;
        case DT_UINT32:  ((unsigned int*)data)[i]  = (unsigned int)std::lround(v); break;
        case DT_FLOAT32: ((float*)data)[i]         = (float)v; break;
        case DT_FLOAT64: ((double*)data)[i]        = v; break;
        default:
          REPORT_ERROR("Unsupported NIfTI datatype on write");
      }
    }

    //! full representable range of an integer NIfTI storage type, used
    //! when requantizing to a *different* datatype than the input had
    inline void datatype_range(int datatype, double& lo, double& hi)
    {
      switch(datatype)
      {
        case DT_UINT8:  lo=0;      hi=255;        break;
        case DT_INT8:   lo=-128;   hi=127;         break;
        case DT_INT16:  lo=-32768; hi=32767;       break;
        case DT_UINT16: lo=0;      hi=65535;       break;
        case DT_INT32:  lo=-2147483648.0; hi=2147483647.0; break;
        case DT_UINT32: lo=0;      hi=4294967295.0; break;
        default:
          REPORT_ERROR("Unsupported NIfTI datatype for range query");
      }
    }

    //! scale factor to convert dx/dy/dz (in the file's xyz_units) to
    //! millimeters, mirroring nii2mnc's unit handling
    inline double xyz_units_to_mm(int xyz_units)
    {
      switch(xyz_units)
      {
        case NIFTI_UNITS_METER:  return 1000.0;
        case NIFTI_UNITS_MICRON: return 0.001;
        default:                 return 1.0; // mm, or unknown -> assume mm
      }
    }
  }

  //! Load a NIfTI-1 volume directly into a simple_volume<T> (real-valued,
  //! i.e. raw*scl_slope+scl_inter), filling vol_res[0..2] with the absolute
  //! voxel spacing in mm. Returns the source header (with data already
  //! freed) so its geometry can later be cloned verbatim for the output by
  //! save_nifti_simple_volume() -- the caller must nifti_image_free() it.
  template<class T>
  nifti_image* load_nifti_simple_volume(const std::string& path, simple_volume<T>& vol, double vol_res[3])
  {
    nifti_image* nim = nifti_image_read(path.c_str(), 1);
    if(!nim)
      REPORT_ERROR(("Can't read NIfTI file:"+path).c_str());

    int nx = nim->nx>0 ? nim->nx : 1;
    int ny = nim->ny>0 ? nim->ny : 1;
    int nz = nim->nz>0 ? nim->nz : 1;

    vol.resize((size_t)nx, (size_t)ny, (size_t)nz);

    double units = _nifti_detail::xyz_units_to_mm(nim->xyz_units);
    vol_res[0] = fabs(nim->dx) * units;
    vol_res[1] = fabs(nim->dy) * units;
    vol_res[2] = fabs(nim->dz) * units;

    double slope = nim->scl_slope;
    double inter = nim->scl_inter;
    if(slope == 0.0) { slope = 1.0; inter = 0.0; }

    size_t n = (size_t)nx * ny * nz;
    T* out = vol.c_buf();
    for(size_t i = 0; i < n; i++)
      out[i] = (T)(_nifti_detail::get_raw_voxel(nim->data, nim->datatype, i) * slope + inter);

    // done with the raw data buffer; keep the header itself (fname/iname,
    // qform/sform, pixdim, units, ...) alive as the geometry template
    free(nim->data);
    nim->data = NULL;

    return nim;
  }

  //! Write vol to path as a NIfTI-1 file, cloning ref_header's geometry and
  //! header fields verbatim (qform/sform/pixdim/units/... are never
  //! recomputed -- NLM never touches geometry). store_datatype is a NIfTI
  //! DT_* code selecting the output storage type; 0 means "same as
  //! ref_header->datatype" (matches both tools' default "keep input dtype"
  //! policy). When store_datatype differs from the input's datatype, a
  //! fresh full-range linear scl_slope/scl_inter is derived from the data;
  //! this quantization is best-effort, not guaranteed to bit-match MINC's.
  //! no_rescale: force identity scl_slope/scl_inter (1/0), writing the
  //! volume's values directly as raw integers -- use this for volumes that
  //! are already exact small integers (e.g. label/count maps), where a
  //! full-range rescale into the target datatype would be meaningless.
  template<class T>
  void save_nifti_simple_volume(const std::string& path, const simple_volume<T>& vol, nifti_image* ref_header, int store_datatype = 0, bool no_rescale = false)
  {
    nifti_image* out = nifti_copy_nim_info(ref_header);
    if(!out)
      REPORT_ERROR(("Can't clone NIfTI header for:"+path).c_str());

    int dt = store_datatype ? store_datatype : ref_header->datatype;
    out->datatype = dt;
    nifti_datatype_sizes(dt, &out->nbyper, NULL);

    out->ndim = 3;
    out->nx = (int)vol.dim(0);
    out->ny = (int)vol.dim(1);
    out->nz = (int)vol.dim(2);
    out->nt = out->nu = out->nv = out->nw = 1;
    out->dim[0] = 3;
    out->dim[1] = out->nx;
    out->dim[2] = out->ny;
    out->dim[3] = out->nz;
    out->dim[4] = out->dim[5] = out->dim[6] = out->dim[7] = 1;
    out->nvox = (size_t)out->nx * out->ny * out->nz;

    bool is_float_type = (dt == DT_FLOAT32 || dt == DT_FLOAT64);

    double slope = 1.0, inter = 0.0;
    const T* buf = vol.c_buf();
    if(!is_float_type && !no_rescale)
    {
      if(dt == ref_header->datatype && ref_header->scl_slope != 0.0)
      {
        // same storage type as the input: reuse its scaling verbatim (this
        // is the common "keep input dtype" default -- no requantization)
        slope = ref_header->scl_slope;
        inter = ref_header->scl_inter;
      }
      else
      {
        double dmin = std::numeric_limits<double>::max();
        double dmax = -std::numeric_limits<double>::max();
        for(size_t i = 0; i < out->nvox; i++)
        {
          double v = (double)buf[i];
          if(v < dmin) dmin = v;
          if(v > dmax) dmax = v;
        }
        double type_min, type_max;
        _nifti_detail::datatype_range(dt, type_min, type_max);
        if(dmax > dmin)
        {
          slope = (dmax - dmin) / (type_max - type_min);
          inter = dmin - slope * type_min;
        }
      }
    }
    out->scl_slope = (float)slope;
    out->scl_inter = (float)inter;

    out->data = calloc(out->nvox, out->nbyper);
    if(!out->data)
      REPORT_ERROR("Out of memory allocating NIfTI output buffer");

    for(size_t i = 0; i < out->nvox; i++)
    {
      double v = (double)buf[i];
      double raw = is_float_type ? v : (slope != 0.0 ? (v - inter) / slope : v);
      _nifti_detail::set_raw_voxel(out->data, dt, i, raw);
    }

    nifti_set_filenames(out, path.c_str(), 0, 1);
    nifti_image_write(out);
    nifti_image_free(out);
  }
}

#endif //MINC_IO_NIFTI_VOLUME_H
