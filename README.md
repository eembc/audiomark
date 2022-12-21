# Introduction

AudioMark(tm) is a benchmark that models a sophisticated, real-world audio 
pipeline that uses a neural net for keyword spotting. EEMBC developed this 
benchmark in response to the massive proliferation of products utilizing an 
audio controlled Human-Machine Interface (HMI) that rely on such a pipeline. 
This includes everything from personal assistants like Alexa, to white-box 
appliances like washers, dryers, and refrigerators, to home entertainment 
systems, and even cars that respond to voice commands.

The benchmark was developed by collaboration of the following member companies:

* Arm
* Infineon
* Intel
* Synopsys
* onsemi
* STMicroelectronics
* Texas Instruments
* EEMBC

# Theory of operation

The benchmark works by processing two microphone inputs listening to both a 
speaker and reflected noise. A state-of-the-art adaptive beamformer determines 
the direction of arrival of the primary speaker. This augmented signal is then 
treated for echo cancellation and noise reduction. The cleaned signal is sent 
to an MFCC feature extractor which feeds a neural net that spots one of ten 
known keywords.

The benchmark API facilitates hardware acceleration for key DSP and NN 
functionality. The file `ee_api.h` describes the functions that the system 
integrator must implement. The acoustic echo canceller (AEC) and audio noise 
reduction (ANR) elements are implemented by the SpeeX libspeexdsp library. 
These functions utilize the SpeeX API, which is a combination of macros that 
preform fixed math operations, and an FFT wrapper for transformation.

This flexibility to utilize whatever hardware is available means the benchmark 
scales across a wide variety of MCUs and SoCs.

All of the components are implemented in 32-bit floating point, with the 
exception of the neural net, which is signed 8-bit integer. The data that flows 
in between components is signed 16-bit integer.

<img width="745" alt="image" src="https://user-images.githubusercontent.com/8249735/207705676-966fe230-8eac-4250-a468-437dc4ceebcd.png">

# Building

Typically this benchmark would be built within a product's specific 
environment, using their own SDK, BSP and methodologies. Given the diversity of 
build environments, EEMBC instead provides a simpler self-hosted `main.c` and 
an Arm implementation using CMSIS to quickly examine the functionality of the 
benchmark on an OS that supports `cmake`. Ideally the target platform would use 
its own DSP and neural-net acceleration APIs.

To build the self-hosted example, from the root directory type":

~~~
% mkdir build && cd build
% cmake .. -DPORT_DIR=arm/cmsis
% make test
% ./audiomark
~~~

This will run the benchmark for at least 10 seconds and produce a score.

# Porting

AudioMark has two port layers, one for the EEMBC code, and one for the SpeeX 
DSP library. Both of these must be adjusted for the target platform. The files 
and methodology differs, and is explained below. An unimplemented empty set of 
files is provided in `ports/barebones`, and a reference can be found in 
`ports/arm_cmsis`.

## EEMBC port layer

The EEMBC port layer is contained in one two files: `th_types.h` and `th_api.c`.

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

## SpeeX port layer

TBD

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

EEMBC formats according to Barr-C Embedded Standards. The `.clang-format` file
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

# Copyright and license

Copyright (C) EEMBC, All rights reserved. Please refer to LICENSE.md.
