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

void
s_riscv_cfft_f32(ee_cfft_f32_t *p_instance,
                 ee_f32_t      *p_buf,
                 uint8_t        ifftFlag,
                 uint8_t        bitReverseFlagR)
{
    (void)bitReverseFlagR;

    if (!p_instance || !p_buf)
    {
        return;
    }

    const int n = p_instance->fft_len;

    /* ---- bit-reversal permutation ---- */
    int j = 0;
    for (int i = 0; i < n - 1; i++)
    {
        if (i < j)
        {
            ee_f32_t tr = p_buf[2 * i], ti = p_buf[2 * i + 1];
            p_buf[2 * i]     = p_buf[2 * j];
            p_buf[2 * i + 1] = p_buf[2 * j + 1];
            p_buf[2 * j]     = tr;
            p_buf[2 * j + 1] = ti;
        }
        int k = n >> 1;
        while (k <= j)
        {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    /* ---- Cooley-Tukey butterfly stages ---- */
    /*
     * sign = -1 → forward FFT:  W = exp(-j * pi * m / step)
     * sign = +1 → inverse FFT:  W = exp(+j * pi * m / step)
     */
    ee_f32_t sign = ifftFlag ? 1.0f : -1.0f;

    for (int step = 1; step < n; step <<= 1)
    {
        ee_f32_t theta = sign * M_PI / (ee_f32_t)step;
        ee_f32_t ws_r  = cosf(theta);
        ee_f32_t ws_i  = sinf(theta);

        for (int i = 0; i < n; i += (step << 1))
        {
            ee_f32_t w_r = 1.0f, w_i = 0.0f;

            for (int m = 0; m < step; m++)
            {
                int p = 2 * (i + m);
                int q = 2 * (i + m + step);

                ee_f32_t xr = p_buf[q], xi = p_buf[q + 1];
                ee_f32_t t_r = w_r * xr - w_i * xi;
                ee_f32_t t_i = w_r * xi + w_i * xr;

                ee_f32_t ur = p_buf[p], ui = p_buf[p + 1];
                p_buf[q]     = ur - t_r;
                p_buf[q + 1] = ui - t_i;
                p_buf[p]     = ur + t_r;
                p_buf[p + 1] = ui + t_i;

                /* twiddle recurrence: w *= ws */
                ee_f32_t nw_r = w_r * ws_r - w_i * ws_i;
                ee_f32_t nw_i = w_r * ws_i + w_i * ws_r;
                w_r           = nw_r;
                w_i           = nw_i;
            }
        }
    }

    /* ---- 1/N scaling for inverse ---- */
    if (ifftFlag)
    {
        ee_f32_t inv_n = 1.0f / (ee_f32_t)n;
        for (int i = 0; i < 2 * n; i++)
        {
            p_buf[i] *= inv_n;
        }
    }
}
