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

#include "ee_kws.h"

#ifdef DEBUG_PRINTF_CLASSES
#include <stdio.h>
#endif

/**
 * The KWS uses an MFCC to extract features per each audio frame, and then sends
 * a group of MFCC features to the neural net for classification. The KWS also
 * uses two FIFOs to synchronize the framerate of the audio output from the
 * BF/AEC/ANR to the neural net, since the frame sizes differ.
 *
 * The Audio FIFO matches the input buffers from the pipeline to the require-
 * ments of the DS CNN. The MFCC feature extractor requires 40 ms of data, with
 * a sliding window of 20 ms. The ANR produces a 16 ms mono buffer. As a result
 * the Audio FIFO must match the 40 ms requirement to the 16 ms input, and
 * maintain the window. To do this, it considers the input data in 4 ms chunks.
 * When 10x 4 ms chunks are available by appending the ANR's 16 ms (4 chunk)
 * buffers, it computes a new MFCC, and then shifts the entire FIFO to discard
 * the first 5x chunks, retaining 5 for the sliding window.
 *
 * Each time the MFCC produces a new frame of features (10), they are added
 * to the MFCC FIFO, and the NN classification is invoked. The MFCC FIFO starts
 * out with zero values (silence), and continues to add new frames until it
 * fills, at which point the the first entry is removed to make way for the
 * next frame of MFCC data.
 *
 * Assumptions mandated by this benchmark:
 * 1. 16 kHz sample rate with 2-byte (16-bit) time-domain samples.
 * 2. Audio pipeline output in 16 ms (256 sample) frames
 * 3. MFCC input in 40 ms (640 sample) frames with a sliding window of 20 ms
 * 4. 10 MFCC freatures per frame
 * 5. 490 features per NN input (49 frames x 10 features)
 * 6. 12 classifier outputs
 */

static ee_status_t
ee_kws_init(kws_instance_t *p_inst)
{
    ee_mfcc_f32_init(&(p_inst->mfcc_inst));
    th_nn_init();
    p_inst->chunk_idx = 0;
    return EE_STATUS_OK;
}

static ee_status_t
ee_kws_run(kws_instance_t *p_inst,
           const int16_t  *p_buffer,
           int8_t         *p_prediction,
           int            *p_new_inference)
{
    ee_status_t status = EE_STATUS_OK;

    /* KWS might not call an inference this time if the FIFO isn't ready. */
    if (p_new_inference != NULL)
    {
        *p_new_inference = 0;
    }

    /* Store the incoming 16ms frame as 4 chunk */
    th_memcpy(&p_inst->p_audio_fifo[p_inst->chunk_idx * SAMPLES_PER_CHUNK],
              p_buffer,
              CHUNKS_PER_INPUT_BUFFER * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
    p_inst->chunk_idx += CHUNKS_PER_INPUT_BUFFER;

    if (p_inst->chunk_idx >= CHUNK_WATERMARK)
    {
        /* Shift off the oldest features to make room for the new ones. */
        th_memmove(p_inst->p_mfcc_fifo,
                   p_inst->p_mfcc_fifo + FEATURES_PER_FRAME,
                   (NUM_MFCC_FRAMES - 1) * FEATURES_PER_FRAME);

        /* Compute a new frames worth of MFCC features */
        ee_mfcc_f32_compute(
            &(p_inst->mfcc_inst),
            p_inst->p_audio_fifo,
            &p_inst->p_mfcc_fifo[(NUM_MFCC_FRAMES - 1) * FEATURES_PER_FRAME]);

        /* Run the inference */
        status = th_nn_classify(p_inst->p_mfcc_fifo, p_prediction);
#ifdef DEBUG_PRINTF_CLASSES
        printf("OUTPUT: ");
        char output_class[12][8]
            = { "Silence", "Unknown", "yes", "no",  "up",   "down",
                "left",    "right",   "on",  "off", "stop", "go" };
        for (int i = 0; i < 12; ++i)
        {
            printf("% 4d ", p_prediction[i]);
        }
        int max_ind = 0;
        int max_val = -128000;
        for (int i = 0; i < 12; i++)
        {
            if (max_val < p_prediction[i])
            {
                max_val = p_prediction[i];
                max_ind = i;
            }
        }
        printf(" --> %8s (%3d%%)",
               output_class[max_ind],
               ((int)(p_prediction[max_ind] + 128) * 100 / 256));
        printf("\n");
#endif
        /* Testing likes to know if there was an inference */
        if (p_new_inference != NULL)
        {
            *p_new_inference = 1;
        }

        /* Shift off the aduio buffer chunks used by the MFCC. */
        p_inst->chunk_idx -= CHUNKS_PER_MFCC_SLIDE;
        th_memmove(p_inst->p_audio_fifo,
                   p_inst->p_audio_fifo + SAMPLES_PER_OUTPUT_MFCC,
                   p_inst->chunk_idx * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
    }
    return status;
}

#define CHECK_SIZE(X, Y) \
    if (X != Y)          \
    {                    \
        return 1;        \
    }

int32_t
ee_kws_f32(int32_t command, void **pp_inst, void *p_data, void *p_params)
{
    ee_status_t status = EE_STATUS_OK;

    switch (command)
    {
        case NODE_MEMREQ: {
            /**
             * N.B.1: The developer manages the buffer for the neural net
             *         in th_api.c
             */
            /**
             * N.B.2: https://arm-software.github.io/CMSIS-DSP/latest/ :
             *
             * When using a vectorized version, provide a little bit of padding
             * after the end of a buffer (3 words) because the vectorized code
             * may read a little bit after the end of a buffer. You don't have
             * to modify your buffers but just ensure that the end of buffer +
             * padding is not outside of a memory region.
             */
            uint32_t size = (3 * 4) // See note above
                            + sizeof(mfcc_instance_t)
                            + 8; /* TODO : justift this */
            *(uint32_t *)(*pp_inst) = size;
            break;
        }
        case NODE_RESET: {
            status = ee_kws_init((kws_instance_t *)(*pp_inst));
            if (status == EE_STATUS_OK)
            {
                ((kws_instance_t *)(*pp_inst))->chunk_idx = 0;
            }
            break;
        }
        case NODE_RUN: {
            PTR_INT        *p_ptr            = NULL;
            int16_t        *p_inbuf          = NULL;
            int8_t         *p_predictions    = NULL;
            uint32_t        inbuf_size       = 0;
            uint32_t        audio_fifo_size  = 0;
            uint32_t        mfcc_fifo_size   = 0;
            uint32_t        predictions_size = 0;
            int            *new_inference    = (int *)p_params;
            kws_instance_t *p_inst           = *pp_inst;

            p_ptr                = (PTR_INT *)p_data;
            p_inbuf              = (int16_t *)(*p_ptr++);
            inbuf_size           = (uint32_t)(*p_ptr++);
            p_inst->p_audio_fifo = (int16_t *)(*p_ptr++);
            audio_fifo_size      = (uint32_t)(*p_ptr++);
            p_inst->p_mfcc_fifo  = (int8_t *)(*p_ptr++);
            mfcc_fifo_size       = (uint32_t)(*p_ptr++);
            p_predictions        = (int8_t *)(*p_ptr++);
            predictions_size     = (uint32_t)(*p_ptr++);

            CHECK_SIZE(inbuf_size, SAMPLES_PER_INPUT_BUFFER * 2);
            CHECK_SIZE(audio_fifo_size, TOTAL_CHUNKS * SAMPLES_PER_CHUNK * 2);
            CHECK_SIZE(mfcc_fifo_size, NUM_MFCC_FRAMES * FEATURES_PER_FRAME);
            CHECK_SIZE(predictions_size, OUT_DIM);

            status = ee_kws_run(p_inst, p_inbuf, p_predictions, new_inference);
            break;
        }
    }
    return status == EE_STATUS_OK ? 0 : 1;
}
