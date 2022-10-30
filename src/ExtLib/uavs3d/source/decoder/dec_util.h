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

#ifndef __DEC_UTIL_H__
#define __DEC_UTIL_H__

#include "com_type.h"

int  dec_check_pic_md5(com_pic_t * pic, u8 md5_buf[16]);

void dec_update_map            (com_core_t * core);
void dec_update_map_for_intra  (com_scu_t *map_scu, s8 *map_ipm, int tb_x, int tb_y, int tb_w, int tb_h, int i_scu, int ipm);
           
void dec_derive_mvp            (com_core_t *core, int ref_list, int emvp_flag, int mvr_idx, s16 mvp[MV_D]);
void dec_derive_skip_mv        (com_core_t * core, int spatial_skip_idx);
void dec_derive_skip_mv_affine (com_core_t * core, int mrg_idx);
void dec_derive_skip_mv_umve   (com_core_t * core, int umve_idx);

void dec_scale_affine_mvp      (com_core_t *core, int lidx, CPMV mvp[VER_NUM][MV_D], u8 curr_mvr);
void update_hmvp_candidates    (com_core_t *core);


static u16 uavs3d_always_inline dec_get_avail_intra(int i_scu, int scup, com_scu_t * map_scu)
{
    return (map_scu[scup -         1].coded << AVAIL_BIT_LE) |
           (map_scu[scup - i_scu    ].coded << AVAIL_BIT_UP) |
           (map_scu[scup - i_scu - 1].coded << AVAIL_BIT_UL);
}

void dec_get_split_struct(int split_mode, int x0, int y0, int cu_width, int cu_height, dec_split_info_t* split_struct);
int  dec_get_split_available(com_seqh_t* seqhdr, int x, int y, int cu_w, int cu_h, int qt_depth, int bet_depth, int slice_type);

void    dec_get_part_info(com_core_t *core, com_part_info_t* sub_info);
#define dec_get_part_num(size) (g_tbl_part_num[size])
#define dec_get_tb_part_size_by_pb(pb_part) (g_tbl_tb_part[pb_part])
int     dec_get_pb_idx_by_tb(com_part_size_t pb_part, int idx);
void    dec_get_tb_width_height(int w, int h, com_part_size_t part, int *tb_w, int *tb_h);
void    dec_get_tb_start_pos(int w, int h, com_part_size_t part, int idx, int *pos_x, int *pos_y);

u8   dec_is_separate_tree(int w, int h, com_split_mode_t split);
int  dec_dt_allow(int cu_w, int cu_h, int pred_mode, int max_dt_size);
u8   dec_cons_allow(int w, int h, com_split_mode_t split);


#endif /* __DEC_UTIL_H__ */
