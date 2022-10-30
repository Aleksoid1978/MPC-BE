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

static void set_ref_pic(com_ref_pic_t * refp, com_pic_t  * pic_ref, s64 ptr)
{
    refp->pic      = pic_ref;
    refp->map_mv   = pic_ref->map_mv;
    refp->map_refi = pic_ref->map_refi;
    refp->dist     = (s16)(ptr - pic_ref->ptr);

    uavs3d_assert(refp->dist);

    if (refp->dist == 0) {
        refp->dist = 1;
    }
}

int com_picman_get_active_refp(com_frm_t *frm, com_pic_manager_t *pm)
{
    com_ref_pic_t(*refp)[REFP_NUM] = frm->refp;
    com_pic_header_t *pichdr = &frm->pichdr;
 
    for (int i = 0; i < MAX_REFS; i++) {
        refp[i][REFP_0].pic = refp[i][REFP_1].pic = NULL;
    }
    frm->num_refp[REFP_0] = frm->num_refp[REFP_1] = 0;

    // L0
    for (int i = 0; i < pichdr->rpl_l0.active; i++) {
        int refPicDoi = (pichdr->decode_order_index - pichdr->rpl_l0.delta_doi[i]);
        int j = 0;

        while (j < pm->cur_pb_size && (pm->list[j]->doi != refPicDoi || !pm->list[j]->is_ref)) j++;

        if (j < pm->cur_pb_size) {
            set_ref_pic(&refp[i][REFP_0], pm->list[j], pichdr->ptr);
            frm->num_refp[REFP_0]++;
            pm->list[j]->ref_cnt++;
        } else {
            return ERR_LOSS_REF_FRAME;  
        }
    }
    if (pichdr->slice_type == SLICE_P) {
        return RET_OK;
    }

    // L1
    for (int i = 0; i < pichdr->rpl_l1.active; i++) {
        int refPicDoi = (pichdr->decode_order_index - pichdr->rpl_l1.delta_doi[i]);
        int j = 0;

        while (j < pm->cur_pb_size && (pm->list[j]->doi != refPicDoi || !pm->list[j]->is_ref)) j++;

        if (j < pm->cur_pb_size) {
            set_ref_pic(&refp[i][REFP_1], pm->list[j], pichdr->ptr);
            frm->num_refp[REFP_1]++;
            pm->list[j]->ref_cnt++;
        } else {
            return ERR_LOSS_REF_FRAME;  
        }
    }

    return RET_OK; 
}

int com_picman_mark_refp(com_pic_manager_t *pm, com_pic_header_t *pichdr)
{
    for (int i = 0; i < pm->cur_pb_size; i++) {
        com_pic_t *pic = pm->list[i];

        if (pic && pic->is_ref) {
            int j;
       
            for (j = 0; j < pichdr->rpl_l0.num; j++) {
                if (pic->doi == (pichdr->decode_order_index - pichdr->rpl_l0.delta_doi[j])) {
                    break;
                }
            }
            if (j == pichdr->rpl_l0.num) {
                for (j = 0; j < pichdr->rpl_l1.num; j++) {
                    if (pic->doi == (pichdr->decode_order_index - pichdr->rpl_l1.delta_doi[j])) {
                        break;
                    }
                }
                if (j == pichdr->rpl_l1.num) {
                    pic->is_ref = 0;
                }
            }
        }
    }
    return RET_OK;
}

com_pic_t* com_picman_get_empty_pic(com_pic_manager_t * pm, int * err)
{
    com_pic_t * pic = NULL;
    int ret;

    for (int i = 0; i < pm->cur_pb_size; i++) {
        pic = pm->list[i];
        if (pic != NULL && !pic->is_ref && !pic->is_output && !pic->ref_cnt) {
            return pic;
        }
    }

    if (pm->cur_pb_size == pm->max_pb_size) {
        *err = ERR_PIC_BUFFER_FULL;
        return NULL;
    }

    pic = com_pic_alloc(&pm->pic_param, &ret);

    if (pic == NULL) {
        *err = ERR_OUT_OF_MEMORY;
        return NULL;
    }
    pm->list[pm->cur_pb_size++] = pic;

    return pic;
}

com_pic_t* com_picman_out_pic(com_pic_manager_t * pm, int * err, u8 cur_pic_doi, int state)
{
    int i, ret, found = 0;
    com_pic_t **list = pm->list;

    BOOL exist_pic = 0;
    s64 min_ptr = LLONG_MAX;
    int min_ptr_idx = 0;

    if (state != 1) {
        for (i = 0; i < pm->cur_pb_size; i++) {
            com_pic_t *pic = list[i];
            if (pic != NULL && pic->is_output) {
                found = 1;
                if ((pic->doi + pic->output_delay <= cur_pic_doi)) {
                    exist_pic = 1;
                    if (min_ptr >= pic->ptr) {
                        min_ptr = pic->ptr;
                        min_ptr_idx = i;
                    }
                }
            }
        }
        if (exist_pic) {
            list[min_ptr_idx]->is_output = 0;
            if (err) *err = RET_OK;
            return list[min_ptr_idx];
        }
    } else { // Bumping 
        for (i = 0; i < pm->cur_pb_size; i++) {
            com_pic_t *pic = list[i];
            if (pic != NULL && pic->is_output) {
                found = 1;

                if (pic->ptr <= min_ptr) {
                    exist_pic = 1;
                    min_ptr = pic->ptr;
                    min_ptr_idx = i;
                }
            }
        }
        if (exist_pic) {
            list[min_ptr_idx]->is_output = 0;
            if (err) *err = RET_OK;
            return list[min_ptr_idx];
        }
    }

    if (found == 0) {
        ret = ERR_UNEXPECTED;
    } else {
        ret = RET_DELAYED;
    }
    if (err) *err = ret;
    return NULL;
}

int com_picman_free(com_pic_manager_t * pm)
{
    for (int i = 0; i < pm->cur_pb_size; i++) {
        if (pm->list[i]) {
            com_pic_free(&pm->pic_param, pm->list[i]);
            pm->list[i] = NULL;
        }
    }
    com_free(pm->list);
    pm->list = NULL;

    return RET_OK;
}

int com_picman_init(com_pic_manager_t *pm, com_seqh_t *seqhdr, int ext_nodes)
{
    pm->pic_param.width     = seqhdr->pic_width;
    pm->pic_param.height    = seqhdr->pic_height;
    pm->pic_param.pad_l     = PIC_PAD_SIZE_L;
    pm->pic_param.pad_c     = PIC_PAD_SIZE_C;
    pm->pic_param.f_scu     = seqhdr->f_scu;
    pm->pic_param.i_scu     = seqhdr->i_scu;
    pm->pic_param.parallel  = ext_nodes > 0;
    pm->pic_param.bit_depth = seqhdr->bit_depth_internal;

    pm->cur_pb_size = 0;
    pm->doi_cycles  = 0;
    pm->prev_doi    = 0;

    pm->max_pb_size = MAX_PB_SIZE + ext_nodes;
    pm->list = com_malloc(sizeof(com_pic_t*) * pm->max_pb_size);

    if (pm->list) {
        return RET_OK;
    } else {
        return ERR_OUT_OF_MEMORY;
    }
}
