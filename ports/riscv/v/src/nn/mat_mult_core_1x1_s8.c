/*
 * Copyright (C) 2010-2022 Arm Limited or its affiliates.
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
 * Modifications copyright (C) 2021-2022 Chair of Electronic Design Automation,
 * TUM
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "rvv_support_guard.h"
#include "rvv_support_functions.h"

#define OUTPUT_CH  64
#define OUT_OFFSET (-128)
#define ACT_MIN    (-128)
#define ACT_MAX    127
#define NUM_COL_A  64

q7_t *
nn_mat_mult_core_1x1_s8(const q7_t *__EE_RESTRICT          input_a,
                        const q7_t *__EE_RESTRICT          act_row,
                        const int32_t *__EE_RESTRICT       out_shift,
                        const int32_t *__EE_RESTRICT       out_mult,
                        const int32_t *const __EE_RESTRICT bias,
                        q7_t *__EE_RESTRICT                out)
{
    const q7_t    *ip_a      = input_a;
    const int32_t *bias_ptr  = bias;
    const int32_t *mult_ptr  = out_mult;
    const int32_t *shift_ptr = out_shift;
    size_t         remaining = OUTPUT_CH;

    while (remaining > 0)
    {
        size_t     vl   = __riscv_vsetvl_e32m8(remaining);
        vint32m8_t vacc = __riscv_vle32_v_i32m8(bias_ptr, vl);
        bias_ptr += vl;

        for (uint16_t k = 0; k < NUM_COL_A; k++)
        {
            vint8m2_t va8 = __riscv_vle8_v_i8m2(
                (const int8_t *)(ip_a + (size_t)k * OUTPUT_CH), vl);
            vint16m4_t va16 = __riscv_vsext_vf2_i16m4(va8, vl);
            int16_t    b    = (int16_t)act_row[k];
            vacc            = __riscv_vwmacc_vx_i32m8(vacc, b, va16, vl);
        }

        vint32m8_t vmult   = __riscv_vle32_v_i32m8(mult_ptr, vl);
        vint32m8_t vshift  = __riscv_vle32_v_i32m8(shift_ptr, vl);
        vint32m8_t vleft_s = __riscv_vmax_vx_i32m8(vshift, 0, vl);
        vint32m8_t vright_s
            = __riscv_vmax_vx_i32m8(__riscv_vneg_v_i32m8(vshift, vl), 0, vl);
        vuint32m8_t vleft_u  = __riscv_vreinterpret_v_i32m8_u32m8(vleft_s);
        vuint32m8_t vright_u = __riscv_vreinterpret_v_i32m8_u32m8(vright_s);
        vacc                 = __riscv_vsll_vv_i32m8(vacc, vleft_u, vl);
        vint32m8_t vres
            = __riscv_vsmul_vv_i32m8(vacc, vmult, __RISCV_VXRM_RNU, vl);
        vres = __riscv_vssra_vv_i32m8(vres, vright_u, __RISCV_VXRM_RNU, vl);
        vres = __riscv_vadd_vx_i32m8(vres, OUT_OFFSET, vl);
        vint16m4_t vout16
            = __riscv_vnclip_wx_i16m4(vres, 0, __RISCV_VXRM_RNU, vl);
        vout16 = __riscv_vmax_vx_i16m4(vout16, ACT_MIN, vl);
        vout16 = __riscv_vmin_vx_i16m4(vout16, ACT_MAX, vl);
        vint8m2_t vout8
            = __riscv_vnclip_wx_i8m2(vout16, 0, __RISCV_VXRM_RNU, vl);
        __riscv_vse8_v_i8m2((int8_t *)out, vout8, vl);

        out += vl;
        mult_ptr += vl;
        shift_ptr += vl;
        ip_a += vl;
        remaining -= vl;
    }

    return out;
}
