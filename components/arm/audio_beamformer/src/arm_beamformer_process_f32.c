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

/*
   swc_run - "the" subroutine  ------------------------------------------
*/
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
