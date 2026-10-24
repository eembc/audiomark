include_directories(
    ${PORT_DIR}
    ${PORT_DIR}/src/
    ${PORT_DIR}/src/dsp/
)

add_definitions(-DUSE_RISCV_DSP)

set(PORT_SOURCE 

    ${PORT_DIR}/th_api.c

    ${PORT_DIR}/src/dsp/tables_f32.c
    ${PORT_DIR}/src/dsp/tables_radix8.c
    ${PORT_DIR}/src/dsp/cfft_f32.c
    ${PORT_DIR}/src/dsp/rfft_fast_f32.c

    ${PORT_DIR}/src/nn/avgpool_25x5x64_s8.c
    ${PORT_DIR}/src/nn/convolve_s8.c
    ${PORT_DIR}/src/nn/depthwise_conv_s8.c
    ${PORT_DIR}/src/nn/fully_connected_s8.c
    ${PORT_DIR}/src/nn/mat_mult_kernel_s8_s16.c
    ${PORT_DIR}/src/nn/q7_to_q15_with_offset.c
    ${PORT_DIR}/src/nn/softmax_row12_s8.c
    ${PORT_DIR}/src/nn/softmax_luts.c
    ${PORT_DIR}/src/nn/vec_mat_mult_t_s8.c

    ${PORT_DIR}/src/absmax_f32.c
    ${PORT_DIR}/src/add_f32.c
    ${PORT_DIR}/src/cfft_f32.c
    ${PORT_DIR}/src/cfft_init_f32.c
    ${PORT_DIR}/src/cmplx_conj_f32.c
    ${PORT_DIR}/src/cmplx_dot_prod_f32.c
    ${PORT_DIR}/src/cmplx_mag_f32.c
    ${PORT_DIR}/src/cmplx_mult_cmplx_f32.c
    ${PORT_DIR}/src/dot_prod_f32.c
    ${PORT_DIR}/src/f32_to_int16.c
    ${PORT_DIR}/src/int16_to_f32.c
    ${PORT_DIR}/src/mat_vec_mult_f32.c
    ${PORT_DIR}/src/multiply_f32.c
    ${PORT_DIR}/src/nn_classify.c
    ${PORT_DIR}/src/nn_init.c
    ${PORT_DIR}/src/offset_f32.c
    ${PORT_DIR}/src/rfft_f32.c
    ${PORT_DIR}/src/rfft_init_f32.c
    ${PORT_DIR}/src/subtract_f32.c
    ${PORT_DIR}/src/vlog_f32.c
)
