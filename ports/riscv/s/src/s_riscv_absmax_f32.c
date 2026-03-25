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

void
s_riscv_absmax_f32(const ee_f32_t *p_in,
                   uint32_t        len,
                   ee_f32_t       *p_max,
                   uint32_t       *p_index)
{
    if (!p_in || !p_max || !p_index || len == 0)
    {
        return;
    }

    ee_f32_t max_val = fabsf(p_in[0]);
    uint32_t max_idx = 0;

    for (uint32_t i = 1; i < len; i++)
    {
        ee_f32_t val = fabsf(p_in[i]);

        if (val > max_val)
        {
            max_val = val;
            max_idx = i;
        }
    }

    *p_max   = max_val;
    *p_index = max_idx;
}
