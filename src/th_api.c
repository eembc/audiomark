/*
 * Copyright (C) EEMBC(R). All Rights Reserved
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */
/*
 * Copyright (C) 2022 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ee_audiomark.h"
#include "ee_api.h"
#include "ee_nn_weights.h"
#include "arm_nnfunctions.h"
#include "dsp/none.h"

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

    status = arm_cfft_init_f32(p_instance, fft_length);
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

    status = arm_rfft_fast_init_f32(p_instance, fft_length);
    if (!status)
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

void
th_softmax_i8(const int8_t *vec_in, const uint16_t dim_vec, int8_t *p_out)
{
    arm_softmax_q7(vec_in, dim_vec, p_out);
}

static const q7_t conv1_wt[CONV1_OUT_CH * CONV1_KX * CONV1_KY] = CONV1_WT;
static const q7_t conv1_bias[CONV1_OUT_CH]                     = CONV1_BIAS;
static const q7_t conv2_ds_wt[CONV1_OUT_CH * CONV2_DS_KX * CONV2_DS_KY]
    = CONV2_DS_WT;
static const q7_t conv2_ds_bias[CONV1_OUT_CH]              = CONV2_DS_BIAS;
static const q7_t conv2_pw_wt[CONV2_OUT_CH * CONV1_OUT_CH] = CONV2_PW_WT;
static const q7_t conv2_pw_bias[CONV2_OUT_CH]              = CONV2_PW_BIAS;
static const q7_t conv3_ds_wt[CONV2_OUT_CH * CONV3_DS_KX * CONV3_DS_KY]
    = CONV3_DS_WT;
static const q7_t conv3_ds_bias[CONV2_OUT_CH]              = CONV3_DS_BIAS;
static const q7_t conv3_pw_wt[CONV3_OUT_CH * CONV2_OUT_CH] = CONV3_PW_WT;
static const q7_t conv3_pw_bias[CONV3_OUT_CH]              = CONV3_PW_BIAS;
static const q7_t conv4_ds_wt[CONV3_OUT_CH * CONV4_DS_KX * CONV4_DS_KY]
    = CONV4_DS_WT;
static const q7_t conv4_ds_bias[CONV3_OUT_CH]              = CONV4_DS_BIAS;
static const q7_t conv4_pw_wt[CONV4_OUT_CH * CONV3_OUT_CH] = CONV4_PW_WT;
static const q7_t conv4_pw_bias[CONV4_OUT_CH]              = CONV4_PW_BIAS;
static const q7_t conv5_ds_wt[CONV4_OUT_CH * CONV5_DS_KX * CONV5_DS_KY]
    = CONV5_DS_WT;
static const q7_t conv5_ds_bias[CONV4_OUT_CH]              = CONV5_DS_BIAS;
static const q7_t conv5_pw_wt[CONV5_OUT_CH * CONV4_OUT_CH] = CONV5_PW_WT;
static const q7_t conv5_pw_bias[CONV5_OUT_CH]              = CONV5_PW_BIAS;
static const q7_t final_fc_wt[CONV5_OUT_CH * OUT_DIM]      = FINAL_FC_WT;
static const q7_t final_fc_bias[OUT_DIM]                   = FINAL_FC_BIAS;

// The developer owns where this memory goes, it is not part of the NODE_MEMREQ
static q7_t  scratch_pad[SCRATCH_BUFFER_SIZE];
static q7_t *col_buffer;
static q7_t *buffer1;
static q7_t *buffer2;

void
th_nn_init(void)
{
    buffer1    = scratch_pad;
    buffer2    = buffer1 + (CONV1_OUT_CH * CONV1_OUT_X * CONV1_OUT_Y);
    col_buffer = buffer2 + (CONV2_OUT_CH * CONV2_OUT_X * CONV2_OUT_Y);
}

static void
arm_avepool_q7_HWC_nonsquare(
    const q7_t    *Im_in,        // input image
    const uint16_t dim_im_in_x,  // input image dimension
    const uint16_t dim_im_in_y,  // input image dimension
    const uint16_t ch_im_in,     // number of input image channels
    const uint16_t dim_kernel_x, // window kernel size
    const uint16_t dim_kernel_y, // window kernel size
    const uint16_t padding_x,    // padding sizes
    const uint16_t padding_y,    // padding sizes
    const uint16_t stride_x,     // stride
    const uint16_t stride_y,     // stride
    const uint16_t dim_im_out_x, // output image dimension
    const uint16_t dim_im_out_y, // output image dimension
    q7_t          *bufferA,      // a buffer for local storage
    q7_t          *Im_out,       // output feature
    const uint16_t out_lshift)   // output left shift (scaling)
{
    int16_t i_ch_in, i_x, i_y;
    int16_t k_x, k_y;

    for (i_ch_in = 0; i_ch_in < ch_im_in; i_ch_in++)
    {
        for (i_y = 0; i_y < dim_im_out_y; i_y++)
        {
            for (i_x = 0; i_x < dim_im_out_x; i_x++)
            {
                int sum   = 0;
                int count = 0;
                for (k_y = i_y * stride_y - padding_y;
                     k_y < i_y * stride_y - padding_y + dim_kernel_y;
                     k_y++)
                {
                    for (k_x = i_x * stride_x - padding_x;
                         k_x < i_x * stride_x - padding_x + dim_kernel_x;
                         k_x++)
                    {
                        if (k_y >= 0 && k_x >= 0 && k_y < dim_im_in_y
                            && k_x < dim_im_in_x)
                        {
                            sum += Im_in[i_ch_in
                                         + ch_im_in
                                               * (k_x + k_y * dim_im_in_x)];
                            count++;
                        }
                    }
                }
                Im_out[i_ch_in + ch_im_in * (i_x + i_y * dim_im_out_x)]
                    = sum * (0x1 << out_lshift) / count;
            }
        }
    }
}

void
th_nn_classify(const int8_t *in_data, int8_t *out_data)
{
    // CONV1 : regular convolution
    arm_convolve_HWC_q7_basic_nonsquare((q7_t *)in_data,
                                        CONV1_IN_X,
                                        CONV1_IN_Y,
                                        1,
                                        conv1_wt,
                                        CONV1_OUT_CH,
                                        CONV1_KX,
                                        CONV1_KY,
                                        CONV1_PX,
                                        CONV1_PY,
                                        CONV1_SX,
                                        CONV1_SY,
                                        conv1_bias,
                                        CONV1_BIAS_LSHIFT,
                                        CONV1_OUT_RSHIFT,
                                        buffer1,
                                        CONV1_OUT_X,
                                        CONV1_OUT_Y,
                                        (q15_t *)col_buffer,
                                        NULL);
    arm_relu_q7(buffer1, CONV1_OUT_X * CONV1_OUT_Y * CONV1_OUT_CH);

    // CONV2 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV2_IN_X,
                                                  CONV2_IN_Y,
                                                  CONV1_OUT_CH,
                                                  conv2_ds_wt,
                                                  CONV1_OUT_CH,
                                                  CONV2_DS_KX,
                                                  CONV2_DS_KY,
                                                  CONV2_DS_PX,
                                                  CONV2_DS_PY,
                                                  CONV2_DS_SX,
                                                  CONV2_DS_SY,
                                                  conv2_ds_bias,
                                                  CONV2_DS_BIAS_LSHIFT,
                                                  CONV2_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV2_OUT_X,
                                                  CONV2_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV2_OUT_X * CONV2_OUT_Y * CONV2_OUT_CH);

    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV2_OUT_X,
                                           CONV2_OUT_Y,
                                           CONV1_OUT_CH,
                                           conv2_pw_wt,
                                           CONV2_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv2_pw_bias,
                                           CONV2_PW_BIAS_LSHIFT,
                                           CONV2_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV2_OUT_X,
                                           CONV2_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV2_OUT_X * CONV2_OUT_Y * CONV2_OUT_CH);

    // CONV3 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV3_IN_X,
                                                  CONV3_IN_Y,
                                                  CONV2_OUT_CH,
                                                  conv3_ds_wt,
                                                  CONV2_OUT_CH,
                                                  CONV3_DS_KX,
                                                  CONV3_DS_KY,
                                                  CONV3_DS_PX,
                                                  CONV3_DS_PY,
                                                  CONV3_DS_SX,
                                                  CONV3_DS_SY,
                                                  conv3_ds_bias,
                                                  CONV3_DS_BIAS_LSHIFT,
                                                  CONV3_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV3_OUT_X,
                                                  CONV3_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV3_OUT_X * CONV3_OUT_Y * CONV3_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV3_OUT_X,
                                           CONV3_OUT_Y,
                                           CONV2_OUT_CH,
                                           conv3_pw_wt,
                                           CONV3_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv3_pw_bias,
                                           CONV3_PW_BIAS_LSHIFT,
                                           CONV3_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV3_OUT_X,
                                           CONV3_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV3_OUT_X * CONV3_OUT_Y * CONV3_OUT_CH);
    // CONV4 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV4_IN_X,
                                                  CONV4_IN_Y,
                                                  CONV3_OUT_CH,
                                                  conv4_ds_wt,
                                                  CONV3_OUT_CH,
                                                  CONV4_DS_KX,
                                                  CONV4_DS_KY,
                                                  CONV4_DS_PX,
                                                  CONV4_DS_PY,
                                                  CONV4_DS_SX,
                                                  CONV4_DS_SY,
                                                  conv4_ds_bias,
                                                  CONV4_DS_BIAS_LSHIFT,
                                                  CONV4_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV4_OUT_X,
                                                  CONV4_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV4_OUT_X * CONV4_OUT_Y * CONV4_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV4_OUT_X,
                                           CONV4_OUT_Y,
                                           CONV3_OUT_CH,
                                           conv4_pw_wt,
                                           CONV4_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv4_pw_bias,
                                           CONV4_PW_BIAS_LSHIFT,
                                           CONV4_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV4_OUT_X,
                                           CONV4_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV4_OUT_X * CONV4_OUT_Y * CONV4_OUT_CH);
    // CONV5 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV5_IN_X,
                                                  CONV5_IN_Y,
                                                  CONV4_OUT_CH,
                                                  conv5_ds_wt,
                                                  CONV4_OUT_CH,
                                                  CONV5_DS_KX,
                                                  CONV5_DS_KY,
                                                  CONV5_DS_PX,
                                                  CONV5_DS_PY,
                                                  CONV5_DS_SX,
                                                  CONV5_DS_SY,
                                                  conv5_ds_bias,
                                                  CONV5_DS_BIAS_LSHIFT,
                                                  CONV5_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV5_OUT_X,
                                                  CONV5_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV5_OUT_X * CONV5_OUT_Y * CONV5_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV5_OUT_X,
                                           CONV5_OUT_Y,
                                           CONV4_OUT_CH,
                                           conv5_pw_wt,
                                           CONV5_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv5_pw_bias,
                                           CONV5_PW_BIAS_LSHIFT,
                                           CONV5_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV5_OUT_X,
                                           CONV5_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV5_OUT_X * CONV5_OUT_Y * CONV5_OUT_CH);
    // Average pool
    arm_avepool_q7_HWC_nonsquare(buffer1,
                                 CONV5_OUT_X,
                                 CONV5_OUT_Y,
                                 CONV5_OUT_CH,
                                 CONV5_OUT_X,
                                 CONV5_OUT_Y,
                                 0,
                                 0,
                                 1,
                                 1,
                                 1,
                                 1,
                                 NULL,
                                 buffer2,
                                 2);
    arm_fully_connected_q7(buffer2,
                           final_fc_wt,
                           CONV5_OUT_CH,
                           OUT_DIM,
                           FINAL_FC_BIAS_LSHIFT,
                           FINAL_FC_OUT_RSHIFT,
                           final_fc_bias,
                           out_data,
                           (q15_t *)col_buffer);
}
