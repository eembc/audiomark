#include "ee_types.h"

void th_nn_init(void);
void th_nn_classify(int8_t p_mfcc[490], int8_t p_output[12]);

void
ee_nn_init(void)
{
    th_nn_init();
}

void
ee_nn_classify(int8_t p_mfcc[490], int8_t p_output[12])
{
    th_nn_classify(p_mfcc, p_output);
}
