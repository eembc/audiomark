# Introduction

AudioMark™ is a benchmark which models a sophisticated, real-world audio 
pipeline that uses a neural net for keyword spotting. EEMBC developed this 
benchmark in response to the massive proliferation of products utilizing an 
audio controlled Human-Machine Interface (HMI) that rely on such a pipeline. 
This includes everything from personal assistants like Alexa, to white-box 
appliances like washers, dryers, and refrigerators, to home entertainment 
systems, and even cars that respond to voice commands.

# Theory of operation

The benchmark works by processing two microphone inputs listening to both a 
speaker and reflected noise. A state-of-the-art adaptive beamformer determines 
the direction of arrival of the primary speaker. This augmented signal is then 
treated for echo cancellation and noise reduction. The cleaned signal is sent 
to an MFCC feature extractor which feeds a neural net that spots one of ten 
known keywords.

The benchmark API facilitates hardware acceleration for key DSP and NN 
functionality. The file `ee_api.h` describes the functions that the system 
integrator must implement. The components were derived from several sources:

* The beamformer and direction of arrival algorithms (BF+DOA) were written and tested by Arm and Infineon.
* The acoustic echo canceller (AEC) and audio noise reduction (ANR) elements are implemented by the SpeeX libspeexdsp library. These functions utilize the SpeeX API, which is a combination of macros and functions that perform floating-point/fixed math operations, and an FFT wrapper for transformation. In AudioMark, only the single precision floating-point version of the library is used.
* The neural net was derived from the [Arm Model Zoo DS CNN KWS](https://github.com/ARM-software/ML-zoo/tree/master/models/keyword_spotting/ds_cnn_small/model_package_tf/model_archive/TFLite/tflite_int8).

This DS-CNN model was chosen for its flexibility of utilizing available hardware. It means the benchmark 
scales across a wide variety of MCUs and SoCs.

When possible, the components are implemented in 32-bit floating point, with the 
exception of the neural net, which is signed 8-bit integer. The data that flows 
in between components is signed 16-bit integer.

<img width="745" alt="image" src="https://user-images.githubusercontent.com/8249735/207705676-966fe230-8eac-4250-a468-437dc4ceebcd.png">



# Building

Typically this benchmark would be built within a product's specific 
environment, using their own SDK, BSP and methodologies. Given the diversity of 
build environments, EEMBC instead provides a simpler self-hosted `main.c` and 
an implementation using Arm's *platform-agnostic* CMSIS functions to quickly 
examine the functionality of the benchmark on an OS that supports `cmake`. 
Ideally the target platform would use its own DSP and neural-net acceleration APIs.

## Linux and macOS

To build the self-hosted example, from the root directory type":

~~~
% mkdir build && cd build
% cmake .. -DPORT_DIR=ports/arm
% make test
% make audiomark
% ./audiomark
~~~

This will run the benchmark for at least 10 seconds and produce a score.

## Windows

x64 Native Tools Command Prompt for VS2019 `cmake` for windows will create a solution (`audiomark.sln`) file which 
can be opened and compiled within Visual Studio.

# Porting

AudioMark has two port layers, one for the EEMBC code, and one for the SpeeX 
DSP library. Both of these must be adjusted for the target platform, and both
layers differ significantly. They are orthogonal, meaning the EEMBC layer is
not above or below the SpeeX layer.

The SpeeX layer contains multiple levels of abstraction:

**The ISA level**: These are macros that perform simple math functions that
could potentially be swapped for a CPU instruction, like `MULT16_32_P15`.
These macros have generic definitions in the file `fixed_generic.h`.

**The DSP library level**: These are typically more advanced functions that
can be swapped out rather than optimizing at the ISA level. For example,
the FFT is abstracted out entirely. See the `fftwrap.c` file for multiple
examples of how an FFT can be instantiated.

**The application level**: One step above the DSP level we have functions
that aren't commonly found in libraries, such as `update_gains_critical_bands`.
There are dozens of optimization options at this level which are described
below.

The EEMBC layer only focuses on the DSP-library level, with the exception
of the neural-net intitialization and inference functions. As a result,
there are far fewer function calls to consider with the EEMBC layer.

**Word of warning**

With all of these options, it is possible to accidentally (or intentionally)
create a port that runs faster at the expense of quality, thus skewing
comparisons. However, for a score to be considered valid, it must pass the 
unit tests. There are five unit tests in total:

- Digital Signal Processing (DSP) tests
  - test_abf (tests/test_abf_f32.c)
  - test_anr (tests/test_anr_f32.c)
  - test_aec (tests/test_aec_f32.c)
  - test_mfcc (tests/test_mfcc_f32.c)
- Neural-network (NN) test
  - test_kws (tests/test_kws.c)  

The DSP unit tests permit at most 50 dB of SNR (Signal-to-Noise 
ratio), a failure of a unit test means the optimizations have gone too far 
to be considered a fair comparison. The KWS unit test use a 35dB ratio for
the comparison but only when a valid data is present.

Note: The actual test codes use Noise-to-Signal ratio instead of 
Signal-to-Noise. SNR has the disadvantage that its value becoming infinity when the results 
are bit-exact match to reference data (i.e. noise level is 0).

## EEMBC port layer

The EEMBC port layer is contained in two files: `th_types.h` and `th_api.c`.
An unimplemented empty set of files is provided in `ports/barebones`, and a 
reference can be found in `ports/arm_cmsis`.

The `th_types.h` file defines the floating-point type, as well as 2D matrix 
object type, and both real and complex FFT object types.
 
The `th_api.c` file contains the definition of the following functions:

### Standard library overrides

* th_malloc
* th_free
* th_memcpy
* th_memmove
* th_memset

The memory allocation and free functions depend on the memory types of the
platform, and how the system integrator wishes to place data in memory. See
the section on the *Memory Model* in this document for more details. The top
of the `th_api.c` file defines all of the static memory buffers required.

### FFT functions

* th_cfft_init_f32
* th_cfft_f32
* th_rfft_init_f32
* th_rfft_f32

These functions initialize an FFT type variable and perform complex and real
FFTs.

### Fundamental math functions

* th_absmax_f32
* th_int16_to_f32
* th_f32_to_int16
* th_cmplx_mult_cmplx_f32
* th_cmplx_conj_f32
* th_cmplx_dot_prod_f32
* th_cmplx_mag_f32
* th_add_f32
* th_subtract_f32
* th_multiply_f32
* th_dot_prod_f32
* th_offset_f32
* th_vlog_f32
* th_mat_vec_mult_f32

Throughtout the EEMBC code, these functions perform the heavy-lifting.

### Neural-net functions

* th_nn_init
* th_nn_classify

The neural-net functions are a bit different than the DSP functions. Where a
math function like `add` is straightforward, the initialization and invocation
of the inference is extremely hardware dependent.

To provide maximum flexibility, the neural net topology, layers, weights, and
biases are all frozen and cannot be modified. Instead, the system integrator
must construct the neural net from the definitions in `src/ee_nn.h` and
`src/ee_nn_tables.c`.

## LibSpeexDSP optimizations

The AEC and ANR AudioMark components which are part of the LibSpeexDSP, can be 
enhanced with architecture specific routines, taking advantage of the 
underlying CPU capabilities. There are three types of optimizations available:

1. FFT - FFT can already be replaced with optimized variants thanks to the 
existing FFT wrapper (`lib/speexdsp/libspeexdsp/fftwrap.c`) and some parts of 
the resampler already have SIMD support for ARM Neon and Intel SSE. Additional 
FFT operations can be added here.

2. Intrinsic primitives - These are simple macros that come with several 
variants (here is the generic implementation of a multiplication: 
[`MULT16_16`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/fixed_generic.h#L77)
and may be replaced as needed. SpeeX comes with several compiler-time selected 
options stored in the files `fixed_*.c`.

3. Override path - This method uses define macros to override individual 
functions and is described below.

For the rest of the software, and given the monolithic nature of the 
LibSpeexDSP structure, customization of intensive parts could be achieved by 
defining compiler conditions that would override some of the inner loops with 
optimized routines that could potentially be vectorized or 
hardware-accelerated. This is similar to what was implemented for the TriMedia 
porting (referring the `lib/speexdsp/libspeexdsp/tmv` folder)

By default, none of these override compiler directives are defined, causing the 
LibSpeexDSP library to act with its vanilla behaviour. However, the system 
integrator is free to define all or part of these compiler conditional options. 
The decision on which of these should be set active is largely architecture and 
compiler dependant. Simple loop structures are typically properly handled by 
recent compilers which support vectorization for SIMD targets, but more complex 
ones could require access to an external library like CMSIS DSP, hand 
optimization through C with intrinsic or even assembly to reach peak 
performance.

As a first example, the AEC power_spectrum routine, which is essentially 
computing the squared magnitude of a complex signal, could use the CMSIS-DSP 
`arm_cmplx_mag_squared_f32` function and for this defining the 
`OVERRIDE_MDF_POWER_SPECTRUM` would deactivate original definition and use the 
optimized variant that will be placed in the 
lib/speexdsp/libspeexdsp/mdf_opt_helium.c and defined the following way:

Here is the generic example in [`mdf_opt_generic.c`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/mdf_opt_generic.c#L84-L94):

```C
static void power_spectrum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    int             i, j;
    ps[0] = MULT16_16(X[0], X[0]);
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        ps[j] = MULT16_16(X[i], X[i]) + MULT16_16(X[i + 1], X[i + 1]);
    }
    ps[j] = MULT16_16(X[i], X[i]);
}
```

While it is possible to gain performance improvement by overriding [`MULT16_16`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/fixed_generic.h#L77), a better optimization could be
attained by overriding the function. Here is an example from [`mdf_opt_helium.c`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/preprocess_opt_helium.c#L172-L180):

```C
void power_spectrum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    ps[0] = MULT16_16(X[0], X[0]);
    arm_cmplx_mag_squared_f32(&X[1], ps + 1, N - 1);
}
```

For this one, most of the recent compilers will be able to vectorize the native implementation. Visual inspection
of the generated assembly would determine whether it is worth having a specific optimized variant.

A second example would be the ANR final stage overlap-add with 16-bit integer conversion. For this one, there is no native library equivalent.
A customized variant with SIMD C intrinsic will allow to take advantage of SIMD if available.

As before, here is the generic implementation from [`preprocess_opt_generic.c`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/preprocess_opt_generic.c#L64-L73):

```C
static void vect_ola(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_int16_t * pDst, uint32_t blockSize)
{
    int             i;
    
    for (i = 0; i < blockSize; i++)
        pDst[i] = WORD2INT(ADD32(EXTEND32(pSrcA[i]), EXTEND32(pSrcB[i])));
}
```

And here would be the ARM with Helium intrinsics version proposal found in [`preprocess_opt_helium.c`](https://github.com/eembc/audiomark-dev/blob/e0fd95e10d5ce6fd724b525fac998327b4f0dd8f/lib/speexdsp/libspeexdsp/preprocess_opt_helium.c#L110-L127):

```C
void vect_ola(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_int16_t * pDst, uint32_t blockSize)
{
    int16x8_t converted = vdupq_n_s16(0);
    
    for (int i = 0; i < blockSize; i += 4) {
        float32x4_t vtmp = vaddq(vld1q(pSrcA), vld1q(pSrcB));
        /* rounding, saturation and 16-bit narrowing with saturation */
        converted = vqmovnbq(vuninitializedq(converted), vcvtaq_s32_f32(vtmp));
        vstrhq_s32(pDst, converted);
        pDst += 4;         
        pSrcA += 4;         
        pSrcB += 4;
    }
}
```

All override compiler defines and associated C routines prototypes will be 
listed below. These are divided into 3 categories, associated to:

* Echo canceller core (`lib/speexdsp/libspeexdsp/mdf.c`),
* Noise suppressor core (`lib/speexdsp/libspeexdsp/preprocess.c`)
* Associated filter banks, which are noise suppressor subparts for 
psychoacoustic analysis (`lib/speexdsp/libspeexdsp/filterbank.c`) 

Code behaviour for these different C routines is respectively provided in:

* `lib/speexdsp/libspeexdsp/mfd_opt_generic.c`
* `lib/speexdsp/libspeexdsp/preprocess_opt_generic.c`
* `lib/speexdsp/libspeexdsp/filterbank_opt_generic.c`

These are provided as models, replicated as-is from the core LibSpeexDSP 
software parts with function embedding and must serve as reference for 
optimized software equivalent.

It is expected to pass these compiler directives inside build systems like 
cmake or configuration header as with the standard LibSpeexDSP `config.h`.

An example of use can be found in the ARM CMSIS build YML files 
(`platform/cmsis/speex.clayer.yml`) where most of these override defines have 
been activated.

```yaml
      misc:
        - C:
          - -DFLOATING_POINT
          - -DEXPORT=
          - -DOS_SUPPORT_CUSTOM
          # speex boosted routines
          # FilterBank
          - -DOVERRIDE_FB_COMPUTE_BANK32
          - -DOVERRIDE_FB_COMPUTE_PSD16
          # ANR
          - -DOVERRIDE_ANR_VEC_MUL
          - -DOVERRIDE_ANR_UPDATE_NOISE_ESTIMATE
          ...
```

### Complete list of overrides, functions, and behavior.

| Override | Function | Beahvior |
| -------- | -------- | ------------------------ |
| OVERRIDE_MDF_ADJUST_PROP | mdf_adjust_prop | Computes filter adaptation rate, proportional to inverse of weight filter energy. |
| OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC | mdf_non_adapt_learning_rate_calc | Part of the process of the computing the adaption rate when filter is not yet adapted enough. This routine divides the adaptation rate by Far End power over the whole subframe. |
| OVERRIDE_MDF_DC_NOTCH | filter_dc_notch16 | Notch filter with strided spx_int16_t (int16_t) type input and spx_word16_t (floating point) output. |
| OVERRIDE_MDF_DEEMPH | mdf_deemph | Compute error signal, check input saturation and convert / saturate strided output to spx_int16_t (int16_t). |
| OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR | void filtered_spectra_cross_corr | Compute filtered spectra and (cross-)correlations. |
| OVERRIDE_MDF_INNER_PROD | mdf_inner_prod | Dot-product. Was already provided as a stand-alone function in the original software. |
| OVERRIDE_MDF_NORM_LEARN_RATE_CALC | mdf_nominal_learning_rate_calc | Normal learning rate calculation once we're past the minimal adaptation phase. |
| OVERRIDE_MDF_POWER_SPECTRUM | power_spectrum | Compute power spectrum of a complex vector. |
| OVERRIDE_MDF_POWER_SPECTRUM_ACCUM | power_spectrum_accum | Same as power_spectrum above, with extra accumulation. |
| OVERRIDE_MDF_PREEMPH_FLT | mdf_preemph | Copy spx_word16_(int16_t) input data to buffer and apply pre-emphasis filter. |
| OVERRIDE_MDF_SMOOTHED_ADD | smoothed_add | Blend error and echo residual to apply a smooth transition to avoid introducing blocking artifacts. |
| OVERRIDE_MDF_SMOOTH_FE_NRG | smooth_fe_nrg | Smooth far end energy estimate over time. |
| OVERRIDE_MDF_SPECTRAL_MUL_ACCUM | spectral_mul_accum | Compute cross-power spectrum of a complex vectors and accumulate. Only relevant for fixed-point as mixes spx_word16_t and spx_word32_t. Floating point version, used in Audiomark, has only plain floating point  versions and does not distinguish with spectral_mul_accum16. |
| OVERRIDE_MDF_SPECTRAL_MUL_ACCUM16 | spectral_mul_accum16 | Same as spectral_mul_accum above but plain spx_word16_t format. Floating point version, used in Audiomark, has only plain floating point versions and does not distinguish with spectral_mul_accum. |
| OVERRIDE_MDF_STRIDED_PREEMPH_FLT | mdf_preemph_with_stride_int | Strided spx_int16_t (int16_t) pre-emphasis filter with saturation check. |
| OVERRIDE_MDF_VEC_ADD | vect_add | spx_word16_t (Floating point) vector addition. |
| OVERRIDE_MDF_VEC_CLEAR | vect_clear | spx_word16_t (Floating point) vector clear. |
| OVERRIDE_MDF_VEC_COPY | vect_copy | spx_word16_t (Floating point) vector copy. |
| OVERRIDE_MDF_VEC_MULT | vect_mult | spx_word16_t (Floating point) vector multiplication for windowing. |
| OVERRIDE_MDF_VEC_SCALE | vect_scale | spx_word16_t (Floating point) vector scaling. |
| OVERRIDE_MDF_VEC_SUB | vect_sub | spx_word16_t (Floating point) vector subtraction. |
| OVERRIDE_MDF_VEC_SUB_INT16 | vect_sub16 | spx_int16_t (16-bit signed integer point) vector subtraction for filtered echo computation, difference of AEC input and output subframes. |
| OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ | weighted_spectral_mul_conj | Compute weighted cross-power spectrum of a complex vector with conjugate. |
| OVERRIDE_ANR_APOSTERIORI_SNR | aposteriori_snr | Compute A-posteriori / A-priori SNRs. |
| OVERRIDE_ANR_APPLY_SPEC_GAIN | apply_spectral_gain | Apply computed spectral gain. |
| OVERRIDE_ANR_COMPUTE_GAIN_FLOOR | compute_gain_floor | Compute the gain floor based on different floors for the background noise and residual echo. |
| OVERRIDE_ANR_HYPERGEOM_GAIN | hypergeom_gain | compute hypergeometric function. |
| OVERRIDE_ANR_OLA | vect_ola | spx_word16_t vector overlap and add. |
| OVERRIDE_ANR_POWER_SPECTRUM | power_spectrum | Complex magnitude squared of a spx_word16_t (floating-point) vector. |
| OVERRIDE_ANR_QCURVE | qcurve | Compute 1 / (1 + 0.15 / (SNR_SCALING_1 * x)) |
| OVERRIDE_ANR_UPDATE_GAINS_CRITICAL_BANDS | update_gains_critical_bands | Update gains in critical bands (MEL scale). |
| OVERRIDE_ANR_UPDATE_GAINS_LINEAR | update_gains_linear | Update gains in linear spectral bands. |
| OVERRIDE_ANR_UPDATE_NOISE_ESTIMATE | update_noise_estimate | Update noise estimates. |
| OVERRIDE_ANR_UPDATE_NOISE_PROB | update_noise_prob | Update noise probabilities and smoothed power spectrum. |
| OVERRIDE_ANR_UPDATE_ZETA | preprocess_update_zeta | Update Smoothed a priori SNR. |
| OVERRIDE_ANR_VEC_CONV_FROM_INT16 | vect_conv_from_int16 | Convert spx_int16_t (16-bit signed integer) vector to spx_word16_t (floating-point). |
| OVERRIDE_ANR_VEC_COPY | vect_copy* | Generic vector copy. |
| OVERRIDE_ANR_VEC_MUL | vect_mult* | `spx_word16_t` vector multiplication for windowing. |
| OVERRIDE_FB_COMPUTE_BANK32 | filterbank_compute_bank32 | Convert linear power spectrum in MEL perceptual scale. |
| OVERRIDE_FB_COMPUTE_PSD16 | filterbank_compute_psd16 | Compute the linear power spectral density from MEL perceptual scale. |

\* It can be noted that ANR vector multiplications and vector copy routines are similar to AEC ones and can be shared.

# Memory model

There are four types of memory required for the benchmark: input audio buffers,
pre-defined inter-component buffers, constant tables, and component-specific
scractch memory requests.

## Input audio buffers

Three channels of input audio are provided: left, right, and noise. There are
roughly 1.69 seconds of audio in these buffers. A fourth buffer, `for_asr` is
used for propagate the AEC output.

## Inter-component buffers

Each component connects to the other components or inputs via one or more
buffers. These statically allocated buffers' storage is defined in `th_api.c`
to allow the system integrator to better control placement.

## Table constants

All files with the name `*_tables.c` define arrays that are referenced via
extern from their respective components. These array variables have been
stored in their own source files to facilitate linker placement optimization.

The adaptive beamformer, MFCC feature extractor, and neural net all have
several large tables of constants.

## Dynamic allocation for scratch memory

Each component needs a certain amount of scratch memory. The components are
written in such a way that they are first queried to indicate how much
memory they require, and then that is allocated by the framework and provided
via buffers during reset and run. This has been abstracted down to the 
`th_malloc` function. The parameters to this function are the number of
required bytes and the component that is requesting it. The system integrator
can use the default STDLIB `malloc` function, or allocate their own memory
buffers to target different types of memory. **The memory is never freed so
there is no need to install a sophisticated memory manager.** Simply assigning
subsequent address pointers is sufficient (provided there is enough memory).

Both the LibSpeexDSP and EEMBC-provided components utilize this dynamic
allocation pattern.

# Coding conventions

## Formatting

EEMBC formats according to [Barr-C Embedded Standards](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf). The `.clang-format` file
in the root directory observes this. This file can be used within VSCode or
MSVC, however it isn't clear if this behaves the same as `clang-format`
version 14 (which aligns pointer stars differently).

## Functions and filenames

Traditionally, all functions and files start with either `ee_` or `th_`, where
the former notation indicates "thou shall not change" and the latter must be
changed in order to port the code (i.e., the Test Harness, hence `th_`).

Since there is so much code from Xiph that we are including, we will not change
all of their code but might have to change some (like FFT wrappers). It may
require a Run Rule to avoid this code being altered. Ideally every function
should fall into a simple `th_api` folder or collection of files so that it is
obvious what needs to be ported. Currently the `components/eembc` folder
illustrates this by separating all of the Arm-specific code into `th_api.c`.

# Run rules

1. The minimum resolution of the system timer used for performance measurement shall be one (1) kHz.

2. The minimum runtime shall be 10 seconds iterating on the `audiomark_run()` function. At least 10 iterations must complete in this 10 seconds, otherwise the runtime shall be whatever achieves 10 iterations. 10 seconds of runtime with a 1 ms resolution is effectively 0.01% measurement resolution.

3. Only functions and files starting with `th_*` may be altered. In the case of the `th_api`, this is required. The one exception to this is the SpeeX DSP library code, where the user may modify the `wrapfft.c` to install the optimal FFT, change the override macros and provide their own associated functionality, or modify the primitive DSP math macros. All of the EEMBC provided `ee_` functions ultimately perform DSP computation through a subset of functions declared in the `ee_api.h` header, and implemented however the system integrator choses (the example puts all of the implementation reference code in the `th_api.c` file).

4. For a score to be considered valid, it must pass the AEC, ANR, ABF, MFCC and KWS regression tests found in the `tests/` directory. The AEC, ANR, MFCC and ABF tests utilize a SNR ratio check of -50 dB over 62.5 ms frames of data. The KWS expects the softmax output to match the Top-1 prediction for each inference, and not the actual probability. This allows for flexibility in optimizing the API functions which may not be bit-exact, but still achieve roughly the same fidelity.

5. All processing must be carried out on the platform locally and not sent to the cloud. The definition of a device includes single chip silicon, and multi-die modules, and multi-chip platforms. External memories are allowed.

6. The AudioMark score shall be computed as "iterations per second * 1000 / 1.5". AudioMark/MHz is simply AudioMark divided by the highest core frequency in MHz. For example, if the benchmark is running on Core A at 100 Mhz and Core A uses a DSP peripheral running at 300 MHz, the highest frequency is used in the computation; in this case, 300 MHz. See footnote #1 below.


7. The benchmark score shall be obtained with the serial computation of the following components--ABF, AEC, ANR, KWS--with execution proceeding from one component to another, on completion. The benchmark shall not be altered or implemented in any way that causes any of the components to execute in parallel.

8. Running multiple processes of AudioMark on a platform is allowed. For example, a dual core product could run one instance of AudioMark (`time_audiomark_run()`) on each core  For N cores running the AudioMark concurrently:

```
    Define: AM(n) is defined in Rule #6. measured per core n
    Define: f(n) is the clock frequency of core n
    AudioMark (AM) = Sum {AM(n): n = 1 to N}
    AudioMark/MHz (AM/MHz) = AM divided by max {f(n): n = 1 to N}
```

**Footnote #1: Explanation of scoring equation**

First, the 1000 factor is introduced to scale the score into a preferred integer range, this is a common EEMBC technique to avoid comparing small fractional numbers. Second, a scaling ratio is added, which was originally intended to compensate for the ratio between the sampling rate and the number of samples being processed in each iteration. The benchmark assumes a pipeline operating on 16 kHz audio input, and the idea is that a platform that is operating efficiently would score 1.0 (with no scalar): it is running exactly at the speed needed, no faster, no slower. A platform with half the performance would measure 0.5 iterations per second. However, the benchmark input dataset is actually 24000 samples worth of data not 16000, so if one iteration completes in one second, the benchmark has in reality performed better than the 16 kHz design goal. To adjust for this, the score should have been multiplied by 1.5. (Note: The benchmark could reduce the number of samples to one second (or 16k samples), however, the lead-in silence is needed to stabilize the ANR, and the keyword utterance spills over the one-second mark of the sample.). However, due to an error a divisor of 1.5 was used in the released code. To avoid confusion for people that are already using AudioMark in their projects, we have decided to keep the current code.

# Submitting scores

1. A score must be submitted to the website before it can be used in any external publication such as: academic journals, technical papers, and marketing assets. An unsubmitted score may only be discussed internally or with a 3rd party under NDA (Non-Disclosure Agreement), but not presented to the public in any way. Note: the score may be submitted but not go live (i.e., visible on the web) until a certain date agreed to between the submitter and EEMBC. This is to account for product launch schedules.

2. A submitted score shall be from hardware that is available for purchase to any member of the general public. Scores for hardware that is yet to be announced must be marked "preliminary". This score may be superseded by a new score on the released product, or cleared by request to EEMBC after the product is launched.

3. A score collected from simulation cannot be submitted. The measured score must come from actual silicon. This includes CPU, MCU, MPU, SoC, and FPGA prototypes.

# Revision history

- v1.0.0 First release (4 Feb 2023)
- v1.0.1 (6 Feb 2023)
  - Fixes a potential bug where a multiply may goes out of bound
- v1.0.2 (6 Sept 2023)
  - Fix an issue in the KWS NN model. The reference C model was converted from the KWS NN TensorFlow Lite model, and there is a small lost of accuracy in the conversion process. After the code update, the reference C model fully match the original model.
- v1.0.3 (14 June 2024)
  - Fix incorrect use of restrict keyword https://github.com/eembc/audiomark/issues/61
  - KWS unit test switched from bit exact checking to Noise to Signal ratio checking.
  - Most of the double precision floating-point operations in SpeexLib library code replaced by single precision.
  - Changes in unit test to clarify the use of Noise-to-Signal ratio instead of Signal-to-Noise (SNR) ratio
  - Documentation update: Score calculation formula uses 1/1.5 scaling rather than 1*1.5.
  - Documentation update: Clarify that only float version of SpeexDSP is supported.
  - Documentation update: KWS unit test description (changed to use Noise-to-Signal ratio)
  - Other minor documentation improvements.

# Credits

This benchmark would not have been possible without the commitment and contributions of the working group members, and the assistance from various domain experts, including (sorted by given name):

* Ashutosh Pandey, Infineon (Technical Lead)
* Dmitry Utyansky, Synopsys
* Fabien Klein, Arm
* Felix Johnny Thomasmathibalan, Arm
* Gary Jacobson: Renesas
* Jim Ryan, onsemi
* Joseph Yiu, Arm
* Kaiping Lee, Infineon
* Laurent Le Faucheur, Arm
* Mark Wallis, STMicroelectronics
* Nagendra (GD) Gulur Dwarakanath, Texas Instruments
* Peter Torelli, EEMBC
* Rita Chattopadhyay, Intel
* Ruud Derwig, Synopsys

# Copyright and license

Copyright (C) EEMBC, All rights reserved. Please refer to LICENSE.md.
