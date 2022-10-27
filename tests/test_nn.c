#include "ee_types.h"
#include <stdio.h>

void th_nn_init(void);
void th_nn_classify(const int8_t input[490], int8_t output[12]);

const int8_t p_input[490] = {
    -40, 3,  10,  -7, 8,   -4,  -2,  3,  2,   0,  -39, 9,   8,   -5, 7,   -5,
    -2,  3,  0,   -2, -21, 18,  7,   -4, 6,   -5, -6,  4,   1,   -2, -23, 18,
    9,   -2, 5,   -5, -6,  3,   1,   -2, -45, 18, 10,  0,   4,   -3, -2,  3,
    0,   -1, -53, 11, 8,   -3,  4,   -2, -2,  4,  -2,  -1,  -34, 6,  9,   -7,
    7,   -3, -4,  1,  -2,  3,   -23, 19, 8,   -4, 6,   -5,  -8,  4,  2,   -1,
    -17, 19, 8,   -3, 6,   -5,  -6,  5,  2,   0,  -21, 21,  9,   -4, 4,   -7,
    -4,  6,  0,   -1, -28, 23,  8,   -5, 4,   -2, -3,  1,   -2,  -1, -32, 25,
    8,   -5, 3,   1,  -3,  -1,  -2,  0,  -28, 25, 6,   -3,  1,   -2, -2,  0,
    -4,  -1, -14, 18, 6,   -5,  -1,  -5, -1,  5,  -4,  -1,  -4,  13, 9,   -4,
    -1,  -4, 0,   5,  -3,  -1,  -3,  11, 9,   -3, -2,  -4,  1,   5,  -2,  0,
    -10, 15, 7,   -1, -2,  -5,  1,   4,  -1,  1,  -35, 16,  6,   0,  0,   -3,
    0,   2,  0,   0,  -49, 11,  5,   0,  0,   -2, -1,  2,   -1,  0,  -53, 11,
    5,   -2, 0,   -3, -2,  2,   0,   -1, -34, 16, -2,  -11, 2,   1,  -1,  1,
    0,   1,  -14, 23, 0,   -14, 2,   4,  -3,  2,  0,   1,   -4,  19, 1,   -11,
    4,   5,  -6,  5,  -1,  2,   -4,  19, 0,   -8, 1,   6,   -9,  8,  -1,  0,
    -6,  19, -1,  -5, -2,  5,   -9,  7,  1,   -2, -4,  17,  0,   -5, 0,   4,
    -8,  10, -1,  -1, -2,  16,  1,   -5, 2,   0,  -4,  8,   -1,  -2, -5,  15,
    4,   -5, 1,   -1, -3,  7,   -1,  -2, -16, 16, 6,   -2,  0,   -3, -2,  7,
    0,   -2, -43, 16, 7,   -1,  1,   -2, -1,  3,  0,   -1,  -52, 12, 4,   -3,
    0,   -3, -3,  3,  -1,  -1,  -55, 12, 2,   -2, 0,   -2,  -2,  3,  0,   -1,
    -58, 13, 6,   -3, 0,   -4,  -2,  3,  0,   -1, -43, 11,  3,   -4, 2,   -3,
    -2,  4,  0,   0,  -17, 18,  0,   -7, 2,   -2, -3,  6,   0,   -5, -10, 18,
    0,   -7, 3,   -2, -2,  7,   0,   -5, -9,  15, 0,   -6,  4,   -1, -2,  6,
    1,   -6, -11, 15, 0,   -5,  3,   -2, -2,  5,  1,   -5,  -11, 14, 2,   -3,
    2,   -2, -1,  5,  0,   -3,  -14, 13, 4,   -3, 1,   -3,  0,   5,  0,   -2,
    -21, 11, 9,   -3, 0,   -4,  -1,  3,  -1,  -1, -28, 11,  12,  -3, -1,  -5,
    -3,  4,  -1,  1,  -35, 11,  13,  -3, 1,   -4, -4,  3,   -2,  1,  -43, 11,
    13,  -2, 1,   -5, -4,  1,   -4,  3,  -45, 12, 13,  -4,  0,   -4, -1,  2,
    -2,  1,  -49, 14, 11,  -5,  1,   -2, -2,  2,  -2,  0,   -54, 14, 10,  -5,
    0,   -1, -2,  1,  -2,  1,   -60, 13, 10,  -3, 1,   -2,  0,   2,  -3,  1,
    -65, 12, 6,   -3, 1,   -3,  0,   3,  -1,  -1
};

int8_t p_output[12];

const int8_t p_expected[12]
    = { -128, -79, -128, -112, -44, -128, 8, 26, -13, -46, -87, -128 };

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