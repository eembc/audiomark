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
s_riscv_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c)
{
    if (!p_a || !p_b || !p_c)
    {
        return;
    }

    uint32_t rows = p_a->numRows;
    uint32_t cols = p_a->numCols;

    for (uint32_t i = 0; i < rows; i++)
    {
        ee_f32_t sum = 0.0f;

        for (uint32_t j = 0; j < cols; j++)
        {
            sum += p_a->pData[i * cols + j] * p_b[j];
        }

        p_c[i] = sum;
    }
}
