/* speex_kernels.h t be optimized for each platforms */

#if defined(USE_CMSIS_DSP)
    #define filter_dc_notch16       arm_filter_dc_notch16
    #define mdf_inner_prod          arm_mdf_inner_prod
    #define power_spectrum          arm_power_spectrum
    #define power_spectrum_accum    arm_power_spectrum_accum
    #define spectral_mul_accum      arm_spectral_mul_accum
    #define spectral_mul_accum16    arm_spectral_mul_accum16
    #define weighted_spectral_mul_conj arm_weighted_spectral_mul_conj
    #define mdf_adjust_prop         arm_mdf_adjust_prop

    extern void         arm_filter_dc_notch16(const spx_int16_t *in, spx_word16_t radius, spx_word16_t *out, int len, spx_mem_t *mem, int stride);
    extern spx_word32_t arm_mdf_inner_prod(const spx_word16_t *x, const spx_word16_t *y, int len);
    extern void         arm_power_spectrum(const spx_word16_t *X, spx_word32_t *ps, int N);
    extern void         arm_power_spectrum_accum(const spx_word16_t *X, spx_word32_t *ps, int N);
    extern void         arm_spectral_mul_accum(const spx_word16_t *X, const spx_word32_t *Y, spx_word16_t *acc, int N, int M);
    extern void         arm_spectral_mul_accum16(const spx_word16_t *X, const spx_word16_t *Y, spx_word16_t *acc, int N, int M);
    extern void         arm_weighted_spectral_mul_conj(const spx_float_t *w, const spx_float_t p, const spx_word16_t *X, const spx_word16_t *Y, spx_word32_t *prod, int N);
    extern void         arm_mdf_adjust_prop(const spx_word32_t *W, int N, int M, int P, spx_word16_t *prop);

#else
    #define filter_dc_notch16       _filter_dc_notch16
    #define mdf_inner_prod          _mdf_inner_prod
    #define power_spectrum          _power_spectrum
    #define power_spectrum_accum    _power_spectrum_accum
    #define spectral_mul_accum      _spectral_mul_accum
    #define spectral_mul_accum16    _spectral_mul_accum16
    #define weighted_spectral_mul_conj _weighted_spectral_mul_conj
    #define mdf_adjust_prop         _mdf_adjust_prop
#endif
