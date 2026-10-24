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
th_cmplx_dot_prod_f32(const ee_f32_t *__EE_RESTRICT p_a,
                      const ee_f32_t *__EE_RESTRICT p_b,
                      uint32_t                      len,
                      ee_f32_t       *__EE_RESTRICT p_r,
                      ee_f32_t       *__EE_RESTRICT p_i)
{
    ee_f32_t real_sum = 0.0f;
    ee_f32_t imag_sum = 0.0f;

    for (uint32_t i = 0; i < len; i++)
    {
        ee_f32_t ar = p_a[2 * i];
        ee_f32_t ai = p_a[2 * i + 1];
        ee_f32_t br = p_b[2 * i];
        ee_f32_t bi = p_b[2 * i + 1];

        real_sum += ar * br - ai * bi;
        imag_sum += ar * bi + ai * br;
    }

    *p_r = real_sum;
    *p_i = imag_sum;
}
