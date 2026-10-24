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
#include "rvv_support_functions.h"

q7_t *
nn_mat_mult_kernel_s8_s16(const q7_t          *input_a,
                          const q15_t         *input_b,
                          const uint16_t       output_ch,
                          const int32_t       *out_shift,
                          const int32_t       *out_mult,
                          const int32_t        out_offset,
                          const int16_t        activation_min,
                          const int16_t        activation_max,
                          const uint16_t       num_col_a,
                          const int32_t *const output_bias,
                          q7_t                *out_0)
{
    /* 7-row output shape: LMUL=4 for i32 accumulators,
     * LMUL=2 for i16 operands, and 7 scalar LHS values broadcast per
     * inner-loop iteration.  Only the single LMUL=2 RHS register is
     * reloaded each k-iteration; all 7 LMUL=4 accumulators are stationary. */

    q7_t *out_1 = out_0 + output_ch;
    q7_t *out_2 = out_1 + output_ch;
    q7_t *out_3 = out_2 + output_ch;
    q7_t *out_4 = out_3 + output_ch;
    q7_t *out_5 = out_4 + output_ch;
    q7_t *out_6 = out_5 + output_ch;

    const q15_t   *ip_b0      = input_b;
    const q15_t   *ip_b1      = ip_b0 + num_col_a;
    const q15_t   *ip_b2      = ip_b1 + num_col_a;
    const q15_t   *ip_b3      = ip_b2 + num_col_a;
    const q15_t   *ip_b4      = ip_b3 + num_col_a;
    const q15_t   *ip_b5      = ip_b4 + num_col_a;
    const q15_t   *ip_b6      = ip_b5 + num_col_a;
    const q7_t    *ip_a_strip = input_a;
    const int32_t *bias_ptr   = output_bias;
    const int32_t *mult_ptr   = out_mult;
    const int32_t *shift_ptr  = out_shift;
    size_t         remaining  = output_ch;

    while (remaining > 0)
    {
        size_t vl = __riscv_vsetvl_e32m4(remaining);

        /* Initialise accumulators with bias or zero */
        vint32m4_t vacc_0, vacc_1, vacc_2, vacc_3, vacc_4, vacc_5, vacc_6;
        if (output_bias)
        {
            vint32m4_t vbias = __riscv_vle32_v_i32m4(bias_ptr, vl);
            vacc_0           = vbias;
            vacc_1           = vbias;
            vacc_2           = vbias;
            vacc_3           = vbias;
            vacc_4           = vbias;
            vacc_5           = vbias;
            vacc_6           = vbias;
            bias_ptr += vl;
        }
        else
        {
            vacc_0 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_1 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_2 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_3 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_4 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_5 = __riscv_vmv_v_x_i32m4(0, vl);
            vacc_6 = __riscv_vmv_v_x_i32m4(0, vl);
        }

        /* Dot product */
        for (uint16_t k = 0; k < num_col_a; k++)
        {
            /* Gather column k across vl output-channel rows; stride = num_col_a
             * bytes*/
            vint8m1_t va_i8 = __riscv_vle8_v_i8m1(
                (const int8_t *)(ip_a_strip + (size_t)k * output_ch), vl);
            vint16m2_t va_i16 = __riscv_vsext_vf2_i16m2(va_i8, vl);

            /* 7 scalar LHS values — one per output row */
            int16_t b0 = (int16_t)ip_b0[k];
            int16_t b1 = (int16_t)ip_b1[k];
            int16_t b2 = (int16_t)ip_b2[k];
            int16_t b3 = (int16_t)ip_b3[k];
            int16_t b4 = (int16_t)ip_b4[k];
            int16_t b5 = (int16_t)ip_b5[k];
            int16_t b6 = (int16_t)ip_b6[k];

            /* i16*i16 -> i32 widening MAC; b0..b6 broadcast as scalars */
            vacc_0 = __riscv_vwmacc_vx_i32m4(vacc_0, b0, va_i16, vl);
            vacc_1 = __riscv_vwmacc_vx_i32m4(vacc_1, b1, va_i16, vl);
            vacc_2 = __riscv_vwmacc_vx_i32m4(vacc_2, b2, va_i16, vl);
            vacc_3 = __riscv_vwmacc_vx_i32m4(vacc_3, b3, va_i16, vl);
            vacc_4 = __riscv_vwmacc_vx_i32m4(vacc_4, b4, va_i16, vl);
            vacc_5 = __riscv_vwmacc_vx_i32m4(vacc_5, b5, va_i16, vl);
            vacc_6 = __riscv_vwmacc_vx_i32m4(vacc_6, b6, va_i16, vl);
        }

        /* Replicate nn_requantize(val, mult, shift) in-vector.
         * mult/shift vectors are loaded once and shared across all 7 rows. */
        vint32m4_t vmult  = __riscv_vle32_v_i32m4(mult_ptr, vl);
        vint32m4_t vshift = __riscv_vle32_v_i32m4(shift_ptr, vl);

        /* left_shift[i]  = max(shift[i], 0);  right_shift[i] = max(-shift[i],
         * 0) */
        vint32m4_t vleft_s = __riscv_vmax_vx_i32m4(vshift, 0, vl);
        vint32m4_t vright_s
            = __riscv_vmax_vx_i32m4(__riscv_vneg_v_i32m4(vshift, vl), 0, vl);
        vuint32m4_t vleft_u  = __riscv_vreinterpret_v_i32m4_u32m4(vleft_s);
        vuint32m4_t vright_u = __riscv_vreinterpret_v_i32m4_u32m4(vright_s);

        /* val << left_shift */
        vacc_0 = __riscv_vsll_vv_i32m4(vacc_0, vleft_u, vl);
        vacc_1 = __riscv_vsll_vv_i32m4(vacc_1, vleft_u, vl);
        vacc_2 = __riscv_vsll_vv_i32m4(vacc_2, vleft_u, vl);
        vacc_3 = __riscv_vsll_vv_i32m4(vacc_3, vleft_u, vl);
        vacc_4 = __riscv_vsll_vv_i32m4(vacc_4, vleft_u, vl);
        vacc_5 = __riscv_vsll_vv_i32m4(vacc_5, vleft_u, vl);
        vacc_6 = __riscv_vsll_vv_i32m4(vacc_6, vleft_u, vl);

        /* doubling_high_mult: round((acc * mult) >> 31) */
        vint32m4_t vresult_0
            = __riscv_vsmul_vv_i32m4(vacc_0, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_1
            = __riscv_vsmul_vv_i32m4(vacc_1, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_2
            = __riscv_vsmul_vv_i32m4(vacc_2, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_3
            = __riscv_vsmul_vv_i32m4(vacc_3, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_4
            = __riscv_vsmul_vv_i32m4(vacc_4, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_5
            = __riscv_vsmul_vv_i32m4(vacc_5, vmult, __RISCV_VXRM_RNU, vl);
        vint32m4_t vresult_6
            = __riscv_vsmul_vv_i32m4(vacc_6, vmult, __RISCV_VXRM_RNU, vl);

        /* divide_by_power_of_two: round(result >> right_shift) */
        vresult_0
            = __riscv_vssra_vv_i32m4(vresult_0, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_1
            = __riscv_vssra_vv_i32m4(vresult_1, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_2
            = __riscv_vssra_vv_i32m4(vresult_2, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_3
            = __riscv_vssra_vv_i32m4(vresult_3, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_4
            = __riscv_vssra_vv_i32m4(vresult_4, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_5
            = __riscv_vssra_vv_i32m4(vresult_5, vright_u, __RISCV_VXRM_RNU, vl);
        vresult_6
            = __riscv_vssra_vv_i32m4(vresult_6, vright_u, __RISCV_VXRM_RNU, vl);

        /* Add output offset */
        vresult_0 = __riscv_vadd_vx_i32m4(vresult_0, out_offset, vl);
        vresult_1 = __riscv_vadd_vx_i32m4(vresult_1, out_offset, vl);
        vresult_2 = __riscv_vadd_vx_i32m4(vresult_2, out_offset, vl);
        vresult_3 = __riscv_vadd_vx_i32m4(vresult_3, out_offset, vl);
        vresult_4 = __riscv_vadd_vx_i32m4(vresult_4, out_offset, vl);
        vresult_5 = __riscv_vadd_vx_i32m4(vresult_5, out_offset, vl);
        vresult_6 = __riscv_vadd_vx_i32m4(vresult_6, out_offset, vl);

        /* Saturating narrow i32->i16; clamp to activation bounds; narrow
         * i16->i8 */
        vint16m2_t vout16_0
            = __riscv_vnclip_wx_i16m2(vresult_0, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_1
            = __riscv_vnclip_wx_i16m2(vresult_1, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_2
            = __riscv_vnclip_wx_i16m2(vresult_2, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_3
            = __riscv_vnclip_wx_i16m2(vresult_3, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_4
            = __riscv_vnclip_wx_i16m2(vresult_4, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_5
            = __riscv_vnclip_wx_i16m2(vresult_5, 0, __RISCV_VXRM_RNU, vl);
        vint16m2_t vout16_6
            = __riscv_vnclip_wx_i16m2(vresult_6, 0, __RISCV_VXRM_RNU, vl);

        vout16_0 = __riscv_vmax_vx_i16m2(vout16_0, activation_min, vl);
        vout16_0 = __riscv_vmin_vx_i16m2(vout16_0, activation_max, vl);
        vout16_1 = __riscv_vmax_vx_i16m2(vout16_1, activation_min, vl);
        vout16_1 = __riscv_vmin_vx_i16m2(vout16_1, activation_max, vl);
        vout16_2 = __riscv_vmax_vx_i16m2(vout16_2, activation_min, vl);
        vout16_2 = __riscv_vmin_vx_i16m2(vout16_2, activation_max, vl);
        vout16_3 = __riscv_vmax_vx_i16m2(vout16_3, activation_min, vl);
        vout16_3 = __riscv_vmin_vx_i16m2(vout16_3, activation_max, vl);
        vout16_4 = __riscv_vmax_vx_i16m2(vout16_4, activation_min, vl);
        vout16_4 = __riscv_vmin_vx_i16m2(vout16_4, activation_max, vl);
        vout16_5 = __riscv_vmax_vx_i16m2(vout16_5, activation_min, vl);
        vout16_5 = __riscv_vmin_vx_i16m2(vout16_5, activation_max, vl);
        vout16_6 = __riscv_vmax_vx_i16m2(vout16_6, activation_min, vl);
        vout16_6 = __riscv_vmin_vx_i16m2(vout16_6, activation_max, vl);

        vint8m1_t vout8_0
            = __riscv_vnclip_wx_i8m1(vout16_0, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_1
            = __riscv_vnclip_wx_i8m1(vout16_1, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_2
            = __riscv_vnclip_wx_i8m1(vout16_2, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_3
            = __riscv_vnclip_wx_i8m1(vout16_3, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_4
            = __riscv_vnclip_wx_i8m1(vout16_4, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_5
            = __riscv_vnclip_wx_i8m1(vout16_5, 0, __RISCV_VXRM_RNU, vl);
        vint8m1_t vout8_6
            = __riscv_vnclip_wx_i8m1(vout16_6, 0, __RISCV_VXRM_RNU, vl);

        __riscv_vse8_v_i8m1((int8_t *)out_0, vout8_0, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_1, vout8_1, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_2, vout8_2, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_3, vout8_3, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_4, vout8_4, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_5, vout8_5, vl);
        __riscv_vse8_v_i8m1((int8_t *)out_6, vout8_6, vl);

        out_0 += vl;
        out_1 += vl;
        out_2 += vl;
        out_3 += vl;
        out_4 += vl;
        out_5 += vl;
        out_6 += vl;
        mult_ptr += vl;
        shift_ptr += vl;
        ip_a_strip += vl;
        remaining -= vl;
    }

    return out_6; /* points one past the last written byte of the 7th output
                     row */
}
