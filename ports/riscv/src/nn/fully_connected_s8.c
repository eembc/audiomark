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
 * Modifications copyright (C) 2021-2024 Chair of Electronic Design Automation,
 * TUM
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "functions.h"
#include "support_functions.h"
#include "ee_api.h"

#include <stdint.h>

/*
 * S8 basic fully-connected and matrix multiplication layer function
 */

int32_t
nn_fully_connected_s8(const nn_fc_params               *__EE_RESTRICT fc_params,
                      const nn_per_tensor_quant_params *__EE_RESTRICT quant_params,
                      const nn_dims                    *__EE_RESTRICT input_dims,
                      const q7_t                       *__EE_RESTRICT input,
                      const nn_dims                    *__EE_RESTRICT filter_dims,
                      const q7_t                       *__EE_RESTRICT kernel,
                      const nn_dims                    *__EE_RESTRICT bias_dims,
                      const int32_t                    *__EE_RESTRICT bias,
                      const nn_dims                    *__EE_RESTRICT output_dims,
                      q7_t                             *__EE_RESTRICT output)
{
    int32_t batch_cnt = input_dims->n;

    while (batch_cnt)
    {
        nn_vec_mat_mult_t_s8(input,
                             kernel,
                             bias,
                             output,
                             fc_params->input_offset,
                             fc_params->output_offset,
                             quant_params->multiplier,
                             quant_params->shift,
                             filter_dims->n, /* col_dim or accum_depth */
                             output_dims->c, /* row_dim or output_depth */
                             fc_params->activation.min,
                             fc_params->activation.max,
                             1L);
        input += filter_dims->n;
        output += output_dims->c;
        batch_cnt--;
    }
    return 0;
}
