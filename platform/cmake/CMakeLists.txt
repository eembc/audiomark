cmake_minimum_required(VERSION 3.10.2)
project(audiomark
	DESCRIPTION "Firmware for the AudoMark Self-Hosted Benchmark"
	LANGUAGES C
	VERSION 0.0.0)

set(CMAKE_C_FLAGS "-pedantic -std=c99 -g -O0 -Wall")
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	# Perhaps we can remove these after cleaning up the SpeeX code.
	# These are not ignored by MSVC
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-newline-eof")
	# Current CMSIS/NN has lots of these with CMSIS/DSP
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-typedef-redefinition")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-macro-redefined")
endif()

include_directories(../../lib/speexdsp)
include_directories(../../lib/speexdsp/include)
include_directories(../../lib/speexdsp/include/speex)
include_directories(../../lib/speexdsp/libspeexdsp)
include_directories(../../src)
# Developer libraries
include_directories(../../lib/th_api/CMSIS/DSP/Include)
include_directories(../../lib/NN/Include)

# Note on configuration of LibSpeeX:
# We can configure with the file `lib/speexdsp/libspeexdsp/config.h`
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

# This is to make sure we include function decor for GCC/CLANG in CMSIS
# This might screw up arm-*-gcc toolchains though? If you see issues in
# arm_math_types.h it is probably related to this.
add_definitions(-D__GNUC_PYTHON__)

set(EE_SOURCE
	../../src/ee_main.c
	../../src/ee_audiomark.c

	../../src/ee_aec_f32.c
	../../src/ee_anr_f32.c
	../../src/ee_abf_f32.c
	../../src/ee_kws.c
	../../src/ee_mfcc_f32.c
	../../src/ee_mfcc_f32_tables.c

	../../lib/speexdsp/libspeexdsp/arm_libspeex_kernels.c
	../../lib/speexdsp/libspeexdsp/buffer.c
	../../lib/speexdsp/libspeexdsp/fftwrap.c
	../../lib/speexdsp/libspeexdsp/filterbank.c
	../../lib/speexdsp/libspeexdsp/jitter.c
	../../lib/speexdsp/libspeexdsp/kiss_fft.c
	../../lib/speexdsp/libspeexdsp/kiss_fftr.c
	../../lib/speexdsp/libspeexdsp/mdf.c
	../../lib/speexdsp/libspeexdsp/preprocess.c
	../../lib/speexdsp/libspeexdsp/resample.c
	../../lib/speexdsp/libspeexdsp/scal.c
	../../lib/speexdsp/libspeexdsp/smallft.c
)

set(TH_SOURCE
	# Separate the implementation code from the benchmark code
	../../src/th_api.c
	
	../../lib/th_api/CMSIS/DSP/Source/BasicMathFunctions/BasicMathFunctions.c
	../../lib/th_api/CMSIS/DSP/Source/CommonTables/CommonTables.c
	../../lib/th_api/CMSIS/DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c
	../../lib/th_api/CMSIS/DSP/Source/FastMathFunctions/FastMathFunctions.c
	../../lib/th_api/CMSIS/DSP/Source/MatrixFunctions/MatrixFunctions.c	
	../../lib/th_api/CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.c
	../../lib/th_api/CMSIS/DSP/Source/SupportFunctions/arm_float_to_q15.c
	../../lib/th_api/CMSIS/DSP/Source/SupportFunctions/arm_q15_to_float.c
	../../lib/th_api/CMSIS/DSP/Source/TransformFunctions/TransformFunctions.c

	../../lib/NN/Source/SoftmaxFunctions/arm_softmax_q7.c
	../../lib/NN/Source/ConvolutionFunctions/arm_convolve_1x1_HWC_q7_fast_nonsquare.c
	../../lib/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic_nonsquare.c
	../../lib/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7_nonsquare.c
	../../lib/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.c
	../../lib/NN/Source/ActivationFunctions/arm_relu_q7.c
)

set(TEST_AEC_F32_SOURCE
	../../src/ee_aec_f32.c
	../../lib/speexdsp/libspeexdsp/arm_libspeex_kernels.c
	../../lib/speexdsp/libspeexdsp/buffer.c
	../../lib/speexdsp/libspeexdsp/fftwrap.c
	../../lib/speexdsp/libspeexdsp/filterbank.c
	../../lib/speexdsp/libspeexdsp/jitter.c
	../../lib/speexdsp/libspeexdsp/kiss_fft.c
	../../lib/speexdsp/libspeexdsp/kiss_fftr.c
	../../lib/speexdsp/libspeexdsp/mdf.c
	../../lib/speexdsp/libspeexdsp/preprocess.c
	../../lib/speexdsp/libspeexdsp/resample.c
	../../lib/speexdsp/libspeexdsp/scal.c
	../../lib/speexdsp/libspeexdsp/smallft.c
	${TH_SOURCE}
	../../tests/data/aec_f32_input_source.c
	../../tests/data/aec_f32_input_echo.c
	../../tests/data/aec_f32_expected.c
	../../tests/test_aec_f32.c
)
add_executable(test_aec_f32 ${TEST_AEC_F32_SOURCE})

set(TEST_ANR_F32_SOURCE
	../../src/ee_anr_f32.c
	../../lib/speexdsp/libspeexdsp/arm_libspeex_kernels.c
	../../lib/speexdsp/libspeexdsp/buffer.c
	../../lib/speexdsp/libspeexdsp/fftwrap.c
	../../lib/speexdsp/libspeexdsp/filterbank.c
	../../lib/speexdsp/libspeexdsp/jitter.c
	../../lib/speexdsp/libspeexdsp/kiss_fft.c
	../../lib/speexdsp/libspeexdsp/kiss_fftr.c
	../../lib/speexdsp/libspeexdsp/mdf.c
	../../lib/speexdsp/libspeexdsp/preprocess.c
	../../lib/speexdsp/libspeexdsp/resample.c
	../../lib/speexdsp/libspeexdsp/scal.c
	../../lib/speexdsp/libspeexdsp/smallft.c
	${TH_SOURCE}
	../../tests/data/anr_f32_input.c
	../../tests/data/anr_f32_expected.c
	../../tests/test_anr_f32.c
)
add_executable(test_anr_f32 ${TEST_ANR_F32_SOURCE})


set(TEST_ABF_F32_SOURCE
	../../src/ee_abf_f32.c
	${TH_SOURCE}
	../../tests/data/abf_f32_input_ch1.c
	../../tests/data/abf_f32_input_ch2.c
	../../tests/data/abf_f32_expected.c
	../../tests/test_abf_f32.c
)
add_executable(test_abf_f32 ${TEST_ABF_F32_SOURCE})

set(TEST_MFCC_F32_SOURCE
	../../src/ee_mfcc_f32.c
	../../src/ee_mfcc_f32_tables.c
	${TH_SOURCE}
	../../tests/data/mfcc_f32_all.c
	../../tests/test_mfcc_f32.c
)
add_executable(test_mfcc_f32 ${TEST_MFCC_F32_SOURCE})

set(TEST_NN_SOURCE
	${TH_SOURCE}
	../../tests/test_nn.c
)
add_executable(test_nn ${TEST_NN_SOURCE})

set(TEST_KWS_SOURCE
	../../src/ee_kws.c
	../../src/ee_mfcc_f32.c
	../../src/ee_mfcc_f32_tables.c
	${TH_SOURCE}
	../../tests/test_kws.c
)
add_executable(test_kws ${TEST_KWS_SOURCE})

add_executable(audiomark ${EE_SOURCE} ${TH_SOURCE})

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
else()
	target_link_libraries(audiomark m)
	target_link_libraries(test_abf_f32 m)
	target_link_libraries(test_aec_f32 m)
	target_link_libraries(test_anr_f32 m)
	target_link_libraries(test_mfcc_f32 m)
	target_link_libraries(test_nn m)
	target_link_libraries(test_kws m)
endif()

add_custom_target(check
	COMMENT "Running unit tests..."
	COMMAND ./test_abf_f32
	COMMAND ./test_aec_f32
	COMMAND ./test_anr_f32
	COMMAND ./test_kws
	COMMAND ./test_mfcc_f32
	COMMAND ./test_nn
	DEPENDS test_mfcc_f32 test_nn test_kws test_abf_f32 test_aec_f32 test_anr_f32
)
