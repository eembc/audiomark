/**
 * Copyright 2026 Harshit Kumar Shivhare
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
#include "riscv_dsp.h"

ee_status_t
th_rfft_init_f32(ee_rfft_f32_t *p_instance, int fft_length)
{
    if (fft_length != 1024)
    {
        return EE_STATUS_ERROR;
    }

    riscv_status status = riscv_rfft_fast_init_f32(p_instance, fft_length);
    if (!status)
    {
        return EE_STATUS_ERROR;
    }
    return EE_STATUS_OK;
}
