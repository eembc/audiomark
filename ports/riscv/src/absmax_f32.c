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

#include <math.h>

void
th_absmax_f32(const ee_f32_t *__EE_RESTRICT p_in,
              uint32_t                     len,
              ee_f32_t       *__EE_RESTRICT p_max,
              uint32_t       *__EE_RESTRICT p_index)
{
    ee_f32_t max_val = fabsf(p_in[0]);
    uint32_t max_idx = 0;

    for (uint32_t i = 1; i < len; i++)
    {
        ee_f32_t val = fabsf(p_in[i]);

        if (val > max_val)
        {
            max_val = val;
            max_idx = i;
        }
    }

    *p_max   = max_val;
    *p_index = max_idx;
}
