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

#ifndef __COM_TABLE_H__
#define __COM_TABLE_H__

#define ITRANS_SIZE_TYPES 6
#define ITRANS_COEFFS_SIZE ((2 * 2 + 4 * 4 + 8 * 8 + 16 * 16 + 32 * 32 + 64 * 64) * NUM_TRANS_TYPE)

extern s8*      g_tbl_itrans[NUM_TRANS_TYPE][ITRANS_SIZE_TYPES];
extern s8       g_tbl_itrans_coeffs[ITRANS_COEFFS_SIZE];

extern tab_s8   g_tbl_itrans_c4[4][4];
extern tab_s8   g_tbl_itrans_c8[4][4];

extern tab_u8                g_tbl_part_num[8];
extern const com_part_size_t g_tbl_tb_part[8];

extern tab_s32  g_tbl_ai_tscpm_div[64];

extern tab_s8   g_tbl_log2[257];

extern tab_s8   g_tbl_mc_coeff_luma_hp[16][8];
extern tab_s8   g_tbl_mc_coeff_chroma_hp[32][4];
extern tab_s8   g_tbl_mc_coeff_luma[4][8];
extern tab_s8   g_tbl_mc_coeff_chroma[8][4];

extern tab_u8   g_tbl_qp_chroma_adjust[64];

extern tab_u32  g_tbl_wq_default_param[2][6];
extern tab_u8   g_tbl_wq_default_matrix_4x4[16];
extern tab_u8   g_tbl_wq_default_matrix_8x8[64];
extern tab_u8   g_tbl_wq_model_4x4[4][16];
extern tab_u8   g_tbl_wq_model_8x8[4][64];

extern tab_u16  g_tbl_scan[64516];
extern tab_u16  g_tbl_scan_blkpos[MAX_CU_LOG2][MAX_CU_LOG2];

#endif // __COM_TABLE_H__ 
