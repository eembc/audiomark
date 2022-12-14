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
#include "speex_echo.h"

#ifdef OS_SUPPORT_CUSTOM
#ifdef FIXED_POINT
#define XPH_AEC_INSTANCE_SIZE 45000
#else
#define XPH_AEC_INSTANCE_SIZE 68100
#endif
// We manipulate these for each speex alloc based off our memory heaps
extern char *spxGlobalHeapPtr;
extern char *spxGlobalHeapEnd;
extern long  cumulatedMalloc;
#endif

static const uint32_t param_aec_f32[1][3] = {
    {
        256,   // FRAME SIZE
        1024,  // FILTER LENGTH
        16000, // SAMPLE RATE
    },
};

int32_t
ee_aec_f32(int32_t command, void **pp_inst, void *p_data, void *p_params)
{
    switch (command)
    {
        case NODE_MEMREQ: {
#ifdef OS_SUPPORT_CUSTOM
            *(uint32_t *)(*pp_inst) = XPH_AEC_INSTANCE_SIZE;
#else
            *(uint32_t *)(*pp_inst) = 0;
#endif
            break;
        }
        case NODE_RESET: {
            uint32_t        config_idx    = 0;
            uint32_t        frame_size    = 0;
            uint32_t        filter_length = 0;
            uint32_t        sample_rate   = 0;
            SpeexEchoState *p_state       = NULL;
#ifdef OS_SUPPORT_CUSTOM
            // speex aligns memory during speex_alloc
            spxGlobalHeapPtr = (char *)(*pp_inst);
            spxGlobalHeapEnd = spxGlobalHeapPtr + XPH_AEC_INSTANCE_SIZE;
#endif
            config_idx    = *((uint32_t *)p_params);
            frame_size    = param_aec_f32[config_idx][0];
            filter_length = param_aec_f32[config_idx][1];
            sample_rate   = param_aec_f32[config_idx][2];

            p_state = speex_echo_state_init(frame_size, filter_length);
            speex_echo_ctl(p_state, SPEEX_ECHO_SET_SAMPLING_RATE, &sample_rate);
            // N.B. Reassign warning: the output pp_inst is now speex object
            *((void **)pp_inst) = p_state;
            break;
        }
        case NODE_RUN: {
            PTR_INT        *ptr              = NULL;
            int16_t        *reference        = NULL;
            int16_t        *echo             = NULL;
            int16_t        *outBufs          = NULL;
            uint32_t        buffer_size      = 0;
            int32_t         nb_input_samples = 0;
            SpeexEchoState *p_state          = *pp_inst;

            ptr         = (PTR_INT *)p_data;
            reference   = (int16_t *)(*ptr++);
            buffer_size = (uint32_t)(*ptr++);
            echo        = (int16_t *)(*ptr++);
            buffer_size = (uint32_t)(*ptr++);
            outBufs     = (int16_t *)(*ptr++);

            nb_input_samples = buffer_size / sizeof(int16_t);

            if (nb_input_samples != 256)
            {
                return 1;
            }

            speex_echo_cancellation(p_state, reference, echo, outBufs);
            break;
        }
    }
    return 0;
}
