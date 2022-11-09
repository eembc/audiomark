#include <stdio.h>
#include "ee_audiomark.h"

#define TEST_NBUFFERS 104
#define NSAMPLES      256

extern const int16_t p_input[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_echo[TEST_NBUFFERS][NSAMPLES];
extern const int16_t p_expected[TEST_NBUFFERS][NSAMPLES];

static int16_t p_input_sub[NSAMPLES];
static int16_t p_echo_sub[NSAMPLES];
static int16_t p_output_sub[NSAMPLES];

static xdais_buffer_t xdais[3];

int
main(int argc, char *argv[])
{
    int      err           = 0;
    uint32_t parameters[1] = { 0 };
    uint32_t instance[2];

    SETUP_XDAIS(xdais[0], p_input_sub, 512);
    SETUP_XDAIS(xdais[1], p_echo_sub, 512);
    SETUP_XDAIS(xdais[2], p_output_sub, 512);

    ee_aec_f32(NODE_RESET, (void **)&instance, xdais, &parameters);

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(p_input_sub, &p_input[i], 512);
        memcpy(p_echo_sub, &p_echo[i], 512);

        ee_aec_f32(NODE_RUN, (void **)&instance, xdais, 0);

        for (int j = 0; j < NSAMPLES; ++j)
        {
            if (p_output_sub[j] != p_expected[i][j])
            {
                err = 1;
                printf("S[%03d]B[%03d]I[%-5d]C[%-5d]O[%-5d]E[%-5d] ... FAIL\n",
                       i,
                       j,
                       p_input_sub[j],
                       p_echo_sub[j],
                       p_output_sub[j],
                       p_expected[i][j]);
            }
        }
    }

    if (err)
    {
        printf("AEC test failed (%d)\n", err);
        return -1;
    }
    printf("AEC test passed\n");
    return 0;
}