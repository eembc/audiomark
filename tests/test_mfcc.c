#include <stdio.h>
#include "ee_mfcc.h"
#include "mfcc_data.h"

int8_t p_output[NUM_MFCC_FEATURES];

int
main(int argc, char *argv[])
{
    int err = 0;

    ee_mfcc_init();
    ee_mfcc_compute(p_input, p_output);

    for (int i = 0; i < NUM_MFCC_FEATURES; ++i)
    {
        if (p_output[i] != p_expected[i])
        {
            err = 1;
            printf("feature[%d]: Computed %d, expected %d ... FAIL\n",
                   i,
                   p_output[i],
                   p_expected[i]);
        }
    }

    if (err)
    {
        printf("MFCC test failed\n");
        return -1;
    }
    printf("MFCC test passed\n");
    return 0;
}