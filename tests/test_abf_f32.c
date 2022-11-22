#include <stdlib.h>
#include <stdio.h>
#include "ee_abf_f32.h"

#define TEST_NBUFFERS 104
#define NSAMPLES      256

extern const int16_t p_channel1[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_channel2[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_expected[TEST_NBUFFERS][NSAMPLES];

static int16_t p_left[NSAMPLES];
static int16_t p_right[NSAMPLES];
static int16_t p_output[NSAMPLES];

static xdais_buffer_t xdais[3];

int
main(int argc, char *argv[])
{
    int       err = 0;
    uint32_t  memreq;
    uint32_t *p_req = &memreq;
    void     *memory;
    void     *inst;

    ee_abf_f32(NODE_MEMREQ, (void **)&p_req, NULL, NULL);

    printf("ABF F32 MEMREQ = %d bytes\n", memreq);
    memory = malloc(memreq);
    if (!memory)
    {
        printf("malloc() fail\n");
        return -1;
    }
    inst = (void *)memory;

    SETUP_XDAIS(xdais[0], p_left, 512);
    SETUP_XDAIS(xdais[1], p_right, 512);
    SETUP_XDAIS(xdais[2], p_output, 512);

    ee_abf_f32(NODE_RESET, (void **)&inst, NULL, NULL);

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(p_left, &p_channel1[i], 512);
        memcpy(p_right, &p_channel2[i], 512);

        ee_abf_f32(NODE_RUN, (void **)&inst, xdais, NULL);

        for (int j = 0; j < NSAMPLES; ++j)
        {
            if (p_output[j] != p_expected[i][j])
            {
                err = 1;
                printf("S[%03d]B[%03d]L[%-5d]R[%-5d]O[%-5d]E[%-5d] ... FAIL\n",
                       i,
                       j,
                       p_left[j],
                       p_right[j],
                       p_output[j],
                       p_expected[i][j]);
            }
        }
    }

    if (err)
    {
        printf("ABF test failed\n");
        return -1;
    }
    printf("ABF test passed\n");
    return 0;
}