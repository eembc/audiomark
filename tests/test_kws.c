#include <stdio.h>
#include "ee_types.h"
#include "kws_data.h"
#include "public.h"

int32_t ee_kws_f32(int32_t command,
                   void  **pp_instance,
                   void   *p_data,
                   void   *p_params);

static int16_t        aec_output[256];     // 5
static int16_t        audio_fifo[13 * 64]; // 6 ptorelli FIXME TODO
static int8_t         mfcc_fifo[490];      // 7 ptorelli FIXME TODO
static int8_t         classes[12];         // 8 ptorelli FIXME TODO
static xdais_buffer_t xdais_kws[4];

int
main(int argc, char *argv[])
{
    int           err               = 0;
    int8_t        p_output[OUT_DIM] = { 0 };
    int           new_inference     = 0;
    const int8_t *p_check           = NULL;
    int           idx_check         = 0;

    SETUP_XDAIS(xdais_kws[0], aec_output, 512);
    SETUP_XDAIS(xdais_kws[1], audio_fifo, 13 * 64 * 2); // ptorelli: fixme
    SETUP_XDAIS(xdais_kws[2], mfcc_fifo, 490);          // ptorelli: fixme
    SETUP_XDAIS(xdais_kws[3], classes, 12);             // ptorelli: fixme

    ee_kws_f32(NODE_RESET, NULL, xdais_kws, 0);

    for (int i = 0; i < TEST_NBUFFERS; ++i)
    {
        memcpy(aec_output, p_input[i], 512);
        ee_kws_f32(NODE_RUN, NULL, xdais_kws, 0);
        if (new_inference)
        {
            printf("new inference %d\n", idx_check + 1);
            p_check = p_expected[idx_check];
            ++idx_check;

            for (int j = 0; j < OUT_DIM; ++j)
            {
                if (p_output[j] != p_check[j])
                {
                    err = 1;
                    printf("buffer[%d]class[%d]: Got %d, expected %d - FAIL\n",
                           i,
                           j,
                           p_output[j],
                           p_check[j]);
                }
            }
        }
    }

    if (err)
    {
        printf("KWS test failed\n");
        return -1;
    }
    printf("KWS test passed\n");
    return 0;
}