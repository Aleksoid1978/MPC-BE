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

#ifndef __PARSER_H__
#define __PARSER_H__

#include "dec_type.h"

void lbac_init        (com_lbac_t *lbac, u8 *cur, u8* end);
int  lbac_dec_bin_trm (com_lbac_t *lbac);

int  dec_parse_sqh              (com_bs_t * bs, com_seqh_t * seqhdr);
int  dec_parse_pic_header       (com_bs_t * bs, com_pic_header_t * pichdr, com_seqh_t * seqhdr, com_pic_manager_t *pm);
int  dec_parse_patch_header     (com_bs_t * bs, com_seqh_t *seqhdr, com_pic_header_t * ph, com_patch_header_t * pichdr);
int  dec_parse_patch_end        (com_bs_t * bs);
int  dec_parse_ext_and_usr_data (com_bs_t * bs, com_seqh_t *seqhdr, com_pic_header_t * pichdr, int i, int slicetype);

int  dec_parse_lcu_delta_qp (com_lbac_t * lbac, int last_dqp);
void dec_parse_sao_param    (com_core_t* core, int lcu_idx, com_sao_param_t *sao_cur_param);
u32  dec_parse_alf_enable   (com_lbac_t * lbac, int compIdx);

s8   dec_parse_split_mode             (com_core_t *core, com_lbac_t *lbac, int split_tab, int cu_width, int cu_height, int qt_depth, int bet_depth);
u8   dec_parse_cons_pred_mode_child   (com_lbac_t * lbac);

int  dec_parse_cu_header        (com_core_t * core);
int  dec_parse_cu_header_chroma (com_core_t * core);
int  dec_parse_run_length_cc    (com_core_t *core, s16 *coef, int log2_w, int log2_h, int ch_type);
void dec_parse_ipcm_start       (com_lbac_t *lbac);
int  dec_parse_ipcm             (com_lbac_t *lbac, int *bit_left, int bits);

#endif /* __PARSER_H__ */
