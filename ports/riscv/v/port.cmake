include_directories(
    ${PORT_DIR}/src
    ${PORT_DIR}/src/nn
    ${PORT_DIR}/src/dsp/
    ${PORT_DIR}/..
    ${PORT_DIR}/../src
    ${PORT_DIR}/../src/nn
    ${PORT_DIR}/../src/dsp/
)

# probe for vector fp support - will fail for zve32x
file(WRITE ${CMAKE_BINARY_DIR}/probe_rvv_fp.c
"
#include <riscv_vector.h>
#if !defined(__riscv_v_elen_fp) || __riscv_v_elen_fp < 32
#error \"RVV floating-point support (__riscv_v_elen_fp >= 32) not available\"
#endif
int main(void) { return 0; }
")

try_compile(RISCV_HAS_VECTOR_FP
    ${CMAKE_BINARY_DIR}
    SOURCES ${CMAKE_BINARY_DIR}/probe_rvv_fp.c
    CMAKE_FLAGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    COMPILE_DEFINITIONS "${CMAKE_C_FLAGS}"
)

message(STATUS "CMAKE_C_COMPILER = '${CMAKE_C_COMPILER}'")
message(STATUS "CMAKE_C_FLAGS at probe time = '${CMAKE_C_FLAGS}'")
message(STATUS "RISCV_HAS_VECTOR_FP = ${RISCV_HAS_VECTOR_FP}")

add_definitions(-DUSE_RISCV_DSP)

if(RISCV_HAS_VECTOR_FP)
    add_compile_options(
        -includemdf_opt_config.h
        -includeanr_opt_config.h
        -includefb_opt_config.h
    )

    set(F32_SOURCES
        ${PORT_DIR}/src/dsp/cfft_f32.c
        ${PORT_DIR}/src/dsp/rfft_fast_f32.c
        ${PORT_DIR}/src/add_f32.c
        ${PORT_DIR}/src/f32_to_int16.c
        ${PORT_DIR}/src/int16_to_f32.c
        ${PORT_DIR}/src/multiply_f32.c
        ${PORT_DIR}/src/offset_f32.c
        ${PORT_DIR}/src/subtract_f32.c
    )
else()
    set(F32_SOURCES
        ${PORT_DIR}/../src/dsp/cfft_f32.c
        ${PORT_DIR}/../src/dsp/rfft_fast_f32.c
        ${PORT_DIR}/../src/add_f32.c
        ${PORT_DIR}/../src/f32_to_int16.c
        ${PORT_DIR}/../src/int16_to_f32.c
        ${PORT_DIR}/../src/multiply_f32.c
        ${PORT_DIR}/../src/offset_f32.c
        ${PORT_DIR}/../src/subtract_f32.c
    )
endif()

set(PORT_SOURCE
    ${PORT_DIR}/../th_api.c

    # dsp tables
    ${PORT_DIR}/../src/dsp/tables_f32.c
    ${PORT_DIR}/../src/dsp/tables_radix4.c
    ${PORT_DIR}/../src/dsp/tables_radix8.c

    # f32 sources
    ${F32_SOURCES}
    ${PORT_DIR}/../src/absmax_f32.c
    ${PORT_DIR}/../src/cfft_f32.c
    ${PORT_DIR}/../src/cfft_init_f32.c
    ${PORT_DIR}/../src/cmplx_conj_f32.c
    ${PORT_DIR}/../src/cmplx_dot_prod_f32.c
    ${PORT_DIR}/../src/cmplx_mag_f32.c
    ${PORT_DIR}/../src/cmplx_mult_cmplx_f32.c
    ${PORT_DIR}/../src/dot_prod_f32.c
    ${PORT_DIR}/../src/mat_vec_mult_f32.c
    ${PORT_DIR}/../src/rfft_f32.c
    ${PORT_DIR}/../src/rfft_init_f32.c
    ${PORT_DIR}/../src/vlog_f32.c

    # nn sources
    ${PORT_DIR}/src/nn/avgpool_25x5x64_s8.c
    ${PORT_DIR}/src/nn/conv0_s8.c
    ${PORT_DIR}/src/nn/conv1x1_s8.c
    ${PORT_DIR}/src/nn/depthwise_conv_s8.c
    ${PORT_DIR}/../src/nn/fully_connected_s8.c
    ${PORT_DIR}/src/nn/mat_mult_kernel_s8_s16.c
    ${PORT_DIR}/src/nn/mat_mult_kernel_s8_s8.c
    ${PORT_DIR}/src/nn/mat_mult_core_1x1_s8.c
    ${PORT_DIR}/src/nn/q7_to_q15_with_offset.c
    ${PORT_DIR}/../src/nn/softmax_row12_s8.c
    ${PORT_DIR}/../src/nn/softmax_luts.c
    ${PORT_DIR}/../src/nn/vec_mat_mult_t_s8.c
    ${PORT_DIR}/../src/nn_classify.c
    ${PORT_DIR}/../src/nn_init.c
)
