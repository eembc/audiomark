// Modifications copyright (C) 2024 Chair of Electronic Design Automation, TUM
/*
 * SPDX-FileCopyrightText: Copyright 2010-2024 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
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
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#ifndef RVV_SUPPORT_FUNCTIONS_H
#define RVV_SUPPORT_FUNCTIONS_H

#include "rvv_support_guard.h"
#include "support_functions.h"
#include "ee_api.h"

q7_t *nn_mat_mult_core_1x1_s8(const q7_t *__EE_RESTRICT          input_a,
                              const q7_t *__EE_RESTRICT          act_row,
                              const int32_t *__EE_RESTRICT       out_shift,
                              const int32_t *__EE_RESTRICT       out_mult,
                              const int32_t *const __EE_RESTRICT bias,
                              q7_t *__EE_RESTRICT                out);

q7_t *nn_mat_mult_kernel_s8_s8(const q7_t *__EE_RESTRICT          input_a,
                               const q7_t *__EE_RESTRICT          input_b,
                               const int32_t *__EE_RESTRICT       out_shift,
                               const int32_t *__EE_RESTRICT       out_mult,
                               const int32_t *const __EE_RESTRICT output_bias,
                               q7_t *__EE_RESTRICT                out_0);

static inline vint32m4_t
nn_requantize_vint32m4(const vint32m4_t val,
                       const q31_t      multiplier,
                       const q31_t      shift,
                       size_t           vl)
{
    vint32m4_t val_internal = __riscv_vsmul_vx_i32m4(
        val, multiplier * (1 << LEFT_SHIFT(shift)), __RISCV_VXRM_RNU, vl);

    const q31_t remainder_mask = (1 << RIGHT_SHIFT(shift)) - 1;
    vint32m4_t  remainder
        = __riscv_vand_vx_i32m4(val_internal, remainder_mask, vl);
    vint32m4_t threshold = __riscv_vmv_v_x_i32m4(remainder_mask >> 1, vl);

    val_internal = __riscv_vsra_vx_i32m4(val_internal, RIGHT_SHIFT(shift), vl);

    vbool8_t mask = __riscv_vmslt_vx_i32m4_b8(val_internal, 0, vl);
    threshold = __riscv_vadd_vx_i32m4_tum(mask, threshold, threshold, 1, vl);

    mask = __riscv_vmsgt_vv_i32m4_b8(remainder, threshold, vl);
    val_internal
        = __riscv_vadd_vx_i32m4_tum(mask, val_internal, val_internal, 1, vl);

    return val_internal;
}

#endif
