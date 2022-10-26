
#include "arm_nnfunctions.h"
#include "dsp/none.h"
#include "ee_mfcc.h"
#include "ee_nn_weights.h"

ee_status_t
th_mfcc_fft_init_f32(ee_mfcc_fft_f32_t *p_instance, int fft_length)
{
    arm_status status;

    status = arm_rfft_fast_init_f32(p_instance, fft_length);
    if (!status)
    {
        return EE_STATUS_ERROR;
    }
    return EE_STATUS_OK;
}

void
th_mfcc_fft_f32(TH_MFCC_FFT_INSTANCE_FLOAT32_TYPE *p_context,
                ee_f32_t                          *p_in,
                ee_f32_t                          *p_out)
{
    arm_rfft_fast_f32(p_context, p_in, p_out, 0);
}

void
th_dot_prod_f32(ee_f32_t *p_a, ee_f32_t *p_b, uint32_t len, ee_f32_t *p_result)
{
    arm_dot_prod_f32(p_a, p_b, len, p_result);
}

void
th_multiply_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t n)
{
    arm_mult_f32(p_a, p_b, p_c, n);
}

void
th_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    arm_cmplx_mag_f32(p_a, p_c, len);
}

void
th_offset_f32(ee_f32_t *p_a, ee_f32_t offset, ee_f32_t *p_c, uint32_t len)
{
    arm_offset_f32(p_a, offset, p_c, len);
}

void
th_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len)
{
    arm_vlog_f32(p_a, p_c, len);
}

void
th_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c)
{
    arm_matrix_instance_f32 m;
    m.numCols = p_a->numCols;
    m.numRows = p_a->numRows;
    m.pData   = p_a->pData;
    arm_mat_vec_mult_f32(&m, p_b, p_c);
}

void
th_softmax_i8(const int8_t *vec_in, const uint16_t dim_vec, int8_t *p_out)
{
    arm_softmax_q7(vec_in, dim_vec, p_out);
}

void *
th_memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    return memcpy(dst, src, n);
}

void *
th_memset(void *b, int c, size_t len)
{
    return memset(b, c, len);
}

#define SAMP_FREQ      16000
#define MFCC_DEC_BITS  1
#define FRAME_SHIFT_MS 20
//#define FRAME_SHIFT ((int16_t)(SAMP_FREQ * 0.001 * FRAME_SHIFT_MS))
#define FRAME_SHIFT       320
#define NUM_FRAMES        49
#define NUM_MFCC_COEFFS   10
#define NUM_MFCC_FEATURES (NUM_MFCC_COEFFS)
#define MFCC_BUFFER_SIZE  (NUM_FRAMES * NUM_MFCC_COEFFS)
#define FRAME_LEN_MS      40
//#define FRAME_LEN ((int16_t)(SAMP_FREQ * 0.001 * FRAME_LEN_MS))
#define FRAME_LEN 640

#define IN_DIM  (NUM_FRAMES * NUM_MFCC_COEFFS)
#define OUT_DIM 12

#define CONV1_OUT_CH      64
#define CONV1_IN_X        NUM_MFCC_COEFFS
#define CONV1_IN_Y        NUM_FRAMES
#define CONV1_KX          4
#define CONV1_KY          10
#define CONV1_SX          2
#define CONV1_SY          2
#define CONV1_PX          1
#define CONV1_PY          4
#define CONV1_OUT_X       5
#define CONV1_OUT_Y       25
#define CONV1_BIAS_LSHIFT 2
#define CONV1_OUT_RSHIFT  6

#define CONV2_OUT_CH         64
#define CONV2_IN_X           CONV1_OUT_X
#define CONV2_IN_Y           CONV1_OUT_Y
#define CONV2_DS_KX          3
#define CONV2_DS_KY          3
#define CONV2_DS_SX          1
#define CONV2_DS_SY          1
#define CONV2_DS_PX          1
#define CONV2_DS_PY          1
#define CONV2_OUT_X          5
#define CONV2_OUT_Y          25
#define CONV2_DS_BIAS_LSHIFT 2
#define CONV2_DS_OUT_RSHIFT  5
#define CONV2_PW_BIAS_LSHIFT 4
#define CONV2_PW_OUT_RSHIFT  8

#define CONV3_OUT_CH         64
#define CONV3_IN_X           CONV2_OUT_X
#define CONV3_IN_Y           CONV2_OUT_Y
#define CONV3_DS_KX          3
#define CONV3_DS_KY          3
#define CONV3_DS_SX          1
#define CONV3_DS_SY          1
#define CONV3_DS_PX          1
#define CONV3_DS_PY          1
#define CONV3_OUT_X          CONV3_IN_X
#define CONV3_OUT_Y          CONV3_IN_Y
#define CONV3_DS_BIAS_LSHIFT 2
#define CONV3_DS_OUT_RSHIFT  4
#define CONV3_PW_BIAS_LSHIFT 5
#define CONV3_PW_OUT_RSHIFT  8

#define CONV4_OUT_CH         64
#define CONV4_IN_X           CONV3_OUT_X
#define CONV4_IN_Y           CONV3_OUT_Y
#define CONV4_DS_KX          3
#define CONV4_DS_KY          3
#define CONV4_DS_SX          1
#define CONV4_DS_SY          1
#define CONV4_DS_PX          1
#define CONV4_DS_PY          1
#define CONV4_OUT_X          CONV4_IN_X
#define CONV4_OUT_Y          CONV4_IN_Y
#define CONV4_DS_BIAS_LSHIFT 3
#define CONV4_DS_OUT_RSHIFT  5
#define CONV4_PW_BIAS_LSHIFT 5
#define CONV4_PW_OUT_RSHIFT  7

#define CONV5_OUT_CH         64
#define CONV5_IN_X           CONV4_OUT_X
#define CONV5_IN_Y           CONV4_OUT_Y
#define CONV5_DS_KX          3
#define CONV5_DS_KY          3
#define CONV5_DS_SX          1
#define CONV5_DS_SY          1
#define CONV5_DS_PX          1
#define CONV5_DS_PY          1
#define CONV5_OUT_X          CONV5_IN_X
#define CONV5_OUT_Y          CONV5_IN_Y
#define CONV5_DS_BIAS_LSHIFT 3
#define CONV5_DS_OUT_RSHIFT  5
#define CONV5_PW_BIAS_LSHIFT 5
#define CONV5_PW_OUT_RSHIFT  8

#define FINAL_FC_BIAS_LSHIFT 2
#define FINAL_FC_OUT_RSHIFT  7

#define SCRATCH_BUFFER_SIZE                           \
    (2 * 2 * CONV1_OUT_CH * CONV2_DS_KX * CONV2_DS_KY \
     + 2 * CONV2_OUT_CH * CONV2_OUT_X * CONV2_OUT_Y)

static const q7_t conv1_wt[CONV1_OUT_CH * CONV1_KX * CONV1_KY] = CONV1_WT;
static const q7_t conv1_bias[CONV1_OUT_CH]                     = CONV1_BIAS;
static const q7_t conv2_ds_wt[CONV1_OUT_CH * CONV2_DS_KX * CONV2_DS_KY]
    = CONV2_DS_WT;
static const q7_t conv2_ds_bias[CONV1_OUT_CH]              = CONV2_DS_BIAS;
static const q7_t conv2_pw_wt[CONV2_OUT_CH * CONV1_OUT_CH] = CONV2_PW_WT;
static const q7_t conv2_pw_bias[CONV2_OUT_CH]              = CONV2_PW_BIAS;
static const q7_t conv3_ds_wt[CONV2_OUT_CH * CONV3_DS_KX * CONV3_DS_KY]
    = CONV3_DS_WT;
static const q7_t conv3_ds_bias[CONV2_OUT_CH]              = CONV3_DS_BIAS;
static const q7_t conv3_pw_wt[CONV3_OUT_CH * CONV2_OUT_CH] = CONV3_PW_WT;
static const q7_t conv3_pw_bias[CONV3_OUT_CH]              = CONV3_PW_BIAS;
static const q7_t conv4_ds_wt[CONV3_OUT_CH * CONV4_DS_KX * CONV4_DS_KY]
    = CONV4_DS_WT;
static const q7_t conv4_ds_bias[CONV3_OUT_CH]              = CONV4_DS_BIAS;
static const q7_t conv4_pw_wt[CONV4_OUT_CH * CONV3_OUT_CH] = CONV4_PW_WT;
static const q7_t conv4_pw_bias[CONV4_OUT_CH]              = CONV4_PW_BIAS;
static const q7_t conv5_ds_wt[CONV4_OUT_CH * CONV5_DS_KX * CONV5_DS_KY]
    = CONV5_DS_WT;
static const q7_t conv5_ds_bias[CONV4_OUT_CH]              = CONV5_DS_BIAS;
static const q7_t conv5_pw_wt[CONV5_OUT_CH * CONV4_OUT_CH] = CONV5_PW_WT;
static const q7_t conv5_pw_bias[CONV5_OUT_CH]              = CONV5_PW_BIAS;
static const q7_t final_fc_wt[CONV5_OUT_CH * OUT_DIM]      = FINAL_FC_WT;
static const q7_t final_fc_bias[OUT_DIM]                   = FINAL_FC_BIAS;
static q7_t       scratch_pad[SCRATCH_BUFFER_SIZE];
static q7_t      *col_buffer;
static q7_t      *buffer1;
static q7_t      *buffer2;

void
th_nn_init(void)
{
    buffer1    = scratch_pad;
    buffer2    = buffer1 + (CONV1_OUT_CH * CONV1_OUT_X * CONV1_OUT_Y);
    col_buffer = buffer2 + (CONV2_OUT_CH * CONV2_OUT_X * CONV2_OUT_Y);
}

void
arm_avepool_q7_HWC_nonsquare(
    const q7_t    *Im_in,        // input image
    const uint16_t dim_im_in_x,  // input image dimension
    const uint16_t dim_im_in_y,  // input image dimension
    const uint16_t ch_im_in,     // number of input image channels
    const uint16_t dim_kernel_x, // window kernel size
    const uint16_t dim_kernel_y, // window kernel size
    const uint16_t padding_x,    // padding sizes
    const uint16_t padding_y,    // padding sizes
    const uint16_t stride_x,     // stride
    const uint16_t stride_y,     // stride
    const uint16_t dim_im_out_x, // output image dimension
    const uint16_t dim_im_out_y, // output image dimension
    q7_t          *bufferA,      // a buffer for local storage
    q7_t          *Im_out,       // output feature
    const uint16_t out_lshift)   // output left shift (scaling)
{
    int16_t i_ch_in, i_x, i_y;
    int16_t k_x, k_y;

    for (i_ch_in = 0; i_ch_in < ch_im_in; i_ch_in++)
    {
        for (i_y = 0; i_y < dim_im_out_y; i_y++)
        {
            for (i_x = 0; i_x < dim_im_out_x; i_x++)
            {
                int sum   = 0;
                int count = 0;
                for (k_y = i_y * stride_y - padding_y;
                     k_y < i_y * stride_y - padding_y + dim_kernel_y;
                     k_y++)
                {
                    for (k_x = i_x * stride_x - padding_x;
                         k_x < i_x * stride_x - padding_x + dim_kernel_x;
                         k_x++)
                    {
                        if (k_y >= 0 && k_x >= 0 && k_y < dim_im_in_y
                            && k_x < dim_im_in_x)
                        {
                            sum += Im_in[i_ch_in
                                         + ch_im_in
                                               * (k_x + k_y * dim_im_in_x)];
                            count++;
                        }
                    }
                }
                Im_out[i_ch_in + ch_im_in * (i_x + i_y * dim_im_out_x)]
                    = sum * (0x1 << out_lshift) / count;
            }
        }
    }
}

void
th_nn_classify(uint8_t in_data[490], int8_t out_data[12])
{
    // CONV1 : regular convolution
    arm_convolve_HWC_q7_basic_nonsquare(in_data,
                                        CONV1_IN_X,
                                        CONV1_IN_Y,
                                        1,
                                        conv1_wt,
                                        CONV1_OUT_CH,
                                        CONV1_KX,
                                        CONV1_KY,
                                        CONV1_PX,
                                        CONV1_PY,
                                        CONV1_SX,
                                        CONV1_SY,
                                        conv1_bias,
                                        CONV1_BIAS_LSHIFT,
                                        CONV1_OUT_RSHIFT,
                                        buffer1,
                                        CONV1_OUT_X,
                                        CONV1_OUT_Y,
                                        (q15_t *)col_buffer,
                                        NULL);
    arm_relu_q7(buffer1, CONV1_OUT_X * CONV1_OUT_Y * CONV1_OUT_CH);

    // CONV2 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV2_IN_X,
                                                  CONV2_IN_Y,
                                                  CONV1_OUT_CH,
                                                  conv2_ds_wt,
                                                  CONV1_OUT_CH,
                                                  CONV2_DS_KX,
                                                  CONV2_DS_KY,
                                                  CONV2_DS_PX,
                                                  CONV2_DS_PY,
                                                  CONV2_DS_SX,
                                                  CONV2_DS_SY,
                                                  conv2_ds_bias,
                                                  CONV2_DS_BIAS_LSHIFT,
                                                  CONV2_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV2_OUT_X,
                                                  CONV2_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV2_OUT_X * CONV2_OUT_Y * CONV2_OUT_CH);

    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV2_OUT_X,
                                           CONV2_OUT_Y,
                                           CONV1_OUT_CH,
                                           conv2_pw_wt,
                                           CONV2_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv2_pw_bias,
                                           CONV2_PW_BIAS_LSHIFT,
                                           CONV2_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV2_OUT_X,
                                           CONV2_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV2_OUT_X * CONV2_OUT_Y * CONV2_OUT_CH);

    // CONV3 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV3_IN_X,
                                                  CONV3_IN_Y,
                                                  CONV2_OUT_CH,
                                                  conv3_ds_wt,
                                                  CONV2_OUT_CH,
                                                  CONV3_DS_KX,
                                                  CONV3_DS_KY,
                                                  CONV3_DS_PX,
                                                  CONV3_DS_PY,
                                                  CONV3_DS_SX,
                                                  CONV3_DS_SY,
                                                  conv3_ds_bias,
                                                  CONV3_DS_BIAS_LSHIFT,
                                                  CONV3_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV3_OUT_X,
                                                  CONV3_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV3_OUT_X * CONV3_OUT_Y * CONV3_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV3_OUT_X,
                                           CONV3_OUT_Y,
                                           CONV2_OUT_CH,
                                           conv3_pw_wt,
                                           CONV3_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv3_pw_bias,
                                           CONV3_PW_BIAS_LSHIFT,
                                           CONV3_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV3_OUT_X,
                                           CONV3_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV3_OUT_X * CONV3_OUT_Y * CONV3_OUT_CH);
    // CONV4 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV4_IN_X,
                                                  CONV4_IN_Y,
                                                  CONV3_OUT_CH,
                                                  conv4_ds_wt,
                                                  CONV3_OUT_CH,
                                                  CONV4_DS_KX,
                                                  CONV4_DS_KY,
                                                  CONV4_DS_PX,
                                                  CONV4_DS_PY,
                                                  CONV4_DS_SX,
                                                  CONV4_DS_SY,
                                                  conv4_ds_bias,
                                                  CONV4_DS_BIAS_LSHIFT,
                                                  CONV4_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV4_OUT_X,
                                                  CONV4_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV4_OUT_X * CONV4_OUT_Y * CONV4_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV4_OUT_X,
                                           CONV4_OUT_Y,
                                           CONV3_OUT_CH,
                                           conv4_pw_wt,
                                           CONV4_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv4_pw_bias,
                                           CONV4_PW_BIAS_LSHIFT,
                                           CONV4_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV4_OUT_X,
                                           CONV4_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV4_OUT_X * CONV4_OUT_Y * CONV4_OUT_CH);
    // CONV5 : DS + PW conv
    // Depthwise separable conv (batch norm params folded into conv wts/bias)
    arm_depthwise_separable_conv_HWC_q7_nonsquare(buffer1,
                                                  CONV5_IN_X,
                                                  CONV5_IN_Y,
                                                  CONV4_OUT_CH,
                                                  conv5_ds_wt,
                                                  CONV4_OUT_CH,
                                                  CONV5_DS_KX,
                                                  CONV5_DS_KY,
                                                  CONV5_DS_PX,
                                                  CONV5_DS_PY,
                                                  CONV5_DS_SX,
                                                  CONV5_DS_SY,
                                                  conv5_ds_bias,
                                                  CONV5_DS_BIAS_LSHIFT,
                                                  CONV5_DS_OUT_RSHIFT,
                                                  buffer2,
                                                  CONV5_OUT_X,
                                                  CONV5_OUT_Y,
                                                  (q15_t *)col_buffer,
                                                  NULL);
    arm_relu_q7(buffer2, CONV5_OUT_X * CONV5_OUT_Y * CONV5_OUT_CH);
    // Pointwise conv
    arm_convolve_1x1_HWC_q7_fast_nonsquare(buffer2,
                                           CONV5_OUT_X,
                                           CONV5_OUT_Y,
                                           CONV4_OUT_CH,
                                           conv5_pw_wt,
                                           CONV5_OUT_CH,
                                           1,
                                           1,
                                           0,
                                           0,
                                           1,
                                           1,
                                           conv5_pw_bias,
                                           CONV5_PW_BIAS_LSHIFT,
                                           CONV5_PW_OUT_RSHIFT,
                                           buffer1,
                                           CONV5_OUT_X,
                                           CONV5_OUT_Y,
                                           (q15_t *)col_buffer,
                                           NULL);
    arm_relu_q7(buffer1, CONV5_OUT_X * CONV5_OUT_Y * CONV5_OUT_CH);
    // Average pool
    arm_avepool_q7_HWC_nonsquare(buffer1,
                                 CONV5_OUT_X,
                                 CONV5_OUT_Y,
                                 CONV5_OUT_CH,
                                 CONV5_OUT_X,
                                 CONV5_OUT_Y,
                                 0,
                                 0,
                                 1,
                                 1,
                                 1,
                                 1,
                                 NULL,
                                 buffer2,
                                 2);
    arm_fully_connected_q7(buffer2,
                           final_fc_wt,
                           CONV5_OUT_CH,
                           OUT_DIM,
                           FINAL_FC_BIAS_LSHIFT,
                           FINAL_FC_OUT_RSHIFT,
                           final_fc_bias,
                           out_data,
                           (q15_t *)col_buffer);
}
