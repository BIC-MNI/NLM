# NLM — Non-Local Means denoising

Automatic and multithreaded denoising of 3D medical images based on the
Non-Local Means (NLM) filter, for MINC (`.mnc`) and NIfTI (`.nii`, `.nii.gz`)
volumes.

This is one subproject of [minc-toolkit-v2](https://github.com/BIC-MNI/minc-toolkit-v2),
but it builds and runs on its own. The code was moved here from
[EZminc/mincnlm](https://github.com/BIC-MNI/EZminc/tree/master/mincnlm); unlike
its old home it has no ITK dependency, so it is built in every toolkit
configuration, including `MT_BUILD_LITE=ON`.

## Contents

| What | Kind | Purpose |
| --- | --- | --- |
| `mincnlm` | program | Classic NL-means denoising (Coupé 2008), voxel-wise or block-wise, Gaussian / speckle / Rician noise models. |
| `minc_anlm` | program | Adaptive NL-means denoising (Manjón 2010), for spatially varying noise. |
| `noise_estimate` | program | Report the noise level or the SNR of a volume; no filtering. |
| `nlm` | static library | The algorithms above with no image I/O attached, for linking into other C++ tools. |

The three programs read and write MINC or NIfTI, chosen from the file
extension. The library knows nothing about either format: volumes cross its
boundary as flat buffers.

## Algorithms and references

**Classic NL-means** (`mincnlm`, `nlm::denoise`) — every voxel is replaced by a
weighted average of voxels whose surrounding patch resembles its own, with the
weights falling off exponentially in patch distance. Patches are searched inside
a local window rather than over the whole volume. The block-wise variant
(default) estimates whole blocks at a time on a sub-lattice and averages the
overlapping estimates, which is both faster and less noisy than the voxel-wise
form:

> P. Coupé, P. Yger, S. Prima, P. Hellier, C. Kervrann, C. Barillot. *An
> Optimized Blockwise Non Local Means Denoising Filter for 3D Magnetic Resonance
> Images.* IEEE Transactions on Medical Imaging, 27(4):425-441, April 2008.

Speckle (ultrasound) noise, selected with `-w 1`, replaces the L2 patch distance
with a Pearson divergence:

> P. Coupé, P. Hellier, C. Kervrann, C. Barillot. *Bayesian non local
> means-based speckle filtering.* ISBI'2008, Paris, France, May 2008.

Rician noise, selected with `-w 2`, keeps the L2 distance and removes the
noise-induced bias from the result, which matters for low-SNR magnitude MRI
such as DT-MRI:

> N. Wiest-Daesslé, S. Prima, P. Coupé, S.P. Morrissey, C. Barillot. *Rician
> noise removal by non-local means filtering for low signal-to-noise ratio MRI:
> Applications to DT-MRI.* MICCAI'2008, New York, USA, September 2008.

**Adaptive NL-means** (`minc_anlm`, `nlm::denoise_adaptive`) — estimates the
noise level locally instead of taking one sigma for the whole volume, so it
copes with the spatially varying noise that parallel imaging and surface-coil
acquisitions produce:

> J.V. Manjón, P. Coupé, L. Martí-Bonmatí, D.L. Collins, M. Robles. *Adaptive
> non-local means denoising of MR images with spatially varying noise levels.*
> Journal of Magnetic Resonance Imaging, 31(1):192-203, January 2010.
> DOI: 10.1002/jmri.22003

**Noise estimation** (`noise_estimate`, `minc::noise_estimate`) — a 3D wavelet
decomposition, k-means object detection on the low-pass band, and a MAD
estimator on the high-pass band, optionally corrected for the Rician bias by
Koay's fixed-point iteration. `mincnlm` calls this when no `-sigma` is given:

> P. Coupé, J.V. Manjón, E. Gedamu, D.L. Arnold, M. Robles, D.L. Collins. *An
> Object-Based Method for Rician Noise Estimation in MR Images.* MICCAI (1)
> 2009: 601-608.

## Command-line tools

### mincnlm

```
mincnlm [options] <in.mnc> <out.mnc>
mincnlm [options] <in.nii.gz> <out.nii.gz>
mincnlm -help
```

Input must be a 3D volume. Input and output must be the same format — use
`nii2mnc`/`mnc2nii` to convert first if they are not.

| Option | Default | Meaning |
| --- | --- | --- |
| `-sigma <f>` | `0` | Noise standard deviation, in the intensity units of the file. `0` estimates it automatically. |
| `-beta <f>` | `1` | Smoothing strength. Below 1 filters less, above 1 filters more; 1 is the published setting. |
| `-v <f>` | `1` | Patch radius in voxels: 1 gives a 3×3×3 patch (26 neighbours), 2 gives 5×5×5 (124 neighbours). |
| `-d <f>` | `5` | Search-window radius in voxels: 5 gives an 11×11×11 window (1331 candidates). |
| `-w <n>` | `0` | Noise model: `0` L2-norm (Gaussian), `1` Pearson divergence (speckle), `2` L2-norm with Rician bias correction. |
| `-aniso` | off | Adapt the patch and window shape to anisotropic voxels: the axis with the coarsest sampling gets patch radius 0 and window radius 1. Requires `-block 0`. |
| `-block <n>` | `1` | `1` block-wise, `0` voxel-wise. |
| `-b_space <n>` | `2` | Distance between blocks, block-wise mode only. |
| `-m_min <f>` | `0.95` | Patch preselection: accept a candidate only if the ratio of local means is between `m_min` and `1/m_min`. |
| `-v_min <f>` | `0.5` | Patch preselection: same test on the ratio of local variances. |
| `-mt <n>` | `4` | Number of threads. Work is split along the last (slowest-varying) axis. |
| `-hallucinate <file>` | — | Experimental: take patch intensities from this volume while computing weights from the input. Must have the same dimensions as the input. |
| `-verbose` | off | Report the parameters and progress. |
| `-debug` | off | Report more, including per-thread progress. |
| `-clobber` | off | Overwrite an existing output file. |
| `-references` | — | Print the citations above. |
| `-help` | — | Print the option list. |

Examples:

```sh
# Estimate the noise level and filter with the published defaults
mincnlm input.mnc denoised.mnc

# Magnitude MRI at low SNR: Rician noise model, 8 threads, chatty
mincnlm -w 2 -mt 8 -verbose input.mnc denoised.mnc

# Known noise level, milder filtering, larger patches
mincnlm -sigma 12.5 -beta 0.7 -v 2 input.mnc denoised.mnc

# Ultrasound speckle: -w 1 has no automatic noise estimator, sigma is required
mincnlm -w 1 -sigma 8 ultrasound.mnc denoised.mnc

# Thick-slice acquisition: shape the patch to the voxels, voxel-wise mode
mincnlm -aniso -block 0 input.mnc denoised.mnc

# NIfTI in, NIfTI out
mincnlm input.nii.gz denoised.nii.gz
```

Parameter combinations the filter rejects, with a message and a non-zero exit
status:

- `-w 1` without `-sigma`: there is no automatic noise estimator for speckle.
- `-aniso` together with block mode: pass `-block 0` as well.
- a patch radius smaller than `b_space/2` in block mode: raise `-v` or lower
  `-b_space`.

Block mode also needs at least `2 × threads` slices along the last axis. With
fewer, `mincnlm` prints a notice and falls back to voxel-wise mode by itself.

### minc_anlm

```
minc_anlm [options] <in.mnc> <out.mnc>
minc_anlm -help
```

| Option | Default | Meaning |
| --- | --- | --- |
| `--rician` | off | Correct the Rician noise bias, for magnitude MRI. |
| `--search <n>` | `2` | Search-window radius in voxels. |
| `--patch <n>` | `1` | Patch radius in voxels. |
| `--beta <f>` | `1` | Smoothing strength, as in `mincnlm`. |
| `--mt <n>`, `--threads <n>` | `1` | Number of threads. **The multithreaded result differs from the single-threaded one** — see [Known issues](#known-issues-and-limitations). |
| `--double`, `--float`, `--short`, `--byte` | input type | Store the output in this type instead of the input's. |
| `--verbose`, `--quiet` | quiet | Chatter. |
| `--debug` | off | Chatter, plus extra volumes next to the output (see below). |
| `--clobber` | off | Overwrite an existing output file. |
| `--help` | — | Print the option list. |

Single-dash spellings (`-rician`, `-search`) are accepted too.

Examples:

```sh
# Spatially varying noise, defaults
minc_anlm input.mnc denoised.mnc

# Magnitude MRI, wider search, 4 threads, output as short
minc_anlm --rician --search 3 --mt 4 --short input.mnc denoised.mnc
```

With `--debug`, two more volumes are written, named after the output file:
`<output>_distance.mnc` (the patch distances the filter worked with) and
`<output>_counts.mnc` (how many block estimates contributed to each voxel).

### noise_estimate

```
noise_estimate [options] <in.mnc>
noise_estimate -help
```

Prints one number on stdout, so it drops straight into a shell variable.

| Option | Default | Meaning |
| --- | --- | --- |
| `--noise` | on | Print the estimated noise standard deviation. |
| `--snr` | off | Print mean signal / noise. Both may be given, `--noise` first. |
| `--gauss` | off | Assume Gaussian noise. Without it the Rician (Koay) correction is applied. |
| `--bins <n>` | `2000` | Histogram bins used by the k-means object detection. |
| `--mask <mask.mnc>` | — | Use this object mask instead of detecting the object. Must match the input's dimensions and step sizes, and be in the same format. |
| `--verbose` | off | Label the output and report the intermediate quantities. |
| `--help` | — | Print the option list. |

```sh
noise_estimate input.mnc                       # e.g. 11.8273
noise_estimate --snr input.mnc                 # e.g. 42.1044
noise_estimate --gauss --mask brain.mnc t1.mnc

# Estimate once, then reuse the value
sigma=$(noise_estimate input.mnc)
mincnlm -sigma "$sigma" input.mnc denoised.mnc
```

## Library API

The `nlm` target is the algorithms without any file format: pthreads, GSL,
FFTW3F and a header-only volume container, nothing more. That is what lets an
unrelated tool link it without inheriting MINC — `nu_correct_cxx -denoise` in
[N3](https://github.com/BIC-MNI/N3) does exactly this (see
`N3/src/N3Pipeline/Denoise.cc` for a complete consumer).

### Linking

Inside the toolkit superbuild (or any CMake project that has this directory as
a subdirectory) the target is available by name and carries its own include
directories and dependencies as usage requirements:

```cmake
TARGET_LINK_LIBRARIES(my_tool nlm)
```

`nlm` is deliberately not installed and ships no `NLMConfig.cmake`, so linking
it from a separate build tree is not supported; only the three programs are
installed.

### Buffer conventions

Volumes are flat buffers of `nx*ny*nz` elements with **x varying fastest**:
index `x + y*nx + z*nx*ny`. `sizes` is `{nx, ny, nz}` and `steps` holds the
voxel dimensions in the same axis order.

MINC and volume_io store volumes in file order, where the *last* dimension
varies fastest. Reversing both the `sizes` and the `steps` triples makes the two
descriptions agree about the same buffer, with no transpose of the data. Getting
that wrong does not crash: it silently filters with the wrong anisotropy, and
for a non-cubic volume reads out of bounds.

### `nlm::denoise` — classic NL-means

```c++
#include <nlm_denoise.h>

namespace nlm {
  struct denoise_params {
    double sigma         = 0.0;   // 0 => estimate automatically
    double beta          = 1.0;
    double patch_radius  = 1.0;   // mincnlm -v
    double search_radius = 5.0;   // mincnlm -d
    int    weight_method = 0;     // 0 L2, 1 Pearson, 2 L2 + Rician bias
    bool   block         = true;
    int    b_space       = 2;
    bool   test_mean     = true;
    bool   test_var      = true;
    double m_min         = 0.95;
    double v_min         = 0.5;
    bool   anisotropic   = false;
    int    threads       = 4;
    int    verbose       = 0;
    int    debug         = 0;
  };

  double denoise(const float *in, float *out, const int sizes[3],
                 const double steps[3], const denoise_params &p,
                 const float *hallucinate = NULL);
}
```

A default-constructed `denoise_params` reproduces `mincnlm in out`. The whole
pipeline is behind this one call: parameter derivation, the local mean and
variance preprocessing, automatic noise estimation, and the multithreaded
denoise itself.

- `in` is read and not modified; `out` is caller-allocated, `nx*ny*nz` floats.
- `steps` is consulted only when `p.anisotropic`, and may be `NULL` otherwise.
- `hallucinate`, when given, is a same-sized volume that patch intensities are
  drawn from while the weights still come from `in` (`mincnlm -hallucinate`).
- Returns the sigma actually used, which is the estimate when `p.sigma` was 0.
- Throws `std::runtime_error` on a null buffer, an empty volume, `threads < 1`,
  or one of the rejected parameter combinations listed under `mincnlm` above.
- With fewer than `2*p.threads` slices along z, block mode is not usable and
  the voxel-wise path is taken instead, as `mincnlm` does.

`nlm_denoise.h` includes nothing but `<cstddef>`, so a consumer needs no MINC or
ezminc headers on its include path.

```c++
#include <nlm_denoise.h>
#include <vector>

// A 3D volume already in memory, x fastest.
std::vector<float> noisy = load_somehow();
const int sizes[3] = { nx, ny, nz };
const double steps[3] = { 1.0, 1.0, 1.0 };

std::vector<float> clean(noisy.size(), 0.0f);

nlm::denoise_params p;
p.weight_method = 2;   // magnitude MRI: Rician bias correction
p.threads       = 8;

const double sigma = nlm::denoise(&noisy[0], &clean[0], sizes, steps, p);
printf("filtered with sigma = %f\n", sigma);
```

### `nlm::denoise_adaptive` — adaptive NL-means

```c++
namespace nlm {
  struct anlm_params {
    int    search_radius = 2;
    int    patch_radius  = 1;
    bool   rician        = false;
    double beta          = 1.0;
    int    threads       = 1;
    bool   debug         = false;
  };

  void denoise_adaptive(const double *in, double *out, const int sizes[3],
                        const anlm_params &p);
}
```

Same buffer conventions, but `double` rather than `float`, and no `steps`: the
adaptive filter derives its own spatially varying noise map and does not use the
voxel dimensions. Defaults are `minc_anlm`'s defaults.

### `simple_volume<T>` overloads

A caller that already holds a `minc::simple_volume<T>` can include
`nlm_denoise_volume.h` instead and skip the buffer bookkeeping. `out` is resized
to match `in`:

```c++
#include <nlm_denoise_volume.h>

minc::simple_volume<float> in = /* ... */, out;
nlm::denoise_params p;
double sigma = nlm::denoise(in, out, p);            // steps default to NULL

minc::simple_volume<double> din = /* ... */, dout;
nlm::denoise_adaptive(din, dout, nlm::anlm_params());
```

### `minc::noise_estimate` — noise level on its own

```c++
#include <noise_estimate.h>

namespace minc {
  double noise_estimate(const simple_volume<float>& input,
                        double &mean_signal,
                        bool gaussian = false,
                        bool verbose = false,
                        int hist_bins = 2000,
                        const minc_byte_volume& mask = minc_byte_volume());
}
```

Returns the estimated noise standard deviation and sets `mean_signal` to the
mean intensity inside the detected (or supplied) object, so `mean_signal /
returned` is the SNR. `gaussian = true` skips the Rician correction. Pass a
non-empty `mask` to override the built-in object detection.

### Lower-level pieces

These are the implementation, used by the entry points above and by the tests.
They are usable but have no stability guarantee, and the first two read
process-wide globals declared in `nlm_globals.h` (`verbose`, `debug`,
`nb_thread`, `testmean`, `testvar`, `block`) which the caller must set first:

| Header | What it holds |
| --- | --- |
| `nl_means.h` | `denoise_mt()` — voxel-wise NL-means; patch distances and weights. |
| `nl_means_block.h` | `denoise_block_mt()` — block-wise NL-means. |
| `nl_means_utils.h` | Local mean/variance preprocessing, intensity scaling, pseudo-residual variance estimation. |
| `anlm_proc.h` | `anlm_proc` — the adaptive filter as a class, exposing its intermediate volumes (`fima`, `means`, `variances`, `distances`, `Label`). It holds its input **by const reference**, which must outlive `exec()`. |
| `noise_estimate.h` | The noise estimator (documented above). |
| `minc_histograms.h` | Histograms, joint histograms, k-means, KL and KS distances. |
| `fftw_blur.h` | Gaussian blurring and gradients through FFTW3F. |
| `dwt_utils.h`, `volume_dwt.h` | The 3D wavelet transform the noise estimator runs on. |

New code should prefer `nlm::denoise_params` over the globals; `nlm::denoise`
sets them from the struct before dispatching. They remain visible because
`mincnlm`'s `ParseArgv` table writes into their addresses.

## Building

### As part of minc-toolkit-v2

Nothing to do: the superbuild adds this directory unconditionally, ahead of N3
so that the `nlm` target exists when N3 links it. Follow the
[toolkit build instructions](../README.md#build-from-source); the three programs
land in `<prefix>/bin`.

MINC support is not optional there. The superbuild always builds libminc with
ezminc and hands this directory the in-tree `minc_io`/`minc2` targets, so
`NLM_HAVE_MINC` is always set and the installed programs always read and write
`.mnc`; if libminc were ever missing the configure fails rather than quietly
building NIfTI-only.

### Standalone

```sh
git clone https://github.com/BIC-MNI/NLM.git
cd NLM
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest
```

Needs CMake 3.10 or newer and a C++14 compiler. By default the build downloads
and builds its own static ZLIB, GSL, FFTW3F and nifticlib, which needs network
access at build time; point it at installed copies instead with:

| Option | Default | Effect |
| --- | --- | --- |
| `NLM_USE_MINC` | `ON` | Look for libminc and build MINC support if it is found. |
| `NLM_USE_SYSTEM_ZLIB` | `OFF` | Use an installed ZLIB. |
| `NLM_USE_SYSTEM_GSL` | `OFF` | Use an installed GSL. |
| `NLM_USE_SYSTEM_FFTW3F` | `OFF` | Use an installed single-precision FFTW3. |
| `NLM_USE_SYSTEM_NIFTI` | `OFF` | Use an installed nifticlib. |

Standalone, MINC support is optional. Without libminc — `NLM_USE_MINC=OFF`, or
simply no libminc to find — the programs are built NIfTI-only and say so when
handed a `.mnc` file; everything else, the library included, is unaffected. To
build against an installed libminc:

```sh
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DLIBMINC_DIR=/opt/minc/1.9.19/lib/cmake \
  -DNLM_USE_SYSTEM_GSL=ON -DNLM_USE_SYSTEM_FFTW3F=ON \
  -DNLM_USE_SYSTEM_ZLIB=ON -DNLM_USE_SYSTEM_NIFTI=ON
```

A standalone checkout with no sibling `libminc/` uses the copies of
`minc_io_simple_volume.h` and friends under `thirdparty/ezminc/`. Inside the
superbuild the real `libminc/ezminc` headers are used instead, so the two never
drift apart.

## Tests

Four CTest tests on deterministic synthetic phantoms — no reference files. The
first three are in-process unit tests with no CLI parsing and no file I/O; they
link the `nlm` library rather than recompiling the sources, so they exercise
exactly the objects a consumer gets.

| Test | What it checks |
| --- | --- |
| `nlm_denoise_psnr` | `denoise_mt` on a phantom with added Gaussian noise gains at least 3 dB PSNR and reaches 28 dB. |
| `anlm_denoise_psnr` | The same bar for `anlm_proc`. |
| `noise_estimate_sigma` | `minc::noise_estimate` recovers a known sigma to within 10%. |
| `nlm_minc_cli` | End to end: writes a phantom to a `.mnc`, runs the `mincnlm` binary on it, and checks that the dimensions and steps survived and that the PSNR cleared the same bar. Only test that runs a program or touches files; built only when `NLM_HAVE_MINC`, so it is absent from a NIfTI-only build. |

```sh
cd build && ctest                      # standalone
ctest -R "nlm_denoise_psnr|anlm_denoise_psnr|noise_estimate_sigma|nlm_minc_cli"
```

## Source layout

```
CMakeLists.txt          dependency resolution; standalone vs superbuild
cmake-modules/          Build*/Find* modules for the standalone dependencies
src/
  mincnlm.cpp           classic NL-means driver (ParseArgv / getopt)
  minc_anlm.cpp         adaptive NL-means driver
  noise_estimate_tool.cpp   noise/SNR reporting driver
  nlm_denoise.{h,cpp}   library entry points; the pipeline behind one call
  nlm_denoise_volume.h  simple_volume<T> overloads of the above
  nlm_globals.{h,cpp}   legacy process-wide state the algorithms read
  nl_means.{h,cpp}      voxel-wise NL-means
  nl_means_block.{h,cpp}    block-wise NL-means
  nl_means_utils.{h,cpp}    preprocessing and scaling helpers
  anlm_proc.{h,cpp}     adaptive NL-means
  noise_estimate.{h,cpp}    wavelet/MAD noise estimator
  minc_histograms.{h,cpp}   histograms, k-means, distribution distances
  fftw_blur.{h,cpp}     FFTW3F blurring and gradients
  dwt.cpp, dwt_utils.*, volume_dwt.h   3D wavelet transform
  minc_io_nifti_volume.h    NIfTI I/O for simple_volume<T>, no libminc
tests/                  the CTest tests (test_minc_cli.cpp runs the mincnlm binary)
thirdparty/ezminc/      vendored fallback copies of the pure ezminc headers
```

## Known issues and limitations

Behavioural limits, in decreasing order of how likely they are to bite:

- **`minc_anlm --mt <n>` with `n > 1` changes the result.** Its own usage text
  says so. Use one thread when reproducibility matters.
- **Automatic noise estimation pads to a cube of the next power of two.**
  `dwt_forward()` pads the volume to `2^k` on every axis, where `2^k` is derived
  from the *largest* dimension, so a 300×300×300 volume allocates 512³ floats
  (512 MB) for the transform. Passing `-sigma` explicitly avoids the estimator
  entirely.
- **3D volumes only, and a 4D NIfTI is silently truncated.** A 4D `.mnc` file is
  rejected (*"Only 3D volume is supported"* from `mincnlm`, *"Need 3D minc
  file"* from the other two). A 4D `.nii`, by contrast, loads its first 3D
  volume with no warning and is written back out as a 3D file
  (`minc_io_nifti_volume.h`, `load_nifti_simple_volume()` reads `nx*ny*nz` and
  ignores `nt`). Split 4D data into volumes first.
- **MINC and NIfTI cannot be mixed** between input and output, or between input
  and mask. Convert first with `nii2mnc`/`mnc2nii`.
- **`minc_anlm`'s second argument is documented as a prefix but used as a
  filename.** The output is written to exactly that path, while the `--debug`
  volumes append to it, producing names like `out.mnc_distance.mnc`
  (`minc_anlm.cpp:141`, `:152`). Two more names, `_variances` and `_means`, are
  computed and never used (`minc_anlm.cpp:153-154`).
- **`-hallucinate` is experimental** and undocumented beyond its option text.

Code-level problems worth fixing, none of which change current behaviour on the
supported paths:

- `nlm::denoise` is **not reentrant**: the algorithm routines read the
  process-wide globals in `nlm_globals.h`, which it writes on every call. Two
  concurrent calls from different threads of the same process interfere. It also
  leaves `block` clobbered — a caller's `block = 1` becomes `0` after a volume
  too thin for block mode.
- `nlm::denoise` **prints to stdout even when `verbose` is 0**: the
  block-to-voxel-wise fallback notice, and the whole `-aniso` parameter dump
  (`nlm_denoise.cpp:74-81`, `:109-110` and `:129-131`). A library should not
  write to a consumer's stdout unasked.
- `minc_histograms.h:84` — `histogram::operator=(const histogram&)` falls off
  the end without returning; using its value is undefined behaviour. Same at
  `:315`, where `interpolate()`'s else branch evaluates `_hist[i];` instead of
  returning it.
- `minc_histograms.h:584,588` — in `kstwo()`, `en2` is set from
  `data1.size()` rather than `data2.size()`, and the loop tests `it1` against
  `data2.end()` rather than `data1.end()`. Unused by anything here, but wrong
  for whoever calls it first.
- `mincnlm.cpp` leaks `in_hallucinate` and keeps a block of commented-out
  intensity-rescaling code with a `///@todo rescale the sigma of noise!!!`
  next to it — the scaling to 0-255 that the published parameter defaults were
  chosen for is disabled, which is why `-sigma` is in the file's own intensity
  units.
- In the no-MINC build `mincnlm` rewrites `argv[1]`/`argv[2]` from `optind`
  after parsing, rather than passing the positional arguments along; it works,
  but it is a trap for the next edit.
- There is no `LICENSE`/`COPYING` file, only the notice below and the
  per-file MNI headers.

## Copyright

Copyright (C) 2008 Pierrick Coupé — L. Collins Lab, and Vladimir S. Fonov,
McConnell Brain Imaging Centre, Montreal Neurological Institute, McGill
University.

This code is the adaptation to MINC of the original code by Pierrick Coupé and
Pierre Yger. The original implementation was protected under the licence
IDDN.FR.001.070033.000.S.P.2007.000.21000.

Individual files carry MNI permissive licence headers; see the headers
themselves for the exact terms.
