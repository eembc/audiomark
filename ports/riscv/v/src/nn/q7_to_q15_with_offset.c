/**
 * Copyright 2026 Robin John
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

#include "rvv_support_functions.h"
#include "rvv_support_guard.h"
#include <stddef.h>

void
nn_q7_to_q15_with_offset(const int8_t *src,
                         int16_t      *dst,
                         int32_t       block_size,
                         int16_t       offset)
{
    size_t remaining;

    remaining          = block_size;
    size_t     vlmax   = __riscv_vsetvlmax_e16m4();
    vint16m4_t vOffset = __riscv_vmv_v_x_i16m4(offset, vlmax);

    while (remaining > 0)
    {
        size_t     vl   = __riscv_vsetvl_e8m2(remaining);
        vint8m2_t  vSrc = __riscv_vle8_v_i8m2(src, vl);
        vint16m4_t vDst = __riscv_vwadd_wv(vOffset, vSrc, vl);
        __riscv_vse16_v_i16m4(dst, vDst, vl);

        /* Decrement the loop counter */
        remaining -= vl;
        src += vl;
        dst += vl;
    }
}
