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

#include "rvv_support_guard.h"
#include "math_types.h"
#include "functions.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Specialized for the KWS model:
 * input: 25x5x64, global avgpool
 */
#define INPUT_Y  25
#define INPUT_X  5
#define CHANNELS 64
#define COUNT    (INPUT_Y * INPUT_X)
#define HALF     (COUNT / 2)

int32_t
nn_avgpool_25x5x64_s8(const q7_t *input_data, q7_t *output_data)
{
    int16_t tmp[CHANNELS];

    int32_t c = 0;

    while (c < CHANNELS)
    {
        /* use m2 loads and m4 accumulation */
        size_t vl = __riscv_vsetvl_e8m2(CHANNELS - c);

        /* accumulator */
        vint16m4_t vsum = __riscv_vmv_v_x_i16m4(0, vl);

        const q7_t *base = input_data + c;

        for (int32_t idx = 0; idx < COUNT; idx++)
        {
            /* load activations in i8m2 */
            vint8m2_t v8 = __riscv_vle8_v_i8m2(base, vl);

            /* widening accumulate into i16m4 */
            vsum = __riscv_vwadd_wv_i16m4(vsum, v8, vl);

            base += CHANNELS;
        }

        __riscv_vse16_v_i16m4(tmp + c, vsum, vl);

        c += vl;
    }

    /* scalar finalize loop */
    for (int32_t i = 0; i < CHANNELS; i++)
    {
        int32_t sum = tmp[i];

        if (sum > 0)
        {
            sum += HALF;
        }
        else
        {
            sum -= HALF;
        }

        output_data[i] = (q7_t)(sum / COUNT);
    }

    return 0;
}
