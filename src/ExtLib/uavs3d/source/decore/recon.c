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

void com_recon_l(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i, j;

    if (cbf == 0) {
        for (i = 0; i < height; i++) {
            memcpy(rec, pred, width * sizeof(pel));
            rec += i_rec;
            pred += i_pred;
        }
    } else {
        int max_val = (1 << bit_depth) - 1;

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                int t0 = *resi++ + pred[j];
                rec[j] = COM_CLIP3(0, max_val, t0);
            }
            pred += i_pred;
            rec += i_rec;
        }
    }
}

void com_recon_c(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i, j;
    int width2 = width << 1;
    pel *p = pred;
    pel *r = rec;
    int max_val = (1 << bit_depth) - 1;

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                int j2 = j << 1;
                int t0 = resi_u[j] + p[j2    ];
                int t1 = resi_v[j] + p[j2 + 1];
                r[j2    ] = COM_CLIP3(0, max_val, t0);
                r[j2 + 1] = COM_CLIP3(0, max_val, t1);
            }
            r += i_rec;
            resi_u += width;
            resi_v += width;
            p += width2;
        }
    } else {
        if (cbf[0] == 0) {
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++) {
                    int j2 = j << 1;
                    int t0 = resi_v[j] + p[j2 + 1];
                    r[j2    ] = p[j2];
                    r[j2 + 1] = COM_CLIP3(0, max_val, t0);
                }
                r += i_rec;
                resi_v += width;
                p += width2;
            }
        }
        else {
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++) {
                    int j2 = j << 1;
                    int t0 = resi_u[j] + p[j2];
                    r[j2    ] = COM_CLIP3(0, max_val, t0);
                    r[j2 + 1] = p[j2 + 1];
                }
                r += i_rec;
                resi_u += width;
                p += width2;
            }
        }
    }
}

void uavs3d_funs_init_recon_c() {
    int i;
    for (i = 0; i < BLOCK_WIDTH_TYPES_NUM; i++) {
        uavs3d_funs_handle.recon_luma[i] = com_recon_l;
        uavs3d_funs_handle.recon_chroma[i] = com_recon_c;
    }
}