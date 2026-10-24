/**
 * Copyright (C) 2024 SPEC Embedded Group
 * Copyright (C) 2022-2023 EEMBC
 * Copyright (C) 2022-2024 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 *
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "ee_audiomark.h"
#include "ee_api.h"
#include "nn/functions.h"
#include "nn/support_functions.h"
#include "nn/buffer_utils.h"

#define BUF_CONV0 \
    NN_CONV0_S8_BUF_SIZE(CONV_0_IN_CH, CONV_0_FILTER_W, CONV_0_FILTER_H)
#define BUF_CONV2 NN_CONV1X1_S8_BUF_SIZE(CONV_2_OUT_CH)
#define BUF_CONV4 NN_CONV1X1_S8_BUF_SIZE(CONV_4_OUT_CH)
#define BUF_CONV6 NN_CONV1X1_S8_BUF_SIZE(CONV_6_OUT_CH)
#define BUF_CONV8 NN_CONV1X1_S8_BUF_SIZE(CONV_8_OUT_CH)

#define MAX_BUF_SIZE \
    (MAX5(BUF_CONV0, BUF_CONV2, BUF_CONV4, BUF_CONV6, BUF_CONV8))

/* Test for a complete int8 DS_CNN_S keyword spotting network from
 * https://github.com/ARM-software/ML-zoo & Tag: 22.02 */
nn_context ctx;

static uint8_t scratch_buffer[MAX_BUF_SIZE] __attribute__((aligned(8)));

void
th_nn_init(void)
{
    ctx.size = MAX_BUF_SIZE;
    ctx.buf  = scratch_buffer;
}
