cmake_minimum_required(VERSION 3.10.2)
project(audiomark
	DESCRIPTION "Firmware for the AudoMark Self-Hosted Benchmark"
	LANGUAGES C
	VERSION 0.0.0)

set(CMAKE_C_FLAGS "-pedantic -std=c99 -g -O0 -Wall")
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	# Perhaps we can remove these after cleaning up the SpeeX code.
	# These are not ignored by MSVC
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-newline-eof")
endif()

include_directories(../../application_demo)
include_directories(../../libraries/DSP/Include)
include_directories(../../libraries/NN/Include)
include_directories(../../components/xiph/speexdsp)
include_directories(../../components/xiph/speexdsp/include)
include_directories(../../components/xiph/speexdsp/include/speex)
include_directories(../../components/xiph/speexdsp/libspeexdsp)
include_directories(../../libraries/eembc)
include_directories(../../components/eembc)

# Note on configuration of LibSpeeX:
# We can configure with the file `components/speexdsp/libspeexdsp/config.h`
# by setting `-DHAVE_CONFIG_H`, but this might lead to a conflict with EEMBC
# configuration files. For now, let's isolate all configuration to the build
# tool and then decide what TODO how to handle a global `config.h` file. Here
# are two settings taken from that `config.h` which set the FFT to CMSIS and
# uses fixed-point (see `fftwrap.c` for the excitement).

# For Xiph we are using Float by default
add_definitions(-DFLOATING_POINT)
add_definitions(-DUSE_SMALLFT)

# This macro is used extensively in LibSpeeX for function visibility.
add_definitions(-DEXPORT=)

set(EE_SOURCE
	../../application_demo/main.c
	../../application_demo/audiomark.c

	../../components/arm/audio_beamformer/src/arm_beamformer_f32.c
	../../components/arm/audio_beamformer/src/arm_beamformer_process_f32.c
	../../components/arm/audio_beamformer/src/arm_beamformer_tables_f32.c

	../../components/xiph/echo_canceller/src/xiph_aec.c

	../../components/xiph/preprocess/src/xiph_anr.c

	../../components/eembc/ee_kws.c
	../../components/eembc/ee_mfcc.c
	../../components/eembc/ee_mfccdata.c

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

set(TH_SOURCE
	# Separate the implementation code from the benchmark code
	../../components/eembc/th_api.c
	
	../../libraries/DSP/Source/BasicMathFunctions/BasicMathFunctions.c
	../../libraries/DSP/Source/CommonTables/CommonTables.c
	../../libraries/DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c
	../../libraries/DSP/Source/FastMathFunctions/FastMathFunctions.c
	../../libraries/DSP/Source/MatrixFunctions/MatrixFunctions.c	
	../../libraries/DSP/Source/StatisticsFunctions/StatisticsFunctions.c
	../../libraries/DSP/Source/SupportFunctions/arm_float_to_q15.c
	../../libraries/DSP/Source/SupportFunctions/arm_q15_to_float.c
	../../libraries/DSP/Source/TransformFunctions/TransformFunctions.c

	../../libraries/NN/Source/SoftmaxFunctions/arm_softmax_q7.c
	../../libraries/NN/Source/ConvolutionFunctions/arm_convolve_1x1_HWC_q7_fast_nonsquare.c
	../../libraries/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic_nonsquare.c
	../../libraries/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7_nonsquare.c
	../../libraries/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.c
	../../libraries/NN/Source/ActivationFunctions/arm_relu_q7.c
)

set(TEST_MFCC_SOURCE
	../../components/eembc/ee_mfcc.c
	../../components/eembc/ee_mfccdata.c
	${TH_SOURCE}
	../../tests/test_mfcc.c
)
add_executable(test_mfcc ${TEST_MFCC_SOURCE})

set(TEST_NN_SOURCE
	${TH_SOURCE}
	../../tests/test_nn.c
)
add_executable(test_nn ${TEST_NN_SOURCE})

set(TEST_KWS_SOURCE
	../../components/eembc/ee_kws.c
	../../components/eembc/ee_mfcc.c
	../../components/eembc/ee_mfccdata.c
	${TH_SOURCE}
	../../tests/test_kws.c
)
add_executable(test_kws ${TEST_KWS_SOURCE})

add_executable(audiomark ${EE_SOURCE} ${TH_SOURCE})

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
else()
	target_link_libraries(audiomark m)
endif()

add_custom_target(check
	COMMENT "Running unit tests..."
	COMMAND ./test_mfcc
	COMMAND ./test_nn
	COMMAND ./test_kws
	DEPENDS test_mfcc test_nn test_kws
)
