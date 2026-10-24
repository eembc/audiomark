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

/**
 * A collection of functions to perform basic operations for neural network
 * layers.
 */

#ifndef NN_FUNCTIONS_H
#define NN_FUNCTIONS_H

#include "math_types.h"
#include "types.h"
#include "ee_api.h"

int32_t nn_convolve_s8(const nn_context *__EE_RESTRICT     ctx,
                       const nn_conv_params *__EE_RESTRICT conv_params,
                       const nn_per_channel_quant_params *__EE_RESTRICT
                                                    quant_params,
                       const nn_dims *__EE_RESTRICT input_dims,
                       const q7_t *__EE_RESTRICT    input_data,
                       const nn_dims *__EE_RESTRICT filter_dims,
                       const q7_t *__EE_RESTRICT    filter_data,
                       const nn_dims *__EE_RESTRICT bias_dims,
                       const int32_t *__EE_RESTRICT bias_data,
                       const nn_dims *__EE_RESTRICT output_dims,
                       q7_t *__EE_RESTRICT          output_data);

int32_t nn_conv0_s8(const nn_context *__EE_RESTRICT ctx,
                    const nn_per_channel_quant_params *__EE_RESTRICT
                                                 quant_params,
                    const q7_t *__EE_RESTRICT    input_data,
                    const q7_t *__EE_RESTRICT    filter_data,
                    const int32_t *__EE_RESTRICT bias_data,
                    q7_t *__EE_RESTRICT          output_data);

int32_t nn_conv1x1_s8(const nn_context *__EE_RESTRICT     ctx,
                      const nn_conv_params *__EE_RESTRICT conv_params,
                      const nn_per_channel_quant_params *__EE_RESTRICT
                                                   quant_params,
                      const q7_t *__EE_RESTRICT    input_data,
                      const q7_t *__EE_RESTRICT    filter_data,
                      const int32_t *__EE_RESTRICT bias_data,
                      q7_t *__EE_RESTRICT          output_data);

int32_t nn_convolve_s8_get_buffer_size(const nn_dims *input_dims,
                                       const nn_dims *filter_dims);

int32_t nn_depthwise_conv_3x3_s8(
    const nn_per_channel_quant_params *__EE_RESTRICT quant_params,
    const q7_t                        *__EE_RESTRICT input,
    const q7_t                        *__EE_RESTRICT kernel,
    const int32_t                     *__EE_RESTRICT bias,
    q7_t                              *__EE_RESTRICT output);

int32_t nn_fully_connected_s8(
    const nn_fc_params               *__EE_RESTRICT fc_params,
    const nn_per_tensor_quant_params *__EE_RESTRICT quant_params,
    const nn_dims                    *__EE_RESTRICT input_dims,
    const int8_t                     *__EE_RESTRICT input_data,
    const nn_dims                    *__EE_RESTRICT filter_dims,
    const int8_t                     *__EE_RESTRICT filter_data,
    const nn_dims                    *__EE_RESTRICT bias_dims,
    const int32_t                    *__EE_RESTRICT bias_data,
    const nn_dims                    *__EE_RESTRICT output_dims,
    int8_t                           *__EE_RESTRICT output_data);

int32_t nn_avgpool_25x5x64_s8(const q7_t *__EE_RESTRICT input_data,
                              q7_t *__EE_RESTRICT       output_data);

void nn_softmax_row12_s8(const int8_t *__EE_RESTRICT input,
                         int8_t *__EE_RESTRICT       output);

#endif
