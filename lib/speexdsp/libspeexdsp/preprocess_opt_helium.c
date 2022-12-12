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



/* ARM with Helium optimized parts */

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



#include <arm_math.h>
#include <arm_helium_utils.h>
#include <arm_vec_math.h>

#define VISIB_ATTR static
//#define VISIB_ATTR __attribute__ ((noinline))

#if defined(FLOATING_POINT)

__STATIC_FORCEINLINE f32x4_t visqrtf_f32(f32x4_t vecIn)
{
    q31x4_t         newtonStartVec;
    f32x4_t         sumHalf, invSqrt;

    newtonStartVec = vdupq_n_s32(INVSQRT_MAGIC_F32) - vshrq((q31x4_t) vecIn, 1);
    sumHalf = vecIn * 0.5f;
    /*
     * compute 2 newton x iterations
     */
    INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, (f32x4_t) newtonStartVec);
    INVSQRT_NEWTON_MVE_F32(invSqrt, sumHalf, invSqrt);

    return invSqrt;
}


__STATIC_FORCEINLINE f32x4_t vsqrtf_f32(f32x4_t vecIn)
{
    /* sqrt(x) = x * invSqrt(x)  */
    return vmulq(vecIn, visqrtf_f32(vecIn));
}



#ifdef OVERRIDE_ANR_VEC_MUL
static void vect_mult(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_mult_f32(pSrcA, pSrcB, pDst, blockSize);
}

#endif


#ifdef OVERRIDE_ANR_VEC_CONV_FROM_INT16
static void vect_conv_from_int16(const spx_int16_t * pSrc, spx_word16_t * pDst, uint32_t blockSize)
{
    arm_q15_to_float(pSrc, pDst, blockSize);
}
#endif



#ifdef OVERRIDE_ANR_OLA
/* vector overlap and add with saturation prior spx_int16_t conversion */
VISIB_ATTR void vect_ola(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_int16_t * pDst, uint32_t blockSize)
{
    int             i;

    int16x8_t       converted = vdupq_n_s16(0);
    for (i = 0; i < blockSize; i += 4) {
        float32x4_t     vtmp = vld1q(pSrcA) + vld1q(pSrcB);

        converted = vqmovnbq(vuninitializedq(converted), vcvtaq_s32_f32(vtmp));
        vstrhq_s32(pDst, converted);
        pDst += 4;
        pSrcA += 4;
        pSrcB += 4;
    }
}
#endif


#ifdef OVERRIDE_ANR_COMPUTE_GAIN_FLOOR
VISIB_ATTR void compute_gain_floor(int noise_suppress, int effective_echo_suppress, spx_word32_t * noise, spx_word32_t * echo, spx_word16_t * gain_floor, int len)
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


    for (i = 0; i < len; i += 4) {
        float32x4_t     vnoise = vld1q(pnoise);
        float32x4_t     vecho = vld1q(pecho);

        // gain_floor[i] =
        //    sqrt(noise_floor*PSHR32(noise[i],NOISE_SHIFT) + echo_floor*echo[i]/(1+PSHR32(noise[i],NOISE_SHIFT) + echo[i]));

        float32x4_t     vden = vaddq(vaddq(vnoise, 1.0f), vecho);
        float32x4_t     vnum = vmulq(vnoise, noise_floor);
        vnum = vfmaq(vnum, vecho, echo_floor);

        float32x4_t     vec_tmp16 = vdiv_f32(vnum, vden);
        vec_tmp16 = vsqrtf_f32(vec_tmp16);

        vst1q(pgain_floor, vec_tmp16);

        pnoise += 4;
        pecho += 4;
        pgain_floor += 4;
    }
}

#endif



#ifdef OVERRIDE_ANR_POWER_SPECTRUM
VISIB_ATTR void power_spectrum(spx_word16_t * ft, spx_word32_t * ps, int N)
{
    int             i;

    ps[0] = MULT16_16(ft[0], ft[0]);
    arm_cmplx_mag_squared_f32(&ft[1], ps + 1, N - 1);
}
#endif


#ifdef OVERRIDE_ANR_UPDATE_NOISE_ESTIMATE
VISIB_ATTR void update_noise_estimate(SpeexPreprocessState * st, spx_word16_t beta, spx_word16_t beta_1)
{
    int             N = st->ps_size;
    int             i;
    int32_t const  *pupdate_prob = st->update_prob;
    float32_t      *pnoise = st->noise;
    float32_t const *pps = st->ps;


    /* Update the noise estimate for the frequencies where it can be */
    for (i = 0; i < N / 4; i++) {
        int32x4_t       prob = vld1q(pupdate_prob);
        float32x4_t     noise = vld1q(pnoise);
        float32x4_t     ps = vld1q(pps);
        /* setup predicate based on update_prob & noise conditions  */
        mve_pred16_t    p0 = vcmpeqq(prob, 0);
        mve_pred16_t    p1 = vcmpltq(ps, noise);

        /* select between max(0, noise*(1-beta) + ps*beta) */
        float32x4_t     tmp = vmaxnmq_m(noise, vdupq_n_f32(0.0f),
                                        vfmaq(vmulq(noise, beta_1), ps, beta), (p0 | p1));

        vst1q(pnoise, tmp);

        pnoise += 4;
        pps += 4;
        pupdate_prob += 4;
    }
}
#endif

#ifdef OVERRIDE_ANR_APOSTERIORI_SNR
VISIB_ATTR void aposteriori_snr(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    float32_t      *ps = (float32_t *) st->ps;;
    float32_t      *pNoise = (float32_t *) st->noise;
    float32_t      *pEchoNoise = (float32_t *) st->echo_noise;
    float32_t      *pRevNoise = (float32_t *) st->reverb_estimate;
    float32_t      *pPs = (float32_t *) ps;
    float32_t      *ppost = (float32_t *) st->post;
    float32_t      *pOldPs = (float32_t *) st->old_ps;
    float32_t      *pPrior = (float32_t *) st->prior;
    float32_t      *pPost = (float32_t *) st->post;


    for (int i = 0; i < ((N + M) / 4); i++) {
        float32x4_t     vnoise = vld1q((float32_t const *) pNoise);
        float32x4_t     vecho = vld1q((float32_t const *) pEchoNoise);
        float32x4_t     vreverb = vld1q((float32_t const *) pRevNoise);
        float32x4_t     vps = vld1q((float32_t const *) pPs);
        float32x4_t     vOldPs = vld1q((float32_t const *) pOldPs);
        float32x4_t     vtmpf32, vtmpf322;
        float32x4_t     vtotalNoise;
        float32x4_t     vprior, vPost;
        float32x4_t     vgamma;

        /* Total noise estimate including residual echo and reverberation */
        vtotalNoise = vaddq(vnoise, 1.0f);
        vtotalNoise = vaddq(vtotalNoise, vaddq(vecho, vreverb));


        /* A posteriori SNR = ps/noise - 1 */
        vtmpf32 = vdiv_f32(vps, vtotalNoise);
        vtmpf32 = vsubq(vtmpf32, 1.0f);

        vPost = vminnmq(vtmpf32, vdupq_n_f32(QCONST32(100.f, SNR_SHIFT)));
        vst1q(pPost, vPost);
        pPost += 4;


        /* Computing update gamma = .1 + .9*(old/(old+noise))^2 */
        vtmpf32 = vdiv_f32(vOldPs, vaddq(vOldPs, vtotalNoise));
        vtmpf32 = vmulq(vtmpf32, vtmpf32);

        vtmpf32 = vmulq(vtmpf32, QCONST32(.89f, 15));
        vgamma = vaddq(vtmpf32, QCONST32(.1f, 15));


        /* A priori SNR update = gamma*max(0,post) + (1-gamma)*old/noise */
        vtmpf32 = vdiv_f32(vOldPs, vtotalNoise);
        vtmpf32 = vmulq(vsubq(vdupq_n_f32(Q15_ONE), vgamma), vtmpf32);

        vtmpf322 = vmaxnmq(vPost, vdupq_n_f32(0));
        vtmpf32 = vfmaq(vtmpf32, vgamma, vtmpf322);
        vprior = vminnmq(vtmpf32, vdupq_n_f32(QCONST32(100.f, SNR_SHIFT)));

        vst1q(pPrior, vprior);
        pPrior += 4;

        pPs += 4;
        pNoise += 4;
        pEchoNoise += 4;
        pRevNoise += 4;
        ppost += 4;
        pOldPs += 4;
    }

}

#endif


#ifdef OVERRIDE_ANR_UPDATE_ZETA
VISIB_ATTR void preprocess_update_zeta(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    int             blkCnt;

    float32_t      *pZeta = (float32_t *) st->zeta;
    float32_t      *pPrior = (float32_t *) st->prior;
    float32_t      *pPriorP1 = pPrior + 1;
    float32_t      *pPriorM1 = pPrior - 1;


    /* Recursive average of the a priori SNR. A bit smoothed for the psd components */
    pZeta[0] = 0.7f * pZeta[0] + 0.3f * pPrior[0];

    pZeta += 1;
    pPrior += 1;
    pPriorP1 += 1;
    pPriorM1 += 1;
    blkCnt = N - 2;
    do {
        mve_pred16_t    tpred = vctp32q(blkCnt);
        float32x4_t     priorPrev = vld1q_z(pPriorM1, tpred);
        float32x4_t     priorcur = vld1q_z(pPrior, tpred);
        float32x4_t     priorNext = vld1q_z(pPriorP1, tpred);
        float32x4_t     zeta = vld1q_z(pZeta, tpred);

        zeta = vmulq_x(zeta, (QCONST32(.7f, 15)), tpred);
        zeta = vfmaq_m(zeta, priorcur, (QCONST32(.15f, 15)), tpred);
        zeta = vfmaq_m(zeta, priorPrev, (QCONST32(.075f, 15)), tpred);
        zeta = vfmaq_m(zeta, priorNext, (QCONST32(.075f, 15)), tpred);
        vst1q_p(pZeta, zeta, tpred);

        pZeta += 4;
        pPrior += 4;
        pPriorP1 += 4;
        pPriorM1 += 4;

        blkCnt -= 4;
    }
    while (blkCnt > 0);


    pZeta = (float32_t *) st->zeta;
    pPrior = (float32_t *) st->prior;
    pZeta += (N - 1);
    pPrior += (N - 1);
    blkCnt = M + 1;
    do {
        mve_pred16_t    tpred = vctp32q(blkCnt);
        float32x4_t     priorcur = vld1q_z(pPrior, tpred);
        float32x4_t     zeta = vld1q_z(pZeta, tpred);

        zeta = vmulq_x(zeta, (QCONST32(.7f, 15)), tpred);
        zeta = vfmaq_m(zeta, priorcur, (QCONST32(.3f, 15)), tpred);
        vst1q_p(pZeta, zeta, tpred);

        pZeta += 4;
        pPrior += 4;
        blkCnt -= 4;
    }
    while (blkCnt > 0);
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


__STATIC_FORCEINLINE float32x4_t vec_qcurve_f32(float32x4_t xx)
{
    float32x4_t     den = vaddq(vmulq(vrecip_f32(xx), 0.15f), 1.0f);
    return vrecip_f32(den);
}


__STATIC_FORCEINLINE float32x4_t vec_hypergeom_gain_f32(float32x4_t xx)
{
    static const float32_t table[32] = {
        0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
        1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
        2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f
    };


    float32x4_t     intg = vrndmq(vmulq(xx, 2.0f));
    uint32x4_t      ind = vcvtq_u32_f32(intg);

    float32x4_t     x = vmulq(xx, EXPIN_SCALING_1);
    float32x4_t     inv = vrecip_f32(x);
    float32x4_t     outbig = vaddq(vmulq(inv, 0.1296f), 1.0f);

    float32x4_t     invSqrt = visqrtf_f32(vaddq(x, .0001f));
    float32x4_t     frac = 2 * x - intg;
    float32x4_t     tabItem0 = vldrwq_gather_shifted_offset(table, ind);
    ind = ind + 1;
    float32x4_t     tabItem1 = vldrwq_gather_shifted_offset(table, ind);

    float32x4_t     outsmall = vmulq(vsubq(vdupq_n_f32(1), frac), tabItem0) + vmulq(frac, tabItem1);

    outsmall = vmulq(outsmall, invSqrt);
    outsmall = vpselq(outbig, outsmall, vcmphiq(ind, 19));

    return outsmall;
}

#ifdef OVERRIDE_ANR_UPDATE_GAINS_CRITICAL_BANDS

VISIB_ATTR void update_gains_critical_bands(SpeexPreprocessState * st, spx_word16_t Pframe)
{

    int             i;
    int             N = st->ps_size;
    int             M = st->nbands;
    float32_t      *ps = (float32_t *) st->ps;

    float32_t      *pprior = (float32_t *) st->prior;
    float32_t      *ppost = (float32_t *) st->post;
    float32_t      *pgain = (float32_t *) st->gain;
    float32_t      *pgain2 = (float32_t *) st->gain2;
    float32_t      *pold_ps = (float32_t *) st->old_ps;
    float32_t      *pzeta = (float32_t *) st->zeta;

    pprior += N;
    ppost += N;
    pgain += N;
    pgain2 += N;
    pold_ps += N;
    ps += N;
    pzeta += N;


    for (i = N; i < N + M; i += 4) {
        /* See EM and Cohen papers */
        float32x4_t     theta;
        /* Gain from hypergeometric function */
        float32x4_t     MM;
        /* Weiner filter gain */
        float32x4_t     prior_ratio;

        float32x4_t     prior = vld1q(pprior);
        float32x4_t     post = vld1q(ppost);


        prior_ratio = vdiv_f32(prior, vaddq(prior, 1.0f));
        theta = vmulq(prior_ratio, vaddq(post, 1.0f));
        MM = vec_hypergeom_gain_f32(theta);

        /* Gain with bound */
        float32x4_t     gain = vminnmq(vmulq(prior_ratio, MM), vdupq_n_f32(1));
        vst1q(pgain, gain);


        /* Save old Bark power spectrum */
        float32x4_t     voldps = vld1q(pold_ps);
        float32x4_t     vps = vld1q(ps);
        voldps = vmulq(voldps, (float32_t) QCONST32(.2f, 15));

        float32x4_t     vtmp;
        vtmp = vmulq(gain, gain);
        vtmp = vmulq(vtmp, (float32_t) QCONST32(.8f, 15));
        vtmp = vmulq(vtmp, vps);

        voldps = voldps + vtmp;
        vst1q(pold_ps, voldps);


        /* a priority probability of speech presence based on Bark sub-band alone */
        float32x4_t     p1v = vaddq(vmulq(vec_qcurve_f32(vld1q(pzeta)),
                                          QCONST32(.8f, 15)),
                                    QCONST32(.199f, 15));

        /* Speech absence a priori probability (considering sub-band and frame) */

        /* potential loss of precision */
        float32x4_t     qv = vsubq(vdupq_n_f32(Q15_ONE), vmulq(p1v, Pframe));

        vtmp = vexpq_f32(vnegq(theta));

        vtmp = vmulq(vtmp, vaddq(prior, 1.0f));
        /* Prevent overflows in the next line */
        vtmp = vminnmq(vdupq_n_f32(QCONST16(3., SNR_SHIFT)), vtmp);
        vtmp = vmulq(vtmp, vdiv_f32(qv, vsubq(vdupq_n_f32(1.0f), qv)));
        vtmp = vaddq(vtmp, 1.0f);
        vtmp = vdiv_f32(vdupq_n_f32(1.0f), vtmp);

        vst1q(pgain2, vtmp);

        pprior += 4;
        ppost += 4;
        pgain += 4;
        pgain2 += 4;
        pzeta += 4;
        pold_ps += 4;
        ps += 4;
    }
}

#endif


#ifdef OVERRIDE_ANR_UPDATE_GAINS_LINEAR

VISIB_ATTR void update_gains_linear(SpeexPreprocessState * st)
{
    int             i;
    int             N = st->ps_size;

    float32_t      *ps = (float32_t *) st->ps;
    float32_t      *pprior = (float32_t *) st->prior;
    float32_t      *ppost = (float32_t *) st->post;
    float32_t      *pgain = (float32_t *) st->gain;
    float32_t      *pgain2 = (float32_t *) st->gain2;
    float32_t      *pold_ps = (float32_t *) st->old_ps;
    float32_t      *pgain_floor = (float32_t *) st->gain_floor;


    for (i = 0; i < N; i += 4) {
        float32x4_t     theta;
        float32x4_t     MM;


        /* Wiener filter gain */
        float32x4_t     prior = vld1q(pprior);
        float32x4_t     post = vld1q(ppost);

        float32x4_t     prior_ratio = vdiv_f32(prior, vaddq(prior, 1.0f));
        theta = vmulq(prior_ratio, vaddq(post, 1.0f));
        /* Optimal estimator for loudness domain */
        MM = vec_hypergeom_gain_f32(theta);

        /* Gain with bound */
        float32x4_t     g = vminnmq_f32(vmulq(prior_ratio, MM), vdupq_n_f32(1));

        /* Interpolated speech probability of presence */
        float32x4_t     p = vld1q(pgain2);

        /* Constrain the gain to be close to the Bark scale gain */
        float32x4_t     vecgain = vld1q(pgain);

        vecgain = vmulq(vecgain, 3.0f);
        g = vminnmq(g, vecgain);
        vst1q(pgain, g);


        /* Save old Bark power spectrum */
        float32x4_t     voldps = vld1q(pold_ps);
        float32x4_t     vps = vld1q(ps);
        voldps = vmulq(voldps, QCONST32(.2f, 15));

        float32x4_t     vtmp;
        vtmp = vmulq(g, g);
        vtmp = vmulq(vtmp, QCONST32(.8f, 15));
        vtmp = vmulq(vtmp, vps);

        voldps = voldps + vtmp;
        vst1q(pold_ps, voldps);

        /* Apply gain floor */
        float32x4_t     vgfloor = vld1q(pgain_floor);
        g = vmaxnmq(g, vgfloor);
        vst1q(pgain, g);

        /* Take into account speech probability of presence (loudness domain MMSE estimator) */
        /* gain2 = [p*sqrt(gain)+(1-p)*sqrt(gain _floor) ]^2 */
        vtmp = vmulq(p, vsqrtf_f32(g))
            + vmulq(vsubq(vdupq_n_f32(Q15_ONE), p), vsqrtf_f32(vgfloor));

        vst1q(pgain2, vmulq(vtmp, vtmp));

        pprior += 4;
        ppost += 4;
        pgain += 4;
        pgain2 += 4;
        pold_ps += 4;
        ps += 4;
        pgain_floor += 4;
    }
}

#endif


#ifdef OVERRIDE_ANR_APPLY_SPEC_GAIN
/* Can be autovectorized with Arm Compiler */
VISIB_ATTR void apply_spectral_gain(SpeexPreprocessState * st)
{
    int             i;
    int             N = st->ps_size;

    arm_cmplx_mult_real_f32(st->ft + 1, st->gain2 + 1, st->ft + 1, N - 1);

    st->ft[0] = MULT16_16_P15(st->gain2[0], st->ft[0]);
    st->ft[2 * N - 1] = MULT16_16_P15(st->gain2[N - 1], st->ft[2 * N - 1]);
}

#endif

#ifdef OVERRIDE_ANR_UPDATE_NOISE_PROB
/* Can be autovectorized with Arm Compiler */
VISIB_ATTR void update_noise_prob(SpeexPreprocessState * st)
{
    int             i;
    int             min_range;
    int             N = st->ps_size;
    float32_t      *pS = st->S + 1;
    float32_t      *pPsm1 = st->ps;
    float32_t      *pPs = st->ps + 1;
    float32_t      *pPsp1 = st->ps + 2;
    int32_t         blockSize = N - 2;
    float32_t       c0 = QCONST16(.8f, 15);
    float32_t       c1 = QCONST16(.05f, 15);
    float32_t       c2 = QCONST16(.1f, 15);
    float32_t       c3 = QCONST16(.05f, 15);
    float32x4_t     vecS, vecPsm1, vecPs, vecPsp1, vecTmp;

    do {
        mve_pred16_t    tpred = vctp32q(blockSize);

        vecS = vld1q(pS);
        vecPsm1 = vld1q(pPsm1);
        vecPs = vld1q(pPs);
        vecPsp1 = vld1q(pPsp1);

        vecTmp = vfmaq(vfmaq(vfmaq(vmulq(vecS, c0), vecPsm1, c1), vecPs, c2), vecPsp1, c3);

        vst1q_p(pS, vecTmp, tpred);
        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pS += 4;
        pPsm1 += 4;
        pPs += 4;
        pPsp1 += 4;

        blockSize -= 4;
    }
    while (blockSize > 0);

    st->S[0] = MULT16_32_Q15(QCONST16(.8f, 15), st->S[0]) + MULT16_32_Q15(QCONST16(.2f, 15), st->ps[0]);
    st->S[N - 1] = MULT16_32_Q15(QCONST16(.8f, 15), st->S[N - 1]) + MULT16_32_Q15(QCONST16(.2f, 15), st->ps[N - 1]);


    if (st->nb_adapt == 1) {
        for (i = 0; i < N; i++)
            st->Smin[i] = st->Stmp[i] = 0;
    }

    if (st->nb_adapt < 100)
        min_range = 15;
    else if (st->nb_adapt < 1000)
        min_range = 50;
    else if (st->nb_adapt < 10000)
        min_range = 150;
    else
        min_range = 300;


    if (st->min_count > min_range) {
        st->min_count = 0;
        float32_t      *pSmin = st->Smin;
        float32_t      *pStmp = st->Stmp;
        pS = st->S;
        for (i = 0; i < N / 4; i++) {
            float32x4_t     S = vld1q(pS);
            vst1q(pSmin, vminnmq(vld1q(pStmp), S));
            vst1q(pStmp, S);
            pS += 4;
            pSmin += 4;
            pStmp += 4;
        }
    } else {
        float32_t      *pSmin = st->Smin;
        float32_t      *pStmp = st->Stmp;
        pS = st->S;
        for (i = 0; i < N / 4; i++) {
            vst1q(pSmin, vminnmq(vld1q(pSmin), vld1q(pS)));
            vst1q(pStmp, vminnmq(vld1q(pStmp), vld1q(pS)));
            pS += 4;
            pSmin += 4;
            pStmp += 4;
        }
    }

    float32_t       c = QCONST16(.4f, 15);
    float32_t      *pSMin = st->Smin;
    int            *pProb = st->update_prob;

    pS = st->S;

    for (i = 0; i < N / 4; i++) {
        float32x4_t     vecS = vld1q(pS);
        float32x4_t     vecSmin = vld1q(pSMin);
        int32x4_t       vecProb = vdupq_n_s32(0);

        vecProb = vdupq_m_n_s32(vecProb, 1, vcmpgtq(vmulq(vecS, c), vecSmin));

        vst1q(pProb, vecProb);
        pProb += 4;
        pS += 4;
        pSMin += 4;
    }
}

#endif

#else

/* FIXED_POINT not needed for EEMBC AudioMark */
#error "Fixed Point Preprocess Helium Optimization is not available : Please contact ARM if needed"

#endif
