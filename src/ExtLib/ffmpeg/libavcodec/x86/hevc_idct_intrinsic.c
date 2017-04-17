#include "config.h"
#include "libavutil/avassert.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/hevc.h"
#include "libavcodec/x86/hevcdsp.h"

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC target("sse2")
#endif

#if HAVE_SSE2
#include <emmintrin.h>
#endif

DECLARE_ALIGNED(16, static const int16_t, transform4x4_luma[8][8] )=
{
    {   29, +84, 29,  +84,  29, +84,  29, +84 },
    {  +74, +55, +74, +55, +74, +55, +74, +55 },
    {   55, -29,  55, -29,  55, -29,  55, -29 },
    {  +74, -84, +74, -84, +74, -84, +74, -84 },
    {   74, -74,  74, -74,  74, -74,  74, -74 },
    {    0, +74,   0, +74,   0, +74,   0, +74 },
    {   84, +55,  84, +55,  84, +55,  84, +55 },
    {  -74, -29, -74, -29, -74, -29, -74, -29 }
};

#if HAVE_SSE2

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define LOAD4x4(dst, src)                                                      \
    dst ## 0 = _mm_load_si128((__m128i *) &src[0]);                            \
    dst ## 1 = _mm_load_si128((__m128i *) &src[8])

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define TRANSPOSE4X4_16(dst)                                                   \
    tmp0 = _mm_unpacklo_epi16(dst ## 0, dst ## 1);                             \
    tmp1 = _mm_unpackhi_epi16(dst ## 0, dst ## 1);                             \
    dst ## 0 = _mm_unpacklo_epi16(tmp0, tmp1);                                 \
    dst ## 1 = _mm_unpackhi_epi16(tmp0, tmp1)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_4x4_luma_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define COMPUTE_LUMA(dst , idx)                                                \
    tmp0 = _mm_load_si128((__m128i *) (transform4x4_luma[idx  ]));             \
    tmp1 = _mm_load_si128((__m128i *) (transform4x4_luma[idx+1]));             \
    tmp0 = _mm_madd_epi16(src0, tmp0);                                         \
    tmp1 = _mm_madd_epi16(src1, tmp1);                                         \
    dst  = _mm_add_epi32(tmp0, tmp1);                                          \
    dst  = _mm_add_epi32(dst, add);                                            \
    dst  = _mm_srai_epi32(dst, shift)
#define COMPUTE_LUMA_ALL()                                                     \
    add  = _mm_set1_epi32(1 << (shift - 1));                                   \
    src0 = _mm_unpacklo_epi16(tmp0, tmp1);                                     \
    src1 = _mm_unpackhi_epi16(tmp0, tmp1);                                     \
    COMPUTE_LUMA(res2 , 0);                                                    \
    COMPUTE_LUMA(res3 , 2);                                                    \
    res0 = _mm_packs_epi32(res2, res3);                                        \
    COMPUTE_LUMA(res2 , 4);                                                    \
    COMPUTE_LUMA(res3 , 6);                                                    \
    res1 = _mm_packs_epi32(res2, res3)

#define TRANSFORM_LUMA(D)                                                      \
void ff_hevc_transform_4x4_luma ## _ ## D ## _sse2(int16_t *res) {             \
    uint8_t  shift = 7;                                                        \
    int16_t *src    = res;                                                     \
    int16_t *coeffs = res;                                                     \
    __m128i res0, res1, res2, res3;                                            \
    __m128i tmp0, tmp1, src0, src1, add;                                       \
    LOAD4x4(tmp, src);                                                         \
    COMPUTE_LUMA_ALL();                                                        \
    shift = 20 - D;                                                            \
    res2  = _mm_unpacklo_epi16(res0, res1);                                    \
    res3  = _mm_unpackhi_epi16(res0, res1);                                    \
    tmp0  = _mm_unpacklo_epi16(res2, res3);                                    \
    tmp1  = _mm_unpackhi_epi16(res2, res3);                                    \
    COMPUTE_LUMA_ALL();                                                        \
    TRANSPOSE4X4_16(res);                                                      \
    _mm_store_si128((__m128i *) res    , res0);                                \
    _mm_store_si128((__m128i *) (res + 8), res1);                              \
}

TRANSFORM_LUMA( 8);
TRANSFORM_LUMA( 10);
TRANSFORM_LUMA( 12);

#endif

#ifdef __GNUC__
#pragma GCC pop_options
#endif
