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

/*
 * s8 vector(lhs) by matrix (transposed) multiplication
 */
int32_t
nn_vec_mat_mult_t_s8(const q7_t   *__EE_RESTRICT lhs,
                     const q7_t   *__EE_RESTRICT rhs,
                     const q31_t  *__EE_RESTRICT bias,
                     q7_t         *__EE_RESTRICT dst,
                     const int32_t lhs_offset,
                     const int32_t dst_offset,
                     const int32_t dst_multiplier,
                     const int32_t dst_shift,
                     const int32_t rhs_cols,
                     const int32_t rhs_rows,
                     const int32_t activation_min,
                     const int32_t activation_max,
                     const int32_t address_offset)
{
    /* Uses 5x loop unrolling in order to expose more ILP */
    const int32_t row_loop_cnt = rhs_rows / 5;
    for (int i_row_loop_cnt = 0; i_row_loop_cnt < row_loop_cnt;
         i_row_loop_cnt++)
    {
        const q7_t *lhs_ptr   = lhs;
        const q7_t *rhs_ptr_0 = &rhs[0];
        const q7_t *rhs_ptr_1 = &rhs[rhs_cols];
        const q7_t *rhs_ptr_2 = &rhs[rhs_cols * 2];
        const q7_t *rhs_ptr_3 = &rhs[rhs_cols * 3];
        const q7_t *rhs_ptr_4 = &rhs[rhs_cols * 4];

        q31_t res00 = 0;
        q31_t res01 = 0;
        q31_t res02 = 0;
        q31_t res03 = 0;
        q31_t res04 = 0;
        if (bias)
        {
            res00 = *bias++;
            res01 = *bias++;
            res02 = *bias++;
            res03 = *bias++;
            res04 = *bias++;
        }
        for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx)
        {
            const q31_t rhs_value0 = (int8_t)*rhs_ptr_0;
            const q31_t rhs_value1 = (int8_t)*rhs_ptr_1;
            const q31_t rhs_value2 = (int8_t)*rhs_ptr_2;
            const q31_t rhs_value3 = (int8_t)*rhs_ptr_3;
            const q31_t rhs_value4 = (int8_t)*rhs_ptr_4;
            const q31_t lhs_value  = (int8_t)*lhs_ptr + lhs_offset;

            res00 += lhs_value * rhs_value0;
            res01 += lhs_value * rhs_value1;
            res02 += lhs_value * rhs_value2;
            res03 += lhs_value * rhs_value3;
            res04 += lhs_value * rhs_value4;

            ++rhs_ptr_0;
            ++rhs_ptr_1;
            ++rhs_ptr_2;
            ++rhs_ptr_3;
            ++rhs_ptr_4;
            ++lhs_ptr;
        }
        /* Quantize down */
        res00 = nn_requantize(res00, dst_multiplier, dst_shift);
        res01 = nn_requantize(res01, dst_multiplier, dst_shift);
        res02 = nn_requantize(res02, dst_multiplier, dst_shift);
        res03 = nn_requantize(res03, dst_multiplier, dst_shift);
        res04 = nn_requantize(res04, dst_multiplier, dst_shift);

        /* Add offset */
        res00 += dst_offset;
        res01 += dst_offset;
        res02 += dst_offset;
        res03 += dst_offset;
        res04 += dst_offset;

        /* Clamp the result */
        res00 = MAX(res00, activation_min);
        res00 = MIN(res00, activation_max);
        res01 = MAX(res01, activation_min);
        res01 = MIN(res01, activation_max);
        res02 = MAX(res02, activation_min);
        res02 = MIN(res02, activation_max);
        res03 = MAX(res03, activation_min);
        res03 = MIN(res03, activation_max);
        res04 = MAX(res04, activation_min);
        res04 = MIN(res04, activation_max);

        *dst                        = (q7_t)res00;
        *(dst + address_offset)     = (q7_t)res01;
        *(dst + 2 * address_offset) = (q7_t)res02;
        *(dst + 3 * address_offset) = (q7_t)res03;
        *(dst + 4 * address_offset) = (q7_t)res04;
        dst += 5 * address_offset;

        rhs += 5 * rhs_cols;
    }

    /* Handle the leftover part from 5x loop unrolling */
    const int loop_cnt = rhs_rows % 5;
    for (int i_loop_cnt = 0; i_loop_cnt < loop_cnt; i_loop_cnt++)
    {

        const q7_t *lhs_ptr = &lhs[0];
        const q7_t *rhs_ptr = &rhs[0];

        q31_t res00 = 0;
        if (bias)
        {
            res00 = *bias++;
        }

        for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx)
        {
            q31_t rhs_value0 = (int8_t)rhs_ptr[0];
            q31_t lhs_value  = (int8_t)lhs_ptr[0] + lhs_offset;

            res00 += lhs_value * rhs_value0;

            ++rhs_ptr;
            ++lhs_ptr;
        }

        /* Quantize down */
        res00 = nn_requantize(res00, dst_multiplier, dst_shift);

        /* Add offset */
        res00 += dst_offset;

        /* Clamp the result */
        res00 = MAX(res00, activation_min);
        res00 = MIN(res00, activation_max);

        *dst = (int8_t)res00;
        dst += address_offset;
        rhs += rhs_cols;
    }
    return 0;
}
