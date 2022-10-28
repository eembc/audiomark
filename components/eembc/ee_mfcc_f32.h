#ifndef __EE_MFCC_F32_H
#define __EE_MFCC_F32_H

#include <math.h>
#include "ee_mfcc_f32_tables.h"
#include "ee_api.h"

/* 40ms frame of 16 kHz audio is 640 16-bit samples. */
#define FRAME_LEN 640
/* Pre-defined */
#define NUM_MFCC_FEATURES 10
#define MFCC_DEC_BITS     1
// frame_len_padded = pow(2,ceil((log(FRAME_LEN)/log(2)))); == 1024
#define PADDED_FRAME_LEN 1024
#define FFT_LEN          PADDED_FRAME_LEN

ee_status_t ee_mfcc_f32_init(void);
void        ee_mfcc_f32_compute(const int16_t *audio_data, int8_t *mfcc_out);

#endif /* __EE_MFCC_H */
