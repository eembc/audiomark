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

#include "math_types.h"
#include "functions.h"
#include "ee_api.h"

#include <stdint.h>

/*
 * Specialized for KWS model:
 * input: 25x5x64
 * output: 1x1x64
 */

#define INPUT_Y  25
#define INPUT_X  5
#define CHANNELS 64
#define COUNT    (INPUT_Y * INPUT_X)
#define HALF     (COUNT / 2)

#define ROUND_SYM(s) ((s) >= 0 ? ((s) + HALF) / COUNT : ((s) - HALF) / COUNT)

int32_t
nn_avgpool_25x5x64_s8(const q7_t *__EE_RESTRICT input_data,
                      q7_t *__EE_RESTRICT       output_data)
{
    for (uint8_t c = 0; c < CHANNELS; c += 16)
    {
        int32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        int32_t s4 = 0, s5 = 0, s6 = 0, s7 = 0;
        int32_t s8 = 0, s9 = 0, s10 = 0, s11 = 0;
        int32_t s12 = 0, s13 = 0, s14 = 0, s15 = 0;

        const q7_t *src = input_data + c;

        for (uint8_t idx = 0; idx < COUNT; idx++)
        {
            s0 += src[0];
            s1 += src[1];
            s2 += src[2];
            s3 += src[3];
            s4 += src[4];
            s5 += src[5];
            s6 += src[6];
            s7 += src[7];
            s8 += src[8];
            s9 += src[9];
            s10 += src[10];
            s11 += src[11];
            s12 += src[12];
            s13 += src[13];
            s14 += src[14];
            s15 += src[15];
            src += CHANNELS;
        }

        output_data[c + 0]  = (q7_t)ROUND_SYM(s0);
        output_data[c + 1]  = (q7_t)ROUND_SYM(s1);
        output_data[c + 2]  = (q7_t)ROUND_SYM(s2);
        output_data[c + 3]  = (q7_t)ROUND_SYM(s3);
        output_data[c + 4]  = (q7_t)ROUND_SYM(s4);
        output_data[c + 5]  = (q7_t)ROUND_SYM(s5);
        output_data[c + 6]  = (q7_t)ROUND_SYM(s6);
        output_data[c + 7]  = (q7_t)ROUND_SYM(s7);
        output_data[c + 8]  = (q7_t)ROUND_SYM(s8);
        output_data[c + 9]  = (q7_t)ROUND_SYM(s9);
        output_data[c + 10] = (q7_t)ROUND_SYM(s10);
        output_data[c + 11] = (q7_t)ROUND_SYM(s11);
        output_data[c + 12] = (q7_t)ROUND_SYM(s12);
        output_data[c + 13] = (q7_t)ROUND_SYM(s13);
        output_data[c + 14] = (q7_t)ROUND_SYM(s14);
        output_data[c + 15] = (q7_t)ROUND_SYM(s15);
    }

    return 0;
}
