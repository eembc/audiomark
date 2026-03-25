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

#include "ee_audiomark.h"
#include "ee_api.h"
#include "s_riscv_audiomark.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static void
radix2_bit_reverse(ee_f32_t *real, ee_f32_t *imag, int n)
{
    int j = 0;
    for (int i = 0; i < n - 1; i++)
    {
        if (i < j)
        {
            ee_f32_t temp_r = real[i];
            ee_f32_t temp_i = imag[i];
            real[i]         = real[j];
            imag[i]         = imag[j];
            real[j]         = temp_r;
            imag[j]         = temp_i;
        }
        int k = n / 2;
        while (k <= j)
        {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}

static void
radix2_cfft(ee_f32_t *real, ee_f32_t *imag, int n, uint8_t is_inverse)
{
    radix2_bit_reverse(real, imag, n);

    for (int step = 1; step < n; step *= 2)
    {
        float theta    = (is_inverse ? 1.0f : -1.0f) * M_PI / step;
        float w_step_r = cosf(theta);
        float w_step_i = sinf(theta);

        for (int i = 0; i < n; i += 2 * step)
        {
            float w_r = 1.0f;
            float w_i = 0.0f;
            for (int j = 0; j < step; j++)
            {
                // Butterfly calculation
                float t_r = w_r * real[i + j + step] - w_i * imag[i + j + step];
                float t_i = w_r * imag[i + j + step] + w_i * real[i + j + step];

                real[i + j + step] = real[i + j] - t_r;
                imag[i + j + step] = imag[i + j] - t_i;

                real[i + j] += t_r;
                imag[i + j] += t_i;

                // Rotate twiddle factor
                float next_w_r = w_r * w_step_r - w_i * w_step_i;
                float next_w_i = w_r * w_step_i + w_i * w_step_r;
                w_r            = next_w_r;
                w_i            = next_w_i;
            }
        }
    }

    // Scale for inverse transform
    if (is_inverse)
    {
        for (int i = 0; i < n; i++)
        {
            real[i] /= n;
            imag[i] /= n;
        }
    }
}

void
s_riscv_rfft_f32(ee_rfft_f32_t *p_instance,
                 ee_f32_t      *p_in,
                 ee_f32_t      *p_out,
                 uint8_t        ifftFlag)
{
    if (!p_instance || !p_in || !p_out || !p_instance->work_real
        || !p_instance->work_imag)
    {
        return;
    }

    int       n    = p_instance->fft_len;
    ee_f32_t *real = p_instance->work_real;
    ee_f32_t *imag = p_instance->work_imag;

    if (ifftFlag == 0)
    {
        /* --- FORWARD RFFT --- */

        // 1. Load real input
        for (int i = 0; i < n; i++)
        {
            real[i] = p_in[i];
            imag[i] = 0.0f;
        }

        // 2. Complex FFT
        radix2_cfft(real, imag, n, 0);

        // 3. Pack output (CMSIS-style)
        p_out[0] = real[0];     // DC
        p_out[1] = real[n / 2]; // Nyquist

        for (int i = 1; i < n / 2; i++)
        {
            p_out[2 * i]     = real[i];
            p_out[2 * i + 1] = imag[i];
        }
    }
    else
    {
        /* --- INVERSE RFFT --- */

        // 1. Unpack
        real[0] = p_in[0];
        imag[0] = 0.0f;

        real[n / 2] = p_in[1];
        imag[n / 2] = 0.0f;

        for (int i = 1; i < n / 2; i++)
        {
            real[i] = p_in[2 * i];
            imag[i] = p_in[2 * i + 1];

            real[n - i] = p_in[2 * i];
            imag[n - i] = -p_in[2 * i + 1];
        }

        // 2. Inverse FFT
        radix2_cfft(real, imag, n, 1);

        // 3. Extract real part
        for (int i = 0; i < n; i++)
        {
            p_out[i] = real[i];
        }
    }
}
