#include <stdlib.h>
#include <stdio.h>
#include "ee_audiomark.h"

#define TEST_NBUFFERS 104
#define NSAMPLES      256

extern const int16_t p_input[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_expected[TEST_NBUFFERS][NSAMPLES];

static int16_t p_input_sub[NSAMPLES];

static xdais_buffer_t xdais[1];

// Used deep inside speex_alloc; assigned by component NODE_RESET
char *spxGlobalHeapPtr;
char *spxGlobalHeapEnd;
long  cumulatedMalloc;

// N.B. Speex will round up to the next 8-byte boundary (or 'long long')
#define HEAP_SIZE (64 * 1024)

int
main(int argc, char *argv[])
{
    int       err           = 0;
    uint32_t  parameters[1] = { 0 };
    void     *heap          = NULL;
    uint32_t  memreq        = 0;
    uint32_t *ptr           = &memreq;

    heap = malloc(HEAP_SIZE);
    if (!heap)
    {
        printf("Error allocating heap\n");
        return -1;
    }
    printf("Heap start %016llx\n", (long long)heap);
    cumulatedMalloc = 0;

    if (ee_anr_f32(NODE_MEMREQ, (void **)&ptr, NULL, NULL))
    {
        printf("ANR NODE_MEMREQ failed\n");
        return -1;
    }

    /* ANR uses an in-place buffer. */
    SETUP_XDAIS(xdais[0], p_input_sub, 512);

    if (ee_anr_f32(NODE_RESET, (void **)&heap, xdais, &parameters))
    {
        printf("cumulatedMalloc = %ld\n", cumulatedMalloc);
        printf("ANR NODE_RESET failed\n");
        return -1;
    }
    if (cumulatedMalloc > HEAP_SIZE)
    {
        printf("cumulatedMalloc = %ld\n", cumulatedMalloc);
        printf("HEAP_SIZE = %d\n", HEAP_SIZE);
        printf("ANR ran out of malloc but didn't complain!\n");
        return -1;
    }
    printf("cumulatedMalloc = %ld\n", cumulatedMalloc);

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(p_input_sub, &p_input[i], 512);

        ee_anr_f32(NODE_RUN, (void **)&heap, xdais, 0);

        for (int j = 0; j < NSAMPLES; ++j)
        {
            if (p_input_sub[j] != p_expected[i][j])
            {
                err = 1;
                printf("S[%03d]B[%03d]O[%-5d]E[%-5d] ... FAIL\n",
                       i,
                       j,
                       p_input_sub[j],
                       p_expected[i][j]);
            }
        }
    }

    if (err)
    {
        printf("ANR test failed (%d)\n", err);
        return -1;
    }
    printf("ANR test passed\n");
    return 0;
}