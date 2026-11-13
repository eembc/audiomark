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

#include "ee_audiomark.h"
#include "ee_api.h"

void
th_mat_vec_mult_f32(ee_matrix_f32_t *__EE_RESTRICT p_a,
                    ee_f32_t        *__EE_RESTRICT p_b,
                    ee_f32_t        *__EE_RESTRICT p_c)
{
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
