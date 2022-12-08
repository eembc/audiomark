/* Copyright (C) 2006 Jean-Marc Valin */
/**
   @file filterbank.c
   @brief Converting between psd and filterbank
 */
/*
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


#ifdef GENERIC_ARCH

/*
 * Reference code for optimized routines
 */

#ifdef OVERRIDE_FB_COMPUTE_BANK32
void filterbank_compute_bank32(FilterBank * bank, spx_word32_t * ps, spx_word32_t * mel)
{
    int             i;
    for (i = 0; i < bank->nb_banks; i++)
        mel[i] = 0;

    for (i = 0; i < bank->len; i++) {
        int             id;
        id = bank->bank_left[i];
        mel[id] += MULT16_32_P15(bank->filter_left[i], ps[i]);
        id = bank->bank_right[i];
        mel[id] += MULT16_32_P15(bank->filter_right[i], ps[i]);
    }
    /* Think I can safely disable normalisation that for fixed-point (and probably float as well) */
#ifndef FIXED_POINT
    /*for (i=0;i<bank->nb_banks;i++)
       mel[i] = MULT16_32_P15(Q15(bank->scaling[i]),mel[i]);
     */
#endif
}
#endif

#ifdef OVERRIDE_FB_COMPUTE_PSD16
void filterbank_compute_psd16(FilterBank * bank, spx_word16_t * mel, spx_word16_t * ps)
{
    int             i;
    for (i = 0; i < bank->len; i++) {
        spx_word32_t    tmp;
        int             id1, id2;
        id1 = bank->bank_left[i];
        id2 = bank->bank_right[i];
        tmp = MULT16_16(mel[id1], bank->filter_left[i]);
        tmp += MULT16_16(mel[id2], bank->filter_right[i]);
        ps[i] = EXTRACT16(PSHR32(tmp, 15));
    }
}

#endif

/* ARM with Helium Extension */
#elif defined __ARM_FEATURE_MVE

#include <arm_mve.h>

#ifdef OVERRIDE_FB_COMPUTE_BANK32

#if !defined(FIXED_POINT)

void filterbank_compute_bank32(FilterBank * bank, spx_word32_t * ps, spx_word32_t * mel)
{
    int32_t         nb_banks = bank->nb_banks;
    int32_t         loopCnt;
    spx_word32_t    histoTmp[nb_banks * 4], *pHistoTmp;

    /* clear histoTmp */
    pHistoTmp = histoTmp;
    loopCnt = nb_banks * 4;
    do {
        mve_pred16_t    p = vctp32q(loopCnt);
        vstrwq_p_f32(pHistoTmp, vdupq_x_n_f32(0.0f, p), p);
        pHistoTmp += 4;
        loopCnt -= 4;
    }
    while (loopCnt > 0);

    /* each lane will be dispatched */
    /* into 4 differents histograms */
    uint32x4_t      histoOffset;
    histoOffset = vidupq_u32((uint32_t) 0, 1);
    histoOffset = histoOffset * nb_banks;


    uint32x4_t      offset0, offset1;
    uint32_t       *pL = (uint32_t *) bank->bank_left;
    uint32_t       *pR = (uint32_t *) bank->bank_right;
    float32x4_t     filtVec, psVec, in;
    spx_word32_t   *pfiltL = bank->filter_left;
    spx_word32_t   *pfiltR = bank->filter_right;
    spx_word32_t   *pPs = ps;

    loopCnt = bank->len;
    do {
        offset0 = vldrwq_u32(pL);
        offset0 = vaddq_u32(offset0, histoOffset);
        pL += 4;

        offset1 = vldrwq_u32(pR);
        offset1 = vaddq_u32(offset1, histoOffset);
        pR += 4;

        psVec = vldrwq_f32(pPs);

        in = vldrwq_gather_shifted_offset_f32(histoTmp, offset0);
        pPs += 4;
        filtVec = vldrwq_f32(pfiltL);
        pfiltL += 4;
        in = vfmaq(in, filtVec, psVec);
        vstrwq_scatter_shifted_offset_f32(histoTmp, offset0, in);


        in = vldrwq_gather_shifted_offset_f32(histoTmp, offset1);
        filtVec = vldrwq_f32(pfiltR);
        pfiltR += 4;
        in = vfmaq(in, filtVec, psVec);
        vstrwq_scatter_shifted_offset_f32(histoTmp, offset1, in);
        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        loopCnt -= 4;
    }
    while (loopCnt > 0);

    float32_t      *pHistoTmp0 = histoTmp;
    float32_t      *pHistoTmp1 = pHistoTmp0 + nb_banks;
    float32_t      *pHistoTmp2 = pHistoTmp1 + nb_banks;
    float32_t      *pHistoTmp3 = pHistoTmp2 + nb_banks;

    loopCnt = nb_banks;
    do {
        mve_pred16_t    p = vctp32q(loopCnt);
        float32x4_t     v0, v1, v2, v3, sum;

        v0 = vldrwq_z_f32(pHistoTmp0, p);
        v1 = vldrwq_z_f32(pHistoTmp1, p);
        v2 = vldrwq_z_f32(pHistoTmp2, p);
        v3 = vldrwq_z_f32(pHistoTmp3, p);

        sum = vaddq_x_f32(v0, v1, p);
        sum = vaddq_x_f32(sum, v2, p);
        sum = vaddq_x_f32(sum, v3, p);

        pHistoTmp0 += 4;
        pHistoTmp1 += 4;
        pHistoTmp2 += 4;
        pHistoTmp3 += 4;

        vstrwq_p_f32(mel, sum, p);
        mel += 4;

        loopCnt -= 4;
    }
    while (loopCnt > 0);
}

#endif
#endif


#ifdef OVERRIDE_FB_COMPUTE_PSD16

#if !defined(FIXED_POINT)

void filterbank_compute_psd16(FilterBank * bank, spx_word16_t * mel, spx_word16_t * ps)
{

    float32_t const *pbank_left = (float32_t const *) bank->bank_left;
    float32_t const *pbank_right = (float32_t const *) bank->bank_right;
    float32_t const *pfilter_left = (float32_t const *) bank->filter_left;
    float32_t const *pfilter_right = (float32_t const *) bank->filter_right;
    int             i;

    for (i = 0; i < bank->len / 4; i++) {
        uint32x4_t      vecId1, vecId2;
        float32x4_t     vecFilter;
        float32x4_t     vecMel;
        float32x4_t     vecTmp;

        vecId1 = vld1q(pbank_left);
        pbank_left += 4;

        vecId2 = vld1q(pbank_right);
        pbank_right += 4;


        vecMel = vldrwq_gather_shifted_offset(mel, vecId1);
        vecFilter = vld1q(pfilter_left);
        pfilter_left += 4;

        vecTmp = vmulq(vecMel, vecFilter);

        vecMel = vldrwq_gather_shifted_offset(mel, vecId2);
        vecFilter = vld1q(pfilter_right);
        pfilter_right += 4;

        vecTmp = vfmaq(vecTmp, vecMel, vecFilter);

        vst1q(ps, vecTmp);
        ps += 4;
    }
}
#endif

#endif
#endif
