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

void
s_riscv_cmplx_dot_prod_f32(const ee_f32_t *p_a,
                           const ee_f32_t *p_b,
                           uint32_t        len,
                           ee_f32_t       *p_r,
                           ee_f32_t       *p_i)
{
    if (!p_a || !p_b || !p_r || !p_i || len == 0)
    {
        return;
    }

    ee_f32_t real_sum = 0.0f;
    ee_f32_t imag_sum = 0.0f;

    for (uint32_t i = 0; i < len; i++)
    {
        ee_f32_t ar = p_a[2 * i];
        ee_f32_t ai = p_a[2 * i + 1];
        ee_f32_t br = p_b[2 * i];
        ee_f32_t bi = p_b[2 * i + 1];

        real_sum += ar * br - ai * bi;
        imag_sum += ar * bi + ai * br;
    }

    *p_r = real_sum;
    *p_i = imag_sum;
}
