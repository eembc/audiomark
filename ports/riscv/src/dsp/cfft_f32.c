/*
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
#include <stdlib.h>

riscv_status
riscv_cfft_init_f32(riscv_cfft_instance_f32 *S, uint16_t fftLen)
{
    /*  Initialise the default arm status */
    riscv_status status = RISCV_MATH_SUCCESS;

    /*  Initialise the FFT length */
    S->fftLen = fftLen;

    /*  Initialise the Twiddle coefficient pointer */
    S->pTwiddle = NULL;

    /*  Initializations of Instance structure depending on the FFT length */
    switch (S->fftLen)
    {
        case 512U:
            S->bitRevLength = RISCVBITREVINDEXTABLE_RADIX8_512_TABLE_LENGTH;
            S->pBitRevTable = riscvBitRevIndexTable_r8_512;
            S->pTwiddle     = twiddleCoef_f32_512;
            break;

        case 256U:
            S->bitRevLength = RISCVBITREVINDEXTABLE_RADIX8_256_TABLE_LENGTH;
            S->pBitRevTable = riscvBitRevIndexTable_r8_256;
            S->pTwiddle     = twiddleCoef_f32_256;
            break;

        case 128U:
            S->bitRevLength = RISCVBITREVINDEXTABLE_RADIX8_128_TABLE_LENGTH;
            S->pBitRevTable = riscvBitRevIndexTable_r8_128;
            S->pTwiddle     = twiddleCoef_f32_128;
            break;

        default:
            /*  Reporting error if fftSize is not valid value */
            status = RISCV_MATH_ERROR;
            break;
    }

    return (status);
}

static void
riscv_radix8_butterfly_f32(float32_t       *pSrc,
                           uint16_t         fftLen,
                           const float32_t *pCoef,
                           uint16_t         twidCoefModifier)
{
    uint32_t ia1, ia2, ia3, ia4, ia5, ia6, ia7;
    uint32_t i1, i2, i3, i4, i5, i6, i7, i8;
    uint32_t id;
    uint32_t n1, n2, j;

    float32_t       r1, r2, r3, r4, r5, r6, r7, r8;
    float32_t       t1, t2;
    float32_t       s1, s2, s3, s4, s5, s6, s7, s8;
    float32_t       p1, p2, p3, p4;
    float32_t       co2, co3, co4, co5, co6, co7, co8;
    float32_t       si2, si3, si4, si5, si6, si7, si8;
    const float32_t C81 = 0.70710678118f;

    n2 = fftLen;

    do
    {
        n1 = n2;
        n2 = n2 >> 3;
        i1 = 0;

        do
        {
            i2               = i1 + n2;
            i3               = i2 + n2;
            i4               = i3 + n2;
            i5               = i4 + n2;
            i6               = i5 + n2;
            i7               = i6 + n2;
            i8               = i7 + n2;
            r1               = pSrc[2 * i1] + pSrc[2 * i5];
            r5               = pSrc[2 * i1] - pSrc[2 * i5];
            r2               = pSrc[2 * i2] + pSrc[2 * i6];
            r6               = pSrc[2 * i2] - pSrc[2 * i6];
            r3               = pSrc[2 * i3] + pSrc[2 * i7];
            r7               = pSrc[2 * i3] - pSrc[2 * i7];
            r4               = pSrc[2 * i4] + pSrc[2 * i8];
            r8               = pSrc[2 * i4] - pSrc[2 * i8];
            t1               = r1 - r3;
            r1               = r1 + r3;
            r3               = r2 - r4;
            r2               = r2 + r4;
            pSrc[2 * i1]     = r1 + r2;
            pSrc[2 * i5]     = r1 - r2;
            r1               = pSrc[2 * i1 + 1] + pSrc[2 * i5 + 1];
            s5               = pSrc[2 * i1 + 1] - pSrc[2 * i5 + 1];
            r2               = pSrc[2 * i2 + 1] + pSrc[2 * i6 + 1];
            s6               = pSrc[2 * i2 + 1] - pSrc[2 * i6 + 1];
            s3               = pSrc[2 * i3 + 1] + pSrc[2 * i7 + 1];
            s7               = pSrc[2 * i3 + 1] - pSrc[2 * i7 + 1];
            r4               = pSrc[2 * i4 + 1] + pSrc[2 * i8 + 1];
            s8               = pSrc[2 * i4 + 1] - pSrc[2 * i8 + 1];
            t2               = r1 - s3;
            r1               = r1 + s3;
            s3               = r2 - r4;
            r2               = r2 + r4;
            pSrc[2 * i1 + 1] = r1 + r2;
            pSrc[2 * i5 + 1] = r1 - r2;
            pSrc[2 * i3]     = t1 + s3;
            pSrc[2 * i7]     = t1 - s3;
            pSrc[2 * i3 + 1] = t2 - r3;
            pSrc[2 * i7 + 1] = t2 + r3;
            r1               = (r6 - r8) * C81;
            r6               = (r6 + r8) * C81;
            r2               = (s6 - s8) * C81;
            s6               = (s6 + s8) * C81;
            t1               = r5 - r1;
            r5               = r5 + r1;
            r8               = r7 - r6;
            r7               = r7 + r6;
            t2               = s5 - r2;
            s5               = s5 + r2;
            s8               = s7 - s6;
            s7               = s7 + s6;
            pSrc[2 * i2]     = r5 + s7;
            pSrc[2 * i8]     = r5 - s7;
            pSrc[2 * i6]     = t1 + s8;
            pSrc[2 * i4]     = t1 - s8;
            pSrc[2 * i2 + 1] = s5 - r7;
            pSrc[2 * i8 + 1] = s5 + r7;
            pSrc[2 * i6 + 1] = t2 - r8;
            pSrc[2 * i4 + 1] = t2 + r8;

            i1 += n1;
        } while (i1 < fftLen);

        if (n2 < 8)
        {
            break;
        }

        ia1 = 0;
        j   = 1;

        do
        {
            /*  index calculation for the coefficients */
            id  = ia1 + twidCoefModifier;
            ia1 = id;
            ia2 = ia1 + id;
            ia3 = ia2 + id;
            ia4 = ia3 + id;
            ia5 = ia4 + id;
            ia6 = ia5 + id;
            ia7 = ia6 + id;

            co2 = pCoef[2 * ia1];
            co3 = pCoef[2 * ia2];
            co4 = pCoef[2 * ia3];
            co5 = pCoef[2 * ia4];
            co6 = pCoef[2 * ia5];
            co7 = pCoef[2 * ia6];
            co8 = pCoef[2 * ia7];
            si2 = pCoef[2 * ia1 + 1];
            si3 = pCoef[2 * ia2 + 1];
            si4 = pCoef[2 * ia3 + 1];
            si5 = pCoef[2 * ia4 + 1];
            si6 = pCoef[2 * ia5 + 1];
            si7 = pCoef[2 * ia6 + 1];
            si8 = pCoef[2 * ia7 + 1];

            i1 = j;

            do
            {
                /*  index calculation for the input */
                i2               = i1 + n2;
                i3               = i2 + n2;
                i4               = i3 + n2;
                i5               = i4 + n2;
                i6               = i5 + n2;
                i7               = i6 + n2;
                i8               = i7 + n2;
                r1               = pSrc[2 * i1] + pSrc[2 * i5];
                r5               = pSrc[2 * i1] - pSrc[2 * i5];
                r2               = pSrc[2 * i2] + pSrc[2 * i6];
                r6               = pSrc[2 * i2] - pSrc[2 * i6];
                r3               = pSrc[2 * i3] + pSrc[2 * i7];
                r7               = pSrc[2 * i3] - pSrc[2 * i7];
                r4               = pSrc[2 * i4] + pSrc[2 * i8];
                r8               = pSrc[2 * i4] - pSrc[2 * i8];
                t1               = r1 - r3;
                r1               = r1 + r3;
                r3               = r2 - r4;
                r2               = r2 + r4;
                pSrc[2 * i1]     = r1 + r2;
                r2               = r1 - r2;
                s1               = pSrc[2 * i1 + 1] + pSrc[2 * i5 + 1];
                s5               = pSrc[2 * i1 + 1] - pSrc[2 * i5 + 1];
                s2               = pSrc[2 * i2 + 1] + pSrc[2 * i6 + 1];
                s6               = pSrc[2 * i2 + 1] - pSrc[2 * i6 + 1];
                s3               = pSrc[2 * i3 + 1] + pSrc[2 * i7 + 1];
                s7               = pSrc[2 * i3 + 1] - pSrc[2 * i7 + 1];
                s4               = pSrc[2 * i4 + 1] + pSrc[2 * i8 + 1];
                s8               = pSrc[2 * i4 + 1] - pSrc[2 * i8 + 1];
                t2               = s1 - s3;
                s1               = s1 + s3;
                s3               = s2 - s4;
                s2               = s2 + s4;
                r1               = t1 + s3;
                t1               = t1 - s3;
                pSrc[2 * i1 + 1] = s1 + s2;
                s2               = s1 - s2;
                s1               = t2 - r3;
                t2               = t2 + r3;
                p1               = co5 * r2;
                p2               = si5 * s2;
                p3               = co5 * s2;
                p4               = si5 * r2;
                pSrc[2 * i5]     = p1 + p2;
                pSrc[2 * i5 + 1] = p3 - p4;
                p1               = co3 * r1;
                p2               = si3 * s1;
                p3               = co3 * s1;
                p4               = si3 * r1;
                pSrc[2 * i3]     = p1 + p2;
                pSrc[2 * i3 + 1] = p3 - p4;
                p1               = co7 * t1;
                p2               = si7 * t2;
                p3               = co7 * t2;
                p4               = si7 * t1;
                pSrc[2 * i7]     = p1 + p2;
                pSrc[2 * i7 + 1] = p3 - p4;
                r1               = (r6 - r8) * C81;
                r6               = (r6 + r8) * C81;
                s1               = (s6 - s8) * C81;
                s6               = (s6 + s8) * C81;
                t1               = r5 - r1;
                r5               = r5 + r1;
                r8               = r7 - r6;
                r7               = r7 + r6;
                t2               = s5 - s1;
                s5               = s5 + s1;
                s8               = s7 - s6;
                s7               = s7 + s6;
                r1               = r5 + s7;
                r5               = r5 - s7;
                r6               = t1 + s8;
                t1               = t1 - s8;
                s1               = s5 - r7;
                s5               = s5 + r7;
                s6               = t2 - r8;
                t2               = t2 + r8;
                p1               = co2 * r1;
                p2               = si2 * s1;
                p3               = co2 * s1;
                p4               = si2 * r1;
                pSrc[2 * i2]     = p1 + p2;
                pSrc[2 * i2 + 1] = p3 - p4;
                p1               = co8 * r5;
                p2               = si8 * s5;
                p3               = co8 * s5;
                p4               = si8 * r5;
                pSrc[2 * i8]     = p1 + p2;
                pSrc[2 * i8 + 1] = p3 - p4;
                p1               = co6 * r6;
                p2               = si6 * s6;
                p3               = co6 * s6;
                p4               = si6 * r6;
                pSrc[2 * i6]     = p1 + p2;
                pSrc[2 * i6 + 1] = p3 - p4;
                p1               = co4 * t1;
                p2               = si4 * t2;
                p3               = co4 * t2;
                p4               = si4 * t1;
                pSrc[2 * i4]     = p1 + p2;
                pSrc[2 * i4 + 1] = p3 - p4;

                i1 += n1;
            } while (i1 < fftLen);

            j++;
        } while (j < n2);

        twidCoefModifier <<= 3;
    } while (n2 > 7);
}

static void
riscv_bitreversal_32(uint32_t       *pSrc,
                     const uint16_t  bitRevLen,
                     const uint16_t *pBitRevTab)
{
    uint32_t a, b, i, tmp;

    for (i = 0; i < bitRevLen;)
    {
        a = pBitRevTab[i] >> 2;
        b = pBitRevTab[i + 1] >> 2;

        // real
        tmp     = pSrc[a];
        pSrc[a] = pSrc[b];
        pSrc[b] = tmp;

        // complex
        tmp         = pSrc[a + 1];
        pSrc[a + 1] = pSrc[b + 1];
        pSrc[b + 1] = tmp;

        i += 2;
    }
}

static void
riscv_cfft_radix8by2_f32(riscv_cfft_instance_f32 *S, float32_t *p1)
{
    uint32_t         L = S->fftLen;
    float32_t       *pCol1, *pCol2, *pMid1, *pMid2;
    float32_t       *p2 = p1 + L;
    const float32_t *tw = (float32_t *)S->pTwiddle;
    float32_t        t1[4], t2[4], t3[4], t4[4], twR, twI;
    float32_t        m0, m1, m2, m3;
    uint32_t         l;

    pCol1 = p1;
    pCol2 = p2;

    /* Define new length */
    L >>= 1;

    /* Initialize mid pointers */
    pMid1 = p1 + L;
    pMid2 = p2 + L;

    /* do two dot Fourier transform */
    for (l = L >> 2; l > 0; l--)
    {
        t1[0] = p1[0];
        t1[1] = p1[1];
        t1[2] = p1[2];
        t1[3] = p1[3];

        t2[0] = p2[0];
        t2[1] = p2[1];
        t2[2] = p2[2];
        t2[3] = p2[3];

        t3[0] = pMid1[0];
        t3[1] = pMid1[1];
        t3[2] = pMid1[2];
        t3[3] = pMid1[3];

        t4[0] = pMid2[0];
        t4[1] = pMid2[1];
        t4[2] = pMid2[2];
        t4[3] = pMid2[3];

        *p1++ = t1[0] + t2[0];
        *p1++ = t1[1] + t2[1];
        *p1++ = t1[2] + t2[2];
        *p1++ = t1[3] + t2[3]; /* col 1 */

        t2[0] = t1[0] - t2[0];
        t2[1] = t1[1] - t2[1];
        t2[2] = t1[2] - t2[2];
        t2[3] = t1[3] - t2[3]; /* for col 2 */

        *pMid1++ = t3[0] + t4[0];
        *pMid1++ = t3[1] + t4[1];
        *pMid1++ = t3[2] + t4[2];
        *pMid1++ = t3[3] + t4[3]; /* col 1 */

        t4[0] = t4[0] - t3[0];
        t4[1] = t4[1] - t3[1];
        t4[2] = t4[2] - t3[2];
        t4[3] = t4[3] - t3[3]; /* for col 2 */

        twR = *tw++;
        twI = *tw++;

        /* multiply by twiddle factors */
        m0 = t2[0] * twR;
        m1 = t2[1] * twI;
        m2 = t2[1] * twR;
        m3 = t2[0] * twI;

        /* R  =  R  *  Tr - I * Ti */
        *p2++ = m0 + m1;
        /* I  =  I  *  Tr + R * Ti */
        *p2++ = m2 - m3;

        /* use vertical symmetry */
        /*  0.9988 - 0.0491i <==> -0.0491 - 0.9988i */
        m0 = t4[0] * twI;
        m1 = t4[1] * twR;
        m2 = t4[1] * twI;
        m3 = t4[0] * twR;

        *pMid2++ = m0 - m1;
        *pMid2++ = m2 + m3;

        twR = *tw++;
        twI = *tw++;

        m0 = t2[2] * twR;
        m1 = t2[3] * twI;
        m2 = t2[3] * twR;
        m3 = t2[2] * twI;

        *p2++ = m0 + m1;
        *p2++ = m2 - m3;

        m0 = t4[2] * twI;
        m1 = t4[3] * twR;
        m2 = t4[3] * twI;
        m3 = t4[2] * twR;

        *pMid2++ = m0 - m1;
        *pMid2++ = m2 + m3;
    }

    /* first col */
    riscv_radix8_butterfly_f32(pCol1, L, (float32_t *)S->pTwiddle, 2U);

    /* second col */
    riscv_radix8_butterfly_f32(pCol2, L, (float32_t *)S->pTwiddle, 2U);
}

static void
riscv_cfft_radix8by4_f32(riscv_cfft_instance_f32 *S, float32_t *p1)
{
    uint32_t   L = S->fftLen >> 1;
    float32_t *pCol1, *pCol2, *pCol3, *pCol4, *pEnd1, *pEnd2, *pEnd3, *pEnd4;
    const float32_t *tw2, *tw3, *tw4;
    float32_t       *p2 = p1 + L;
    float32_t       *p3 = p2 + L;
    float32_t       *p4 = p3 + L;
    float32_t        t2[4], t3[4], t4[4], twR, twI;
    float32_t        p1ap3_0, p1sp3_0, p1ap3_1, p1sp3_1;
    float32_t        m0, m1, m2, m3;
    uint32_t         l, twMod2, twMod3, twMod4;

    pCol1 = p1; /* points to real values by default */
    pCol2 = p2;
    pCol3 = p3;
    pCol4 = p4;
    pEnd1 = p2 - 1; /* points to imaginary values by default */
    pEnd2 = p3 - 1;
    pEnd3 = p4 - 1;
    pEnd4 = pEnd3 + L;

    tw2 = tw3 = tw4 = (float32_t *)S->pTwiddle;

    L >>= 1;

    /* do four dot Fourier transform */

    twMod2 = 2;
    twMod3 = 4;
    twMod4 = 6;

    /* TOP */
    p1ap3_0 = p1[0] + p3[0];
    p1sp3_0 = p1[0] - p3[0];
    p1ap3_1 = p1[1] + p3[1];
    p1sp3_1 = p1[1] - p3[1];

    /* col 2 */
    t2[0] = p1sp3_0 + p2[1] - p4[1];
    t2[1] = p1sp3_1 - p2[0] + p4[0];
    /* col 3 */
    t3[0] = p1ap3_0 - p2[0] - p4[0];
    t3[1] = p1ap3_1 - p2[1] - p4[1];
    /* col 4 */
    t4[0] = p1sp3_0 - p2[1] + p4[1];
    t4[1] = p1sp3_1 + p2[0] - p4[0];
    /* col 1 */
    *p1++ = p1ap3_0 + p2[0] + p4[0];
    *p1++ = p1ap3_1 + p2[1] + p4[1];

    /* Twiddle factors are ones */
    *p2++ = t2[0];
    *p2++ = t2[1];
    *p3++ = t3[0];
    *p3++ = t3[1];
    *p4++ = t4[0];
    *p4++ = t4[1];

    tw2 += twMod2;
    tw3 += twMod3;
    tw4 += twMod4;

    for (l = (L - 2) >> 1; l > 0; l--)
    {
        /* TOP */
        p1ap3_0 = p1[0] + p3[0];
        p1sp3_0 = p1[0] - p3[0];
        p1ap3_1 = p1[1] + p3[1];
        p1sp3_1 = p1[1] - p3[1];
        /* col 2 */
        t2[0] = p1sp3_0 + p2[1] - p4[1];
        t2[1] = p1sp3_1 - p2[0] + p4[0];
        /* col 3 */
        t3[0] = p1ap3_0 - p2[0] - p4[0];
        t3[1] = p1ap3_1 - p2[1] - p4[1];
        /* col 4 */
        t4[0] = p1sp3_0 - p2[1] + p4[1];
        t4[1] = p1sp3_1 + p2[0] - p4[0];
        /* col 1 - top */
        *p1++ = p1ap3_0 + p2[0] + p4[0];
        *p1++ = p1ap3_1 + p2[1] + p4[1];

        /* BOTTOM */
        p1ap3_1 = pEnd1[-1] + pEnd3[-1];
        p1sp3_1 = pEnd1[-1] - pEnd3[-1];
        p1ap3_0 = pEnd1[0] + pEnd3[0];
        p1sp3_0 = pEnd1[0] - pEnd3[0];
        /* col 2 */
        t2[2] = pEnd2[0] - pEnd4[0] + p1sp3_1;
        t2[3] = pEnd1[0] - pEnd3[0] - pEnd2[-1] + pEnd4[-1];
        /* col 3 */
        t3[2] = p1ap3_1 - pEnd2[-1] - pEnd4[-1];
        t3[3] = p1ap3_0 - pEnd2[0] - pEnd4[0];
        /* col 4 */
        t4[2] = pEnd2[0] - pEnd4[0] - p1sp3_1;
        t4[3] = pEnd4[-1] - pEnd2[-1] - p1sp3_0;
        /* col 1 - Bottom */
        *pEnd1-- = p1ap3_0 + pEnd2[0] + pEnd4[0];
        *pEnd1-- = p1ap3_1 + pEnd2[-1] + pEnd4[-1];

        /* COL 2 */
        /* read twiddle factors */
        twR = *tw2++;
        twI = *tw2++;
        /* multiply by twiddle factors */
        /*  let    Z1 = a + i(b),   Z2 = c + i(d) */
        /*   =>  Z1 * Z2  =  (a*c - b*d) + i(b*c + a*d) */

        /* Top */
        m0 = t2[0] * twR;
        m1 = t2[1] * twI;
        m2 = t2[1] * twR;
        m3 = t2[0] * twI;

        *p2++ = m0 + m1;
        *p2++ = m2 - m3;
        /* use vertical symmetry col 2 */
        /* 0.9997 - 0.0245i  <==>  0.0245 - 0.9997i */
        /* Bottom */
        m0 = t2[3] * twI;
        m1 = t2[2] * twR;
        m2 = t2[2] * twI;
        m3 = t2[3] * twR;

        *pEnd2-- = m0 - m1;
        *pEnd2-- = m2 + m3;

        /* COL 3 */
        twR = tw3[0];
        twI = tw3[1];
        tw3 += twMod3;
        /* Top */
        m0 = t3[0] * twR;
        m1 = t3[1] * twI;
        m2 = t3[1] * twR;
        m3 = t3[0] * twI;

        *p3++ = m0 + m1;
        *p3++ = m2 - m3;
        /* use vertical symmetry col 3 */
        /* 0.9988 - 0.0491i  <==>  -0.9988 - 0.0491i */
        /* Bottom */
        m0 = -t3[3] * twR;
        m1 = t3[2] * twI;
        m2 = t3[2] * twR;
        m3 = t3[3] * twI;

        *pEnd3-- = m0 - m1;
        *pEnd3-- = m3 - m2;

        /* COL 4 */
        twR = tw4[0];
        twI = tw4[1];
        tw4 += twMod4;
        /* Top */
        m0 = t4[0] * twR;
        m1 = t4[1] * twI;
        m2 = t4[1] * twR;
        m3 = t4[0] * twI;

        *p4++ = m0 + m1;
        *p4++ = m2 - m3;
        /* use vertical symmetry col 4 */
        /* 0.9973 - 0.0736i  <==>  -0.0736 + 0.9973i */
        /* Bottom */
        m0 = t4[3] * twI;
        m1 = t4[2] * twR;
        m2 = t4[2] * twI;
        m3 = t4[3] * twR;

        *pEnd4-- = m0 - m1;
        *pEnd4-- = m2 + m3;
    }

    /* MIDDLE */
    /* Twiddle factors are */
    /*  1.0000  0.7071-0.7071i  -1.0000i  -0.7071-0.7071i */
    p1ap3_0 = p1[0] + p3[0];
    p1sp3_0 = p1[0] - p3[0];
    p1ap3_1 = p1[1] + p3[1];
    p1sp3_1 = p1[1] - p3[1];

    /* col 2 */
    t2[0] = p1sp3_0 + p2[1] - p4[1];
    t2[1] = p1sp3_1 - p2[0] + p4[0];
    /* col 3 */
    t3[0] = p1ap3_0 - p2[0] - p4[0];
    t3[1] = p1ap3_1 - p2[1] - p4[1];
    /* col 4 */
    t4[0] = p1sp3_0 - p2[1] + p4[1];
    t4[1] = p1sp3_1 + p2[0] - p4[0];
    /* col 1 - Top */
    *p1++ = p1ap3_0 + p2[0] + p4[0];
    *p1++ = p1ap3_1 + p2[1] + p4[1];

    /* COL 2 */
    twR = tw2[0];
    twI = tw2[1];

    m0 = t2[0] * twR;
    m1 = t2[1] * twI;
    m2 = t2[1] * twR;
    m3 = t2[0] * twI;

    *p2++ = m0 + m1;
    *p2++ = m2 - m3;
    /* COL 3 */
    twR = tw3[0];
    twI = tw3[1];

    m0 = t3[0] * twR;
    m1 = t3[1] * twI;
    m2 = t3[1] * twR;
    m3 = t3[0] * twI;

    *p3++ = m0 + m1;
    *p3++ = m2 - m3;
    /* COL 4 */
    twR = tw4[0];
    twI = tw4[1];

    m0 = t4[0] * twR;
    m1 = t4[1] * twI;
    m2 = t4[1] * twR;
    m3 = t4[0] * twI;

    *p4++ = m0 + m1;
    *p4++ = m2 - m3;

    /* first col */
    riscv_radix8_butterfly_f32(pCol1, L, (float32_t *)S->pTwiddle, 4U);

    /* second col */
    riscv_radix8_butterfly_f32(pCol2, L, (float32_t *)S->pTwiddle, 4U);

    /* third col */
    riscv_radix8_butterfly_f32(pCol3, L, (float32_t *)S->pTwiddle, 4U);

    /* fourth col */
    riscv_radix8_butterfly_f32(pCol4, L, (float32_t *)S->pTwiddle, 4U);
}

void
riscv_cfft_f32(const riscv_cfft_instance_f32 *S,
               float32_t                     *p1,
               uint8_t                        ifftFlag,
               uint8_t                        bitReverseFlag)
{
    uint32_t  L = S->fftLen, l;
    float32_t invL, *pSrc;

    if (ifftFlag == 1U)
    {
        /* Conjugate input data */
        pSrc = p1 + 1;
        for (l = 0; l < L; l++)
        {
            *pSrc = -*pSrc;
            pSrc += 2;
        }
    }

    switch (L)
    {
        case 16:
        case 128:
        case 1024:
            riscv_cfft_radix8by2_f32((riscv_cfft_instance_f32 *)S, p1);
            break;
        case 32:
        case 256:
        case 2048:
            riscv_cfft_radix8by4_f32((riscv_cfft_instance_f32 *)S, p1);
            break;
        case 64:
        case 512:
        case 4096:
            riscv_radix8_butterfly_f32(p1, L, (float32_t *)S->pTwiddle, 1);
            break;
    }

    if (bitReverseFlag)
    {
        riscv_bitreversal_32((uint32_t *)p1, S->bitRevLength, S->pBitRevTable);
    }

    if (ifftFlag == 1U)
    {
        invL = 1.0f / (float32_t)L;

        /* Conjugate and scale output data */
        pSrc = p1;
        for (l = 0; l < L; l++)
        {
            *pSrc++ *= invL;
            *pSrc = -(*pSrc) * invL;
            pSrc++;
        }
    }
}
