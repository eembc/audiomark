/**
 * Copyright (C) 2024 SPEC Embedded Group
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

#define restrict __restrict__
#include "ee_audiomark.h"
#include "ee_api.h"
#include "s_riscv_audiomark.h"

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
th_memmove(void *dst, const void *src, size_t n)
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
    return s_riscv_cfft_init_f32(p_instance, fft_length);
}

void
th_cfft_f32(ee_cfft_f32_t *p_instance,
            ee_f32_t      *p_buf,
            uint8_t        ifftFlag,
            uint8_t        bitReverseFlagR)
{
    s_riscv_cfft_f32(p_instance, p_buf, ifftFlag, bitReverseFlagR);
}

ee_status_t
th_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length)
{
    return s_riscv_rfft_init_f32(p_instance, fft_length);
}

void
th_rfft_f32(ee_rfft_f32_t *p_instance,
            ee_f32_t      *p_in,
            ee_f32_t      *p_out,
            uint8_t        ifftFlag)
{
    s_riscv_rfft_f32(p_instance, p_in, p_out, ifftFlag);
}

void
th_absmax_f32(const ee_f32_t *p_in,
              uint32_t        len,
              ee_f32_t       *p_max,
              uint32_t       *p_index)
{
    s_riscv_absmax_f32(p_in, len, p_max, p_index);
}

void
th_cmplx_mult_cmplx_f32(const ee_f32_t *p_a,
                        const ee_f32_t *p_b,
                        ee_f32_t       *p_c,
                        uint32_t        len)
{
    s_riscv_cmplx_mult_cmplx_f32(p_a, p_b, p_c, len);
}

void
th_cmplx_conj_f32(const ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_cmplx_conj_f32(p_a, p_c, len);
}

void
th_cmplx_dot_prod_f32(const ee_f32_t *p_a,
                      const ee_f32_t *p_b,
                      uint32_t        len,
                      ee_f32_t       *p_r,
                      ee_f32_t       *p_i)
{
    s_riscv_cmplx_dot_prod_f32(p_a, p_b, len, p_r, p_i);
}

void
th_int16_to_f32(const int16_t *p_src, ee_f32_t *p_dst, uint32_t len)
{
    s_riscv_int16_to_f32(p_src, p_dst, len);
}

void
th_f32_to_int16(const ee_f32_t *p_src, int16_t *p_dst, uint32_t len)
{
    s_riscv_f32_to_int16(p_src, p_dst, len);
}

void
th_add_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_add_f32(p_a, p_b, p_c, len);
}

void
th_subtract_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_subtract_f32(p_a, p_b, p_c, len);
}

void
th_dot_prod_f32(ee_f32_t *p_a, ee_f32_t *p_b, uint32_t len, ee_f32_t *p_result)
{
    s_riscv_dot_prod_f32(p_a, p_b, len, p_result);
}

void
th_multiply_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_multiply_f32(p_a, p_b, p_c, len);
}

void
th_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_cmplx_mag_f32(p_a, p_c, len);
}

void
th_offset_f32(ee_f32_t *p_a, ee_f32_t offset, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_offset_f32(p_a, offset, p_c, len);
}

void
th_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    s_riscv_vlog_f32(p_a, p_c, len);
}

void
th_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c)
{
    s_riscv_mat_vec_mult_f32(p_a, p_b, p_c);
}

void
th_nn_init(void)
{
    s_riscv_nn_init();
}

ee_status_t
th_nn_classify(const int8_t in_data[490], int8_t out_data[12])
{
    return s_riscv_nn_classify();
}
