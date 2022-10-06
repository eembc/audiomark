

#ifndef _ARM_ABS_MAX_H
#define _ARM_ABS_MAX_H


#include "arm_math.h"
#include "arm_math_f16.h"

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION < 6000000)
    typedef float32_t float16_t;
#endif
 

#define satabs_q31(in)      (in > 0) ? in : (q31_t)__QSUB(0, in);
#define satabs_q15(in)      (in > 0) ? in : (q15_t)__QSUB16(0, in)
#define satabs_f32(in)      (in > 0) ? in : -in;

#define update_max(x,y)     (x > y)
#define update_min(x,y)     (x < y)
#define update_absmax(x,y)  (x > y)
#define update_absmin(x,y)  (x < y)


#define abs_WITH_IDX1(op, TYP, prefix, VARTYP)                                              \
    VARTYP   cur_##op, out;       /* Temporary variables to store the output value. */\
    uint32_t blkCnt, outIndex; /* loop counter */                                  \
                                                                                   \
    /* Initialize the index value to zero. */                                      \
    outIndex = 0U;                                                                 \
    /* Load first input value that act as reference value for comparison */        \
    out = *pSrc++;                                                                 \
    out = satabs##_##prefix(out);                                                  \
                                                                                   \
    blkCnt = (blockSize - 1U);                                                     \
                                                                                   \
    while (blkCnt > 0U)                                                            \
    {                                                                              \
      /* Initialize extremum to the next consecutive values one by one */          \
      cur_##op = *pSrc++;                                                          \
      cur_##op = satabs##_##prefix(cur_##op);                                      \
                                                                                   \
      /* compare for the maximum value */                                          \
      if (update_##op(cur_##op, out))                                              \
      {                                                                            \
        /* Update the extremum value and it's index */                             \
        out = cur_##op;                                                            \
        outIndex = blockSize - blkCnt;                                             \
      }                                                                            \
                                                                                   \
      /* Decrement the loop counter */                                             \
      blkCnt--;                                                                    \
    }                                                                              \
                                                                                   \
    /* Store the maximum value and it's index into destination pointers */         \
    *pResult = out;                                                                \
    *pIndex = outIndex;


#define abs_WITH_IDX(op, TYP, prefix, VARTYP)                                                               \
{                                                                                                           \
        VARTYP cur_##op, out;                           /* Temporary variables to store the output value. */\
        uint32_t blkCnt, outIndex;                     /* Loop counter */                                   \
        uint32_t index;                                /* index of maximum value */                         \
                                                                                                            \
  /* Initialize index value to zero. */                                                                     \
  outIndex = 0U;                                                                                            \
  /* Load first input value that act as reference value for comparision */                                  \
  out = *pSrc++;                                                                                            \
  out = satabs##_##prefix(out);                                                                             \
  /* Initialize index of extrema value. */                                                                  \
  index = 0U;                                                                                               \
                                                                                                            \
  /* Loop unrolling: Compute 4 outputs at a time */                                                         \
  blkCnt = (blockSize - 1U) >> 2U;                                                                          \
                                                                                                            \
  while (blkCnt > 0U)                                                                                       \
  {                                                                                                         \
    /* Initialize cur_##op to next consecutive values one by one */                                         \
    cur_##op = *pSrc++;                                                                                     \
    cur_##op = satabs##_##prefix(cur_##op);                                                                 \
    /* compare for the extrema value */                                                                     \
    if (update_##op(cur_##op, out))                                                                         \
    {                                                                                                       \
      /* Update the extrema value and it's index */                                                         \
      out = cur_##op;                                                                                       \
      outIndex = index + 1U;                                                                                \
    }                                                                                                       \
                                                                                                            \
    cur_##op = *pSrc++;                                                                                     \
    cur_##op = satabs##_##prefix(cur_##op);                                                                 \
    if (update_##op(cur_##op, out))                                                                         \
    {                                                                                                       \
      out = cur_##op;                                                                                       \
      outIndex = index + 2U;                                                                                \
    }                                                                                                       \
                                                                                                            \
    cur_##op = *pSrc++;                                                                                     \
    cur_##op = satabs##_##prefix(cur_##op);                                                                 \
    if (update_##op(cur_##op, out))                                                                         \
    {                                                                                                       \
      out = cur_##op;                                                                                       \
      outIndex = index + 3U;                                                                                \
    }                                                                                                       \
                                                                                                            \
    cur_##op = *pSrc++;                                                                                     \
    cur_##op = satabs##_##prefix(cur_##op);                                                                 \
    if (update_##op(cur_##op, out))                                                                         \
    {                                                                                                       \
      out = cur_##op;                                                                                       \
      outIndex = index + 4U;                                                                                \
    }                                                                                                       \
                                                                                                            \
    index += 4U;                                                                                            \
                                                                                                            \
    /* Decrement loop counter */                                                                            \
    blkCnt--;                                                                                               \
  }                                                                                                         \
                                                                                                            \
  /* Loop unrolling: Compute remaining outputs */                                                           \
  blkCnt = (blockSize - 1U) % 4U;                                                                           \
                                                                                                            \
                                                                                                            \
  while (blkCnt > 0U)                                                                                       \
  {                                                                                                         \
    cur_##op = *pSrc++;                                                                                     \
    cur_##op = satabs##_##prefix(cur_##op);                                                                 \
    if (update_##op(cur_##op, out))                                                                         \
    {                                                                                                       \
      out = cur_##op;                                                                                       \
      outIndex = blockSize - blkCnt;                                                                        \
    }                                                                                                       \
                                                                                                            \
    /* Decrement loop counter */                                                                            \
    blkCnt--;                                                                                               \
  }                                                                                                         \
                                                                                                            \
  /* Store the extrema value and it's index into destination pointers */                                    \
  *pResult = out;                                                                                           \
  *pIndex = outIndex;                                                                                       \
}


void      arm_absmax_f32(const float32_t * pSrc, uint32_t blockSize, float32_t * pResult, uint32_t * pIndex);
void      arm_absmax_q31(const q31_t * pSrc, uint32_t blockSize, q31_t * pResult, uint32_t * pIndex);
void      arm_absmax_q15(const q15_t * pSrc, uint32_t blockSize, q15_t * pResult, uint32_t * pIndex);

#endif
