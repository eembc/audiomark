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

ee_status_t
s_riscv_cfft_init_f32(ee_cfft_f32_t *p_instance, int fft_length)
{
    if (!p_instance || fft_length <= 0)
    {
        return EE_STATUS_ERROR;
    }

    /* checking if fft_length is a power of 2 */
    if ((fft_length & (fft_length - 1)) != 0)
    {
        return EE_STATUS_ERROR;
    }

    p_instance->fft_len = fft_length;

    return EE_STATUS_OK;
}
