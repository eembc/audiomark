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

#include "public.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "speex_preprocess.h"
#include "arch.h"
#include "speex_echo.h"


#ifdef OS_SUPPORT_CUSTOM
#ifdef FIXED_POINT
    #define XPH_AEC_INSTANCE_SIZE 45000
#else
    #define XPH_AEC_INSTANCE_SIZE 68100
#endif
    extern char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
    extern long cumulatedMalloc;
#endif

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
            
            usage arm_beamformer_f32(NODE_MEMREQ, 
                    **data requirement pointer,  
                    0, 
                    parameters used for memory needed
        */
        case NODE_MEMREQ:
        {   
            #ifdef OS_SUPPORT_CUSTOM
            *(uint32_t *)(*instance) = XPH_AEC_INSTANCE_SIZE;
            #else
            *(uint32_t *)(*instance) = 0;
            #endif
            break;
        }


        /* usage arm_beamformer_f32(NODE_RESET, 
                    **instance pointer,  can be modified
                    0, 
                    parameters, default configuration index + patch parameters
        */
        case NODE_RESET: 
        {   
            uint32_t nn, tail, fs, configuration_index;
            SpeexEchoState *ptr_intance;

            #ifdef OS_SUPPORT_CUSTOM
            /* memory alignment on 32bits */
            spxGlobalHeapPtr = (char *)(((PTR_INT)(*instance) + 3) & ~3);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_AEC_INSTANCE_SIZE;
            #endif
            configuration_index = *(uint32_t *)parameters;
            nn   = param_aec_f32[configuration_index][0];
            tail = param_aec_f32[configuration_index][1];
            fs   = param_aec_f32[configuration_index][2];

            ptr_intance = speex_echo_state_init(nn, tail);
            speex_echo_ctl(ptr_intance, SPEEX_ECHO_SET_SAMPLING_RATE, &Fs);
            *(void **)instance = ptr_intance;
            break;
        }


        /* usage arm_beamformer_f32(NODE_RUN, 
                    *instance pointer,  
                    pointer to pairs of (ptr,size) 
                    execution parameters 
        */                                       
        case NODE_RUN:       
        {
            PTR_INT *pt_pt=0;
            uint32_t buffer_size;
            int32_t nb_input_samples;
            int16_t *reference=0, *echo=0, *outBufs;

            /* parameter points to input { (*,n),(*,n),..} updated at the end */

            pt_pt = (PTR_INT *)data;
            reference = (int16_t *)(*pt_pt++);
            buffer_size = (uint32_t )(*pt_pt++);
            echo = (int16_t *)(*pt_pt++);
            buffer_size = (uint32_t )(*pt_pt++);
            outBufs = (int16_t *)(*pt_pt++); 

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
