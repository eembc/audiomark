/*
 * SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 */
#include "ds_cnn_model.hpp"
#include "log_macros.h"

const tflite::MicroOpResolver &DSCNNModel::GetOpResolver() {
    return this->m_opResolver;
}

bool DSCNNModel::EnlistOperations() {

    this->m_opResolver.AddReshape();
    this->m_opResolver.AddAveragePool2D();
    this->m_opResolver.AddConv2D();
    this->m_opResolver.AddDepthwiseConv2D();
    this->m_opResolver.AddFullyConnected();
    this->m_opResolver.AddSoftmax();
    if (kTfLiteOk == this->m_opResolver.AddEthosU()) {
        printf("Added %s support to op resolver\n",
               tflite::GetString_ETHOSU());
    } else {
        printf_err("Failed to add Arm NPU support to op resolver.");
        return false;
    }
    return true;
}