/**
 * Copyright 2026 Harshit Kumar Shivhare
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

#include "ee_api.h"
#include "ee_audiomark.h"
#include "rvv_support_guard.h"

void
th_f32_to_int16(const ee_f32_t *p_src, int16_t *p_dst, uint32_t len)
{
    while (len > 0)
    {
        size_t vl = __riscv_vsetvl_e32m4(len);

        vfloat32m4_t v_src = __riscv_vle32_v_f32m4(p_src, vl);
        v_src              = __riscv_vfmul_vf_f32m4(v_src, 32768.0f, vl);

        vint16m2_t v_result = __riscv_vfncvt_rtz_x_f_w_i16m2(v_src, vl);
        __riscv_vse16_v_i16m2(p_dst, v_result, vl);

        len -= vl;
        p_src += vl;
        p_dst += vl;
    }
}
