/**************************************************************************************
 * Copyright (c) 2018-2022 ["Peking University Shenzhen Graduate School",
 *   "Peng Cheng Laboratory", and "Guangdong Bohua UHD Innovation Corporation"]
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organizations (Peking University Shenzhen Graduate School,
 *    Peng Cheng Laboratory and Guangdong Bohua UHD Innovation Corporation) nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * For more information, contact us at rgwang@pkusz.edu.cn.
 **************************************************************************************/

#include "com_type.h"
#include "com_util.h"

uavs3d_funs_handle_t uavs3d_funs_handle;

void *align_malloc(int i_size)
{
    int mask = ALIGN_BASIC - 1;
    u8 *align_buf = NULL;
    u8 *buf = (u8 *)malloc(i_size + mask + sizeof(void **));
    
    if (buf) {
        align_buf = buf + mask + sizeof(void **);
        align_buf -= (intptr_t)align_buf & mask;
        *(((void **)align_buf) - 1) = buf;
        memset(align_buf, 0, i_size);
    } 
    return align_buf;
}

void align_free(void *p)
{
    free(*(((void **)p) - 1));
}

com_pic_t * com_picbuf_alloc(int width, int height, int pad_l, int pad_c, int i_scu, int f_scu, int bit_depth, int parallel, int *err)
{
    int total_mem_size = 0;
    u8 *buf;
    com_pic_t *pic = com_malloc(sizeof(com_pic_t));

    uavs3d_assert_goto(pic != NULL, ERR);

    pic->bit_depth      = bit_depth;
    pic->width_luma     = width;
    pic->height_luma    = height;
    pic->width_chroma   = width  / 2;
    pic->height_chroma  = height / 2;
    pic->padsize_luma   = pad_l;
    pic->padsize_chroma = pad_c;
    pic->stride_luma    = width + pad_l * 2;
    pic->stride_chroma  = (width / 2 + pad_c * 2) * 2;

    /* allocate maps */
    total_mem_size = sizeof(pel) * (pic->stride_luma * (height + 2 * pad_l) + pic->stride_chroma * (height / 2 + pad_c * 2)) + // yuv buffer
                     sizeof( s8) * f_scu * REFP_NUM        + ALIGN_MASK + // map_refi
                     sizeof(s16) * f_scu * REFP_NUM * MV_D + ALIGN_MASK + // map_mv
                     ALIGN_MASK * 4;

    pic->mem_base = buf = com_malloc(total_mem_size);
    uavs3d_assert_goto(pic->mem_base, ERR);

    buf = ALIGN_POINTER(buf);

    GIVE_BUFFER (pic->y,        buf, sizeof(pel) * pic->stride_luma   * (height     + pad_l * 2));  pic->y        += pad_l * pic->stride_luma   + pad_l;
    GIVE_BUFFER (pic->uv,       buf, sizeof(pel) * pic->stride_chroma * (height / 2 + pad_c * 2));  pic->uv       += pad_c * pic->stride_chroma + pad_c * 2;
    GIVE_BUFFERV(pic->map_refi, buf, sizeof( s8) * f_scu * REFP_NUM ,   -1);                        pic->map_refi += i_scu + 1;
    GIVE_BUFFER (pic->map_mv,   buf, sizeof(s16) * f_scu * REFP_NUM * MV_D);                        pic->map_mv   += i_scu + 1;

    if (parallel) {
        pic->parallel_enable = 1;
        if (!uavs3d_pthread_mutex_init(&pic->mutex, NULL)) {
            if (uavs3d_pthread_cond_init(&pic->cond, NULL)) {
                uavs3d_pthread_mutex_destroy(&pic->mutex);
                uavs3d_assert_goto(0, ERR);
            }
        } else {
            uavs3d_assert_goto(0, ERR);
        }
    } else {
        pic->parallel_enable = 0;
    }
    if (err) {
        *err = RET_OK;
    }

    return pic;
ERR:
    if (pic) {
        com_free(pic->mem_base);   
        com_free(pic);
    }
    if (err) {
        *err = ERR_OUT_OF_MEMORY;
    }
    return NULL;
}


com_pic_t * com_pic_alloc(com_pic_param_t * pic_param, int * ret)
{
    return com_picbuf_alloc(pic_param->width, pic_param->height, pic_param->pad_l, pic_param->pad_c, pic_param->i_scu, pic_param->f_scu, pic_param->bit_depth, pic_param->parallel, ret);
}

void com_pic_free(com_pic_param_t *pic_param, com_pic_t *pic)
{
    if (pic) {
        if (pic->parallel_enable) {
            uavs3d_pthread_mutex_destroy(&pic->mutex);
            uavs3d_pthread_cond_destroy(&pic->cond);
        }
        com_free(pic->mem_base);
        com_free(pic);
    }
}

void com_core_free(com_core_t *core)
{
    if (core) {
        com_free(core->map.map_base);
        com_free(core);
    }
}

com_core_t* com_core_init(com_seqh_t *seqhdr)
{
    int total_mem_size = 0;
    u8 *buf;
    com_map_t *map;
    com_core_t *core = com_malloc(sizeof(com_core_t));
    int sao_l_size = 0, sao_c_size = 0, sao_map_size = 0;
    int alf_l_size = 0, alf_c_size = 0, alf_map_size = 0;

    if (core == NULL) {
        return NULL;
    }

    map = &core->map;

    total_mem_size = ALIGN_MASK     + sizeof(com_scu_t) * seqhdr->f_scu +             // map_scu
                     ALIGN_MASK     + sizeof(       s8) * seqhdr->f_scu +             // map_ipm
                     ALIGN_MASK     + sizeof(      u32) * seqhdr->f_scu +             // map_pos
                     ALIGN_MASK     + sizeof(       u8) * seqhdr->f_scu +             // map_edge
                     ALIGN_MASK     + sizeof(       s8) * seqhdr->f_lcu +             // map_patch
                     ALIGN_MASK     + sizeof(       s8) * seqhdr->f_lcu +             // map_qp
                     ALIGN_MASK * 2 + sizeof(pel) * seqhdr->pic_width * 2 +           // linebuf_intra
                     ALIGN_MASK * 2 + sizeof(pel) * (seqhdr->pic_width * 2 + 3) +     // linebuf_sao
                     ALIGN_MASK;

    if (seqhdr->sample_adaptive_offset_enable_flag) {
        sao_l_size   = sizeof(pel) * ((MAX_CU_SIZE + SAO_SHIFT_PIX_NUM + 2) * (MAX_CU_SIZE + SAO_SHIFT_PIX_NUM + 2) + seqhdr->pic_width + ALIGN_MASK);
        sao_c_size   = sizeof(pel) * ((MAX_CU_SIZE / 2 + SAO_SHIFT_PIX_NUM + 2) * (MAX_CU_SIZE / 2 + SAO_SHIFT_PIX_NUM + 2) + seqhdr->pic_width / 2 + ALIGN_MASK) * 2;
        sao_map_size = sizeof(com_sao_param_t) * N_C * seqhdr->f_lcu;
        total_mem_size += sao_l_size + sao_c_size + sao_map_size + ALIGN_MASK * 3;
    }
    if (seqhdr->adaptive_leveling_filter_enable_flag) {
        alf_l_size = sizeof(pel) * ((MAX_CU_SIZE + ALIGN_BASIC) * (MAX_CU_SIZE + 4) + seqhdr->pic_width + ALIGN_BASIC * 2);
        alf_c_size = sizeof(pel) * ((MAX_CU_SIZE / 2 + ALIGN_BASIC) * (MAX_CU_SIZE / 2 + 4) + seqhdr->pic_width / 2 + ALIGN_BASIC * 2) * 2;
        alf_map_size = sizeof(u8) * N_C * seqhdr->f_lcu;
        total_mem_size += alf_l_size + alf_c_size + alf_map_size + ALIGN_MASK * 3;
    }

    map->map_base = buf = com_malloc(total_mem_size);

    if (buf == NULL) {
        com_free(core);
        return NULL;
    }

    buf = ALIGN_POINTER(buf);
    GIVE_BUFFERV(map->map_scu,    buf, sizeof(com_scu_t) * seqhdr->f_scu, 0);   map->map_scu    += seqhdr->i_scu + 1;
    GIVE_BUFFER (map->map_ipm,    buf, sizeof(       s8) * seqhdr->f_scu   );   map->map_ipm    += seqhdr->i_scu + 1;
    GIVE_BUFFER (map->map_pos,    buf, sizeof(      u32) * seqhdr->f_scu   );   map->map_pos    += seqhdr->i_scu + 1;
    GIVE_BUFFERV(map->map_edge,   buf, sizeof(       u8) * seqhdr->f_scu, 0);   map->map_edge   += seqhdr->i_scu + 1;

    GIVE_BUFFER(map->map_patch,  buf, sizeof( s8) * seqhdr->f_lcu);
    GIVE_BUFFER(map->map_qp,     buf, sizeof( s8) * seqhdr->f_lcu);

    GIVE_BUFFER(core->linebuf_intra[0], buf, sizeof(pel) *  seqhdr->pic_width);
    GIVE_BUFFER(core->linebuf_intra[1], buf, sizeof(pel) *  seqhdr->pic_width);
    GIVE_BUFFER(core->linebuf_sao  [0], buf, sizeof(pel) * (seqhdr->pic_width + 1)); core->linebuf_sao[0] += 1;
    GIVE_BUFFER(core->linebuf_sao  [1], buf, sizeof(pel) * (seqhdr->pic_width + 2)); core->linebuf_sao[1] += 2;

    if (seqhdr->sample_adaptive_offset_enable_flag) {
        GIVE_BUFFER(core->sao_src_buf[0], buf, sao_l_size);
        GIVE_BUFFER(core->sao_src_buf[1], buf, sao_c_size);
        GIVE_BUFFER(core->sao_param_map,  buf, sao_map_size);
    }
    if (seqhdr->adaptive_leveling_filter_enable_flag) {
        GIVE_BUFFER(core->alf_src_buf[0], buf, alf_l_size);
        GIVE_BUFFER(core->alf_src_buf[1], buf, alf_c_size);
        GIVE_BUFFER(core->alf_enable_map, buf, alf_map_size);
    }
    return core;
}

static void padding_rows_luma(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i;
    pel *p;

    start = max(start, 0);
    rows  = min(rows, height - start);

    if (start + rows == height) {
        rows += padv;
        p = src + i_src * (height - 1);
        for (i = 1; i <= padv; i++) {
            memcpy(p + i_src * i, p, width * sizeof(pel));
        }
    }

    if (start == 0) {
        start = -padv;
        rows += padv;
        p = src;
        for (i = 1; i <= padv; i++) {
            memcpy(p - i_src * i, p, width * sizeof(pel));
        }
    }

    p = src + start * i_src;

    // left & right
    for (i = 0; i < rows; i++) {
        com_mset_pel(p - padh, p[0], padh);
        com_mset_pel(p + width, p[width - 1], padh);
        p += i_src;
    }
}

static void padding_rows_chroma(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i;
    pel *p;

    start = max(start, 0);
    rows = min(rows, height - start);

    if (start + rows == height) {
        rows += padv;
        p = src + i_src * (height - 1);
        for (i = 1; i <= padv; i++) {
            memcpy(p + i_src * i, p, width * sizeof(pel));
        }
    }

    if (start == 0) {
        start = -padv;
        rows += padv;
        p = src;
        for (i = 1; i <= padv; i++) {
            memcpy(p - i_src * i, p, width * sizeof(pel));
        }
    }

    p = src + start * i_src;

    // left & right
    int shift = sizeof(pel) * 8;
   
    for (i = 0; i < rows; i++) {
        int uv_pixel = (p[1] << shift) + p[0];
        com_mset_2pel(p - padh, uv_pixel, padh >> 1);

        uv_pixel = (p[width - 1] << shift) + p[width - 2];
        com_mset_2pel(p + width, uv_pixel, padh >> 1);

        p += i_src;
    }
}

/* MD5 functions */
#define MD5FUNC(f, w, x, y, z, msg1, s,msg2 ) \
     ( w += f(x, y, z) + msg1 + msg2,  w = w<<s | w>>(32-s),  w += x )
#define FF(x, y, z) (z ^ (x & (y ^ z)))
#define GG(x, y, z) (y ^ (z & (x ^ y)))
#define HH(x, y, z) (x ^ y ^ z)
#define II(x, y, z) (y ^ (x | ~z))

static void com_md5_trans(u32 *buf, u32 *msg)
{
    register u32 a, b, c, d;
    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];
    MD5FUNC(FF, a, b, c, d, msg[ 0],  7, 0xd76aa478); /* 1 */
    MD5FUNC(FF, d, a, b, c, msg[ 1], 12, 0xe8c7b756); /* 2 */
    MD5FUNC(FF, c, d, a, b, msg[ 2], 17, 0x242070db); /* 3 */
    MD5FUNC(FF, b, c, d, a, msg[ 3], 22, 0xc1bdceee); /* 4 */
    MD5FUNC(FF, a, b, c, d, msg[ 4],  7, 0xf57c0faf); /* 5 */
    MD5FUNC(FF, d, a, b, c, msg[ 5], 12, 0x4787c62a); /* 6 */
    MD5FUNC(FF, c, d, a, b, msg[ 6], 17, 0xa8304613); /* 7 */
    MD5FUNC(FF, b, c, d, a, msg[ 7], 22, 0xfd469501); /* 8 */
    MD5FUNC(FF, a, b, c, d, msg[ 8],  7, 0x698098d8); /* 9 */
    MD5FUNC(FF, d, a, b, c, msg[ 9], 12, 0x8b44f7af); /* 10 */
    MD5FUNC(FF, c, d, a, b, msg[10], 17, 0xffff5bb1); /* 11 */
    MD5FUNC(FF, b, c, d, a, msg[11], 22, 0x895cd7be); /* 12 */
    MD5FUNC(FF, a, b, c, d, msg[12],  7, 0x6b901122); /* 13 */
    MD5FUNC(FF, d, a, b, c, msg[13], 12, 0xfd987193); /* 14 */
    MD5FUNC(FF, c, d, a, b, msg[14], 17, 0xa679438e); /* 15 */
    MD5FUNC(FF, b, c, d, a, msg[15], 22, 0x49b40821); /* 16 */
    /* Round 2 */
    MD5FUNC(GG, a, b, c, d, msg[ 1],  5, 0xf61e2562); /* 17 */
    MD5FUNC(GG, d, a, b, c, msg[ 6],  9, 0xc040b340); /* 18 */
    MD5FUNC(GG, c, d, a, b, msg[11], 14, 0x265e5a51); /* 19 */
    MD5FUNC(GG, b, c, d, a, msg[ 0], 20, 0xe9b6c7aa); /* 20 */
    MD5FUNC(GG, a, b, c, d, msg[ 5],  5, 0xd62f105d); /* 21 */
    MD5FUNC(GG, d, a, b, c, msg[10],  9,  0x2441453); /* 22 */
    MD5FUNC(GG, c, d, a, b, msg[15], 14, 0xd8a1e681); /* 23 */
    MD5FUNC(GG, b, c, d, a, msg[ 4], 20, 0xe7d3fbc8); /* 24 */
    MD5FUNC(GG, a, b, c, d, msg[ 9],  5, 0x21e1cde6); /* 25 */
    MD5FUNC(GG, d, a, b, c, msg[14],  9, 0xc33707d6); /* 26 */
    MD5FUNC(GG, c, d, a, b, msg[ 3], 14, 0xf4d50d87); /* 27 */
    MD5FUNC(GG, b, c, d, a, msg[ 8], 20, 0x455a14ed); /* 28 */
    MD5FUNC(GG, a, b, c, d, msg[13],  5, 0xa9e3e905); /* 29 */
    MD5FUNC(GG, d, a, b, c, msg[ 2],  9, 0xfcefa3f8); /* 30 */
    MD5FUNC(GG, c, d, a, b, msg[ 7], 14, 0x676f02d9); /* 31 */
    MD5FUNC(GG, b, c, d, a, msg[12], 20, 0x8d2a4c8a); /* 32 */
    /* Round 3 */
    MD5FUNC(HH, a, b, c, d, msg[ 5],  4, 0xfffa3942); /* 33 */
    MD5FUNC(HH, d, a, b, c, msg[ 8], 11, 0x8771f681); /* 34 */
    MD5FUNC(HH, c, d, a, b, msg[11], 16, 0x6d9d6122); /* 35 */
    MD5FUNC(HH, b, c, d, a, msg[14], 23, 0xfde5380c); /* 36 */
    MD5FUNC(HH, a, b, c, d, msg[ 1],  4, 0xa4beea44); /* 37 */
    MD5FUNC(HH, d, a, b, c, msg[ 4], 11, 0x4bdecfa9); /* 38 */
    MD5FUNC(HH, c, d, a, b, msg[ 7], 16, 0xf6bb4b60); /* 39 */
    MD5FUNC(HH, b, c, d, a, msg[10], 23, 0xbebfbc70); /* 40 */
    MD5FUNC(HH, a, b, c, d, msg[13],  4, 0x289b7ec6); /* 41 */
    MD5FUNC(HH, d, a, b, c, msg[ 0], 11, 0xeaa127fa); /* 42 */
    MD5FUNC(HH, c, d, a, b, msg[ 3], 16, 0xd4ef3085); /* 43 */
    MD5FUNC(HH, b, c, d, a, msg[ 6], 23,  0x4881d05); /* 44 */
    MD5FUNC(HH, a, b, c, d, msg[ 9],  4, 0xd9d4d039); /* 45 */
    MD5FUNC(HH, d, a, b, c, msg[12], 11, 0xe6db99e5); /* 46 */
    MD5FUNC(HH, c, d, a, b, msg[15], 16, 0x1fa27cf8); /* 47 */
    MD5FUNC(HH, b, c, d, a, msg[ 2], 23, 0xc4ac5665); /* 48 */
    /* Round 4 */
    MD5FUNC(II, a, b, c, d, msg[ 0],  6, 0xf4292244); /* 49 */
    MD5FUNC(II, d, a, b, c, msg[ 7], 10, 0x432aff97); /* 50 */
    MD5FUNC(II, c, d, a, b, msg[14], 15, 0xab9423a7); /* 51 */
    MD5FUNC(II, b, c, d, a, msg[ 5], 21, 0xfc93a039); /* 52 */
    MD5FUNC(II, a, b, c, d, msg[12],  6, 0x655b59c3); /* 53 */
    MD5FUNC(II, d, a, b, c, msg[ 3], 10, 0x8f0ccc92); /* 54 */
    MD5FUNC(II, c, d, a, b, msg[10], 15, 0xffeff47d); /* 55 */
    MD5FUNC(II, b, c, d, a, msg[ 1], 21, 0x85845dd1); /* 56 */
    MD5FUNC(II, a, b, c, d, msg[ 8],  6, 0x6fa87e4f); /* 57 */
    MD5FUNC(II, d, a, b, c, msg[15], 10, 0xfe2ce6e0); /* 58 */
    MD5FUNC(II, c, d, a, b, msg[ 6], 15, 0xa3014314); /* 59 */
    MD5FUNC(II, b, c, d, a, msg[13], 21, 0x4e0811a1); /* 60 */
    MD5FUNC(II, a, b, c, d, msg[ 4],  6, 0xf7537e82); /* 61 */
    MD5FUNC(II, d, a, b, c, msg[11], 10, 0xbd3af235); /* 62 */
    MD5FUNC(II, c, d, a, b, msg[ 2], 15, 0x2ad7d2bb); /* 63 */
    MD5FUNC(II, b, c, d, a, msg[ 9], 21, 0xeb86d391); /* 64 */
    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

typedef struct uavs3d_com_md5_t
{
    u32     h[4];    
    u8      msg[64]; 
    u32     bits[2]; 
} com_md5_t;

static void md5_init(com_md5_t *md5)
{
    md5->h[0] = 0x67452301;
    md5->h[1] = 0xefcdab89;
    md5->h[2] = 0x98badcfe;
    md5->h[3] = 0x10325476;
    md5->bits[0] = 0;
    md5->bits[1] = 0;
}

static void md5_update(com_md5_t *md5, void *buf_t, u32 len)
{
    u8 *buf;
    u32 i, idx, part_len;
    buf = (u8*)buf_t;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);
    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3)) {
        (md5->bits[1])++;
    }
    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;
    if(len >= part_len) {
        memcpy(md5->msg + idx, buf, part_len);
        com_md5_trans(md5->h, (u32 *)md5->msg);
        for(i = part_len; i + 63 < len; i += 64) {
            com_md5_trans(md5->h, (u32 *)(buf + i));
        }
        idx = 0;
    } else {
        i = 0;
    }
    if(len - i > 0) {
        memcpy(md5->msg + idx, buf + i, len - i);
    }
}

static void md5_update_uv(com_md5_t *md5, void *buf_t, u32 len, int offset)
{
    u8 *buf;
    u32 i, idx, part_len, j;
    u8 t[8196 * 10];
    buf = (u8 *)buf_t + offset;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);
 
    for (j = 0; j < len; j += 2) {
        t[j] = buf[0];
        t[j + 1] = buf[1];
        buf += 4;
    }
    md5->bits[0] += (len << 3);
    if (md5->bits[0] < (len << 3)) {
        (md5->bits[1])++;
    }
    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if (len >= part_len) {
        memcpy(md5->msg + idx, t, part_len);
        com_md5_trans(md5->h, (u32 *)md5->msg);
        for (i = part_len; i + 63 < len; i += 64) {
            com_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    } else {
        i = 0;
    }
    if (len - i > 0) {
        memcpy(md5->msg + idx, t + i, len - i);
    }
}

static void md5_update_s16(com_md5_t *md5, void *buf_t, u32 len)
{
    u8 *buf;
    u32 i, idx, part_len, j;
    u8 t[8196 * 10];
    buf = (u8 *)buf_t;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);
    len = len * 2;
    for(j = 0; j < len; j += 2) {
        t[j] = (u8)(*(buf));
        t[j + 1] = 0;
        buf++;
    }
    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3)) {
        (md5->bits[1])++;
    }
    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;
    if(len >= part_len) {
        memcpy(md5->msg + idx, t, part_len);
        com_md5_trans(md5->h, (u32 *)md5->msg);
        for(i = part_len; i + 63 < len; i += 64) {
            com_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    } else {
        i = 0;
    }
    if(len - i > 0) {
        memcpy(md5->msg + idx, t + i, len - i);
    }
}

static void md5_update_uv_s16(com_md5_t *md5, void *buf_t, u32 len, int offset)
{
    u8 *buf;
    u32 i, idx, part_len, j;
    u8 t[8196 * 10];
    buf = (u8 *)buf_t + offset;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);
    len = len * 2;
    for (j = 0; j < len; j += 2) {
        t[j] = buf[j];
        t[j + 1] = 0;
    }
    md5->bits[0] += (len << 3);
    if (md5->bits[0] < (len << 3)) {
        (md5->bits[1])++;
    }
    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;
    if (len >= part_len) {
        memcpy(md5->msg + idx, t, part_len);
        com_md5_trans(md5->h, (u32 *)md5->msg);
        for (i = part_len; i + 63 < len; i += 64) {
            com_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    } else {
        i = 0;
    }
    if (len - i > 0) {
        memcpy(md5->msg + idx, t + i, len - i);
    }
}

static void md5_finish(com_md5_t *md5, u8 digest[16])
{
    u8 *pos;
    int cnt;
    cnt = (md5->bits[0] >> 3) & 0x3F;
    pos = md5->msg + cnt;
    *pos++ = 0x80;
    cnt = 64 - 1 - cnt;

    if(cnt < 8) {
        memset(pos, 0, cnt);
        com_md5_trans(md5->h, (u32 *)md5->msg);
        memset(md5->msg, 0, 56);
    } else {
        memset(pos, 0, cnt - 8);
    }
    memcpy((md5->msg + 14 * sizeof(u32)), &md5->bits[0], sizeof(u32));
    memcpy((md5->msg + 15 * sizeof(u32)), &md5->bits[1], sizeof(u32));
    com_md5_trans(md5->h, (u32 *)md5->msg);
    memcpy(digest, md5->h, 16);
    memset(md5, 0, sizeof(com_md5_t));
}

int  com_md5_image(com_pic_t *pic, u8 digest[16])
{
    com_md5_t md5;
    int j;
    md5_init(&md5);

    for (j = 0; j < pic->height_luma; j++) {
#if (BIT_DEPTH == 8)
        md5_update_s16(&md5, pic->y + j * pic->stride_luma, pic->width_luma * sizeof(pel));
#else
        md5_update(&md5, pic->y + j * pic->stride_luma, pic->width_luma * sizeof(pel));
#endif
    }

    for (j = 0; j < pic->height_chroma; j++) {
#if (BIT_DEPTH == 8)
        md5_update_uv_s16(&md5, pic->uv + j * pic->stride_chroma, pic->width_chroma * sizeof(pel), 0);
#else
        md5_update_uv(&md5, pic->uv + j * pic->stride_chroma, pic->width_chroma * sizeof(pel), 0);
#endif
    }

    for (j = 0; j < pic->height_chroma; j++) {
#if (BIT_DEPTH == 8)
        md5_update_uv_s16(&md5, pic->uv + j * pic->stride_chroma, pic->width_chroma * sizeof(pel), 1);
#else
        md5_update_uv(&md5, pic->uv + j * pic->stride_chroma, pic->width_chroma * sizeof(pel), 2);
#endif
    }
    md5_finish(&md5, digest);
    return RET_OK;
}


void com_lbac_ctx_init(com_lbac_all_ctx_t *sbac_ctx)
{
    int i, num;
    lbac_ctx_model_t *p;
    memset(sbac_ctx, 0x00, sizeof(*sbac_ctx));

    /* Initialization of the context models */
    num = sizeof(com_lbac_all_ctx_t) / sizeof(lbac_ctx_model_t);
    p = (lbac_ctx_model_t*)sbac_ctx;

    for (i = 0; i < num; i++) {
        p[i] = ((1 << PROB_LPS_BITS_STORE) - 1) << 1;
    }
}

static void conv_fmt_8bit_c(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift) {
    int i, j;
    unsigned char* psrc = src_y;
    unsigned char* pdst = dst[0];
    for (i = 0; i < height; i++) {
        memcpy(pdst, psrc, width);
        psrc += src_stride;
        pdst += dst_stride[0];
    }

    width >>= uv_shift;
    height >>= uv_shift;

    psrc = src_uv;
    pdst = dst[1];
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = psrc[j << 1];
        }
        psrc += src_stridec;
        pdst += dst_stride[1];
    }

    psrc = src_uv + 1;
    pdst = dst[2];
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = psrc[j << 1];
        }
        psrc += src_stridec;
        pdst += dst_stride[2];
    }
}

static void conv_fmt_16bit_c(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift) {
    int i, j;
    short* psrc = (short*)src_y;
    short* pdst = (short*)dst[0];
    int width2 = width << 1;
    int i_src = src_stride >> 1;
    int i_dst = dst_stride[0] >> 1;

    for (i = 0; i < height; i++) {
        memcpy(pdst, psrc, width2);
        psrc += i_src;
        pdst += i_dst;
    }

    width >>= uv_shift;
    height >>= uv_shift;

    psrc = (short*)src_uv;
    pdst = (short*)dst[1];
    i_src = src_stridec >> 1;
    i_dst = dst_stride[1] >> 1;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = psrc[j << 1];
        }
        psrc += i_src;
        pdst += i_dst;
    }

    psrc = ((short*)src_uv) + 1;
    pdst = (short*)dst[2];
    i_dst = dst_stride[2] >> 1;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = psrc[j << 1];
        }
        psrc += i_src;
        pdst += i_dst;
    }
}

static void conv_fmt_16to8bit_c(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift) {
    int i, j;
    short* psrc = (short*)src_y;
    unsigned char* pdst = dst[0];
    int i_dst, i_src;

    i_src = src_stride >> 1;
    i_dst = dst_stride[0];
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = (unsigned char)psrc[j];
        }
        psrc += i_src;
        pdst += i_dst;
    }

    width >>= uv_shift;
    height >>= uv_shift;

    psrc = (short*)src_uv;
    pdst = dst[1];
    i_src = src_stridec >> 1;
    i_dst = dst_stride[1];
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = (unsigned char)psrc[j << 1];
        }
        psrc += i_src;
        pdst += i_dst;
    }

    psrc = ((short*)src_uv) + 1;
    pdst = dst[2];
    i_dst = dst_stride[2];
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pdst[j] = (unsigned char)psrc[j << 1];
        }
        psrc += i_src;
        pdst += i_dst;
    }
}

void reset_map_scu(com_scu_t *map_scu, int length)
{
    const com_scu_t mask = { 0, 0, 0, 0, 1, 0, 0 };
    char *p = (char*)map_scu;
    char *s = (char*)&mask;

    for (int i = 0; i < length; i++) {
        p[i] = p[i] & *s;
    }
}

void uavs3d_funs_init_pixel_opt_c()
{
    uavs3d_funs_handle.padding_rows_luma = padding_rows_luma;
    uavs3d_funs_handle.padding_rows_chroma = padding_rows_chroma;
    uavs3d_funs_handle.conv_fmt_8bit = conv_fmt_8bit_c;
    uavs3d_funs_handle.conv_fmt_16bit = conv_fmt_16bit_c;
    uavs3d_funs_handle.conv_fmt_16to8bit = conv_fmt_16to8bit_c;
    uavs3d_funs_handle.reset_map_scu = reset_map_scu;
}

void uavs3d_funs_init_c() {
    uavs3d_funs_init_mc_c();
    uavs3d_funs_init_deblock_c();
    uavs3d_funs_init_sao_c();
    uavs3d_funs_init_alf_c();
    uavs3d_funs_init_intra_pred_c();
    uavs3d_funs_init_pixel_opt_c();
    uavs3d_funs_init_recon_c();
    uavs3d_funs_init_itrans_c();
}