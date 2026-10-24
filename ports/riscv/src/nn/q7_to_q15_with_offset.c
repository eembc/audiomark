// Modifications copyright (C) 2023 Chair of Electronic Design Automation, TUM
/*
 * SPDX-FileCopyrightText: Copyright 2010-2023 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in_q7x4 compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in_q7x4 writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications copyright (C) 2026 Sohail Raj Satapathy
 */

#include "support_functions.h"
#include "ee_api.h"

void
nn_q7_to_q15_with_offset(const int8_t *__EE_RESTRICT src,
                         int16_t      *__EE_RESTRICT dst,
                         int32_t                      block_size,
                         int16_t                      offset)
{
    int32_t block_cnt;

    block_cnt = block_size;

    while (block_cnt > 0)
    {
        *dst++ = (int16_t)*src++ + offset;

        /* Decrement the loop counter */
        block_cnt--;
    }
}
