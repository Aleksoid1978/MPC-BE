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

#include <time.h>
#include "modules.h"
#include "dec_type.h"
#include "../../version.h"

#define PIC_ALIGN_SIZE 8

static void seq_free(uavs3d_dec_t * ctx)
{
    if (ctx->frm_threads_pool) {
        uavs3d_threadpool_delete(ctx->frm_threads_pool);
    }

    com_free(ctx->alf_idx_map);
    com_picman_free(&ctx->pic_manager);

    com_core_free(ctx->core);
    ctx->core = NULL;

    for (int i = 0; i < ctx->frm_nodes; i++) {
        com_free(ctx->frm_nodes_list[i].bs_buf);
    }
    com_free(ctx->frm_nodes_list);
}

static void update_seqhdr(uavs3d_dec_t * ctx, com_seqh_t * seqhdr)
{
    int size;

    /*** init extension data for com_seqh_t ***/

    seqhdr->bit_depth_internal = (seqhdr->encoding_precision == 2) ? 10 : 8;
    seqhdr->bit_depth_input = (seqhdr->sample_precision == 1) ? 8 : 10;
    seqhdr->bit_depth_2_qp_offset = (8 * (seqhdr->bit_depth_internal - 8));

    seqhdr->pic_width = ((seqhdr->horizontal_size + PIC_ALIGN_SIZE - 1) / PIC_ALIGN_SIZE) * PIC_ALIGN_SIZE;
    seqhdr->pic_height = ((seqhdr->vertical_size + PIC_ALIGN_SIZE - 1) / PIC_ALIGN_SIZE) * PIC_ALIGN_SIZE;

    seqhdr->max_cuwh = 1 << seqhdr->log2_max_cu_width_height;
    seqhdr->log2_max_cuwh = COM_LOG2(seqhdr->max_cuwh);

    size = seqhdr->max_cuwh;
    seqhdr->pic_width_in_lcu = (seqhdr->pic_width + (size - 1)) / size;
    seqhdr->pic_height_in_lcu = (seqhdr->pic_height + (size - 1)) / size;
    seqhdr->f_lcu = seqhdr->pic_width_in_lcu * seqhdr->pic_height_in_lcu;

    seqhdr->pic_width_in_scu = ((seqhdr->pic_width + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2);
    seqhdr->pic_height_in_scu = ((seqhdr->pic_height + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2);
    seqhdr->i_scu = ((seqhdr->pic_width + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2) + 2;
    seqhdr->f_scu = seqhdr->i_scu * (seqhdr->pic_height_in_scu + 2);
    seqhdr->a_scu = seqhdr->i_scu * (seqhdr->pic_height_in_scu);

    seqhdr->patch_width = seqhdr->pic_width_in_lcu;
    seqhdr->patch_height = COM_MIN(seqhdr->patch_height, seqhdr->pic_height_in_lcu);

    if (seqhdr->patch_stable) {
        if (seqhdr->patch_uniform) {
            seqhdr->patch_columns = seqhdr->pic_width_in_lcu / seqhdr->patch_width;

            for (int i = 0; i < seqhdr->patch_columns; i++) {
                seqhdr->patch_column_width[i] = seqhdr->patch_width;
            }
            if (seqhdr->pic_width_in_lcu % seqhdr->patch_width != 0) {
                if (seqhdr->patch_columns == 0) {
                    seqhdr->patch_column_width[0] = seqhdr->pic_width_in_lcu;
                    seqhdr->patch_columns = 1;
                } else {
                    seqhdr->patch_column_width[seqhdr->patch_columns] += seqhdr->pic_width_in_lcu - seqhdr->patch_width * seqhdr->patch_columns;
                    seqhdr->patch_columns += 1;
                }
            }
            seqhdr->patch_rows = seqhdr->pic_height_in_lcu / seqhdr->patch_height;

            for (int i = 0; i < seqhdr->patch_rows; i++) {
                seqhdr->patch_row_height[i] = seqhdr->patch_height;
            }
            if (seqhdr->pic_height_in_lcu % seqhdr->patch_height != 0) {
                if (seqhdr->patch_rows == 0) {
                    seqhdr->patch_row_height[0] = seqhdr->pic_height_in_lcu;
                    seqhdr->patch_rows = 1;
                } else {
                    seqhdr->patch_row_height[seqhdr->patch_rows] += seqhdr->pic_height_in_lcu - seqhdr->patch_height * seqhdr->patch_rows;
                    seqhdr->patch_rows += 1;
                }
            }
        }
    } else {
        uavs3d_assert(0);
    }

    memcpy(&ctx->seqhdr, seqhdr, sizeof(com_seqh_t));

    ctx->seqhdr.alf_idx_map = ctx->alf_idx_map;
}

static int seq_init(uavs3d_dec_t * ctx)
{
    int ret;
    com_seqh_t * seqhdr = &ctx->seqhdr;

    /*** init uavs3d_dec_t ***/

    if (ctx->init_flag) {
        seq_free(ctx);
    }
    ret = com_picman_init(&ctx->pic_manager, seqhdr, ctx->frm_threads_nodes);
    uavs3d_assert_goto(!ret, ERR);

    ctx->alf_idx_map = (u8*)com_malloc(sizeof(u8) * ctx->seqhdr.f_lcu);
    uavs3d_assert_goto(ctx->alf_idx_map, ERR);
    com_alf_init_map(&ctx->seqhdr, ctx->alf_idx_map);

    if (ctx->dec_cfg.frm_threads > 1) {
        ret = uavs3d_threadpool_init(&ctx->frm_threads_pool, ctx->dec_cfg.frm_threads, ctx->frm_threads_nodes, (void *(*)(void *))com_core_init, &ctx->seqhdr, (void(*)(void *))com_core_free);
        uavs3d_assert_goto(!ret, ERR);
    } else {
        ctx->core = com_core_init(&ctx->seqhdr);
        uavs3d_assert_goto(ctx->core, ERR);
    }
    ctx->frm_nodes = ctx->frm_threads_nodes + 1;
    ctx->frm_nodes_list = com_malloc(sizeof(com_frm_t) * ctx->frm_nodes);
    uavs3d_assert_goto(ctx->frm_nodes_list, ERR);

    ctx->frm_node_start = 0;
    ctx->frm_node_end = 0;
    ctx->init_flag = 1;
    ctx->output = 0;

    return RET_OK;

ERR:
    seq_free(ctx);
    ctx->init_flag = 0;
    
    return ERR_OUT_OF_MEMORY;
}

static int dec_cu(com_core_t * core, int x, int y, int cu_log2w, int cu_log2h)
{
    int ret;
    com_seqh_t *seqhdr = core->seqhdr;
    com_pic_t  *pic  = core->pic;
    com_pic_header_t *pichdr = core->pichdr;

    int i_scu     = seqhdr->i_scu;
    int bit_depth = seqhdr->bit_depth_internal;
    int cu_width  = 1 << cu_log2w;
    int cu_height = 1 << cu_log2h;
    
    core->cu_log2w  = cu_log2w;
    core->cu_log2h  = cu_log2h;
    core->cu_width  = cu_width;
    core->cu_height = cu_height;
    core->cu_pix_x  = x;
    core->cu_pix_y  = y;

//#define POS_X 584
//#define POS_Y 1020
//
//    if (x <= POS_X && x + cu_width >= POS_X && y <= POS_Y && y + cu_height >= POS_Y && pic->ptr == 6) {
//        int a = 0;
//    }

    /* parse CU info */
    if (core->tree_status != TREE_C) {
        ret = dec_parse_cu_header(core);
    } else {
        ret = dec_parse_cu_header_chroma(core);
    }
 
    uavs3d_assert_return(!ret, ret);

    if (core->tree_status != TREE_C) {
        dec_update_map(core);

        if (pichdr->loop_filter_disable_flag == 0) {
            com_deblock_set_edge(core);
        }
    }
    
    /* prediction */
    if (core->cu_mode != MODE_INTRA) {
        ALIGNED_32(pel pred_buf[MAX_CU_DIM + MAX_CU_DIM / 2]);

        uavs3d_assert(REFI_IS_VALID(core->refi[0]) || REFI_IS_VALID(core->refi[1]));

        if (core->refi[0] == REFI_INVALID && core->refi[1] == REFI_INVALID) {
            core->refi[0] = 0;
        }
        uavs3d_assert(core->refi[0] < core->num_refp[0] && core->refi[1] < core->num_refp[1]);
        core->refi[0] = COM_MIN(core->refi[0], core->num_refp[0] - 1);
        core->refi[1] = COM_MIN(core->refi[1], core->num_refp[1] - 1);

        if (core->affine_flag) {
            com_mc_affine(core, pred_buf);
        } else {
            com_mc(core, pred_buf);
        }
        /* reconstruction */
        if (core->tree_status != TREE_C && M32(core->cbfy)) {
            int i_rec = pic->stride_luma;
            pel* rec = pic->y + (y * i_rec) + x;

            int k, part_num = dec_get_part_num(core->tb_part);
            int tb_height, tb_width;

            dec_get_tb_width_height(cu_width, cu_height, core->tb_part, &tb_width, &tb_height);

            int log2_tb_w = g_tbl_log2[tb_width];
            int log2_tb_h = g_tbl_log2[tb_height];

            for (k = 0; k < part_num; k++) {
                int tb_x, tb_y;
                ALIGNED_32(s16 coef[MAX_TR_DIM]);
                ALIGNED_32(s16 resi[MAX_TR_DIM]);
                dec_get_tb_start_pos(cu_width, cu_height, core->tb_part, k, &tb_x, &tb_y);

                if (core->cbfy[k]) {
                    dec_parse_run_length_cc(core, coef, log2_tb_w, log2_tb_h, Y_C);
                    com_itrans(core, Y_C, k, coef, resi, log2_tb_w, log2_tb_h, bit_depth, 0, 0);
                }
                uavs3d_funs_handle.recon_luma[log2_tb_w - 2](resi, pred_buf + tb_y * cu_width + tb_x, cu_width, tb_width, tb_height, rec + tb_y * i_rec + tb_x, i_rec, core->cbfy[k], bit_depth);
            }
        }

        /* chroma */
        if (core->tree_status != TREE_L && M16(core->cbfc)) {
            int off = x + (y >> 1) * pic->stride_chroma;
            ALIGNED_32(s16 coef[MAX_TR_DIM]);
            ALIGNED_32(s16 resi_u[MAX_TR_DIM]);
            ALIGNED_32(s16 resi_v[MAX_TR_DIM]);

            if (core->cbfc[0]) {
                dec_parse_run_length_cc(core, coef, cu_log2w - 1, cu_log2h - 1, U_C);
                com_itrans(core, U_C, 0, coef, resi_u, cu_log2w - 1, cu_log2h - 1, bit_depth, 0, 0);
            }
            if (core->cbfc[1]) {
                dec_parse_run_length_cc(core, coef, cu_log2w - 1, cu_log2h - 1, V_C);
                com_itrans(core, V_C, 0, coef, resi_v, cu_log2w - 1, cu_log2h - 1, bit_depth, 0, 0);
            }
            uavs3d_funs_handle.recon_chroma[cu_log2w - 3](resi_u, resi_v, pred_buf + cu_width * cu_height, cu_width >> 1, cu_height >> 1, pic->uv + off, pic->stride_chroma, core->cbfc, bit_depth);
        }
    } else {
        //pred and recon for luma
        if (core->tree_status != TREE_C) {
            int i_rec = pic->stride_luma;
            if (core->ipm[0] == IPD_IPCM) {
                com_lbac_t *lbac = &core->lbac;
                pel* rec_cu = pic->y + (y * i_rec) + x;
                int tb_w = COM_MIN(32, cu_width);
                int tb_h = COM_MIN(32, cu_height);
                int shift = seqhdr->bit_depth_internal - seqhdr->bit_depth_input;
                int left = 8;
                int trm = lbac_dec_bin_trm(lbac); 
                uavs3d_assert(trm == 1);

                dec_parse_ipcm_start(lbac);
                
                for (int tb_y = 0; tb_y < cu_height; tb_y += tb_h) {
                    for (int tb_x = 0; tb_x < cu_width; tb_x += tb_w) {
                        pel* rec = rec_cu + tb_y * i_rec + tb_x;
                        for (int i = 0; i < tb_h; i++) {
                            for (int j = 0; j < tb_w; j++) {
                                rec[j] = dec_parse_ipcm(lbac, &left, seqhdr->bit_depth_input) << shift;
                            }
                            rec += i_rec;
                        }
                    }
                }
                uavs3d_assert(left == 8);
                if (core->tree_status == TREE_L || core->ipm_c != IPD_DM_C) {
                    lbac_init(lbac, lbac->cur, lbac->end);
                }
            } else {
                int tb_w, tb_h, tb_x, tb_y, tb_scup, log2_tb_w, log2_tb_h;
                int num_tb_in_pb = dec_get_part_num(core->tb_part);
                int sec_trans_enable = core->seqhdr->secondary_transform_enable_flag;

                dec_get_tb_width_height(cu_width, cu_height, core->tb_part, &tb_w, &tb_h);

                log2_tb_w = g_tbl_log2[tb_w];
                log2_tb_h = g_tbl_log2[tb_h];

                for (int tb_idx = 0; tb_idx < num_tb_in_pb; tb_idx++) {
                    int avail_tb;
                    ALIGNED_32(pel temp_buf[MAX_TR_DIM * 2]); // tmp buffer, may be used, or not. *2 for s16 
                    int intra_mode = core->ipm[dec_get_pb_idx_by_tb(core->pb_part, tb_idx)];
                    ALIGNED_32(pel neighbor_buf[MAX_CU_SIZE / 2 * 5]);
                    pel *src = neighbor_buf + MAX_CU_SIZE + 32;

                    dec_get_tb_start_pos(cu_width, cu_height, core->tb_part, tb_idx, &tb_x, &tb_y);
                    tb_x += x;
                    tb_y += y;
                    tb_scup = (tb_y >> MIN_CU_LOG2) * i_scu + (tb_x >> MIN_CU_LOG2);

                    avail_tb = dec_get_avail_intra(i_scu, tb_scup, core->map.map_scu);

                    /* prediction */
                    pel *rec = pic->y + (tb_y * i_rec) + tb_x;
                    pel *recT = (tb_y == core->lcu_pix_y) ? core->linebuf_intra[0] + tb_x : rec - i_rec;
                    com_get_nbr_l(src, intra_mode, core->ipf_flag, tb_x, tb_y, tb_w, tb_h, recT, rec - 1, i_rec, avail_tb, tb_scup, core->map.map_scu, i_scu, bit_depth);

                    if (core->cbfy[tb_idx]) {
                        ALIGNED_32(s16 resi[MAX_TR_DIM]);
                        ALIGNED_32(s16 coef[MAX_TR_DIM]);
                        int sec_trans = 0;

                        if (sec_trans_enable) {
                            if (log2_tb_w > 2 || log2_tb_h > 2) {
                                int block_available_up = IS_AVAIL(avail_tb, AVAIL_UP);
                                int block_available_left = IS_AVAIL(avail_tb, AVAIL_LE);
                                int vt = intra_mode < IPD_HOR;
                                int ht = (intra_mode > IPD_VER && intra_mode < IPD_CNT) || intra_mode <= IPD_BI;
                                vt = vt && block_available_up;
                                ht = ht && block_available_left;
                                sec_trans = (vt << 1) | ht;
                            }
                        }
                        dec_parse_run_length_cc(core, coef, log2_tb_w, log2_tb_h, Y_C);
                        com_itrans(core, Y_C, tb_idx, coef, resi, log2_tb_w, log2_tb_h, bit_depth, sec_trans, sec_trans_enable);
                        com_ipred_l(src, temp_buf, tb_w, temp_buf, intra_mode, tb_w, tb_h, bit_depth, avail_tb, core->ipf_flag);
                        uavs3d_funs_handle.recon_luma[log2_tb_w - 2](resi, temp_buf, tb_w, tb_w, tb_h, rec, i_rec, core->cbfy[tb_idx], bit_depth);
                    } else {
                        com_ipred_l(src, rec, i_rec, temp_buf, intra_mode, tb_w, tb_h, bit_depth, avail_tb, core->ipf_flag);
                    }
                }
            }
        }

        //pred and recon for uv
        if (core->tree_status != TREE_L) {
            int i_rec = pic->stride_chroma;
            pel *rec_cu = pic->uv + x + (y >> 1) * i_rec;

            if (core->ipm[0] == IPD_IPCM && core->ipm_c == IPD_DM_C) {
                int cu_width_c = cu_width >> 1;
                int cu_height_c = cu_height >> 1;
                int tb_w = COM_MIN(32, cu_width_c);
                int tb_h = COM_MIN(32, cu_height_c);
                int shift = seqhdr->bit_depth_internal - seqhdr->bit_depth_input;
                com_lbac_t *lbac = &core->lbac;
                int left = 8;

                uavs3d_assert(shift >= 0);
                if (core->tree_status == TREE_C) {
                    int trm = lbac_dec_bin_trm(lbac);
                    uavs3d_assert(trm == 1);
                    dec_parse_ipcm_start(lbac);
                }
                for (int tb_y = 0; tb_y < cu_height_c; tb_y += tb_h) {
                    for (int tb_x = 0; tb_x < cu_width_c; tb_x += tb_w) {
                        pel* rec = rec_cu + tb_y * i_rec + (tb_x << 1);
                        for (int i = 0; i < tb_h; i++) {
                            for (int j = 0; j < tb_w << 1; j += 2) {
                                rec[j] = dec_parse_ipcm(lbac, &left, seqhdr->bit_depth_input) << shift;
                            }
                            rec += i_rec;
                        }
                    }
                } 
                for (int tb_y = 0; tb_y < cu_height_c; tb_y += tb_h) {
                    for (int tb_x = 0; tb_x < cu_width_c; tb_x += tb_w) {
                        pel* rec = rec_cu + tb_y * i_rec + (tb_x << 1) + 1;
                        for (int i = 0; i < tb_h; i++) {
                            for (int j = 0; j < tb_w << 1; j+= 2) {
                                rec[j] = dec_parse_ipcm(lbac, &left, seqhdr->bit_depth_input) << shift;
                            }
                            rec += i_rec;
                        }
                    }
                }
                uavs3d_assert(left == 8);
                lbac_init(lbac, lbac->cur, lbac->end);
            } else {
                u16 avail_cu = dec_get_avail_intra(i_scu, core->cu_scup, core->map.map_scu);
                ALIGNED_32(s16 temp_buf[MAX_TR_DIM * 2]);
                ALIGNED_32(s16 resi_u[MAX_TR_DIM]);
                ALIGNED_32(s16 resi_v[MAX_TR_DIM]);
                int i_luma = pic->stride_luma;
                pel *rec_luma = pic->y + (y * i_luma) + x;
                com_scu_t *map_scu = core->map.map_scu;
                pel *recT = (y == core->lcu_pix_y) ? core->linebuf_intra[1] + x : rec_cu - i_rec;
                pel neighbor_buf_luma[MAX_CU_SIZE / 2 * 5];
                pel neighbor_buf[MAX_CU_SIZE / 2 * 2 * 5];
                pel *src = neighbor_buf + MAX_CU_SIZE / 2 * 5;
                pel *src_luma = neighbor_buf_luma + MAX_CU_SIZE + 32;

                /* prediction */
                if (IPD_TSCPM_C == core->ipm_c) {
                    pel *recT_luma = (y == core->lcu_pix_y) ? core->linebuf_intra[0] + x : rec_luma - i_luma;
                    com_get_nbr_l(src_luma, IPD_DC, 0, x, y, cu_width, cu_height, recT_luma, rec_luma - 1, i_luma, avail_cu, core->cu_scup, map_scu, i_scu, bit_depth);
                }
                com_get_nbr_c(src, core->ipm_c, core->ipm[PB0], x, y >> 1, cu_width >> 1, cu_height >> 1, recT, rec_cu - 2, i_rec, avail_cu, core->cu_scup, map_scu, i_scu, bit_depth);

                if (M16(core->cbfc)) {
                    if (core->cbfc[0]) {
                        dec_parse_run_length_cc(core, temp_buf, cu_log2w - 1, cu_log2h - 1, U_C);
                        com_itrans(core, U_C, 0, temp_buf, resi_u, cu_log2w - 1, cu_log2h - 1, bit_depth, 0, 0);
                    }
                    if (core->cbfc[1]) {
                        dec_parse_run_length_cc(core, temp_buf, cu_log2w - 1, cu_log2h - 1, V_C);
                        com_itrans(core, V_C, 0, temp_buf, resi_v, cu_log2w - 1, cu_log2h - 1, bit_depth, 0, 0);
                    }

                    com_ipred_c((pel*)temp_buf, cu_width, src, src_luma, temp_buf, core->ipm_c, core->ipm[PB0], cu_width >> 1, cu_height >> 1, bit_depth, avail_cu, rec_luma, i_luma);
                    uavs3d_funs_handle.recon_chroma[cu_log2w - 3](resi_u, resi_v, (pel*)temp_buf, cu_width >> 1, cu_height >> 1, pic->uv + x + (y >> 1) * pic->stride_chroma, pic->stride_chroma, core->cbfc, bit_depth);
                } else {
                    com_ipred_c(rec_cu, i_rec, src, src_luma, temp_buf, core->ipm_c, core->ipm[PB0], cu_width >> 1, cu_height >> 1, bit_depth, avail_cu, rec_luma, i_luma);
                }
            }
        }
    }
    return RET_OK;
}

static int decode_cu_tree(com_core_t * core, int x0, int y0, int cu_log2w, int cu_log2h, int qt_depth, int bet_depth, u8 cons_pred_mode, u8 tree_status)
{
    int ret;
    s8  split_mode = NO_SPLIT;
    int cu_width  = 1 << cu_log2w;
    int cu_height = 1 << cu_log2h;
    com_seqh_t *seqhdr = core->seqhdr;
    com_lbac_t *lbac = &core->lbac;

    core->tree_scup = (y0 >> MIN_CU_LOG2) * core->seqhdr->i_scu + (x0 >> MIN_CU_LOG2);

    if (cu_width > MIN_CU_SIZE || cu_height > MIN_CU_SIZE) {
        int split_tab = dec_get_split_available(seqhdr, x0, y0, cu_width, cu_height, qt_depth, bet_depth, core->slice_type);
        split_mode = dec_parse_split_mode(core, lbac, split_tab, cu_width, cu_height, qt_depth, bet_depth);
    }
    if (split_mode != NO_SPLIT) {
        dec_split_info_t split_struct;
        u8 tree_status_child = (tree_status == TREE_LC && dec_is_separate_tree(cu_width, cu_height, split_mode)) ? TREE_L : tree_status;
        dec_get_split_struct(split_mode, x0, y0, cu_width, cu_height, &split_struct);

        if (cons_pred_mode == NO_MODE_CONS && core->slice_type != SLICE_I && dec_cons_allow(cu_width, cu_height, split_mode)) {
            cons_pred_mode = dec_parse_cons_pred_mode_child(lbac);
        } 

#define INC_QT_DEPTH(qtd, smode)           (smode == SPLIT_QUAD? (qtd + 1) : qtd)
#define INC_BET_DEPTH(betd, smode)         (smode != SPLIT_QUAD? (betd + 1) : betd)

        qt_depth  = INC_QT_DEPTH (qt_depth,  split_mode);
        bet_depth = INC_BET_DEPTH(bet_depth, split_mode);

        for (int part_num = 0; part_num < split_struct.part_count; ++part_num) {
            int log2_sub_cuw = split_struct.log_cuw[part_num];
            int log2_sub_cuh = split_struct.log_cuh[part_num];
            int x_pos        = split_struct.x_pos  [part_num];
            int y_pos        = split_struct.y_pos  [part_num];

            if (x_pos < seqhdr->pic_width && y_pos < seqhdr->pic_height) {
                ret = decode_cu_tree(core, x_pos, y_pos, log2_sub_cuw, log2_sub_cuh, qt_depth, bet_depth, cons_pred_mode, tree_status_child);
                uavs3d_assert_return(!ret, ret);
            }
        }
        if (tree_status_child == TREE_L && tree_status == TREE_LC) {
            core->tree_status = TREE_C;
            core->cu_scup = (y0 >> MIN_CU_LOG2) * core->seqhdr->i_scu + (x0 >> MIN_CU_LOG2);
            ret = dec_cu(core, x0, y0, cu_log2w, cu_log2h);
            uavs3d_assert_return(!ret, ret);
        }
    } else {
        core->tree_status = tree_status;
        core->cons_pred_mode = cons_pred_mode;
        core->cu_scup = core->tree_scup;
        ret = dec_cu(core, x0, y0, cu_log2w, cu_log2h);
        uavs3d_assert_return(!ret, ret);

        if (seqhdr->num_of_hmvp_cand && core->cu_mode != MODE_INTRA && !core->affine_flag) {
            update_hmvp_candidates(core);
        }
    }
    return RET_OK;
}


void uavs3d_always_inline dec_set_patch_idx(com_core_t* core, int idx, int lcu_x, int lcu_y, int w_in_lcu, int h_in_lcu)
{
    s8 *map_patch = core->map.map_patch + lcu_y * core->seqhdr->pic_width_in_lcu + lcu_x;

    for (int i = 0; i < h_in_lcu; i++) {
        memset(map_patch, idx, w_in_lcu * sizeof(u8));
        map_patch += core->seqhdr->pic_width_in_lcu;
    }
}

void dec_all_loopfilter(com_core_t *core, int lcu_y)
{
    com_seqh_t *seqhdr = core->seqhdr;
    com_pic_header_t *pichdr = core->pichdr;
    com_pic_t  *pic  = core->pic;

    int pix_y = lcu_y * seqhdr->max_cuwh;
    int finished_line_num;

    if (lcu_y < seqhdr->pic_height_in_lcu - 1) {
        memcpy(core->linebuf_intra[0], pic->y  +  (pix_y + seqhdr->max_cuwh      - 1) * pic->stride_luma,   seqhdr->pic_width * sizeof(pel));
        memcpy(core->linebuf_intra[1], pic->uv + ((pix_y + seqhdr->max_cuwh) / 2 - 1) * pic->stride_chroma, seqhdr->pic_width * sizeof(pel));
        finished_line_num = pix_y + seqhdr->max_cuwh - 8;
    } else {
        finished_line_num = seqhdr->pic_height + pic->padsize_luma;
    }
#if 1
    if (pichdr->loop_filter_disable_flag == 0) { /* deblocking filter */
        com_deblock_lcu_row(core, lcu_y);
    }

    if (seqhdr->sample_adaptive_offset_enable_flag) { /* sao filter */
        com_sao_lcu_row(core, lcu_y);
    }

    if (seqhdr->adaptive_leveling_filter_enable_flag && pichdr->alf_param.pic_alf_enable) { /* alf filter */
        com_alf_lcu_row(core, lcu_y);
    }
#endif

    int pad_start = pix_y ? pix_y - 8 : 0;
    int pad_rows = finished_line_num - pad_start;
    int pad  = pic->padsize_luma;
    int padc = pic->padsize_chroma;

    uavs3d_funs_handle.padding_rows_luma(pic->y, pic->stride_luma, pic->width_luma, pic->height_luma, pad_start, pad_rows, pad, pad);
    uavs3d_funs_handle.padding_rows_chroma(pic->uv, pic->stride_chroma, pic->width_chroma << 1, pic->height_chroma, pad_start >> 1, pad_rows >> 1, padc << 1, padc);

    if (pic->parallel_enable) {
        uavs3d_pthread_mutex_lock(&pic->mutex);
        pic->finished_line = finished_line_num;
        uavs3d_pthread_cond_broadcast(&pic->cond);
        uavs3d_pthread_mutex_unlock(&pic->mutex);
    } else {
        pic->finished_line = finished_line_num;
    }
}

void init_core_by_frm(com_core_t *core, com_frm_t *frm)
{
    core->pic = frm->pic;
    core->bs.cur = frm->bs_buf;
    core->bs.end = frm->bs_buf + frm->bs_length;
    core->bs.leftbits = 0;
    core->bs.code = 0;

    memcpy(core->refp, frm->refp, sizeof(frm->refp)); 
    memcpy(core->num_refp, frm->num_refp, sizeof(frm->num_refp));

    core->pichdr = &frm->pichdr;
}

com_pic_t* dec_pic(com_core_t *core, com_frm_t *frm)
{
    core->seqhdr = &frm->seqhdr;

    init_core_by_frm(core, frm);

    com_seqh_t *seqhdr =  core->seqhdr;
    com_pic_t  *pic  =  core->pic;
    com_pic_header_t *pichdr =  core->pichdr;
    com_patch_header_t *sh   = &core->pathdr;
    com_lbac_t *lbac = &core->lbac;
    com_bs_t  *bs   = &core->bs;
    int ret;
    int lcu_qp;
    int lcu_qp_delta;
    int new_patch = 1;
    int left_lcu_num = seqhdr->f_lcu;
    int patch_start_x, patch_end_x, patch_start_y, patch_end_y;

    core->slice_type   = pichdr->slice_type;
    core->map.map_refi = pic->map_refi;
    core->map.map_mv   = pic->map_mv;

    if (pichdr->loop_filter_disable_flag == 0) {
        memset(core->map.map_edge, 0, seqhdr->a_scu * sizeof(u8));
    }

    while (1) {
        int lcu_idx;
        int adj_qp_cb, adj_qp_cr;

        if (new_patch) {
            u8 *end, *next;
            int patch_x, patch_y, patch_idx;

            end = dec_bs_get_one_unit(bs, &next);

            patch_idx = dec_parse_patch_header(bs, seqhdr, pichdr, sh);
 
            if (patch_idx == -1 || patch_idx >= seqhdr->patch_columns * seqhdr->patch_rows) {
                break;
            }
            /* initialize pathdr */
            if (patch_idx == 0 || pichdr->loop_filter_disable_flag || !seqhdr->cross_patch_loop_filter) {
                memset(core->map.map_scu, 0, sizeof(com_scu_t) * seqhdr->a_scu);
            } else {
                uavs3d_funs_handle.reset_map_scu(core->map.map_scu, seqhdr->a_scu);
            }

            /* init SBAC */
            lbac_init(lbac, bs->cur, end);
            bs->cur = next;

            com_lbac_ctx_init(&(lbac->ctx));

            /*initial pathdr info*/
            patch_x = patch_idx % seqhdr->patch_columns;
            patch_y = patch_idx / seqhdr->patch_columns;
            patch_start_x = patch_start_y = 0;

            /*set location at above-left of pathdr*/
            for (int i = 0; i < patch_x; i++) {
                patch_start_x += seqhdr->patch_column_width[i];
            }
            for (int i = 0; i < patch_y; i++) {
                patch_start_y += seqhdr->patch_row_height[i];
            }

            core->lcu_x = patch_start_x;
            core->lcu_y = patch_start_y;
            patch_end_x = patch_start_x + seqhdr->patch_column_width[patch_x];
            patch_end_y = patch_start_y + seqhdr->patch_row_height  [patch_y];
            dec_set_patch_idx(core, patch_idx, patch_start_x, patch_start_y, seqhdr->patch_column_width[patch_x], seqhdr->patch_row_height[patch_y]);

            lcu_qp = core->pathdr.slice_qp;
            lcu_qp_delta = 0;
        } else {
            core->lcu_x++;

            if (core->lcu_x >= patch_end_x) {
                core->lcu_x = patch_start_x;
                core->lcu_y++;

                if (core->lcu_y >= patch_end_y) {
                    uavs3d_assert(0);
                    break;
                }
            }
        }

        core->lcu_pix_x = core->lcu_x << seqhdr->log2_max_cuwh;
        core->lcu_pix_y = core->lcu_y << seqhdr->log2_max_cuwh;

        lcu_idx = core->lcu_y * seqhdr->pic_width_in_lcu + core->lcu_x;

        if (lcu_idx >= seqhdr->f_lcu) {
            uavs3d_assert(0);
            break;
        }

        if (core->lcu_x == patch_start_x) {
            core->hmvp_cands_cnt = 0;
        }
        if (core->pathdr.fixed_slice_qp_flag) {
            lcu_qp = core->pathdr.slice_qp;
        } else {
            lcu_qp += (lcu_qp_delta = dec_parse_lcu_delta_qp(lbac, lcu_qp_delta));
        }
        lcu_qp = COM_CLIP3(0, MAX_QUANT_BASE + seqhdr->bit_depth_2_qp_offset, lcu_qp);

        adj_qp_cb = lcu_qp + pichdr->chroma_quant_param_delta_cb - seqhdr->bit_depth_2_qp_offset;
        adj_qp_cr = lcu_qp + pichdr->chroma_quant_param_delta_cr - seqhdr->bit_depth_2_qp_offset;

        if (adj_qp_cb >= 0) {
            adj_qp_cb = g_tbl_qp_chroma_adjust[COM_MIN(MAX_QUANT_BASE, adj_qp_cb)];
        }
        if (adj_qp_cr >= 0) {
            adj_qp_cr = g_tbl_qp_chroma_adjust[COM_MIN(MAX_QUANT_BASE, adj_qp_cr)];
        }

        core->lcu_qp_y = lcu_qp;
        core->lcu_qp_u = COM_MAX(0, adj_qp_cb + seqhdr->bit_depth_2_qp_offset);
        core->lcu_qp_v = COM_MAX(0, adj_qp_cr + seqhdr->bit_depth_2_qp_offset);

        core->map.map_qp[lcu_idx] = lcu_qp;

        if (seqhdr->sample_adaptive_offset_enable_flag) {
            dec_parse_sao_param(core, lcu_idx, core->sao_param_map[lcu_idx]);
        }
        if (seqhdr->adaptive_leveling_filter_enable_flag) {
            for (int compIdx = 0; compIdx < N_C; compIdx++) {
                if (pichdr->alf_param.alf_flag[compIdx]) {
                    core->alf_enable_map[lcu_idx][compIdx] = dec_parse_alf_enable(lbac, compIdx);
                } else {
                    core->alf_enable_map[lcu_idx][compIdx] = FALSE;
                }
            }
        }
        ret = decode_cu_tree(core, core->lcu_pix_x, core->lcu_pix_y, seqhdr->log2_max_cuwh, seqhdr->log2_max_cuwh, 0, 0, NO_MODE_CONS, TREE_LC);
       
        if (ret) {
            break;
        }

        /* read end_of_picture_flag */
        new_patch = lbac_dec_bin_trm(lbac);

        if (new_patch) {
            dec_parse_patch_end(bs);

            while (dec_bs_next(bs, 24) != 0x1 && bs->cur <= bs->end) {
                dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
            }
        }

        if (core->lcu_x == seqhdr->pic_width_in_lcu - 1) {
            dec_all_loopfilter(core, core->lcu_y);
        }
 
        if ((--left_lcu_num) <= 0) {
            uavs3d_assert(new_patch == 1);
            break;
        }
    }

    if (pic->parallel_enable) {
        uavs3d_pthread_mutex_lock(&pic->mutex);
        pic->finished_line = seqhdr->pic_height + pic->padsize_luma;
        uavs3d_pthread_cond_broadcast(&pic->cond);
        uavs3d_pthread_mutex_unlock(&pic->mutex);
    } else {
        pic->finished_line = seqhdr->pic_height + pic->padsize_luma;
    }

    return core->pic;
}

int init_dec_frm(uavs3d_dec_t *ctx, com_frm_t *frm, uavs3d_io_frm_t *frm_io)
{
    int ret;
    com_pic_header_t *pichdr = &frm->pichdr;
    com_pic_t *pic = com_picman_get_empty_pic(&ctx->pic_manager, &ret);
    com_bs_t *bs = &ctx->bs;

    if (pic == NULL) {
        return ret;
    }

    frm->pic = pic;

    pic->pts = frm_io->pts;
    pic->dts = frm_io->dts;
    pic->ptr = pichdr->ptr;
    pic->dtr = frm->pichdr.decode_order_index + (DOI_CYCLE_LENGTH * (int)ctx->pic_manager.doi_cycles);
    pic->doi = pichdr->decode_order_index;
    pic->type = pichdr->slice_type;

    pic->pkt_pos  = frm_io->pkt_pos;
    pic->pkt_size = frm_io->pkt_size;

    for (int i = 0; i < frm->num_refp[REFP_0]; i++) {
        pic->refpic[REFP_0][i] = frm->refp[i][REFP_0].pic->ptr;
    }
    for (int i = 0; i < frm->num_refp[REFP_1]; i++) {
        pic->refpic[REFP_1][i] = frm->refp[i][REFP_1].pic->ptr;
    }
    pic->refpic_num[REFP_0] = frm->num_refp[REFP_0];
    pic->refpic_num[REFP_1] = frm->num_refp[REFP_1];


    for (int i = 0; i < frm->num_refp[REFP_0]; i++) {
        pic->list_dist[i] = frm->refp[i][REFP_0].dist;
    }
    pic->temporal_id = pichdr->temporal_id;
    pic->output_delay = pichdr->output_delay;
    pic->is_ref = 1;
    pic->is_output = 1;
    pic->finished_line = -pic->padsize_luma;

    if (frm->bs_buf_size < bs->end - bs->cur + 1) {
        com_free(frm->bs_buf);
        frm->bs_buf_size = COM_MAX((int)(bs->end - bs->cur + 1) * 2, 1024);
        frm->bs_buf = com_malloc(frm->bs_buf_size);
        uavs3d_assert_return(frm->bs_buf, ERR_OUT_OF_MEMORY);
    }
    frm->bs_length = (int)(bs->end - bs->cur + 1);
    memcpy(frm->bs_buf, bs->cur, frm->bs_length);
    frm->bs_buf[frm->bs_length] = 0xFF;

    return RET_OK;
}

void clean_ref_cnt(com_frm_t *frm)
{
    for (int i = 0; i < frm->num_refp[REFP_0]; i++) {
        frm->refp[i][REFP_0].pic->ref_cnt--;
    }
    for (int i = 0; i < frm->num_refp[REFP_1]; i++) {
        frm->refp[i][REFP_1].pic->ref_cnt--;
    }
}

int uavs3d_output_frame(void *id, uavs3d_io_frm_t *frm, int flush, uavs3d_lib_output_callback_t callback) {
    int ret = 0;
    uavs3d_dec_t *ctx = id;

    if(frm) frm->got_pic = 0;

    if (ctx->output > 0) {
        com_pic_t *pic = com_picman_out_pic(&ctx->pic_manager, &ret, ctx->cur_decoded_doi, flush);

        if (pic && frm) {
            frm->num_plane = 2;
            frm->bit_depth = pic->bit_depth;

            frm->buffer[0] = pic->y;
            frm->stride[0] = pic->stride_luma * sizeof(pel);
            frm->width [0] = pic->width_luma;
            frm->height[0] = pic->height_luma;

            frm->buffer[1] = pic->uv;
            frm->stride[1] = pic->stride_chroma * sizeof(pel);
            frm->width [1] = pic->width_chroma;
            frm->height[1] = pic->height_chroma;

            frm->ptr  = pic->ptr;
            frm->pts  = pic->pts;
            frm->dtr  = pic->dtr;
            frm->dts  = pic->dts;
            frm->type = pic->type;

            frm->pkt_pos  = pic->pkt_pos;
            frm->pkt_size = pic->pkt_size;

            frm->refpic_num[0] = pic->refpic_num[0];
            frm->refpic_num[1] = pic->refpic_num[1];
            memcpy(frm->refpic, pic->refpic, 2 * 16 * sizeof(long long));

            frm->seqhdr = &ctx->seqhdr;
            frm->got_pic = 1;
            ctx->output--;

            if (callback) {
                callback(frm);
            }
        }
    } else {
        ret = -1;
    }

    return ret;
}

int __cdecl uavs3d_decode(void *h, uavs3d_io_frm_t* frm_io)
{
    uavs3d_dec_t *ctx = h;
    com_bs_t  *bs = &ctx->bs;
    int nal_type;
    int ret;

    frm_io->nal_type = 0;
    frm_io->got_pic = 0;

    dec_bs_init(bs, frm_io->bs, frm_io->bs_len);

    if (bs->cur[3] == 0xB0) {
        com_seqh_t seqhdr = { 0 };
        nal_type = NAL_SEQ_HEADER;

        bs->end = dec_bs_demulate(bs->cur + 3, bs->end);

        ret = dec_parse_sqh(bs, &seqhdr);

        if (ret) {
            return ret;
        }

        dec_parse_ext_and_usr_data(bs, &seqhdr, NULL, 0, -1);

        if (seqhdr.horizontal_size          != ctx->seqhdr.horizontal_size          || 
            seqhdr.vertical_size            != ctx->seqhdr.vertical_size            ||
            seqhdr.log2_max_cu_width_height != ctx->seqhdr.log2_max_cu_width_height ||
            seqhdr.encoding_precision       != ctx->seqhdr.encoding_precision) 
        {
            if (ctx->seqhdr.horizontal_size) {
                return ERR_SEQ_MAININFO_CHANGED;
            }
            
            update_seqhdr(ctx, &seqhdr);
            ret = seq_init(ctx);
            uavs3d_assert_return(!ret, ret);
        } else {
            update_seqhdr(ctx, &seqhdr);
        }
        frm_io->seqhdr = &ctx->seqhdr;
    }
    else if (bs->cur[3] == 0xB1) {
        nal_type = NAL_SEQ_END;
        ret = RET_OK;
    }
    else if (bs->cur[3] == 0xB3 || bs->cur[3] == 0xB6) {
        com_frm_t *frm = &ctx->frm_nodes_list[ctx->frm_node_end];
        com_pic_header_t *pichdr = &frm->pichdr;
        nal_type = NAL_PIC_HEADER;

        if (!ctx->init_flag) {
            return ERR_SEQ_HEADER_UNREADY;
        }

        bs->end = dec_bs_demulate(bs->cur + 3, bs->end);

        /* decode picture header */
        ret = dec_parse_pic_header(bs, pichdr, &ctx->seqhdr, &ctx->pic_manager);
        dec_parse_ext_and_usr_data(bs, &ctx->seqhdr, pichdr, 1, pichdr->slice_type);

        // init rpl list 
        ret = com_picman_mark_refp(&ctx->pic_manager, pichdr);
        uavs3d_assert_return(ret == RET_OK, ret);

        /* reference picture lists construction */
        ret = com_picman_get_active_refp(frm, &ctx->pic_manager);
        frm->pic_header_inited = (ret == 0 ? 1 : 0);
    } else if (bs->cur[3] >= 0x00 && bs->cur[3] <= 0x8E) {
        com_pic_t *pic;
        com_frm_t *frm = &ctx->frm_nodes_list[ctx->frm_node_end];
        int flush_flag = 0;

        if (frm_io->bs_len <= 0) {
            flush_flag = 1;
        }

        if (!ctx->init_flag) {
            return ERR_SEQ_HEADER_UNREADY;
        }
        if (!frm->pic_header_inited) {
            return ERR_PIC_HEADER_UNREADY;
        }

        nal_type = NAL_SLICE;

        ret = init_dec_frm(ctx, frm, frm_io);

        if (ret) {
            if (ret == ERR_PIC_BUFFER_FULL) {
                uavs3d_reset(h);
            }
            return ret;
        }

        ctx->frm_node_end = (ctx->frm_node_end + 1) % ctx->frm_nodes;
        ctx->frm_nodes_list[ctx->frm_node_end].pic_header_inited = 0;

        if (ctx->dec_cfg.frm_threads == 1) {
            memcpy(&frm->seqhdr, &ctx->seqhdr, sizeof(com_seqh_t));
            pic = dec_pic(ctx->core, frm);
            uavs3d_assert_return(!ret, ret);

            ctx->frm_node_start = (ctx->frm_node_start + 1) % ctx->frm_nodes;
            clean_ref_cnt(frm);

            if (ctx->dec_cfg.check_md5 && frm->pichdr.pic_md5_exist) {
                dec_check_pic_md5(pic, frm->pichdr.pic_md5);
            }
            ctx->cur_decoded_doi = pic->doi;
            ctx->output++;
            uavs3d_output_frame(ctx, frm_io, flush_flag, ctx->callback);
        } else {
            memcpy(&frm->seqhdr, &ctx->seqhdr, sizeof(com_seqh_t));

            if (uavs3d_threadpool_run_try(ctx->frm_threads_pool, (void *(*)(void *, void *))dec_pic, frm, 1) < 0) {
                int got_pic = 0;

                do {
                    com_frm_t *frm = &ctx->frm_nodes_list[ctx->frm_node_start];

                    if (got_pic == 0) {
                        pic = uavs3d_threadpool_wait(ctx->frm_threads_pool, frm);
                    } else {
                        pic = uavs3d_threadpool_wait_try(ctx->frm_threads_pool, frm);
                    }
                    if (pic) {
                        ctx->frm_node_start = (ctx->frm_node_start + 1) % ctx->frm_nodes;
                        clean_ref_cnt(frm);

                        if (ctx->dec_cfg.check_md5 && frm->pichdr.pic_md5_exist) {
                            dec_check_pic_md5(pic, frm->pichdr.pic_md5);
                        }
                        ctx->cur_decoded_doi = pic->doi;
                        got_pic = 1;
                        ctx->output++;
                    }
                } while (pic);

                uavs3d_threadpool_run(ctx->frm_threads_pool, (void *(*)(void *, void *))dec_pic, frm, 1);
            }
            uavs3d_output_frame(ctx, frm_io, flush_flag, ctx->callback);
        }
    } else {
        return ERR_UNKNOWN_NAL;
    }

    frm_io->nal_type = nal_type;

    return ret;
}

int __cdecl uavs3d_flush(void *h, uavs3d_io_frm_t* frm_out)
{
    uavs3d_dec_t *ctx = h;
    int ret = 0;

    if (ctx) {
        while (ctx->frm_node_start != ctx->frm_node_end) {
            com_frm_t *frm = &ctx->frm_nodes_list[ctx->frm_node_start];
            com_pic_t *pic = uavs3d_threadpool_wait(ctx->frm_threads_pool, frm);

            if (pic) {
                ctx->frm_node_start = (ctx->frm_node_start + 1) % ctx->frm_nodes;

                clean_ref_cnt(frm);

                if (ctx->dec_cfg.check_md5 && frm->pichdr.pic_md5_exist) {
                    dec_check_pic_md5(pic, frm->pichdr.pic_md5);
                }
                ctx->output++;
            }
        }
    }
    
    ret = uavs3d_output_frame(ctx, frm_out, 1, ctx->callback);

    return ret;
}

void* __cdecl uavs3d_create(uavs3d_cfg_t * dec_cfg, uavs3d_lib_output_callback_t callback, int * err)
{
    uavs3d_dec_t *ctx;

    printf("libuavs3d(%2d): %s_%s, %s\n", BIT_DEPTH, VERSION_STR, VERSION_TYPE, VERSION_SHA1);

#if CHECK_RAND_STRM
    srand((int)time(0));
#endif

    ctx = (uavs3d_dec_t*)com_malloc(sizeof(uavs3d_dec_t));

    if (ctx == NULL) {
        if (err) *err = ERR_OUT_OF_MEMORY;
        return NULL;
    }

    memcpy(&ctx->dec_cfg, dec_cfg, sizeof(uavs3d_cfg_t));

    ctx->init_flag = 0;
    com_dct_coef_init();

#if defined(ENABLE_FUNCTION_C)
    uavs3d_funs_init_c();
#endif 

#if defined(ENABLE_FUNCTION_ARM64)
    uavs3d_funs_init_arm64();
#elif defined(ENABLE_FUNCTION_ARM32)
    uavs3d_funs_init_armv7();
#elif defined(ENABLE_FUNCTION_X86)
    uavs3d_funs_init_sse();
    if (uavs3d_simd_avx_level(NULL) >= 2) {
        uavs3d_funs_init_avx2();
    }
#endif

    ctx->dec_cfg.frm_threads = COM_CLIP3(1, 32, ctx->dec_cfg.frm_threads);

#define ADDED_FRM_NODES 3

    if (ctx->dec_cfg.frm_threads > 1) {
        ctx->frm_threads_nodes = ctx->dec_cfg.frm_threads - 1 + ADDED_FRM_NODES;
    } else {
        ctx->frm_threads_nodes = 0;
    }
    ctx->frm_threads_pool = NULL;
    ctx->callback = callback;

    return ctx;
}

void __cdecl uavs3d_reset(void *h)
{
    uavs3d_dec_t *ctx = h;

    if (ctx) {
        int ret;

        do {
            ret = uavs3d_flush(h, NULL);
        } while (ret >= 0);

        com_pic_manager_t *pm = &ctx->pic_manager;
        ctx->frm_node_start = 0;
        ctx->frm_node_end = 0;

        for (int i = 0; i < pm->cur_pb_size; i++) {
            if (pm->list[i]) {
                pm->list[i]->is_ref = 0;
                pm->list[i]->is_output = 0;
                pm->list[i]->ref_cnt = 0;
            }
        }
        pm->doi_cycles  = 0;
        pm->prev_doi    = 0;

        ctx->output = 0;
    }
}


void __cdecl uavs3d_delete(void *h)
{
    uavs3d_dec_t *ctx = (uavs3d_dec_t *)h;
    seq_free(ctx);
    com_free(ctx);
}
