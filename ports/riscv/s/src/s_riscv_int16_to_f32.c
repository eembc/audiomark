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
s_riscv_int16_to_f32(const int16_t *p_src, ee_f32_t *p_dst, uint32_t len)
{
    if (!p_src || !p_dst || len == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        p_dst[i] = (ee_f32_t)p_src[i];
    }
}
