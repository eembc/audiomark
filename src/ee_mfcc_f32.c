/**
 * Copyright (C) 2022 EEMBC
 * Copyright (C) 2022 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#include "ee_mfcc_f32.h"

/**
 * @brief
 *
 * @param p_src - FFT_LEN array of float audio (normalized)
 * @param p_dst - MFCC features' DCT array (10 features * 40 coeffs)
 */
static void
ee_mfcc_f32(mfcc_instance_t *p_inst)
{
    const ee_f32_t *coeffs = ee_mfcc_filter_coefs_f32;
    ee_f32_t       *p_src  = p_inst->mfcc_input_frame;
    ee_f32_t       *p_dst  = p_inst->mfcc_out;
    ee_matrix_f32_t dct_matrix;

    /* Multiply by window */
    th_multiply_f32(
        p_src, (ee_f32_t *)ee_mfcc_window_coefs_f32, p_src, EE_NUM_MFCC_WIN_COEFS);

    /* Compute spectrum magnitude, g_tmp is now the FFT */
    th_rfft_f32(&(p_inst->rfft_instance), p_src, p_inst->tmp, 0);

    /* Unpack real values */
    p_inst->tmp[FFT_LEN]     = p_inst->tmp[1];
    p_inst->tmp[FFT_LEN + 1] = 0.0f;
    p_inst->tmp[1]           = 0.0f;

    /* Apply MEL filters */
    /* N.B. This overwrites p_src and reliquinshes p_inst->tmp */
    th_cmplx_mag_f32(p_inst->tmp, p_src, FFT_LEN / 2);
    for (int i = 0; i < EE_NUM_MFCC_FILTER_CONFIG; i++)
    {
        /* p_inst->tmp[i] is now the MEL energy for that bin. */
        th_dot_prod_f32(p_src + ee_mfcc_filter_pos[i],
                        (ee_f32_t *)coeffs,
                        ee_mfcc_filter_len[i],
                        &(p_inst->tmp[i]));
        coeffs += ee_mfcc_filter_len[i];
    }

    /* Compute the log */
    th_offset_f32(p_inst->tmp, 1.0e-6f, p_inst->tmp, EE_NUM_MFCC_FILTER_CONFIG);
    th_vlog_f32(p_inst->tmp, p_inst->tmp, EE_NUM_MFCC_FILTER_CONFIG);

    /* Multiply the energies (p_inst->tmp) with the DCT matrix */
    dct_matrix.numRows = NUM_MFCC_FEATURES;
    dct_matrix.numCols = EE_NUM_MFCC_FILTER_CONFIG;
    dct_matrix.pData   = (ee_f32_t *)ee_mfcc_dct_coefs_f32;
    th_mat_vec_mult_f32(&dct_matrix, p_inst->tmp, p_dst);
}

ee_status_t
ee_mfcc_f32_init(mfcc_instance_t *p_inst)
{
    ee_status_t status;
    // Great way to catch memory errors on some compilers!
    memset(p_inst, 0, sizeof(mfcc_instance_t));
    status = th_rfft_init_f32(&p_inst->rfft_instance, FFT_LEN);
    return status;
}

void
ee_mfcc_f32_compute(mfcc_instance_t *p_inst,
                    const int16_t   *p_audio_data,
                    int8_t          *p_mfcc_out)
{
    /* TensorFlow way of normalizing .wav data to (-1,1) */
    for (int i = 0; i < FRAME_LEN; i++)
    {
        p_inst->mfcc_input_frame[i] = (ee_f32_t)p_audio_data[i] / (1 << 15);
    }

    /* Pad the remaining frame with zeroes, since FFT_LEN >= FRAME_LEN */
    memset(&(p_inst->mfcc_input_frame[FRAME_LEN]),
           0,
           sizeof(ee_f32_t) * (PADDED_FRAME_LEN - FRAME_LEN));

    ee_mfcc_f32(p_inst);

    for (int i = 0; i < NUM_MFCC_FEATURES; i++)
    {
        ee_f32_t sum = p_inst->mfcc_out[i];

        sum = sum * MFCC_SCALE + MFCC_OFFSET;

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
            p_mfcc_out[i] = (int8_t)sum;
        }
    }
}
