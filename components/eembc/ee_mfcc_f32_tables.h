#ifndef __EE_MFCCDATA_H
#define __EE_MFCCDATA_H

#include "ee_types.h"

#define NUM_MFCC_FEATURES 10

#define EE_NUM_MFCC_DCT_COEFS 400
extern const ee_f32_t ee_mfcc_dct_coefs_f32[EE_NUM_MFCC_DCT_COEFS];

#define EE_NUM_MFCC_WIN_COEFS 640
extern const ee_f32_t ee_mfcc_window_coefs_f32[EE_NUM_MFCC_WIN_COEFS];

#define EE_NUM_MFCC_FILTER_CONFIG 40
extern const uint32_t ee_mfcc_filter_pos[EE_NUM_MFCC_FILTER_CONFIG];
extern const uint32_t ee_mfcc_filter_len[EE_NUM_MFCC_FILTER_CONFIG];

#define EE_NUM_MFCC_FILTER_COEFS 493
extern const ee_f32_t ee_mfcc_filter_coefs_f32[EE_NUM_MFCC_FILTER_COEFS];

#endif
