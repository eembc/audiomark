#include <stdio.h>
#include "ee_audiomark.h"

#define TEST_NBUFFERS 104
#define NSAMPLES      256

extern const int16_t p_input[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_expected[TEST_NBUFFERS][NSAMPLES];

static int16_t p_input_sub[NSAMPLES];

static xdais_buffer_t xdais[1];

int
main(int argc, char *argv[])
{
    int      err           = 0;
    uint32_t parameters[1] = { 0 };
    uint32_t instance[2];

    /* ANR uses an in-place buffer. */
    SETUP_XDAIS(xdais[0], p_input_sub, 512);

    ee_anr_f32(NODE_RESET, (void **)&instance, xdais, &parameters);

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(p_input_sub, &p_input[i], 512);

        ee_anr_f32(NODE_RUN, (void **)&instance, xdais, 0);

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