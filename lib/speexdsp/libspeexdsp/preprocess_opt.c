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

/*
 * Reference code for optimized routines
 */

#ifdef OVERRIDE_ANR_VEC_MUL
/* vector mult for windowing */
static void vect_mult(
  const spx_word16_t * pSrcA,
  const spx_word16_t * pSrcB,
        spx_word16_t * pDst,
        uint32_t blockSize)
{
    int i;

    for (i=0;i<blockSize;i++)
      pDst[i] = MULT16_16_Q15(pSrcA[i], pSrcB[i]);
}
#endif


#ifdef OVERRIDE_ANR_OLA
/* vector overlap and add */
static void vect_ola(
  const spx_word16_t * pSrcA,
  const spx_word16_t * pSrcB,
        spx_int16_t  * pDst,
        uint32_t blockSize)
{
    int i;

   for (i=0;i<blockSize;i++)
      pDst[i] = WORD2INT(ADD32(EXTEND32(pSrcA[i]), EXTEND32(pSrcB[i])));
}
#endif




#ifdef OVERRIDE_ANR_COMPUTE_GAIN_FLOOR
static void compute_gain_floor(int noise_suppress, int effective_echo_suppress,
                        spx_word32_t * noise, spx_word32_t * echo, spx_word16_t * gain_floor, int len)
{
    int             i;
    float           echo_floor;
    float           noise_floor;

    noise_floor = exp(.2302585f * noise_suppress);
    echo_floor = exp(.2302585f * effective_echo_suppress);

    /* Compute the gain floor based on different floors for the background noise and residual echo */
    for (i = 0; i < len; i++)
        gain_floor[i] =
            FRAC_SCALING * sqrt(noise_floor * PSHR32(noise[i], NOISE_SHIFT) +
                                echo_floor * echo[i]) / sqrt(1 + PSHR32(noise[i], NOISE_SHIFT) + echo[i]);
}

#endif



#ifdef OVERRIDE_ANR_POWER_SPECTRUM
static void power_spectrum(spx_word16_t * ft, spx_word32_t * ps, int N
#if defined(FIXED_POINT)
                           , int frame_shift
#endif
    )
{
    int             i;

    ps[0] = MULT16_16(ft[0], ft[0]);
    for (i = 1; i < N; i++)
        ps[i] = MULT16_16(ft[2 * i - 1], ft[2 * i - 1]) + MULT16_16(ft[2 * i], ft[2 * i]);

    for (i = 0; i < N; i++)
        ps[i] = PSHR32(ps[i], frame_shift);
}
#endif


#ifdef OVERRIDE_ANR_UPDATE_NOISE_ESTIMATE
static void update_noise_estimate(SpeexPreprocessState * st, spx_word16_t beta, spx_word16_t beta_1)
{
    int             N = st->ps_size;
    int             i;

    for (i = 0; i < N; i++) {
        if (!st->update_prob[i] || st->ps[i] < PSHR32(st->noise[i], NOISE_SHIFT))
            st->noise[i] =
                MAX32(EXTEND32(0),
                      MULT16_32_Q15(beta_1, st->noise[i]) + MULT16_32_Q15(beta, SHL32(st->ps[i], NOISE_SHIFT)));
    }
}

#endif

#ifdef OVERRIDE_ANR_APOSTERIORI_SNR
static void aposteriori_snr(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;
    int             i;

    for (i = 0; i < N + M; i++) {
        spx_word16_t    gamma;

        /* Total noise estimate including residual echo and reverberation */
        spx_word32_t    tot_noise =
            ADD32(ADD32(ADD32(EXTEND32(1), PSHR32(st->noise[i], NOISE_SHIFT)), st->echo_noise[i]),
                  st->reverb_estimate[i]);

        /* A posteriori SNR = ps/noise - 1 */
        st->post[i] = SUB16(DIV32_16_Q8(ps[i], tot_noise), QCONST16(1.f, SNR_SHIFT));
        st->post[i] = MIN16(st->post[i], QCONST16(100.f, SNR_SHIFT));

        /* Computing update gamma = .1 + .9*(old/(old+noise))^2 */
        gamma =
            QCONST16(.1f, 15) + MULT16_16_Q15(QCONST16(.89f, 15),
                                              SQR16_Q15(DIV32_16_Q15(st->old_ps[i], ADD32(st->old_ps[i], tot_noise))));

        /* A priori SNR update = gamma*max(0,post) + (1-gamma)*old/noise */
        st->prior[i] =
            EXTRACT16(PSHR32
                      (ADD32
                       (MULT16_16(gamma, MAX16(0, st->post[i])),
                        MULT16_16(Q15_ONE - gamma, DIV32_16_Q8(st->old_ps[i], tot_noise))), 15));
        st->prior[i] = MIN16(st->prior[i], QCONST16(100.f, SNR_SHIFT));
    }
}
#endif


#ifdef OVERRIDE_ANR_UPDATE_ZETA
static void preprocess_update_zeta(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    int             i;


    st->zeta[0] =
        PSHR32(ADD32(MULT16_16(QCONST16(.7f, 15), st->zeta[0]), MULT16_16(QCONST16(.3f, 15), st->prior[0])), 15);
    for (i = 1; i < N - 1; i++)
        st->zeta[i] =
            PSHR32(ADD32
                   (ADD32
                    (ADD32
                     (MULT16_16(QCONST16(.7f, 15), st->zeta[i]),
                      MULT16_16(QCONST16(.15f, 15), st->prior[i])),
                     MULT16_16(QCONST16(.075f, 15), st->prior[i - 1])),
                    MULT16_16(QCONST16(.075f, 15), st->prior[i + 1])), 15);
    for (i = N - 1; i < N + M; i++)
        st->zeta[i] =
            PSHR32(ADD32(MULT16_16(QCONST16(.7f, 15), st->zeta[i]), MULT16_16(QCONST16(.3f, 15), st->prior[i])), 15);
}

#endif


#ifdef OVERRIDE_ANR_HYPERGEOM_GAIN
static inline spx_word32_t hypergeom_gain(spx_word32_t xx)
{
    int             ind;
    float           integer, frac;
    float           x;
    static const float table[21] = {
        0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
        1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
        2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f
    };
    x = EXPIN_SCALING_1 * xx;
    integer = floor(2 * x);
    ind = (int) integer;
    if (ind < 0)
        return FRAC_SCALING;
    if (ind > 19)
        return FRAC_SCALING * (1 + .1296 / x);
    frac = 2 * x - integer;
    return FRAC_SCALING * ((1 - frac) * table[ind] + frac * table[ind + 1]) / sqrt(x + .0001f);
}
#endif

#ifdef OVERRIDE_ANR_QCURVE
static inline spx_word16_t qcurve(spx_word16_t x)
{
    return 1.f / (1.f + .15f / (SNR_SCALING_1 * x));
}
#endif

#ifdef OVERRIDE_ANR_UPDATE_GAINS_CRITICAL_BANDS
static void update_gains_critical_bands(SpeexPreprocessState * st, spx_word16_t Pframe)
{
    int             i;
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;

    for (i = N; i < N + M; i++) {
        /* See EM and Cohen papers */
        spx_word32_t    theta;
        /* Gain from hypergeometric function */
        spx_word32_t    MM;
        /* Weiner filter gain */
        spx_word16_t    prior_ratio;
        /* a priority probability of speech presence based on Bark sub-band alone */
        spx_word16_t    P1;
        /* Speech absence a priori probability (considering sub-band and frame) */
        spx_word16_t    q;
#ifdef FIXED_POINT
        spx_word16_t    tmp;
#endif

        prior_ratio = PDIV32_16(SHL32(EXTEND32(st->prior[i]), 15), ADD16(st->prior[i], SHL32(1, SNR_SHIFT)));
        theta =
            MULT16_32_P15(prior_ratio,
                          QCONST32(1.f, EXPIN_SHIFT) + SHL32(EXTEND32(st->post[i]), EXPIN_SHIFT - SNR_SHIFT));

        MM = hypergeom_gain(theta);
        /* Gain with bound */
        st->gain[i] = EXTRACT16(MIN32(Q15_ONE, MULT16_32_Q15(prior_ratio, MM)));
        /* Save old Bark power spectrum */
        st->old_ps[i] =
            MULT16_32_P15(QCONST16(.2f, 15),
                          st->old_ps[i]) + MULT16_32_P15(MULT16_16_P15(QCONST16(.8f, 15),
                                                                       SQR16_Q15(st->gain[i])), ps[i]);

        P1 = QCONST16(.199f, 15) + MULT16_16_Q15(QCONST16(.8f, 15), qcurve(st->zeta[i]));
        q = Q15_ONE - MULT16_16_Q15(Pframe, P1);
#ifdef FIXED_POINT
        theta = MIN32(theta, EXTEND32(32767));
/*Q8*/ tmp =
            MULT16_16_Q15((SHL32(1, SNR_SHIFT) + st->prior[i]),
                          EXTRACT16(MIN32(Q15ONE, SHR32(spx_exp(-EXTRACT16(theta)), 1))));
        tmp = MIN16(QCONST16(3., SNR_SHIFT), tmp);      /* Prevent overflows in the next line */
/*Q8*/ tmp =
            EXTRACT16(PSHR32(MULT16_16(PDIV32_16(SHL32(EXTEND32(q), 8), (Q15_ONE - q)), tmp), 8));
        st->gain2[i] = DIV32_16(SHL32(EXTEND32(32767), SNR_SHIFT), ADD16(256, tmp));
#else
        st->gain2[i] = 1 / (1.f + (q / (1.f - q)) * (1 + st->prior[i]) * exp(-theta));
#endif
    }
}

#endif

#ifdef OVERRIDE_ANR_UPDATE_GAINS_LINEAR
static void update_gains_linear(SpeexPreprocessState * st, spx_word16_t Pframe)
{
    int             i;
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;

    for (i = 0; i < N; i++) {
        spx_word32_t    MM;
        spx_word32_t    theta;
        spx_word16_t    prior_ratio;
        spx_word16_t    tmp;
        spx_word16_t    p;
        spx_word16_t    g;

        /* Wiener filter gain */
        prior_ratio = PDIV32_16(SHL32(EXTEND32(st->prior[i]), 15), ADD16(st->prior[i], SHL32(1, SNR_SHIFT)));
        theta =
            MULT16_32_P15(prior_ratio,
                          QCONST32(1.f, EXPIN_SHIFT) + SHL32(EXTEND32(st->post[i]), EXPIN_SHIFT - SNR_SHIFT));

        /* Optimal estimator for loudness domain */
        MM = hypergeom_gain(theta);
        /* EM gain with bound */
        g = EXTRACT16(MIN32(Q15_ONE, MULT16_32_Q15(prior_ratio, MM)));
        /* Interpolated speech probability of presence */
        p = st->gain2[i];

        /* Constrain the gain to be close to the Bark scale gain */
        if (MULT16_16_Q15(QCONST16(.333f, 15), g) > st->gain[i])
            g = MULT16_16(3, st->gain[i]);
        st->gain[i] = g;

        /* Save old power spectrum */
        st->old_ps[i] =
            MULT16_32_P15(QCONST16(.2f, 15),
                          st->old_ps[i]) + MULT16_32_P15(MULT16_16_P15(QCONST16(.8f, 15),
                                                                       SQR16_Q15(st->gain[i])), ps[i]);

        /* Apply gain floor */
        if (st->gain[i] < st->gain_floor[i])
            st->gain[i] = st->gain_floor[i];

        /* Exponential decay model for reverberation (unused) */
        /*st->reverb_estimate[i] = st->reverb_decay*st->reverb_estimate[i] + st->reverb_decay*st->reverb_level*st->gain[i]*st->gain[i]*st->ps[i]; */

        /* Take into account speech probability of presence (loudness domain MMSE estimator) */
        /* gain2 = [p*sqrt(gain)+(1-p)*sqrt(gain _floor) ]^2 */
        tmp =
            MULT16_16_P15(p,
                          spx_sqrt(SHL32(EXTEND32(st->gain[i]), 15))) + MULT16_16_P15(SUB16(Q15_ONE, p),
                                                                                      spx_sqrt(SHL32
                                                                                               (EXTEND32
                                                                                                (st->gain_floor[i]),
                                                                                                15)));
        st->gain2[i] = SQR16_Q15(tmp);

        /* Use this if you want a log-domain MMSE estimator instead */
        /*st->gain2[i] = pow(st->gain[i], p) * pow(st->gain_floor[i],1.f-p); */
    }
}

#endif

#ifdef OVERRIDE_ANR_APPLY_SPEC_GAIN
static void apply_spectral_gain(SpeexPreprocessState * st)
{
    int             i;
    int             N = st->ps_size;

    for (i = 1; i < N; i++) {
        st->ft[2 * i - 1] = MULT16_16_P15(st->gain2[i], st->ft[2 * i - 1]);
        st->ft[2 * i] = MULT16_16_P15(st->gain2[i], st->ft[2 * i]);
    }
    st->ft[0] = MULT16_16_P15(st->gain2[0], st->ft[0]);
    st->ft[2 * N - 1] = MULT16_16_P15(st->gain2[N - 1], st->ft[2 * N - 1]);
}

#endif


/*
 * ARM with Helium optimized routines
 */

#elif defined (__ARM_FEATURE_MVE)

#include <arm_mve.h>
#include <arm_math.h>
#include <arm_math_f16.h>
#include <arm_helium_utils.h>
#include <arm_vec_math.h>
#include <arm_vec_math_f16.h>

__STATIC_FORCEINLINE f32x4_t visqrtf_f32(f32x4_t x)
{
    f32x4_t         b, c;
    any32x4_t       a;

    //fast invsqrt approx
    a.f = x;
    a.i = INVSQRT_MAGIC_F32 - (a.i >> 1);
    c = x * a.f;
    b = (3.0f - c * a.f) * 0.5f;
    a.f = a.f * b;
    c = x * a.f;
    b = (3.0f - c * a.f) * 0.5f;
    a.f = a.f * b;

    /*
     * set negative values to NAN
     */
    a.f = vdupq_m(a.f, NAN, vcmpltq(x, 0.0f));
    a.f = vdupq_m(a.f, INFINITY, vcmpeqq(x, 0.0f));
    return a.f;
}

__STATIC_FORCEINLINE f32x4_t vsqrtf_f32(f32x4_t vecIn)
{
    f32x4_t         vecDst;

    /* inverse square root unsing 2 newton iterations */
    vecDst = visqrtf_f32(vecIn);
    vecDst = vdupq_m(vecDst, 0.0f, vcmpeqq(vecIn, 0.0f));
    vecDst = vecDst * vecIn;
    return vecDst;
}



__STATIC_FORCEINLINE f16x8_t visqrtf_f16(f16x8_t vecIn)
{
    q15x8_t         vecNewtonInit = vdupq_n_s16(INVSQRT_MAGIC_F16);
    f16x8_t         vecOneHandHalf = vdupq_n_f16(1.5f16);
    f16x8_t         vecHalf;
    q15x8_t         vecTmpInt;
    f16x8_t         vecTmpFlt, vecTmpFlt1;


    vecHalf = vmulq(vecIn, (float16_t) 0.5f16);
    /*
     * cast input float vector to integer and right shift by 1
     */
    vecTmpInt = vshrq((q15x8_t) vecIn, 1);
    /*
     * INVSQRT_MAGIC - ((vec_q16_t)vecIn >> 1)
     */
    vecTmpInt = vsubq(vecNewtonInit, vecTmpInt);

    /*
     *------------------------------
     * 1st iteration
     *------------------------------
     * (1.5f-xhalf*x*x)
     */
    vecTmpFlt1 = vmulq((f16x8_t) vecTmpInt, (f16x8_t) vecTmpInt);
    vecTmpFlt1 = vmulq(vecTmpFlt1, vecHalf);
    vecTmpFlt1 = vsubq(vecOneHandHalf, vecTmpFlt1);
    /*
     * x = x*(1.5f-xhalf*x*x);
     */
    vecTmpFlt = vmulq((f16x8_t) vecTmpInt, vecTmpFlt1);

    /*
     *------------------------------
     * 2nd iteration
     *------------------------------
     */
    vecTmpFlt1 = vmulq(vecTmpFlt, vecTmpFlt);
    vecTmpFlt1 = vmulq(vecTmpFlt1, vecHalf);
    vecTmpFlt1 = vsubq(vecOneHandHalf, vecTmpFlt1);
    vecTmpFlt = vmulq(vecTmpFlt, vecTmpFlt1);

    /*
     * set negative values to nan
     */
    vecTmpFlt = vdupq_m(vecTmpFlt, (float16_t) NAN, vcmpltq(vecIn, (float16_t) 0.0f16));
    vecTmpFlt = vdupq_m(vecTmpFlt, (float16_t) INFINITY, vcmpeqq(vecIn, (float16_t) 0.0f16));

    return vecTmpFlt;
}

__STATIC_FORCEINLINE f16x8_t vsqrtf_f16(f16x8_t vecIn)
{
    f16x8_t         vecDst;

    /* inverse square root unsing 2 newton iterations */
    vecDst = visqrtf_f16(vecIn);
    vecDst = vdupq_m(vecDst, 0.0f, vcmpeqq(vecIn, 0.0f));
    vecDst = vecDst * vecIn;
    return vecDst;
}


#ifdef OVERRIDE_ANR_VEC_MUL
static void vect_mult(
  const spx_word16_t * pSrcA,
  const spx_word16_t * pSrcB,
        spx_word16_t * pDst,
        uint32_t blockSize)
{
    arm_mult_f32(pSrcA, pSrcB, pDst, blockSize);
}

#endif

#ifdef OVERRIDE_ANR_OLA
/* vector overlap and add */
static void vect_ola(
  const spx_word16_t * pSrcA,
  const spx_word16_t * pSrcB,
        spx_int16_t * pDst,
        uint32_t blockSize)
{
    int i;

   for (i=0;i<blockSize;i++)
      pDst[i] = WORD2INT(ADD32(EXTEND32(pSrcA[i]), EXTEND32(pSrcB[i])));
}
#endif


#ifdef OVERRIDE_ANR_COMPUTE_GAIN_FLOOR
static void compute_gain_floor(int noise_suppress, int effective_echo_suppress,
                        spx_word32_t * noise, spx_word32_t * echo, spx_word16_t * gain_floor, int len)
{
    int             i;
    float32_t       echo_floor;
    float32_t       noise_floor;

    noise_floor = expf(.2302585f * noise_suppress);
    echo_floor = expf(.2302585f * effective_echo_suppress);

    /* Compute the gain floor based on different floors for the background noise and residual echo */
    float32_t      *pnoise = (float32_t *) noise;
    float32_t      *pecho = (float32_t *) echo;
    float32_t      *pgain_floor = (float32_t *) gain_floor;

#if 1
    for (i = 0; i < len; i += 8) {
        float32x4x2_t   vnoise = vld2q(pnoise);
        float32x4x2_t   vecho = vld2q(pecho);

        float16x8_t     vnoise16 = vdupq_n_f16(0);
        vnoise16 = vcvtbq_f16_f32(vnoise16, vnoise.val[0]);
        vnoise16 = vcvttq_f16_f32(vnoise16, vnoise.val[1]);

        float16x8_t     vecho16 = vdupq_n_f16(0);
        vecho16 = vcvtbq_f16_f32(vecho16, vecho.val[0]);
        vecho16 = vcvttq_f16_f32(vecho16, vecho.val[1]);

        // gain_floor[i] =
        //    sqrt(noise_floor*PSHR32(noise[i],NOISE_SHIFT) + echo_floor*echo[i]/(1+PSHR32(noise[i],NOISE_SHIFT) + echo[i]));

        float16x8_t     vden = vaddq(vaddq_n_f16(vnoise16, 1.0f16), vecho16);
        float16x8_t     vnum = vmulq_n_f16(vnoise16, (float16_t) noise_floor);
        vnum = vfmaq_n_f16(vnum, vecho16, (float16_t) echo_floor);

        float16x8_t     vec_tmp16 = vdiv_f16(vnum, vden);
        vec_tmp16 = vsqrtf_f16(vec_tmp16);

        float32x4x2_t   vec_tmp32;
        vec_tmp32.val[0] = vcvtbq_f32_f16(vec_tmp16);
        vec_tmp32.val[1] = vcvttq_f32_f16(vec_tmp16);
        vst2q(pgain_floor, vec_tmp32);

        pnoise += 8;
        pecho += 8;
        pgain_floor += 8;
    }
#else
    for (i = 0; i < len; i += 4) {
        float32x4_t     vnoise = vld1q(pnoise);
        float32x4_t     vecho = vld1q(pecho);

        // gain_floor[i] =
        //    sqrt(noise_floor*PSHR32(noise[i],NOISE_SHIFT) + echo_floor*echo[i]/(1+PSHR32(noise[i],NOISE_SHIFT) + echo[i]));

        float32x4_t     vden = vaddq(vaddq_n_f32(vnoise, 1.0f), vecho);
        float32x4_t     vnum = vmulq_n_f32(vnoise, noise_floor);
        vnum = vfmaq_n_f32(vnum, vecho, echo_floor);

        float32x4_t     vec_tmp16 = vdiv_f32(vnum, vden);
        vec_tmp16 = vsqrtf_f32(vec_tmp16);

        vst1q(pgain_floor, vec_tmp16);

        pnoise += 4;
        pecho += 4;
        pgain_floor += 4;
    }
#endif
//printf("mve: ");
//    dump_buf(gain_floor, len, 32, "%.5f");
}

#endif




#ifdef OVERRIDE_ANR_POWER_SPECTRUM
static void power_spectrum(spx_word16_t * ft, spx_word32_t * ps, int N
#if defined(FIXED_POINT)
                           , int frame_shift
#endif
    )
{
    int             i;
#if defined(USE_CMSIS_DSP) && !defined(FIXED_POINT)
    ps[0] = MULT16_16(ft[0], ft[0]);
    arm_cmplx_mag_squared_f32(&ft[1], ps + 1, N - 1);
#else
    ps[0] = MULT16_16(ft[0], ft[0]);
    for (i = 1; i < N; i++)
        ps[i] = MULT16_16(ft[2 * i - 1], ft[2 * i - 1]) + MULT16_16(ft[2 * i], ft[2 * i]);

    for (i = 0; i < N; i++)
        ps[i] = PSHR32(ps[i], frame_shift);
#endif
}
#endif


#ifdef OVERRIDE_ANR_UPDATE_NOISE_ESTIMATE
static void update_noise_estimate(SpeexPreprocessState * st, spx_word16_t beta, spx_word16_t beta_1)
{
    int             N = st->ps_size;
    int             i;

    for (i = 0; i < N; i++) {
        if (!st->update_prob[i] || st->ps[i] < PSHR32(st->noise[i], NOISE_SHIFT))
            st->noise[i] =
                MAX32(EXTEND32(0),
                      MULT16_32_Q15(beta_1, st->noise[i]) + MULT16_32_Q15(beta, SHL32(st->ps[i], NOISE_SHIFT)));
    }
}

#endif

#ifdef OVERRIDE_ANR_APOSTERIORI_SNR
static void aposteriori_snr(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;
    int             i;

    for (i = 0; i < N + M; i++) {
        spx_word16_t    gamma;

        /* Total noise estimate including residual echo and reverberation */
        spx_word32_t    tot_noise =
            ADD32(ADD32(ADD32(EXTEND32(1), PSHR32(st->noise[i], NOISE_SHIFT)), st->echo_noise[i]),
                  st->reverb_estimate[i]);

        /* A posteriori SNR = ps/noise - 1 */
        st->post[i] = SUB16(DIV32_16_Q8(ps[i], tot_noise), QCONST16(1.f, SNR_SHIFT));
        st->post[i] = MIN16(st->post[i], QCONST16(100.f, SNR_SHIFT));

        /* Computing update gamma = .1 + .9*(old/(old+noise))^2 */
        gamma =
            QCONST16(.1f, 15) + MULT16_16_Q15(QCONST16(.89f, 15),
                                              SQR16_Q15(DIV32_16_Q15(st->old_ps[i], ADD32(st->old_ps[i], tot_noise))));

        /* A priori SNR update = gamma*max(0,post) + (1-gamma)*old/noise */
        st->prior[i] =
            EXTRACT16(PSHR32
                      (ADD32
                       (MULT16_16(gamma, MAX16(0, st->post[i])),
                        MULT16_16(Q15_ONE - gamma, DIV32_16_Q8(st->old_ps[i], tot_noise))), 15));
        st->prior[i] = MIN16(st->prior[i], QCONST16(100.f, SNR_SHIFT));
    }
}
#endif


#ifdef OVERRIDE_ANR_UPDATE_ZETA
static void preprocess_update_zeta(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    int             i;


    st->zeta[0] =
        PSHR32(ADD32(MULT16_16(QCONST16(.7f, 15), st->zeta[0]), MULT16_16(QCONST16(.3f, 15), st->prior[0])), 15);
    for (i = 1; i < N - 1; i++)
        st->zeta[i] =
            PSHR32(ADD32
                   (ADD32
                    (ADD32
                     (MULT16_16(QCONST16(.7f, 15), st->zeta[i]),
                      MULT16_16(QCONST16(.15f, 15), st->prior[i])), MULT16_16(QCONST16(.075f, 15),
                                                                              st->prior[i - 1])),
                    MULT16_16(QCONST16(.075f, 15), st->prior[i + 1])), 15);
    for (i = N - 1; i < N + M; i++)
        st->zeta[i] =
            PSHR32(ADD32(MULT16_16(QCONST16(.7f, 15), st->zeta[i]), MULT16_16(QCONST16(.3f, 15), st->prior[i])), 15);
}

#endif


#ifdef OVERRIDE_ANR_HYPERGEOM_GAIN
static inline spx_word32_t hypergeom_gain(spx_word32_t xx)
{
    int             ind;
    float           integer, frac;
    float           x;
    static const float table[21] = {
        0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
        1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
        2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f
    };
    x = EXPIN_SCALING_1 * xx;
    integer = floor(2 * x);
    ind = (int) integer;
    if (ind < 0)
        return FRAC_SCALING;
    if (ind > 19)
        return FRAC_SCALING * (1 + .1296 / x);
    frac = 2 * x - integer;
    return FRAC_SCALING * ((1 - frac) * table[ind] + frac * table[ind + 1]) / sqrt(x + .0001f);
}
#endif

#ifdef OVERRIDE_ANR_QCURVE
static inline spx_word16_t qcurve(spx_word16_t x)
{
    return 1.f / (1.f + .15f / (SNR_SCALING_1 * x));
}
#endif

#ifdef OVERRIDE_ANR_UPDATE_GAINS_CRITICAL_BANDS
static void update_gains_critical_bands(SpeexPreprocessState * st, spx_word16_t Pframe)
{
    int             i;
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;

    for (i = N; i < N + M; i++) {
        /* See EM and Cohen papers */
        spx_word32_t    theta;
        /* Gain from hypergeometric function */
        spx_word32_t    MM;
        /* Weiner filter gain */
        spx_word16_t    prior_ratio;
        /* a priority probability of speech presence based on Bark sub-band alone */
        spx_word16_t    P1;
        /* Speech absence a priori probability (considering sub-band and frame) */
        spx_word16_t    q;
#ifdef FIXED_POINT
        spx_word16_t    tmp;
#endif

        prior_ratio = PDIV32_16(SHL32(EXTEND32(st->prior[i]), 15), ADD16(st->prior[i], SHL32(1, SNR_SHIFT)));
        theta =
            MULT16_32_P15(prior_ratio,
                          QCONST32(1.f, EXPIN_SHIFT) + SHL32(EXTEND32(st->post[i]), EXPIN_SHIFT - SNR_SHIFT));

        MM = hypergeom_gain(theta);
        /* Gain with bound */
        st->gain[i] = EXTRACT16(MIN32(Q15_ONE, MULT16_32_Q15(prior_ratio, MM)));
        /* Save old Bark power spectrum */
        st->old_ps[i] =
            MULT16_32_P15(QCONST16(.2f, 15),
                          st->old_ps[i]) + MULT16_32_P15(MULT16_16_P15(QCONST16(.8f, 15),
                                                                       SQR16_Q15(st->gain[i])), ps[i]);

        P1 = QCONST16(.199f, 15) + MULT16_16_Q15(QCONST16(.8f, 15), qcurve(st->zeta[i]));
        q = Q15_ONE - MULT16_16_Q15(Pframe, P1);
#ifdef FIXED_POINT
        theta = MIN32(theta, EXTEND32(32767));
/*Q8*/ tmp =
            MULT16_16_Q15((SHL32(1, SNR_SHIFT) + st->prior[i]),
                          EXTRACT16(MIN32(Q15ONE, SHR32(spx_exp(-EXTRACT16(theta)), 1))));
        tmp = MIN16(QCONST16(3., SNR_SHIFT), tmp);      /* Prevent overflows in the next line */
/*Q8*/ tmp =
            EXTRACT16(PSHR32(MULT16_16(PDIV32_16(SHL32(EXTEND32(q), 8), (Q15_ONE - q)), tmp), 8));
        st->gain2[i] = DIV32_16(SHL32(EXTEND32(32767), SNR_SHIFT), ADD16(256, tmp));
#else
        st->gain2[i] = 1 / (1.f + (q / (1.f - q)) * (1 + st->prior[i]) * exp(-theta));
#endif
    }
}

#endif


#ifdef OVERRIDE_ANR_UPDATE_GAINS_LINEAR
static void update_gains_linear(SpeexPreprocessState * st, spx_word16_t Pframe)
{
    int             i;
    int             N = st->ps_size;
    int             M = st->nbands;
    spx_word32_t   *ps = st->ps;

    for (i = 0; i < N; i++) {
        spx_word32_t    MM;
        spx_word32_t    theta;
        spx_word16_t    prior_ratio;
        spx_word16_t    tmp;
        spx_word16_t    p;
        spx_word16_t    g;

        /* Wiener filter gain */
        prior_ratio = PDIV32_16(SHL32(EXTEND32(st->prior[i]), 15), ADD16(st->prior[i], SHL32(1, SNR_SHIFT)));
        theta =
            MULT16_32_P15(prior_ratio,
                          QCONST32(1.f, EXPIN_SHIFT) + SHL32(EXTEND32(st->post[i]), EXPIN_SHIFT - SNR_SHIFT));

        /* Optimal estimator for loudness domain */
        MM = hypergeom_gain(theta);
        /* EM gain with bound */
        g = EXTRACT16(MIN32(Q15_ONE, MULT16_32_Q15(prior_ratio, MM)));
        /* Interpolated speech probability of presence */
        p = st->gain2[i];

        /* Constrain the gain to be close to the Bark scale gain */
        if (MULT16_16_Q15(QCONST16(.333f, 15), g) > st->gain[i])
            g = MULT16_16(3, st->gain[i]);
        st->gain[i] = g;

        /* Save old power spectrum */
        st->old_ps[i] =
            MULT16_32_P15(QCONST16(.2f, 15),
                          st->old_ps[i]) +
            MULT16_32_P15(MULT16_16_P15(QCONST16(.8f, 15),
                             SQR16_Q15(st->gain[i])), ps[i]);

        /* Apply gain floor */
        if (st->gain[i] < st->gain_floor[i])
            st->gain[i] = st->gain_floor[i];

        /* Exponential decay model for reverberation (unused) */
        /*st->reverb_estimate[i] = st->reverb_decay*st->reverb_estimate[i] + st->reverb_decay*st->reverb_level*st->gain[i]*st->gain[i]*st->ps[i]; */

        /* Take into account speech probability of presence (loudness domain MMSE estimator) */
        /* gain2 = [p*sqrt(gain)+(1-p)*sqrt(gain _floor) ]^2 */
        tmp =
            MULT16_16_P15(p,
                          spx_sqrt(SHL32(EXTEND32(st->gain[i]), 15))) +
                              MULT16_16_P15(
                                  SUB16(Q15_ONE, p),
                                  spx_sqrt(SHL32(EXTEND32(st->gain_floor[i]), 15)));
        st->gain2[i] = SQR16_Q15(tmp);

        /* Use this if you want a log-domain MMSE estimator instead */
        /*st->gain2[i] = pow(st->gain[i], p) * pow(st->gain_floor[i],1.f-p); */
    }
}

#endif


#ifdef OVERRIDE_ANR_APPLY_SPEC_GAIN
static void apply_spectral_gain(SpeexPreprocessState * st)
{
    int             i;
    int             N = st->ps_size;

    for (i = 1; i < N; i++) {
        st->ft[2 * i - 1] = MULT16_16_P15(st->gain2[i], st->ft[2 * i - 1]);
        st->ft[2 * i] = MULT16_16_P15(st->gain2[i], st->ft[2 * i]);
    }
    st->ft[0] = MULT16_16_P15(st->gain2[0], st->ft[0]);
    st->ft[2 * N - 1] = MULT16_16_P15(st->gain2[N - 1], st->ft[2 * N - 1]);
}

#endif

#endif                          // __ARM_FEATURE_MVE
