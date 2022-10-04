cmake_minimum_required(VERSION 3.10.2)
project(sec-tls
	DESCRIPTION "Firmware for the AudoMark Self-Hosted Benchmark"
	LANGUAGES C
	VERSION 0.0.0)

set(CMAKE_C_FLAGS "-pedantic -std=c99 -g -O0 -Wall -Wno-unused-function  -Wno-newline-eof")

include_directories(../../application_demo)
include_directories(../../libraries/DSP/Include)
include_directories(../../components/xiph/speexdsp)
include_directories(../../components/xiph/speexdsp/include)
include_directories(../../components/xiph/speexdsp/include/speex)
include_directories(../../components/xiph/speexdsp/libspeexdsp)

# Note: Changing configuration in `config.h` will require different sources
add_definitions(-DHAVE_CONFIG_H)

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
)

add_executable(audiomark ${SOURCE})
