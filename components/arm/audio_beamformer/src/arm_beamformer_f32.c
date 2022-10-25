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
  * Title:        arm_beamformer_f32.c
  * Description:  software component for beam forming

  * $Date:        May 2022
  * $Revision:    V.0.0.1
  *
  * Target Processor:  Cortex-M cores
  * -------------------------------------------------------------------- */

#include "public.h"
#include "arm_beamformer_f32.h"     /* definition of struct beamformer_f32_instance */

beamformer_f32_instance bf_instance;

/* Fast coefficient structure of the instance */
beamformer_f32_fastdata_static_t beamformer_f32_fastdata_static;
beamformer_f32_fastdata_working_t beamformer_f32_fastdata_working;

/*
   swc_run - "the" subroutine  ------------------------------------------
*/
extern void arm_beamformer_f32_run(beamformer_f32_instance *instance, 
    int16_t* input_buffer_left, int16_t* input_buffer_right, int32_t input_buffer_size, 
    int16_t* output_buffer, int32_t *input_samples_consumed, int32_t *output_samples_produced, 
    int32_t* returned_state);

/*
   reset instance ------------------------------------------
*/
void arm_beamformer_f32_reset(void)
{
    int i;
    extern const float w_hanning_div2[];
    extern const float rotation[];
    
    bf_instance.window = (float *)w_hanning_div2;
    bf_instance.wrot = (float *)rotation;
    bf_instance.st = &beamformer_f32_fastdata_static;
    bf_instance.w = &beamformer_f32_fastdata_working;

    for (i = 0; i < NFFT; i++)                         /* reset static buffers */
    {   bf_instance.st->old_left[i] = 0;
        bf_instance.st->old_right[i] = 0;
    }
    for (i = 0; i < NFFTD2; i++)             
    {   bf_instance.st->ola_new[i] = 0;
    }

    arm_rfft_fast_init_f32(&((bf_instance.st)->rS), NFFT);  /* init rFFT tables */
    arm_cfft_init_f32(&((bf_instance.st)->cS), NFFT);       /* init cFFT tables */

    adapBF_init(&bf_prms, &bf_mem);                     /* adaptive filter reset */
    bf_mem.GSC_det_avg = 0; //0.94083f;
    bf_mem.adptBF_coefs_update_enable = 1;
}

/*
   wrapper to the float32 beamformer --------------------------------------------------------------------
*/
int32_t arm_beamformer_f32(int32_t command, void **instance, void *data, void *parameters)
{
    int32_t swc_returned_status = 0;

    switch (command)
    {
        /* usage arm_beamformer_f32(NODE_MEMREQ, 
                    **instance pointer,  will be mo
                    0, 
                    parameters[] = 
        */
        case NODE_MEMREQ:
        {   // static variables
            *(uint32_t *)(*instance)= 0;
            break; 
        }
        /* usage arm_beamformer_f32(NODE_RESET, 
                    **instance pointer,  will be mo
                    0, 
                    parameters[] = 
        */
        case NODE_RESET: 
        {   
            arm_beamformer_f32_reset();
            break;
        }
                                      
        
        /* usage arm_beamformer_f32(NODE_RUN, 
                    *instance pointer,  
                    pointer to pairs of (ptr,size) 
                    execution parameters 
        */ 
        case NODE_RUN:     /* func(LINKNODE_RUN, instance, ptr to data, 0) */         
        {
            PTR_INT *pt_pt=0;
            uint32_t buffer1_size, buffer2_size;
            int32_t nb_input_samples, input_samples_consumed, output_samples_produced;
            uint8_t *inBufs1stChannel=0, *inBufs2ndChannel=0, *outBufs;

            /* parameter points to input { (*,n),(*,n),..} */

            pt_pt = (PTR_INT *)data;
            inBufs1stChannel = (uint8_t *)(*pt_pt++);
            buffer1_size =     (uint32_t )(*pt_pt++);
            inBufs2ndChannel = (uint8_t *)(*pt_pt++);
            buffer2_size =     (uint32_t )(*pt_pt++);
            outBufs = (uint8_t *)(*pt_pt++); 

            nb_input_samples = buffer1_size / sizeof(int16_t);
            arm_beamformer_f32_run(&bf_instance, (int16_t *)inBufs1stChannel,  
                                         (int16_t *)inBufs2ndChannel, nb_input_samples, 
                                         (int16_t *)outBufs, 
                                         &input_samples_consumed, 
                                         &output_samples_produced, 
                                         &swc_returned_status);
            break;
        }
    }
    return swc_returned_status;
}
