#ifndef __EE_MFCC_H
#define __EE_MFCC_H

#include "ee_types.h"
#include "ee_mfccdata.h"
#include <math.h>

extern ee_mfcc_fft_f32_t g_mfcc_fft_instance;

ee_status_t ee_mfcc_init(void);
void        ee_mfcc_compute(const int16_t *audio_data, int8_t *mfcc_out);

ee_status_t th_mfcc_fft_init_f32(ee_mfcc_fft_f32_t *p_instance, int fft_length);
void        th_mfcc_fft_f32(ee_mfcc_fft_f32_t *pInstance,
                            ee_f32_t          *pIn,
                            ee_f32_t          *pOut);

/* This borrows _heavily_ from Arm CMSIS/DSP see copyright. */

/* result = A dot B */
void th_dot_prod_f32(ee_f32_t *p_a,
                     ee_f32_t *p_b,
                     uint32_t  len,
                     ee_f32_t *p_result);

/* C = A * B */
void th_multiply_f32(ee_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c, uint32_t len);

/* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */
void th_cmplx_mag_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

/* C = A + offset */
void th_offset_f32(ee_f32_t *p_a, ee_f32_t offset, ee_f32_t *p_c, uint32_t len);

/* C = log(A) */
void th_vlog_f32(ee_f32_t *p_a, ee_f32_t *p_c, uint32_t len);

/* C[m] = A[m,n] * B[m] */
void th_mat_vec_mult_f32(ee_matrix_f32_t *p_a, ee_f32_t *p_b, ee_f32_t *p_c);

#endif /* __EE_MFCC_H */
