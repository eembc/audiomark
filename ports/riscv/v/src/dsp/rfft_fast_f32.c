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
riscv_rfft_fast_init_f32(riscv_rfft_fast_instance_f32 *__EE_RESTRICT S, uint16_t fftLenReal)
{
    riscv_status status = RISCV_MATH_SUCCESS;

    /*  Initialize the Real FFT length */
    S->fftLenRFFT = (uint16_t)fftLenReal;

    /*  Initialization of coef modifier depending on the FFT length */
    switch (fftLenReal)
    {
        case 1024U:
            S->pTwiddleRFFT = (float32_t *)twiddleCoef_rfft_f32_1024;
            status          = riscv_cfft_init_f32(&(S->Sint), 512);
            break;
        case 512U:
            S->pTwiddleRFFT = (float32_t *)twiddleCoef_rfft_f32_512;
            status          = riscv_cfft_init_f32(&(S->Sint), 256);
            break;
        default:
            status = RISCV_MATH_ERROR;
            break;
    }

    return status;
}

void
stage_rfft_f32(const riscv_rfft_fast_instance_f32 *__EE_RESTRICT S,
               float32_t                          *__EE_RESTRICT p,
               float32_t                          *__EE_RESTRICT pOut)
{
    int32_t          k      = (S->Sint).fftLen - 1;
    const float32_t *pCoeff = S->pTwiddleRFFT;

    float32_t *pA = p;
    float32_t *pB = p;

    float32_t xBR = pB[0];
    float32_t xBI = pB[1];
    float32_t xAR = pA[0];
    float32_t xAI = pA[1];

    pCoeff += 2;

    float32_t t1a = xBR + xAR;
    float32_t t1b = xBI + xAI;

    *pOut++ = 0.5f * (t1a + t1b);
    *pOut++ = 0.5f * (t1a - t1b);

    pB = p + 2 * k;
    pA += 2;

    size_t blkCnt = k;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vA  = __riscv_vlseg2e32_v_f32m2x2(pA, vl);
        vfloat32m2_t   xAr = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
        vfloat32m2_t   xAi = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

        vfloat32m2x2_t vB  = __riscv_vlsseg2e32_v_f32m2x2(pB, -8, vl);
        vfloat32m2_t   xBr = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
        vfloat32m2_t   xBi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

        vfloat32m2x2_t vTw = __riscv_vlseg2e32_v_f32m2x2(pCoeff, vl);
        vfloat32m2_t   twR = __riscv_vget_v_f32m2x2_f32m2(vTw, 0);
        vfloat32m2_t   twI = __riscv_vget_v_f32m2x2_f32m2(vTw, 1);

        /* tmp1 = xA + xB */
        vfloat32m2_t tmp1R = __riscv_vfadd_vv_f32m2(xAr, xBr, vl);
        vfloat32m2_t tmp1I = __riscv_vfsub_vv_f32m2(xAi, xBi, vl);

        /* tmp2 = xB - xA */
        vfloat32m2_t tmp2R = __riscv_vfsub_vv_f32m2(xBr, xAr, vl);
        vfloat32m2_t tmp2I = __riscv_vfadd_vv_f32m2(xBi, xAi, vl);

        /* res = tw * tmp2 */
        vfloat32m2_t resR = __riscv_vfmul_vv_f32m2(twR, tmp2R, vl);
        resR              = __riscv_vfmacc_vv_f32m2(resR, twI, tmp2I, vl);

        vfloat32m2_t resI = __riscv_vfmul_vv_f32m2(twI, tmp2R, vl);
        resI              = __riscv_vfnmsac_vv_f32m2(resI, twR, tmp2I, vl);

        vfloat32m2_t half1R = __riscv_vfmul_vf_f32m2(tmp1R, 0.5f, vl);
        vfloat32m2_t half1I = __riscv_vfmul_vf_f32m2(tmp1I, 0.5f, vl);

        /* res = (res + tmp1) * 0.5f*/
        resR = __riscv_vfmacc_vf_f32m2(half1R, 0.5f, resR, vl);
        resI = __riscv_vfmacc_vf_f32m2(half1I, 0.5f, resI, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pOut, __riscv_vcreate_v_f32m2x2(resR, resI), vl);

        pA += 2 * vl;
        pB -= 2 * vl;
        pCoeff += 2 * vl;
        pOut += 2 * vl;
        blkCnt -= vl;
    }
}

void
merge_rfft_f32(const riscv_rfft_fast_instance_f32 *S,
               float32_t                          *p,
               float32_t                          *pOut)
{
    int32_t          k      = (S->Sint).fftLen - 1;
    const float32_t *pCoeff = S->pTwiddleRFFT;

    float32_t *pA = p;
    float32_t *pB = p;

    float32_t xAR = pA[0];
    float32_t xAI = pA[1];

    pCoeff += 2;

    *pOut++ = 0.5f * (xAR + xAI);
    *pOut++ = 0.5f * (xAR - xAI);

    pB = p + 2 * k;
    pA += 2;

    size_t blkCnt = k;

    while (blkCnt > 0)
    {
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);

        vfloat32m2x2_t vA  = __riscv_vlseg2e32_v_f32m2x2(pA, vl);
        vfloat32m2_t   xAr = __riscv_vget_v_f32m2x2_f32m2(vA, 0);
        vfloat32m2_t   xAi = __riscv_vget_v_f32m2x2_f32m2(vA, 1);

        vfloat32m2x2_t vB  = __riscv_vlsseg2e32_v_f32m2x2(pB, -8, vl);
        vfloat32m2_t   xBr = __riscv_vget_v_f32m2x2_f32m2(vB, 0);
        vfloat32m2_t   xBi = __riscv_vget_v_f32m2x2_f32m2(vB, 1);

        vfloat32m2x2_t vTw = __riscv_vlseg2e32_v_f32m2x2(pCoeff, vl);
        vfloat32m2_t   twR = __riscv_vget_v_f32m2x2_f32m2(vTw, 0);
        vfloat32m2_t   twI = __riscv_vget_v_f32m2x2_f32m2(vTw, 1);

        vfloat32m2_t tmp1R = __riscv_vfadd_vv_f32m2(xAr, xBr, vl);
        vfloat32m2_t tmp1I = __riscv_vfsub_vv_f32m2(xAi, xBi, vl);

        vfloat32m2_t tmp2R = __riscv_vfsub_vv_f32m2(xAr, xBr, vl);
        vfloat32m2_t tmp2I = __riscv_vfadd_vv_f32m2(xAi, xBi, vl);

        /* res = conj(tw) * tmp2 */
        vfloat32m2_t resR = __riscv_vfmul_vv_f32m2(twR, tmp2R, vl);
        resR              = __riscv_vfmacc_vv_f32m2(resR, twI, tmp2I, vl);

        vfloat32m2_t resI = __riscv_vfmul_vv_f32m2(twR, tmp2I, vl);
        resI              = __riscv_vfnmsac_vv_f32m2(resI, twI, tmp2R, vl);

        vfloat32m2_t half1R = __riscv_vfmul_vf_f32m2(tmp1R, 0.5f, vl);
        vfloat32m2_t half1I = __riscv_vfmul_vf_f32m2(tmp1I, 0.5f, vl);

        /* res = (res + tmp1) * 0.5f*/
        resR = __riscv_vfnmsac_vf_f32m2(half1R, 0.5f, resR, vl);
        resI = __riscv_vfnmsac_vf_f32m2(half1I, 0.5f, resI, vl);

        __riscv_vsseg2e32_v_f32m2x2(
            pOut, __riscv_vcreate_v_f32m2x2(resR, resI), vl);

        pA += 2 * vl;
        pB -= 2 * vl;
        pCoeff += 2 * vl;
        pOut += 2 * vl;
        blkCnt -= vl;
    }
}

void
riscv_rfft_fast_f32(const riscv_rfft_fast_instance_f32 *__EE_RESTRICT S,
                    float32_t                          *__EE_RESTRICT p,
                    float32_t                          *__EE_RESTRICT pOut,
                    uint8_t                                           ifftFlag)
{
    const riscv_cfft_instance_f32 *Sint = &(S->Sint);

    /* Calculation of Real FFT */
    if (ifftFlag)
    {
        /*  Real FFT compression */
        merge_rfft_f32(S, p, pOut);

        /* Complex radix-4 IFFT process */
        riscv_cfft_f32(Sint, pOut, ifftFlag, 1);
    }
    else
    {
        /* Calculation of RFFT of input */
        riscv_cfft_f32(Sint, p, ifftFlag, 1);

        /*  Real FFT extraction */
        stage_rfft_f32(S, p, pOut);
    }
}
