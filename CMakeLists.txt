cmake_minimum_required(VERSION 3.10.2)
project(audiomark
	DESCRIPTION "Firmware for the AudoMark Self-Hosted Benchmark"
	LANGUAGES C
	VERSION 1.0.0)

set(CMAKE_C_FLAGS "-O2")

include_directories(lib/speexdsp)
include_directories(lib/speexdsp/include)
include_directories(lib/speexdsp/include/speex)
include_directories(lib/speexdsp/libspeexdsp)
include_directories(src)

if(DEFINED PORT_DIR)
	include(${PORT_DIR}/port.cmake)
else()
	message(FATAL_ERROR "Please define PORT_DIR, e.g. -DPORT_DIR=<path>")
endif()

# For Xiph we are using Float, never Fixed
add_definitions(-DFLOATING_POINT)
# This macro is used extensively in LibSpeeX for function visibility.
add_definitions(-DEXPORT=)
# Benchmark system integrator provides heap
add_definitions(-DOVERRIDE_SPEEX_ALLOC)
add_definitions(-DOS_SUPPORT_CUSTOM)
add_definitions(-D__RESTRICT=)

set(EE_SOURCE
	# Note: main.c is just for self-hosted testing
	main.c
	src/ee_audiomark.c

	src/ee_aec_f32.c
	src/ee_anr_f32.c
	src/ee_abf_f32.c
	src/ee_abf_f32_tables.c
	src/ee_kws.c
	src/ee_mfcc_f32.c
	src/ee_mfcc_f32_tables.c

	lib/speexdsp/libspeexdsp/buffer.c
	lib/speexdsp/libspeexdsp/fftwrap.c
	lib/speexdsp/libspeexdsp/filterbank.c
	lib/speexdsp/libspeexdsp/jitter.c
	lib/speexdsp/libspeexdsp/kiss_fft.c
	lib/speexdsp/libspeexdsp/kiss_fftr.c
	lib/speexdsp/libspeexdsp/mdf.c
	lib/speexdsp/libspeexdsp/preprocess.c
	lib/speexdsp/libspeexdsp/resample.c
	lib/speexdsp/libspeexdsp/scal.c
	lib/speexdsp/libspeexdsp/smallft.c
)

set(TH_SOURCE
	# This is essentially an include needed in th_api.c
	src/ee_nn_tables.c
	src/ee_abf_f32_tables.c

	${PORT_SOURCE}
)

set(TEST_AEC_F32_SOURCE
	src/ee_aec_f32.c
	lib/speexdsp/libspeexdsp/buffer.c
	lib/speexdsp/libspeexdsp/fftwrap.c
	lib/speexdsp/libspeexdsp/filterbank.c
	lib/speexdsp/libspeexdsp/jitter.c
	lib/speexdsp/libspeexdsp/kiss_fft.c
	lib/speexdsp/libspeexdsp/kiss_fftr.c
	lib/speexdsp/libspeexdsp/mdf.c
	lib/speexdsp/libspeexdsp/preprocess.c
	lib/speexdsp/libspeexdsp/resample.c
	lib/speexdsp/libspeexdsp/scal.c
	lib/speexdsp/libspeexdsp/smallft.c
	${TH_SOURCE}
	tests/data/aec_f32_input_source.c
	tests/data/aec_f32_input_echo.c
	tests/data/aec_f32_expected.c
	tests/test_aec_f32.c
)
add_executable(test_aec_f32 ${TEST_AEC_F32_SOURCE})

set(TEST_ANR_F32_SOURCE
	src/ee_anr_f32.c
	lib/speexdsp/libspeexdsp/buffer.c
	lib/speexdsp/libspeexdsp/fftwrap.c
	lib/speexdsp/libspeexdsp/filterbank.c
	lib/speexdsp/libspeexdsp/jitter.c
	lib/speexdsp/libspeexdsp/kiss_fft.c
	lib/speexdsp/libspeexdsp/kiss_fftr.c
	lib/speexdsp/libspeexdsp/mdf.c
	lib/speexdsp/libspeexdsp/preprocess.c
	lib/speexdsp/libspeexdsp/resample.c
	lib/speexdsp/libspeexdsp/scal.c
	lib/speexdsp/libspeexdsp/smallft.c
	${TH_SOURCE}
	tests/data/anr_f32_input.c
	tests/data/anr_f32_expected.c
	tests/test_anr_f32.c
)
add_executable(test_anr_f32 ${TEST_ANR_F32_SOURCE})


set(TEST_ABF_F32_SOURCE
	src/ee_abf_f32.c
	${TH_SOURCE}
	tests/data/abf_f32_input_ch1.c
	tests/data/abf_f32_input_ch2.c
	tests/data/abf_f32_expected.c
	tests/test_abf_f32.c
)
add_executable(test_abf_f32 ${TEST_ABF_F32_SOURCE})

set(TEST_MFCC_F32_SOURCE
	src/ee_mfcc_f32.c
	src/ee_mfcc_f32_tables.c
	${TH_SOURCE}
	tests/data/mfcc_f32_all.c
	tests/test_mfcc_f32.c
)
add_executable(test_mfcc_f32 ${TEST_MFCC_F32_SOURCE})

set(TEST_KWS_SOURCE
	src/ee_kws.c
	src/ee_mfcc_f32.c
	src/ee_mfcc_f32_tables.c
	${TH_SOURCE}
	tests/test_kws.c
	tests/data/kws_input.c
	tests/data/kws_expected.c
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
	target_link_libraries(test_kws m)
endif()

add_custom_target(test
	COMMENT "Running unit tests..."
	COMMAND ./test_abf_f32
	COMMAND ./test_aec_f32
	COMMAND ./test_anr_f32
	COMMAND ./test_kws
	COMMAND ./test_mfcc_f32
	DEPENDS test_mfcc_f32 test_kws test_abf_f32 test_aec_f32 test_anr_f32
)
