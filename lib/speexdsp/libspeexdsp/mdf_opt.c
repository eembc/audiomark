/* Copyright (C) 2003 Epic Games (written by Jean-Marc Valin)
   Copyright (C) 2004-2006 Epic Games

   File: preprocess.c
   Preprocessor with denoising based on the algorithm by Ephraim and Malah

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/



/* ARM optimized parts */

/*
 * Copyright (c) 2010-2022 Arm Limited or its affiliates. All rights reserved.
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



#ifdef GENERIC_ARCH

#include <stdint.h>

/*
 * Reference code for optimized routines
 */


#ifdef OVERRIDE_MDF_DC_NOTCH
static void filter_dc_notch16(const spx_int16_t * in, spx_word16_t radius, spx_word16_t * out, int len, spx_mem_t * mem, int stride)
{
    int             i;
    spx_word16_t    den2;
#ifdef FIXED_POINT
    den2 = MULT16_16_Q15(radius, radius) + MULT16_16_Q15(QCONST16(.7, 15), MULT16_16_Q15(32767 - radius, 32767 - radius));
#else
    den2 = radius * radius + .7 * (1 - radius) * (1 - radius);
#endif
    /*printf ("%d %d %d %d %d %d\n", num[0], num[1], num[2], den[0], den[1], den[2]); */
    for (i = 0; i < len; i++) {
        spx_word16_t    vin = in[i * stride];
        spx_word32_t    vout = mem[0] + SHL32(EXTEND32(vin), 15);
#ifdef FIXED_POINT
        mem[0] = mem[1] + SHL32(SHL32(-EXTEND32(vin), 15) + MULT16_32_Q15(radius, vout), 1);
#else
        mem[0] = mem[1] + 2 * (-vin + radius * vout);
#endif
        mem[1] = SHL32(EXTEND32(vin), 15) - MULT16_32_Q15(den2, vout);
        out[i] = SATURATE32(PSHR32(MULT16_32_Q15(radius, vout), 15), 32767);
    }
}

#endif

#ifdef OVERRIDE_MDF_INNER_PROD
static spx_word32_t mdf_inner_prod(const spx_word16_t * x, const spx_word16_t * y, int len)
{
    spx_word32_t    sum = 0;
    len >>= 1;
    while (len--) {
        spx_word32_t    part = 0;
        part = MAC16_16(part, *x++, *y++);
        part = MAC16_16(part, *x++, *y++);
        /* HINT: If you had a 40-bit accumulator, you could shift only at the end */
        sum = ADD32(sum, SHR32(part, 6));
    }
    return sum;
}
#endif

#ifdef OVERRIDE_MDF_POWER_SPECTRUM
static void power_spectrum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    int             i, j;
    ps[0] = MULT16_16(X[0], X[0]);
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        ps[j] = MULT16_16(X[i], X[i]) + MULT16_16(X[i + 1], X[i + 1]);
    }
    ps[j] = MULT16_16(X[i], X[i]);
}
#endif

/** Compute power spectrum of a half-complex (packed) vector and accumulate */
#ifdef OVERRIDE_MDF_POWER_SPECTRUM_ACCUM
static void power_spectrum_accum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    int             i, j;
    ps[0] += MULT16_16(X[0], X[0]);
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        ps[j] += MULT16_16(X[i], X[i]) + MULT16_16(X[i + 1], X[i + 1]);
    }
    ps[j] += MULT16_16(X[i], X[i]);
}
#endif


#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM
#ifndef FIXED_POINT
static void spectral_mul_accum(const spx_word16_t * X, const spx_word32_t * Y, spx_word16_t * acc, int N, int M)
{
    int             i, j;
    for (i = 0; i < N; i++)
        acc[i] = 0;
    for (j = 0; j < M; j++) {
        acc[0] += X[0] * Y[0];
        for (i = 1; i < N - 1; i += 2) {
            acc[i] += (X[i] * Y[i] - X[i + 1] * Y[i + 1]);
            acc[i + 1] += (X[i + 1] * Y[i] + X[i] * Y[i + 1]);
        }
        acc[i] += X[i] * Y[i];
        X += N;
        Y += N;
    }
}
#else
#error "fixed-point spectral_mul_accum to be added"
#endif

#endif

#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM16
#error "fixed-point spectral_mul_accum16 to be added"
#endif

#ifdef OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ
static void weighted_spectral_mul_conj(const spx_float_t * w, const spx_float_t p, const spx_word16_t * X, const spx_word16_t * Y, spx_word32_t * prod, int N)
{
    int             i, j;
    spx_float_t     W;
    W = FLOAT_AMULT(p, w[0]);
    prod[0] = FLOAT_MUL32(W, MULT16_16(X[0], Y[0]));
    for (i = 1, j = 1; i < N - 1; i += 2, j++) {
        W = FLOAT_AMULT(p, w[j]);
        prod[i] = FLOAT_MUL32(W, MAC16_16(MULT16_16(X[i], Y[i]), X[i + 1], Y[i + 1]));
        prod[i + 1] = FLOAT_MUL32(W, MAC16_16(MULT16_16(-X[i + 1], Y[i]), X[i], Y[i + 1]));
    }
    W = FLOAT_AMULT(p, w[j]);
    prod[i] = FLOAT_MUL32(W, MULT16_16(X[i], Y[i]));
}
#endif

#ifdef OVERRIDE_MDF_ADJUST_PROP
static void mdf_adjust_prop(const spx_word32_t * W, int N, int M, int P, spx_word16_t * prop)
{
    int             i, j, p;
    spx_word16_t    max_sum = 1;
    spx_word32_t    prop_sum = 1;
    for (i = 0; i < M; i++) {
        spx_word32_t    tmp = 1;
        for (p = 0; p < P; p++)
            for (j = 0; j < N; j++)
                tmp += MULT16_16(EXTRACT16(SHR32(W[p * N * M + i * N + j], 18)), EXTRACT16(SHR32(W[p * N * M + i * N + j], 18)));
#ifdef FIXED_POINT
        /* Just a security in case an overflow were to occur */
        tmp = MIN32(ABS32(tmp), 536870912);
#endif
        prop[i] = spx_sqrt(tmp);
        if (prop[i] > max_sum)
            max_sum = prop[i];
    }
    for (i = 0; i < M; i++) {
        prop[i] += MULT16_16_Q15(QCONST16(.1f, 15), max_sum);
        prop_sum += EXTEND32(prop[i]);
    }
    for (i = 0; i < M; i++) {
        prop[i] = DIV32(MULT16_16(QCONST16(.99f, 15), prop[i]), prop_sum);
        /*printf ("%f ", prop[i]); */
    }
    /*printf ("\n"); */
}

#endif

#ifdef OVERRIDE_MDF_PREEMPH_FLT
static int mdf_preemph(spx_word16_t * in, spx_word16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem)
{
    int             i;
    int             saturated = 0;

    for (i = 0; i < len; i++) {
        spx_word32_t    tmp32;
        /* FIXME: This core has changed a bit, need to merge properly */
        tmp32 = SUB32(EXTEND32(in[i]), EXTEND32(MULT16_16_P15(preemph, *mem)));
#ifdef FIXED_POINT
        if (tmp32 > 32767) {
            tmp32 = 32767;
            if (saturated == 0)
                saturated = 1;
        }
        if (tmp32 < -32767) {
            tmp32 = -32767;
            if (saturated == 0)
                saturated = 1;
        }
#endif
        *mem = in[i];
        out[i] = EXTRACT16(tmp32);
    }
    return saturated;
}
#endif


#ifdef OVERRIDE_MDF_STRIDED_PREEMPH_FLT
static int mdf_preemph_with_stride_int(const spx_int16_t * in, spx_word16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem, int stride)
{
    int             i;
    int             saturated = 0;

    for (i = 0; i < len; i++) {
        spx_word32_t    tmp32;
        tmp32 = SUB32(EXTEND32(in[i * stride]), EXTEND32(MULT16_16_P15(preemph, *mem)));
#ifdef FIXED_POINT
        /*FIXME: If saturation occurs here, we need to freeze adaptation for M frames (not just one) */
        if (tmp32 > 32767) {
            tmp32 = 32767;
            saturated = 1;
        }
        if (tmp32 < -32767) {
            tmp32 = -32767;
            saturated = 1;
        }
#endif
        out[i] = EXTRACT16(tmp32);
        *mem = in[i * stride];
    }
    return saturated;
}
#endif


#ifdef OVERRIDE_MDF_VEC_SUB
static void vect_sub(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = SUB16(pSrcA[i], pSrcB[i]);
}
#endif


#ifdef OVERRIDE_MDF_VEC_SUB16
static void vect_sub16(const spx_int16_t * pSrcA, const spx_int16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = (pSrcA[i] - pSrcB[i]);
}
#endif


#ifdef OVERRIDE_MDF_VEC_ADD
static void vect_add(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = pSrcA[i] + pSrcB[i];
}
#endif


#ifdef OVERRIDE_MDF_VEC_MULT
/* vector mult for windowing */
static void vect_mult(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = MULT16_16_Q15(pSrcA[i], pSrcB[i]);
}
#endif

#ifdef OVERRIDE_MDF_VEC_SCALE
static void vect_scale(const spx_word16_t * pSrc, spx_word16_t scale, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = (spx_int32_t) MULT16_32_Q15(scale, pSrc[i]);
}
#endif

#ifdef OVERRIDE_MDF_VEC_CLEAR
static void vect_clear(spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = 0.0f;
}
#endif

#ifdef OVERRIDE_MDF_VEC_COPY
static void vect_copy(void *dst, const void *src, uint32_t blockSize)
{
    th_memcpy(dst, src, blockSize);
}
#endif


#ifdef OVERRIDE_MDF_SMOOTHED_ADD
static void smoothed_add(const spx_word16_t * pSrc1, const spx_word16_t * pWin1,
                         const spx_word16_t * pSrc2, const spx_word16_t * pWin2, spx_word16_t * pDst, uint16_t frame_size, uint16_t nbChan, uint16_t N)
{
    int             chan;
    int             i;

    for (chan = 0; chan < nbChan; chan++)
        for (i = 0; i < frame_size; i++)
            pDst[chan * N + i] = MULT16_16_Q15(pWin1[i], pSrc1[chan * N + i])
                + MULT16_16_Q15(pWin2[i], pSrc2[chan * N + i]);

}
#endif


#ifdef OVERRIDE_MDF_DEEMPH
static int mdf_deemph(const spx_int16_t * micin, spx_word16_t * input,
                      spx_word16_t * e, spx_int16_t * out, spx_word16_t preemph, int frame_size, spx_word16_t * mem, int stride)
{
    int32_t         saturated = 0;
    int             i;

    for (i = 0; i < frame_size; i++) {
        spx_word32_t    tmp_out;
#ifdef TWO_PATH
        tmp_out = SUB32(EXTEND32(input[i]), EXTEND32(e[i]));
#else
        tmp_out = SUB32(EXTEND32(input[i]), EXTEND32(y[i]));
#endif
        tmp_out = ADD32(tmp_out, EXTEND32(MULT16_16_P15(preemph, *mem)));
        /* This is an arbitrary test for saturation in the microphone signal */
        if (micin[i * stride] <= -32000 || micin[i * stride] >= 32000) {
            if (saturated == 0)
                saturated = 1;
        }
        out[i * stride] = WORD2INT(tmp_out);
        *mem = tmp_out;
    }
    return saturated;
}
#endif

#ifdef OVERRIDE_MDF_SMOOTH_FE_NRG
static void smooth_fe_nrg(spx_word32_t * in1, spx_word16_t c1, spx_word32_t * in2, spx_word16_t c2, spx_word32_t * pDst, uint16_t frame_size)
{
    int             j;

    for (j = 0; j <= frame_size; j++)
        pDst[j] = MULT16_32_Q15(c1, in1[j]) + 1 + MULT16_32_Q15(c2, in2[j]);
}
#endif

#ifdef OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR
static void filtered_spectra_cross_corr(spx_word32_t * pRf, spx_word32_t * pEh, spx_word32_t * pYf, spx_word32_t * pYh,
                                        spx_float_t * Pey, spx_float_t * Pyy, spx_word16_t spec_average, uint16_t frame_size)
{
    int             j;

    for (j = frame_size; j >= 0; j--) {
        spx_float_t     Eh, Yh;
        Eh = PSEUDOFLOAT(pRf[j] - pEh[j]);
        Yh = PSEUDOFLOAT(pYf[j] - pYh[j]);
        *Pey = FLOAT_ADD(*Pey, FLOAT_MULT(Eh, Yh));
        *Pyy = FLOAT_ADD(*Pyy, FLOAT_MULT(Yh, Yh));
#ifdef FIXED_POINT
        pEh[j] = MAC16_32_Q15(MULT16_32_Q15(SUB16(32767, spec_average), pEh[j]), spec_average, pRf[j]);
        pYh[j] = MAC16_32_Q15(MULT16_32_Q15(SUB16(32767, spec_average), pYh[j]), spec_average, pYf[j]);
#else
        pEh[j] = (1 - spec_average) * pEh[j] + spec_average * pRf[j];
        pYh[j] = (1 - spec_average) * pYh[j] + spec_average * pYf[j];
#endif
    }
}
#endif


#ifdef OVERRIDE_MDF_NORM_LEARN_RATE_CALC
static void mdf_nominal_learning_rate_calc(spx_word32_t * pRf, spx_word32_t * power,
                                           spx_word32_t * pYf, spx_float_t * power_1, spx_word16_t leak_estimate, spx_word16_t RER, uint16_t frame_size)
{
    int             i;

    for (i = 0; i < frame_size; i++) {
        spx_word32_t    r, e;
        /* Compute frequency-domain adaptation mask */
        r = MULT16_32_Q15(leak_estimate, SHL32(pYf[i], 3));
        e = SHL32(pRf[i], 3) + 1;
#ifdef FIXED_POINT
        if (r > SHR32(e, 1))
            r = SHR32(e, 1);
#else
        if (r > .5 * e)
            r = .5 * e;
#endif
        r = MULT16_32_Q15(QCONST16(.7, 15), r) + MULT16_32_Q15(QCONST16(.3, 15), (spx_word32_t) (MULT16_32_Q15(RER, e)));
        /*st->power_1[i] = adapt_rate*r/(e*(1+st->power[i])); */
        power_1[i] = FLOAT_SHL(FLOAT_DIV32_FLOAT(r, FLOAT_MUL32U(e, power[i] + 10)), WEIGHT_SHIFT + 16);
    }
}

#endif

#ifdef OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC
static void mdf_non_adapt_learning_rate_calc(spx_word32_t * power, spx_float_t * power_1, spx_word16_t adapt_rate, uint16_t frame_size)
{
    int             i;

    for (i = 0; i < frame_size; i++)
        power_1[i] = FLOAT_SHL(FLOAT_DIV32(EXTEND32(adapt_rate), ADD32(power[i], 10)), WEIGHT_SHIFT + 1);
}

#endif


/*
 * ARM with Helium optimized routines
 * (floating-point only)
 */

#elif defined (__ARM_FEATURE_MVE) && defined(FLOATING_POINT) && defined(USE_CMSIS_DSP)

#include <arm_mve.h>
#include <arm_math.h>
#include <arm_math_f16.h>
#include <arm_helium_utils.h>
#include <arm_vec_math.h>
#include <arm_vec_math_f16.h>


#define VISIB_ATTR static
//#define VISIB_ATTR __attribute__ ((noinline))


#ifdef OVERRIDE_MDF_DC_NOTCH

VISIB_ATTR void filter_dc_notch16(const spx_int16_t * in, spx_word16_t radius, spx_word16_t * out, int len, spx_mem_t * mem, int stride)
{
    int             i;
    spx_word16_t    den2;
    spx_word16_t    radius2 = radius * 2;
    uint32x4_t      idx = vmulq_n_u32(vidupq_n_u32(0, 1), stride);
    float32_t       mem1 = mem[1];
    float32_t       mem0 = mem[0];

    den2 = radius * radius + .7f * (1.0f - radius) * (1.0f - radius);

    for (i = 0; i < len / 4; i++) {
        spx_word32_t    vout;
        float32x4_t     vinV = vcvtq_f32_s32(vldrhq_gather_shifted_offset_s32(in, idx));
        float32x4_t     vinV2 = vmulq(vinV, -2.0f);
        float32x4_t     voutF;

        /* increment gather load indexes */
        idx = vaddq_n_s32(idx, stride * 4);

        for (int j = 0; j < 4; j++) {
            vout = mem0 + vinV[j];
            voutF[j] = vout;

            mem0 = mem1 + vinV2[j] + radius2 * vout;
            mem1 = vinV[j] - den2 * vout;
        }

        vstrwq_f32(out, vmulq(voutF, radius));
        out += 4;
    }

    mem[1] = mem1;
    mem[0] = mem0;
}

#endif

#ifdef OVERRIDE_MDF_INNER_PROD
/* autovectorized */
VISIB_ATTR spx_word32_t mdf_inner_prod(const spx_word16_t * x, const spx_word16_t * y, int len)
{
    spx_word32_t    sum;

    if (x == y)
        arm_power_f32(x, len, &sum);
    else
        arm_dot_prod_f32(x, y, len, &sum);
    return sum;
}
#endif

#ifdef OVERRIDE_MDF_POWER_SPECTRUM
/* autovectorized */
VISIB_ATTR void power_spectrum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    ps[0] = MULT16_16(X[0], X[0]);
    arm_cmplx_mag_squared_f32(&X[1], ps + 1, N - 1);
}
#endif


/** Compute power spectrum of a half-complex (packed) vector and accumulate */
#ifdef OVERRIDE_MDF_POWER_SPECTRUM_ACCUM
/* autovectorized */
static void arm_cmplx_mag_squared_accum_f32(const float32_t * pSrc, float32_t * pDst, int32_t blockSize)
{
    f32x4x2_t       vecSrc;
    f32x4_t         sum;
    do {
        mve_pred16_t    tpred = vctp32q(blockSize);

        vecSrc = vld2q(pSrc);
        sum = vld1q_z(pDst, tpred);
        sum = vfmaq_m(sum, vecSrc.val[0], vecSrc.val[0], tpred);
        sum = vfmaq_m(sum, vecSrc.val[1], vecSrc.val[1], tpred);
        vst1q_p(pDst, sum, tpred);

        pSrc += 8;
        pDst += 4;
        blockSize -= 4;
    } while (blockSize > 0);
}

VISIB_ATTR void power_spectrum_accum(const spx_word16_t * X, spx_word32_t * ps, int N)
{
    ps[0] += MULT16_16(X[0], X[0]);
    ps[N / 2] += MULT16_16(X[N - 1], X[N - 1]);

    arm_cmplx_mag_squared_accum_f32(X + 1, ps + 1, N / 2 - 1);

}
#endif

#ifdef OVERRIDE_MDF_SPECTRAL_MUL_ACCUM
VISIB_ATTR void spectral_mul_accum(const spx_word16_t * X, const spx_word32_t * Y, spx_word16_t * acc, int N, int M)
{
    int             i, j;

    /* non accumulated loop */
    {
        acc[0] = X[0] * Y[0];
        acc[N - 1] = X[N - 1] * Y[N - 1];

        float32_t      *pAcc = acc + 1;
        const float32_t *pX = X + 1;
        const float32_t *pY = Y + 1;
        int32_t         blockSize = (N - 2);
        do {
            mve_pred16_t    tpred = vctp32q(blockSize);

            float32x4_t     vecAcc;
            float32x4_t     vecX = vld1q_z(pX, tpred);
            float32x4_t     vecY = vld1q_z(pY, tpred);

            vecAcc = vcmulq_m(vuninitializedq(vecAcc), vecX, vecY, tpred);
            vecAcc = vcmlaq_rot90_m(vecAcc, vecX, vecY, tpred);

            vst1q_p(pAcc, vecAcc, tpred);
            /*
             * Decrement the blockSize loop counter
             * Advance vector source and destination pointers
             */
            pX += 4;
            pY += 4;
            pAcc += 4;
            blockSize -= 4;
        }
        while (blockSize > 0);

        X += N;
        Y += N;
    }

    for (j = 1; j < M; j++) {
        acc[0] += X[0] * Y[0];
        acc[N - 1] += X[N - 1] * Y[N - 1];

        float32_t      *pAcc = acc + 1;
        const float32_t *pX = X + 1;
        const float32_t *pY = Y + 1;
        int32_t         blockSize = (N - 2);
        do {
            mve_pred16_t    tpred = vctp32q(blockSize);

            float32x4_t     vecAcc = vld1q_z(pAcc, tpred);
            float32x4_t     vecX = vld1q_z(pX, tpred);
            float32x4_t     vecY = vld1q_z(pY, tpred);

            vecAcc = vcmlaq_m(vecAcc, vecX, vecY, tpred);
            vecAcc = vcmlaq_rot90_m(vecAcc, vecX, vecY, tpred);

            vst1q_p(pAcc, vecAcc, tpred);
            /*
             * Decrement the blockSize loop counter
             * Advance vector source and destination pointers
             */
            pX += 4;
            pY += 4;
            pAcc += 4;
            blockSize -= 4;
        }
        while (blockSize > 0);

        X += N;
        Y += N;
    }

}

#endif


#ifdef OVERRIDE_MDF_WEIGHT_SPECT_MUL_CONJ
VISIB_ATTR void weighted_spectral_mul_conj(const spx_float_t * w, const spx_float_t p, const spx_word16_t * X, const spx_word16_t * Y, spx_word32_t * prod, int N)
{


    spx_float_t     W;
    uint32x4_t      str = { 0, 0, 1, 1 };
    int32_t         blockSize = N - 2;

    W = FLOAT_AMULT(p, w[0]);
    prod[0] = FLOAT_MUL32(W, MULT16_16(X[0], Y[0]));

    W = FLOAT_AMULT(p, w[N / 2]);
    prod[N - 1] = FLOAT_MUL32(W, MULT16_16(X[N - 1], Y[N - 1]));


    X += 1;
    Y += 1;
    w += 1;
    prod += 1;

    do {
        mve_pred16_t    tpred = vctp32q(blockSize);

        float32x4_t     vecW = vldrwq_gather_shifted_offset_z_f32(w, str, tpred);
        str = vaddq_x(str, 2, tpred);
        vecW = vmulq_x(vecW, p, tpred);

        float32x4_t     vecX = vld1q_z(X, tpred);
        float32x4_t     vecY = vld1q_z(Y, tpred);
        float32x4_t     res;
        res = vcmlaq_rot270_m(vcmulq_x(vecX, vecY, tpred), vecX, vecY, tpred);
        res = vmulq_x(vecW, res, tpred);

        vst1q_p(prod, res, tpred);
        prod += 4;
        X += 4;
        Y += 4;
        blockSize -= 4;
    }
    while (blockSize > 0);
}
#endif

#ifdef OVERRIDE_MDF_ADJUST_PROP
VISIB_ATTR void mdf_adjust_prop(const spx_word32_t * W, int N, int M, int P, spx_word16_t * prop)
{
    int             i, p;
    float32_t       max_sum = 1.0f;
    float32_t       prop_sum = 1.0f;
    int32_t         cnt;

    float32x4_t     acc;

    if (M <= 4) {
        float32x4_t     vprop = vdupq_n_f32(0.0f);
        mve_pred16_t    tpred = vctp32q(M);
        for (i = 0; i < M; i++) {
            acc = vdupq_n_f32(0.f);
            for (p = 0; p < P; p++) {
                cnt = N;
                const spx_word32_t *pW = &W[p * N * M + i * N];
                while (cnt > 0) {
                    mve_pred16_t    tpred = vctp32q(cnt);
                    float32x4_t     vecW = vld1q_z(pW, tpred);
                    acc = vfmaq_m(acc, vecW, vecW, tpred);
                    pW += 4;
                    cnt -= 4;
                };
            }
            vprop[i] = 1.0f + acc[0] + acc[1] + acc[2] + acc[3];
        }

        /* vector float32_t square root using newton method */
        q31x4_t         newtonStartVec;
        f32x4_t         sumHalf, invSqrt;

        newtonStartVec = vdupq_n_s32(INVSQRT_MAGIC_F32) - vshrq((q31x4_t) vprop, 1);
        sumHalf = vprop * 0.5f;
        /* compute 2 newton x iterations */
        INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, (f32x4_t) newtonStartVec);
        INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, invSqrt);
        /* sqrt(x) = x * invSqrt(x)  */
        vprop = vmulq(vprop, invSqrt);

        max_sum = vmaxnmvq(max_sum, vprop);

        vprop = vaddq(vprop, MULT16_16_Q15(QCONST16(.1f, 15), max_sum));
        prop_sum += vprop[0] + vprop[1] + vprop[2] + vprop[3];

        vprop = vmulq(vprop, (QCONST16(.99f, 15) / prop_sum));
        vst1q_p(prop, vprop, tpred);

    } else {
        spx_word32_t    tmp[M];
        for (i = 0; i < M; i++) {
            acc = vdupq_n_f32(0);
            for (p = 0; p < P; p++) {
                cnt = N;
                const spx_word32_t *pW = &W[p * N * M + i * N];
                while (cnt > 0) {
                    mve_pred16_t    tpred = vctp32q(cnt);
                    float32x4_t     vecW = vld1q_z(pW, tpred);
                    acc = vfmaq_m(acc, vecW, vecW, tpred);
                    pW += 4;
                    cnt -= 4;
                };
            }
            tmp[i] = 1.0f + acc[0] + acc[1] + acc[2] + acc[3];
        }

        /* Just a security in case an overflow were to occur */
        spx_word32_t   *pTmp = tmp;


        cnt = M;
        spx_word16_t   *pprop = prop;
        while (cnt > 0) {
            mve_pred16_t    tpred = vctp32q(cnt);
            float32x4_t     vecT = vld1q_z(pTmp, tpred);

            /* vector float32_t square root using newton method */
            q31x4_t         newtonStartVec;
            f32x4_t         sumHalf, invSqrt;

            newtonStartVec = vdupq_n_s32(INVSQRT_MAGIC_F32) - vshrq((q31x4_t) vecT, 1);
            sumHalf = vecT * 0.5f;
            /*
             * compute 2 newton x iterations
             */
            INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, (f32x4_t) newtonStartVec);
            INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, invSqrt);

            /* sqrt(x) = x * invSqrt(x)  */
            vecT = vmulq(vecT, invSqrt);

            vst1q_p(pprop, vecT, tpred);
            max_sum = vmaxnmvq_p(max_sum, vecT, tpred);

            pprop += 4;
            vecT += 4;
            cnt -= 4;
        };

        for (i = 0; i < M; i++) {
            prop[i] += MULT16_16_Q15(QCONST16(.1f, 15), max_sum);
            prop_sum += EXTEND32(prop[i]);
        }
        for (i = 0; i < M; i++) {
            prop[i] = DIV32(MULT16_16(QCONST16(.99f, 15), prop[i]), prop_sum);
        }
    }
}



#endif

#ifdef OVERRIDE_MDF_PREEMPH_FLT

int32_t mdf_preemph(spx_word16_t * input, spx_word16_t * out, spx_word16_t preemph, int frame_size, spx_word16_t * mem)
{
    spx_word16_t    state = *mem;
    int32_t         saturated = 0;
    float32x4_t     stateVec = vld1q(input - 1);
    float32x4_t     vecIn;

    stateVec[0] = state;
    preemph = -preemph;

    do {
        mve_pred16_t    tpred = vctp32q(frame_size);
        float32x4_t     sum;

        vecIn = vld1q_z(input, tpred);
        sum = vfmaq_m(vecIn, stateVec, preemph, tpred);
        stateVec = vld1q_z(input + 3, tpred);
        vst1q_p(input, sum, tpred);
        input += 4;
        frame_size -= 4;
    } while (frame_size > 0);

    /* save state */
    *mem = vecIn[3];
    return saturated;
}
#endif

#ifdef OVERRIDE_MDF_STRIDED_PREEMPH_FLT
VISIB_ATTR int mdf_preemph_with_stride_int(const spx_int16_t * in, spx_word16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem, int stride)
{

    if (stride == 1) {
        spx_word16_t    state = (spx_word16_t) * mem;
        int32_t         saturated = 0;
        float32x4_t     vecIn;
        float32x4_t     stateVec;
        const spx_int16_t *inN = in + 3;

        stateVec = vcvtq_f32_s32(vldrhq_s32(in - 1));
        stateVec[0] = state;

        preemph = -preemph;
        /* save state */
        *mem = (spx_mem_t) in[(len - 1)];

        do {
            mve_pred16_t    tpred = vctp32q(len);
            float32x4_t     vout;

            vecIn = vcvtq_f32_s32(vldrhq_z_s32(in, tpred));
            vout = vfmaq_m(vecIn, stateVec, preemph, tpred);
            stateVec = vcvtq_f32_s32(vldrhq_z_s32(in + 3, tpred));

            vst1q_p(out, vout, tpred);

            in += 4;
            out += 4;
            len -= 4;
        } while (len > 0);

    } else {
        spx_word16_t    state = (spx_word16_t) * mem;
        uint32x4_t      idx = vmulq(vidupq_n_u32(0, 1), stride);
        float32x4_t     vecIn;
        float32x4_t     stateVec;
        const spx_int16_t *inN = in + 3 * stride;

        stateVec = vcvtq_f32_s32(vldrhq_gather_shifted_offset_s32(in - stride, idx));
        stateVec[0] = state;

        preemph = -preemph;
        /* save state */
        *mem = (spx_mem_t) in[stride * (len - 1)];

        do {
            mve_pred16_t    tpred = vctp32q(len);
            float32x4_t     vout;

            vecIn = vcvtq_f32_s32(vldrhq_gather_shifted_offset_z_s32(in, idx, tpred));
            vout = vfmaq_m(vecIn, stateVec, preemph, tpred);
            stateVec = vcvtq_f32_s32(vldrhq_gather_shifted_offset_z_s32(inN, idx, tpred));

            vst1q_p(out, vout, tpred);

            idx = vaddq(idx, (uint32_t) (stride * 4));
            out += 4;
            len -= 4;
        } while (len > 0);

    }
    return 0;
}

#endif

#ifdef OVERRIDE_MDF_VEC_SUB
VISIB_ATTR void vect_sub(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_sub_f32(pSrcA, pSrcB, pDst, blockSize);
}
#endif


#ifdef OVERRIDE_MDF_VEC_SUB16
/* subtract spx_int16_t inputs => spx_word16_t dest */
VISIB_ATTR void vect_sub16(const spx_int16_t * pSrcA, const spx_int16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    int             i;

    for (i = 0; i < blockSize; i++)
        pDst[i] = (pSrcA[i] - pSrcB[i]);
}
#endif

#ifdef OVERRIDE_MDF_VEC_ADD
VISIB_ATTR void vect_add(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_add_f32(pSrcA, pSrcB, pDst, blockSize);
}
#endif

#ifdef OVERRIDE_MDF_VEC_MULT
/* vector mult for windowing */
VISIB_ATTR void vect_mult(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_mult_f32(pSrcA, pSrcB, pDst, blockSize);
}
#endif


#ifdef OVERRIDE_MDF_VEC_SCALE
VISIB_ATTR void vect_scale(const spx_word16_t * pSrc, spx_word16_t scale, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_scale_f32(pSrc, scale, pDst, blockSize);
}
#endif


#ifdef OVERRIDE_MDF_VEC_CLEAR
VISIB_ATTR void vect_clear(spx_word16_t * pDst, uint32_t blockSize)
{
    arm_fill_f32(0.0f, pDst, blockSize);
}
#endif

#ifdef OVERRIDE_MDF_SMOOTHED_ADD
/* autovectorized */

void smoothed_add(const spx_word16_t * pSrc1, const spx_word16_t * pWin1,
                  const spx_word16_t * pSrc2, const spx_word16_t * pWin2, spx_word16_t * pDst, uint16_t frame_size, uint16_t nbChan, uint16_t N)
{

    for (int chan = 0; chan < nbChan; chan++) {
        int32_t         blkCnt = frame_size;    /* loop counters */
        const float32_t *_pSrc1 = pSrc1;
        const float32_t *_pWin1 = pWin1;
        const float32_t *_pSrc2 = pSrc2;
        const float32_t *_pWin2 = pWin2;
        float32_t      *_pDst = pDst;

        /* Compute 4 outputs at a time */
        while (blkCnt > 0) {
            mve_pred16_t    tpred = vctp32q(blkCnt);
            float32x4_t     vecSrc, vecWin, vecTmp;
            int16x8_t       vecDst;

            vecSrc = vld1q_z(_pSrc1, tpred);
            vecWin = vld1q_z(_pWin1, tpred);
            vecTmp = vmulq_m(vuninitializedq(vecSrc), vecSrc, vecWin, tpred);

            vecSrc = vld1q_z(_pSrc2, tpred);
            vecWin = vld1q_z(_pWin2, tpred);
            vecTmp = vfmaq_m(vecTmp, vecSrc, vecWin, tpred), vst1q_p(_pDst, vecTmp, tpred);
            blkCnt -= 4;
            _pSrc1 += 4;
            _pSrc2 += 4;
            _pWin1 += 4;
            _pWin2 += 4;
            _pDst += 4;
        }
        pSrc1 += N;
        pSrc2 += N;
        pDst += N;
    }
}
#endif



#ifdef OVERRIDE_MDF_DEEMPH

VISIB_ATTR int mdf_deemph(const spx_int16_t * micin, spx_word16_t * input,
                    spx_word16_t * e, spx_int16_t * out, spx_word16_t preemph, int len, spx_word16_t * mem, int stride)
{
    spx_word16_t    memL = *mem;
    int32_t         saturated = 0;
    uint32x4_t      offset = vmulq_n_u32(vidupq_n_u32(0, 1), stride);
    uint32x4_t      vMicMax = vdupq_n_s32(0);
    int16x8_t       vdst = vdupq_n_s16(0);

    /* Compute error signal (for the output with de-emphasis) */
    for (int i = 0; i < len >> 2; i++) {
        float32x4_t     vInput = vld1q(input);
        float32x4_t     vE = vld1q(e);
        float32x4_t     vTmpOut = vInput - vE;

        vTmpOut[0] = vTmpOut[0] + MULT16_16_P15(preemph, memL);
        memL = vTmpOut[0];

        vTmpOut[1] = vTmpOut[1] + MULT16_16_P15(preemph, memL);
        memL = vTmpOut[1];

        vTmpOut[2] = vTmpOut[2] + MULT16_16_P15(preemph, memL);
        memL = vTmpOut[2];

        vTmpOut[3] = vTmpOut[3] + MULT16_16_P15(preemph, memL);
        memL = vTmpOut[3];


        // narrow & saturate
        vdst = vqmovnbq_s32(vdst, vcvtaq_s32_f32(vTmpOut));
        vstrhq_scatter_shifted_offset_s32(out, offset, (int32x4_t) vdst);

        /* keeps track of microphone signal amplitude */
        vMicMax = vmaxaq_s32(vMicMax, vldrhq_gather_shifted_offset_s32(micin, offset));

        input += 4;
        e += 4;
        offset = offset + 4 * stride;
    }

    /* tail */
    out += (stride) * 4 * (len >> 2);
    for (int i = 0; i < (len & 3); i++) {
        spx_word32_t    tmp_out;

        tmp_out = SUB32(EXTEND32(input[i]), EXTEND32(e[i]));
        tmp_out = ADD32(tmp_out, EXTEND32(MULT16_16_P15(preemph, memL)));
        if (micin[i * stride] <= -32000 || micin[i * stride] >= 32000) {
            saturated = 1;
        }
        out[i * stride] = WORD2INT(tmp_out);
        memL = tmp_out;
    }
    /* This is an arbitrary test for saturation in the microphone signal */
    uint32_t        absmax = vmaxavq_s32(0UL, vMicMax);
    if (absmax >= 32000)
        saturated = 1;

    *mem = memL;

    return saturated;
}

#endif

#ifdef OVERRIDE_MDF_SMOOTH_FE_NRG
/* autovectorized */
VISIB_ATTR void smooth_fe_nrg(spx_word32_t * in1, spx_word16_t c1, spx_word32_t * in2, spx_word16_t c2, spx_word32_t * pDst, uint16_t frame_size)
{
    spx_word32_t   *pDst1 = pDst;
    int32_t         blkCnt = frame_size;

    /* Compute 4 outputs at a time */
    while (blkCnt > 0) {
        mve_pred16_t    tpred = vctp32q(blkCnt);
        float32x4_t     vecIn1, vecIn2;
        float32x4_t     vecDst = vdupq_n_f32(1.0f);

        vecIn1 = vld1q_z(in1, tpred);
        vecIn2 = vld1q_z(in2, tpred);

        vecDst = vfmaq_m(vfmaq_m(vecDst, vecIn1, c1, tpred), vecIn2, c2, tpred);
        vst1q_p(pDst, vecDst, tpred);
        blkCnt -= 4;
        in1 += 4;
        in2 += 4;
        pDst += 4;
    }
}
#endif

#ifdef OVERRIDE_MDF_FILTERED_SPEC_AD_XCORR

VISIB_ATTR void filtered_spectra_cross_corr(spx_word32_t * pRf, spx_word32_t * pEh, spx_word32_t * pYf, spx_word32_t * pYh,
                                      spx_float_t * Pey, spx_float_t * Pyy, spx_word16_t spec_average, uint16_t frame_size)
{
    spx_word32_t   *pEh1 = pEh;
    spx_word32_t   *pYh1 = pYh;
    int             blockSize = frame_size;
    float32x4_t     vsumPey = vdupq_n_f32(0);
    float32x4_t     vsumPyy = vdupq_n_f32(0);;
    float32_t       spec_avg_comp_32 = (1.0f - spec_average);

    do {
        mve_pred16_t    tpred = vctp32q(blockSize);

        float32x4_t     vecRf = vld1q_z(pRf, tpred);
        float32x4_t     vecEh = vld1q_z(pEh, tpred);
        float32x4_t     vecYf = vld1q_z(pYf, tpred);
        float32x4_t     vecYh = vld1q_z(pYh, tpred);

        float32x4_t     vEhD = vsubq_x(vecRf, vecEh, tpred);
        float32x4_t     vYhD = vsubq_x(vecYf, vecYh, tpred);

        vsumPey = vfmaq_m(vsumPey, vEhD, vYhD, tpred);
        vsumPyy = vfmaq_m(vsumPyy, vYhD, vYhD, tpred);
        vst1q_p(pEh, vfmaq_m(vmulq_m(vuninitializedq(vecEh), vecEh, spec_avg_comp_32, tpred), vecRf, spec_average, tpred), tpred);
        vst1q_p(pYh, vfmaq_m(vmulq_m(vuninitializedq(vecYh), vecYh, spec_avg_comp_32, tpred), vecYf, spec_average, tpred), tpred);
        pRf += 4;
        pEh += 4;
        pYf += 4;
        pYh += 4;
        blockSize -= 4;
    }
    while (blockSize > 0);

    *Pey += vecAddAcrossF32Mve(vsumPey);        //[0] + vsumPey[1] + vsumPey[2] + vsumPey[3];
    *Pyy += vecAddAcrossF32Mve(vsumPyy);        //[0] + vsumPey[1] + vsumPey[2] + vsumPey[3];

}

#endif



#ifdef OVERRIDE_MDF_NORM_LEARN_RATE_CALC


VISIB_ATTR void mdf_nominal_learning_rate_calc(spx_word32_t * pRf, spx_word32_t * power,
                                         spx_word32_t * pYf, spx_float_t * power_1, spx_word16_t leak_estimate, spx_word16_t RER, uint16_t len)
{
    int             blockSize = len >> 2;
    float32_t       cst_0_7 = QCONST16(.7, 15);
    float32_t       cst_0_3 = QCONST16(.3, 15);

    do {
        float32x4_t     vecYf = vld1q(pYf);
        float32x4_t     vecRf = vld1q(pRf);

        /* Compute frequency-domain adaptation mask */
        float32x4_t     vecR = vmulq(vecYf, leak_estimate);
        float32x4_t     vecE = vaddq(vecRf, 1.0f);
        float32x4_t     vecEh = vmulq(vecE, 0.5f);

        /* if (r > .5 * e)  r = .5 * e; */
        vecR = vpselq(vecEh, vecR, vcmpgtq(vecR, vecEh));

        vecR = vfmaq(vmulq(vecR, cst_0_7), vmulq(vecE, RER), cst_0_3);

        /*st->power_1[i] = adapt_rate*r/(e*(1+st->power[i])); */
        float32x4_t     vecPwr = vld1q(power);
        vecPwr = vmulq(vecE, vaddq(vecPwr, 10.f));

        vst1q(power_1, vdiv_f32(vecR, vecPwr));

        pRf += 4;
        pYf += 4;
        power += 4;
        blockSize -= 1;
        power_1 += 4;
    }
    while (blockSize > 0);

    /* tail */
    for (int i = 0; i <= (len & 3); i++) {
        spx_word32_t    r, e;
        /* Compute frequency-domain adaptation mask */
        r = MULT16_32_Q15(leak_estimate, SHL32(*pYf++, 3));
        e = SHL32(*pRf++, 3) + 1;

        if (r > .5 * e)
            r = .5 * e;

        r = MULT16_32_Q15(QCONST16(.7, 15), r) + MULT16_32_Q15(QCONST16(.3, 15), (spx_word32_t) (MULT16_32_Q15(RER, e)));
        /*st->power_1[i] = adapt_rate*r/(e*(1+st->power[i])); */
        *power_1++ = FLOAT_SHL(FLOAT_DIV32_FLOAT(r, FLOAT_MUL32U(e, *power++ + 10)), WEIGHT_SHIFT + 16);
    }
}

#endif


#ifdef OVERRIDE_MDF_CONVERG_LEARN_RATE_CALC

VISIB_ATTR void mdf_non_adapt_learning_rate_calc(spx_word32_t * power, spx_float_t * power_1, spx_word16_t adapt_rate, uint16_t len)
{
    int32_t         blockSize = len >> 2;
    float32x4_t     vadapt_rate = vdupq_n_f32(adapt_rate);

    do {
        float32x4_t     vecPwr = vld1q(power);
        vecPwr = vaddq(vecPwr, 10.0f);

        vst1q(power_1, vdiv_f32(vadapt_rate, vecPwr));

        power += 4;
        blockSize -= 1;
        power_1 += 4;
    }
    while (blockSize > 0);

    /* tail */
    for (int i = 0; i <= (len & 3); i++) {
        *power_1++ = FLOAT_SHL(FLOAT_DIV32(EXTEND32(adapt_rate), ADD32(*power++, 10)), WEIGHT_SHIFT + 1);
    }
}

#endif

#endif
