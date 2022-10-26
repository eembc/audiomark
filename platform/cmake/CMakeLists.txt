cmake_minimum_required(VERSION 3.10.2)
project(sec-tls
	DESCRIPTION "Firmware for the AudoMark Self-Hosted Benchmark"
	LANGUAGES C
	VERSION 0.0.0)

set(PRECISION "float" CACHE STRING "precision")
option(PRECISION "Set to `fixed` or `float` [def] (q15 or fp32 for now)")
message("Precision set to ${PRECISION}")

set(CMAKE_C_FLAGS "-pedantic -std=c99 -g -O0 -Wall")
# Perhaps we can remove these after cleaning up the SpeeX code.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-newline-eof")

include_directories(../../application_demo)
include_directories(../../libraries/DSP/Include)
include_directories(../../components/xiph/speexdsp)
include_directories(../../components/xiph/speexdsp/include)
include_directories(../../components/xiph/speexdsp/include/speex)
include_directories(../../components/xiph/speexdsp/libspeexdsp)

# Note on configuration of LibSpeeX:
# We can configure with the file `components/speexdsp/libspeexdsp/config.h`
# by setting `-DHAVE_CONFIG_H`, but this might lead to a conflict with EEMBC
# configuration files. For now, let's isolate all configuration to the build
# tool and then decide what TODO how to handle a global `config.h` file. Here
# are two settings taken from that `config.h` which set the FFT to CMSIS and
# uses fixed-point (see `fftwrap.c` for the excitement).

if (PRECISION MATCHES "fixed")
	add_definitions(-DFIXED_POINT)
	add_definitions(-DUSE_CMSIS_DSP)
elseif(PRECISION MATCHES "float")
	add_definitions(-DFLOATING_POINT)
	add_definitions(-DUSE_SMALLFT)
else()
	message(FATAL_ERROR "`PRECISION` must be `fixed` or `float`")
endif()

# This macro is used extensively in LibSpeeX for function visibility.
add_definitions(-DEXPORT=)

set(SOURCE
	../../application_demo/main.c
	../../application_demo/audiomark.c

	../../components/arm/audio_beamformer/src/arm_beamformer_f32.c
	../../components/arm/audio_beamformer/src/arm_beamformer_process_f32.c
	../../components/arm/audio_beamformer/src/arm_beamformer_tables_f32.c

	../../components/xiph/echo_canceller/src/xiph_aec.c

	../../components/xiph/preprocess/src/xiph_anr.c

	../../components/xiph/speexdsp/libspeexdsp/arm_libspeex_kernels.c
	../../components/xiph/speexdsp/libspeexdsp/buffer.c
	../../components/xiph/speexdsp/libspeexdsp/fftwrap.c
	../../components/xiph/speexdsp/libspeexdsp/filterbank.c
	../../components/xiph/speexdsp/libspeexdsp/jitter.c
	../../components/xiph/speexdsp/libspeexdsp/kiss_fft.c
	../../components/xiph/speexdsp/libspeexdsp/kiss_fftr.c
	../../components/xiph/speexdsp/libspeexdsp/mdf.c
	../../components/xiph/speexdsp/libspeexdsp/preprocess.c
	../../components/xiph/speexdsp/libspeexdsp/resample.c
	../../components/xiph/speexdsp/libspeexdsp/scal.c
	../../components/xiph/speexdsp/libspeexdsp/smallft.c
	../../libraries/DSP/Source/CommonTables/CommonTables.c
	../../libraries/DSP/Source/BasicMathFunctions/BasicMathFunctions.c
	../../libraries/DSP/Source/FastMathFunctions/FastMathFunctions.c
	../../libraries/DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c

	# For new beamformer
	# TODO: FFT here and in Xiph, reduce to one?
	../../libraries/DSP/Source/TransformFunctions/TransformFunctions.c
	../../libraries/DSP/Source/MatrixFunctions/MatrixFunctions.c
	../../libraries/DSP/Source/StatisticsFunctions/StatisticsFunctions.c
	../../libraries/DSP/Source/SupportFunctions/arm_q15_to_float.c
	../../libraries/DSP/Source/SupportFunctions/arm_float_to_q15.c

)

add_executable(audiomark ${SOURCE})
target_link_libraries(audiomark m)
