#include <stdio.h>
#include "ee_types.h"
#include "nn_data.h"

void th_nn_init(void);
void th_nn_classify(const int8_t input[490], int8_t output[12]);

int8_t p_output[12];

int
main(int argc, char *argv[])
{
    int err = 0;

    th_nn_init();
    th_nn_classify(p_input, p_output);

    for (int i = 0; i < 12; ++i)
    {
        if (p_output[i] != p_expected[i])
        {
            err = 1;
            printf("class[%d]: Computed %d, expected %d ... FAIL\n",
                   i,
                   p_output[i],
                   p_expected[i]);
        }
    }

    if (err)
    {
        printf("NN test failed\n");
        return -1;
    }
    printf("NN test passed\n");
    return 0;
}