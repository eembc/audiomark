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
#include "../../speexdsp/include/speex/speex_echo.h"

#define XPH_AEC_INSTANCE_SIZE 45000

// for libspeex malloc
extern char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
extern uint32_t cumulatedMalloc;

const uint32_t param_aec_f32[1][3] = {{
    256,   // NN
    1024,  // TAIL
    16000, // FS
},};

/*
   link wrapper to the float32 beamformer --------------------------------------------------------------------
*/
int32_t xiph_libspeex_aec_f32 (int32_t command, void **instance, void *data, void *parameters)
{
    int32_t swc_returned_status = 0, Fs=16000;

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
            *(uint32_t *)(*instance) = XPH_AEC_INSTANCE_SIZE;
            break;
        }


        /* usage arm_beamformer_f32(_NODE_RESET, 
                    **instance pointer,  can be modified
                    0, 
                    parameters used use-case tuning 
        */
        case _NODE_RESET: 
        {   
            uint32_t nn, tail, fs, configuration_index;
            SpeexEchoState *ptr_intance;

            /* memory alignment on 32bits */
            spxGlobalHeapPtr = (char *)(((uint32_t)(*instance) + 3) & ~3);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_AEC_INSTANCE_SIZE;

            configuration_index = *(uint32_t *)parameters;
            nn   = param_aec_f32[configuration_index][0];
            tail = param_aec_f32[configuration_index][1];
            fs   = param_aec_f32[configuration_index][2];

            ptr_intance = speex_echo_state_init(nn, tail);
            speex_echo_ctl(ptr_intance, SPEEX_ECHO_SET_SAMPLING_RATE, &Fs);
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
            uint8_t *reference=0, *echo=0, *outBufs;

            /* parameter points to input { (*,n),(*,n),..} updated at the end */

            pt_pt = (uint32_t *)data;
            reference = (uint8_t *)(*pt_pt++);
            buffer_size = (uint32_t )(*pt_pt++);
            echo = (uint8_t *)(*pt_pt++);
            buffer_size = (uint32_t )(*pt_pt++);
            outBufs = (uint8_t *)(*pt_pt++); 

            nb_input_samples = buffer_size / sizeof(int16_t);
            speex_echo_cancellation((SpeexEchoState *)*instance, 
                                    (int16_t *)reference,  
                                    (int16_t *)echo,
                                    (int16_t *)outBufs);
            break;
        }
    }
    return swc_returned_status;
}

