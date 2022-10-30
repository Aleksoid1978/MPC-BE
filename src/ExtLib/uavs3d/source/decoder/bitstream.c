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

#include "dec_type.h"

static tab_u8 tbl_zero_count4[16] =
{
    4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

void dec_bs_init(com_bs_t * bs, u8 * buf, int size)
{
    bs->cur      = buf;
    bs->end      = buf + size - 1; 
    bs->code     = 0;
    bs->leftbits = 0;
}

static int dec_bs_read_bytes(com_bs_t * bs, int byte)
{
    int shift = 24;
    u32 code = 0;
    int remained = (int)(bs->end - bs->cur) + 1;

    if (byte > remained) {
        if (remained <= 0) {
            return -1;
        }
        byte = remained;
    }
    bs->leftbits = byte << 3;
    bs->cur     += byte;

    while (byte) {
#if CHECK_RAND_STRM
        if (rand() % 100 == 0) {
            code |= (rand() & 0xFF) << shift;
        } else {
            code |= *(bs->cur - byte) << shift;
        }
#else
        code |= *(bs->cur - byte) << shift;
#endif
        byte--;
        shift -= 8;
    }
    bs->code = code;
    return 0;
}

u32 dec_bs_read(com_bs_t * bs, int bits, u32 min, u32 max)
{
    u32 code = 0;

    if (bs->leftbits < bits) {
        code = bs->code >> (32 - bits);
        bits -= bs->leftbits;
        if (dec_bs_read_bytes(bs, 4)) {
            return min;
        }
    }
    code |= bs->code >> (32 - bits);

    bs->code <<= bits; 
    bs->leftbits -= bits;

    if (code < min || code > max) {
        uavs3d_assert(0);
        code = min;
    }

    return code;
}

int dec_bs_read1(com_bs_t * bs, int val)
{
    int code;
    if (bs->leftbits == 0) {
        if (dec_bs_read_bytes(bs, 4)) {
            return 0;
        }
    }
    code = (int)(bs->code >> 31);
    bs->code    <<= 1;
    bs->leftbits -= 1;

    if (val != -1 && code != val) {
        uavs3d_assert(0);
        code = val;
    }

    return code;
}

static int dec_bs_clz_in_code(u32 code)
{
    if (code == 0) {
        return 32; /* protect infinite loop */
    }

    int bits4 =  0;
    int clz   =  0;
    int shift = 28;

    while (bits4 == 0 && shift >= 0) {
        bits4 = (code >> shift) & 0xf;
        clz += tbl_zero_count4[bits4];
        shift -= 4;
    }
    return clz;
}

u32 dec_bs_read_ue(com_bs_t * bs, u32 min, u32 max)
{
    if ((bs->code >> 31) == 1) {
        bs->code    <<= 1;
        bs->leftbits -= 1;
        return 0;
    }

    int clz = 0;
    if (bs->code == 0) {
        clz = bs->leftbits;
        if (dec_bs_read_bytes(bs, 4)) {
            return min;
        }
    }

    int len = dec_bs_clz_in_code(bs->code);
    clz += len;

    if (clz == 0) {
        bs->code <<= 1;
        bs->leftbits--;
        return 0;
    }

    u32 code = dec_bs_read(bs, len + clz + 1, 0, COM_UINT32_MAX) - 1;

    if (code < min || code > max) {
        uavs3d_assert(0);
        code = min;
    }

    return code;
}

int dec_bs_read_se(com_bs_t * bs, int min, int max)
{
    int val;
    int code;

    val = dec_bs_read_ue(bs, 0, COM_UINT32_MAX);
    code = ((val & 0x01) ? ((val + 1) >> 1) : -(val >> 1));

    if (code < min || code > max) {
        uavs3d_assert(0);
        code = min;
    }

    return code;
}

u32 dec_bs_next(com_bs_t * bs, int size)
{
    u32 code = 0;
    int shift = 24;
    u32 newcode = 0;
    int byte = 4;
    u8* cur = bs->cur;
    
    if (bs->leftbits < size) {
        code  = bs->code >> (32 - size);
        size -= bs->leftbits;
        int remained = (int)(bs->end - bs->cur) + 1;

        if (byte > remained) {
            byte = remained;
        }

        cur += byte;
        while (byte) {
            newcode |= *(cur - byte) << shift;
            byte--;
            shift -= 8;
        }
    } else {
        newcode = bs->code;
    }
    code |= newcode >> (32 - size);

    return code;
}

static uavs3d_always_inline int dec_bs_find_start_code(const u8 *src, int length, int *left)
{
    int i;

#define FIND_FIRST_ZERO         \
    if (i > 0 && !src[i])  i--; \
    while (src[i])         i++

#define AV_RN32A(x) (*(unsigned int*)(x))

    for (i = 0; i + 1 < length; i += 5) {
        if (!((~AV_RN32A(src + i) & (AV_RN32A(src + i) - 0x01000101U)) & 0x80008080U)) {
            continue;
        }
        FIND_FIRST_ZERO;

        if (i + 3 < length && src[i + 1] == 0 && src[i + 2] == 1) {
            *left = length - i;
            return 1;
        }
        i -= 3;
    }
    return 0;
}

u8* dec_bs_demulate(u8 *start, u8 *end)
{
    int i;
    int len = (int)(end - start);
    int prev_bytes = 0;
    u8 *d, *s;
    int bit_pos;
    int left_bits;

#define DEMU_FIND_FIRST_ZERO        \
    if (i > 0 && !start[i])  i--;   \
    while (start[i])         i++

    // look for the first emulate code
    for (i = 0; i + 1 < len; i += 5) {
        if (!((~AV_RN32A(start + i) & (AV_RN32A(start + i) - 0x01000101U)) & 0x80008080U)) {
            continue;
        }
        DEMU_FIND_FIRST_ZERO;

        if (i + 3 < len && start[i + 1] == 0 && start[i + 2] == 2) {
            break;
        }
        i -= 3;
    }

    if (i + 1 >= len) {
        return end;
    }

    d = s = start + i;
    left_bits = (len - i) * 8;
    bit_pos = 0;

    for (i = 0; i < left_bits; i += 8) {
        u8 val = *s++;
        prev_bytes = ((prev_bytes << 8) | val) & 0xffffff;

        if (prev_bytes != 2) {
            *d++ = (val << bit_pos) | ((*s) >> (8 - bit_pos));
        }
        else {
            val = 0;
            bit_pos += 2;

            if (bit_pos == 8) {
                val = *s++;
                *d++ = val;
                prev_bytes = ((prev_bytes << 8) | val) & 0xffffff;
                bit_pos = 0;
            }
            else {
                *d++ = (*s) >> (8 - bit_pos);
            }
        }
    }

    return d;
}

u8* dec_bs_get_one_unit(com_bs_t *bs, u8 **next_start)
{
    u8 *start = bs->cur;
    u8 *end   = bs->end + 1;
    int len = (int)(end - start);
    int left_bytes;

    if (dec_bs_find_start_code(start + 4, len - 4, &left_bytes)) {
        end = start + len - left_bytes;
    }

    *next_start = end;

    return dec_bs_demulate(start - 1, end);
}
