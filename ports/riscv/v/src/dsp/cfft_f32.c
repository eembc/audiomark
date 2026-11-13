/*
 * Copyright 2026 Robin John
 * Copyright 2026 Harshit Kumar Shivhare
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
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
 */

#include <riscv_dsp_types.h>
#include "riscv_dsp.h"
#include "dsp_f32.h"
#include "rvv_support_guard.h"
#include "ee_api.h"

riscv_status
riscv_cfft_init_f32(riscv_cfft_instance_f32 *__EE_RESTRICT p_instance, uint16_t fftLength)
{
    riscv_status status = RISCV_MATH_SUCCESS;
    p_instance->fftLen  = fftLength;
    switch (fftLength)
    {
        case 128U:
            p_instance->pTwiddle     = twiddleCoef_f32_128;
            p_instance->pBitRevTable = riscvBitRevIndexTable_r4_128;
            p_instance->bitRevLength
                = RISCVBITREVINDEXTABLE_RADIX4_128_TABLE_LENGTH;
            p_instance->rearranged_twiddle_stride1
                = rearranged_twiddle_stride1_64_f32;
            p_instance->rearranged_twiddle_stride2
                = rearranged_twiddle_stride2_64_f32;
            p_instance->rearranged_twiddle_stride3
                = rearranged_twiddle_stride3_64_f32;
            p_instance->rearranged_twiddle_tab_stride1_arr
                = rearranged_twiddle_tab_stride1_arr_64_f32;
            p_instance->rearranged_twiddle_tab_stride2_arr
                = rearranged_twiddle_tab_stride2_arr_64_f32;
            p_instance->rearranged_twiddle_tab_stride3_arr
                = rearranged_twiddle_tab_stride3_arr_64_f32;
            break;
        case 256U:
            p_instance->pTwiddle     = twiddleCoef_f32_256;
            p_instance->pBitRevTable = riscvBitRevIndexTable_r4_256;
            p_instance->bitRevLength
                = RISCVBITREVINDEXTABLE_RADIX4_256_TABLE_LENGTH;
            p_instance->rearranged_twiddle_stride1
                = rearranged_twiddle_stride1_256_f32;
            p_instance->rearranged_twiddle_stride2
                = rearranged_twiddle_stride2_256_f32;
            p_instance->rearranged_twiddle_stride3
                = rearranged_twiddle_stride3_256_f32;
            p_instance->rearranged_twiddle_tab_stride1_arr
                = rearranged_twiddle_tab_stride1_arr_256_f32;
            p_instance->rearranged_twiddle_tab_stride2_arr
                = rearranged_twiddle_tab_stride2_arr_256_f32;
            p_instance->rearranged_twiddle_tab_stride3_arr
                = rearranged_twiddle_tab_stride3_arr_256_f32;
            break;
        case 512U:
            p_instance->pTwiddle     = twiddleCoef_f32_512;
            p_instance->pBitRevTable = riscvBitRevIndexTable_r4_512;
            p_instance->bitRevLength
                = RISCVBITREVINDEXTABLE_RADIX4_512_TABLE_LENGTH;
            p_instance->rearranged_twiddle_stride1
                = rearranged_twiddle_stride1_256_f32;
            p_instance->rearranged_twiddle_stride2
                = rearranged_twiddle_stride2_256_f32;
            p_instance->rearranged_twiddle_stride3
                = rearranged_twiddle_stride3_256_f32;
            p_instance->rearranged_twiddle_tab_stride1_arr
                = rearranged_twiddle_tab_stride1_arr_256_f32;
            p_instance->rearranged_twiddle_tab_stride2_arr
                = rearranged_twiddle_tab_stride2_arr_256_f32;
            p_instance->rearranged_twiddle_tab_stride3_arr
                = rearranged_twiddle_tab_stride3_arr_256_f32;
            break;
        default:
            status = RISCV_MATH_ERROR;
            break;
    }

    return status;
}

void
riscv_bitreversal_32_inpl(uint32_t       *__EE_RESTRICT pSrc,
                          const uint16_t                bitRevLen,
                          const uint16_t *__EE_RESTRICT pBitRevTab)
{
    int32_t   remaining = bitRevLen >> 1;

    /* load first iteration's offsets before entering loop */
    size_t        vl  = __riscv_vsetvl_e32m4(remaining);
    vuint16m2x2_t seg = __riscv_vlseg2e16_v_u16m2x2(pBitRevTab, vl);
    vuint16m2_t   oa  = __riscv_vget_v_u16m2x2_u16m2(seg, 0);
    vuint16m2_t   ob  = __riscv_vget_v_u16m2x2_u16m2(seg, 1);
    pBitRevTab += vl * 2;
    remaining -= vl;

    while (remaining > 0)
    {
        vuint32m4x2_t va = __riscv_vluxseg2ei16_v_u32m4x2(pSrc, oa, vl);
        vuint32m4x2_t vb = __riscv_vluxseg2ei16_v_u32m4x2(pSrc, ob, vl);
        __riscv_vsuxseg2ei16_v_u32m4x2(pSrc, oa, vb, vl);
        __riscv_vsuxseg2ei16_v_u32m4x2(pSrc, ob, va, vl);

        vl  = __riscv_vsetvl_e32m4(remaining);
        seg = __riscv_vlseg2e16_v_u16m2x2(pBitRevTab, vl);
        oa  = __riscv_vget_v_u16m2x2_u16m2(seg, 0);
        ob  = __riscv_vget_v_u16m2x2_u16m2(seg, 1);
        pBitRevTab += vl * 2;
        remaining -= vl;
    }

    /* process last primed offsets */
    vuint32m4x2_t va = __riscv_vluxseg2ei16_v_u32m4x2(pSrc, oa, vl);
    vuint32m4x2_t vb = __riscv_vluxseg2ei16_v_u32m4x2(pSrc, ob, vl);
    __riscv_vsuxseg2ei16_v_u32m4x2(pSrc, oa, vb, vl);
    __riscv_vsuxseg2ei16_v_u32m4x2(pSrc, ob, va, vl);
}

static float32_t
riscv_inverse_fft_length_f32(uint16_t fftLen)
{
    float32_t retValue = 1.0;

    switch (fftLen)
    {

        case 512U:
            retValue = 0.001953125;
            break;

        case 256U:
            retValue = 0.00390625f;
            break;

        case 128U:
            retValue = 0.0078125;
            break;

        default:
            break;
    }
    return (retValue);
}

void
riscv_radix4_butterfly_inverse_f32(const riscv_cfft_instance_f32 *__EE_RESTRICT S,
                                   float32_t                     *__EE_RESTRICT pSrc,
                                   uint32_t                                      fftLen)
{
    float32_t onebyfftLen = riscv_inverse_fft_length_f32(S->fftLen);
    uint32_t  n2          = fftLen;
    uint32_t  n1          = n2;
    n2 >>= 2u;
    uint32_t stage = 0;
    int32_t  iter  = 1;

    while (n2 > 1)
    {
        float32_t const *pW1
            = &S->rearranged_twiddle_stride1
                   [S->rearranged_twiddle_tab_stride1_arr[stage]];
        float32_t const *pW2
            = &S->rearranged_twiddle_stride2
                   [S->rearranged_twiddle_tab_stride2_arr[stage]];
        float32_t const *pW3
            = &S->rearranged_twiddle_stride3
                   [S->rearranged_twiddle_tab_stride3_arr[stage]];

        float32_t *pBase = pSrc;
        for (int i = 0; i < iter; i++)
        {
            float32_t *inA = pBase;
            float32_t *inB = inA + n2 * 2;
            float32_t *inC = inB + n2 * 2;
            float32_t *inD = inC + n2 * 2;

            float32_t const *tw1 = pW1;
            float32_t const *tw2 = pW2;
            float32_t const *tw3 = pW3;

            size_t blkCnt = n2;

            while (blkCnt > 0)
            {
                size_t vl = __riscv_vsetvl_e32m2(blkCnt);

                vfloat32m2x2_t vA = __riscv_vlseg2e32_v_f32m2x2(inA, vl);
                vfloat32m2_t   Ar = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
                vfloat32m2_t   Ai = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

                vfloat32m2x2_t vC = __riscv_vlseg2e32_v_f32m2x2(inC, vl);
                vfloat32m2_t   Cr = __riscv_vget_v_f32m2x2_f32m2(vC, 0);
                vfloat32m2_t   Ci = __riscv_vget_v_f32m2x2_f32m2(vC, 1);

                vfloat32m2_t Sum0r  = __riscv_vfadd_vv_f32m2(Ar, Cr, vl);
                vfloat32m2_t Sum0i  = __riscv_vfadd_vv_f32m2(Ai, Ci, vl);
                vfloat32m2_t Diff0r = __riscv_vfsub_vv_f32m2(Ar, Cr, vl);
                vfloat32m2_t Diff0i = __riscv_vfsub_vv_f32m2(Ai, Ci, vl);

                vfloat32m2x2_t vB = __riscv_vlseg2e32_v_f32m2x2(inB, vl);
                vfloat32m2_t   Br = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
                vfloat32m2_t   Bi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

                vfloat32m2x2_t vD = __riscv_vlseg2e32_v_f32m2x2(inD, vl);
                vfloat32m2_t   Dr = __riscv_vget_v_f32m2x2_f32m2(vD, 0);
                vfloat32m2_t   Di = __riscv_vget_v_f32m2x2_f32m2(vD, 1);

                vfloat32m2_t Sum1r  = __riscv_vfadd_vv_f32m2(Br, Dr, vl);
                vfloat32m2_t Sum1i  = __riscv_vfadd_vv_f32m2(Bi, Di, vl);
                vfloat32m2_t Diff1r = __riscv_vfsub_vv_f32m2(Br, Dr, vl);
                vfloat32m2_t Diff1i = __riscv_vfsub_vv_f32m2(Bi, Di, vl);

                /* Out0 = Sum0 + Sum1 */
                vfloat32m2_t out0r = __riscv_vfadd_vv_f32m2(Sum0r, Sum1r, vl);
                vfloat32m2_t out0i = __riscv_vfadd_vv_f32m2(Sum0i, Sum1i, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inA, __riscv_vcreate_v_f32m2x2(out0r, out0i), vl);

                vfloat32m2x2_t vW2 = __riscv_vlseg2e32_v_f32m2x2(tw2, vl);
                vfloat32m2_t   W2r = __riscv_vget_v_f32m2x2_f32m2(vW2, 0);
                vfloat32m2_t   W2i = __riscv_vget_v_f32m2x2_f32m2(vW2, 1);

                /* T1 = Sum0 - Sum1 */
                vfloat32m2_t T1r = __riscv_vfsub_vv_f32m2(Sum0r, Sum1r, vl);
                vfloat32m2_t T1i = __riscv_vfsub_vv_f32m2(Sum0i, Sum1i, vl);

                /* Out1 = T1 * W2 */
                vfloat32m2_t out1r = __riscv_vfmul_vv_f32m2(T1r, W2r, vl);
                out1r = __riscv_vfnmsac_vv_f32m2(out1r, T1i, W2i, vl);
                vfloat32m2_t out1i = __riscv_vfmul_vv_f32m2(T1r, W2i, vl);
                out1i = __riscv_vfmacc_vv_f32m2(out1i, T1i, W2r, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inB, __riscv_vcreate_v_f32m2x2(out1r, out1i), vl);

                vfloat32m2x2_t vW1 = __riscv_vlseg2e32_v_f32m2x2(tw1, vl);
                vfloat32m2_t   W1r = __riscv_vget_v_f32m2x2_f32m2(vW1, 0);
                vfloat32m2_t   W1i = __riscv_vget_v_f32m2x2_f32m2(vW1, 1);

                /* T2 = Diff0 + i*Diff1 */
                vfloat32m2_t T2r = __riscv_vfsub_vv_f32m2(Diff0r, Diff1i, vl);
                vfloat32m2_t T2i = __riscv_vfadd_vv_f32m2(Diff0i, Diff1r, vl);

                /* Out2 = T2 * W1 */
                vfloat32m2_t out2r = __riscv_vfmul_vv_f32m2(T2r, W1r, vl);
                out2r = __riscv_vfnmsac_vv_f32m2(out2r, T2i, W1i, vl);
                vfloat32m2_t out2i = __riscv_vfmul_vv_f32m2(T2r, W1i, vl);
                out2i = __riscv_vfmacc_vv_f32m2(out2i, T2i, W1r, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inC, __riscv_vcreate_v_f32m2x2(out2r, out2i), vl);

                vfloat32m2x2_t vW3 = __riscv_vlseg2e32_v_f32m2x2(tw3, vl);
                vfloat32m2_t   W3r = __riscv_vget_v_f32m2x2_f32m2(vW3, 0);
                vfloat32m2_t   W3i = __riscv_vget_v_f32m2x2_f32m2(vW3, 1);

                /* T3 = Diff0 - i*Diff1 */
                vfloat32m2_t T3r = __riscv_vfadd_vv_f32m2(Diff0r, Diff1i, vl);
                vfloat32m2_t T3i = __riscv_vfsub_vv_f32m2(Diff0i, Diff1r, vl);

                /* Out3 = T3 * W3 */
                vfloat32m2_t out3r = __riscv_vfmul_vv_f32m2(T3r, W3r, vl);
                out3r = __riscv_vfnmsac_vv_f32m2(out3r, T3i, W3i, vl);
                vfloat32m2_t out3i = __riscv_vfmul_vv_f32m2(T3r, W3i, vl);
                out3i = __riscv_vfmacc_vv_f32m2(out3i, T3i, W3r, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inD, __riscv_vcreate_v_f32m2x2(out3r, out3i), vl);

                inA += 2 * vl;
                inB += 2 * vl;
                inC += 2 * vl;
                inD += 2 * vl;
                tw1 += 2 * vl;
                tw2 += 2 * vl;
                tw3 += 2 * vl;

                blkCnt -= vl;
            }
            pBase += 2 * n1;
        }
        n1 = n2;
        n2 >>= 2u;
        iter <<= 2;
        stage++;
    }

    float32_t *pBase  = pSrc;
    size_t     blkCnt = fftLen / 4;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vA = __riscv_vlsseg2e32_v_f32m2x2(pBase, 32, vl);
        vfloat32m2_t   Ar = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
        vfloat32m2_t   Ai = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

        vfloat32m2x2_t vC = __riscv_vlsseg2e32_v_f32m2x2(pBase + 4, 32, vl);
        vfloat32m2_t   Cr = __riscv_vget_v_f32m2x2_f32m2(vC, 0);
        vfloat32m2_t   Ci = __riscv_vget_v_f32m2x2_f32m2(vC, 1);

        vfloat32m2_t Sum0r  = __riscv_vfadd_vv_f32m2(Ar, Cr, vl);
        vfloat32m2_t Sum0i  = __riscv_vfadd_vv_f32m2(Ai, Ci, vl);
        vfloat32m2_t Diff0r = __riscv_vfsub_vv_f32m2(Ar, Cr, vl);
        vfloat32m2_t Diff0i = __riscv_vfsub_vv_f32m2(Ai, Ci, vl);

        vfloat32m2x2_t vB = __riscv_vlsseg2e32_v_f32m2x2(pBase + 2, 32, vl);
        vfloat32m2_t   Br = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
        vfloat32m2_t   Bi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

        vfloat32m2x2_t vD = __riscv_vlsseg2e32_v_f32m2x2(pBase + 6, 32, vl);
        vfloat32m2_t   Dr = __riscv_vget_v_f32m2x2_f32m2(vD, 0);
        vfloat32m2_t   Di = __riscv_vget_v_f32m2x2_f32m2(vD, 1);

        vfloat32m2_t Sum1r  = __riscv_vfadd_vv_f32m2(Br, Dr, vl);
        vfloat32m2_t Sum1i  = __riscv_vfadd_vv_f32m2(Bi, Di, vl);
        vfloat32m2_t Diff1r = __riscv_vfsub_vv_f32m2(Br, Dr, vl);
        vfloat32m2_t Diff1i = __riscv_vfsub_vv_f32m2(Bi, Di, vl);

        vfloat32m2_t out0r = __riscv_vfmul_vf_f32m2(
            __riscv_vfadd_vv_f32m2(Sum0r, Sum1r, vl), onebyfftLen, vl);
        vfloat32m2_t out0i = __riscv_vfmul_vf_f32m2(
            __riscv_vfadd_vv_f32m2(Sum0i, Sum1i, vl), onebyfftLen, vl);

        vfloat32m2_t out1r = __riscv_vfmul_vf_f32m2(
            __riscv_vfsub_vv_f32m2(Sum0r, Sum1r, vl), onebyfftLen, vl);
        vfloat32m2_t out1i = __riscv_vfmul_vf_f32m2(
            __riscv_vfsub_vv_f32m2(Sum0i, Sum1i, vl), onebyfftLen, vl);

        __riscv_vssseg2e32_v_f32m2x2(
            pBase, 32, __riscv_vcreate_v_f32m2x2(out0r, out0i), vl);
        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 2, 32, __riscv_vcreate_v_f32m2x2(out1r, out1i), vl);

        vfloat32m2_t out2r = __riscv_vfmul_vf_f32m2(
            __riscv_vfsub_vv_f32m2(Diff0r, Diff1i, vl), onebyfftLen, vl);
        vfloat32m2_t out2i = __riscv_vfmul_vf_f32m2(
            __riscv_vfadd_vv_f32m2(Diff0i, Diff1r, vl), onebyfftLen, vl);

        vfloat32m2_t out3r = __riscv_vfmul_vf_f32m2(
            __riscv_vfadd_vv_f32m2(Diff0r, Diff1i, vl), onebyfftLen, vl);
        vfloat32m2_t out3i = __riscv_vfmul_vf_f32m2(
            __riscv_vfsub_vv_f32m2(Diff0i, Diff1r, vl), onebyfftLen, vl);

        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 4, 32, __riscv_vcreate_v_f32m2x2(out2r, out2i), vl);
        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 6, 32, __riscv_vcreate_v_f32m2x2(out3r, out3i), vl);

        pBase += 8 * vl;
        blkCnt -= vl;
    }
}

void
riscv_radix4_butterfly_f32(const riscv_cfft_instance_f32 *__EE_RESTRICT S,
                           float32_t                     *__EE_RESTRICT pSrc,
                           uint32_t                                    fftLen)
{
    uint32_t n2 = fftLen;
    uint32_t n1 = n2;
    n2 >>= 2u;
    uint32_t stage = 0;
    int32_t  iter  = 1;

    while (n2 > 1)
    {
        float32_t const *pW1
            = &S->rearranged_twiddle_stride1
                   [S->rearranged_twiddle_tab_stride1_arr[stage]];
        float32_t const *pW2
            = &S->rearranged_twiddle_stride2
                   [S->rearranged_twiddle_tab_stride2_arr[stage]];
        float32_t const *pW3
            = &S->rearranged_twiddle_stride3
                   [S->rearranged_twiddle_tab_stride3_arr[stage]];

        float32_t *pBase = pSrc;
        for (int i = 0; i < iter; i++)
        {
            float32_t *inA = pBase;
            float32_t *inB = inA + n2 * 2;
            float32_t *inC = inB + n2 * 2;
            float32_t *inD = inC + n2 * 2;

            float32_t const *tw1 = pW1;
            float32_t const *tw2 = pW2;
            float32_t const *tw3 = pW3;

            size_t blkCnt = n2;

            while (blkCnt > 0)
            {
                size_t vl = __riscv_vsetvl_e32m2(blkCnt);

                vfloat32m2x2_t vA = __riscv_vlseg2e32_v_f32m2x2(inA, vl);
                vfloat32m2_t   Ar = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
                vfloat32m2_t   Ai = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

                vfloat32m2x2_t vC = __riscv_vlseg2e32_v_f32m2x2(inC, vl);
                vfloat32m2_t   Cr = __riscv_vget_v_f32m2x2_f32m2(vC, 0);
                vfloat32m2_t   Ci = __riscv_vget_v_f32m2x2_f32m2(vC, 1);

                vfloat32m2_t Sum0r  = __riscv_vfadd_vv_f32m2(Ar, Cr, vl);
                vfloat32m2_t Sum0i  = __riscv_vfadd_vv_f32m2(Ai, Ci, vl);
                vfloat32m2_t Diff0r = __riscv_vfsub_vv_f32m2(Ar, Cr, vl);
                vfloat32m2_t Diff0i = __riscv_vfsub_vv_f32m2(Ai, Ci, vl);

                vfloat32m2x2_t vB = __riscv_vlseg2e32_v_f32m2x2(inB, vl);
                vfloat32m2_t   Br = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
                vfloat32m2_t   Bi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

                vfloat32m2x2_t vD = __riscv_vlseg2e32_v_f32m2x2(inD, vl);
                vfloat32m2_t   Dr = __riscv_vget_v_f32m2x2_f32m2(vD, 0);
                vfloat32m2_t   Di = __riscv_vget_v_f32m2x2_f32m2(vD, 1);

                vfloat32m2_t Sum1r  = __riscv_vfadd_vv_f32m2(Br, Dr, vl);
                vfloat32m2_t Sum1i  = __riscv_vfadd_vv_f32m2(Bi, Di, vl);
                vfloat32m2_t Diff1r = __riscv_vfsub_vv_f32m2(Br, Dr, vl);
                vfloat32m2_t Diff1i = __riscv_vfsub_vv_f32m2(Bi, Di, vl);

                /* Out0 = Sum0 + Sum1 */
                vfloat32m2_t out0r = __riscv_vfadd_vv_f32m2(Sum0r, Sum1r, vl);
                vfloat32m2_t out0i = __riscv_vfadd_vv_f32m2(Sum0i, Sum1i, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inA, __riscv_vcreate_v_f32m2x2(out0r, out0i), vl);

                vfloat32m2x2_t vW2 = __riscv_vlseg2e32_v_f32m2x2(tw2, vl);
                vfloat32m2_t   W2r = __riscv_vget_v_f32m2x2_f32m2(vW2, 0);
                vfloat32m2_t   W2i = __riscv_vget_v_f32m2x2_f32m2(vW2, 1);

                /* T1 = Sum0 - Sum1 */
                vfloat32m2_t T1r = __riscv_vfsub_vv_f32m2(Sum0r, Sum1r, vl);
                vfloat32m2_t T1i = __riscv_vfsub_vv_f32m2(Sum0i, Sum1i, vl);

                /* Out1 = T1 * conj(W2) */
                vfloat32m2_t out1r = __riscv_vfmul_vv_f32m2(T1r, W2r, vl);
                out1r = __riscv_vfmacc_vv_f32m2(out1r, T1i, W2i, vl);
                vfloat32m2_t out1i = __riscv_vfmul_vv_f32m2(T1i, W2r, vl);
                out1i = __riscv_vfnmsac_vv_f32m2(out1i, T1r, W2i, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inB, __riscv_vcreate_v_f32m2x2(out1r, out1i), vl);

                vfloat32m2x2_t vW1 = __riscv_vlseg2e32_v_f32m2x2(tw1, vl);
                vfloat32m2_t   W1r = __riscv_vget_v_f32m2x2_f32m2(vW1, 0);
                vfloat32m2_t   W1i = __riscv_vget_v_f32m2x2_f32m2(vW1, 1);

                vfloat32m2_t T2r = __riscv_vfadd_vv_f32m2(Diff0r, Diff1i, vl);
                vfloat32m2_t T2i = __riscv_vfsub_vv_f32m2(Diff0i, Diff1r, vl);

                /* Out2 = T2 * conj(W1) */
                vfloat32m2_t out2r = __riscv_vfmul_vv_f32m2(T2r, W1r, vl);
                out2r = __riscv_vfmacc_vv_f32m2(out2r, T2i, W1i, vl);
                vfloat32m2_t out2i = __riscv_vfmul_vv_f32m2(T2i, W1r, vl);
                out2i = __riscv_vfnmsac_vv_f32m2(out2i, T2r, W1i, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inC, __riscv_vcreate_v_f32m2x2(out2r, out2i), vl);

                vfloat32m2x2_t vW3 = __riscv_vlseg2e32_v_f32m2x2(tw3, vl);
                vfloat32m2_t   W3r = __riscv_vget_v_f32m2x2_f32m2(vW3, 0);
                vfloat32m2_t   W3i = __riscv_vget_v_f32m2x2_f32m2(vW3, 1);

                vfloat32m2_t T3r = __riscv_vfsub_vv_f32m2(Diff0r, Diff1i, vl);
                vfloat32m2_t T3i = __riscv_vfadd_vv_f32m2(Diff0i, Diff1r, vl);

                /* Out3 = T3 * conj(W3) */
                vfloat32m2_t out3r = __riscv_vfmul_vv_f32m2(T3r, W3r, vl);
                out3r = __riscv_vfmacc_vv_f32m2(out3r, T3i, W3i, vl);
                vfloat32m2_t out3i = __riscv_vfmul_vv_f32m2(T3i, W3r, vl);
                out3i = __riscv_vfnmsac_vv_f32m2(out3i, T3r, W3i, vl);
                __riscv_vsseg2e32_v_f32m2x2(
                    inD, __riscv_vcreate_v_f32m2x2(out3r, out3i), vl);

                inA += 2 * vl;
                inB += 2 * vl;
                inC += 2 * vl;
                inD += 2 * vl;
                tw1 += 2 * vl;
                tw2 += 2 * vl;
                tw3 += 2 * vl;

                blkCnt -= vl;
            }
            pBase += 2 * n1;
        }
        n1 = n2;
        n2 >>= 2u;
        iter <<= 2;
        stage++;
    }

    float32_t *pBase  = pSrc;
    size_t     blkCnt = fftLen / 4;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vA = __riscv_vlsseg2e32_v_f32m2x2(pBase, 32, vl);
        vfloat32m2_t   Ar = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
        vfloat32m2_t   Ai = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

        vfloat32m2x2_t vC = __riscv_vlsseg2e32_v_f32m2x2(pBase + 4, 32, vl);
        vfloat32m2_t   Cr = __riscv_vget_v_f32m2x2_f32m2(vC, 0);
        vfloat32m2_t   Ci = __riscv_vget_v_f32m2x2_f32m2(vC, 1);

        vfloat32m2_t Sum0r  = __riscv_vfadd_vv_f32m2(Ar, Cr, vl);
        vfloat32m2_t Sum0i  = __riscv_vfadd_vv_f32m2(Ai, Ci, vl);
        vfloat32m2_t Diff0r = __riscv_vfsub_vv_f32m2(Ar, Cr, vl);
        vfloat32m2_t Diff0i = __riscv_vfsub_vv_f32m2(Ai, Ci, vl);

        vfloat32m2x2_t vB = __riscv_vlsseg2e32_v_f32m2x2(pBase + 2, 32, vl);
        vfloat32m2_t   Br = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
        vfloat32m2_t   Bi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

        vfloat32m2x2_t vD = __riscv_vlsseg2e32_v_f32m2x2(pBase + 6, 32, vl);
        vfloat32m2_t   Dr = __riscv_vget_v_f32m2x2_f32m2(vD, 0);
        vfloat32m2_t   Di = __riscv_vget_v_f32m2x2_f32m2(vD, 1);

        vfloat32m2_t Sum1r  = __riscv_vfadd_vv_f32m2(Br, Dr, vl);
        vfloat32m2_t Sum1i  = __riscv_vfadd_vv_f32m2(Bi, Di, vl);
        vfloat32m2_t Diff1r = __riscv_vfsub_vv_f32m2(Br, Dr, vl);
        vfloat32m2_t Diff1i = __riscv_vfsub_vv_f32m2(Bi, Di, vl);

        vfloat32m2_t out0r = __riscv_vfadd_vv_f32m2(Sum0r, Sum1r, vl);
        vfloat32m2_t out0i = __riscv_vfadd_vv_f32m2(Sum0i, Sum1i, vl);

        vfloat32m2_t out1r = __riscv_vfsub_vv_f32m2(Sum0r, Sum1r, vl);
        vfloat32m2_t out1i = __riscv_vfsub_vv_f32m2(Sum0i, Sum1i, vl);

        __riscv_vssseg2e32_v_f32m2x2(
            pBase, 32, __riscv_vcreate_v_f32m2x2(out0r, out0i), vl);
        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 2, 32, __riscv_vcreate_v_f32m2x2(out1r, out1i), vl);

        vfloat32m2_t out2r = __riscv_vfadd_vv_f32m2(Diff0r, Diff1i, vl);
        vfloat32m2_t out2i = __riscv_vfsub_vv_f32m2(Diff0i, Diff1r, vl);

        vfloat32m2_t out3r = __riscv_vfsub_vv_f32m2(Diff0r, Diff1i, vl);
        vfloat32m2_t out3i = __riscv_vfadd_vv_f32m2(Diff0i, Diff1r, vl);

        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 4, 32, __riscv_vcreate_v_f32m2x2(out2r, out2i), vl);
        __riscv_vssseg2e32_v_f32m2x2(
            pBase + 6, 32, __riscv_vcreate_v_f32m2x2(out3r, out3i), vl);

        pBase += 8 * vl;
        blkCnt -= vl;
    }
}

void
riscv_cfft_radix4by2_f32(const riscv_cfft_instance_f32 *__EE_RESTRICT S,
                         float32_t                     *__EE_RESTRICT pSrc,
                         uint32_t                                    fftLen)
{
    float32_t const *pCoef = S->pTwiddle;
    uint32_t         n2    = fftLen >> 1;

    float32_t       *pIn0     = pSrc;
    float32_t       *pIn1     = pSrc + fftLen;
    float32_t const *pCoefVec = pCoef;

    size_t blkCnt = n2;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vIn0  = __riscv_vlseg2e32_v_f32m2x2(pIn0, vl);
        vfloat32m2_t   In0_r = __riscv_vget_v_f32m2x2_f32m2(vIn0, 0);
        vfloat32m2_t   In0_i = __riscv_vget_v_f32m2x2_f32m2(vIn0, 1);

        vfloat32m2x2_t vIn1  = __riscv_vlseg2e32_v_f32m2x2(pIn1, vl);
        vfloat32m2_t   In1_r = __riscv_vget_v_f32m2x2_f32m2(vIn1, 0);
        vfloat32m2_t   In1_i = __riscv_vget_v_f32m2x2_f32m2(vIn1, 1);

        vfloat32m2x2_t vTw  = __riscv_vlseg2e32_v_f32m2x2(pCoefVec, vl);
        vfloat32m2_t   Tw_r = __riscv_vget_v_f32m2x2_f32m2(vTw, 0);
        vfloat32m2_t   Tw_i = __riscv_vget_v_f32m2x2_f32m2(vTw, 1);

        vfloat32m2_t Sum_r = __riscv_vfadd_vv_f32m2(In0_r, In1_r, vl);
        vfloat32m2_t Sum_i = __riscv_vfadd_vv_f32m2(In0_i, In1_i, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pIn0, __riscv_vcreate_v_f32m2x2(Sum_r, Sum_i), vl);

        vfloat32m2_t Diff_r = __riscv_vfsub_vv_f32m2(In0_r, In1_r, vl);
        vfloat32m2_t Diff_i = __riscv_vfsub_vv_f32m2(In0_i, In1_i, vl);

        /* Real = Diff_r * Tw_r + Diff_i * Tw_i */
        vfloat32m2_t CmpTmp_r = __riscv_vfmul_vv_f32m2(Diff_r, Tw_r, vl);
        CmpTmp_r = __riscv_vfmacc_vv_f32m2(CmpTmp_r, Diff_i, Tw_i, vl);

        /* Imag = Diff_i * Tw_r - Diff_r * Tw_i */
        vfloat32m2_t CmpTmp_i = __riscv_vfmul_vv_f32m2(Diff_i, Tw_r, vl);
        CmpTmp_i = __riscv_vfnmsac_vv_f32m2(CmpTmp_i, Diff_r, Tw_i, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pIn1, __riscv_vcreate_v_f32m2x2(CmpTmp_r, CmpTmp_i), vl);

        pIn0 += 2 * vl;
        pIn1 += 2 * vl;
        pCoefVec += 2 * vl;
        blkCnt -= vl;
    }

    riscv_radix4_butterfly_f32(S, pSrc, n2);
    riscv_radix4_butterfly_f32(S, pSrc + fftLen, n2);
}

void
riscv_cfft_radix4by2_inverse_f32(const riscv_cfft_instance_f32 *__EE_RESTRICT S,
                                 float32_t                     *__EE_RESTRICT pSrc,
                                 uint32_t                                      fftLen)
{
    float32_t const *pCoef = S->pTwiddle;
    uint32_t         n2    = fftLen >> 1;

    float32_t       *pIn0     = pSrc;
    float32_t       *pIn1     = pSrc + fftLen;
    float32_t const *pCoefVec = pCoef;

    size_t blkCnt = n2;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vIn0  = __riscv_vlseg2e32_v_f32m2x2(pIn0, vl);
        vfloat32m2_t   In0_r = __riscv_vget_v_f32m2x2_f32m2(vIn0, 0);
        vfloat32m2_t   In0_i = __riscv_vget_v_f32m2x2_f32m2(vIn0, 1);

        vfloat32m2x2_t vIn1  = __riscv_vlseg2e32_v_f32m2x2(pIn1, vl);
        vfloat32m2_t   In1_r = __riscv_vget_v_f32m2x2_f32m2(vIn1, 0);
        vfloat32m2_t   In1_i = __riscv_vget_v_f32m2x2_f32m2(vIn1, 1);

        vfloat32m2x2_t vTw  = __riscv_vlseg2e32_v_f32m2x2(pCoefVec, vl);
        vfloat32m2_t   Tw_r = __riscv_vget_v_f32m2x2_f32m2(vTw, 0);
        vfloat32m2_t   Tw_i = __riscv_vget_v_f32m2x2_f32m2(vTw, 1);

        vfloat32m2_t Sum_r = __riscv_vfadd_vv_f32m2(In0_r, In1_r, vl);
        vfloat32m2_t Sum_i = __riscv_vfadd_vv_f32m2(In0_i, In1_i, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pIn0, __riscv_vcreate_v_f32m2x2(Sum_r, Sum_i), vl);

        vfloat32m2_t Diff_r = __riscv_vfsub_vv_f32m2(In0_r, In1_r, vl);
        vfloat32m2_t Diff_i = __riscv_vfsub_vv_f32m2(In0_i, In1_i, vl);

        /* Real = Diff_r * Tw_r - Diff_i * Tw_i */
        vfloat32m2_t CmpTmp_r = __riscv_vfmul_vv_f32m2(Diff_r, Tw_r, vl);
        CmpTmp_r = __riscv_vfnmsac_vv_f32m2(CmpTmp_r, Diff_i, Tw_i, vl);

        /* Imag = Diff_i * Tw_r + Diff_r * Tw_i */
        vfloat32m2_t CmpTmp_i = __riscv_vfmul_vv_f32m2(Diff_i, Tw_r, vl);
        CmpTmp_i = __riscv_vfmacc_vv_f32m2(CmpTmp_i, Diff_r, Tw_i, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pIn1, __riscv_vcreate_v_f32m2x2(CmpTmp_r, CmpTmp_i), vl);

        pIn0 += 2 * vl;
        pIn1 += 2 * vl;
        pCoefVec += 2 * vl;
        blkCnt -= vl;
    }

    riscv_radix4_butterfly_inverse_f32(S, pSrc, n2);
    riscv_radix4_butterfly_inverse_f32(S, pSrc + fftLen, n2);
}

void
riscv_cfft_f32(const riscv_cfft_instance_f32 *__EE_RESTRICT p_instance,
               float32_t                     *__EE_RESTRICT pSrc,
               uint8_t                                      ifftFlag,
               uint8_t                                      bitReverseFlagR)
{
    uint32_t fftLen = p_instance->fftLen;

    if (ifftFlag == 1U)
    {
        switch (fftLen)
        {
            case 256:
                riscv_radix4_butterfly_inverse_f32(p_instance, pSrc, fftLen);
                break;
            case 128:
            case 512:
                riscv_cfft_radix4by2_inverse_f32(p_instance, pSrc, fftLen);
                break;
        }
    }
    else
    {
        switch (fftLen)
        {
            case 256:
                riscv_radix4_butterfly_f32(p_instance, pSrc, fftLen);
                break;
            case 128:
            case 512:
                riscv_cfft_radix4by2_f32(p_instance, pSrc, fftLen);
                break;
        }
    }

    if (bitReverseFlagR)
    {
        riscv_bitreversal_32_inpl((uint32_t *)pSrc,
                                  p_instance->bitRevLength,
                                  p_instance->pBitRevTable);
    }
}
