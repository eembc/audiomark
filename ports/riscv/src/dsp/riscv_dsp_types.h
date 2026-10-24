/*
 * Copyright 2026 Harshit Kumar Shivhare
 * Copyright (c) 2010-2021 Arm Limited or its affiliates. All rights reserved.
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
 */

#ifndef _DSP_TYPES_H

#define _DSP_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    typedef int32_t q31_t;

    typedef enum
    {
        RISCV_MATH_SUCCESS = 0,
        RISCV_MATH_ERROR   = -1,
    } riscv_status;

    typedef float float32_t;

    typedef struct
    {
        uint16_t         fftLen;   /**< length of the FFT. */
        const float32_t *pTwiddle; /**< points to the Twiddle factor table. */
        const uint16_t  *pBitRevTable; /**< points to the bit reversal table. */
        uint16_t         bitRevLength; /**< bit reversal table length. */
    } riscv_cfft_instance_f32;

    typedef struct
    {
        riscv_cfft_instance_f32 Sint;       /**< Internal CFFT structure. */
        uint16_t                fftLenRFFT; /**< length of the real sequence */
        const float32_t *pTwiddleRFFT;      /**< Twiddle factors real stage  */
    } riscv_rfft_fast_instance_f32;

typedef riscv_cfft_instance_f32 riscv_cfft_instance;

typedef riscv_rfft_fast_instance_f32 riscv_rfft_fast_instance;

#ifdef __cplusplus
}
#endif

#endif /* ifndef _DSP_TYPES_H*/
