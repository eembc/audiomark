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



/* RISCV with Vector optimized parts */

/*
 * Copyright (c) 2026 Robin John
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

#include <riscv_vector.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#if defined(FLOATING_POINT)

#define VISIB_ATTR static
#define __STATIC_FORCEINLINE  static __attribute__((always_inline)) inline

#ifdef OVERRIDE_ANR_VEC_MUL
VISIB_ATTR void vect_mult(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_word16_t * pDst, uint32_t blockSize)
{
    vfloat32m2_t vSrcA;
    vfloat32m2_t vSrcB;
    vfloat32m2_t vRes;
    size_t len = blockSize;
    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m2(len);
        /* C = A + B */
        vSrcA = __riscv_vle32_v_f32m2(pSrcA, vl);
        vSrcB = __riscv_vle32_v_f32m2(pSrcB, vl);
        vRes  = __riscv_vfmul_vv_f32m2(vSrcA, vSrcB, vl);
        __riscv_vse32_v_f32m2(pDst, vRes, vl);
        /* Increment pointers */
        pSrcA += vl;
        pSrcB += vl; 
        pDst  += vl;
        /* Decrement the loop counter */
        len -= vl;
    }
}

#endif

#ifdef OVERRIDE_ANR_VEC_CONV_FROM_INT16
VISIB_ATTR void vect_conv_from_int16(const spx_int16_t * pSrc, spx_word16_t * pDst, uint32_t blockSize)
{
    size_t len = blockSize;

    while(len > 0){
        size_t vl = __riscv_vsetvl_e16m1(len);
        vint16m1_t vSrc = __riscv_vle16_v_i16m1(pSrc, vl);

        vfloat32m2_t vRes = __riscv_vfwcvt_f_x_v_f32m2(vSrc, vl);
        __riscv_vse32_v_f32m2(pDst, vRes, vl);

        /* Increment pointers */
        pDst += vl;
        pSrc += vl;
        /* Decrement loop counter */
        len -= vl;
    }
}

#endif

#ifdef OVERRIDE_ANR_OLA
/* vector overlap and add with saturation prior spx_int16_t conversion */
VISIB_ATTR void vect_ola(const spx_word16_t * pSrcA, const spx_word16_t * pSrcB, spx_int16_t * pDst, uint32_t blockSize)
{
    size_t len = blockSize;
    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m4(len);

        vfloat32m4_t vSrcA = __riscv_vle32_v_f32m4(pSrcA, vl);
        vfloat32m4_t vSrcB = __riscv_vle32_v_f32m4(pSrcB, vl);
        vfloat32m4_t vTmp  = __riscv_vfadd_vv_f32m4(vSrcA, vSrcB, vl);
        vint16m2_t   vConv = __riscv_vfncvt_x_f_w_i16m2_rm(vTmp, __RISCV_FRM_RMM, vl); 

        __riscv_vse16_v_i16m2(pDst, vConv, vl);

        pSrcA += vl;
        pSrcB += vl;
        pDst  += vl;
        len   -= vl;
    }

}
#endif

#ifdef OVERRIDE_ANR_COMPUTE_GAIN_FLOOR
VISIB_ATTR void compute_gain_floor(int noise_suppress, int effective_echo_suppress, spx_word32_t * noise, spx_word32_t * echo, spx_word16_t * gain_floor, int len)
{
    float       echo_floor;
    float       noise_floor;

    noise_floor = expf(.2302585f * noise_suppress);
    echo_floor  = expf(.2302585f * effective_echo_suppress);

    /* Compute the gain floor based on different floors for the background noise and residual echo */
    float      *pnoise = (float *) noise;
    float      *pecho = (float *) echo;
    float      *pgain_floor = (float *) gain_floor;

    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m2(len);
        vfloat32m2_t vNoise = __riscv_vle32_v_f32m2(pnoise, vl);
        vfloat32m2_t vEcho  = __riscv_vle32_v_f32m2(pecho, vl);

        vfloat32m2_t vDen = __riscv_vfadd_vf_f32m2(vNoise, 1.0f, vl);
        vDen = __riscv_vfadd_vv_f32m2(vDen, vEcho, vl);
        vfloat32m2_t vNum = __riscv_vfmul_vf_f32m2(vNoise, noise_floor, vl);
        vNum = __riscv_vfmacc_vf_f32m2(vNum, echo_floor, vEcho, vl);

        vfloat32m2_t vTmp = __riscv_vfdiv_vv_f32m2(vNum, vDen, vl);
        vTmp = __riscv_vfsqrt_v_f32m2(vTmp, vl);

        __riscv_vse32_v_f32m2(pgain_floor, vTmp, vl);

        pnoise += vl;
        pecho += vl;
        pgain_floor += vl;

        len -= vl;
    }

}
#endif

#ifdef OVERRIDE_ANR_POWER_SPECTRUM
VISIB_ATTR void power_spectrum(spx_word16_t * ft, spx_word32_t * ps, int N)
{
    ps[0] = MULT16_16(ft[0], ft[0]);

    size_t len = N - 1;

    float *pSrc = ft + 1;
    float *pDst = ps + 1;

    while(len > 0){
        size_t vl             = __riscv_vsetvl_e32m2(len);
        vfloat32m2x2_t vCmplx = __riscv_vlseg2e32_v_f32m2x2(pSrc, vl);
        vfloat32m2_t   vReal  = __riscv_vget_v_f32m2x2_f32m2(vCmplx, 0);
        vfloat32m2_t   vImg   = __riscv_vget_v_f32m2x2_f32m2(vCmplx, 1);

        vfloat32m2_t vSum = __riscv_vfmul_vv_f32m2(vReal, vReal, vl);

        vSum = __riscv_vfmacc_vv_f32m2(vSum, vImg, vImg, vl);

        __riscv_vse32_v_f32m2(pDst, vSum, vl);
        pSrc += vl * 2;
        pDst += vl;
        len -= vl;
    }

}
#endif

#ifdef OVERRIDE_ANR_UPDATE_NOISE_ESTIMATE
VISIB_ATTR void update_noise_estimate(SpeexPreprocessState * st, spx_word16_t beta, spx_word16_t beta_1)
{
    int             len = st->ps_size;
    int32_t const  *pupdate_prob = (int32_t const *)st->update_prob;
    float          *pnoise = st->noise;
    float   const  *pps = st->ps;

    /* Update the noise estimate for the frequencies where it can be */
    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m2(len);

        vint32m2_t   vProb  = __riscv_vle32_v_i32m2(pupdate_prob, vl);
        vfloat32m2_t vNoise = __riscv_vle32_v_f32m2(pnoise, vl);
        vfloat32m2_t vPs    = __riscv_vle32_v_f32m2(pps, vl);

        /* setup mask based on update_prob & noise conditions  */
        vbool16_t vP0   = __riscv_vmseq_vx_i32m2_b16(vProb, 0, vl);
        vbool16_t vP1   = __riscv_vmflt_vv_f32m2_b16(vPs, vNoise, vl);
        vbool16_t vMask = __riscv_vmor_mm_b16(vP0, vP1, vl);

        /* Tmp = Noise + beta * (Ps - Noise) */
        vfloat32m2_t vTmp = __riscv_vfmul_vf_f32m2(vNoise, beta_1, vl);
        vTmp = __riscv_vfmacc_vf_f32m2(vTmp, beta, vPs, vl);
        /* select between max(0, noise*(1-beta) + ps*beta) */
        vTmp = __riscv_vfmax_vf_f32m2_mu(vMask, vNoise, vTmp, 0.0f, vl);

        __riscv_vse32_v_f32m2(pnoise, vTmp, vl);

        pnoise       += vl;
        pps          += vl;
        pupdate_prob += vl;

        len          -= vl;
    }

}
#endif

#ifdef OVERRIDE_ANR_APOSTERIORI_SNR
VISIB_ATTR void aposteriori_snr(SpeexPreprocessState * st)
{
    int                  N = st->ps_size;
    int                  M = st->nbands;
    float      *pNoise     = (float *) st->noise;
    float      *pEchoNoise = (float *) st->echo_noise;
    float      *pRevNoise  = (float *) st->reverb_estimate;
    float      *pPs        = (float *) st->ps;
    float      *pOldPs     = (float *) st->old_ps;
    float      *pPrior     = (float *) st->prior;
    float      *pPost      = (float *) st->post;

    int len = N + M;
    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m2(len);
        vfloat32m2_t vNoise  = __riscv_vle32_v_f32m2(pNoise, vl);
        vfloat32m2_t vEcho   = __riscv_vle32_v_f32m2(pEchoNoise, vl);
        vfloat32m2_t vReverb = __riscv_vle32_v_f32m2(pRevNoise, vl);
        vfloat32m2_t vPs     = __riscv_vle32_v_f32m2(pPs, vl);
        vfloat32m2_t vOldPs  = __riscv_vle32_v_f32m2(pOldPs, vl);

        vfloat32m2_t vTmpf32, vTmpf322;
        vfloat32m2_t vTotalNoise;
        vfloat32m2_t vPrior, vPost;
        vfloat32m2_t vGamma, vTmp;

        /* Total noise estimate including residual echo and reverberation */
        vTotalNoise = __riscv_vfadd_vf_f32m2(vNoise, 1.0f, vl);
        vTmp        = __riscv_vfadd_vv_f32m2(vEcho, vReverb, vl);
        vTotalNoise = __riscv_vfadd_vv_f32m2(vTotalNoise, vTmp, vl);

        /* A posteriori SNR = ps/noise - 1 */
        vTmpf32 = __riscv_vfdiv_vv_f32m2(vPs, vTotalNoise, vl); 
        vTmpf32 = __riscv_vfsub_vf_f32m2(vTmpf32, 1.0f, vl);
        vPost   = __riscv_vfmin_vf_f32m2(vTmpf32, QCONST32(100.f, SNR_SHIFT), vl);

        __riscv_vse32_v_f32m2(pPost, vPost, vl);

        pPost += vl;
        vTmp = __riscv_vfadd_vv_f32m2(vOldPs, vTotalNoise, vl);

        /* Computing update gamma = .1 + .9*(old/(old+noise))^2 */
        vTmpf32 = __riscv_vfdiv_vv_f32m2(vOldPs, vTmp, vl);
        vTmpf32 = __riscv_vfmul_vv_f32m2(vTmpf32, vTmpf32, vl);
        vTmpf32 = __riscv_vfmul_vf_f32m2(vTmpf32, QCONST32(0.89f, 15), vl);
        vGamma  = __riscv_vfadd_vf_f32m2(vTmpf32, QCONST32(0.1f, 15), vl);

        /* A priori SNR update = gamma*max(0,post) + (1-gamma)*old/noise */
        vTmpf32 = __riscv_vfdiv_vv_f32m2(vOldPs, vTotalNoise, vl);
        vTmp    = __riscv_vfrsub_vf_f32m2(vGamma, Q15_ONE, vl);
        vTmpf32 = __riscv_vfmul_vv_f32m2(vTmp, vTmpf32, vl);

        vTmp     = __riscv_vfmax_vf_f32m2(vPost, 0.0f, vl);
        vTmpf32  = __riscv_vfmacc_vv_f32m2(vTmpf32, vGamma, vTmp, vl);
        vPrior   = __riscv_vfmin_vf_f32m2(vTmpf32, QCONST32(100.f, SNR_SHIFT), vl);

        __riscv_vse32_v_f32m2(pPrior, vPrior, vl);

        pPrior     += vl;

        pPs        += vl;
        pNoise     += vl;
        pEchoNoise += vl;
        pRevNoise  += vl;
        pOldPs     += vl;

        len        -= vl;


    }

}

#endif

#ifdef OVERRIDE_ANR_UPDATE_ZETA
VISIB_ATTR void preprocess_update_zeta(SpeexPreprocessState * st)
{
    int             N = st->ps_size;
    int             M = st->nbands;
    int             blkCnt;

    float      *pZeta = (float *) st->zeta;
    float      *pPrior = (float *) st->prior;

    /* Recursive average of the a priori SNR. A bit smoothed for the psd components */
    pZeta[0] = 0.7f * pZeta[0] + 0.3f * pPrior[0];

    float pPriorPrv = pPrior[0];
    float pPriorNxt; 

    pZeta += 1;
    blkCnt = N - 2;
    while(blkCnt > 0){
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);
        vfloat32m2_t vPriorCur  = __riscv_vle32_v_f32m2(pPrior + 1, vl);

        pPriorNxt = pPrior[1 + vl];

        vfloat32m2_t vPriorPrv = __riscv_vfslide1up_vf_f32m2(vPriorCur, pPriorPrv, vl);
        vfloat32m2_t vPriorNxt = __riscv_vfslide1down_vf_f32m2(vPriorCur, pPriorNxt, vl);
        vfloat32m2_t zeta      = __riscv_vle32_v_f32m2(pZeta, vl);

        zeta = __riscv_vfmul_vf_f32m2(zeta, QCONST32(.7f, 15), vl);
        zeta = __riscv_vfmacc_vf_f32m2(zeta, QCONST32(.15f, 15), vPriorCur, vl);
        zeta = __riscv_vfmacc_vf_f32m2(zeta, QCONST32(.075f, 15), vPriorPrv, vl);
        zeta = __riscv_vfmacc_vf_f32m2(zeta, QCONST32(.075f, 15), vPriorNxt, vl);

        __riscv_vse32_v_f32m2(pZeta, zeta, vl);

        pPriorPrv = pPrior[vl];

        pZeta  += vl;
        pPrior += vl;

        blkCnt -= vl;
    }

    pZeta  = (float *) st->zeta;
    pPrior = (float *) st->prior;
    pZeta  += (N - 1);
    pPrior += (N - 1);
    blkCnt = M + 1;

    while(blkCnt > 0){
        size_t vl = __riscv_vsetvl_e32m2(blkCnt);
        vfloat32m2_t priorcur = __riscv_vle32_v_f32m2(pPrior, vl);
        vfloat32m2_t zeta = __riscv_vle32_v_f32m2(pZeta, vl);

        zeta = __riscv_vfmul_vf_f32m2(zeta, QCONST32(.7f, 15), vl);
        zeta = __riscv_vfmacc_vf_f32m2(zeta, QCONST32(.3f, 15), priorcur, vl);
        __riscv_vse32_v_f32m2(pZeta, zeta, vl);

        pZeta  += vl;
        pPrior += vl;
        blkCnt -= vl;
    }

}

#endif

#ifdef OVERRIDE_ANR_HYPERGEOM_GAIN
static inline spx_word32_t hypergeom_gain(spx_word32_t xx)
{
    int             ind;
    float           integer, frac;
    static const float table[21] = {
        0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
        1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
        2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f
    };
    integer = floor(2 * xx);
    ind = (int) integer;
    if (ind < 0)
        return 1.f;
    if (ind > 19)
        return (1 + .1296 / xx);
    frac = 2 * xx - integer;
    return ((1 - frac) * table[ind] + frac * table[ind + 1]) / sqrt(xx + .0001f);
}
#endif

__STATIC_FORCEINLINE vfloat32m2_t vec_hypergeom_gain_f32(vfloat32m2_t xx, size_t vl)
{
    static const float table[32] = {
        0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
        1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
        2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f
    };


    vfloat32m2_t intg = __riscv_vfmul_vf_f32m2(xx, 2.0f, vl);
    vuint32m2_t  ind0 = __riscv_vfcvt_xu_f_v_u32m2_rm(intg, __RISCV_FRM_RDN, vl);

    vfloat32m2_t intgFloor = __riscv_vfcvt_f_xu_v_f32m2(ind0, vl);

    vfloat32m2_t inv    = __riscv_vfrdiv_vf_f32m2(xx, 1.0f, vl);
    vfloat32m2_t outbig = __riscv_vfmul_vf_f32m2(inv, 0.1296f, vl);
    outbig = __riscv_vfadd_vf_f32m2(outbig, 1.0f, vl);

    vfloat32m2_t invSqrt = __riscv_vfadd_vf_f32m2(xx, 0.0001f, vl);
    invSqrt = __riscv_vfsqrt_v_f32m2(invSqrt, vl); 
    invSqrt = __riscv_vfrdiv_vf_f32m2(invSqrt, 1.0f, vl);
    vfloat32m2_t frac = __riscv_vfsub_vv_f32m2(intg, intgFloor, vl);

    vuint32m2_t  offset0  = __riscv_vsll_vx_u32m2(ind0, 2, vl);
    vfloat32m2_t tabItem0 = __riscv_vluxei32_v_f32m2(table, offset0, vl);

    vuint32m2_t  ind1     = __riscv_vadd_vx_u32m2(ind0, 1, vl);
    vuint32m2_t  offset1  = __riscv_vsll_vx_u32m2(ind1, 2, vl);
    vfloat32m2_t tabItem1 = __riscv_vluxei32_v_f32m2(table, offset1, vl);

    vfloat32m2_t tabDiff  = __riscv_vfsub_vv_f32m2(tabItem1, tabItem0, vl);
    vfloat32m2_t outsmall = __riscv_vfmacc_vv_f32m2(tabItem0, frac, tabDiff, vl);

    outsmall = __riscv_vfmul_vv_f32m2(outsmall, invSqrt, vl);

    vbool16_t mask = __riscv_vmsgtu_vx_u32m2_b16(ind1, 19, vl);
    outsmall = __riscv_vmerge_vvm_f32m2(outsmall, outbig, mask, vl);

    return outsmall;

}

__STATIC_FORCEINLINE vfloat32m2_t vtaylor_polyq_f32(vfloat32m2_t x, size_t vl)
{
    /* Exponent polynomial coefficients */
    const float c0 = 1.0f;
    const float c1 = 0.0416598916054f;
    const float c2 = 0.500000596046f;
    const float c3 = 0.0014122662833f;
    const float c4 = 1.00000011921f;
    const float c5 = 0.00833693705499f;
    const float c6 = 0.166665703058f;
    const float c7 = 0.000195780929062f;

    vfloat32m2_t         A = __riscv_vfmul_vf_f32m2(x, c0, vl);
    A = __riscv_vfadd_vf_f32m2(A, c4, vl);
    vfloat32m2_t         B = __riscv_vfmul_vf_f32m2(x, c6, vl);
    B = __riscv_vfadd_vf_f32m2(B, c2, vl);
    vfloat32m2_t         C = __riscv_vfmul_vf_f32m2(x, c5, vl);
    C = __riscv_vfadd_vf_f32m2(C, c1, vl);
    vfloat32m2_t         D = __riscv_vfmul_vf_f32m2(x, c7, vl);
    D = __riscv_vfadd_vf_f32m2(D, c3, vl);

    vfloat32m2_t         x2 = __riscv_vfmul_vv_f32m2(x, x, vl);
    vfloat32m2_t         x4 = __riscv_vfmul_vv_f32m2(x2, x2, vl);

    vfloat32m2_t         ABx2 = __riscv_vfmacc_vv_f32m2(A, B, x2, vl);
    vfloat32m2_t         CDx2 = __riscv_vfmacc_vv_f32m2(C, D, x2, vl);

    vfloat32m2_t         res  = __riscv_vfmacc_vv_f32m2(ABx2, CDx2, x4, vl);

    return res;
}

__STATIC_FORCEINLINE vfloat32m2_t vexpq_f32(vfloat32m2_t x, size_t vl)
{
    /* Perform range reduction [-log(2),log(2)] */
    vfloat32m2_t scale = __riscv_vfmul_vf_f32m2(x, 1.4426950408f, vl);
    vint32m2_t   mi32  = __riscv_vfcvt_rtz_x_f_v_i32m2(scale, vl);
    vfloat32m2_t mf32  = __riscv_vfcvt_f_x_v_f32m2(mi32, vl);
    vfloat32m2_t val   = __riscv_vfnmsub_vf_f32m2(mf32, 0.6931471805f, x, vl);

    /* Polynomial Approximation */
    vfloat32m2_t polyf32 = vtaylor_polyq_f32(val, vl);

    /* Reconstruct */
    vint32m2_t mScaled = __riscv_vsll_vx_i32m2(mi32, 23, vl);
    vint32m2_t polyi32 = __riscv_vreinterpret_v_f32m2_i32m2(polyf32);
    polyi32 = __riscv_vadd_vv_i32m2(polyi32, mScaled, vl);
    polyf32 = __riscv_vreinterpret_v_i32m2_f32m2(polyi32);

    vbool16_t uMask = __riscv_vmslt_vx_i32m2_b16(mi32, -126, vl);
    polyf32 = __riscv_vfmerge_vfm_f32m2(polyf32, 0.0f, uMask, vl);

    return polyf32;
}

#ifdef OVERRIDE_ANR_UPDATE_GAINS_CRITICAL_BANDS
VISIB_ATTR void update_gains_critical_bands(SpeexPreprocessState * st, spx_word16_t Pframe)
{

    int             N = st->ps_size;
    int             M = st->nbands;

    float      *ps      = (float *) st->ps;
    float      *pprior  = (float *) st->prior;
    float      *ppost   = (float *) st->post;
    float      *pgain   = (float *) st->gain;
    float      *pgain2  = (float *) st->gain2;
    float      *pold_ps = (float *) st->old_ps;
    float      *pzeta   = (float *) st->zeta;

    pprior  += N;
    ppost   += N;
    pgain   += N;
    pgain2  += N;
    pold_ps += N;
    ps      += N;
    pzeta   += N;

    int        len = M;
    while(len > 0){

        size_t vl = __riscv_vsetvl_e32m2(len);

        /* See EM and Cohen papers */
        vfloat32m2_t vTheta;
        /* Gain from hypergeometric function */
        vfloat32m2_t vMM;
        vfloat32m2_t vPriorRatio;

        vfloat32m2_t vPrior        = __riscv_vle32_v_f32m2(pprior, vl);
        vfloat32m2_t vPost         = __riscv_vle32_v_f32m2(ppost, vl);
        vfloat32m2_t vPriorPlusOne = __riscv_vfadd_vf_f32m2(vPrior, 1.0f, vl);
        vPriorRatio = __riscv_vfdiv_vv_f32m2(vPrior, vPriorPlusOne, vl);
        vTheta      = __riscv_vfadd_vf_f32m2(vPost, 1.0f, vl);
        vTheta      = __riscv_vfmul_vv_f32m2(vPriorRatio, vTheta, vl);
        vMM         = vec_hypergeom_gain_f32(vTheta, vl);

        /* Gain with bound */
        vfloat32m2_t vTmp  = __riscv_vfmul_vv_f32m2(vPriorRatio, vMM, vl);
        vfloat32m2_t vGain = __riscv_vfmin_vf_f32m2(vTmp, 1.0f, vl);
        __riscv_vse32_v_f32m2(pgain, vGain, vl);

        /* Save old Bark power spectrum */
        vfloat32m2_t vOldPs = __riscv_vle32_v_f32m2(pold_ps, vl);
        vfloat32m2_t vPs    = __riscv_vle32_v_f32m2(ps, vl);
        vOldPs = __riscv_vfmul_vf_f32m2(vOldPs, QCONST32(.2f, 15), vl);

        vTmp   = __riscv_vfmul_vv_f32m2(vGain, vGain, vl);
        vTmp   = __riscv_vfmul_vf_f32m2(vTmp, QCONST32(.8f, 15), vl);
        vOldPs = __riscv_vfmacc_vv_f32m2(vOldPs, vTmp, vPs, vl);

        __riscv_vse32_v_f32m2(pold_ps, vOldPs, vl);

        /* a priority probability of speech presence based on Bark sub-band alone */
        vTmp = __riscv_vle32_v_f32m2(pzeta, vl);

        vfloat32m2_t vDen = __riscv_vfadd_vf_f32m2(vTmp, 0.15f, vl);
        vTmp = __riscv_vfdiv_vv_f32m2(vTmp, vDen, vl);

        vTmp = __riscv_vfmul_vf_f32m2(vTmp, QCONST32(.8f, 15), vl);
        vfloat32m2_t vPv = __riscv_vfadd_vf_f32m2(vTmp, QCONST32(.199f, 15), vl);
        vfloat32m2_t vP  = __riscv_vfmul_vf_f32m2(vPv, Pframe, vl);
        /* Speech absence a priori probability (considering sub-band and frame) */
        /* potential loss of precision */
        vfloat32m2_t vQ = __riscv_vfrsub_vf_f32m2(vP, Q15_ONE, vl);
        vTmp = vexpq_f32(__riscv_vfneg_v_f32m2(vTheta, vl), vl);
        vTmp = __riscv_vfmul_vv_f32m2(vTmp, vPriorPlusOne, vl);

        /* Prevent overflows in the next line */
        vTmp = __riscv_vfmin_vf_f32m2(vTmp,QCONST16(3., SNR_SHIFT), vl);
        vTmp = __riscv_vfmul_vv_f32m2(vTmp, vQ, vl);
        vTmp = __riscv_vfadd_vv_f32m2(vTmp, vP, vl);
        vTmp = __riscv_vfdiv_vv_f32m2(vP, vTmp, vl);

        __riscv_vse32_v_f32m2(pgain2, vTmp, vl);

        pprior  += vl;
        ppost   += vl;
        pgain   += vl;
        pgain2  += vl;
        pzeta   += vl;
        pold_ps += vl;
        ps      += vl;

        len     -= vl;
    }
}

#endif

#ifdef OVERRIDE_ANR_UPDATE_GAINS_LINEAR
VISIB_ATTR void update_gains_linear(SpeexPreprocessState * st)
{
    int             N = st->ps_size;

    float      *ps = (float *) st->ps;
    float      *pprior = (float *) st->prior;
    float      *ppost = (float *) st->post;
    float      *pgain = (float *) st->gain;
    float      *pgain2 = (float *) st->gain2;
    float      *pold_ps = (float *) st->old_ps;
    float      *pgain_floor = (float *) st->gain_floor;

    int        len = N;

    while(len > 0){

        size_t vl = __riscv_vsetvl_e32m2(len);
        vfloat32m2_t vTheta;
        vfloat32m2_t vMM;

        /* Wiener filter gain */
        vfloat32m2_t vPrior = __riscv_vle32_v_f32m2(pprior, vl);
        vfloat32m2_t vPost = __riscv_vle32_v_f32m2(ppost, vl);

        vfloat32m2_t vPriorRatio = __riscv_vfadd_vf_f32m2(vPrior, 1.0f, vl);
        vPriorRatio = __riscv_vfdiv_vv_f32m2(vPrior, vPriorRatio, vl);
        vTheta      = __riscv_vfadd_vf_f32m2(vPost, 1.0f, vl);
        vTheta      = __riscv_vfmul_vv_f32m2(vPriorRatio, vTheta, vl);
        /* Optimal Estimator for Loudness Domain */
        vMM = vec_hypergeom_gain_f32(vTheta, vl);

        /* Gain with bound */
        vfloat32m2_t vGain = __riscv_vfmul_vv_f32m2(vPriorRatio, vMM, vl);
        vGain = __riscv_vfmin_vf_f32m2(vGain, 1.0f, vl);

        /* Interpolated speech probability of presence */
        vfloat32m2_t vP = __riscv_vle32_v_f32m2(pgain2, vl);

        /* Constrain the gain to be close to the Bark Scale Gain */
        vfloat32m2_t vecGain = __riscv_vle32_v_f32m2(pgain, vl);
        vecGain = __riscv_vfmul_vf_f32m2(vecGain, 3.0f, vl);
        vGain   = __riscv_vfmin_vv_f32m2(vGain, vecGain, vl);

        /* Save old Bark power spectrum */
        vfloat32m2_t vOldPs = __riscv_vle32_v_f32m2(pold_ps, vl);
        vfloat32m2_t vPs    = __riscv_vle32_v_f32m2(ps, vl);
        vOldPs = __riscv_vfmul_vf_f32m2(vOldPs, QCONST32(.2f, 15), vl);

        vfloat32m2_t vTmp;
        vTmp = __riscv_vfmul_vv_f32m2(vGain, vGain, vl);
        vTmp = __riscv_vfmul_vf_f32m2(vTmp, QCONST32(.8f, 15), vl);
        vTmp = __riscv_vfmul_vv_f32m2(vTmp, vPs, vl);

        vOldPs = __riscv_vfadd_vv_f32m2(vOldPs, vTmp, vl);
        __riscv_vse32_v_f32m2(pold_ps, vOldPs, vl);

        /* Apply gain floor */
        vfloat32m2_t vGFloor = __riscv_vle32_v_f32m2(pgain_floor, vl);
        vGain = __riscv_vfmax_vv_f32m2(vGain, vGFloor, vl);
        __riscv_vse32_v_f32m2(pgain, vGain, vl);

        /* Take into account speech probability of presence (loudness domain MMSE estimator) */
        /* gain2 = [p*sqrt(gain)+(1-p)*sqrt(gain _floor) ]^2 */
        vfloat32m2_t vSqrtG      = __riscv_vfsqrt_v_f32m2(vGain, vl);
        vfloat32m2_t vSqrtGFloor = __riscv_vfsqrt_v_f32m2(vGFloor, vl);
        vfloat32m2_t vSqrtDiff   = __riscv_vfsub_vv_f32m2(vSqrtG, vSqrtGFloor, vl);

        vecGain = __riscv_vfmacc_vv_f32m2(vSqrtGFloor, vP, vSqrtDiff, vl);
        vecGain = __riscv_vfmul_vv_f32m2(vecGain, vecGain, vl);
        __riscv_vse32_v_f32m2(pgain2, vecGain, vl);

        pprior      += vl;
        ppost       += vl;
        pgain       += vl;
        pgain2      += vl;
        pold_ps     += vl;
        ps          += vl;
        pgain_floor += vl;

        len -= vl;
    }

}

#endif

#ifdef OVERRIDE_ANR_APPLY_SPEC_GAIN
VISIB_ATTR void apply_spectral_gain(SpeexPreprocessState * st)
{
    int             N = st->ps_size;

    const float * pSrcReal  = st->gain2 + 1;
    float * pCmplx = st->ft + 1;

    size_t len = N - 1;

    while (len > 0U) 
    {
        size_t vl = __riscv_vsetvl_e32m2(len);
        vfloat32m2_t   vGain      = __riscv_vle32_v_f32m2(pSrcReal, vl);
        vfloat32m2x2_t vCmplx     = __riscv_vlseg2e32_v_f32m2x2(pCmplx, vl);
        vfloat32m2_t   vCmplxReal = __riscv_vget_v_f32m2x2_f32m2(vCmplx, 0);
        vfloat32m2_t   vCmplxImag = __riscv_vget_v_f32m2x2_f32m2(vCmplx, 1);
        vCmplxReal = __riscv_vfmul_vv_f32m2(vCmplxReal, vGain, vl);
        vCmplxImag = __riscv_vfmul_vv_f32m2(vCmplxImag, vGain, vl);
        vfloat32m2x2_t vCmplxOut = __riscv_vcreate_v_f32m2x2(vCmplxReal, vCmplxImag);
        __riscv_vsseg2e32_v_f32m2x2(pCmplx, vCmplxOut, vl);

        pSrcReal += vl;
        pCmplx   += vl * 2;
        len      -= vl;
    }

    st->ft[0] = MULT16_16_P15(st->gain2[0], st->ft[0]);
    st->ft[2 * N - 1] = MULT16_16_P15(st->gain2[N - 1], st->ft[2 * N - 1]);
}

#endif

#ifdef OVERRIDE_ANR_UPDATE_NOISE_PROB
VISIB_ATTR void update_noise_prob(SpeexPreprocessState * st)
{
    int             min_range;
    int             N = st->ps_size;

    float      *pS = st->S + 1;
    float      *pPsBase = st->ps; 

    int32_t         blockSize = N - 2;
    float       c0 = QCONST16(.8f, 15);
    float       c1 = QCONST16(.05f, 15);
    float       c2 = QCONST16(.1f, 15);
    float       c3 = QCONST16(.05f, 15);

    float psPrv = pPsBase[0];
    float psNxt;

    size_t vlmax = __riscv_vsetvlmax_e32m2();
    vint32m2_t vIZero = __riscv_vmv_v_x_i32m2(0, vlmax);
    vfloat32m2_t vFZero = __riscv_vfmv_v_f_f32m2(.0f, vlmax);

    while(blockSize > 0){

        size_t vl = __riscv_vsetvl_e32m2(blockSize);

        vfloat32m2_t vecS  = __riscv_vle32_v_f32m2(pS, vl);
        vfloat32m2_t vecPS = __riscv_vle32_v_f32m2(pPsBase + 1, vl);

        psNxt = pPsBase[1 + vl];
        vfloat32m2_t vecPSM1 = __riscv_vfslide1up_vf_f32m2(vecPS, psPrv, vl);
        vfloat32m2_t vecPSP1 = __riscv_vfslide1down_vf_f32m2(vecPS, psNxt, vl);

        vfloat32m2_t vecTmp     = __riscv_vfmul_vf_f32m2(vecS, c0, vl);
        vecTmp = __riscv_vfmacc_vf_f32m2(vecTmp, c2, vecPS, vl);
        vfloat32m2_t vecPsSides = __riscv_vfadd_vv_f32m2(vecPSM1, vecPSP1, vl);
        vecTmp = __riscv_vfmacc_vf_f32m2(vecTmp, c1, vecPsSides, vl);

        __riscv_vse32_v_f32m2(pS, vecTmp, vl);

        psPrv = pPsBase[vl];

        pS        += vl;
        pPsBase   += vl;
        blockSize -= vl;
    }

    st->S[0] = MULT16_32_Q15(QCONST16(.8f, 15), st->S[0]) + MULT16_32_Q15(QCONST16(.2f, 15), st->ps[0]);
    st->S[N - 1] = MULT16_32_Q15(QCONST16(.8f, 15), st->S[N - 1]) + MULT16_32_Q15(QCONST16(.2f, 15), st->ps[N - 1]);


    if (st->nb_adapt == 1) {
        float *pSmin = st->Smin;
        float *pStmp = st->Stmp;
        size_t len = N;
        while (len > 0) {
            size_t vl = __riscv_vsetvl_e32m2(len);
            __riscv_vse32_v_f32m2(pSmin, vFZero, vl);
            __riscv_vse32_v_f32m2(pStmp, vFZero, vl);
            pSmin += vl;
            pStmp += vl;
            len   -= vl;
        }
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
        float      *pSmin = st->Smin;
        float      *pStmp = st->Stmp;
        pS = st->S;
        size_t len = N;
        while(len > 0){
            size_t vl = __riscv_vsetvl_e32m2(len);
            vfloat32m2_t vS    = __riscv_vle32_v_f32m2(pS, vl);
            vfloat32m2_t vSmin = __riscv_vle32_v_f32m2(pStmp, vl);
            vSmin = __riscv_vfmin_vv_f32m2(vSmin, vS, vl);
            __riscv_vse32_v_f32m2(pSmin, vSmin, vl);
            __riscv_vse32_v_f32m2(pStmp, vS, vl);
            pS    += vl;
            pSmin += vl;
            pStmp += vl;
            len   -= vl;
        }
    } else {
        float      *pSmin = st->Smin;
        float      *pStmp = st->Stmp;
        pS = st->S;
        size_t len = N;
        while(len > 0){
            size_t vl = __riscv_vsetvl_e32m2(len);

            vfloat32m2_t vS    = __riscv_vle32_v_f32m2(pS, vl);
            vfloat32m2_t vSmin = __riscv_vle32_v_f32m2(pSmin, vl);
            vSmin = __riscv_vfmin_vv_f32m2(vSmin, vS, vl);
            __riscv_vse32_v_f32m2(pSmin, vSmin, vl);
            vfloat32m2_t vStmp = __riscv_vle32_v_f32m2(pStmp, vl);
            vStmp = __riscv_vfmin_vv_f32m2(vStmp, vS, vl);
            __riscv_vse32_v_f32m2(pStmp, vStmp, vl);
            pS    += vl;
            pSmin += vl;
            pStmp += vl;
            len   -= vl;
        }

    }

    float       c = QCONST16(.4f, 15);
    float      *pSMin = st->Smin;
    int32_t        *pProb = (int32_t *)st->update_prob;

    pS = st->S;
    size_t len = N;
    while(len > 0){
        size_t vl = __riscv_vsetvl_e32m2(len);
        vfloat32m2_t vecS    = __riscv_vle32_v_f32m2(pS, vl);
        vfloat32m2_t vecSmin = __riscv_vle32_v_f32m2(pSMin, vl);
        vecS = __riscv_vfmul_vf_f32m2(vecS, c, vl);
        vbool16_t vMask = __riscv_vmfgt_vv_f32m2_b16(vecS, vecSmin, vl);

        vint32m2_t vecProb = __riscv_vmerge_vxm_i32m2(vIZero, 1, vMask, vl);
        __riscv_vse32_v_i32m2(pProb, vecProb, vl);
        pProb += vl;
        pS    += vl;
        pSMin += vl;
        len   -= vl;
    }

}

#endif

#else

/* FIXED_POINT not needed for EEMBC AudioMark */
#error "Fixed Point Preprocess Vector Optimization is not available"

#endif
