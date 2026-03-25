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
s_riscv_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length)
{
    if (!p_instance || fft_length <= 0)
    {
        return EE_STATUS_ERROR;
    }

    if ((fft_length & (fft_length - 1)) != 0 || fft_length < 2)
    {
        return EE_STATUS_ERROR;
    }

    p_instance->fft_len = fft_length;

    p_instance->work_real
        = th_malloc(sizeof(ee_f32_t) * fft_length, COMPONENT_KWS);
    p_instance->work_imag
        = th_malloc(sizeof(ee_f32_t) * fft_length, COMPONENT_KWS);

    if (!p_instance->work_real || !p_instance->work_imag)
    {
        return EE_STATUS_ERROR;
    }

    return EE_STATUS_OK;
}
