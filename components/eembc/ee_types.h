#ifndef __EE_TYPES_H
#define __EE_TYPES_H

#include "th_types.h"
#include <inttypes.h>

typedef TH_FLOAT32_TYPE               ee_f32_t;
typedef TH_RFFT_INSTANCE_FLOAT32_TYPE ee_rfft_f32_t;
typedef TH_CFFT_INSTANCE_FLOAT32_TYPE ee_cfft_f32_t;

typedef enum
{
    EE_STATUS_OK = 0,
    EE_STATUS_ERROR
} ee_status_t;

/* Because we expect a particular nomemclature in our MFCC function: */
typedef struct
{
    uint16_t  numRows; /**< number of rows of the matrix.     */
    uint16_t  numCols; /**< number of columns of the matrix.  */
    ee_f32_t *pData;   /**< points to the data of the matrix. */
} ee_matrix_f32_t;

#endif /* __EE_TYPES_H */