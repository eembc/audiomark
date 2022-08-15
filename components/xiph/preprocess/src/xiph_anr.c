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
  * Title:        SWC_SRC.c
  * Description:  software component for rate conversion

  * $Date:        May 2022
  * $Revision:    V.0.0.1
  *
  * Target Processor:  Cortex-M cores
  * -------------------------------------------------------------------- */


#include "../../../../application_demo/public.h"

#include "../../speexdsp/include/speex/config.h"
#include "../../speexdsp/include/speex/speex_preprocess.h"
#include "../../speexdsp/libspeexdsp/arch.h"

#define XPH_ANR_INSTANCE_SIZE 29000


// for libspeex malloc
extern char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
extern uint32_t cumulatedMalloc;

const uint32_t param_anr_f32[1][2] = {{
    256,   // NN
    16000, // FS
},};

/*
   link wrapper to the float32 beamformer --------------------------------------------------------------------
*/
int32_t xiph_libspeex_anr_f32 (int32_t command, void **instance, void *data, void *parameters)
{
    int32_t swc_returned_status = 0;

    switch (command)
    {
        /*  return the memory requirements of xiph_libspeex_anr_f32
            
            usage arm_beamformer_f32(_NODE_MEMREQ, 
                    **data requirement pointer,  
                    0, 
                    parameters used for memory needed
        */
        case _NODE_MEMREQ:
        {   
            *(uint32_t *)(*instance) = XPH_ANR_INSTANCE_SIZE;
            break;
        }


        /* usage arm_beamformer_f32(_NODE_RESET, 
                    **instance pointer,  can be modified
                    0, 
                    parameters used use-case tuning 
        */
        case _NODE_RESET: 
        {   
            uint32_t nn, fs, configuration_index;
            SpeexPreprocessState *ptr_intance;

            /* memory alignment on 32bits */
            spxGlobalHeapPtr = (char *)(((uint32_t)(*instance) + 3) & ~3);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_ANR_INSTANCE_SIZE;

            configuration_index = *(uint32_t *)parameters;
            nn = param_anr_f32[configuration_index][0];
            fs = param_anr_f32[configuration_index][1];

            ptr_intance = speex_preprocess_state_init(nn, fs);
            speex_preprocess_ctl(ptr_intance, SPEEX_PREPROCESS_SET_ECHO_STATE, 0);
            *(void **)instance = ptr_intance;

            break;
        }

        /* usage arm_beamformer_f32(_NODE_RUN, 
                    *instance pointer,  
                    pointer to pairs of (ptr,size) 
                    execution parameters 
        */ 
        case _NODE_RUN:       
        {
            uint32_t buffer_size, *pt_pt=0;
            int32_t nb_input_samples;
            uint8_t *in_place_buffer=0;

            /* parameter points to input { (*,n),(*,n),..} updated at the end */

            pt_pt = (uint32_t *)data;
            in_place_buffer = (uint8_t *)(*pt_pt++);
            buffer_size =     (uint32_t )(*pt_pt++);
            nb_input_samples = buffer_size / sizeof(int16_t);
            speex_preprocess_run((SpeexPreprocessState *)*instance, 
                                 (int16_t *)in_place_buffer);
            break;
        }
    }
    return swc_returned_status;
}

