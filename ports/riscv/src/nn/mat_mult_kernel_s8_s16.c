
/*
 * Copyright (C) 2010-2022 Arm Limited or its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications copyright (C) 2021-2023 Chair of Electronic Design Automation,
 * TUM
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "support_functions.h"
#include "ee_api.h"

/*
 * Matrix-multiplication function for convolution with per-channel
 * requantization.
 */
q7_t *
nn_mat_mult_kernel_s8_s16(const q7_t          *__EE_RESTRICT input_a,
                          const q15_t         *__EE_RESTRICT input_b,
                          const uint16_t                     output_ch,
                          const int32_t       *__EE_RESTRICT out_shift,
                          const int32_t       *__EE_RESTRICT out_mult,
                          const int32_t                      out_offset,
                          const int16_t                      activation_min,
                          const int16_t                      activation_max,
                          const uint16_t                     num_col_a,
                          const int32_t *const __EE_RESTRICT output_bias,
                          q7_t                *__EE_RESTRICT out_0)
{
    /* set up the second output pointers */
    q7_t          *out_1 = out_0 + output_ch;
    const int32_t *bias  = output_bias;

    uint16_t row_count = output_ch >> 2;

    const q7_t *ip_a0 = input_a;
    /* this loop over rows in A */
    while (row_count)
    {
        /* setup pointers for B */
        const q15_t *ip_b0 = input_b;
        const q15_t *ip_b1 = ip_b0 + num_col_a;

        /* align the pointers for A */
        const q7_t *ip_a1 = ip_a0 + num_col_a;
        const q7_t *ip_a2 = ip_a1 + num_col_a;
        const q7_t *ip_a3 = ip_a2 + num_col_a;

        q31_t ch_0_out_0 = 0;
        q31_t ch_0_out_1 = 0;

        q31_t ch_1_out_0 = 0;
        q31_t ch_1_out_1 = 0;

        q31_t ch_2_out_0 = 0;
        q31_t ch_2_out_1 = 0;

        q31_t ch_3_out_0 = 0;
        q31_t ch_3_out_1 = 0;
        /* Init accumulator with bias for channel N and N + 1 */
        if (bias)
        {
            ch_0_out_0 = *bias;
            ch_0_out_1 = *bias++;

            ch_1_out_0 = *bias;
            ch_1_out_1 = *bias++;

            ch_2_out_0 = *bias;
            ch_2_out_1 = *bias++;

            ch_3_out_0 = *bias;
            ch_3_out_1 = *bias++;
        }

        uint16_t col_count = num_col_a;

        while (col_count)
        {

            q7_t  a0 = *ip_a0++;
            q15_t b0 = *ip_b0++;
            q7_t  a1 = *ip_a1++;
            q15_t b1 = *ip_b1++;

            q7_t a2 = *ip_a2++;
            q7_t a3 = *ip_a3++;

            ch_0_out_0 += a0 * b0;
            ch_0_out_1 += a0 * b1;
            ch_1_out_0 += a1 * b0;
            ch_1_out_1 += a1 * b1;

            ch_2_out_0 += a2 * b0;
            ch_2_out_1 += a2 * b1;
            ch_3_out_0 += a3 * b0;
            ch_3_out_1 += a3 * b1;

            col_count--;

        } /* while over col_count */

        ch_0_out_0 = nn_requantize(ch_0_out_0, *out_mult, *out_shift);
        ch_0_out_0 += out_offset;
        ch_0_out_0 = MAX(ch_0_out_0, activation_min);
        ch_0_out_0 = MIN(ch_0_out_0, activation_max);
        *out_0++   = (q7_t)ch_0_out_0;

        ch_0_out_1 = nn_requantize(ch_0_out_1, *out_mult, *out_shift);
        ch_0_out_1 += out_offset;
        ch_0_out_1 = MAX(ch_0_out_1, activation_min);
        ch_0_out_1 = MIN(ch_0_out_1, activation_max);
        *out_1++   = (q7_t)ch_0_out_1;
        out_mult++;
        out_shift++;

        ch_1_out_0 = nn_requantize(ch_1_out_0, *out_mult, *out_shift);
        ch_1_out_0 += out_offset;
        ch_1_out_0 = MAX(ch_1_out_0, activation_min);
        ch_1_out_0 = MIN(ch_1_out_0, activation_max);
        *out_0++   = (q7_t)ch_1_out_0;

        ch_1_out_1 = nn_requantize(ch_1_out_1, *out_mult, *out_shift);
        ch_1_out_1 += out_offset;
        ch_1_out_1 = MAX(ch_1_out_1, activation_min);
        ch_1_out_1 = MIN(ch_1_out_1, activation_max);
        *out_1++   = (q7_t)ch_1_out_1;
        out_mult++;
        out_shift++;

        ch_2_out_0 = nn_requantize(ch_2_out_0, *out_mult, *out_shift);
        ch_2_out_0 += out_offset;
        ch_2_out_0 = MAX(ch_2_out_0, activation_min);
        ch_2_out_0 = MIN(ch_2_out_0, activation_max);
        *out_0++   = (q7_t)ch_2_out_0;

        ch_2_out_1 = nn_requantize(ch_2_out_1, *out_mult, *out_shift);
        ch_2_out_1 += out_offset;
        ch_2_out_1 = MAX(ch_2_out_1, activation_min);
        ch_2_out_1 = MIN(ch_2_out_1, activation_max);
        *out_1++   = (q7_t)ch_2_out_1;
        out_mult++;
        out_shift++;

        ch_3_out_0 = nn_requantize(ch_3_out_0, *out_mult, *out_shift);
        ch_3_out_0 += out_offset;
        ch_3_out_0 = MAX(ch_3_out_0, activation_min);
        ch_3_out_0 = MIN(ch_3_out_0, activation_max);
        *out_0++   = (q7_t)ch_3_out_0;

        ch_3_out_1 = nn_requantize(ch_3_out_1, *out_mult, *out_shift);
        ch_3_out_1 += out_offset;
        ch_3_out_1 = MAX(ch_3_out_1, activation_min);
        ch_3_out_1 = MIN(ch_3_out_1, activation_max);
        *out_1++   = (q7_t)ch_3_out_1;
        out_mult++;
        out_shift++;

        /* skip rows */
        ip_a0 += (num_col_a * 3);
        row_count--;
    }

    /* compute the last three rows if any */
    uint8_t remaining = output_ch % 4;
    while (remaining)
    {
        /* setup pointers for B */
        const q15_t *ip_b0 = input_b;
        const q15_t *ip_b1 = ip_b0 + num_col_a;

        q31_t ch_0_out_0 = 0;
        q31_t ch_0_out_1 = 0;

        /* load the bias */
        if (bias)
        {
            ch_0_out_0 = *bias;
            ch_0_out_1 = *bias++;
        }

        uint16_t col_count = num_col_a;

        while (col_count)
        {
            q7_t  a0 = *ip_a0++;
            q15_t b0 = *ip_b0++;
            q15_t b1 = *ip_b1++;

            ch_0_out_0 += a0 * b0;
            ch_0_out_1 += a0 * b1;
            col_count--;
        }
        ch_0_out_0 = nn_requantize(ch_0_out_0, *out_mult, *out_shift);
        ch_0_out_0 += out_offset;
        ch_0_out_0 = MAX(ch_0_out_0, activation_min);
        ch_0_out_0 = MIN(ch_0_out_0, activation_max);
        *out_0++   = (q7_t)ch_0_out_0;

        ch_0_out_1 = nn_requantize(ch_0_out_1, *out_mult, *out_shift);
        ch_0_out_1 += out_offset;
        ch_0_out_1 = MAX(ch_0_out_1, activation_min);
        ch_0_out_1 = MIN(ch_0_out_1, activation_max);
        *out_1++   = (q7_t)ch_0_out_1;
        out_mult++;
        out_shift++;
        remaining--;
    }

    out_0 += output_ch;

    /* return the new output pointer with offset */
    return out_0;
}
