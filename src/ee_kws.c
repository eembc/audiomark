/*
 * Copyright (C) EEMBC(R). All Rights Reserved
 *
 * All EEMBC Benchmark Software are products of EEMBC and are provided under the
 * terms of the EEMBC Benchmark License Agreements. The EEMBC Benchmark Software
 * are proprietary intellectual properties of EEMBC and its Members and is
 * protected under all applicable laws, including all applicable copyright laws.
 *
 * If you received this EEMBC Benchmark Software without having a currently
 * effective EEMBC Benchmark License Agreement, you must discontinue use.
 */
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

#include "ee_mfcc_f32.h"
#include "ee_audiomark.h"
#include <stdio.h>

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

/* TODO: Coalesce the massive amount of #defines! */

#define OUT_DIM            12
#define NUM_MFCC_FRAMES    49 /* 49 * 320 = 15.680e3 samples */
#define FEATURES_PER_FRAME NUM_MFCC_FEATURES

#define BYTES_PER_SAMPLE         2
#define BYTES_PER_FEATURE        1
#define SAMPLES_PER_CHUNK        64 /* 4ms is 64 samples @ 16 kHz */
#define CHUNKS_PER_INPUT_BUFFER  4  /* Incoming frame is 4 chunks (16ms) */
#define SAMPLES_PER_INPUT_BUFFER (CHUNKS_PER_INPUT_BUFFER * SAMPLES_PER_CHUNK)
#define CHUNKS_PER_MFCC_SLIDE    5 /* Outgoing is 5 chunks (20ms) */
#define SAMPLES_PER_OUTPUT_MFCC  (CHUNKS_PER_MFCC_SLIDE * SAMPLES_PER_CHUNK)
#define CHUNK_WATERMARK          10 /* Enough data in FIFO for an MFCC */
#define TOTAL_CHUNKS             13 /* Will never be more than this. */
/* CHUNK_WATERMARK = (CHUNKS_PER_MFCC_SLIDE * 2) */
/* TOTAL_CHUNKS = (CHUNK_WATERMARK + CHUNKS_PER_INPUT_BUFFER - 1) */

static int16_t *p_audio_fifo = NULL; // [TOTAL_CHUNKS * SAMPLES_PER_CHUNK];
static int8_t  *p_mfcc_fifo  = NULL; // [NUM_MFCC_FRAMES * FEATURES_PER_FRAME];

static int32_t chunk_idx = 0;

void
ee_kws_init(void)
{
    ee_mfcc_f32_init();
    th_nn_init();
}

void
ee_kws_run(const int16_t *p_buffer, int8_t *p_prediction, int *p_new_inference)
{
    /* KWS might not call an inference this time if the FIFO isn't ready. */
    *p_new_inference = 0;

    /* Store the incoming 16ms frame as 4 chunk */
    th_memcpy(&p_audio_fifo[chunk_idx * SAMPLES_PER_CHUNK],
              p_buffer,
              CHUNKS_PER_INPUT_BUFFER * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
    chunk_idx += CHUNKS_PER_INPUT_BUFFER;

    if (chunk_idx >= CHUNK_WATERMARK)
    {
        /* Shift off the oldest features to make room for the new ones. */
        th_memcpy(p_mfcc_fifo,
                  p_mfcc_fifo + FEATURES_PER_FRAME,
                  (NUM_MFCC_FRAMES - 1) * FEATURES_PER_FRAME);

        /* Compute a new frames worth of MFCC features */
        ee_mfcc_f32_compute(
            p_audio_fifo,
            &p_mfcc_fifo[(NUM_MFCC_FRAMES - 1) * FEATURES_PER_FRAME]);

        /* Run the inference */
        th_nn_classify(p_mfcc_fifo, p_prediction);
        th_softmax_i8(p_prediction, OUT_DIM, p_prediction);
        *p_new_inference = 1;

        /* Shift off the aduio buffer chunks used by the MFCC. */
        chunk_idx -= CHUNKS_PER_MFCC_SLIDE;
        th_memcpy(p_audio_fifo,
                  p_audio_fifo + SAMPLES_PER_OUTPUT_MFCC,
                  chunk_idx * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
    }
}

#define CHECK_SIZE(X, Y)                          \
    if (X != Y)                                   \
    {                                             \
        printf("Size mismatch %d != %d\n", X, Y); \
        return 1;                                 \
    }

int32_t
ee_kws_f32(int32_t command, void **pp_instance, void *p_data, void *p_params)
{
    PTR_INT *p_ptr = NULL;

    int16_t *p_inbuf       = NULL;
    int8_t  *p_predictions = NULL;

    uint32_t inbuf_size       = 0;
    uint32_t audio_fifo_size  = 0;
    uint32_t mfcc_fifo_size   = 0;
    uint32_t predictions_size = 0;
    int      new_inference    = 0;

    switch (command)
    {
        case NODE_MEMREQ:
            *(uint32_t *)(*pp_instance) = 0;
            break;
        case NODE_RESET:
            ee_kws_init();
            break;
        case NODE_RUN:
            p_ptr            = (PTR_INT *)p_data;
            p_inbuf          = (int16_t *)(*p_ptr++);
            inbuf_size       = (uint32_t)(*p_ptr++);
            p_audio_fifo     = (int16_t *)(*p_ptr++);
            audio_fifo_size  = (uint32_t)(*p_ptr++);
            p_mfcc_fifo      = (int8_t *)(*p_ptr++);
            mfcc_fifo_size   = (uint32_t)(*p_ptr++);
            p_predictions    = (int8_t *)(*p_ptr++);
            predictions_size = (uint32_t)(*p_ptr++);

            CHECK_SIZE(inbuf_size, SAMPLES_PER_INPUT_BUFFER * 2);
            CHECK_SIZE(audio_fifo_size, TOTAL_CHUNKS * SAMPLES_PER_CHUNK * 2);
            CHECK_SIZE(mfcc_fifo_size, NUM_MFCC_FRAMES * FEATURES_PER_FRAME);
            CHECK_SIZE(predictions_size, OUT_DIM);

            ee_kws_run(p_inbuf, p_predictions, &new_inference);

            break;
            /* case default: */
    }
    return 0;
}
