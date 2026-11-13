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

#ifndef _RISCV_DSP_H

#define _RISCV_DSP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "riscv_dsp_types.h" /* each port brings their own types.h */

#define RISCVBITREVINDEXTABLE_RADIX4_128_TABLE_LENGTH ((uint16_t)112)
    extern const uint16_t riscvBitRevIndexTable_r4_128
        [RISCVBITREVINDEXTABLE_RADIX4_128_TABLE_LENGTH];

#define RISCVBITREVINDEXTABLE_RADIX4_256_TABLE_LENGTH ((uint16_t)240)
    extern const uint16_t riscvBitRevIndexTable_r4_256
        [RISCVBITREVINDEXTABLE_RADIX4_256_TABLE_LENGTH];

#define RISCVBITREVINDEXTABLE_RADIX4_512_TABLE_LENGTH ((uint16_t)480)
    extern const uint16_t riscvBitRevIndexTable_r4_512
        [RISCVBITREVINDEXTABLE_RADIX4_512_TABLE_LENGTH];

#define RISCVBITREVINDEXTABLE_RADIX8_128_TABLE_LENGTH ((uint16_t)208)
    extern const uint16_t riscvBitRevIndexTable_r8_128
        [RISCVBITREVINDEXTABLE_RADIX8_128_TABLE_LENGTH];

#define RISCVBITREVINDEXTABLE_RADIX8_256_TABLE_LENGTH ((uint16_t)440)
    extern const uint16_t riscvBitRevIndexTable_r8_256
        [RISCVBITREVINDEXTABLE_RADIX8_256_TABLE_LENGTH];

#define RISCVBITREVINDEXTABLE_RADIX8_512_TABLE_LENGTH ((uint16_t)448)
    extern const uint16_t riscvBitRevIndexTable_r8_512
        [RISCVBITREVINDEXTABLE_RADIX8_512_TABLE_LENGTH];

    void riscv_cfft_f32(const riscv_cfft_instance *S,
                        float32_t                 *p1,
                        uint8_t                    ifftFlag,
                        uint8_t                    bitReverseFlag);

    riscv_status riscv_cfft_init_f32(riscv_cfft_instance *S, uint16_t fftLen);

    riscv_status riscv_rfft_fast_init_f32(riscv_rfft_fast_instance *S,
                                          uint16_t                  fftLen);

    void riscv_rfft_fast_f32(const riscv_rfft_fast_instance *S,
                             float32_t                      *p,
                             float32_t                      *pOut,

                             uint8_t ifftFlag);

    void riscv_cfft_q31(const riscv_cfft_instance *S,
                        q31_t                     *p1,
                        uint8_t                    ifftFlag,
                        uint8_t                    bitReverseFlag);

    riscv_status riscv_cfft_init_q31(riscv_cfft_instance *S, uint16_t fftLen);

    riscv_status riscv_rfft_fast_init_q31(riscv_rfft_fast_instance *S,
                                          uint16_t                  fftLen);

    void riscv_rfft_fast_q31(const riscv_rfft_fast_instance *S,
                             q31_t                          *p,
                             q31_t                          *pOut,
                             uint8_t                         ifftFlag);

#ifdef __cplusplus
}
#endif

#endif /*ifndef _RISCV_DSP_H*/
