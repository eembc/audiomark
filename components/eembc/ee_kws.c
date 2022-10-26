#include "ee_mfcc.h"
#include "public.h"
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

void *th_memcpy(void *restrict dst, const void *restrict src, size_t n);
void *th_memset(void *b, int c, size_t len);
void th_softmax_i8(const int8_t *vec_in, const uint16_t dim_vec, int8_t *p_out);
void th_nn_init(void);
void th_nn_classify(int8_t input[490], int8_t output[12]);

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

static int16_t audio_fifo[TOTAL_CHUNKS * SAMPLES_PER_CHUNK];
static int8_t  mfcc_fifo[NUM_MFCC_FRAMES * FEATURES_PER_FRAME];
static int8_t  output[OUT_DIM];

static int32_t chunk_idx = 0;
static int32_t mfcc_idx  = 0;

void
ee_kws_init(void)
{
    ee_mfcc_init();
    th_nn_init();

    /* Absolutely necessary so that the FIFO starts with silence. */
    th_memset(
        mfcc_fifo, 0, NUM_MFCC_FRAMES * FEATURES_PER_FRAME * BYTES_PER_FEATURE);
    /* Not really necessary, but good for debugging. */
    th_memset(
        audio_fifo, 0, TOTAL_CHUNKS * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
}

static void
debug_audio_fifo(void)
{
    printf("audio_fifo ... chunk_idx=%d\n", chunk_idx);
    for (int i = 0; i < TOTAL_CHUNKS; ++i)
    {
        printf("i[%02d]: ", i);
        for (int j = 0; j < SAMPLES_PER_CHUNK; ++j)
        {
            printf("%02d ", audio_fifo[i * SAMPLES_PER_CHUNK + j]);
        }
        if (i == chunk_idx)
        {
            printf(" *");
        }
        printf("\n");
        if (i == (CHUNK_WATERMARK - 1))
        {
            printf("---------WATERMARK---------\n");
        }
    }
}

static void
debug_mfcc_fifo(void)
{
    printf("mfcc_fifo ... mfcc_idx=%d\n", mfcc_idx);
    for (int i = 0; i < NUM_MFCC_FRAMES; ++i)
    {
        printf("i[%02d]: ", i);
        for (int j = 0; j < FEATURES_PER_FRAME; ++j)
        {
            printf("%02x ", (uint8_t)mfcc_fifo[i * FEATURES_PER_FRAME + j]);
        }
        if (i == mfcc_idx)
        {
            printf(" *");
        }
        printf("\n");
    }
}

void
ee_kws(int16_t *p_buffer, int *prediction)
{
    /* Store the incoming 16ms frame as 4 chunk */
    th_memcpy(&audio_fifo[chunk_idx * SAMPLES_PER_CHUNK],
              p_buffer,
              CHUNKS_PER_INPUT_BUFFER * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);
    chunk_idx += CHUNKS_PER_INPUT_BUFFER;

    if (chunk_idx >= CHUNK_WATERMARK)
    {
        ee_mfcc_compute(audio_fifo, &mfcc_fifo[mfcc_idx * FEATURES_PER_FRAME]);

        /* If we're now at the top of the FIFO, start shifting downward. */
        if (mfcc_idx == NUM_MFCC_FRAMES - 1)
        {
            /* Shift off last set of features, don't need to zeroize. */
            th_memcpy(mfcc_fifo,
                      mfcc_fifo + FEATURES_PER_FRAME,
                      mfcc_idx * FEATURES_PER_FRAME * BYTES_PER_FEATURE);
        }
        else
        {
            ++mfcc_idx;
        }

        /* Shift off the chunks used by the MFCC. */
        chunk_idx -= CHUNKS_PER_MFCC_SLIDE;
        th_memcpy(audio_fifo,
                  audio_fifo + SAMPLES_PER_OUTPUT_MFCC,
                  chunk_idx * SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE);

        th_nn_classify(mfcc_fifo, output);
        th_softmax_i8(output, OUT_DIM, output);
        for (int i = 0; i < OUT_DIM; ++i)
        {
            prediction[i] = (int)output[i];
        }
    }
}

void
ee_kws_test(void)
{
    int16_t paudio[SAMPLES_PER_INPUT_BUFFER];
    int8_t  predictions[OUT_DIM];
    ee_kws_init();

    /* Test 1sec audio at 16kHz */
    for (int i = 0; i < 1000; ++i)
    {
        for (int j = 0; j < SAMPLES_PER_INPUT_BUFFER; ++j)
        {
            paudio[j] = i;
        }
        ee_kws(paudio, predictions);
    }
}

int32_t
ee_kws_f32(int32_t command, void **pp_instance, void *p_data, void *p_params)
{
    PTR_INT *p_ptr = NULL;

    uint8_t *p_inbuf      = NULL;
    uint8_t *p_audio_fifo = NULL;
    uint8_t *p_mfcc_fifo  = NULL;
    uint8_t *p_classes    = NULL;

    uint32_t inbuf_size      = 0;
    uint32_t audio_fifo_size = 0;
    uint32_t mfcc_fifo_size  = 0;
    uint32_t classes_size    = 0;

    switch (command)
    {
        case NODE_MEMREQ:
            *(uint32_t *)(*pp_instance) = 0;
            break;
        case NODE_RESET:
            ee_kws_init();
            break;
        case NODE_RUN:
            p_ptr          = (PTR_INT *)p_data;
            p_inbuf        = (uint8_t *)(*p_ptr++);
            inbuf_size     = (uint32_t *)(*p_ptr++);
            p_audio_fifo   = (uint8_t *)(*p_ptr++);
            audio_fifo_size = (uint32_t *)(*p_ptr++);
            p_mfcc_fifo    = (uint8_t *)(*p_ptr++);
            mfcc_fifo_size = (uint32_t *)(*p_ptr++);
            p_classes      = (uint8_t *)(*p_ptr++);
            classes_size   = (uint32_t *)(*p_ptr++);

            ee_kws((int16_t *)p_inbuf, (int *)p_classes);
            break;
            /* case default: */
    }
    return 0;
}
