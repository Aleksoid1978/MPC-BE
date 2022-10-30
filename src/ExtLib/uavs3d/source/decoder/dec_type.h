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

#ifndef __DEC_TYPE_H__
#define __DEC_TYPE_H__

#include "modules.h"
#include "uavs3d.h"
#include "bitstream.h"

#define SPLIT_MAX_PART_COUNT 4

typedef struct uavs3d_dec_split_info_t {
    int       part_count;
    int       width[SPLIT_MAX_PART_COUNT];
    int       height[SPLIT_MAX_PART_COUNT];
    int       log_cuw[SPLIT_MAX_PART_COUNT];
    int       log_cuh[SPLIT_MAX_PART_COUNT];
    int       x_pos[SPLIT_MAX_PART_COUNT];
    int       y_pos[SPLIT_MAX_PART_COUNT];
} dec_split_info_t;

typedef struct uavs3d_dec_t {
    u8                    init_flag;

    /* Senquence Level Shared Data */
    com_seqh_t            seqhdr;
    u8                   *alf_idx_map;

    uavs3d_cfg_t          dec_cfg;

    /* CORE data for pic decode, only used in single thread */
    com_core_t           *core;

    int                   frm_nodes;
    int                   frm_node_start;
    int                   frm_node_end;
    com_frm_t            *frm_nodes_list;

    /* current decoding bitstream */
    com_bs_t              bs;

    /* Reference Frame Management */
    com_pic_manager_t     pic_manager;
    int                   cur_decoded_doi;
    int                   output;

    /* Frame Level Parallel */
    threadpool_t         *frm_threads_pool;
    int                   frm_threads_nodes;

    void                 *callback;
} uavs3d_dec_t;


#include "dec_util.h"
#include "parser.h"

#endif /* __DEC_DEF_H__ */
