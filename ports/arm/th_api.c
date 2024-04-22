/**
 * Copyright (C) 2022-2023 EEMBC
 * Copyright (C) 2022-2024 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#define restrict __restrict__
#include "ee_audiomark.h"
#include "ee_api.h"
#include "dsp/none.h"

#include "ee_nn.h"

#include "arm_nnfunctions.h"

// These are the input audio files and some scratchpad
const int16_t downlink_audio[NINPUT_SAMPLES] = {
#include "ee_data/noise.txt"
};
const int16_t left_microphone_capture[NINPUT_SAMPLES] = {
#include "ee_data/left0.txt"
};
const int16_t right_microphone_capture[NINPUT_SAMPLES] = {
#include "ee_data/right0.txt"
};
int16_t for_asr[NINPUT_SAMPLES];

// These are the inter-component buffers
int16_t audio_input[SAMPLES_PER_AUDIO_FRAME];       // 1
int16_t left_capture[SAMPLES_PER_AUDIO_FRAME];      // 2
int16_t right_capture[SAMPLES_PER_AUDIO_FRAME];     // 3
int16_t beamformer_output[SAMPLES_PER_AUDIO_FRAME]; // 4
int16_t aec_output[SAMPLES_PER_AUDIO_FRAME];        // 5
int16_t audio_fifo[AUDIO_FIFO_SAMPLES];             // 6
int8_t  mfcc_fifo[MFCC_FIFO_BYTES];                 // 7
int8_t  classes[OUT_DIM];                           // 8

void *
th_malloc(size_t size, int req)
{
    switch (req)
    {
        // The system integrator can assign working memory wherever they like
        case COMPONENT_BMF:
        case COMPONENT_AEC:
        case COMPONENT_ANR:
        case COMPONENT_KWS:
        default:
            return malloc(size);
    }
}

void
th_free(void *mem, int req)
{
    switch (req)
    {
        // The system integrator can assign working memory wherever they like
        case COMPONENT_BMF:
        case COMPONENT_AEC:
        case COMPONENT_ANR:
        case COMPONENT_KWS:
        default:
            free(mem);
    }
}

void *
th_memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    return memcpy(dst, src, n);
}

void *
th_memmove(void *restrict dst, const void *restrict src, size_t n)
{
    return memmove(dst, src, n);
}

void *
th_memset(void *b, int c, size_t len)
{
    return memset(b, c, len);
}

ee_status_t
th_cfft_init_f32(ee_cfft_f32_t *p_instance, int fft_length)
{
    arm_status status;

    if (fft_length != 128)
    {
        return EE_STATUS_ERROR;
    }
#ifdef REDUCED_FFT_TABSZ
        status = arm_cfft_init_128_f32(p_instance);
#else
        status = arm_cfft_init_f32(p_instance, 128);
#endif
    if (!status)
    {
        return EE_STATUS_ERROR;
    }
    return EE_STATUS_OK;
}

void
th_cfft_f32(ee_cfft_f32_t *p_instance,
            ee_f32_t      *p_buf,
            uint8_t        ifftFlag,
            uint8_t        bitReverseFlagR)
{
    arm_cfft_f32(p_instance, p_buf, ifftFlag, bitReverseFlagR);
}

ee_status_t
th_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length)
{
    arm_status status;

    if (fft_length == 128)
    {
#ifdef REDUCED_FFT_TABSZ
            status = arm_rfft_fast_init_128_f32(p_instance);
#else
            status = arm_rfft_fast_init_f32(p_instance, 128);
#endif
    if (!status)
    {
        return EE_STATUS_ERROR;
    }
    }
    else if (fft_length == 1024)
    {
#ifdef REDUCED_FFT_TABSZ
            status = arm_rfft_fast_init_1024_f32(p_instance);
#else
            status = arm_rfft_fast_init_f32(p_instance, 1024);
#endif
        if (!status)
        {
            return EE_STATUS_ERROR;
        }
    }
    else
    {
        return EE_STATUS_ERROR;
    }

    return EE_STATUS_OK;
}

void
th_rfft_f32(ee_rfft_f32_t *p_instance,
            ee_f32_t      *p_in,
            ee_f32_t      *p_out,
            uint8_t        ifftFlag)
{
    arm_rfft_fast_f32(p_instance, p_in, p_out, ifftFlag);
}

void
th_absmax_f32(const ee_f32_t *p_in,
              uint32_t        len,
              ee_f32_t       *p_max,
              uint32_t       *p_index)
{
    arm_absmax_f32(p_in, len, p_max, p_index);
}

void
th_cmplx_mult_cmplx_f32(const ee_f32_t *p_a,
                        const ee_f32_t *p_b,
                        ee_f32_t       *p_c,
                        uint32_t        len)
{
    arm_cmplx_mult_cmplx_f32(p_a, p_b, p_c, len);
}

void
th_cmplx_conj_f32(const ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    arm_cmplx_conj_f32(p_a, p_c, len);
}

void
th_cmplx_dot_prod_f32(const ee_f32_t *p_a,
                      const ee_f32_t *p_b,
                      uint32_t        len,
                      ee_f32_t       *p_r,
                      ee_f32_t       *p_i)
{
    arm_cmplx_dot_prod_f32(p_a, p_b, len, p_r, p_i);
}

void
th_int16_to_f32(const int16_t *p_src, ee_f32_t *p_dst, uint32_t len)
{
    arm_q15_to_float(p_src, p_dst, len);
}

void
th_f32_to_int16(const ee_f32_t *p_src, int16_t *p_dst, uint32_t len)
{
    arm_float_to_q15(p_src, p_dst, len);
}
void
th_add_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    arm_add_f32(p_a, p_b, p_c, len);
}

void
th_subtract_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    arm_sub_f32(p_a, p_b, p_c, len);
}

void
th_dot_prod_f32(ee_f32_t *p_a, ee_f32_t *p_b, uint32_t len, ee_f32_t *p_result)
{
    arm_dot_prod_f32(p_a, p_b, len, p_result);
}

void
th_multiply_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    arm_mult_f32(p_a, p_b, p_c, len);
}

void
th_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    arm_cmplx_mag_f32(p_a, p_c, len);
}

void
th_offset_f32(ee_f32_t *p_a, ee_f32_t offset, ee_f32_t *p_c, uint32_t len)
{
    arm_offset_f32(p_a, offset, p_c, len);
}

void
th_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    arm_vlog_f32(p_a, p_c, len);
}

void
th_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c)
{
    arm_matrix_instance_f32 m;
    m.numCols = p_a->numCols;
    m.numRows = p_a->numRows;
    m.pData   = p_a->pData;
    arm_mat_vec_mult_f32(&m, p_b, p_c);
}

typedef int8_t input_tensor_t[490];
typedef int8_t output_tensor_t[12];

#if !defined(TF_INTERPRETER)
extern const int32_t ds_cnn_s_layer_12_fc_bias[12];
extern const int8_t  ds_cnn_s_layer_12_fc_weights[768];
extern const int32_t ds_cnn_s_layer_1_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_1_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_1_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_1_conv2d_weights[2560];
extern const int32_t ds_cnn_s_layer_2_dw_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_2_dw_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_2_dw_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_2_dw_conv2d_weights[576];
extern const int32_t ds_cnn_s_layer_3_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_3_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_3_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_3_conv2d_weights[4096];
extern const int32_t ds_cnn_s_layer_4_dw_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_4_dw_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_4_dw_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_4_dw_conv2d_weights[576];
extern const int32_t ds_cnn_s_layer_5_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_5_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_5_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_5_conv2d_weights[4096];
extern const int32_t ds_cnn_s_layer_6_dw_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_6_dw_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_6_dw_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_6_dw_conv2d_weights[576];
extern const int32_t ds_cnn_s_layer_7_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_7_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_7_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_7_conv2d_weights[4096];
extern const int32_t ds_cnn_s_layer_8_dw_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_8_dw_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_8_dw_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_8_dw_conv2d_weights[576];
extern const int32_t ds_cnn_s_layer_9_conv2d_bias[64];
extern const int32_t ds_cnn_s_layer_9_conv2d_output_mult[64];
extern const int32_t ds_cnn_s_layer_9_conv2d_output_shift[64];
extern const int8_t  ds_cnn_s_layer_9_conv2d_weights[4096];


#define MAX_DIM_SIZE_BYTE_0 (CONV_0_OUTPUT_W * CONV_0_OUTPUT_H * CONV_0_OUT_CH)
#define MAX_DIM_SIZE_BYTE_1 \
    (DW_CONV_1_OUTPUT_H * DW_CONV_1_OUTPUT_W * DW_CONV_1_OUT_CH)

#define MAX_SIZE_BYTES                                               \
    (MAX_DIM_SIZE_BYTE_0 > MAX_DIM_SIZE_BYTE_1 ? MAX_DIM_SIZE_BYTE_0 \
                                               : MAX_DIM_SIZE_BYTE_1)

// Word aligned start addresses to prevent unalinged access.
#define MAX_NUM_WORDS_IN_OUT \
    ((MAX_DIM_SIZE_BYTE_0 + MAX_DIM_SIZE_BYTE_1 + 3) / 4)
#define IN_OUT_BUFER_0_BYTE_OFFSET (0)
#define IN_OUT_BUFER_1_BYTE_OFFSET (MAX_SIZE_BYTES + MAX_SIZE_BYTES % 4)

// TODO
static int32_t in_out_buf_main[MAX_NUM_WORDS_IN_OUT];
// TODO

/* Get size of additional buffers required by library/framework */
static int
ds_cnn_s_s8_get_buffer_size(void)
{
    /* Custom function based on knowledge that only select layers of DS_CNN_S
     * network require additional buffers. */
    int                  max_buffer = 0;
    cmsis_nn_conv_params conv_params;
    cmsis_nn_dims        input_dims;
    cmsis_nn_dims        filter_dims;
    cmsis_nn_dims        output_dims;

    // Layer 0 - Conv
    conv_params.padding.h  = CONV_0_PAD_H;
    conv_params.padding.w  = CONV_0_PAD_W;
    conv_params.stride.h   = CONV_0_STRIDE_H;
    conv_params.stride.w   = CONV_0_STRIDE_W;
    conv_params.dilation.h = CONV_0_DILATION_H;
    conv_params.dilation.w = CONV_0_DILATION_W;

    input_dims.n = CONV_0_INPUT_BATCHES;
    input_dims.h = CONV_0_INPUT_H;
    input_dims.w = CONV_0_INPUT_W;
    input_dims.c = CONV_0_IN_CH;

    filter_dims.h = CONV_0_FILTER_H;
    filter_dims.w = CONV_0_FILTER_W;

    output_dims.n = input_dims.n;
    output_dims.h = CONV_0_OUTPUT_H;
    output_dims.w = CONV_0_OUTPUT_W;
    output_dims.c = CONV_0_OUT_CH;

    int32_t size = arm_convolve_wrapper_s8_get_buffer_size(
        &conv_params, &input_dims, &filter_dims, &output_dims);

    max_buffer = size > max_buffer ? size : max_buffer;

    // Layer 0 - DW Conv
    cmsis_nn_dw_conv_params dw_conv_params;
    dw_conv_params.activation.min = DW_CONV_1_OUT_ACTIVATION_MIN;
    dw_conv_params.activation.max = DW_CONV_1_OUT_ACTIVATION_MAX;
    dw_conv_params.ch_mult        = 1;
    dw_conv_params.dilation.h     = DW_CONV_1_DILATION_H;
    dw_conv_params.dilation.w     = DW_CONV_1_DILATION_W;
    dw_conv_params.input_offset   = DW_CONV_1_INPUT_OFFSET;
    dw_conv_params.output_offset  = DW_CONV_1_OUTPUT_OFFSET;
    dw_conv_params.padding.h      = DW_CONV_1_PAD_H;
    dw_conv_params.padding.w      = DW_CONV_1_PAD_W;
    dw_conv_params.stride.h       = DW_CONV_1_STRIDE_H;
    dw_conv_params.stride.w       = DW_CONV_1_STRIDE_W;

    filter_dims.h = DW_CONV_1_FILTER_H;
    filter_dims.w = DW_CONV_1_FILTER_W;

    input_dims.n = 1;
    input_dims.h = DW_CONV_1_INPUT_H;
    input_dims.w = DW_CONV_1_INPUT_W;
    input_dims.c = DW_CONV_1_OUT_CH;

    output_dims.h = DW_CONV_1_OUTPUT_H;
    output_dims.w = DW_CONV_1_OUTPUT_W;
    output_dims.c = DW_CONV_1_OUT_CH;

    size = arm_depthwise_conv_wrapper_s8_get_buffer_size(
        &dw_conv_params, &input_dims, &filter_dims, &output_dims);

    max_buffer = size > max_buffer ? size : max_buffer;

    return max_buffer;
}

/* Test for a complete int8 DS_CNN_S keyword spotting network from
 * https://github.com/ARM-software/ML-zoo & Tag: 22.02 */
cmsis_nn_context ctx;

#else

    #if defined(ETHOSU_ARCH)
        int extern ethosu_nn_init(void);
        extern int classify_on_ethosu(const input_tensor_t in_data, output_tensor_t out_data);
    #else
        int extern cmsis_nn_init(void);
        extern int classify_on_cmsis_nn(const input_tensor_t in_data, output_tensor_t out_data);
    #endif
#endif

void
th_nn_init(void) {


#if defined(TF_INTERPRETER)
    #if defined(ETHOSU_ARCH)
        ethosu_nn_init();
    #else
        cmsis_nn_init();
    #endif
#else
    // unused const arm_cmsis_nn_status expected = ARM_CMSIS_NN_SUCCESS;
    ctx.size = ds_cnn_s_s8_get_buffer_size();

    /* N.B. The developer owns this file so they can allocate how they like. */
    ctx.buf = malloc(ctx.size);

    // we don't free in audiomark
#endif
}

ee_status_t
th_nn_classify(const input_tensor_t in_data, output_tensor_t out_data) {

#if TF_INTERPRETER
    ee_status_t status = EE_STATUS_ERROR;

    #if defined(ETHOSU_ARCH)
        status = classify_on_ethosu(in_data, out_data);
    #else
        status = classify_on_cmsis_nn(in_data, out_data);
    #endif

    return status;

#else

    int8_t *in_out_buf_0
        = (int8_t *)&in_out_buf_main[IN_OUT_BUFER_0_BYTE_OFFSET >> 2];
    int8_t *in_out_buf_1
        = (int8_t *)&in_out_buf_main[IN_OUT_BUFER_1_BYTE_OFFSET >> 2];
    // Layer 0 - Implicit reshape
    // 1x490 is interpreted as 49x10

    // Layer 1 - Conv
    cmsis_nn_conv_params              conv_params;
    cmsis_nn_per_channel_quant_params quant_params;
    cmsis_nn_dims                     in_out_dim_0;
    cmsis_nn_dims                     conv_filter_dims;
    cmsis_nn_dims                     dw_conv_filter_dims;
    cmsis_nn_dims                     in_out_dim_1;
    cmsis_nn_dims                     bias_dims;

    conv_params.padding.h     = CONV_0_PAD_H;
    conv_params.padding.w     = CONV_0_PAD_W;
    conv_params.stride.h      = CONV_0_STRIDE_H;
    conv_params.stride.w      = CONV_0_STRIDE_W;
    conv_params.dilation.h    = CONV_0_DILATION_H;
    conv_params.dilation.w    = CONV_0_DILATION_W;
    conv_params.input_offset  = CONV_0_INPUT_OFFSET;
    conv_params.output_offset = CONV_0_OUTPUT_OFFSET;
    // Not repeated subsequently as it is the same for all in this specific
    // case.
    conv_params.activation.min = -128;
    conv_params.activation.max = 127;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_1_conv2d_output_mult;
    quant_params.shift      = (int32_t *)ds_cnn_s_layer_1_conv2d_output_shift;

    in_out_dim_0.n = CONV_0_INPUT_BATCHES;
    in_out_dim_0.h = CONV_0_INPUT_H;
    in_out_dim_0.w = CONV_0_INPUT_W;
    in_out_dim_0.c = CONV_0_IN_CH;

    conv_filter_dims.h = CONV_0_FILTER_H;
    conv_filter_dims.w = CONV_0_FILTER_W;

    in_out_dim_1.n = in_out_dim_0.n;
    in_out_dim_1.h = CONV_0_OUTPUT_H;
    in_out_dim_1.w = CONV_0_OUTPUT_W;
    in_out_dim_1.c = CONV_0_OUT_CH;
    bias_dims.c    = CONV_0_OUT_CH;

    arm_cmsis_nn_status status
        = arm_convolve_wrapper_s8(&ctx,
                                  &conv_params,
                                  &quant_params,
                                  &in_out_dim_0,
                                  in_data,
                                  &conv_filter_dims,
                                  ds_cnn_s_layer_1_conv2d_weights,
                                  &bias_dims,
                                  ds_cnn_s_layer_1_conv2d_bias,
                                  &in_out_dim_1,
                                  in_out_buf_0);

    /***************************** Depthwise Separable Block 1 ***************
     */
    // Layer 1 - DW Conv
    // Common params for DW conv in subsequent layers
    cmsis_nn_dw_conv_params dw_conv_params;
    dw_conv_params.activation.min = DW_CONV_1_OUT_ACTIVATION_MIN;
    dw_conv_params.activation.max = DW_CONV_1_OUT_ACTIVATION_MAX;
    dw_conv_params.ch_mult        = 1;
    dw_conv_params.dilation.h     = DW_CONV_1_DILATION_H;
    dw_conv_params.dilation.w     = DW_CONV_1_DILATION_W;
    dw_conv_params.padding.h      = DW_CONV_1_PAD_H;
    dw_conv_params.padding.w      = DW_CONV_1_PAD_W;
    dw_conv_params.stride.h       = DW_CONV_1_STRIDE_H;
    dw_conv_params.stride.w       = DW_CONV_1_STRIDE_W;

    // Layer specific params
    dw_conv_params.input_offset  = DW_CONV_1_INPUT_OFFSET;
    dw_conv_params.output_offset = DW_CONV_1_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_2_dw_conv2d_output_mult;
    quant_params.shift = (int32_t *)ds_cnn_s_layer_2_dw_conv2d_output_shift;

    dw_conv_filter_dims.h = DW_CONV_1_FILTER_H;
    dw_conv_filter_dims.w = DW_CONV_1_FILTER_W;

    in_out_dim_0.h = DW_CONV_1_OUTPUT_H;
    in_out_dim_0.w = DW_CONV_1_OUTPUT_W;
    in_out_dim_0.c = DW_CONV_1_OUT_CH;

    // Same for all layers in DS block
    bias_dims.c = in_out_dim_0.c;

    status |= arm_depthwise_conv_wrapper_s8(&ctx,
                                            &dw_conv_params,
                                            &quant_params,
                                            &in_out_dim_1,
                                            in_out_buf_0,
                                            &dw_conv_filter_dims,
                                            ds_cnn_s_layer_2_dw_conv2d_weights,
                                            &bias_dims,
                                            ds_cnn_s_layer_2_dw_conv2d_bias,
                                            &in_out_dim_0,
                                            in_out_buf_1);

    // Layer 2 - Conv

    // Common params for Conv in rest of DS blocks
    in_out_dim_1.h     = in_out_dim_0.h;
    in_out_dim_1.w     = in_out_dim_0.w;
    in_out_dim_1.c     = in_out_dim_0.c;
    conv_filter_dims.h = CONV_2_FILTER_H;
    conv_filter_dims.w = CONV_2_FILTER_W;

    conv_params.padding.h = CONV_2_PAD_H;
    conv_params.padding.w = CONV_2_PAD_W;
    conv_params.stride.h  = CONV_2_STRIDE_H;
    conv_params.stride.w  = CONV_2_STRIDE_W;

    // Layer specific params
    conv_params.input_offset  = CONV_2_INPUT_OFFSET;
    conv_params.output_offset = CONV_2_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_3_conv2d_output_mult;
    quant_params.shift      = (int32_t *)ds_cnn_s_layer_3_conv2d_output_shift;

    status |= arm_convolve_wrapper_s8(&ctx,
                                      &conv_params,
                                      &quant_params,
                                      &in_out_dim_0,
                                      in_out_buf_1,
                                      &conv_filter_dims,
                                      ds_cnn_s_layer_3_conv2d_weights,
                                      &bias_dims,
                                      ds_cnn_s_layer_3_conv2d_bias,
                                      &in_out_dim_1,
                                      in_out_buf_0);

    /***************************** Depthwise Separable Block 2 ***************
     */
    // Layer specific
    dw_conv_params.input_offset  = DW_CONV_3_INPUT_OFFSET;
    dw_conv_params.output_offset = DW_CONV_3_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_4_dw_conv2d_output_mult;
    quant_params.shift = (int32_t *)ds_cnn_s_layer_4_dw_conv2d_output_shift;

    status |= arm_depthwise_conv_wrapper_s8(&ctx,
                                            &dw_conv_params,
                                            &quant_params,
                                            &in_out_dim_1,
                                            in_out_buf_0,
                                            &dw_conv_filter_dims,
                                            ds_cnn_s_layer_4_dw_conv2d_weights,
                                            &bias_dims,
                                            ds_cnn_s_layer_4_dw_conv2d_bias,
                                            &in_out_dim_0,
                                            in_out_buf_1);

    // Layer specific params
    conv_params.input_offset  = CONV_4_INPUT_OFFSET;
    conv_params.output_offset = CONV_4_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_5_conv2d_output_mult;
    quant_params.shift      = (int32_t *)ds_cnn_s_layer_5_conv2d_output_shift;

    status |= arm_convolve_wrapper_s8(&ctx,
                                      &conv_params,
                                      &quant_params,
                                      &in_out_dim_0,
                                      in_out_buf_1,
                                      &conv_filter_dims,
                                      ds_cnn_s_layer_5_conv2d_weights,
                                      &bias_dims,
                                      ds_cnn_s_layer_5_conv2d_bias,
                                      &in_out_dim_1,
                                      in_out_buf_0);

    /***************************** Depthwise Separable Block 3 ***************
     */
    // Layer specific
    dw_conv_params.input_offset  = DW_CONV_5_INPUT_OFFSET;
    dw_conv_params.output_offset = DW_CONV_5_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_6_dw_conv2d_output_mult;
    quant_params.shift = (int32_t *)ds_cnn_s_layer_6_dw_conv2d_output_shift;

    status |= arm_depthwise_conv_wrapper_s8(&ctx,
                                            &dw_conv_params,
                                            &quant_params,
                                            &in_out_dim_1,
                                            in_out_buf_0,
                                            &dw_conv_filter_dims,
                                            ds_cnn_s_layer_6_dw_conv2d_weights,
                                            &bias_dims,
                                            ds_cnn_s_layer_6_dw_conv2d_bias,
                                            &in_out_dim_0,
                                            in_out_buf_1);

    // Layer specific params
    conv_params.input_offset  = CONV_6_INPUT_OFFSET;
    conv_params.output_offset = CONV_6_OUTPUT_OFFSET;
    quant_params.multiplier   = (int32_t *)ds_cnn_s_layer_7_conv2d_output_mult;
    quant_params.shift        = (int32_t *)ds_cnn_s_layer_7_conv2d_output_shift;

    status |= arm_convolve_wrapper_s8(&ctx,
                                      &conv_params,
                                      &quant_params,
                                      &in_out_dim_0,
                                      in_out_buf_1,
                                      &conv_filter_dims,
                                      ds_cnn_s_layer_7_conv2d_weights,
                                      &bias_dims,
                                      ds_cnn_s_layer_7_conv2d_bias,
                                      &in_out_dim_1,
                                      in_out_buf_0);

    /***************************** Depthwise Separable Block 4 ***************
     */
    // Layer specific
    dw_conv_params.input_offset  = DW_CONV_7_INPUT_OFFSET;
    dw_conv_params.output_offset = DW_CONV_7_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_8_dw_conv2d_output_mult;
    quant_params.shift = (int32_t *)ds_cnn_s_layer_8_dw_conv2d_output_shift;

    status |= arm_depthwise_conv_wrapper_s8(&ctx,
                                            &dw_conv_params,
                                            &quant_params,
                                            &in_out_dim_1,
                                            in_out_buf_0,
                                            &dw_conv_filter_dims,
                                            ds_cnn_s_layer_8_dw_conv2d_weights,
                                            &bias_dims,
                                            ds_cnn_s_layer_8_dw_conv2d_bias,
                                            &in_out_dim_0,
                                            in_out_buf_1);

    conv_params.input_offset  = CONV_8_INPUT_OFFSET;
    conv_params.output_offset = CONV_8_OUTPUT_OFFSET;

    quant_params.multiplier = (int32_t *)ds_cnn_s_layer_9_conv2d_output_mult;
    quant_params.shift      = (int32_t *)ds_cnn_s_layer_9_conv2d_output_shift;

    status |= arm_convolve_wrapper_s8(&ctx,
                                      &conv_params,
                                      &quant_params,
                                      &in_out_dim_0,
                                      in_out_buf_1,
                                      &conv_filter_dims,
                                      ds_cnn_s_layer_9_conv2d_weights,
                                      &bias_dims,
                                      ds_cnn_s_layer_9_conv2d_bias,
                                      &in_out_dim_1,
                                      in_out_buf_0);

    /***************************** Average Pool *************** */

    cmsis_nn_pool_params pool_params;
    pool_params.activation.max = AVERAGE_POOL_9_OUT_ACTIVATION_MAX;
    pool_params.activation.min = AVERAGE_POOL_9_OUT_ACTIVATION_MIN;
    pool_params.padding.h      = AVERAGE_POOL_9_PAD_H;
    pool_params.padding.w      = AVERAGE_POOL_9_PAD_W;
    pool_params.stride.h       = AVERAGE_POOL_9_STRIDE_H;
    pool_params.stride.w       = AVERAGE_POOL_9_STRIDE_W;

    conv_filter_dims.h = AVERAGE_POOL_9_FILTER_H;
    conv_filter_dims.w = AVERAGE_POOL_9_FILTER_W;

    in_out_dim_0.n = 1;
    in_out_dim_0.h = AVERAGE_POOL_9_OUTPUT_H;
    in_out_dim_0.w = AVERAGE_POOL_9_OUTPUT_W;
    in_out_dim_0.c = in_out_dim_1.c;

    status |= arm_avgpool_s8(&ctx,
                             &pool_params,
                             &in_out_dim_1,
                             in_out_buf_0,
                             &conv_filter_dims,
                             &in_out_dim_0,
                             in_out_buf_1);

    /***************************** Fully Connected ****************/
    cmsis_nn_fc_params fc_params;
    fc_params.activation.max = FULLY_CONNECTED_11_OUT_ACTIVATION_MAX;
    fc_params.activation.min = FULLY_CONNECTED_11_OUT_ACTIVATION_MIN;
    fc_params.filter_offset  = 0;
    fc_params.input_offset   = FULLY_CONNECTED_11_INPUT_OFFSET;
    fc_params.output_offset  = FULLY_CONNECTED_11_OUTPUT_OFFSET;

    cmsis_nn_per_tensor_quant_params per_tensor_quant_params;
    per_tensor_quant_params.multiplier = FULLY_CONNECTED_11_OUTPUT_MULTIPLIER;
    per_tensor_quant_params.shift      = FULLY_CONNECTED_11_OUTPUT_SHIFT;

    in_out_dim_0.c = in_out_dim_0.c * in_out_dim_0.h * in_out_dim_0.w;
    in_out_dim_0.h = 1;
    in_out_dim_0.w = 1;

    conv_filter_dims.n = in_out_dim_0.c;
    conv_filter_dims.h = 1;
    conv_filter_dims.w = 1;
    conv_filter_dims.c = FULLY_CONNECTED_11_OUTPUT_W;

    in_out_dim_1.n = 1;
    in_out_dim_1.h = 1;
    in_out_dim_1.w = 1;
    in_out_dim_1.c = FULLY_CONNECTED_11_OUTPUT_W;

    bias_dims.c = in_out_dim_1.c;

    status |= arm_fully_connected_s8(&ctx,
                                     &fc_params,
                                     &per_tensor_quant_params,
                                     &in_out_dim_0,
                                     in_out_buf_1,
                                     &conv_filter_dims,
                                     ds_cnn_s_layer_12_fc_weights,
                                     &bias_dims,
                                     ds_cnn_s_layer_12_fc_bias,
                                     &in_out_dim_1,
                                     in_out_buf_0);
    /***************************** Softmax *************** */

    arm_softmax_s8(in_out_buf_0,
                   SOFTMAX_12_NUM_ROWS,
                   SOFTMAX_12_ROW_SIZE,
                   SOFTMAX_12_MULT,
                   SOFTMAX_12_SHIFT,
                   SOFTMAX_12_DIFF_MIN,
                   out_data);

    return status == ARM_CMSIS_NN_SUCCESS ? EE_STATUS_OK : EE_STATUS_ERROR;
#endif
}
