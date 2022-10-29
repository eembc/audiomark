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

#include "ee_mfcc_f32.h"

/* TODO: ptorelli: does EVERYTHING need to be memreq'd? */
static ee_f32_t g_mfcc_input_frame[FFT_LEN];
static ee_f32_t g_mfcc_out[NUM_MFCC_FEATURES];
/* Used for several things, max size is complex FFT = FFT_LEN + 2 */
static ee_f32_t g_tmp[FFT_LEN + 2];

/* This code only ever instantiates one MFCC; this can be global. */
ee_rfft_f32_t g_rfft_instance;

/**
 * @brief
 *
 * @param p_src - FFT_LEN array of float audio (normalized)
 * @param p_dst - MFCC features' DCT array (10 features * 40 coeffs)
 */
static void
ee_mfcc_f32(ee_f32_t *p_src, ee_f32_t *p_dst)
{
    const ee_f32_t *coeffs = ee_mfcc_filter_coefs_f32;
    ee_matrix_f32_t dct_matrix;

    /* Multiply by window */
    th_multiply_f32(
        p_src, (ee_f32_t *)ee_mfcc_window_coefs_f32, p_src, FFT_LEN);

    /* Compute spectrum magnitude, g_tmp is now the FFT */
    th_rfft_f32(&g_rfft_instance, p_src, g_tmp, 0);

    /* Unpack real values */
    g_tmp[FFT_LEN]     = g_tmp[1];
    g_tmp[FFT_LEN + 1] = 0.0f;
    g_tmp[1]           = 0.0f;

    /* Apply MEL filters */
    /* N.B. This overwrites p_src and reliquinshes g_tmp */
    th_cmplx_mag_f32(g_tmp, p_src, FFT_LEN);
    for (int i = 0; i < EE_NUM_MFCC_FILTER_CONFIG; i++)
    {
        /* g_tmp[i] is now the MEL energy for that bin. */
        th_dot_prod_f32(p_src + ee_mfcc_filter_pos[i],
                        (ee_f32_t *)coeffs,
                        ee_mfcc_filter_len[i],
                        &(g_tmp[i]));
        coeffs += ee_mfcc_filter_len[i];
    }

    /* Compute the log */
    th_offset_f32(g_tmp, 1.0e-6f, g_tmp, EE_NUM_MFCC_FILTER_CONFIG);
    th_vlog_f32(g_tmp, g_tmp, EE_NUM_MFCC_FILTER_CONFIG);

    /* Multiply the energies (g_tmp) with the DCT matrix */
    dct_matrix.numRows = NUM_MFCC_FEATURES;
    dct_matrix.numCols = EE_NUM_MFCC_FILTER_CONFIG;
    dct_matrix.pData   = (ee_f32_t *)ee_mfcc_dct_coefs_f32;
    th_mat_vec_mult_f32(&dct_matrix, g_tmp, p_dst);
}

ee_status_t
ee_mfcc_f32_init(void)
{
    ee_status_t status;
    /* TODO What other INIT should we put here? */
    status = th_rfft_init_f32(&g_rfft_instance, FFT_LEN);
    return status;
}

void
ee_mfcc_f32_compute(const int16_t *p_audio_data, int8_t *p_mfcc_out)
{
    /* TensorFlow way of normalizing .wav data to (-1,1) */
    for (int i = 0; i < FRAME_LEN; i++)
    {
        g_mfcc_input_frame[i] = (ee_f32_t)p_audio_data[i] / (1 << 15);
    }

    /* Pad the remaining frame with zeroes, since FFT_LEN >= FRAME_LEN */
    memset(&(g_mfcc_input_frame[FRAME_LEN]),
           0,
           sizeof(ee_f32_t) * (PADDED_FRAME_LEN - FRAME_LEN));

    ee_mfcc_f32(g_mfcc_input_frame, g_mfcc_out);

    for (int i = 0; i < NUM_MFCC_FEATURES; i++)
    {
        float sum = g_mfcc_out[i];
        /* Input is Qx.mfcc_dec_bits (from quantization step) */
        sum *= (0x1 << MFCC_DEC_BITS);
        sum = round(sum);
        if (sum >= 127)
        {
            p_mfcc_out[i] = 127;
        }
        else if (sum <= -128)
        {
            p_mfcc_out[i] = -128;
        }
        else
        {
            p_mfcc_out[i] = sum;
        }
    }
}
