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


#include <string.h>
#include "public.h"
#include "arm_math_types.h"
#include "dsp/transform_functions.h"
#include "dsp/statistics_functions.h"
#include "dsp/support_functions.h"

#include "arm_beamformer_f32.h"

adapBF_f32_fastdata_prms_t bf_prms;
adapBF_f32_fastdata_static_t bf_mem;

/*
   adative beamforomer subroutine
*/
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
    for (int i = 0; i < (NFFT/2) - 1; i++) {
        adap_cmplx_out_pt[2 * i + NFFT + 2] = adap_cmplx_out_pt[NFFT - 2*i - 2];
        adap_cmplx_out_pt[1 + 2 * i + NFFT + 2] = -adap_cmplx_out_pt[NFFT - 2*i - 1];
    }
}

/*
   swc_run - "the" subroutine  ------------------------------------------
*/
void arm_beamformer_f32_run(beamformer_f32_instance *instance, 
                        int16_t* input_buffer_left, int16_t* input_buffer_right, int32_t input_buffer_size, 
                        int16_t* output_buffer, int32_t *input_samples_consumed, int32_t *output_samples_produced, 
                        int32_t* returned_state)
{
    int32_t input_index, ilag, i;
    float *pf32_1, *pf32_2, *pf32_3, *pf32_out, ftmp, R, C;
    beamformer_f32_instance *pinstance = (beamformer_f32_instance *)instance;

    input_index = 0;
    while (input_index + NFFTD2 <= input_buffer_size)
    {
    /* datachunkLeft = [old_left ; inputLeft]; old_left = inputLeft;
       datachunkRight = [old_right ; inputRight]; old_right = inputRight;
       X0 = fft(datachunkLeft );
       Y0 = fft(datachunkRight); 
    */
    pf32_1 = pinstance->st->old_left; pf32_2 = pinstance->w->X0; /* old samples -> mic[0..NFFTD2[ */
    for (i = 0; i < NFFTD2*COMPLEX; i++)
    {   *pf32_2++ = *pf32_1++;
    }
    pf32_3 = pinstance->w->CY0;     /* temporary buffer */
    arm_q15_to_float(&(input_buffer_left[input_index]), pf32_3, NFFTD2);

    pf32_1 = pinstance->st->old_left;   /* save samples for next frame */
    for (i = 0; i < NFFTD2*REAL; i++)
    {   *pf32_1++ = *pf32_2++ = *pf32_3++;
        *pf32_1++ = *pf32_2++ = 0;
    }
    pf32_2 = pinstance->w->X0; 
    arm_cfft_f32(&((pinstance->st)->cS), pf32_2, 0, 1);   

    /* Right microphone */
    pf32_1 = pinstance->st->old_right; pf32_2 = pinstance->w->Y0; /* old samples -> mic[0..NFFTD2[ */
    for (i = 0; i < NFFTD2*COMPLEX; i++)
    {   *pf32_2++ = *pf32_1++;
    }
    pf32_3 = pinstance->w->CY0;     /* temporary buffer */
    arm_q15_to_float(&(input_buffer_right[input_index]), pf32_3, NFFTD2);

    pf32_1 = pinstance->st->old_right;   /* save samples for next frame */
    for (i = 0; i < NFFTD2*REAL; i++)
    {   *pf32_1++ = *pf32_2++ = *pf32_3++;
        *pf32_1++ = *pf32_2++ = 0;
    }
    pf32_2 = pinstance->w->Y0; 
    arm_cfft_f32(&((pinstance->st)->cS), pf32_2, 0, 1); 
    
    /* XY = X0(HalfRange) .* conj(Y0(HalfRange));
    */
    pf32_1 = pinstance->w->Y0; pf32_2 = pinstance->w->CY0;
    arm_cmplx_conj_f32(pf32_1, pf32_2, NFFTD2);

    pf32_1 = pinstance->w->X0;
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
    for (i = 0; i < NFFT; i++) {
        ftmp = *pf32_1++;
        if (ftmp == 0) continue;
        *pf32_2++ /= ftmp;  // real part
        *pf32_2++ /= ftmp;  // imaginary part
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
    pf32_out = pinstance->w->allDerot;
    for (ilag = 0; ilag < LAGSTEP; ilag++)
    {   pf32_3 = pinstance->w->CY0;     /* ZZ in Matlab reference */
        arm_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_3, NFFTD2);
        ftmp = 0;
        for (i = 0; i < NFFTD2; i++) {
            ftmp += (*pf32_3); 
            pf32_3 +=2;             /* sum of the real */
        }
        *pf32_out++ = ftmp;
        pf32_1 += NFFT*COMPLEX;     /* next rotation vector */
    }
    pf32_out = pinstance->w->allDerot;
    arm_absmax_f32(pf32_out, NFFTD2, &(pinstance->w->corr), &(pinstance->w->icorr));

    /* SYNTHESIS
       wrot2 = wrot(fixed_lag,:);
       NewSpectrum = X0 + Y0.*wrot2.'; 
    */
    pf32_1 = pinstance->wrot + (FIXED_DIRECTION * NFFT);
    pf32_2 = pinstance->w->Y0;
    pf32_out = pinstance->w->XY;    /* temporary buffer Y0.*wrot2 */
    arm_cmplx_mult_cmplx_f32(pf32_1, pf32_2, pf32_out, NFFT);

    pf32_1 = pinstance->w->X0;
    pf32_2 = pinstance->w->XY;
    pf32_out = pinstance->w->BF;                            /* (X0 + Y0.*wrot2.') = fix_bf_out() */
    arm_add_f32(pf32_1, pf32_2, pf32_out, NFFT*COMPLEX);    
    pf32_out = pinstance->w->BM;                            /* (X0 - Y0.*wrot2.') = fix_bm_out() */
    arm_sub_f32(pf32_1, pf32_2, pf32_out, NFFT*COMPLEX);    

    /* Synthesis = 0.5*hann(NFFT) .* real(ifft(NewSpectrum));       
       Synthesis_adap = w_hann .* real(ifft(NewSpectrum_adap));

      with NewSpectrum_adap = adapBF(BF, BM, out, states, mem);
    */
    pf32_1 = pinstance->w->BF;
    pf32_2 = pinstance->w->BM;
    pf32_out = pinstance->w->CY0;  
    adapBF(pf32_1, pf32_2, pf32_out, bf_prms, &bf_mem);     /* CY0 = synthesis spectrum */

    pf32_2 = pinstance->w->CY0; 
    arm_cfft_f32(&((pinstance->st)->cS), pf32_2, 1, 1);     /* in-place processing */

    pf32_1 = pf32_2;
    for (i = 0; i < NFFT; i++)      /* extract the real part */
    {   *pf32_1++ = *pf32_2; 
        pf32_2 += COMPLEX;
    }

    pf32_1 = pinstance->w->CY0;     /* apply the Hanning window */
    pf32_2 = pinstance->window;   
    pf32_3 = pinstance->w->CY0;     /* hanning window temporary */
    arm_mult_f32(pf32_1, pf32_2, pf32_3, NFFTD2);

    pf32_1 = pinstance->st->ola_new;/* overlap and add with the previous buffer */    
    pf32_2 = pinstance->w->CY0;
    arm_add_f32(pf32_1, pf32_2, pf32_2, NFFTD2);
    arm_float_to_q15(pf32_2, output_buffer, NFFTD2);

    pf32_1 = pinstance->w->CY0 + NFFTD2;
    pf32_2 = pinstance->window + NFFTD2;   
    pf32_3 = pinstance->st->ola_new;
    arm_mult_f32(pf32_1, pf32_2, pf32_3, NFFTD2);

    input_index += NFFTD2;         /* number of samples used in the input buffer */
    output_buffer += NFFTD2;
    }

    *input_samples_consumed = input_index;  
    *output_samples_produced = input_index;
    *returned_state = 0;
}
