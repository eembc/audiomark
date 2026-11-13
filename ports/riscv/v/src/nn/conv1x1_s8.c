/*
 * Copyright (C) 2010-2021 Arm Limited or its affiliates.
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
#include "ee_api.h"
#include "functions.h"
#include "rvv_support_functions.h"
#include "convolve_config.h"

#define INPUT_W     5
#define INPUT_H     25
#define INPUT_N     1
#define INPUT_CH    64
#define OUTPUT_CH   64
#define OUT_OFFSET  (-128)
#define OUT_ACT_MIN (-128)
#define OUT_ACT_MAX 127
#define COL_LEN     (INPUT_W * INPUT_H * INPUT_N)

static void
nn_fold_input_offset_s8(int32_t *__EE_RESTRICT       corrected_bias,
                        const int32_t *__EE_RESTRICT bias_data,
                        const q7_t *__EE_RESTRICT    weights_rvv,
                        int32_t                      input_offset)
{
    int32_t oc_off = 0;
    while (oc_off < OUTPUT_CH)
    {
        size_t     vl   = __riscv_vsetvl_e32m4(OUTPUT_CH - oc_off);
        vint32m4_t vacc = bias_data
                              ? __riscv_vle32_v_i32m4(bias_data + oc_off, vl)
                              : __riscv_vmv_v_x_i32m4(0, vl);
        vint32m4_t vsum = __riscv_vmv_v_x_i32m4(0, vl);
        for (int32_t k = 0; k < INPUT_CH; k++)
        {
            vint8m1_t w8 = __riscv_vle8_v_i8m1(
                weights_rvv + (size_t)k * OUTPUT_CH + oc_off, vl);
            vint32m4_t w32 = __riscv_vsext_vf4_i32m4(w8, vl);
            vsum           = __riscv_vadd_vv_i32m4(vsum, w32, vl);
        }
        vsum = __riscv_vmul_vx_i32m4(vsum, input_offset, vl);
        vacc = __riscv_vadd_vv_i32m4(vacc, vsum, vl);
        __riscv_vse32_v_i32m4(corrected_bias + oc_off, vacc, vl);
        oc_off += (int32_t)vl;
    }
}

int32_t
nn_conv1x1_s8(const nn_context *__EE_RESTRICT                  ctx,
              const nn_conv_params *__EE_RESTRICT              conv_params,
              const nn_per_channel_quant_params *__EE_RESTRICT quant_params,
              const q7_t *__EE_RESTRICT                        input_data,
              const q7_t *__EE_RESTRICT                        filter_data,
              const int32_t *__EE_RESTRICT                     bias_data,
              q7_t *__EE_RESTRICT                              output_data)
{
    const int32_t *__EE_RESTRICT out_mult       = quant_params->multiplier;
    const int32_t *__EE_RESTRICT out_shift      = quant_params->shift;
    int32_t *__EE_RESTRICT       corrected_bias = (int32_t *)ctx->buf;

    nn_fold_input_offset_s8(
        corrected_bias, bias_data, filter_data, conv_params->input_offset);

    int32_t i_items = 0;
    for (; i_items <= COL_LEN - NN_KERNEL_COLS; i_items += NN_KERNEL_COLS)
    {
        output_data
            = nn_mat_mult_kernel_s8_s8(filter_data,
                                       input_data + (size_t)i_items * INPUT_CH,
                                       out_shift,
                                       out_mult,
                                       corrected_bias,
                                       output_data);
    }

    for (; i_items < COL_LEN; i_items++)
    {
        output_data
            = nn_mat_mult_core_1x1_s8(filter_data,
                                      input_data + (size_t)i_items * INPUT_CH,
                                      out_shift,
                                      out_mult,
                                      corrected_bias,
                                      output_data);
    }

    return 0;
}
