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
th_f32_to_int16(const ee_f32_t *__EE_RESTRICT p_src,
                int16_t        *__EE_RESTRICT p_dst,
                uint32_t                      len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        ee_f32_t x = p_src[i];

        if (x > 32767.0f)
        {
            x = 32767.0f;
        }
        else if (x < -32768.0f)
        {
            x = -32768.0f;
        }
        p_dst[i] = (int16_t)x;
    }
}
