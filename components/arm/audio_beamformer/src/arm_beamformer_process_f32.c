/*
 * Copyright (C) 2022 Arm Limited or its affiliates. All rights reserved.
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

 /* ----------------------------------------------------------------------
  * Project:      Arm C300 Voice Demo Front end
  * Title:        arm_beamformer.c
  * Description:  software component for beam forming

  * $Date:        May 2022
  * $Revision:    V.0.0.1
  *
  * Target Processor:  Cortex-M cores
  * -------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "public.h"
#include "arm_math_types.h"
#include "dsp/transform_functions.h"
#include "dsp/statistics_functions.h"
#include "dsp/support_functions.h"
#include "dsp/complex_math_functions.h"

#include "arm_beamformer_f32.h"

/*
   adative beamfromer subroutine
*/
#define LEN_BM_ADF 6

typedef struct adapBF_f32_fastdata_prms_t
{
    float32_t alpha_BM_NLMS;
    float32_t DS_DET_TH;
    float32_t ep_GSC;
} adapBF_f32_fastdata_prms_t;

typedef struct adapBF_f32_fastdata_static_t
{
    float32_t states_BM_ADF[NFFT/2 + 1][LEN_BM_ADF*2];
    float32_t coefs_BM_ADF[NFFT/2 + 1][LEN_BM_ADF*2];
    float32_t Norm_out_BM[NFFT/2 + 1];
    float32_t lookBF_out[NFFT/2 + 1];
    float32_t GSC_det_avg;
    uint8_t adptBF_coefs_update_enable;
} adapBF_f32_fastdata_static_t;

void adapBF_init(adapBF_f32_fastdata_prms_t* bf_prms, adapBF_f32_fastdata_static_t* bf_mem)
{
    bf_prms->alpha_BM_NLMS = 0.01f;
    bf_prms->DS_DET_TH = 0.2f;
    bf_prms->ep_GSC = 1e-12f;

    for (int i = 0; i < NFFT/2 + 1; i++)
    {
        for (int j = 0; j < LEN_BM_ADF * 2; j++) {
            bf_mem->states_BM_ADF[i][j] = 0;
            bf_mem->coefs_BM_ADF[i][j] = 0;
        }
        bf_mem->Norm_out_BM[i] = 0;
        bf_mem->lookBF_out[i] = 0;
    }
    bf_mem->GSC_det_avg = 0;
    bf_mem->adptBF_coefs_update_enable = 0;
}

void adapBF(float32_t *bf_cmplx_in_pt, float32_t *bm_cmplx_in_pt, float32_t* adap_cmplx_out_pt, adapBF_f32_fastdata_prms_t bf_prms, adapBF_f32_fastdata_static_t *bf_mem)
{
    float32_t adap_out[2], error_out[2];
    float32_t temp[LEN_BM_ADF*2];
    float32_t sum0=0.0f, sum1=0.0f;

    //Update delay line for reference signal
    for (int i = 0; i < NFFT/2 + 1; i++)
    {
        for (int j = (LEN_BM_ADF * 2 - 3); j >= 0;  j--) {
            bf_mem->states_BM_ADF[i][j+2] = bf_mem->states_BM_ADF[i][j];
        }
    }
    for (int i = 0; i < NFFT/2 + 1; i++) {
        bf_mem->states_BM_ADF[i][0] = bm_cmplx_in_pt[2*i];
        bf_mem->states_BM_ADF[i][1] = bm_cmplx_in_pt[2*i+1];
    }

    for (int i = 0; i < NFFT/2 + 1; i++) {
        //adaptive filter
        arm_cmplx_conj_f32(&bf_mem->coefs_BM_ADF[i][0], temp, LEN_BM_ADF);
        arm_cmplx_dot_prod_f32(temp, &bf_mem->states_BM_ADF[i][0], LEN_BM_ADF, &adap_out[0], &adap_out[1]);
        //calculate error
        error_out[0] = bf_cmplx_in_pt[2*i] - adap_out[0];
        error_out[1] = bf_cmplx_in_pt[2*i + 1] - adap_out[1];
        if (bf_mem->adptBF_coefs_update_enable && bf_mem->GSC_det_avg > bf_prms.DS_DET_TH) { //update adptBF coefficients
            float tmp = bf_prms.alpha_BM_NLMS / (bf_mem->Norm_out_BM[i] + bf_prms.ep_GSC);
            for (int j = 0; j < LEN_BM_ADF * 2; j += 2) {
                bf_mem->coefs_BM_ADF[i][j] += tmp * (error_out[0] * bf_mem->states_BM_ADF[i][j] + error_out[1] * bf_mem->states_BM_ADF[i][j + 1]);
                bf_mem->coefs_BM_ADF[i][j + 1] += tmp * (error_out[0] * bf_mem->states_BM_ADF[i][j + 1] - error_out[1] * bf_mem->states_BM_ADF[i][j]);
            }
        }
        adap_cmplx_out_pt[2 * i] = error_out[0];
        adap_cmplx_out_pt[2 * i + 1] = error_out[1];
        bf_mem->Norm_out_BM[i] = 0.9f * bf_mem->Norm_out_BM[i] + 0.1f * (bm_cmplx_in_pt[2 * i] * bm_cmplx_in_pt[2 * i] + bm_cmplx_in_pt[2 * i + 1] * bm_cmplx_in_pt[2 * i + 1]);
        sum0 += bf_mem->Norm_out_BM[i];
        bf_mem->lookBF_out[i] = 0.9f * bf_mem->lookBF_out[i] + 0.1f * (bf_cmplx_in_pt[2 * i] * bf_cmplx_in_pt[2 * i] + bf_cmplx_in_pt[2 * i + 1] * bf_cmplx_in_pt[2 * i + 1]);
        sum1 += bf_mem->lookBF_out[i];
    }
    //update GSC_det_avg
    bf_mem->GSC_det_avg = 0.9f * bf_mem->GSC_det_avg + 0.1f * sum0 / (sum1 + bf_prms.ep_GSC);
    if (bf_mem->GSC_det_avg > 2.0f) bf_mem->GSC_det_avg = 2.0f;
    //generate output
    for (int i = 0; i < (NFFT/2) - 2; i++) {
        adap_cmplx_out_pt[2 * i + NFFT + 2] = adap_cmplx_out_pt[NFFT - 2*i - 2];
        adap_cmplx_out_pt[1 + 2 * i + NFFT + 2] = -adap_cmplx_out_pt[NFFT - 2*i - 1];
    }
}

#if 0   //Disabled since this is adapBF unit test code
float32_t bf_cmplx_in_pt[]={
#include "fix_bf_out.h"
};

float32_t bm_cmplx_in_pt[]={
#include "fix_bm_out.h"
};

float32_t states_BM_ADF[] = {
#include "states_BM_ADF.h"
};

float32_t states_BM_ADFout[] = {
#include "states_BM_ADFout.h"
};

float32_t coefs_BM_ADF[] = {
#include "coefs_BM_ADF.h"
};

float32_t coefs_BM_ADFout[] = {
#include "coefs_BM_ADFout.h"
};

float32_t Norm_out_BM[] = {
#include "Norm_out_BM.h"
};

float32_t Norm_out_BMout[] = {
#include "Norm_out_BMout.h"
};

float32_t lookBF_out[] = {
#include "lookBF_out.h"
};

float32_t lookBF_out1[] = {
#include "lookBF_out1.h"
};

float32_t adapBF_out[] = {
#include "adapBF_out.h"
};

int adapBF_test(void)
{
    adapBF_f32_fastdata_prms_t bf_prms;
    adapBF_f32_fastdata_static_t bf_mem;
    float32_t adap_cmplx_out_pt[2* NFFT];
    float32_t Scoef = 0, Ncoef = 0, S0 = 0, N0 = 0, S1 = 0, N1 = 0;

    adapBF_init(&bf_prms, &bf_mem);
    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        for (int j = 0; j < LEN_BM_ADF * 2; j++) {
            bf_mem.states_BM_ADF[i][j] = states_BM_ADF[i* LEN_BM_ADF * 2 + j];
            bf_mem.coefs_BM_ADF[i][j] = coefs_BM_ADF[i * LEN_BM_ADF * 2 + j];
        }
        bf_mem.Norm_out_BM[i] = Norm_out_BM[i];
        bf_mem.lookBF_out[i] = lookBF_out[i];
    }
    bf_mem.GSC_det_avg = 0.94083;
    bf_mem.adptBF_coefs_update_enable = 1;

    adapBF(bf_cmplx_in_pt, bm_cmplx_in_pt, adap_cmplx_out_pt, bf_prms, &bf_mem);

    for (int i = 0; i < NFFT / 2 + 1; i++)
    {
        for (int j = 0; j < LEN_BM_ADF * 2; j++) {
            if (bf_mem.states_BM_ADF[i][j] != states_BM_ADFout[i * LEN_BM_ADF * 2 + j]) {
                printf("i=%d, j=%d, states_BM_ADF not match!\n", i, j); // Should match otherwise error!
                return -1;
            }
        }
        //Collect signal and noise power compared with Matlab reference of adaptive coefficients
        for (int j = 0; j < LEN_BM_ADF; j=j+2) {
            float32_t s, n;
            s = coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j] * coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j] + coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j + 1] * coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j + 1];
            Scoef += s;
            n = (bf_mem.coefs_BM_ADF[i][j] - coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j]) * (bf_mem.coefs_BM_ADF[i][j] - coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j]) +
                (bf_mem.coefs_BM_ADF[i][j+1] - coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j+1]) * (bf_mem.coefs_BM_ADF[i][j+1] - coefs_BM_ADFout[i * LEN_BM_ADF * 2 + j+1]);
            Ncoef += n;
        }
        //Collect signal and noise power compared with Matlab reference of other state variables
        S0 += Norm_out_BMout[i] * Norm_out_BMout[i];
        N0 += (Norm_out_BMout[i] - bf_mem.Norm_out_BM[i]) * (Norm_out_BMout[i] - bf_mem.Norm_out_BM[i]);
        S1 += lookBF_out1[i] * lookBF_out1[i];
        N1 += (lookBF_out1[i] - bf_mem.lookBF_out[i]) * (lookBF_out1[i] - bf_mem.lookBF_out[i]);
    }

    //Calculate SNR compared with Matlab reference
    float32_t SNRcoef, SNR0, SNR1, SNRout;
    SNRcoef = 10.0f * log10f(Scoef / Ncoef);
    SNR0 = 10.0f * log10f(S0 / N0);
    SNR1 = 10.0f * log10f(S1 / N1);

    //Calculate output SNR compared with Matlab reference
    S0 = 0, N0 = 0;
    for (int i = 0; i < NFFT; i++)
    {
        S0 += adapBF_out[i] * adapBF_out[i];
        N0 += (adapBF_out[i] - adap_cmplx_out_pt[i]) * (adapBF_out[i] - adap_cmplx_out_pt[i]);
    }
    SNRout = 10.0f * log10f(S0 / N0);

    printf("SNRcoef=%f, SNR0=%f, SNR1=%f, SNRout=%f, GSC_det_avg=%f\n", SNRcoef, SNR0, SNR1, SNRout, bf_mem.GSC_det_avg);
    return 0;
}
#endif

void arm_beamformer_f32_run(swc_instance *instance, 
                        int16_t* input_buffer_left, int16_t* input_buffer_right, int32_t input_buffer_size, 
                        int16_t* output_buffer, int32_t *input_samples_consumed, int32_t *output_samples_produced, 
                        int32_t* returned_state)
{
    int i;
#if 1 // waiting the adaptive filter
    for (i = 0; i < input_buffer_size; i++)
        output_buffer[i] = input_buffer_left[i];
#else
    int32_t input_index, ilag;
    float *pf32_1, *pf32_2, *pf32_3, *pf32_out, ftmp;
    extern const arm_cfft_instance_f32 arm_cfft_sR_f32_len128;

    beamformer_f32_instance *pinstance = (beamformer_f32_instance *)instance;
    input_index = 0;

    while (input_index + NFFTD2 < input_buffer_size)
    {
    /* datachunkLeft = [old_left ; inputLeft]; old_left = inputLeft;
       datachunkRight = [old_right ; inputRight]; old_right = inputRight;
       X0 = fft(datachunkLeft );
       Y0 = fft(datachunkRight); 
    */
    pf32_1 = pinstance->st->old_left; pf32_1 += NFFTD2;
    arm_q15_to_float(&(input_buffer_left[input_index]),pf32_1,NFFTD2);

    pf32_1 = pinstance->st->old_right; pf32_1 += NFFTD2;
    arm_q15_to_float(&(input_buffer_right[input_index]),pf32_1,NFFTD2);

    pf32_1 = pinstance->st->old_left;
    arm_cfft_f32(&arm_cfft_sR_f32_len128, pf32_1, 0, 1);    /* in-place processing */

    pf32_1 = pinstance->st->old_right;
    arm_cfft_f32(&arm_cfft_sR_f32_len128, pf32_1, 0, 1);    /* in-place processing */
    
    /* XY = X0(HalfRange) .* conj(Y0(HalfRange));
    */
    pf32_1 = pinstance->st->old_right;
    pf32_out = pinstance->w->CY0;
    arm_cmplx_conj_f32(pf32_1, pf32_out, NFFTD2);

    pf32_1 = pinstance->st->old_left;
    pf32_2 = pinstance->w->CY0;
    pf32_out = pinstance->w->XY;
    arm_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_out, NFFTD2);

    /* PHATNORM = max(abs(XY), 1e-12);
    */
    pf32_1 = pinstance->w->XY;
    pf32_out = pinstance->w->PHATNORM;
    arm_cmplx_mag_f32(pf32_1, pf32_out, NFFTD2);

    /*  XY = XY ./ PHATNORM;
    */
    pf32_1 = pinstance->w->PHATNORM;
    pf32_2 = pinstance->w->XY;
    pf32_out = pinstance->w->XY;
    for (i = 0; i < NFFT; i++) {
        if ((ftmp = *pf32_1++) == 0) continue;
        *pf32_2++ /= ftmp;
    }

    /* for ilag = LagRange         % check all angles
        ZZ = XY .* wrot(idxLag,HalfRange).';
        allDerot(idxLag) = sum(real(ZZ));
        idxLag = idxLag+1;      % save all the beams
    end
    [corr, icorr] = max(allDerot); % keep the best one
    */
    pf32_1 = pinstance->wrot;
    pf32_2 = pinstance->w->XY;
    pf32_3 = pinstance->w->CY0; // ZZ
    pf32_out = pinstance->w->allDerot;
    for (ilag = 0; ilag < LAGSTEP; ilag++)
    {   arm_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_3, NFFTD2);
        pf32_3 = pinstance->w->CY0; 
        ftmp = 0;
        for (i = 0; i < NFFTD2; i++) {
            ftmp += (*pf32_3); 
            pf32_3 +=2;
        }
        *pf32_out++ = ftmp;
        pf32_1 += NFFT;
    }
    pf32_out -= LAGSTEP;
    arm_absmax_f32(pf32_out, NFFTD2, &(pinstance->w->corr), &(pinstance->w->icorr));

    /* SYNTHESIS
       wrot2 = wrot(fixed_lag,:);
       NewSpectrum = X0 + Y0.*wrot2.'; 
       Synthesis = 0.5*hann(NFFT) .* real(ifft(NewSpectrum));       
    */
    pf32_1 = pinstance->wrot + (FIXED_DIRECTION * NFFT);
    pf32_2 = pinstance->w->Y0;
    pf32_out = pinstance->w->XY;    /* temporary buffer */
    arm_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_out, NFFTD2);

    pf32_1 = pinstance->w->XY;
    pf32_2 = pinstance->w->X0;
    pf32_out = pinstance->w->XY;    /* new spectrum */
    arm_add_f32(pf32_1, pf32_2, pf32_out, NFFT);

    pf32_1 = pinstance->w->X0;
    arm_cfft_f32(&arm_cfft_sR_f32_len128, pf32_1, 1, 1); /* in-place processing */

    pf32_1 = (pinstance->w->Y0)+1;
    pf32_2 = (pinstance->w->Y0)+2;
    for (i = 1; i < NFFT; i++)      /* extract the real part */
    {   *pf32_1++ = *pf32_2; 
        pf32_2 += 2;
    }

    pf32_1 = pinstance->w->Y0;      /* apply the Hanning window */
    pf32_out = pinstance->window;   
    arm_mult_f32(pf32_1, pf32_2, pf32_1, NFFT);
    arm_float_to_q15(pf32_1, output_buffer, NFFTD2);

    input_index += NFFTD2;         /* number of samples used in the input buffer */
    }

    *input_samples_consumed = input_index;  
    *output_samples_produced = input_index;
#endif
    *returned_state = 0;
}
