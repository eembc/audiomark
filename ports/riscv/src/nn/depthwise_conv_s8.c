/**
 * Copyright 2026 Sohail Raj Satapathy
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "functions.h"
#include "support_functions.h"
#include "ee_api.h"

#include <stdint.h>
#include <stdbool.h>

#define KERNEL_DIM     3
#define INPUT_W        5
#define INPUT_H        25
#define INPUT_CH       64
#define PAD_W          1
#define PAD_H          1
#define STRIDE_W       1
#define STRIDE_H       1
#define OUTPUT_W       5
#define OUTPUT_H       25
#define INPUT_OFFSET   128
#define OUTPUT_OFFSET  (-128)
#define OUT_ACT_MIN    (-128)
#define OUT_ACT_MAX    127
#define COL_STRIDE     INPUT_CH
#define INP_ROW_STRIDE (INPUT_CH * INPUT_W)
#define KER_ROW_STRIDE (INPUT_CH * KERNEL_DIM)
#define INPUT_X_STEP   (STRIDE_W * COL_STRIDE)

static inline void
acc_col_4ch(int32_t *__EE_RESTRICT      b0,
            int32_t *__EE_RESTRICT      b1,
            int32_t *__EE_RESTRICT      b2,
            int32_t *__EE_RESTRICT      b3,
            const int8_t *__EE_RESTRICT ip,
            const int8_t *__EE_RESTRICT kp,
            int32_t                     kh_start,
            int32_t                     kh_end)
{
    int32_t start = kh_start < 0 ? 0 : kh_start;
    int32_t end   = kh_end > KERNEL_DIM ? KERNEL_DIM : kh_end;
    if (start >= end)
    {
        return;
    }
    switch (start)
    {
        case 0: {
            const int8_t *in  = ip;
            const int8_t *ker = kp;
            *b0 += ((int32_t)in[0] + INPUT_OFFSET) * (int32_t)ker[0];
            *b1 += ((int32_t)in[1] + INPUT_OFFSET) * (int32_t)ker[1];
            *b2 += ((int32_t)in[2] + INPUT_OFFSET) * (int32_t)ker[2];
            *b3 += ((int32_t)in[3] + INPUT_OFFSET) * (int32_t)ker[3];
            if (end == 1)
            {
                break;
            }
        }
        case 1: {
            const int8_t *in  = ip + INP_ROW_STRIDE;
            const int8_t *ker = kp + KER_ROW_STRIDE;
            *b0 += ((int32_t)in[0] + INPUT_OFFSET) * (int32_t)ker[0];
            *b1 += ((int32_t)in[1] + INPUT_OFFSET) * (int32_t)ker[1];
            *b2 += ((int32_t)in[2] + INPUT_OFFSET) * (int32_t)ker[2];
            *b3 += ((int32_t)in[3] + INPUT_OFFSET) * (int32_t)ker[3];
            if (end == 2)
            {
                break;
            }
        }
        case 2: {
            const int8_t *in  = ip + 2 * INP_ROW_STRIDE;
            const int8_t *ker = kp + 2 * KER_ROW_STRIDE;
            *b0 += ((int32_t)in[0] + INPUT_OFFSET) * (int32_t)ker[0];
            *b1 += ((int32_t)in[1] + INPUT_OFFSET) * (int32_t)ker[1];
            *b2 += ((int32_t)in[2] + INPUT_OFFSET) * (int32_t)ker[2];
            *b3 += ((int32_t)in[3] + INPUT_OFFSET) * (int32_t)ker[3];
            break;
        }
        default:
            break;
    }
}

static inline void
acc_col_1ch(int32_t *__EE_RESTRICT      b0,
            const int8_t *__EE_RESTRICT ip,
            const int8_t *__EE_RESTRICT kp,
            int32_t                     kh_start,
            int32_t                     kh_end)
{
    int32_t start = kh_start < 0 ? 0 : kh_start;
    int32_t end   = kh_end > KERNEL_DIM ? KERNEL_DIM : kh_end;
    if (start >= end)
    {
        return;
    }
    switch (start)
    {
        case 0: {
            *b0 += ((int32_t)ip[0] + INPUT_OFFSET) * (int32_t)kp[0];
            if (end == 1)
            {
                break;
            }
        }
        case 1: {
            const int8_t *in  = ip + INP_ROW_STRIDE;
            const int8_t *ker = kp + KER_ROW_STRIDE;
            *b0 += ((int32_t)in[0] + INPUT_OFFSET) * (int32_t)ker[0];
            if (end == 2)
            {
                break;
            }
        }
        case 2: {
            const int8_t *in  = ip + 2 * INP_ROW_STRIDE;
            const int8_t *ker = kp + 2 * KER_ROW_STRIDE;
            *b0 += ((int32_t)in[0] + INPUT_OFFSET) * (int32_t)ker[0];
            break;
        }
        default:
            break;
    }
}

int32_t
nn_depthwise_conv_3x3_s8(const nn_per_channel_quant_params *__EE_RESTRICT
                                                      quant_params,
                         const q7_t *__EE_RESTRICT    input,
                         const q7_t *__EE_RESTRICT    kernel,
                         const int32_t *__EE_RESTRICT bias,
                         q7_t *__EE_RESTRICT          output)
{
    const int32_t *__EE_RESTRICT output_mult  = quant_params->multiplier;
    const int32_t *__EE_RESTRICT output_shift = quant_params->shift;

    int32_t int_h0 = 0;
    while (int_h0 < OUTPUT_H && (int_h0 * STRIDE_H - PAD_H) < 0)
    {
        ++int_h0;
    }

    int32_t int_h1 = int_h0;
    while (int_h1 < OUTPUT_H && (int_h1 * STRIDE_H - PAD_H + 2) < INPUT_H)
    {
        ++int_h1;
    }

    int32_t int_w0 = 0;
    while (int_w0 < OUTPUT_W && (int_w0 * STRIDE_W - PAD_W) < 0)
    {
        ++int_w0;
    }

    int32_t int_w1 = int_w0;
    while (int_w1 < OUTPUT_W && (int_w1 * STRIDE_W - PAD_W + 2) < INPUT_W)
    {
        ++int_w1;
    }

    int32_t out_idx = 0;

    for (int32_t out_h = 0; out_h < int_h0; ++out_h)
    {
        const int32_t in_h     = out_h * STRIDE_H - PAD_H;
        const int32_t kh_start = -in_h;
        const int32_t kh_end   = MIN(KERNEL_DIM, INPUT_H - in_h);

        for (int32_t out_w = 0; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w     = out_w * STRIDE_W - PAD_W;
            const int32_t kw_start = (in_w < 0) ? -in_w : 0;
            const bool    right_ok = (in_w + 2) < INPUT_W;
            const int8_t *inp_base
                = input + in_h * INP_ROW_STRIDE + in_w * COL_STRIDE;

            int32_t ch = 0;
            for (; ch <= (INPUT_CH - 4); ch += 4)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch + 0], b1 = bias[ch + 1];
                int32_t       b2 = bias[ch + 2], b3 = bias[ch + 3];

                if (kw_start == 0)
                {
                    acc_col_4ch(&b0, &b1, &b2, &b3, ip, kp, kh_start, kh_end);
                }
                acc_col_4ch(&b0,
                            &b1,
                            &b2,
                            &b3,
                            ip + COL_STRIDE,
                            kp + COL_STRIDE,
                            kh_start,
                            kh_end);
                if (right_ok)
                {
                    acc_col_4ch(&b0,
                                &b1,
                                &b2,
                                &b3,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                kh_start,
                                kh_end);
                }

                b0 = nn_requantize(
                         b0, output_mult[ch + 0], output_shift[ch + 0])
                     + OUTPUT_OFFSET;
                b1 = nn_requantize(
                         b1, output_mult[ch + 1], output_shift[ch + 1])
                     + OUTPUT_OFFSET;
                b2 = nn_requantize(
                         b2, output_mult[ch + 2], output_shift[ch + 2])
                     + OUTPUT_OFFSET;
                b3 = nn_requantize(
                         b3, output_mult[ch + 3], output_shift[ch + 3])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b1, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b2, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b3, OUT_ACT_MIN), OUT_ACT_MAX);
            }
            for (; ch < INPUT_CH; ++ch)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch];

                if (kw_start == 0)
                {
                    acc_col_1ch(&b0, ip, kp, kh_start, kh_end);
                }
                acc_col_1ch(
                    &b0, ip + COL_STRIDE, kp + COL_STRIDE, kh_start, kh_end);
                if (right_ok)
                {
                    acc_col_1ch(&b0,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                kh_start,
                                kh_end);
                }

                b0 = nn_requantize(b0, output_mult[ch], output_shift[ch])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
            }
        }
    }

    for (int32_t out_h = int_h0; out_h < int_h1; ++out_h)
    {
        const int32_t in_h = out_h * STRIDE_H - PAD_H;

        for (int32_t out_w = 0; out_w < int_w0; ++out_w)
        {
            const int32_t in_w     = out_w * STRIDE_W - PAD_W;
            const int32_t kw_start = -in_w;
            const bool    right_ok = (in_w + 2) < INPUT_W;
            const int8_t *inp_base
                = input + in_h * INP_ROW_STRIDE + in_w * COL_STRIDE;

            int32_t ch = 0;
            for (; ch <= (INPUT_CH - 4); ch += 4)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch + 0], b1 = bias[ch + 1];
                int32_t       b2 = bias[ch + 2], b3 = bias[ch + 3];

                if (kw_start == 0)
                {
                    acc_col_4ch(&b0, &b1, &b2, &b3, ip, kp, 0, KERNEL_DIM);
                }
                acc_col_4ch(&b0,
                            &b1,
                            &b2,
                            &b3,
                            ip + COL_STRIDE,
                            kp + COL_STRIDE,
                            0,
                            KERNEL_DIM);
                if (right_ok)
                {
                    acc_col_4ch(&b0,
                                &b1,
                                &b2,
                                &b3,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                KERNEL_DIM);
                }

                b0 = nn_requantize(
                         b0, output_mult[ch + 0], output_shift[ch + 0])
                     + OUTPUT_OFFSET;
                b1 = nn_requantize(
                         b1, output_mult[ch + 1], output_shift[ch + 1])
                     + OUTPUT_OFFSET;
                b2 = nn_requantize(
                         b2, output_mult[ch + 2], output_shift[ch + 2])
                     + OUTPUT_OFFSET;
                b3 = nn_requantize(
                         b3, output_mult[ch + 3], output_shift[ch + 3])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b1, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b2, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b3, OUT_ACT_MIN), OUT_ACT_MAX);
            }
            for (; ch < INPUT_CH; ++ch)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch];

                if (kw_start == 0)
                {
                    acc_col_1ch(&b0, ip, kp, 0, KERNEL_DIM);
                }
                acc_col_1ch(
                    &b0, ip + COL_STRIDE, kp + COL_STRIDE, 0, KERNEL_DIM);
                if (right_ok)
                {
                    acc_col_1ch(&b0,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                KERNEL_DIM);
                }

                b0 = nn_requantize(b0, output_mult[ch], output_shift[ch])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
            }
        }

        const int8_t *ip0 = input + in_h * INP_ROW_STRIDE
                            + int_w0 * STRIDE_W * COL_STRIDE
                            - PAD_W * COL_STRIDE;
        for (int32_t out_w = int_w0; out_w < int_w1;
             ++out_w, ip0 += INPUT_X_STEP)
        {
            int32_t ch = 0;
            for (; ch <= (INPUT_CH - 4); ch += 4)
            {
                const int8_t *ip = ip0 + ch;
                const int8_t *kp = kernel + ch;

                const int8_t *in0 = ip;
                const int8_t *in1 = ip + INP_ROW_STRIDE;
                const int8_t *in2 = ip + 2 * INP_ROW_STRIDE;
                const int8_t *k0  = kp;
                const int8_t *k1  = kp + KER_ROW_STRIDE;
                const int8_t *k2  = kp + 2 * KER_ROW_STRIDE;

                const int8_t *in00 = in0;
                const int8_t *in01 = in0 + COL_STRIDE;
                const int8_t *in02 = in0 + 2 * COL_STRIDE;
                const int8_t *in10 = in1;
                const int8_t *in11 = in1 + COL_STRIDE;
                const int8_t *in12 = in1 + 2 * COL_STRIDE;
                const int8_t *in20 = in2;
                const int8_t *in21 = in2 + COL_STRIDE;
                const int8_t *in22 = in2 + 2 * COL_STRIDE;
                const int8_t *k00  = k0;
                const int8_t *k01  = k0 + COL_STRIDE;
                const int8_t *k02  = k0 + 2 * COL_STRIDE;
                const int8_t *k10  = k1;
                const int8_t *k11  = k1 + COL_STRIDE;
                const int8_t *k12  = k1 + 2 * COL_STRIDE;
                const int8_t *k20  = k2;
                const int8_t *k21  = k2 + COL_STRIDE;
                const int8_t *k22  = k2 + 2 * COL_STRIDE;

                int32_t b0 = bias[ch + 0], b1 = bias[ch + 1];
                int32_t b2 = bias[ch + 2], b3 = bias[ch + 3];

                b0 += ((int32_t)in00[0] + INPUT_OFFSET) * (int32_t)k00[0];
                b1 += ((int32_t)in00[1] + INPUT_OFFSET) * (int32_t)k00[1];
                b2 += ((int32_t)in00[2] + INPUT_OFFSET) * (int32_t)k00[2];
                b3 += ((int32_t)in00[3] + INPUT_OFFSET) * (int32_t)k00[3];

                b0 += ((int32_t)in01[0] + INPUT_OFFSET) * (int32_t)k01[0];
                b1 += ((int32_t)in01[1] + INPUT_OFFSET) * (int32_t)k01[1];
                b2 += ((int32_t)in01[2] + INPUT_OFFSET) * (int32_t)k01[2];
                b3 += ((int32_t)in01[3] + INPUT_OFFSET) * (int32_t)k01[3];

                b0 += ((int32_t)in02[0] + INPUT_OFFSET) * (int32_t)k02[0];
                b1 += ((int32_t)in02[1] + INPUT_OFFSET) * (int32_t)k02[1];
                b2 += ((int32_t)in02[2] + INPUT_OFFSET) * (int32_t)k02[2];
                b3 += ((int32_t)in02[3] + INPUT_OFFSET) * (int32_t)k02[3];

                b0 += ((int32_t)in10[0] + INPUT_OFFSET) * (int32_t)k10[0];
                b1 += ((int32_t)in10[1] + INPUT_OFFSET) * (int32_t)k10[1];
                b2 += ((int32_t)in10[2] + INPUT_OFFSET) * (int32_t)k10[2];
                b3 += ((int32_t)in10[3] + INPUT_OFFSET) * (int32_t)k10[3];

                b0 += ((int32_t)in11[0] + INPUT_OFFSET) * (int32_t)k11[0];
                b1 += ((int32_t)in11[1] + INPUT_OFFSET) * (int32_t)k11[1];
                b2 += ((int32_t)in11[2] + INPUT_OFFSET) * (int32_t)k11[2];
                b3 += ((int32_t)in11[3] + INPUT_OFFSET) * (int32_t)k11[3];

                b0 += ((int32_t)in12[0] + INPUT_OFFSET) * (int32_t)k12[0];
                b1 += ((int32_t)in12[1] + INPUT_OFFSET) * (int32_t)k12[1];
                b2 += ((int32_t)in12[2] + INPUT_OFFSET) * (int32_t)k12[2];
                b3 += ((int32_t)in12[3] + INPUT_OFFSET) * (int32_t)k12[3];

                b0 += ((int32_t)in20[0] + INPUT_OFFSET) * (int32_t)k20[0];
                b1 += ((int32_t)in20[1] + INPUT_OFFSET) * (int32_t)k20[1];
                b2 += ((int32_t)in20[2] + INPUT_OFFSET) * (int32_t)k20[2];
                b3 += ((int32_t)in20[3] + INPUT_OFFSET) * (int32_t)k20[3];

                b0 += ((int32_t)in21[0] + INPUT_OFFSET) * (int32_t)k21[0];
                b1 += ((int32_t)in21[1] + INPUT_OFFSET) * (int32_t)k21[1];
                b2 += ((int32_t)in21[2] + INPUT_OFFSET) * (int32_t)k21[2];
                b3 += ((int32_t)in21[3] + INPUT_OFFSET) * (int32_t)k21[3];

                b0 += ((int32_t)in22[0] + INPUT_OFFSET) * (int32_t)k22[0];
                b1 += ((int32_t)in22[1] + INPUT_OFFSET) * (int32_t)k22[1];
                b2 += ((int32_t)in22[2] + INPUT_OFFSET) * (int32_t)k22[2];
                b3 += ((int32_t)in22[3] + INPUT_OFFSET) * (int32_t)k22[3];

                b0 = nn_requantize(
                         b0, output_mult[ch + 0], output_shift[ch + 0])
                     + OUTPUT_OFFSET;
                b1 = nn_requantize(
                         b1, output_mult[ch + 1], output_shift[ch + 1])
                     + OUTPUT_OFFSET;
                b2 = nn_requantize(
                         b2, output_mult[ch + 2], output_shift[ch + 2])
                     + OUTPUT_OFFSET;
                b3 = nn_requantize(
                         b3, output_mult[ch + 3], output_shift[ch + 3])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b1, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b2, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b3, OUT_ACT_MIN), OUT_ACT_MAX);
            }
            for (; ch < INPUT_CH; ++ch)
            {
                const int8_t *ip = ip0 + ch;
                const int8_t *kp = kernel + ch;

                const int8_t *in0 = ip;
                const int8_t *in1 = ip + INP_ROW_STRIDE;
                const int8_t *in2 = ip + 2 * INP_ROW_STRIDE;
                const int8_t *k0  = kp;
                const int8_t *k1  = kp + KER_ROW_STRIDE;
                const int8_t *k2  = kp + 2 * KER_ROW_STRIDE;

                int32_t b0 = bias[ch];

                b0 += ((int32_t)in0[0 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k0[0 * COL_STRIDE];
                b0 += ((int32_t)in0[1 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k0[1 * COL_STRIDE];
                b0 += ((int32_t)in0[2 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k0[2 * COL_STRIDE];
                b0 += ((int32_t)in1[0 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k1[0 * COL_STRIDE];
                b0 += ((int32_t)in1[1 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k1[1 * COL_STRIDE];
                b0 += ((int32_t)in1[2 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k1[2 * COL_STRIDE];
                b0 += ((int32_t)in2[0 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k2[0 * COL_STRIDE];
                b0 += ((int32_t)in2[1 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k2[1 * COL_STRIDE];
                b0 += ((int32_t)in2[2 * COL_STRIDE] + INPUT_OFFSET)
                      * (int32_t)k2[2 * COL_STRIDE];

                b0 = nn_requantize(b0, output_mult[ch], output_shift[ch])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
            }
        }

        for (int32_t out_w = int_w1; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w     = out_w * STRIDE_W - PAD_W;
            const bool    right_ok = (in_w + 2) < INPUT_W;
            const int8_t *inp_base
                = input + in_h * INP_ROW_STRIDE + in_w * COL_STRIDE;

            int32_t ch = 0;
            for (; ch <= (INPUT_CH - 4); ch += 4)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch + 0], b1 = bias[ch + 1];
                int32_t       b2 = bias[ch + 2], b3 = bias[ch + 3];

                acc_col_4ch(&b0, &b1, &b2, &b3, ip, kp, 0, KERNEL_DIM);
                acc_col_4ch(&b0,
                            &b1,
                            &b2,
                            &b3,
                            ip + COL_STRIDE,
                            kp + COL_STRIDE,
                            0,
                            KERNEL_DIM);
                if (right_ok)
                {
                    acc_col_4ch(&b0,
                                &b1,
                                &b2,
                                &b3,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                KERNEL_DIM);
                }

                b0 = nn_requantize(
                         b0, output_mult[ch + 0], output_shift[ch + 0])
                     + OUTPUT_OFFSET;
                b1 = nn_requantize(
                         b1, output_mult[ch + 1], output_shift[ch + 1])
                     + OUTPUT_OFFSET;
                b2 = nn_requantize(
                         b2, output_mult[ch + 2], output_shift[ch + 2])
                     + OUTPUT_OFFSET;
                b3 = nn_requantize(
                         b3, output_mult[ch + 3], output_shift[ch + 3])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b1, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b2, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b3, OUT_ACT_MIN), OUT_ACT_MAX);
            }
            for (; ch < INPUT_CH; ++ch)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch];

                acc_col_1ch(&b0, ip, kp, 0, KERNEL_DIM);
                acc_col_1ch(
                    &b0, ip + COL_STRIDE, kp + COL_STRIDE, 0, KERNEL_DIM);
                if (right_ok)
                {
                    acc_col_1ch(&b0,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                KERNEL_DIM);
                }

                b0 = nn_requantize(b0, output_mult[ch], output_shift[ch])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
            }
        }
    }

    for (int32_t out_h = int_h1; out_h < OUTPUT_H; ++out_h)
    {
        const int32_t in_h   = out_h * STRIDE_H - PAD_H;
        const int32_t kh_end = MIN(KERNEL_DIM, INPUT_H - in_h);

        for (int32_t out_w = 0; out_w < OUTPUT_W; ++out_w)
        {
            const int32_t in_w     = out_w * STRIDE_W - PAD_W;
            const int32_t kw_start = (in_w < 0) ? -in_w : 0;
            const bool    right_ok = (in_w + 2) < INPUT_W;
            const int8_t *inp_base
                = input + in_h * INP_ROW_STRIDE + in_w * COL_STRIDE;

            int32_t ch = 0;
            for (; ch <= (INPUT_CH - 4); ch += 4)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch + 0], b1 = bias[ch + 1];
                int32_t       b2 = bias[ch + 2], b3 = bias[ch + 3];

                if (kw_start == 0)
                {
                    acc_col_4ch(&b0, &b1, &b2, &b3, ip, kp, 0, kh_end);
                }
                acc_col_4ch(&b0,
                            &b1,
                            &b2,
                            &b3,
                            ip + COL_STRIDE,
                            kp + COL_STRIDE,
                            0,
                            kh_end);
                if (right_ok)
                {
                    acc_col_4ch(&b0,
                                &b1,
                                &b2,
                                &b3,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                kh_end);
                }

                b0 = nn_requantize(
                         b0, output_mult[ch + 0], output_shift[ch + 0])
                     + OUTPUT_OFFSET;
                b1 = nn_requantize(
                         b1, output_mult[ch + 1], output_shift[ch + 1])
                     + OUTPUT_OFFSET;
                b2 = nn_requantize(
                         b2, output_mult[ch + 2], output_shift[ch + 2])
                     + OUTPUT_OFFSET;
                b3 = nn_requantize(
                         b3, output_mult[ch + 3], output_shift[ch + 3])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b1, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b2, OUT_ACT_MIN), OUT_ACT_MAX);
                output[out_idx++]
                    = (int8_t)MIN(MAX(b3, OUT_ACT_MIN), OUT_ACT_MAX);
            }
            for (; ch < INPUT_CH; ++ch)
            {
                const int8_t *ip = inp_base + ch;
                const int8_t *kp = kernel + ch;
                int32_t       b0 = bias[ch];

                if (kw_start == 0)
                {
                    acc_col_1ch(&b0, ip, kp, 0, kh_end);
                }
                acc_col_1ch(&b0, ip + COL_STRIDE, kp + COL_STRIDE, 0, kh_end);
                if (right_ok)
                {
                    acc_col_1ch(&b0,
                                ip + 2 * COL_STRIDE,
                                kp + 2 * COL_STRIDE,
                                0,
                                kh_end);
                }

                b0 = nn_requantize(b0, output_mult[ch], output_shift[ch])
                     + OUTPUT_OFFSET;
                output[out_idx++]
                    = (int8_t)MIN(MAX(b0, OUT_ACT_MIN), OUT_ACT_MAX);
            }
        }
    }

    return 0;
}
