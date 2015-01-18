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

DECLARE_ALIGNED( 16, static const int16_t, transform4x4[4][8] ) = {
    { 64,  64, 64,  64, 64,  64, 64,  64 },
    { 64, -64, 64, -64, 64, -64, 64, -64 },
    { 83,  36, 83,  36, 83,  36, 83,  36 },
    { 36, -83, 36, -83, 36, -83, 36, -83 }
};

DECLARE_ALIGNED(16, static const int16_t, transform8x8[12][1][8] )=
{
    {{  89,  75,  89,  75, 89,  75, 89,  75 }},
    {{  50,  18,  50,  18, 50,  18, 50,  18 }},
    {{  75, -18,  75, -18, 75, -18, 75, -18 }},
    {{ -89, -50, -89, -50,-89, -50,-89, -50 }},
    {{  50, -89,  50, -89, 50, -89, 50, -89 }},
    {{  18,  75,  18,  75, 18,  75, 18,  75 }},
    {{  18, -50,  18, -50, 18, -50, 18, -50 }},
    {{  75, -89,  75, -89, 75, -89, 75, -89 }},
    {{  64,  64,  64,  64, 64,  64, 64,  64 }},
    {{  64, -64,  64, -64, 64, -64, 64, -64 }},
    {{  83,  36,  83,  36, 83,  36, 83,  36 }},
    {{  36, -83,  36, -83, 36, -83, 36, -83 }}
};

DECLARE_ALIGNED(16, static const int16_t, transform16x16_1[4][8][8] )=
{
    {/*1-3*/ /*2-6*/
        { 90,  87,  90,  87,  90,  87,  90,  87 },
        { 87,  57,  87,  57,  87,  57,  87,  57 },
        { 80,   9,  80,   9,  80,   9,  80,   9 },
        { 70, -43,  70, -43,  70, -43,  70, -43 },
        { 57, -80,  57, -80,  57, -80,  57, -80 },
        { 43, -90,  43, -90,  43, -90,  43, -90 },
        { 25, -70,  25, -70,  25, -70,  25, -70 },
        { 9,  -25,   9, -25,   9, -25,   9, -25 },
    },{ /*5-7*/ /*10-14*/
        {  80,  70,  80,  70,  80,  70,  80,  70 },
        {   9, -43,   9, -43,   9, -43,   9, -43 },
        { -70, -87, -70, -87, -70, -87, -70, -87 },
        { -87,   9, -87,   9, -87,   9, -87,   9 },
        { -25,  90, -25,  90, -25,  90, -25,  90 },
        {  57,  25,  57,  25,  57,  25,  57,  25 },
        {  90, -80,  90, -80,  90, -80,  90, -80 },
        {  43, -57,  43, -57,  43, -57,  43, -57 },
    },{ /*9-11*/ /*18-22*/
        {  57,  43,  57,  43,  57,  43,  57,  43 },
        { -80, -90, -80, -90, -80, -90, -80, -90 },
        { -25,  57, -25,  57, -25,  57, -25,  57 },
        {  90,  25,  90,  25,  90,  25,  90,  25 },
        {  -9,  -87, -9,  -87, -9,  -87, -9, -87 },
        { -87,  70, -87,  70, -87,  70, -87,  70 },
        {  43,   9,  43,   9,  43,   9,  43,   9 },
        {  70, -80,  70, -80,  70, -80,  70, -80 },
    },{/*13-15*/ /*  26-30   */
        {  25,   9,  25,   9,  25,   9,  25,   9 },
        { -70, -25, -70, -25, -70, -25, -70, -25 },
        {  90,  43,  90,  43,  90,  43,  90,  43 },
        { -80, -57, -80, -57, -80, -57, -80, -57 },
        {  43,  70,  43,  70,  43,  70,  43,  70 },
        {  9,  -80,   9, -80,   9, -80,   9, -80 },
        { -57,  87, -57,  87, -57,  87, -57,  87 },
        {  87, -90,  87, -90,  87, -90,  87, -90 },
    }
};

DECLARE_ALIGNED(16, static const int16_t, transform32x32[8][16][8] )=
{
    { /*   1-3     */
        { 90,  90, 90,  90, 90,  90, 90,  90 },
        { 90,  82, 90,  82, 90,  82, 90,  82 },
        { 88,  67, 88,  67, 88,  67, 88,  67 },
        { 85,  46, 85,  46, 85,  46, 85,  46 },
        { 82,  22, 82,  22, 82,  22, 82,  22 },
        { 78,  -4, 78,  -4, 78,  -4, 78,  -4 },
        { 73, -31, 73, -31, 73, -31, 73, -31 },
        { 67, -54, 67, -54, 67, -54, 67, -54 },
        { 61, -73, 61, -73, 61, -73, 61, -73 },
        { 54, -85, 54, -85, 54, -85, 54, -85 },
        { 46, -90, 46, -90, 46, -90, 46, -90 },
        { 38, -88, 38, -88, 38, -88, 38, -88 },
        { 31, -78, 31, -78, 31, -78, 31, -78 },
        { 22, -61, 22, -61, 22, -61, 22, -61 },
        { 13, -38, 13, -38, 13, -38, 13, -38 },
        { 4,  -13,  4, -13,  4, -13,  4, -13 },
    },{/*  5-7 */
        {  88,  85,  88,  85,  88,  85,  88,  85 },
        {  67,  46,  67,  46,  67,  46,  67,  46 },
        {  31, -13,  31, -13,  31, -13,  31, -13 },
        { -13, -67, -13, -67, -13, -67, -13, -67 },
        { -54, -90, -54, -90, -54, -90, -54, -90 },
        { -82, -73, -82, -73, -82, -73, -82, -73 },
        { -90, -22, -90, -22, -90, -22, -90, -22 },
        { -78,  38, -78,  38, -78,  38, -78,  38 },
        { -46,  82, -46,  82, -46,  82, -46,  82 },
        {  -4,  88,  -4,  88,  -4,  88,  -4,  88 },
        {  38,  54,  38,  54,  38,  54,  38,  54 },
        {  73,  -4,  73,  -4,  73,  -4,  73,  -4 },
        {  90, -61,  90, -61,  90, -61,  90, -61 },
        {  85, -90,  85, -90,  85, -90,  85, -90 },
        {  61, -78,  61, -78,  61, -78,  61, -78 },
        {  22, -31,  22, -31,  22, -31,  22, -31 },
    },{/*  9-11   */
        {  82,  78,  82,  78,  82,  78,  82,  78 },
        {  22,  -4,  22,  -4,  22,  -4,  22,  -4 },
        { -54, -82, -54, -82, -54, -82, -54, -82 },
        { -90, -73, -90, -73, -90, -73, -90, -73 },
        { -61,  13, -61,  13, -61,  13, -61,  13 },
        {  13,  85,  13,  85,  13,  85,  13,  85 },
        {  78,  67,  78,  67,  78,  67,  78,  67 },
        {  85, -22,  85, -22,  85, -22,  85, -22 },
        {  31, -88,  31, -88,  31, -88,  31, -88 },
        { -46, -61, -46, -61, -46, -61, -46, -61 },
        { -90,  31, -90,  31, -90,  31, -90,  31 },
        { -67,  90, -67,  90, -67,  90, -67,  90 },
        {   4,  54,   4,  54,   4,  54,   4,  54 },
        {  73, -38,  73, -38,  73, -38,  73, -38 },
        {  88, -90,  88, -90,  88, -90,  88, -90 },
        {  38, -46,  38, -46,  38, -46,  38, -46 },
    },{/*  13-15   */
        {  73,  67,  73,  67,  73,  67,  73,  67 },
        { -31, -54, -31, -54, -31, -54, -31, -54 },
        { -90, -78, -90, -78, -90, -78, -90, -78 },
        { -22,  38, -22,  38, -22,  38, -22,  38 },
        {  78,  85,  78,  85,  78,  85,  78,  85 },
        {  67, -22,  67, -22,  67, -22,  67, -22 },
        { -38, -90, -38, -90, -38, -90, -38, -90 },
        { -90,   4, -90,   4, -90,   4, -90,   4 },
        { -13,  90, -13,  90, -13,  90, -13,  90 },
        {  82,  13,  82,  13,  82,  13,  82,  13 },
        {  61, -88,  61, -88,  61, -88,  61, -88 },
        { -46, -31, -46, -31, -46, -31, -46, -31 },
        { -88,  82, -88,  82, -88,  82, -88,  82 },
        { -4,   46, -4,   46, -4,   46, -4,   46 },
        {  85, -73,  85, -73,  85, -73,  85, -73 },
        {  54, -61,  54, -61,  54, -61,  54, -61 },
    },{/*  17-19   */
        {  61,  54,  61,  54,  61,  54,  61,  54 },
        { -73, -85, -73, -85, -73, -85, -73, -85 },
        { -46,  -4, -46,  -4, -46,  -4, -46,  -4 },
        {  82,  88,  82,  88,  82,  88,  82,  88 },
        {  31, -46,  31, -46,  31, -46,  31, -46 },
        { -88, -61, -88, -61, -88, -61, -88, -61 },
        { -13,  82, -13,  82, -13,  82, -13,  82 },
        {  90,  13,  90,  13,  90,  13,  90,  13 },
        { -4, -90,  -4, -90,  -4, -90,  -4, -90 },
        { -90,  38, -90,  38, -90,  38, -90,  38 },
        {  22,  67,  22,  67,  22,  67,  22,  67 },
        {  85, -78,  85, -78,  85, -78,  85, -78 },
        { -38, -22, -38, -22, -38, -22, -38, -22 },
        { -78,  90, -78,  90, -78,  90, -78,  90 },
        {  54, -31,  54, -31,  54, -31,  54, -31 },
        {  67, -73,  67, -73,  67, -73,  67, -73 },
    },{ /*  21-23   */
        {  46,  38,  46,  38,  46,  38,  46,  38 },
        { -90, -88, -90, -88, -90, -88, -90, -88 },
        {  38,  73,  38,  73,  38,  73,  38,  73 },
        {  54,  -4,  54,  -4,  54,  -4,  54,  -4 },
        { -90, -67, -90, -67, -90, -67, -90, -67 },
        {  31,  90,  31,  90,  31,  90,  31,  90 },
        {  61, -46,  61, -46,  61, -46,  61, -46 },
        { -88, -31, -88, -31, -88, -31, -88, -31 },
        {  22,  85,  22,  85,  22,  85,  22,  85 },
        {  67, -78,  67, -78,  67, -78,  67, -78 },
        { -85,  13, -85,  13, -85,  13, -85,  13 },
        {  13,  61,  13,  61,  13,  61,  13,  61 },
        {  73, -90,  73, -90,  73, -90,  73, -90 },
        { -82,  54, -82,  54, -82,  54, -82,  54 },
        {   4,  22,   4,  22,   4,  22,   4,  22 },
        {  78, -82,  78, -82,  78, -82,  78, -82 },
    },{ /*  25-27   */
        {  31,  22,  31,  22,  31,  22,  31,  22 },
        { -78, -61, -78, -61, -78, -61, -78, -61 },
        {  90,  85,  90,  85,  90,  85,  90,  85 },
        { -61, -90, -61, -90, -61, -90, -61, -90 },
        {   4,  73,   4,  73,   4,  73,   4,  73 },
        {  54, -38,  54, -38,  54, -38,  54, -38 },
        { -88,  -4, -88,  -4, -88,  -4, -88,  -4 },
        {  82,  46,  82,  46,  82,  46,  82,  46 },
        { -38, -78, -38, -78, -38, -78, -38, -78 },
        { -22,  90, -22,  90, -22,  90, -22,  90 },
        {  73, -82,  73, -82,  73, -82,  73, -82 },
        { -90,  54, -90,  54, -90,  54, -90,  54 },
        {  67, -13,  67, -13,  67, -13,  67, -13 },
        { -13, -31, -13, -31, -13, -31, -13, -31 },
        { -46,  67, -46,  67, -46,  67, -46,  67 },
        {  85, -88,  85, -88,  85, -88,  85, -88 },
    },{/*  29-31   */
        {  13,   4,  13,   4,  13,   4,  13,   4 },
        { -38, -13, -38, -13, -38, -13, -38, -13 },
        {  61,  22,  61,  22,  61,  22,  61,  22 },
        { -78, -31, -78, -31, -78, -31, -78, -31 },
        {  88,  38,  88,  38,  88,  38,  88,  38 },
        { -90, -46, -90, -46, -90, -46, -90, -46 },
        {  85,  54,  85,  54,  85,  54,  85,  54 },
        { -73, -61, -73, -61, -73, -61, -73, -61 },
        {  54,  67,  54,  67,  54,  67,  54,  67 },
        { -31, -73, -31, -73, -31, -73, -31, -73 },
        {   4,  78,   4,  78,   4,  78,   4,  78 },
        {  22, -82,  22, -82,  22, -82,  22, -82 },
        { -46,  85, -46,  85, -46,  85, -46,  85 },
        {  67, -88,  67, -88,  67, -88,  67, -88 },
        { -82,  90, -82,  90, -82,  90, -82,  90 },
        {  90, -90,  90, -90,  90, -90,  90, -90 },
    }
};

#define shift_1st 7
#define add_1st (1 << (shift_1st - 1))

#define CLIP_PIXEL_MAX_10 0x03FF
#define CLIP_PIXEL_MAX_12 0x0FFF

#if HAVE_SSE2
void ff_hevc_transform_skip_8_sse(uint8_t *_dst, int16_t *coeffs, ptrdiff_t _stride)
{
    uint8_t *dst = (uint8_t*)_dst;
    ptrdiff_t stride = _stride;
    int shift = 5;
    int offset = 16;
    __m128i r0, r1, r2, r3, r4, r5, r6, r9;

    r9 = _mm_setzero_si128();
    r2 = _mm_set1_epi16(offset);

    r0 = _mm_load_si128((__m128i*)(coeffs));
    r1 = _mm_load_si128((__m128i*)(coeffs + 8));


    r0 = _mm_adds_epi16(r0, r2);
    r1 = _mm_adds_epi16(r1, r2);

    r0 = _mm_srai_epi16(r0, shift);
    r1 = _mm_srai_epi16(r1, shift);

    r3 = _mm_loadl_epi64((__m128i*)(dst));
    r4 = _mm_loadl_epi64((__m128i*)(dst + stride));
    r5 = _mm_loadl_epi64((__m128i*)(dst + 2 * stride));
    r6 = _mm_loadl_epi64((__m128i*)(dst + 3 * stride));

    r3 = _mm_unpacklo_epi8(r3, r9);
    r4 = _mm_unpacklo_epi8(r4, r9);
    r5 = _mm_unpacklo_epi8(r5, r9);
    r6 = _mm_unpacklo_epi8(r6, r9);
    r3 = _mm_unpacklo_epi64(r3, r4);
    r4 = _mm_unpacklo_epi64(r5, r6);


    r3 = _mm_adds_epi16(r3, r0);
    r4 = _mm_adds_epi16(r4, r1);

    r3 = _mm_packus_epi16(r3, r4);

    *((uint32_t *)(dst)) = _mm_cvtsi128_si32(r3);
    dst+=stride;
    *((uint32_t *)(dst)) = _mm_cvtsi128_si32(_mm_srli_si128(r3, 4));
    dst+=stride;
    *((uint32_t *)(dst)) = _mm_cvtsi128_si32(_mm_srli_si128(r3, 8));
    dst+=stride;
    *((uint32_t *)(dst)) = _mm_cvtsi128_si32(_mm_srli_si128(r3, 12));
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define INIT_8()                                                               \
    uint8_t *dst = (uint8_t*) _dst;                                            \
    ptrdiff_t stride = _stride
#define INIT_10()                                                              \
    uint16_t *dst = (uint16_t*) _dst;                                          \
    ptrdiff_t stride = _stride>>1

#define INIT_12() INIT_10()
#define INIT8_12() INIT8_10()

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define LOAD_EMPTY(dst, src)
#define LOAD4x4(dst, src)                                                      \
    dst ## 0 = _mm_load_si128((__m128i *) &src[0]);                           \
    dst ## 1 = _mm_load_si128((__m128i *) &src[8])
#define LOAD4x4_STEP(dst, src, sstep)                                          \
    tmp0 = _mm_loadl_epi64((__m128i *) &src[0 * sstep]);                       \
    tmp1 = _mm_loadl_epi64((__m128i *) &src[1 * sstep]);                       \
    tmp2 = _mm_loadl_epi64((__m128i *) &src[2 * sstep]);                       \
    tmp3 = _mm_loadl_epi64((__m128i *) &src[3 * sstep]);                       \
    dst ## 0 = _mm_unpacklo_epi16(tmp0, tmp2);                                 \
    dst ## 1 = _mm_unpacklo_epi16(tmp1, tmp3)
#define LOAD8x8_E(dst, src, sstep)                                             \
    dst ## 0 = _mm_load_si128((__m128i *) &src[0 * sstep]);                   \
    dst ## 1 = _mm_load_si128((__m128i *) &src[1 * sstep]);                   \
    dst ## 2 = _mm_load_si128((__m128i *) &src[2 * sstep]);                   \
    dst ## 3 = _mm_load_si128((__m128i *) &src[3 * sstep])
#define LOAD8x8_O(dst, src, sstep)                                             \
    tmp0 = _mm_load_si128((__m128i *) &src[1 * sstep]);                       \
    tmp1 = _mm_load_si128((__m128i *) &src[3 * sstep]);                       \
    tmp2 = _mm_load_si128((__m128i *) &src[5 * sstep]);                       \
    tmp3 = _mm_load_si128((__m128i *) &src[7 * sstep]);                       \
    dst ## 0 = _mm_unpacklo_epi16(tmp0, tmp1);                                 \
    dst ## 1 = _mm_unpackhi_epi16(tmp0, tmp1);                                 \
    dst ## 2 = _mm_unpacklo_epi16(tmp2, tmp3);                                 \
    dst ## 3 = _mm_unpackhi_epi16(tmp2, tmp3)
#define LOAD16x16_O(dst, src, sstep)                                           \
    LOAD8x8_O(dst, src, sstep);                                                \
    tmp0 = _mm_load_si128((__m128i *) &src[ 9 * sstep]);                      \
    tmp1 = _mm_load_si128((__m128i *) &src[11 * sstep]);                      \
    tmp2 = _mm_load_si128((__m128i *) &src[13 * sstep]);                      \
    tmp3 = _mm_load_si128((__m128i *) &src[15 * sstep]);                      \
    dst ## 4 = _mm_unpacklo_epi16(tmp0, tmp1);                                 \
    dst ## 5 = _mm_unpackhi_epi16(tmp0, tmp1);                                 \
    dst ## 6 = _mm_unpacklo_epi16(tmp2, tmp3);                                 \
    dst ## 7 = _mm_unpackhi_epi16(tmp2, tmp3)

#define LOAD_8x32(dst, dst_stride, src0, src1, idx)                            \
    src0 = _mm_load_si128((__m128i *) &dst[idx*dst_stride]);                   \
    src1 = _mm_load_si128((__m128i *) &dst[idx*dst_stride+4])

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define ASSIGN_EMPTY(dst, dst_stride, src)
#define SAVE_8x16(dst, dst_stride, src)                                        \
    _mm_store_si128((__m128i *) dst, src);                                    \
    dst += dst_stride
#define SAVE_8x32(dst, dst_stride, src0, src1, idx)                            \
    _mm_store_si128((__m128i *) &dst[idx*dst_stride]  , src0);                \
    _mm_store_si128((__m128i *) &dst[idx*dst_stride+4], src1)

#define ASSIGN2(dst, dst_stride, src0, src1, assign)                           \
    assign(dst, dst_stride, src0);                                             \
    assign(dst, dst_stride, _mm_srli_si128(src0, 8));                          \
    assign(dst, dst_stride, src1);                                             \
    assign(dst, dst_stride, _mm_srli_si128(src1, 8))
#define ASSIGN4(dst, dst_stride, src0, src1, src2, src3, assign)               \
    assign(dst, dst_stride, src0);                                             \
    assign(dst, dst_stride, src1);                                             \
    assign(dst, dst_stride, src2);                                             \
    assign(dst, dst_stride, src3)
#define ASSIGN4_LO(dst, dst_stride, src, assign)                               \
    ASSIGN4(dst, dst_stride, src ## 0, src ## 1, src ## 2, src ## 3, assign)
#define ASSIGN4_HI(dst, dst_stride, src, assign)                               \
    ASSIGN4(dst, dst_stride, src ## 4, src ## 5, src ## 6, src ## 7, assign)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define TRANSPOSE4X4_16(dst)                                                   \
    tmp0 = _mm_unpacklo_epi16(dst ## 0, dst ## 1);                             \
    tmp1 = _mm_unpackhi_epi16(dst ## 0, dst ## 1);                             \
    dst ## 0 = _mm_unpacklo_epi16(tmp0, tmp1);                                 \
    dst ## 1 = _mm_unpackhi_epi16(tmp0, tmp1)
#define TRANSPOSE4X4_16_S(dst, dst_stride, src, assign)                        \
    TRANSPOSE4X4_16(src);                                                      \
    ASSIGN2(dst, dst_stride, src ## 0, src ## 1, assign)

#define TRANSPOSE8X8_16(dst)                                                   \
    tmp0 = _mm_unpacklo_epi16(dst ## 0, dst ## 1);                             \
    tmp1 = _mm_unpacklo_epi16(dst ## 2, dst ## 3);                             \
    tmp2 = _mm_unpacklo_epi16(dst ## 4, dst ## 5);                             \
    tmp3 = _mm_unpacklo_epi16(dst ## 6, dst ## 7);                             \
    src0 = _mm_unpacklo_epi32(tmp0, tmp1);                                     \
    src1 = _mm_unpacklo_epi32(tmp2, tmp3);                                     \
    src2 = _mm_unpackhi_epi32(tmp0, tmp1);                                     \
    src3 = _mm_unpackhi_epi32(tmp2, tmp3);                                     \
    tmp0 = _mm_unpackhi_epi16(dst ## 0, dst ## 1);                             \
    tmp1 = _mm_unpackhi_epi16(dst ## 2, dst ## 3);                             \
    tmp2 = _mm_unpackhi_epi16(dst ## 4, dst ## 5);                             \
    tmp3 = _mm_unpackhi_epi16(dst ## 6, dst ## 7);                             \
    dst ## 0 = _mm_unpacklo_epi64(src0 , src1);                                \
    dst ## 1 = _mm_unpackhi_epi64(src0 , src1);                                \
    dst ## 2 = _mm_unpacklo_epi64(src2 , src3);                                \
    dst ## 3 = _mm_unpackhi_epi64(src2 , src3);                                \
    src0 = _mm_unpacklo_epi32(tmp0, tmp1);                                     \
    src1 = _mm_unpacklo_epi32(tmp2, tmp3);                                     \
    src2 = _mm_unpackhi_epi32(tmp0, tmp1);                                     \
    src3 = _mm_unpackhi_epi32(tmp2, tmp3);                                     \
    dst ## 4 = _mm_unpacklo_epi64(src0 , src1);                                \
    dst ## 5 = _mm_unpackhi_epi64(src0 , src1);                                \
    dst ## 6 = _mm_unpacklo_epi64(src2 , src3);                                \
    dst ## 7 = _mm_unpackhi_epi64(src2 , src3)
#define TRANSPOSE8x8_16_S(out, sstep_out, src, assign)                         \
    TRANSPOSE8X8_16(src);                                                      \
    p_dst = out;                                                               \
    ASSIGN4_LO(p_dst, sstep_out, src, assign);                                 \
    ASSIGN4_HI(p_dst, sstep_out, src, assign)
#define TRANSPOSE8x8_16_LS(out, sstep_out, in, sstep_in, assign)               \
    e0  = _mm_load_si128((__m128i *) &in[0*sstep_in]);                         \
    e1  = _mm_load_si128((__m128i *) &in[1*sstep_in]);                         \
    e2  = _mm_load_si128((__m128i *) &in[2*sstep_in]);                         \
    e3  = _mm_load_si128((__m128i *) &in[3*sstep_in]);                         \
    e4  = _mm_load_si128((__m128i *) &in[4*sstep_in]);                         \
    e5  = _mm_load_si128((__m128i *) &in[5*sstep_in]);                         \
    e6  = _mm_load_si128((__m128i *) &in[6*sstep_in]);                         \
    e7  = _mm_load_si128((__m128i *) &in[7*sstep_in]);                         \
    TRANSPOSE8x8_16_S(out, sstep_out, e, assign)

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#define TR_COMPUTE_TRANFORM(dst1, dst2, src0, src1, src2, src3, i, j, transform)\
    tmp1 = _mm_load_si128((__m128i *) transform[i  ][j]);                      \
    tmp3 = _mm_load_si128((__m128i *) transform[i+1][j]);                      \
    tmp0 = _mm_madd_epi16(src0, tmp1);                                         \
    tmp1 = _mm_madd_epi16(src1, tmp1);                                         \
    tmp2 = _mm_madd_epi16(src2, tmp3);                                         \
    tmp3 = _mm_madd_epi16(src3, tmp3);                                         \
    dst1 = _mm_add_epi32(tmp0, tmp2);                                          \
    dst2 = _mm_add_epi32(tmp1, tmp3)

#define SCALE8x8_2x32(dst0, src0, src1)                                        \
    src0 = _mm_srai_epi32(src0, shift);                                        \
    src1 = _mm_srai_epi32(src1, shift);                                        \
    dst0 = _mm_packs_epi32(src0, src1)
#define SCALE_4x32(dst0, dst1, src0, src1, src2, src3)                         \
    SCALE8x8_2x32(dst0, src0, src1);                                           \
    SCALE8x8_2x32(dst1, src2, src3)
#define SCALE16x16_2x32(dst, dst_stride, src0, src1, j)                        \
    e0   = _mm_load_si128((__m128i *) &o16[j*8+0]);                           \
    e7   = _mm_load_si128((__m128i *) &o16[j*8+4]);                           \
    tmp4 = _mm_add_epi32(src0, e0);                                            \
    src0 = _mm_sub_epi32(src0, e0);                                            \
    e0   = _mm_add_epi32(src1, e7);                                            \
    src1 = _mm_sub_epi32(src1, e7);                                            \
    SCALE_4x32(e0, e7, tmp4, e0, src0, src1);                                  \
    _mm_store_si128((__m128i *) &dst[dst_stride*(             j)]  , e0);     \
    _mm_store_si128((__m128i *) &dst[dst_stride*(dst_stride-1-j)]  , e7)

#define SCALE32x32_2x32(dst, dst_stride, j)                                    \
    e0   = _mm_load_si128((__m128i *) &e32[j*16+0]);                          \
    e1   = _mm_load_si128((__m128i *) &e32[j*16+4]);                          \
    e4   = _mm_load_si128((__m128i *) &o32[j*16+0]);                          \
    e5   = _mm_load_si128((__m128i *) &o32[j*16+4]);                          \
    tmp0 = _mm_add_epi32(e0, e4);                                              \
    tmp1 = _mm_add_epi32(e1, e5);                                              \
    tmp2 = _mm_sub_epi32(e1, e5);                                              \
    tmp3 = _mm_sub_epi32(e0, e4);                                              \
    SCALE_4x32(tmp0, tmp1, tmp0, tmp1, tmp3, tmp2);                            \
    _mm_store_si128((__m128i *) &dst[dst_stride*i+0]  , tmp0);                \
    _mm_store_si128((__m128i *) &dst[dst_stride*(dst_stride-1-i)+0]  , tmp1)

#define SAVE16x16_2x32(dst, dst_stride, src0, src1, j)                        \
    e0   = _mm_load_si128((__m128i *) &o16[j*8+0]);                           \
    e7   = _mm_load_si128((__m128i *) &o16[j*8+4]);                           \
    tmp4 = _mm_add_epi32(src0, e0);                                            \
    src0 = _mm_sub_epi32(src0, e0);                                            \
    e0   = _mm_add_epi32(src1, e7);                                            \
    src1 = _mm_sub_epi32(src1, e7);                                            \
    _mm_store_si128((__m128i *) &dst[dst_stride*(             j)]  , tmp4);   \
    _mm_store_si128((__m128i *) &dst[dst_stride*(             j)+4], e0);     \
    _mm_store_si128((__m128i *) &dst[dst_stride*(dst_stride-1-j)]  , src0);   \
    _mm_store_si128((__m128i *) &dst[dst_stride*(dst_stride-1-j)+4], src1)


#define SCALE8x8_2x32_WRAPPER(dst, dst_stride, dst0, src0, src1, idx)          \
    SCALE8x8_2x32(dst0, src0, src1)
#define SCALE16x16_2x32_WRAPPER(dst, dst_stride, dst0, src0, src1, idx)        \
    SCALE16x16_2x32(dst, dst_stride, src0, src1, idx)
#define SAVE16x16_2x32_WRAPPER(dst, dst_stride, dst0, src0, src1, idx)         \
    SAVE16x16_2x32(dst, dst_stride, src0, src1, idx)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_4x4_luma_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define COMPUTE_LUMA(dst , idx)                                                \
    tmp0 = _mm_load_si128((__m128i *) (transform4x4_luma[idx  ]));            \
    tmp1 = _mm_load_si128((__m128i *) (transform4x4_luma[idx+1]));            \
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

#define TRANSFORM_LUMA(D)                                                  \
void ff_hevc_transform_4x4_luma ## _ ## D ## _sse2(int16_t *_coeffs) {          \
    uint8_t  shift = 7;                                                        \
    int16_t *src    = _coeffs;                                                 \
    int16_t *coeffs = _coeffs;                                                 \
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
    _mm_store_si128((__m128i *) coeffs    , res0);                             \
    _mm_store_si128((__m128i *) (coeffs + 8), res1);                           \
}

TRANSFORM_LUMA( 8);
TRANSFORM_LUMA( 10);
TRANSFORM_LUMA( 12);

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_4x4_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define COMPUTE4x4(dst0, dst1, dst2, dst3)                                     \
    tmp0 = _mm_load_si128((__m128i *) transform4x4[0]);                        \
    tmp1 = _mm_load_si128((__m128i *) transform4x4[1]);                        \
    tmp2 = _mm_load_si128((__m128i *) transform4x4[2]);                        \
    tmp3 = _mm_load_si128((__m128i *) transform4x4[3]);                        \
    tmp0 = _mm_madd_epi16(e6, tmp0);                                           \
    tmp1 = _mm_madd_epi16(e6, tmp1);                                           \
    tmp2 = _mm_madd_epi16(e7, tmp2);                                           \
    tmp3 = _mm_madd_epi16(e7, tmp3);                                           \
    e6   = _mm_set1_epi32(add);                                                \
    tmp0 = _mm_add_epi32(tmp0, e6);                                            \
    tmp1 = _mm_add_epi32(tmp1, e6);                                            \
    dst0 = _mm_add_epi32(tmp0, tmp2);                                          \
    dst1 = _mm_add_epi32(tmp1, tmp3);                                          \
    dst2 = _mm_sub_epi32(tmp1, tmp3);                                          \
    dst3 = _mm_sub_epi32(tmp0, tmp2)
#define COMPUTE4x4_LO()                                                        \
    COMPUTE4x4(e0, e1, e2, e3)
#define COMPUTE4x4_HI(dst)                                                     \
    COMPUTE4x4(e7, e6, e5, e4)

#define TR_4(dst, dst_stride, in, sstep, load, assign)                         \
    load(e, in);                                                               \
    e6 = _mm_unpacklo_epi16(e0, e1);                                           \
    e7 = _mm_unpackhi_epi16(e0, e1);                                           \
    COMPUTE4x4_LO();                                                           \
    SCALE_4x32(e0, e1, e0, e1, e2, e3);                                        \
    TRANSPOSE4X4_16_S(dst, dst_stride, e, assign)                              \

#define TR_4_1( dst, dst_stride, src)    TR_4( dst, dst_stride, src,  4, LOAD4x4, ASSIGN_EMPTY)
#define TR_4_2( dst, dst_stride, src, D) TR_4( dst, dst_stride, src,  4, LOAD_EMPTY, ASSIGN_EMPTY)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_8x8_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define TR_4_set8x4(in, sstep)                                                 \
    LOAD8x8_E(src, in, sstep);                                                 \
    e6 = _mm_unpacklo_epi16(src0, src2);                                       \
    e7 = _mm_unpacklo_epi16(src1, src3);                                       \
    COMPUTE4x4_LO();                                                           \
    e6 = _mm_unpackhi_epi16(src0, src2);                                       \
    e7 = _mm_unpackhi_epi16(src1, src3);                                       \
    COMPUTE4x4_HI()

#define TR_COMPUTE8x8(e0, e1, i)                                               \
    TR_COMPUTE_TRANFORM(tmp2, tmp3, src0, src1, src2, src3, i, 0, transform8x8);\
    tmp0 = _mm_add_epi32(e0, tmp2);                                            \
    tmp1 = _mm_add_epi32(e1, tmp3);                                            \
    tmp3 = _mm_sub_epi32(e1, tmp3);                                            \
    tmp2 = _mm_sub_epi32(e0, tmp2)

#define TR_8(dst, dst_stride, in, sstep, assign)                               \
    TR_4_set8x4(in, 2 * sstep);                                                \
    LOAD8x8_O(src, in, sstep);                                                 \
    TR_COMPUTE8x8(e0, e7, 0);                                                  \
    assign(dst, dst_stride, e0, tmp0, tmp1, 0);                                \
    assign(dst, dst_stride, e7, tmp2, tmp3, 7);                                \
    TR_COMPUTE8x8(e1, e6, 2);                                                  \
    assign(dst, dst_stride, e1, tmp0, tmp1, 1);                                \
    assign(dst, dst_stride, e6, tmp2, tmp3, 6);                                \
    TR_COMPUTE8x8(e2, e5, 4);                                                  \
    assign(dst, dst_stride, e2, tmp0, tmp1, 2);                                \
    assign(dst, dst_stride, e5, tmp2, tmp3, 5);                                \
    TR_COMPUTE8x8(e3, e4, 6);                                                  \
    assign(dst, dst_stride, e3, tmp0, tmp1, 3);                                \
    assign(dst, dst_stride, e4, tmp2, tmp3, 4);                                \

#define TR_8_1( dst, dst_stride, src)                                         \
    TR_8( dst, dst_stride, src,  8, SCALE8x8_2x32_WRAPPER);                    \
    TRANSPOSE8x8_16_S(dst, dst_stride, e, SAVE_8x16)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_XxX_X_sse2
////////////////////////////////////////////////////////////////////////////////

#define TRANSFORM_4x4(D)                                                       \
void ff_hevc_transform_4x4_ ## D ## _sse2 (int16_t *_coeffs, int col_limit) {  \
    int16_t *src    = _coeffs;                                                 \
    int16_t *coeffs = _coeffs;                                                 \
    int      shift  = 7;                                                       \
    int      add    = 1 << (shift - 1);                                        \
    __m128i tmp0, tmp1, tmp2, tmp3;                                            \
    __m128i e0, e1, e2, e3, e6, e7;                                            \
    TR_4_1(p_dst1, 4, src);                                                    \
    shift   = 20 - D;                                                          \
    add     = 1 << (shift - 1);                                                \
    TR_4_2(coeffs, 8, tmp, D);                                                 \
    _mm_store_si128((__m128i *) coeffs    , e0);                               \
    _mm_store_si128((__m128i *) (coeffs + 8), e1);                             \
}
#define TRANSFORM_8x8(D)                                                       \
void ff_hevc_transform_8x8_ ## D ## _sse2 (int16_t *coeffs, int col_limit) {    \
    DECLARE_ALIGNED(16, int16_t, tmp[8*8]);                                    \
    int16_t *src    = coeffs;                                                  \
    int16_t *p_dst1 = tmp;                                                     \
    int16_t *p_dst;                                                            \
    int      shift  = 7;                                                       \
    int      add    = 1 << (shift - 1);                                        \
    __m128i src0, src1, src2, src3;                                            \
    __m128i tmp0, tmp1, tmp2, tmp3;                                            \
    __m128i e0, e1, e2, e3, e4, e5, e6, e7;                                    \
    TR_8_1(p_dst1, 8, src);                                                    \
    shift   = 20 - D;                                                          \
    add     = 1 << (shift - 1);                                                \
    TR_8_1(coeffs, 8, tmp);                                                    \
}

TRANSFORM_4x4( 8)
TRANSFORM_4x4(10)
TRANSFORM_4x4(12)
TRANSFORM_8x8( 8)
TRANSFORM_8x8(10)
TRANSFORM_8x8(12)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_16x16_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define TR_COMPUTE16x16(dst1, dst2,src0, src1, src2, src3, i, j)              \
    TR_COMPUTE_TRANFORM(dst1, dst2,src0, src1, src2, src3, i, j, transform16x16_1)
#define TR_COMPUTE16x16_FIRST(j)                                               \
    TR_COMPUTE16x16(src0, src1, e0, e1, e2, e3, 0, j)
#define TR_COMPUTE16x16_NEXT(i, j)                                             \
    TR_COMPUTE16x16(tmp0, tmp1, e4, e5, e6, e7, i, j);                         \
    src0 = _mm_add_epi32(src0, tmp0);                                          \
    src1 = _mm_add_epi32(src1, tmp1)

#define TR_16(dst, dst_stride, in, sstep, assign)                              \
    {                                                                          \
        int i;                                                                 \
        int o16[8*8];                                                          \
        LOAD16x16_O(e, in, sstep);                                             \
        for (i = 0; i < 8; i++) {                                              \
            TR_COMPUTE16x16_FIRST(i);                                          \
            TR_COMPUTE16x16_NEXT(2, i);                                        \
            SAVE_8x32(o16, 8, src0, src1, i);                                  \
        }                                                                      \
        TR_8(dst, dst_stride, in, 2 * sstep, assign);                          \
    }

#define TR_16_1( dst, dst_stride, src)        TR_16( dst, dst_stride, src,     16, SCALE16x16_2x32_WRAPPER)
#define TR_16_2( dst, dst_stride, src, sstep) TR_16( dst, dst_stride, src,  sstep, SAVE16x16_2x32_WRAPPER )

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_32x32_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define TR_COMPUTE32x32(dst1, dst2,src0, src1, src2, src3, i, j)              \
    TR_COMPUTE_TRANFORM(dst1, dst2, src0, src1, src2, src3, i, j, transform32x32)
#define TR_COMPUTE32x32_FIRST(i, j)                                            \
    TR_COMPUTE32x32(tmp0, tmp1, e0, e1, e2, e3, i, j);                         \
    src0 = _mm_add_epi32(src0, tmp0);                                          \
    src1 = _mm_add_epi32(src1, tmp1)
#define TR_COMPUTE32x32_NEXT(i, j)                                             \
    TR_COMPUTE32x32(tmp0, tmp1, e4, e5, e6, e7, i, j);                         \
    src0 = _mm_add_epi32(src0, tmp0);                                          \
    src1 = _mm_add_epi32(src1, tmp1)

#define TR_32(dst, dst_stride, in, sstep)                                      \
    {                                                                          \
        int i;                                                                 \
        DECLARE_ALIGNED(16, int, e32[16*16]);                                  \
        DECLARE_ALIGNED(16, int, o32[16*16]);                                  \
        LOAD16x16_O(e, in, sstep);                                             \
        for (i = 0; i < 16; i++) {                                             \
            src0 = _mm_setzero_si128();                                        \
            src1 = _mm_setzero_si128();                                        \
            TR_COMPUTE32x32_FIRST(0, i);                                       \
            TR_COMPUTE32x32_NEXT(2, i);                                        \
            SAVE_8x32(o32, 16, src0, src1, i);                                 \
        }                                                                      \
        LOAD16x16_O(e, (&in[16*sstep]), sstep);                                \
        for (i = 0; i < 16; i++) {                                             \
            LOAD_8x32(o32, 16, src0, src1, i);                                 \
            TR_COMPUTE32x32_FIRST(4, i);                                       \
            TR_COMPUTE32x32_NEXT(6, i);                                        \
            SAVE_8x32(o32, 16, src0, src1, i);                                 \
        }                                                                      \
        TR_16_2(e32, 16, in, 2 * sstep);                                       \
        for (i = 0; i < 16; i++) {                                             \
            SCALE32x32_2x32(dst, dst_stride, i);                               \
        }                                                                      \
    }

#define TR_32_1( dst, dst_stride, src)        TR_32( dst, dst_stride, src, 32)

////////////////////////////////////////////////////////////////////////////////
// ff_hevc_transform_XxX_X_sse2
////////////////////////////////////////////////////////////////////////////////
#define TRANSFORM2(H, D)                                                   \
void ff_hevc_transform_ ## H ## x ## H ## _ ## D ## _sse2 (                \
    int16_t *coeffs, int col_limit) {                                          \
    int i, j, k, add;                                                          \
    int      shift = 7;                                                        \
    int16_t *src   = coeffs;                                                   \
    DECLARE_ALIGNED(16, int16_t, tmp[H*H]);                                    \
    DECLARE_ALIGNED(16, int16_t, tmp_2[H*H]);                                  \
    int16_t *p_dst, *p_tra = tmp_2;                                            \
    __m128i src0, src1, src2, src3;                                            \
    __m128i tmp0, tmp1, tmp2, tmp3, tmp4;                                      \
    __m128i e0, e1, e2, e3, e4, e5, e6, e7;                                    \
    for (k = 0; k < 2; k++) {                                                  \
        add   = 1 << (shift - 1);                                              \
        for (i = 0; i < H; i+=8) {                                             \
            p_dst = tmp + i;                                                   \
            TR_ ## H ## _1(p_dst, H, src);                                     \
            src   += 8;                                                        \
            for (j = 0; j < H; j+=8) {                                         \
               TRANSPOSE8x8_16_LS((&p_tra[i*H+j]), H, (&tmp[j*H+i]), H, SAVE_8x16);\
            }                                                                  \
        }                                                                      \
        src   = tmp_2;                                                         \
        p_tra = coeffs;                                                         \
        shift = 20 - D;                                                        \
    }                                                                          \
}

TRANSFORM2(16,  8);
TRANSFORM2(16, 10);
TRANSFORM2(16, 12);

TRANSFORM2(32,  8);
TRANSFORM2(32, 10);
TRANSFORM2(32, 12);

#endif

#ifdef __GNUC__
#pragma GCC pop_options
#endif
