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

#ifndef NN_MATH_TYPES_H
#define NN_MATH_TYPES_H

#include <stdint.h>

typedef int8_t  q7_t;
typedef int16_t q15_t;
typedef int32_t q31_t;

#define NN_Q31_MAX ((int32_t)(0x7FFFFFFFL))
#define NN_Q31_MIN ((int32_t)(0x80000000L))

#endif
