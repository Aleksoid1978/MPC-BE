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

#include "modules.h"

/******************   DCT-2   ******************************************/

static uavs3d_inline void dct_butterfly_h4(s16* src, s16* dst, int line, int shift, int bit_depth) {
    int j;
    int E[2], O[2];
    int add = 1 << (shift - 1);
    int max_tr_val = (1 << bit_depth) - 1;
    int min_tr_val = -(1 << bit_depth);
    s8* dct_coeffs = g_tbl_itrans[DCT2][1];

#define DCT_COEFFS(i, j) dct_coeffs[i*4 + j]

    for (j = 0; j < line; j++)
    {
        O[0] = DCT_COEFFS(1, 0) * src[1 * line + j] + DCT_COEFFS(3, 0) * src[3 * line + j];
        O[1] = DCT_COEFFS(1, 1) * src[1 * line + j] + DCT_COEFFS(3, 1) * src[3 * line + j];
        E[0] = DCT_COEFFS(0, 0) * src[0 * line + j] + DCT_COEFFS(2, 0) * src[2 * line + j];
        E[1] = DCT_COEFFS(0, 1) * src[0 * line + j] + DCT_COEFFS(2, 1) * src[2 * line + j];
        
        dst[j * 4 + 0] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[0] + O[0] + add) >> shift);
        dst[j * 4 + 1] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[1] + O[1] + add) >> shift);
        dst[j * 4 + 2] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[1] - O[1] + add) >> shift);
        dst[j * 4 + 3] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[0] - O[0] + add) >> shift);
    }
#undef DCT_COEFFS

}

static uavs3d_inline void dct_butterfly_h8(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth) {
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);
    int max_tr_val = (1 << bit_depth) - 1;
    int min_tr_val = -(1 << bit_depth);
    s8* dct_coeffs = g_tbl_itrans[DCT2][2];

#define DCT_COEFFS(i, j) dct_coeffs[i*8 + j]
    for (j = 0; j < line; j++)
    {
        for (k = 0; k < 4; k++)
        {
            O[k] = DCT_COEFFS(1, k) * src[1 * i_src + j] + DCT_COEFFS(3, k) * src[3 * i_src + j] +
                   DCT_COEFFS(5, k) * src[5 * i_src + j] + DCT_COEFFS(7, k) * src[7 * i_src + j];
        }
        EO[0] = DCT_COEFFS(2, 0) * src[2 * i_src + j] + DCT_COEFFS(6, 0) * src[6 * i_src + j];
        EO[1] = DCT_COEFFS(2, 1) * src[2 * i_src + j] + DCT_COEFFS(6, 1) * src[6 * i_src + j];
        EE[0] = DCT_COEFFS(0, 0) * src[0 * i_src + j] + DCT_COEFFS(4, 0) * src[4 * i_src + j];
        EE[1] = DCT_COEFFS(0, 1) * src[0 * i_src + j] + DCT_COEFFS(4, 1) * src[4 * i_src + j];
        
        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];
        for (k = 0; k < 4; k++)
        {
            dst[j * 8 + k] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[k] + O[k] + add) >> shift);
            dst[j * 8 + k + 4] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[3 - k] - O[3 - k] + add) >> shift);
        }
    }
#undef DCT_COEFFS

}

static uavs3d_inline void dct_butterfly_h16(s16 *src, int i_src, s16 *dst, int line, int shift, int bit_depth)
{
    int j, k;
    int E[8], O[8];
    int EE[4], EO[4];
    int EEE[2], EEO[2];
    int add = 1 << (shift - 1);
    int max_tr_val = (1 << bit_depth) - 1;
    int min_tr_val = -(1 << bit_depth);
    s8* dct_coeffs = g_tbl_itrans[DCT2][3];

#define DCT_COEFFS(i, j) dct_coeffs[i*16 + j]
    for(j = 0; j < line; j++)
    {
        for(k = 0; k < 8; k++)
        {
            O[k] = DCT_COEFFS(1 , k) * src[1 *i_src + j] + DCT_COEFFS(3 , k) * src[3 *i_src + j] +
                   DCT_COEFFS(5 , k) * src[5 *i_src + j] + DCT_COEFFS(7 , k) * src[7 *i_src + j] +
                   DCT_COEFFS(9 , k) * src[9 *i_src + j] + DCT_COEFFS(11, k) * src[11*i_src + j] +
                   DCT_COEFFS(13, k) * src[13*i_src + j] + DCT_COEFFS(15, k) * src[15*i_src + j];
        }
        for(k = 0; k < 4; k++)
        {
            EO[k] = DCT_COEFFS(2 , k) * src[2  * i_src + j] + DCT_COEFFS(6 , k) * src[6 * i_src + j] +
                    DCT_COEFFS(10, k) * src[10 * i_src + j] + DCT_COEFFS(14, k) * src[14 * i_src + j];
        }
        EEO[0] = DCT_COEFFS(4, 0) * src[4 * i_src + j] + DCT_COEFFS(12, 0) * src[12 * i_src + j];
        EEE[0] = DCT_COEFFS(0, 0) * src[0 * i_src + j] + DCT_COEFFS(8 , 0) * src[8 * i_src + j];
        EEO[1] = DCT_COEFFS(4, 1) * src[4 * i_src + j] + DCT_COEFFS(12, 1) * src[12 * i_src + j];
        EEE[1] = DCT_COEFFS(0, 1) * src[0 * i_src + j] + DCT_COEFFS(8 , 1) * src[8 * i_src + j];

        for(k = 0; k < 2; k++)
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k + 2] = EEE[1 - k] - EEO[1 - k];
        }
        for(k = 0; k < 4; k++)
        {
            E[k] = EE[k] + EO[k];
            E[k + 4] = EE[3 - k] - EO[3 - k];
        }
        for(k = 0; k < 8; k++)
        {
            dst[j * 16 + k] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[k] + O[k] + add) >> shift);
            dst[j * 16 + k + 8] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[7 - k] - O[7 - k] + add) >> shift);
        }
    }
#undef DCT_COEFFS
}

static uavs3d_inline void dct_butterfly_h32(s16 *src, int i_src, s16 *dst, int line, int shift, int bit_depth)
{
    int j, k;
    int E[16], O[16];
    int EE[8], EO[8];
    int EEE[4], EEO[4];
    int EEEE[2], EEEO[2];
    int add = 1 << (shift - 1);
    int max_tr_val = (1 << bit_depth) - 1;
    int min_tr_val = -(1 << bit_depth);
    s8* dct_coeffs = g_tbl_itrans[DCT2][4];

#define DCT_COEFFS(i, j) dct_coeffs[i*32 + j]
    for(j = 0; j < line; j++) {
        for(k = 0; k < 16; k++) {
            O[k] = DCT_COEFFS( 1, k)*src[ 1*i_src+j] + \
                   DCT_COEFFS( 3, k)*src[ 3*i_src+j] + \
                   DCT_COEFFS( 5, k)*src[ 5*i_src+j] + \
                   DCT_COEFFS( 7, k)*src[ 7*i_src+j] + \
                   DCT_COEFFS( 9, k)*src[ 9*i_src+j] + \
                   DCT_COEFFS(11, k)*src[11*i_src+j] + \
                   DCT_COEFFS(13, k)*src[13*i_src+j] + \
                   DCT_COEFFS(15, k)*src[15*i_src+j] + \
                   DCT_COEFFS(17, k)*src[17*i_src+j] + \
                   DCT_COEFFS(19, k)*src[19*i_src+j] + \
                   DCT_COEFFS(21, k)*src[21*i_src+j] + \
                   DCT_COEFFS(23, k)*src[23*i_src+j] + \
                   DCT_COEFFS(25, k)*src[25*i_src+j] + \
                   DCT_COEFFS(27, k)*src[27*i_src+j] + \
                   DCT_COEFFS(29, k)*src[29*i_src+j] + \
                   DCT_COEFFS(31, k)*src[31*i_src+j];
        }
        for(k = 0; k < 8; k++)
        {
            EO[k] = DCT_COEFFS( 2, k)*src[ 2*i_src+j] + \
                    DCT_COEFFS( 6, k)*src[ 6*i_src+j] + \
                    DCT_COEFFS(10, k)*src[10*i_src+j] + \
                    DCT_COEFFS(14, k)*src[14*i_src+j] + \
                    DCT_COEFFS(18, k)*src[18*i_src+j] + \
                    DCT_COEFFS(22, k)*src[22*i_src+j] + \
                    DCT_COEFFS(26, k)*src[26*i_src+j] + \
                    DCT_COEFFS(30, k)*src[30*i_src+j];
        }
        for(k = 0; k < 4; k++)
        {
            EEO[k] = DCT_COEFFS( 4, k)*src[ 4*i_src+j] + \
                     DCT_COEFFS(12, k)*src[12*i_src+j] + \
                     DCT_COEFFS(20, k)*src[20*i_src+j] + \
                     DCT_COEFFS(28, k)*src[28*i_src+j];
        }
        EEEO[0] = DCT_COEFFS(8, 0)*src[8*i_src+j] + DCT_COEFFS(24, 0)*src[24*i_src+j];
        EEEO[1] = DCT_COEFFS(8, 1)*src[8*i_src+j] + DCT_COEFFS(24, 1)*src[24*i_src+j];
        EEEE[0] = DCT_COEFFS(0, 0)*src[0*i_src+j] + DCT_COEFFS(16, 0)*src[16*i_src+j];
        EEEE[1] = DCT_COEFFS(0, 1)*src[0*i_src+j] + DCT_COEFFS(16, 1)*src[16*i_src+j];
        EEE[0] = EEEE[0] + EEEO[0];
        EEE[3] = EEEE[0] - EEEO[0];
        EEE[1] = EEEE[1] + EEEO[1];
        EEE[2] = EEEE[1] - EEEO[1];
        for (k=0; k<4; k++)
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k+4] = EEE[3-k] - EEO[3-k];
        }
        for (k=0; k<8; k++)
        {
            E[k] = EE[k] + EO[k];
            E[k+8] = EE[7-k] - EO[7-k];
        }
        for (k=0; k<16; k++)
        {
            dst[j*32+k]    = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[k] + O[k] + add)>>shift);
            dst[j*32+k+16] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[15-k] - O[15-k] + add)>>shift);
        }
    }
#undef DCT_COEFFS
}

static uavs3d_inline void dct_butterfly_h64(s16 *src, int i_src, s16 *dst, int line, int shift, int bit_depth)
{
    const int tx_size = 64;
    int j, k;
    int E[32], O[32];
    int EE[16], EO[16];
    int EEE[8], EEO[8];
    int EEEE[4], EEEO[4];
    int add = 1 << (shift - 1);
    int max_tr_val = (1 << bit_depth) - 1;
    int min_tr_val = -(1 << bit_depth);
    s8* dct_coeffs = g_tbl_itrans[DCT2][5];

#define DCT_COEFFS(i, j) dct_coeffs[i*64 + j]
    for (j = 0; j < line; j++)
    {
        for (k = 0; k < 32; k++)
        {
            O[k] = DCT_COEFFS(1 , k) * src[     i_src] + DCT_COEFFS(3 , k) * src[3  * i_src] + DCT_COEFFS(5 , k) * src[5  * i_src] + DCT_COEFFS(7 , k) * src[7  * i_src] +
                   DCT_COEFFS(9 , k) * src[9  * i_src] + DCT_COEFFS(11, k) * src[11 * i_src] + DCT_COEFFS(13, k) * src[13 * i_src] + DCT_COEFFS(15, k) * src[15 * i_src] +
                   DCT_COEFFS(17, k) * src[17 * i_src] + DCT_COEFFS(19, k) * src[19 * i_src] + DCT_COEFFS(21, k) * src[21 * i_src] + DCT_COEFFS(23, k) * src[23 * i_src] +
                   DCT_COEFFS(25, k) * src[25 * i_src] + DCT_COEFFS(27, k) * src[27 * i_src] + DCT_COEFFS(29, k) * src[29 * i_src] + DCT_COEFFS(31, k) * src[31 * i_src];
        }
        for (k = 0; k < 16; k++)
        {
            EO[k] = DCT_COEFFS(2 , k) * src[2  * i_src] + DCT_COEFFS(6 , k) * src[6  * i_src] + DCT_COEFFS(10, k) * src[10 * i_src] + DCT_COEFFS(14, k) * src[14 * i_src] +
                    DCT_COEFFS(18, k) * src[18 * i_src] + DCT_COEFFS(22, k) * src[22 * i_src] + DCT_COEFFS(26, k) * src[26 * i_src] + DCT_COEFFS(30, k) * src[30 * i_src];
        }
        for (k = 0; k < 8; k++)
        {
            EEO[k] = DCT_COEFFS(4, k) * src[4 * i_src] + DCT_COEFFS(12, k) * src[12 * i_src] + DCT_COEFFS(20, k) * src[20 * i_src] + DCT_COEFFS(28, k) * src[28 * i_src];
        }
        for (k = 0; k<4; k++)
        {
            EEEO[k] = DCT_COEFFS(8, k) * src[8 * i_src] + DCT_COEFFS(24, k) * src[24 * i_src];
        }

        EEEE[0] = DCT_COEFFS(0, 0) * src[0] +   DCT_COEFFS(16, 0)  * src[16 * i_src];
        EEEE[1] = DCT_COEFFS(0, 1) * src[0] +   DCT_COEFFS(16, 1)  * src[16 * i_src];
        EEEE[2] = DCT_COEFFS(0, 1) * src[0] + (-DCT_COEFFS(16, 1)) * src[16 * i_src];
        EEEE[3] = DCT_COEFFS(0, 0) * src[0] + (-DCT_COEFFS(16, 0)) * src[16 * i_src];

        for (k = 0; k < 4; k++)
        {
            EEE[k] = EEEE[k] + EEEO[k];
            EEE[k + 4] = EEEE[3 - k] - EEEO[3 - k];
        }
        for (k = 0; k < 8; k++)
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k + 8] = EEE[7 - k] - EEO[7 - k];
        }
        for (k = 0; k < 16; k++)
        {
            E[k] = EE[k] + EO[k];
            E[k + 16] = EE[15 - k] - EO[15 - k];
        }
        for (k = 0; k < 32; k++)
        {
            dst[k] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[k] + O[k] + add) >> shift);
            dst[k + 32] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (E[31 - k] - O[31 - k] + add) >> shift);
        }
        src++;
        dst += tx_size;
    }
#undef DCT_COEFFS
}

static void itrans_dct2_h4_w4(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[4 * 4];
    dct_butterfly_h4(src, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h4(tmp, dst, 4, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h4_w8(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[4 * 8];
    dct_butterfly_h4(src, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h8(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h4_w16(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[4 * 16];
    dct_butterfly_h4(src, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h16(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h4_w32(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[4 * 32];
    dct_butterfly_h4(src, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h32(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

static void itrans_dct2_h8_w4(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[8 * 4];
    dct_butterfly_h8(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h4(tmp, dst, 8, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h8_w8(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[8 * 8];
    dct_butterfly_h8(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h8(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h8_w16(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[8 * 16];
    dct_butterfly_h8(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h16(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h8_w32(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[8 * 32];
    dct_butterfly_h8(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h32(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h8_w64(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[8 * 64];
    dct_butterfly_h8(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h64(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

static void itrans_dct2_h16_w4(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[16 * 4];
    dct_butterfly_h16(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h4(tmp, dst, 16, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h16_w8(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[16 * 8];
    dct_butterfly_h16(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h8(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h16_w16(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[16 * 16];
    dct_butterfly_h16(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h16(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h16_w32(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[16 * 32];
    dct_butterfly_h16(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h32(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h16_w64(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[16 * 64];
    dct_butterfly_h16(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h64(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

static void itrans_dct2_h32_w4(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[32 * 4];
    dct_butterfly_h32(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h4(tmp, dst, 32, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h32_w8(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[32 * 8];
    dct_butterfly_h32(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h8(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h32_w16(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[32 * 16];
    dct_butterfly_h32(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h16(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h32_w32(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[32 * 32];
    dct_butterfly_h32(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h32(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h32_w64(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[32 * 64];
    dct_butterfly_h32(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h64(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

static void itrans_dct2_h64_w8(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[64 * 8];
    dct_butterfly_h64(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h8(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h64_w16(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[64 * 16];
    dct_butterfly_h64(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h16(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h64_w32(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[64 * 32];
    dct_butterfly_h64(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h32(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}
static void itrans_dct2_h64_w64(s16 *src, s16 *dst, int bit_depth)
{
    s16 tmp[64 * 64];
    dct_butterfly_h64(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct_butterfly_h64(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

/******************   DCT-8   ******************************************/

void itx_dct8_pb4(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input tmp, output block
{
    int i;
    int rnd_factor = 1 << (shift - 1);
    int c[4];
    const int  reducedLine = line;

    for (i = 0; i<reducedLine; i++) {
        c[0] = coeff[0 * line] + coeff[3 * line];
        c[1] = coeff[2 * line] + coeff[0 * line];
        c[2] = coeff[3 * line] - coeff[2 * line];
        c[3] = it[1] * coeff[1 * line];

        block[0] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[3] * c[0] + it[2] * c[1] + c[3] + rnd_factor) >> shift);
        block[1] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[1] * (coeff[0 * line] - coeff[2 * line] - coeff[3 * line]) + rnd_factor) >> shift);
        block[2] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[3] * c[2] + it[2] * c[0] - c[3] + rnd_factor) >> shift);
        block[3] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[3] * c[1] - it[2] * c[2] - c[3] + rnd_factor) >> shift);

        block += 4;
        coeff++;
    }
}

void itx_dct8_pb8(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 8;
    const int  reducedLine = line;
    const int  cutoff = 8;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

void itx_dct8_pb16(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 16;
    const int  reducedLine = line;
    const int  cutoff = uiTrSize;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

void itx_dct8_pb32(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 32;
    const int  reducedLine = line;
    const int  cutoff = uiTrSize;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

/******************   DST-7   ******************************************/
void itx_dst7_pb4(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input tmp, output block
{
    int i, c[4];
    int rnd_factor = 1 << (shift - 1);
    const int  reducedLine = line;

    for (i = 0; i<reducedLine; i++)
    {
        c[0] = coeff[0 * line] + coeff[2 * line];
        c[1] = coeff[2 * line] + coeff[3 * line];
        c[2] = coeff[0 * line] - coeff[3 * line];
        c[3] = it[2] * coeff[1 * line];

        block[0] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[0] * c[0] + it[1] * c[1] + c[3] + rnd_factor) >> shift);
        block[1] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[1] * c[2] - it[0] * c[1] + c[3] + rnd_factor) >> shift);
        block[2] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[2] * (coeff[0 * line] - coeff[2 * line] + coeff[3 * line]) + rnd_factor) >> shift);
        block[3] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (it[1] * c[0] + it[0] * c[2] - c[3] + rnd_factor) >> shift);

        block += 4;
        coeff++;
    }
}

void itx_dst7_pb8(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 8;
    const int  reducedLine = line;
    const int  cutoff = uiTrSize;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

void itx_dst7_pb16(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 16;
    const int  reducedLine = line;
    const int  cutoff = uiTrSize;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

void itx_dst7_pb32(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *it)  // input block, output coeff
{
    int i, j, k, iSum;
    int rnd_factor = 1 << (shift - 1);
    const int uiTrSize = 32;
    const int  reducedLine = line;
    const int  cutoff = uiTrSize;

    for (i = 0; i<reducedLine; i++)
    {
        for (j = 0; j<uiTrSize; j++)
        {
            iSum = 0;
            for (k = 0; k<cutoff; k++)
            {
                iSum += coeff[k*line] * it[k*uiTrSize + j];
            }
            block[j] = (s16)COM_CLIP3(min_tr_val, max_tr_val, (int)(iSum + rnd_factor) >> shift);
        }
        block += uiTrSize;
        coeff++;
    }
}

static void xCTr_4_1d_Inv_Ver_Hor(s16 *src, s16 *resi, int bit_depth)
{
    int i, j, k, sum;
    int rnd_factor = 1 << (5 - 1);
    //s16 *p, *s, tmp[16];
    int min_pixel = -(1 << bit_depth);
    int max_pixel = (1 << bit_depth) - 1;
    int shift = 22 - bit_depth;
    int add = 1 << (shift - 1);
 
    s16 tmpSrc[4][4];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            sum = rnd_factor;
            for (k = 0; k < 4; k++) {
                sum += g_tbl_itrans_c4[i][k] * src[k * 4 + j];
            }
            tmpSrc[i][j] = (s16)COM_CLIP3(-32768, 32767, sum >> 5);
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            sum = add;
            for (k = 0; k < 4; k++) {
                sum += g_tbl_itrans_c4[i][k] * tmpSrc[j][k];
            }
            resi[j* 4 + i] = (s16)COM_CLIP3(min_pixel, max_pixel, sum >> shift);
        }
    }
}



static void xTr2nd_8_1d_Inv_Vert(s16 *src, int i_src)
{
    int i, j, k, sum;
    int tmpSrc[4][4];
    int rnd_factor = 1 << (7 - 1);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            tmpSrc[i][j] = src[i * i_src + j];
        }
    }
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            sum = rnd_factor;
            for (k = 0; k < 4; k++)
            {
                sum += g_tbl_itrans_c8[i][k] * tmpSrc[k][j];
            }
            src[i * i_src + j] = (s16)COM_CLIP3(-32768, 32767, sum >> 7);
        }
    }
}

static void xTr2nd_8_1d_Inv_Hor(s16 *src, int i_src)
{
    int i, j, k, sum;
    int tmpSrc[4][4];
    int rnd_factor = 1 << (7 - 1);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            tmpSrc[i][j] = src[i * i_src + j];
        }
    }
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            sum = rnd_factor;
            for (k = 0; k < 4; k++)
            {
                sum += g_tbl_itrans_c8[i][k] * tmpSrc[j][k];
            }
            src[j* i_src + i] = (s16)COM_CLIP3(-32768, 32767, sum >> 7);
        }
    }
}

static void xTr2nd_8_1d_Inv_Hor_Ver(s16 *src, int i_src)
{
    int i, j, sum;
    s16 *p, *s, tmp[16];
    const int rnd_factor = 1 << (7 - 1);

    p = tmp;
    for (i = 0; i < 4; i++) {
        s = src;
        for (j = 0; j < 4; j++) {
            sum = rnd_factor +  g_tbl_itrans_c8[i][0] * s[0] + 
                                g_tbl_itrans_c8[i][1] * s[1] +
                                g_tbl_itrans_c8[i][2] * s[2] +
                                g_tbl_itrans_c8[i][3] * s[3];
           
            p[j] = (s16)COM_CLIP3(-32768, 32767, sum >> 7);
            s += i_src;
        }
        p += 4;
    }
    s = src;
    for (i = 0; i < 4; i++) {
        p = tmp;
        for (j = 0; j < 4; j++) {
            sum = rnd_factor +  g_tbl_itrans_c8[i][0] * p[0] + 
                                g_tbl_itrans_c8[i][1] * p[1] +
                                g_tbl_itrans_c8[i][2] * p[2] +
                                g_tbl_itrans_c8[i][3] * p[3];
            s[j] = (s16)COM_CLIP3(-32768, 32767, sum >> 7);
            p += 4;
        }
        s += i_src;
    }
}

void com_itrans(com_core_t *core, int plane, int blk_idx, s16 *coef, s16 *resi, int log2_w, int log2_h, int bit_depth, int sec_t_vh, int alt_4x4)
{
    if (alt_4x4 && log2_w == 2 && log2_h == 2) {
        xCTr_4_1d_Inv_Ver_Hor(coef, resi, bit_depth);
    } else {
        int max_tr_val, min_tr_val;

        if (sec_t_vh == 3) {
            xTr2nd_8_1d_Inv_Hor_Ver(coef, 1 << log2_w);
        } else if (sec_t_vh & 1) {
            xTr2nd_8_1d_Inv_Hor(coef, 1 << log2_w);
        } else if (sec_t_vh >> 1) {
            xTr2nd_8_1d_Inv_Vert(coef, 1 << log2_w);
        }
        if (plane == Y_C && core->tb_part == SIZE_NxN) {
            ALIGNED_32(s16 coef_temp[MAX_TR_DIM]);
            max_tr_val = (1 << MAX_TX_DYNAMIC_RANGE) - 1;
            min_tr_val = -(1 << MAX_TX_DYNAMIC_RANGE);
            if (blk_idx >> 1) { // DST7
                uavs3d_funs_handle.itrans_dst7[log2_h - 2](coef, coef_temp, 5, 1 << log2_w, COM_MIN(1 << log2_w, 32), max_tr_val, min_tr_val, g_tbl_itrans[DST7][log2_h - 1]);
            } else {
                uavs3d_funs_handle.itrans_dct8[log2_h - 2](coef, coef_temp, 5, 1 << log2_w, COM_MIN(1 << log2_w, 32), max_tr_val, min_tr_val, g_tbl_itrans[DCT8][log2_h - 1]);
            }
            max_tr_val = (1 << bit_depth) - 1;
            min_tr_val = -(1 << bit_depth);
            if (blk_idx & 1) { // DST7
                uavs3d_funs_handle.itrans_dst7[log2_w - 2](coef_temp, resi, 20 - bit_depth, 1 << log2_h, 1 << log2_h, max_tr_val, min_tr_val, g_tbl_itrans[DST7][log2_w - 1]);
            }
            else {
                uavs3d_funs_handle.itrans_dct8[log2_w - 2](coef_temp, resi, 20 - bit_depth, 1 << log2_h, 1 << log2_h, max_tr_val, min_tr_val, g_tbl_itrans[DCT8][log2_w - 1]);
            }
        } else {
            uavs3d_funs_handle.itrans_dct2[log2_h - 1][log2_w - 1](coef, resi, bit_depth);
        }
    }
}

void uavs3d_funs_init_itrans_c() {
    uavs3d_funs_handle.itrans_dct2[1][1] = itrans_dct2_h4_w4;
    uavs3d_funs_handle.itrans_dct2[1][2] = itrans_dct2_h4_w8;
    uavs3d_funs_handle.itrans_dct2[1][3] = itrans_dct2_h4_w16;
    uavs3d_funs_handle.itrans_dct2[1][4] = itrans_dct2_h4_w32;

    uavs3d_funs_handle.itrans_dct2[2][1] = itrans_dct2_h8_w4;
    uavs3d_funs_handle.itrans_dct2[2][2] = itrans_dct2_h8_w8;
    uavs3d_funs_handle.itrans_dct2[2][3] = itrans_dct2_h8_w16;
    uavs3d_funs_handle.itrans_dct2[2][4] = itrans_dct2_h8_w32;
    uavs3d_funs_handle.itrans_dct2[2][5] = itrans_dct2_h8_w64;

    uavs3d_funs_handle.itrans_dct2[3][1] = itrans_dct2_h16_w4;
    uavs3d_funs_handle.itrans_dct2[3][2] = itrans_dct2_h16_w8;
    uavs3d_funs_handle.itrans_dct2[3][3] = itrans_dct2_h16_w16;
    uavs3d_funs_handle.itrans_dct2[3][4] = itrans_dct2_h16_w32;
    uavs3d_funs_handle.itrans_dct2[3][5] = itrans_dct2_h16_w64;

    uavs3d_funs_handle.itrans_dct2[4][1] = itrans_dct2_h32_w4;
    uavs3d_funs_handle.itrans_dct2[4][2] = itrans_dct2_h32_w8;
    uavs3d_funs_handle.itrans_dct2[4][3] = itrans_dct2_h32_w16;
    uavs3d_funs_handle.itrans_dct2[4][4] = itrans_dct2_h32_w32;
    uavs3d_funs_handle.itrans_dct2[4][5] = itrans_dct2_h32_w64;

    uavs3d_funs_handle.itrans_dct2[5][2] = itrans_dct2_h64_w8;
    uavs3d_funs_handle.itrans_dct2[5][3] = itrans_dct2_h64_w16;
    uavs3d_funs_handle.itrans_dct2[5][4] = itrans_dct2_h64_w32;
    uavs3d_funs_handle.itrans_dct2[5][5] = itrans_dct2_h64_w64;

    uavs3d_funs_handle.itrans_dct8[0] = itx_dct8_pb4;
    uavs3d_funs_handle.itrans_dct8[1] = itx_dct8_pb8;
    uavs3d_funs_handle.itrans_dct8[2] = itx_dct8_pb16;
    uavs3d_funs_handle.itrans_dct8[3] = itx_dct8_pb32;

    uavs3d_funs_handle.itrans_dst7[0] = itx_dst7_pb4;
    uavs3d_funs_handle.itrans_dst7[1] = itx_dst7_pb8;
    uavs3d_funs_handle.itrans_dst7[2] = itx_dst7_pb16;
    uavs3d_funs_handle.itrans_dst7[3] = itx_dst7_pb32;
}
