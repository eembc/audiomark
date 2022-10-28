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
#include "os_support.h"
#endif

#include "speex_preprocess.h"
#include "arch.h"

#ifdef OS_SUPPORT_CUSTOM
#ifdef FIXED_POINT
    #define XPH_ANR_INSTANCE_SIZE 29000
#else
    #define XPH_ANR_INSTANCE_SIZE 45250
#endif
    extern char* spxGlobalHeapPtr, * spxGlobalHeapEnd;
    extern long cumulatedMalloc;
#endif

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
        case NODE_MEMREQ:
        {   
            #ifdef OS_SUPPORT_CUSTOM
            *(uint32_t *)(*instance) = XPH_ANR_INSTANCE_SIZE;
            #else
            *(uint32_t *)(*instance) = 0;
            #endif
            break;
        }
        case NODE_RESET: 
        {   
            uint32_t nn, fs, configuration_index;
            SpeexPreprocessState *ptr_intance;

            #ifdef OS_SUPPORT_CUSTOM
            /* memory alignment on 32bits */
            spxGlobalHeapPtr = (char *)(((PTR_INT)(*instance) + 3) & ~3);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_ANR_INSTANCE_SIZE;
            #endif

            configuration_index = *(uint32_t *)parameters;
            nn = param_anr_f32[configuration_index][0];
            fs = param_anr_f32[configuration_index][1];

            ptr_intance = speex_preprocess_state_init(nn, fs);
            speex_preprocess_ctl(ptr_intance, SPEEX_PREPROCESS_SET_ECHO_STATE, 0);
            *(void **)instance = ptr_intance;

            break;
        }
        case NODE_RUN:       
        {
            PTR_INT *pt_pt=0;
            uint32_t buffer_size;
            int32_t nb_input_samples;
            int16_t *in_place_buffer=0;

            /* parameter points to input { (*,n),(*,n),..} updated at the end */

            pt_pt = (PTR_INT *)data;
            in_place_buffer = (int16_t *)(*pt_pt++);
            buffer_size =     (uint32_t )(*pt_pt++);
            nb_input_samples = buffer_size / sizeof(int16_t);
            speex_preprocess_run((SpeexPreprocessState *)*instance, 
                                 (int16_t *)in_place_buffer);
            break;
        }
    }
    return swc_returned_status;
}
