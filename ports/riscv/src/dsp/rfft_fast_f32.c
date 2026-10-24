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

riscv_status
riscv_rfft_fast_init_f32(riscv_rfft_fast_instance_f32 *S, uint16_t fftLenReal)
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
            status = RISCV_MATH_SUCCESS;
            break;
    }

    return status;
}

static void
stage_rfft_f32(const riscv_rfft_fast_instance_f32 *S,
               float32_t                          *p,
               float32_t                          *pOut)
{
    int32_t          k;        /* Loop Counter */
    float32_t        twR, twI; /* RFFT Twiddle coefficients */
    const float32_t *pCoeff
        = S->pTwiddleRFFT;         /* Points to RFFT Twiddle factors */
    float32_t *pA = p;             /* increasing pointer */
    float32_t *pB = p;             /* decreasing pointer */
    float32_t  xAR, xAI, xBR, xBI; /* temporary variables */
    float32_t  t1a, t1b;           /* temporary variables */
    float32_t  p0, p1, p2, p3;     /* temporary variables */

    k = (S->Sint).fftLen - 1;

    /* Pack first and last sample of the frequency domain together */

    xBR = pB[0];
    xBI = pB[1];
    xAR = pA[0];
    xAI = pA[1];

    twR = *pCoeff++;
    twI = *pCoeff++;

    // U1 = XA(1) + XB(1); % It is real
    t1a = xBR + xAR;

    // U2 = XB(1) - XA(1); % It is imaginary
    t1b = xBI + xAI;

    // real(tw * (xB - xA)) = twR * (xBR - xAR) - twI * (xBI - xAI);
    // imag(tw * (xB - xA)) = twI * (xBR - xAR) + twR * (xBI - xAI);
    *pOut++ = 0.5f * (t1a + t1b);
    *pOut++ = 0.5f * (t1a - t1b);

    // XA(1) = 1/2*( U1 - imag(U2) +  i*( U1 +imag(U2) ));
    pB = p + 2 * k;
    pA += 2;

    do
    {
        /*
           function X = my_split_rfft(X, ifftFlag)
           % X is a series of real numbers
           L  = length(X);
           XC = X(1:2:end) +i*X(2:2:end);
           XA = fft(XC);
           XB = conj(XA([1 end:-1:2]));
           TW = i*exp(-2*pi*i*[0:L/2-1]/L).';
           for l = 2:L/2
              XA(l) = 1/2 * (XA(l) + XB(l) + TW(l) * (XB(l) - XA(l)));
           end
           XA(1) = 1/2* (XA(1) + XB(1) + TW(1) * (XB(1) - XA(1))) + i*( 1/2*(
           XA(1) + XB(1) + i*( XA(1) - XB(1)))); X = XA;
        */

        xBI = pB[1];
        xBR = pB[0];
        xAR = pA[0];
        xAI = pA[1];

        twR = *pCoeff++;
        twI = *pCoeff++;

        t1a = xBR - xAR;
        t1b = xBI + xAI;

        // real(tw * (xB - xA)) = twR * (xBR - xAR) - twI * (xBI - xAI);
        // imag(tw * (xB - xA)) = twI * (xBR - xAR) + twR * (xBI - xAI);
        p0 = twR * t1a;
        p1 = twI * t1a;
        p2 = twR * t1b;
        p3 = twI * t1b;

        *pOut++ = 0.5f * (xAR + xBR + p0 + p3); // xAR
        *pOut++ = 0.5f * (xAI - xBI + p1 - p2); // xAI

        pA += 2;
        pB -= 2;
        k--;
    } while (k > 0);
}

/* Prepares data for inverse cfft */
static void
merge_rfft_f32(const riscv_rfft_fast_instance_f32 *S,
               float32_t                          *p,
               float32_t                          *pOut)
{
    int32_t          k;        /* Loop Counter */
    float32_t        twR, twI; /* RFFT Twiddle coefficients */
    const float32_t *pCoeff
        = S->pTwiddleRFFT;           /* Points to RFFT Twiddle factors */
    float32_t *pA = p;               /* increasing pointer */
    float32_t *pB = p;               /* decreasing pointer */
    float32_t  xAR, xAI, xBR, xBI;   /* temporary variables */
    float32_t  t1a, t1b, r, s, t, u; /* temporary variables */

    k = (S->Sint).fftLen - 1;

    xAR = pA[0];
    xAI = pA[1];

    pCoeff += 2;

    *pOut++ = 0.5f * (xAR + xAI);
    *pOut++ = 0.5f * (xAR - xAI);

    pB = p + 2 * k;
    pA += 2;

    while (k > 0)
    {
        /* G is half of the frequency complex spectrum */
        // for k = 2:N
        //     Xk(k) = 1/2 * (G(k) + conj(G(N-k+2)) + Tw(k)*( G(k) -
        //     conj(G(N-k+2))));
        xBI = pB[1];
        xBR = pB[0];
        xAR = pA[0];
        xAI = pA[1];

        twR = *pCoeff++;
        twI = *pCoeff++;

        t1a = xAR - xBR;
        t1b = xAI + xBI;

        r = twR * t1a;
        s = twI * t1b;
        t = twI * t1a;
        u = twR * t1b;

        // real(tw * (xA - xB)) = twR * (xAR - xBR) - twI * (xAI - xBI);
        // imag(tw * (xA - xB)) = twI * (xAR - xBR) + twR * (xAI - xBI);
        *pOut++ = 0.5f * (xAR + xBR - r - s); // xAR
        *pOut++ = 0.5f * (xAI - xBI + t - u); // xAI

        pA += 2;
        pB -= 2;
        k--;
    }
}

void
riscv_rfft_fast_f32(const riscv_rfft_fast_instance_f32 *S,
                    float32_t                          *p,
                    float32_t                          *pOut,
                    uint8_t                             ifftFlag)
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
