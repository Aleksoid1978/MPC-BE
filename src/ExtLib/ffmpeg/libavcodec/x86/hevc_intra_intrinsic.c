#include "config.h"
#include "libavutil/avassert.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/hevc.h"
#include "libavcodec/x86/hevcpred.h"

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC target("sse4.2")
#endif

#if HAVE_SSE2
#include <emmintrin.h>
#endif
#if HAVE_SSSE3
#include <tmmintrin.h>
#endif
#if HAVE_SSE42
#include <smmintrin.h>
#endif

#if HAVE_SSE42
#define _MM_PACKUS_EPI32 _mm_packus_epi32
#else
static av_always_inline __m128i _MM_PACKUS_EPI32( __m128i a, __m128i b )
{
     a = _mm_slli_epi32 (a, 16);
     a = _mm_srai_epi32 (a, 16);
     b = _mm_slli_epi32 (b, 16);
     b = _mm_srai_epi32 (b, 16);
     a = _mm_packs_epi32 (a, b);
    return a;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#if HAVE_SSE42
#define PLANAR_INIT_8()                                                        \
    uint8_t *src = (uint8_t*)_src;                                             \
    const uint8_t *top = (const uint8_t*)_top;                                 \
    const uint8_t *left = (const uint8_t*)_left
#define PLANAR_INIT_10()                                                       \
    uint16_t *src = (uint16_t*)_src;                                           \
    const uint16_t *top = (const uint16_t*)_top;                               \
    const uint16_t *left = (const uint16_t*)_left

#define PLANAR_COMPUTE(val, shift)                                             \
    add = _mm_mullo_epi16(_mm_set1_epi16(1+y), l0);                            \
    ly1 = _mm_unpacklo_epi16(ly , ly );                                        \
    ly1 = _mm_unpacklo_epi32(ly1, ly1);                                        \
    ly1 = _mm_unpacklo_epi64(ly1, ly1);                                        \
    c0  = _mm_mullo_epi16(tmp1, ly1);                                          \
    x0  = _mm_mullo_epi16(_mm_set1_epi16(val - y), tx);                        \
    c0  = _mm_add_epi16(c0, c1);                                               \
    x0  = _mm_add_epi16(x0, c0);                                               \
    x0  = _mm_add_epi16(x0, add);                                              \
    c0  = _mm_srli_epi16(x0, shift)

#define PLANAR_COMPUTE_HI(val, shift)                                          \
    C0  = _mm_mullo_epi16(tmp2, ly1);                                          \
    x0  = _mm_mullo_epi16(_mm_set1_epi16(val - y), th);                        \
    C0  = _mm_add_epi16(C0, C1);                                               \
    x0  = _mm_add_epi16(x0, C0);                                               \
    x0  = _mm_add_epi16(x0, add);                                              \
    C0  = _mm_srli_epi16(x0, shift)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define PLANAR_LOAD_0_8()                                                      \
    ly   = _mm_loadl_epi64((__m128i*) left);                                   \
    tx   = _mm_loadl_epi64((__m128i*) top);                                    \
    ly   = _mm_unpacklo_epi8(ly, _mm_setzero_si128());                         \
    tx   = _mm_unpacklo_epi8(tx, _mm_setzero_si128());                         \
    ly   = _mm_unpacklo_epi16(ly, ly);                                         \
    tx   = _mm_unpacklo_epi64(tx, tx)
#define PLANAR_LOAD_0_10()                                                     \
    ly   = _mm_loadl_epi64((__m128i*) left);                                   \
    tx   = _mm_loadl_epi64((__m128i*) top);                                    \
    ly   = _mm_unpacklo_epi16(ly, ly);                                         \
    tx   = _mm_unpacklo_epi64(tx, tx)

#define PLANAR_COMPUTE_0(dst , v1, v2, v3, v4)                                 \
    dst = _mm_mullo_epi16(tmp1, ly1);                                          \
    x0  = _mm_mullo_epi16(_mm_set_epi16(v1,v1,v1,v1,v2,v2,v2,v2), tx);         \
    add = _mm_mullo_epi16(_mm_set_epi16(v3,v3,v3,v3,v4,v4,v4,v4), l0);         \
    dst = _mm_add_epi16(dst, c1);                                              \
    x0  = _mm_add_epi16(x0, add);                                              \
    dst = _mm_add_epi16(dst, x0);                                              \
    dst = _mm_srli_epi16(dst, 3)

#define PLANAR_STORE_0_8()                                                     \
    c0  = _mm_packus_epi16(c0,C0);                                             \
    *((uint32_t *) src              ) = _mm_cvtsi128_si32(c0   );              \
    *((uint32_t *)(src +     stride)) = _mm_extract_epi32(c0, 1);              \
    *((uint32_t *)(src + 2 * stride)) = _mm_extract_epi32(c0, 2);              \
    *((uint32_t *)(src + 3 * stride)) = _mm_extract_epi32(c0, 3)
#define PLANAR_STORE_0_10()                                                    \
    _mm_storel_epi64((__m128i*)(src             ), c0);                        \
    _mm_storel_epi64((__m128i*)(src +     stride), _mm_unpackhi_epi64(c0, c0));\
    _mm_storel_epi64((__m128i*)(src + 2 * stride), C0);                        \
    _mm_storel_epi64((__m128i*)(src + 3 * stride), _mm_unpackhi_epi64(C0, C0))

#define PRED_PLANAR_0(D)                                                       \
void pred_planar_0_ ## D ## _sse(uint8_t *_src, const uint8_t *_top,           \
        const uint8_t *_left, ptrdiff_t stride) {                              \
    __m128i ly, l0, tx, ly1;                                                   \
    __m128i tmp1, add, x0, c0, c1, C0;                                         \
    PLANAR_INIT_ ## D();                                                       \
    tx   = _mm_set1_epi16(top[4]);                                             \
    l0   = _mm_set1_epi16(left[4]);                                            \
    add  = _mm_set1_epi16(4);                                                  \
    tmp1 = _mm_set_epi16(0,1,2,3,0,1,2,3);                                     \
    c1   = _mm_mullo_epi16(_mm_set_epi16(4,3,2,1,4,3,2,1), tx);                \
    c1   = _mm_add_epi16(c1, add);                                             \
    PLANAR_LOAD_0_ ##D();                                                      \
                                                                               \
    ly1 = _mm_unpacklo_epi32(ly, ly);                                          \
    PLANAR_COMPUTE_0(c0, 2, 3, 2, 1);                                          \
    ly1 = _mm_unpackhi_epi32(ly, ly);                                          \
    PLANAR_COMPUTE_0(C0, 0, 1, 4, 3);                                          \
    PLANAR_STORE_0_ ## D();                                                    \
}
PRED_PLANAR_0( 8)
PRED_PLANAR_0(10)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define PLANAR_LOAD_1_8()                                                      \
    ly   = _mm_loadl_epi64((__m128i*)left);                                    \
    tx   = _mm_loadl_epi64((__m128i*)top);                                     \
    ly   = _mm_unpacklo_epi8(ly,_mm_setzero_si128());                          \
    tx   = _mm_unpacklo_epi8(tx,_mm_setzero_si128())
#define PLANAR_LOAD_1_10()                                                     \
    ly   = _mm_loadu_si128((__m128i*)left);                                    \
    tx   = _mm_loadu_si128((__m128i*)top)

#define PLANAR_COMPUTE_1()                                                     \
    PLANAR_COMPUTE(7, 4)

#define PLANAR_STORE_1_8()                                                     \
    c0  = _mm_packus_epi16(c0,_mm_setzero_si128());                            \
    _mm_storel_epi64((__m128i*)(src), c0);                                     \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly,2)
#define PLANAR_STORE_1_10()                                                    \
    _mm_storeu_si128((__m128i*)(src), c0);                                     \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly,2)

#define PRED_PLANAR_1(D)                                                       \
void pred_planar_1_ ## D ## _sse(uint8_t *_src, const uint8_t *_top,           \
        const uint8_t *_left, ptrdiff_t stride) {                              \
    int y;                                                                     \
    __m128i ly, l0, tx, ly1;                                                   \
    __m128i tmp1, add, x0, c0, c1;                                             \
    PLANAR_INIT_ ## D();                                                       \
    tx   = _mm_set1_epi16(top[8]);                                             \
    l0   = _mm_set1_epi16(left[8]);                                            \
    add  = _mm_set1_epi16(8);                                                  \
    tmp1 = _mm_set_epi16(0,1,2,3,4,5,6,7);                                     \
    c1   = _mm_mullo_epi16(_mm_set_epi16(8,7,6,5,4,3,2,1), tx);                \
    c1   = _mm_add_epi16(c1,add);                                              \
    PLANAR_LOAD_1_ ## D();                                                     \
    for (y = 0; y < 8; y++) {                                                  \
        PLANAR_COMPUTE_1();                                                    \
        PLANAR_STORE_1_ ## D();                                                \
    }                                                                          \
}

PRED_PLANAR_1( 8)
PRED_PLANAR_1(10)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define PLANAR_LOAD_2_8()                                                      \
    ly   = _mm_loadu_si128((__m128i*) left);                                   \
    tx   = _mm_loadu_si128((__m128i*) top);                                    \
    lh   = _mm_unpackhi_epi8(ly,_mm_setzero_si128());                          \
    ly   = _mm_unpacklo_epi8(ly,_mm_setzero_si128());                          \
    th   = _mm_unpackhi_epi8(tx,_mm_setzero_si128());                          \
    tx   = _mm_unpacklo_epi8(tx,_mm_setzero_si128())

#define PLANAR_LOAD_2_10()                                                     \
    ly   = _mm_loadu_si128((__m128i*) left);                                   \
    lh   = _mm_loadu_si128((__m128i*)&left[8]);                                \
    tx   = _mm_loadu_si128((__m128i*) top);                                    \
    th   = _mm_loadu_si128((__m128i*)&top[8])

#define PLANAR_COMPUTE_2()                                                     \
    PLANAR_COMPUTE(15, 5)
#define PLANAR_COMPUTE_HI_2()                                                  \
    PLANAR_COMPUTE_HI(15, 5)

#define PLANAR_STORE_2_8()                                                     \
    c0  = _mm_packus_epi16(c0, C0);                                            \
    _mm_storeu_si128((__m128i*) src, c0);                                      \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly,2)
#define PLANAR_STORE_2_10()                                                    \
    _mm_storeu_si128((__m128i*) src   , c0);                                   \
    _mm_storeu_si128((__m128i*)&src[8], C0);                                   \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly,2)

#define PRED_PLANAR_2(D)                                                       \
void pred_planar_2_ ## D ## _sse(uint8_t *_src, const uint8_t *_top,           \
        const uint8_t *_left, ptrdiff_t stride) {                              \
    int y, i;                                                                  \
    __m128i ly, lh, l0, tx, th, ly1;                                           \
    __m128i tmp1, tmp2, add, x0, c0, c1, C0, C1;                               \
    PLANAR_INIT_ ## D();                                                       \
    tx   = _mm_set1_epi16(top[16]);                                            \
    l0   = _mm_set1_epi16(left[16]);                                           \
    add  = _mm_set1_epi16(16);                                                 \
    tmp1 = _mm_set_epi16( 8, 9,10,11,12,13,14,15);                             \
    tmp2 = _mm_set_epi16( 0, 1, 2, 3, 4, 5, 6, 7);                             \
    c1   = _mm_mullo_epi16(_mm_set_epi16( 8, 7, 6, 5, 4, 3, 2, 1), tx);        \
    C1   = _mm_mullo_epi16(_mm_set_epi16(16,15,14,13,12,11,10, 9), tx);        \
    c1   = _mm_add_epi16(c1, add);                                             \
    C1   = _mm_add_epi16(C1, add);                                             \
    PLANAR_LOAD_2_ ## D();                                                     \
    for (i = 0; i < 2; i++) {                                                  \
        for (y = i*8; y < i*8+8; y++) {                                        \
            PLANAR_COMPUTE_2();                                                \
            PLANAR_COMPUTE_HI_2();                                             \
            PLANAR_STORE_2_ ## D();                                            \
        }                                                                      \
        ly = lh;                                                               \
    }                                                                          \
}

PRED_PLANAR_2( 8)
PRED_PLANAR_2(10)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define PLANAR_LOAD_3_8()                                                      \
    ly   = _mm_loadu_si128((__m128i*) left);                                   \
    lh   = _mm_unpackhi_epi8(ly,_mm_setzero_si128());                          \
    ly   = _mm_unpacklo_epi8(ly,_mm_setzero_si128());                          \
    tx   = _mm_loadu_si128((__m128i*) top);                                    \
    th   = _mm_unpackhi_epi8(tx,_mm_setzero_si128());                          \
    tx   = _mm_unpacklo_epi8(tx,_mm_setzero_si128());                          \
    TX   = _mm_loadu_si128((__m128i*)(top + 16));                              \
    TH   = _mm_unpackhi_epi8(TX,_mm_setzero_si128());                          \
    TX   = _mm_unpacklo_epi8(TX,_mm_setzero_si128())
#define PLANAR_LOAD_3_10()                                                     \
    ly   = _mm_loadu_si128((__m128i*) left   );                                \
    lh   = _mm_loadu_si128((__m128i*)&left[8]);                                \
    tx   = _mm_loadu_si128((__m128i*) top    );                                \
    th   = _mm_loadu_si128((__m128i*)&top[ 8]);                                \
    TX   = _mm_loadu_si128((__m128i*)&top[16]);                                \
    TH   = _mm_loadu_si128((__m128i*)&top[24])

#define PLANAR_RELOAD_3_8()                                                    \
    ly = _mm_loadu_si128((__m128i*)(left+16));                                 \
    lh = _mm_unpackhi_epi8(ly,_mm_setzero_si128());                            \
    ly = _mm_unpacklo_epi8(ly,_mm_setzero_si128())
#define PLANAR_RELOAD_3_10()                                                   \
    ly = _mm_loadu_si128((__m128i*)&left[16]);                                 \
    lh = _mm_loadu_si128((__m128i*)&left[24])

#define PLANAR_COMPUTE_3()                                                     \
    PLANAR_COMPUTE(31, 6)
#define PLANAR_COMPUTE_HI_3()                                                  \
    PLANAR_COMPUTE_HI(31, 6)
#define PLANAR_COMPUTE_HI2_3()                                                 \
    c0  = _mm_mullo_epi16(TMP1, ly1);                                          \
    x0  = _mm_mullo_epi16(_mm_set1_epi16(31 - y), TX);                         \
    c0  = _mm_add_epi16(c0, c2);                                               \
    x0  = _mm_add_epi16(x0, c0);                                               \
    x0  = _mm_add_epi16(x0, add);                                              \
    c0  = _mm_srli_epi16(x0, 6)
#define PLANAR_COMPUTE_HI3_3()                                                 \
    C0  = _mm_mullo_epi16(TMP2, ly1);                                          \
    x0  = _mm_mullo_epi16(_mm_set1_epi16(31 - y), TH);                         \
    C0  = _mm_add_epi16(C0, C2);                                               \
    x0  = _mm_add_epi16(x0, C0);                                               \
    x0  = _mm_add_epi16(x0, add);                                              \
    C0  = _mm_srli_epi16(x0, 6)

#define PLANAR_STORE1_3_8()                                                    \
    c0 = _mm_packus_epi16(c0, C0);                                             \
    _mm_storeu_si128((__m128i*) src, c0)
#define PLANAR_STORE2_3_8()                                                    \
    c0  = _mm_packus_epi16(c0, C0);                                            \
    _mm_storeu_si128((__m128i*) (src + 16), c0);                               \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly, 2)

#define PLANAR_STORE1_3_10()                                                   \
    _mm_storeu_si128((__m128i*) src    , c0);                                  \
    _mm_storeu_si128((__m128i*)&src[ 8], C0)
#define PLANAR_STORE2_3_10()                                                   \
    _mm_storeu_si128((__m128i*)&src[16], c0);                                  \
    _mm_storeu_si128((__m128i*)&src[24], C0);                                  \
    src+= stride;                                                              \
    ly  = _mm_srli_si128(ly, 2)


#define PRED_PLANAR_3(D)                                                       \
void pred_planar_3_ ## D ## _sse(uint8_t *_src, const uint8_t *_top,           \
        const uint8_t *_left, ptrdiff_t stride) {                              \
    int y, i;                                                                  \
    __m128i l0, ly, lh, ly1, tx, th, TX, TH, tmp1, tmp2, TMP1, TMP2;           \
    __m128i x0, c0, c1, c2, C0, C1, C2, add;                                   \
    PLANAR_INIT_ ## D();                                                       \
    tx   = _mm_set1_epi16(top[32]);                                            \
    l0   = _mm_set1_epi16(left[32]);                                           \
    add  = _mm_set1_epi16(32);                                                 \
    tmp1 = _mm_set_epi16(24,25,26,27,28,29,30,31);                             \
    tmp2 = _mm_set_epi16(16,17,18,19,20,21,22,23);                             \
    TMP1 = _mm_set_epi16( 8, 9,10,11,12,13,14,15);                             \
    TMP2 = _mm_set_epi16( 0, 1, 2, 3, 4, 5, 6, 7);                             \
    c1   = _mm_mullo_epi16(_mm_set_epi16( 8, 7, 6, 5, 4, 3, 2, 1), tx);        \
    C1   = _mm_mullo_epi16(_mm_set_epi16(16,15,14,13,12,11,10, 9), tx);        \
    c2   = _mm_mullo_epi16(_mm_set_epi16(24,23,22,21,20,19,18,17), tx);        \
    C2   = _mm_mullo_epi16(_mm_set_epi16(32,31,30,29,28,27,26,25), tx);        \
    c1   = _mm_add_epi16(c1, add);                                             \
    C1   = _mm_add_epi16(C1, add);                                             \
    c2   = _mm_add_epi16(c2, add);                                             \
    C2   = _mm_add_epi16(C2, add);                                             \
    PLANAR_LOAD_3_ ## D();                                                     \
    for (i = 0; i < 4; i++) {                                                  \
        for (y = 0+i*8; y < 8+i*8; y++) {                                      \
            PLANAR_COMPUTE_3();                                                \
            PLANAR_COMPUTE_HI_3();                                             \
            PLANAR_STORE1_3_ ## D();                                           \
            PLANAR_COMPUTE_HI2_3();                                            \
            PLANAR_COMPUTE_HI3_3();                                            \
            PLANAR_STORE2_3_ ## D();                                           \
        }                                                                      \
        if (i == 0 || i == 2) {                                                \
            ly = lh;                                                           \
        } else {                                                               \
            PLANAR_RELOAD_3_ ## D();                                           \
        }                                                                      \
    }                                                                          \
}

PRED_PLANAR_3( 8)
PRED_PLANAR_3(10)

#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define STORE8(out, sstep_out)                                                 \
    _mm_storel_epi64((__m128i*)&out[0*sstep_out], m10);                        \
    _mm_storel_epi64((__m128i*)&out[2*sstep_out], m12);                        \
    _mm_storel_epi64((__m128i*)&out[4*sstep_out], m11);                        \
    _mm_storel_epi64((__m128i*)&out[6*sstep_out], m13);                        \
    m10 = _mm_unpackhi_epi64(m10, m10);                                        \
    m12 = _mm_unpackhi_epi64(m12, m12);                                        \
    m11 = _mm_unpackhi_epi64(m11, m11);                                        \
    m13 = _mm_unpackhi_epi64(m13, m13);                                        \
    _mm_storel_epi64((__m128i*)&out[1*sstep_out], m10);                        \
    _mm_storel_epi64((__m128i*)&out[3*sstep_out], m12);                        \
    _mm_storel_epi64((__m128i*)&out[5*sstep_out], m11);                        \
    _mm_storel_epi64((__m128i*)&out[7*sstep_out], m13)

#define STORE16(out, sstep_out)                                                \
    _mm_storeu_si128((__m128i *) &out[0*sstep_out], m0);                       \
    _mm_storeu_si128((__m128i *) &out[1*sstep_out], m1);                       \
    _mm_storeu_si128((__m128i *) &out[2*sstep_out], m2);                       \
    _mm_storeu_si128((__m128i *) &out[3*sstep_out], m3);                       \
    _mm_storeu_si128((__m128i *) &out[4*sstep_out], m4);                       \
    _mm_storeu_si128((__m128i *) &out[5*sstep_out], m5);                       \
    _mm_storeu_si128((__m128i *) &out[6*sstep_out], m6);                       \
    _mm_storeu_si128((__m128i *) &out[7*sstep_out], m7)

#define TRANSPOSE4x4_8(in, sstep_in, out, sstep_out)                           \
    {                                                                          \
        __m128i m0  = _mm_loadl_epi64((__m128i *) &in[0*sstep_in]);            \
        __m128i m1  = _mm_loadl_epi64((__m128i *) &in[1*sstep_in]);            \
        __m128i m2  = _mm_loadl_epi64((__m128i *) &in[2*sstep_in]);            \
        __m128i m3  = _mm_loadl_epi64((__m128i *) &in[3*sstep_in]);            \
                                                                               \
        __m128i m10 = _mm_unpacklo_epi8(m0, m1);                               \
        __m128i m11 = _mm_unpacklo_epi8(m2, m3);                               \
                                                                               \
        m0  = _mm_unpacklo_epi16(m10, m11);                                    \
                                                                               \
        *((uint32_t *) (out+0*sstep_out)) =_mm_cvtsi128_si32(m0);              \
        *((uint32_t *) (out+1*sstep_out)) =_mm_extract_epi32(m0, 1);           \
        *((uint32_t *) (out+2*sstep_out)) =_mm_extract_epi32(m0, 2);           \
        *((uint32_t *) (out+3*sstep_out)) =_mm_extract_epi32(m0, 3);           \
    }
#define TRANSPOSE8x8_8(in, sstep_in, out, sstep_out)                           \
    {                                                                          \
        __m128i m0  = _mm_loadl_epi64((__m128i *) &in[0*sstep_in]);            \
        __m128i m1  = _mm_loadl_epi64((__m128i *) &in[1*sstep_in]);            \
        __m128i m2  = _mm_loadl_epi64((__m128i *) &in[2*sstep_in]);            \
        __m128i m3  = _mm_loadl_epi64((__m128i *) &in[3*sstep_in]);            \
        __m128i m4  = _mm_loadl_epi64((__m128i *) &in[4*sstep_in]);            \
        __m128i m5  = _mm_loadl_epi64((__m128i *) &in[5*sstep_in]);            \
        __m128i m6  = _mm_loadl_epi64((__m128i *) &in[6*sstep_in]);            \
        __m128i m7  = _mm_loadl_epi64((__m128i *) &in[7*sstep_in]);            \
                                                                               \
        __m128i m10 = _mm_unpacklo_epi8(m0, m1);                               \
        __m128i m11 = _mm_unpacklo_epi8(m2, m3);                               \
        __m128i m12 = _mm_unpacklo_epi8(m4, m5);                               \
        __m128i m13 = _mm_unpacklo_epi8(m6, m7);                               \
                                                                               \
        m0  = _mm_unpacklo_epi16(m10, m11);                                    \
        m1  = _mm_unpacklo_epi16(m12, m13);                                    \
        m2  = _mm_unpackhi_epi16(m10, m11);                                    \
        m3  = _mm_unpackhi_epi16(m12, m13);                                    \
                                                                               \
        m10 = _mm_unpacklo_epi32(m0 , m1 );                                    \
        m11 = _mm_unpacklo_epi32(m2 , m3 );                                    \
        m12 = _mm_unpackhi_epi32(m0 , m1 );                                    \
        m13 = _mm_unpackhi_epi32(m2 , m3 );                                    \
                                                                               \
        STORE8(out, sstep_out);                                                \
    }
#define TRANSPOSE16x16_8(in, sstep_in, out, sstep_out)                        \
    for (y = 0; y < sstep_in; y+=8)                                           \
        for (x = 0; x < sstep_in; x+=8)                                       \
            TRANSPOSE8x8_8((&in[y*sstep_in+x]), sstep_in, (&out[x*sstep_out+y]), sstep_out)
#define TRANSPOSE32x32_8(in, sstep_in, out, sstep_out)                        \
    for (y = 0; y < sstep_in; y+=8)                                           \
        for (x = 0; x < sstep_in; x+=8)                                       \
            TRANSPOSE8x8_8((&in[y*sstep_in+x]), sstep_in, (&out[x*sstep_out+y]), sstep_out)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define TRANSPOSE4x4_10(in, sstep_in, out, sstep_out)                          \
    {                                                                          \
        __m128i m0  = _mm_loadl_epi64((__m128i *) &in[0*sstep_in]);            \
        __m128i m1  = _mm_loadl_epi64((__m128i *) &in[1*sstep_in]);            \
        __m128i m2  = _mm_loadl_epi64((__m128i *) &in[2*sstep_in]);            \
        __m128i m3  = _mm_loadl_epi64((__m128i *) &in[3*sstep_in]);            \
                                                                               \
        __m128i m10 = _mm_unpacklo_epi16(m0, m1);                              \
        __m128i m11 = _mm_unpacklo_epi16(m2, m3);                              \
                                                                               \
        m0  = _mm_unpacklo_epi32(m10, m11);                                    \
        m1  = _mm_unpackhi_epi32(m10, m11);                                    \
                                                                               \
        _mm_storel_epi64((__m128i *) (out+0*sstep_out) , m0);                  \
        _mm_storel_epi64((__m128i *) (out+1*sstep_out) , _mm_unpackhi_epi64(m0, m0));\
        _mm_storel_epi64((__m128i *) (out+2*sstep_out) , m1);                  \
        _mm_storel_epi64((__m128i *) (out+3*sstep_out) , _mm_unpackhi_epi64(m1, m1));\
    }
#define TRANSPOSE8x8_10(in, sstep_in, out, sstep_out)                          \
    {                                                                          \
        __m128i tmp0, tmp1, tmp2, tmp3, src0, src1, src2, src3;                \
        __m128i m0  = _mm_loadu_si128((__m128i *) &in[0*sstep_in]);            \
        __m128i m1  = _mm_loadu_si128((__m128i *) &in[1*sstep_in]);            \
        __m128i m2  = _mm_loadu_si128((__m128i *) &in[2*sstep_in]);            \
        __m128i m3  = _mm_loadu_si128((__m128i *) &in[3*sstep_in]);            \
        __m128i m4  = _mm_loadu_si128((__m128i *) &in[4*sstep_in]);            \
        __m128i m5  = _mm_loadu_si128((__m128i *) &in[5*sstep_in]);            \
        __m128i m6  = _mm_loadu_si128((__m128i *) &in[6*sstep_in]);            \
        __m128i m7  = _mm_loadu_si128((__m128i *) &in[7*sstep_in]);            \
                                                                               \
        tmp0 = _mm_unpacklo_epi16(m0, m1);                                     \
        tmp1 = _mm_unpacklo_epi16(m2, m3);                                     \
        tmp2 = _mm_unpacklo_epi16(m4, m5);                                     \
        tmp3 = _mm_unpacklo_epi16(m6, m7);                                     \
        src0 = _mm_unpacklo_epi32(tmp0, tmp1);                                 \
        src1 = _mm_unpacklo_epi32(tmp2, tmp3);                                 \
        src2 = _mm_unpackhi_epi32(tmp0, tmp1);                                 \
        src3 = _mm_unpackhi_epi32(tmp2, tmp3);                                 \
        tmp0 = _mm_unpackhi_epi16(m0, m1);                                     \
        tmp1 = _mm_unpackhi_epi16(m2, m3);                                     \
        tmp2 = _mm_unpackhi_epi16(m4, m5);                                     \
        tmp3 = _mm_unpackhi_epi16(m6, m7);                                     \
        m0   = _mm_unpacklo_epi64(src0 , src1);                                \
        m1   = _mm_unpackhi_epi64(src0 , src1);                                \
        m2   = _mm_unpacklo_epi64(src2 , src3);                                \
        m3   = _mm_unpackhi_epi64(src2 , src3);                                \
        src0 = _mm_unpacklo_epi32(tmp0, tmp1);                                 \
        src1 = _mm_unpacklo_epi32(tmp2, tmp3);                                 \
        src2 = _mm_unpackhi_epi32(tmp0, tmp1);                                 \
        src3 = _mm_unpackhi_epi32(tmp2, tmp3);                                 \
        m4   = _mm_unpacklo_epi64(src0 , src1);                                \
        m5   = _mm_unpackhi_epi64(src0 , src1);                                \
        m6   = _mm_unpacklo_epi64(src2 , src3);                                \
        m7   = _mm_unpackhi_epi64(src2 , src3);                                \
        STORE16(out, sstep_out);                                               \
    }
#define TRANSPOSE16x16_10(in, sstep_in, out, sstep_out)                        \
    for (y = 0; y < sstep_in; y+=8)                                           \
        for (x = 0; x < sstep_in; x+=8)                                       \
            TRANSPOSE8x8_10((&in[y*sstep_in+x]), sstep_in, (&out[x*sstep_out+y]), sstep_out)
#define TRANSPOSE32x32_10(in, sstep_in, out, sstep_out)                        \
    for (y = 0; y < sstep_in; y+=8)                                           \
        for (x = 0; x < sstep_in; x+=8)                                       \
            TRANSPOSE8x8_10((&in[y*sstep_in+x]), sstep_in, (&out[x*sstep_out+y]), sstep_out)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define ANGULAR_COMPUTE_8(W)                                                   \
    for (x = 0; x < W; x += 8) {                                               \
        r3 = _mm_set1_epi16((fact << 8) + (32 - fact));                        \
        r1 = _mm_loadu_si128((__m128i*)(&ref[x+idx+1]));                       \
        r0 = _mm_srli_si128(r1, 1);                                            \
        r1 = _mm_unpacklo_epi8(r1, r0);                                        \
        r1 = _mm_maddubs_epi16(r1, r3);                                        \
        r1 = _mm_mulhrs_epi16(r1, _mm_set1_epi16(1024));                                           \
        r1 = _mm_packus_epi16(r1, r1);                                         \
        _mm_storel_epi64((__m128i *) &p_src[x], r1);                           \
    }


#define ANGULAR_COMPUTE4_8()                                                   \
    r3 = _mm_set1_epi16((fact << 8) + (32 - fact));                            \
    r1 = _mm_loadu_si128((__m128i*)(&ref[idx+1]));                             \
    r0 = _mm_srli_si128(r1, 1);                                                \
    r1 = _mm_unpacklo_epi8(r1, r0);                                            \
    r1 = _mm_maddubs_epi16(r1, r3);                                            \
    r1 = _mm_mulhrs_epi16(r1, _mm_set1_epi16(1024));                                           \
    r1 = _mm_packus_epi16(r1, r1);                                             \
    *((uint32_t *)p_src) = _mm_cvtsi128_si32(r1)
#define ANGULAR_COMPUTE8_8()     ANGULAR_COMPUTE_8( 8)
#define ANGULAR_COMPUTE16_8()    ANGULAR_COMPUTE_8(16)
#define ANGULAR_COMPUTE32_8()    ANGULAR_COMPUTE_8(32)

#define ANGULAR_COMPUTE_ELSE4_8()                                              \
    r1 = _mm_loadl_epi64((__m128i*) &ref[idx+1]);                              \
    *((uint32_t *)p_src) = _mm_cvtsi128_si32(r1)
#define ANGULAR_COMPUTE_ELSE8_8()                                              \
    r1 = _mm_loadl_epi64((__m128i*) &ref[idx+1]);                              \
    _mm_storel_epi64((__m128i *) p_src, r1)
#define ANGULAR_COMPUTE_ELSE16_8()                                             \
    r1 = _mm_loadu_si128((__m128i*) &ref[idx+1]);                              \
    _mm_storeu_si128((__m128i *) p_src, r1)
#define ANGULAR_COMPUTE_ELSE32_8()                                             \
    r1 = _mm_loadu_si128((__m128i*) &ref[idx+1]);                              \
    _mm_storeu_si128((__m128i *) p_src ,r1);                                   \
    r1 = _mm_loadu_si128((__m128i*) &ref[idx+17]);                             \
    _mm_storeu_si128((__m128i *)&p_src[16] ,r1)

#define CLIP_PIXEL(src1, src2)                                                 \
    r3  = _mm_loadu_si128((__m128i*)src1);                                     \
    r1  = _mm_set1_epi16(src1[-1]);                                            \
    r2  = _mm_set1_epi16(src2[0]);                                             \
    r0  = _mm_unpacklo_epi8(r3,_mm_setzero_si128());                           \
    r0  = _mm_subs_epi16(r0, r1);                                              \
    r0  = _mm_srai_epi16(r0, 1);                                               \
    r0  = _mm_add_epi16(r0, r2)
#define CLIP_PIXEL_HI()                                                        \
    r3  = _mm_unpackhi_epi8(r3,_mm_setzero_si128());                           \
    r3  = _mm_subs_epi16(r3, r1);                                              \
    r3  = _mm_srai_epi16(r3, 1);                                               \
    r3  = _mm_add_epi16(r3, r2)

#define CLIP_PIXEL1_4_8()                                                      \
    p_src = src;                                                               \
    CLIP_PIXEL(src2, src1);                                                    \
    r0  = _mm_packus_epi16(r0, r0);                                            \
    *((char *) p_src) = _mm_extract_epi8(r0, 0);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 1);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 2);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 3)
#define CLIP_PIXEL1_8_8()                                                      \
    CLIP_PIXEL1_4_8();                                                         \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 4);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 5);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 6);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 7)
#define CLIP_PIXEL1_16_8()                                                     \
    p_src = src;                                                               \
    CLIP_PIXEL(src2, src1);                                                    \
    CLIP_PIXEL_HI();                                                           \
    r0  = _mm_packus_epi16(r0, r3);                                            \
    *((char *) p_src) = _mm_extract_epi8(r0, 0);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 1);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 2);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 3);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 4);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 5);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 6);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 7);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 8);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0, 9);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,10);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,11);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,12);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,13);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,14);                               \
    p_src += stride;                                                           \
    *((char *) p_src) = _mm_extract_epi8(r0,15)
#define CLIP_PIXEL1_32_8()

#define CLIP_PIXEL2_4_8()                                                      \
    CLIP_PIXEL(src2, src1);                                                    \
    r0  = _mm_packus_epi16(r0, r0);                                            \
    *((uint32_t *)_src) = _mm_cvtsi128_si32(r0)
#define CLIP_PIXEL2_8_8()                                                      \
    CLIP_PIXEL(src2, src1);                                                    \
    r0  = _mm_packus_epi16(r0, r0);                                            \
    _mm_storel_epi64((__m128i*)_src, r0)
#define CLIP_PIXEL2_16_8()                                                     \
    CLIP_PIXEL(src2, src1);                                                    \
    CLIP_PIXEL_HI();                                                           \
    r0  = _mm_packus_epi16(r0, r3);                                            \
    _mm_storeu_si128((__m128i*) _src , r0)
#define CLIP_PIXEL2_32_8()

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#if HAVE_SSE42
#define ANGULAR_COMPUTE_10(W)                                                  \
    for (x = 0; x < W; x += 4) {                                               \
        r3 = _mm_set1_epi32((fact << 16) + (32 - fact));                       \
        r1 = _mm_loadu_si128((__m128i*)(&ref[x+idx+1]));                       \
        r0 = _mm_srli_si128(r1, 2);                                            \
        r1 = _mm_unpacklo_epi16(r1, r0);                                       \
        r1 = _mm_madd_epi16(r1, r3);                                           \
        r1 = _mm_mulhrs_epi16(r1, _mm_set1_epi16(1024));                                           \
        r1 = _MM_PACKUS_EPI32(r1, r1);                                         \
        _mm_storel_epi64((__m128i *) &p_src[x], r1);                           \
    }
#define ANGULAR_COMPUTE4_10()    ANGULAR_COMPUTE_10( 4)
#define ANGULAR_COMPUTE8_10()    ANGULAR_COMPUTE_10( 8)
#define ANGULAR_COMPUTE16_10()   ANGULAR_COMPUTE_10(16)
#define ANGULAR_COMPUTE32_10()   ANGULAR_COMPUTE_10(32)

#define ANGULAR_COMPUTE_ELSE_10(W)                                             \
    for (x = 0; x < W; x += 8) {                                               \
        r1 = _mm_loadu_si128((__m128i*)(&ref[x+idx+1]));                       \
        _mm_storeu_si128((__m128i *) &p_src[x], r1);                           \
    }

#define ANGULAR_COMPUTE_ELSE4_10()                                             \
    r1 = _mm_loadl_epi64((__m128i*)(&ref[idx+1]));                             \
    _mm_storel_epi64((__m128i *) p_src, r1)

#define ANGULAR_COMPUTE_ELSE8_10()      ANGULAR_COMPUTE_ELSE_10(8)
#define ANGULAR_COMPUTE_ELSE16_10()     ANGULAR_COMPUTE_ELSE_10(16)
#define ANGULAR_COMPUTE_ELSE32_10()     ANGULAR_COMPUTE_ELSE_10(32)

#define CLIP_PIXEL_10()                                                        \
    r0  = _mm_loadu_si128((__m128i*)src2);                                     \
    r1  = _mm_set1_epi16(src2[-1]);                                            \
    r2  = _mm_set1_epi16(src1[0]);                                             \
    r0  = _mm_subs_epi16(r0, r1);                                              \
    r0  = _mm_srai_epi16(r0, 1);                                               \
    r0  = _mm_add_epi16(r0, r2)
#define CLIP_PIXEL_HI_10()                                                     \
    r3  = _mm_loadu_si128((__m128i*)&src2[8]);                                 \
    r3  = _mm_subs_epi16(r3, r1);                                              \
    r3  = _mm_srai_epi16(r3, 1);                                               \
    r3  = _mm_add_epi16(r3, r2)

#define CLIP_PIXEL1_4_10()                                                     \
    p_src = src;                                                               \
    CLIP_PIXEL_10();                                                           \
    r0  = _mm_max_epi16(r0, _mm_setzero_si128());                              \
    r0  = _mm_min_epi16(r0, _mm_set1_epi16(0x03ff));                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 0);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 1);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 2);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 3)
#define CLIP_PIXEL1_8_10()                                                     \
    CLIP_PIXEL1_4_10();                                                        \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 4);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 5);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 6);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 7)
#define CLIP_PIXEL1_16_10()                                                    \
    p_src = src;                                                               \
    CLIP_PIXEL_10();                                                           \
    CLIP_PIXEL_HI_10();                                                        \
    r0  = _mm_max_epi16(r0, _mm_setzero_si128());                              \
    r0  = _mm_min_epi16(r0, _mm_set1_epi16(0x03ff));                           \
    r3  = _mm_max_epi16(r3, _mm_setzero_si128());                              \
    r3  = _mm_min_epi16(r3, _mm_set1_epi16(0x03ff));                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 0);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 1);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 2);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 3);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 4);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 5);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 6);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r0, 7);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 0);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 1);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 2);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 3);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 4);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 5);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 6);                          \
    p_src += stride;                                                           \
    *((uint16_t *) p_src) = _mm_extract_epi16(r3, 7)
#define CLIP_PIXEL1_32_10()

#define CLIP_PIXEL2_4_10()                                                     \
    CLIP_PIXEL_10();                                                           \
    r0  = _mm_max_epi16(r0, _mm_setzero_si128());                              \
    r0  = _mm_min_epi16(r0, _mm_set1_epi16(0x03ff));                           \
    _mm_storel_epi64((__m128i*) _src    , r0)
#define CLIP_PIXEL2_8_10()                                                     \
    CLIP_PIXEL_10();                                                           \
    r0  = _mm_max_epi16(r0, _mm_setzero_si128());                              \
    r0  = _mm_min_epi16(r0, _mm_set1_epi16(0x03ff));                           \
    _mm_storeu_si128((__m128i*) _src    , r0)
#define CLIP_PIXEL2_16_10()                                                    \
    CLIP_PIXEL_10();                                                           \
    CLIP_PIXEL_HI_10();                                                        \
    r0  = _mm_max_epi16(r0, _mm_setzero_si128());                              \
    r0  = _mm_min_epi16(r0, _mm_set1_epi16(0x03ff));                           \
    r3  = _mm_max_epi16(r3, _mm_setzero_si128());                              \
    r3  = _mm_min_epi16(r3, _mm_set1_epi16(0x03ff));                           \
    _mm_storeu_si128((__m128i*) p_out    , r0);                                \
    _mm_storeu_si128((__m128i*) &p_out[8], r3);

#define CLIP_PIXEL2_32_10()

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define PRED_ANGULAR_INIT_8(W)                                                 \
    const uint8_t *src1;                                                       \
    const uint8_t *src2;                                                       \
    uint8_t       *ref, *p_src, *src, *p_out;                                  \
    uint8_t        src_tmp[W*W];                                               \
    if (mode >= 18) {                                                          \
        src1   = (const uint8_t*) _top;                                        \
        src2   = (const uint8_t*) _left;                                       \
        src    = (uint8_t*) _src;                                              \
        stride = _stride;                                                      \
        p_src  = src;                                                          \
    } else {                                                                   \
        src1   = (const uint8_t*) _left;                                       \
        src2   = (const uint8_t*) _top;                                        \
        src    = &src_tmp[0];                                                  \
        stride = W;                                                            \
        p_src  = src;                                                          \
    }                                                                          \
    p_out  = (uint8_t*) _src;                                                  \
    ref = (uint8_t*) (src1 - 1)
#define PRED_ANGULAR_INIT_10(W)                                                \
    const uint16_t *src1;                                                      \
    const uint16_t *src2;                                                      \
    uint16_t       *ref, *p_src, *src, *p_out;                                 \
    uint16_t        src_tmp[W*W];                                              \
    if (mode >= 18) {                                                          \
        src1   = (const uint16_t*) _top;                                       \
        src2   = (const uint16_t*) _left;                                      \
        src    = (uint16_t*) _src;                                             \
        stride = _stride;                                                      \
        p_src  = src;                                                          \
    } else {                                                                   \
        src1   = (const uint16_t*) _left;                                      \
        src2   = (const uint16_t*) _top;                                       \
        src    = &src_tmp[0];                                                  \
        stride = W;                                                            \
        p_src  = src;                                                          \
    }                                                                          \
    p_out  = (uint16_t*) _src;                                                 \
    ref = (uint16_t*) (src1 - 1)

#define PRED_ANGULAR_WAR()                                                     \
    int y;                                                                     \
    __m128i r0, r1, r3

#define PRED_ANGULAR_WAR4_8()                                                  \
    PRED_ANGULAR_WAR();                                                        \
    __m128i r2
#define PRED_ANGULAR_WAR8_8()                                                  \
    PRED_ANGULAR_WAR4_8();                                                       \
    int x
#define PRED_ANGULAR_WAR16_8()                                                 \
    PRED_ANGULAR_WAR8_8()
#define PRED_ANGULAR_WAR32_8()                                                 \
    PRED_ANGULAR_WAR();                                                        \
    int x

#define PRED_ANGULAR_WAR4_10()    PRED_ANGULAR_WAR8_8()
#define PRED_ANGULAR_WAR8_10()    PRED_ANGULAR_WAR8_8()
#define PRED_ANGULAR_WAR16_10()   PRED_ANGULAR_WAR16_8()
#define PRED_ANGULAR_WAR32_10()   PRED_ANGULAR_WAR32_8()

#define PRED_ANGULAR(W, D)                                                     \
static av_always_inline void pred_angular_ ## W ##_ ## D ## _sse(uint8_t *_src,\
        const uint8_t *_top, const uint8_t *_left, ptrdiff_t _stride, int c_idx, int mode) {\
    const int intra_pred_angle[] = {                                           \
         32, 26, 21, 17, 13,  9,  5,  2,  0, -2, -5, -9,-13,-17,-21,-26,       \
        -32,-26,-21,-17,-13, -9, -5, -2,  0,  2,  5,  9, 13, 17, 21, 26, 32    \
    };                                                                         \
    const int inv_angle[] = {                                                  \
        -4096, -1638, -910, -630, -482, -390, -315, -256, -315, -390, -482,    \
        -630, -910, -1638, -4096                                               \
    };                                                                         \
    PRED_ANGULAR_WAR ## W ## _ ## D();                                         \
    int            angle   = intra_pred_angle[mode-2];                         \
    int            angle_i = angle;                                            \
    int            last    = (W * angle) >> 5;                                 \
    int            stride;                                                     \
    PRED_ANGULAR_INIT_ ## D(W);                                                \
    if (angle < 0 && last < -1) {                                              \
        for (y = last; y <= -1; y++)                                           \
            ref[y] = src2[-1 + ((y * inv_angle[mode-11] + 128) >> 8)];         \
    }                                                                          \
    for (y = 0; y < W; y++) {                                                  \
        int idx  = (angle_i) >> 5;                                             \
        int fact = (angle_i) & 31;                                             \
        if (fact) {                                                            \
            ANGULAR_COMPUTE ## W ## _ ## D();                                  \
        } else {                                                               \
            ANGULAR_COMPUTE_ELSE ## W ## _ ## D();                             \
        }                                                                      \
        angle_i += angle;                                                      \
        p_src   += stride;                                                     \
    }                                                                          \
    if (mode >= 18) {                                                          \
        if (mode == 26 && c_idx == 0) {                                        \
            CLIP_PIXEL1_ ## W ## _ ## D();                                     \
        }                                                                      \
    } else {                                                                   \
        TRANSPOSE ## W ## x ## W ## _ ## D(src_tmp, W, p_out, _stride);        \
        if (mode == 10 && c_idx == 0) {                                        \
            CLIP_PIXEL2_ ## W ## _ ## D();                                     \
        }                                                                      \
    }                                                                          \
}

PRED_ANGULAR( 4, 8)
PRED_ANGULAR( 8, 8)
PRED_ANGULAR(16, 8)
PRED_ANGULAR(32, 8)

PRED_ANGULAR( 4,10)
PRED_ANGULAR( 8,10)
PRED_ANGULAR(16,10)
PRED_ANGULAR(32,10)

void pred_angular_0_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
            ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_4_8_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_1_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
        ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_8_8_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_2_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
        ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_16_8_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_3_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
        ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_32_8_sse(_src, _top, _left, _stride, c_idx, mode);
}

void pred_angular_0_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
            ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_4_10_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_1_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
            ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_8_10_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_2_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
            ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_16_10_sse(_src, _top, _left, _stride, c_idx, mode);
}
void pred_angular_3_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left,
            ptrdiff_t _stride, int c_idx, int mode) {
    pred_angular_32_10_sse(_src, _top, _left, _stride, c_idx, mode);
}
#endif

#ifdef __GNUC__
#pragma GCC pop_options
#endif
