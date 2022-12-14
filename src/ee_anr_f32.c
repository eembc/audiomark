/**
 * Copyright (C) 2022 EEMBC
 * Copyright (C) 2022 Arm Limited
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */

#include "ee_audiomark.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "speex_preprocess.h"
#include "arch.h"

#ifdef OS_SUPPORT_CUSTOM
#ifdef FIXED_POINT
#define XPH_ANR_INSTANCE_SIZE 29000
#else
#define XPH_ANR_INSTANCE_SIZE 45250
#endif
// We manipulate these for each speex alloc based off our memory heaps
extern char *spxGlobalHeapPtr;
extern char *spxGlobalHeapEnd;
extern long  cumulatedMalloc;
#endif

static const uint32_t param_anr_f32[1][2] = {
    {
        256,   // FRAME SIZE
        16000, // SAMPLE RATE
    },
};

int32_t
ee_anr_f32(int32_t command, void **pp_inst, void *p_data, void *p_params)
{
    switch (command)
    {
        case NODE_MEMREQ: {
#ifdef OS_SUPPORT_CUSTOM
            *(uint32_t *)(*pp_inst) = XPH_ANR_INSTANCE_SIZE;
#else
            *(uint32_t *)(*pp_inst) = 0;
#endif
            break;
        }
        case NODE_RESET: {
            uint32_t              config_idx  = 0;
            uint32_t              frame_size  = 0;
            uint32_t              sample_rate = 0;
            SpeexPreprocessState *p_state     = NULL;

#ifdef OS_SUPPORT_CUSTOM
            // speex aligns memory during speex_alloc
            spxGlobalHeapPtr = (char *)(*pp_inst);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_ANR_INSTANCE_SIZE;
#endif
            config_idx  = *((uint32_t *)p_params);
            frame_size  = param_anr_f32[config_idx][0];
            sample_rate = param_anr_f32[config_idx][1];

            p_state = speex_preprocess_state_init(frame_size, sample_rate);
            speex_preprocess_ctl(p_state, SPEEX_PREPROCESS_SET_ECHO_STATE, 0);
            *((void **)pp_inst) = p_state;
            break;
        }
        case NODE_RUN: {
            PTR_INT              *ptr               = NULL;
            uint32_t              buffer_size       = 0;
            int32_t               nb_input_samples  = 0;
            int16_t              *p_in_place_buffer = NULL;
            SpeexPreprocessState *p_state           = *pp_inst;

            ptr               = (PTR_INT *)p_data;
            p_in_place_buffer = (int16_t *)(*ptr++);
            buffer_size       = (uint32_t)(*ptr++);

            nb_input_samples = buffer_size / sizeof(int16_t);

            if (nb_input_samples != 256)
            {
                return 1;
            }

            speex_preprocess_run(p_state, p_in_place_buffer);
            break;
        }
    }
    return 0;
}
