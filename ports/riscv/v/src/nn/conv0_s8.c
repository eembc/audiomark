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
 * Modifications copyright (C) 2021-2024 Chair of Electronic Design Automation,
 * TUM
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "functions.h"
#include "support_functions.h"
#include "ee_api.h"
#include "convolve_config.h"

#include <stdint.h>

#define INPUT_X   10
#define INPUT_Y   49
#define INPUT_CH  1
#define KERNEL_X  4
#define KERNEL_Y  10
#define OUTPUT_X  5
#define OUTPUT_Y  25
#define OUTPUT_CH 64
#define STRIDE_X  2
#define STRIDE_Y  2
#define PAD_X     1
#define PAD_Y     4

#define INPUT_OFFSET (-100)
#define OUT_OFFSET   (-128)
#define OUT_ACT_MIN  (-128)
#define OUT_ACT_MAX  127

int32_t
nn_conv0_s8(const nn_context *__EE_RESTRICT                  ctx,
            const nn_per_channel_quant_params *__EE_RESTRICT quant_params,
            const q7_t *__EE_RESTRICT                        input_data,
            const q7_t *__EE_RESTRICT                        filter_data,
            const int32_t *__EE_RESTRICT                     bias_data,
            q7_t *__EE_RESTRICT                              output_data)
{
    q15_t *__EE_RESTRICT const im2col_buf = (q15_t *)ctx->buf;

    const int32_t *__EE_RESTRICT output_mult  = quant_params->multiplier;
    const int32_t *__EE_RESTRICT output_shift = quant_params->shift;

    q15_t *__EE_RESTRICT const col_buf_full
        = im2col_buf
          + (int32_t)NN_KERNEL_COLS * (INPUT_CH * KERNEL_Y * KERNEL_X);

    q15_t *__EE_RESTRICT col_buf = im2col_buf;
    q7_t *__EE_RESTRICT  out     = output_data;

    for (int32_t i_out_y = 0; i_out_y < OUTPUT_Y; i_out_y++)
    {
        for (int32_t i_out_x = 0; i_out_x < OUTPUT_X; i_out_x++)
        {
            const int32_t base_idx_y = STRIDE_Y * i_out_y - PAD_Y;
            const int32_t base_idx_x = STRIDE_X * i_out_x - PAD_X;

            for (int32_t i_ker_y = 0; i_ker_y < KERNEL_Y; i_ker_y++)
            {
                for (int32_t i_ker_x = 0; i_ker_x < KERNEL_X; i_ker_x++)
                {
                    // Dilation is fixed at 1, so no multiply is needed here.
                    const int32_t k_y = base_idx_y + i_ker_y;
                    const int32_t k_x = base_idx_x + i_ker_x;

                    if (k_y < 0 || k_y >= INPUT_Y || k_x < 0 || k_x >= INPUT_X)
                    {
                        th_memset(
                            (int8_t *)col_buf, 0, sizeof(q15_t) * INPUT_CH);
                    }
                    else
                    {
                        nn_q7_to_q15_with_offset(
                            input_data + (k_y * INPUT_X + k_x) * INPUT_CH,
                            col_buf,
                            INPUT_CH,
                            INPUT_OFFSET);
                    }
                    col_buf += INPUT_CH;
                }
            }

            if (col_buf == col_buf_full)
            {
                out = nn_mat_mult_kernel_s8_s16(filter_data,
                                                im2col_buf,
                                                OUTPUT_CH,
                                                output_shift,
                                                output_mult,
                                                OUT_OFFSET,
                                                OUT_ACT_MIN,
                                                OUT_ACT_MAX,
                                                INPUT_CH * KERNEL_Y * KERNEL_X,
                                                bias_data,
                                                out);

                col_buf = im2col_buf;
            }
        }
    }

    if (col_buf != im2col_buf)
    {
        // num_col_a declared locally — dead during the hot loop above
        const uint16_t num_col_a       = INPUT_CH * KERNEL_Y * KERNEL_X;
        const int32_t  leftover_pixels = (OUTPUT_X * OUTPUT_Y) % NN_KERNEL_COLS;
        const q15_t *__EE_RESTRICT patch = im2col_buf;

        for (int32_t p = 0; p < leftover_pixels; p++, patch += num_col_a)
        {
            for (int32_t i = 0; i < OUTPUT_CH; i++)
            {
                q31_t                      sum = bias_data ? bias_data[i] : 0;
                const q15_t *__EE_RESTRICT col = patch;
                const q7_t *__EE_RESTRICT  ker_a
                    = filter_data + i; /* k=0, channel i, transposed layout */
                uint16_t col_count = num_col_a;

                while (col_count--)
                {
                    sum += (*ker_a) * (*col++);
                    ker_a += OUTPUT_CH; /* step to next k */
                }

                sum = nn_requantize(sum, output_mult[i], output_shift[i]);
                sum += OUT_OFFSET;
                sum    = MAX(sum, OUT_ACT_MIN);
                sum    = MIN(sum, OUT_ACT_MAX);
                *out++ = (q7_t)sum;
            }
        }
    }

    input_data += INPUT_X * INPUT_Y * INPUT_CH;
    output_data += OUTPUT_X * OUTPUT_Y * OUTPUT_CH;

    return 0;
}
