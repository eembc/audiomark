/* Copyright (C) 2006 Jean-Marc Valin */
/**
   @file filterbank.c
   @brief Converting between psd and filterbank
 */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/


#include <riscv_vector.h>

#if defined(FLOATING_POINT)

#ifdef OVERRIDE_FB_COMPUTE_BANK32

void filterbank_compute_bank32(FilterBank *bank, spx_word32_t *ps, spx_word32_t *mel)
{
   int i;
   /* clear histoTmp */
   size_t len = bank->nb_banks;
   spx_word32_t *pMel = mel;
   size_t vlmax = __riscv_vsetvlmax_e32m2();
   vfloat32m2_t vZero = __riscv_vfmv_v_f_f32m2(0.0f, vlmax);
   while(len > 0){
       size_t vl = __riscv_vsetvl_e32m2(len);
       __riscv_vse32_v_f32m2(pMel, vZero, vl);
       pMel += vl;
       len  -= vl;
   }

   int * pBankL = bank->bank_left;
   int * pBankR = bank->bank_right;
   spx_word16_t * pFiltL = bank->filter_left;
   spx_word16_t * pFiltR = bank->filter_right;
   spx_word32_t * pPs = ps;

   for (i = 0; i < bank->len; i++)
   {
      spx_word32_t ps_val = *pPs++;
      int idL = *pBankL++;
      int idR = *pBankR++;

      mel[idL] += MULT16_32_P15(*pFiltL++, ps_val);
      mel[idR] += MULT16_32_P15(*pFiltR++, ps_val);
   }
}

#endif

#ifdef OVERRIDE_FB_COMPUTE_PSD16
void filterbank_compute_psd16(FilterBank * bank, spx_word16_t * mel, spx_word16_t * ps)
{
    uint32_t *pL  = (uint32_t *) bank->bank_left;
    uint32_t *pR  = (uint32_t *) bank->bank_right;
    float *pfiltL = (float *) bank->filter_left;
    float *pfiltR = (float *) bank->filter_right;

    float *pMel = (float *) mel;
    float *pPs  = (float *) ps;

    size_t len = bank->len;

    while (len > 0) {
        size_t vl = __riscv_vsetvl_e32m2(len); 

        vuint32m2_t vIdL     = __riscv_vle32_v_u32m2(pL, vl);
        vuint32m2_t vIdR     = __riscv_vle32_v_u32m2(pR, vl);

        vuint32m2_t vOffsetL = __riscv_vsll_vx_u32m2(vIdL, 2, vl);
        vuint32m2_t vOffsetR = __riscv_vsll_vx_u32m2(vIdR, 2, vl);

        vfloat32m2_t vMelL   = __riscv_vluxei32_v_f32m2(pMel, vOffsetL, vl);
        vfloat32m2_t vFiltL  = __riscv_vle32_v_f32m2(pfiltL, vl);

        vfloat32m2_t vTmp    = __riscv_vfmul_vv_f32m2(vMelL, vFiltL, vl);

        vfloat32m2_t vMelR   = __riscv_vluxei32_v_f32m2(pMel, vOffsetR, vl);
        vfloat32m2_t vFiltR  = __riscv_vle32_v_f32m2(pfiltR, vl);

        vTmp = __riscv_vfmacc_vv_f32m2(vTmp, vMelR, vFiltR, vl);

        __riscv_vse32_v_f32m2(pPs, vTmp, vl);

        pL     += vl;
        pR     += vl;
        pfiltL += vl;
        pfiltR += vl;
        pPs    += vl;
        len    -= vl;
    }
}
#endif

#else

/* FIXED_POINT not needed for EEMBC AudioMark */
#error "Fixed Point FilterBank Vector Optimization is not available"

#endif

