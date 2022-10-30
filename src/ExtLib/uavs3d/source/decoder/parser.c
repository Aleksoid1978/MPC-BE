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

/* ============  VLC ============== */

static int dec_parse_rlp(com_bs_t * bs, com_rpl_t * rpl, int max_dpb_size)
{
    rpl->num = (u32)dec_bs_read_ue(bs, 0, max_dpb_size);

    if (rpl->num > 0) { 
        rpl->delta_doi[0] = (u32)dec_bs_read_ue(bs, 1, 255);  
        rpl->delta_doi[0] *= 1 - ((u32)dec_bs_read1(bs, -1) << 1);
    }
    for (int i = 1; i < rpl->num; ++i) {
        int deltaRefPic = (u32)dec_bs_read_ue(bs, 0, 255);
        if (deltaRefPic != 0) {
            deltaRefPic *= 1 - ((u32)dec_bs_read1(bs, -1) << 1);
        }
        rpl->delta_doi[i] = rpl->delta_doi[i - 1] + deltaRefPic;
    }
    return RET_OK;
}

static void read_wq_matrix(com_bs_t *bs, u8 *m4x4, u8 *m8x8)
{
    int i;
    for (i = 0; i < 16; i++) {
        m4x4[i] = dec_bs_read_ue(bs, 1, 255);
    }
    for (i = 0; i < 64; i++) {
        m8x8[i] = dec_bs_read_ue(bs, 1, 255);
    }
}

int dec_parse_sqh(com_bs_t * bs, com_seqh_t * seqhdr)
{
    unsigned int ret = dec_bs_read(bs, 24, 1, 1);             
    unsigned int start_code = dec_bs_read(bs, 8, 0xB0, 0xB0);  
                                                        
    seqhdr->profile_id = (u8)dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
    com_check_val2(seqhdr->profile_id, 0x22, 0x20);

    seqhdr->level_id   = (u8)dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
    seqhdr->progressive_sequence = (u8)dec_bs_read1(bs,  1);        
    seqhdr->field_coded_sequence = (u8)dec_bs_read1(bs, -1);        
                                                                
    int library_stream_flag = dec_bs_read1(bs, 0);                
    int library_picture_enable_flag = dec_bs_read1(bs, 0);        
                                                                
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->horizontal_size  = dec_bs_read(bs, 14, 0, COM_UINT32_MAX);
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->vertical_size = dec_bs_read(bs, 14, 0, COM_UINT32_MAX);
                                                                
    seqhdr->display_horizontal_size = seqhdr->horizontal_size;
    seqhdr->display_vertical_size   = seqhdr->vertical_size;
																
    seqhdr->chroma_format    = (u8)dec_bs_read(bs, 2, 1, 1);      
    seqhdr->sample_precision = (u8)dec_bs_read(bs, 3, 1, 2);     
                                                                
    if (seqhdr->profile_id == 0x22) {                           
        seqhdr->encoding_precision = (u8)dec_bs_read(bs, 3, 1, 2); 
    } else {                                                    
        seqhdr->encoding_precision = 1;                         
    }                                                           
                                                                
#if (BIT_DEPTH == 8)                                            
    if (seqhdr->encoding_precision == 2) {                      
        return ERR_BAD_BITDEPTH;                            
    }                                                           
#endif                                                          
                                                                
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->aspect_ratio    = (u8)dec_bs_read(bs, 4, 1, 4);        
    seqhdr->frame_rate_code = (u8)dec_bs_read(bs, 4, 1, 13);       
                                                                
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->bit_rate_lower  = dec_bs_read(bs, 18, 0, COM_UINT32_MAX);
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->bit_rate_upper = dec_bs_read(bs, 12, 0, COM_UINT32_MAX);
                                                                
    seqhdr->low_delay               = (u8)dec_bs_read1(bs, -1);             
    seqhdr->temporal_id_enable_flag = (u8)dec_bs_read1(bs, -1);     
    dec_bs_read1(bs, 1); //marker_bit

    seqhdr->bbv_buffer_size = dec_bs_read(bs, 18, 0, COM_UINT32_MAX);
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->max_dpb_size = dec_bs_read(bs, 4, 0, 15) + 1;

    seqhdr->rpl1_index_exist_flag  = (u32)dec_bs_read1(bs, -1);
    seqhdr->rpl1_same_as_rpl0_flag = (u32)dec_bs_read1(bs, -1);
    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->rpls_l0_num = (u32)dec_bs_read_ue(bs, 0, 64);
    
    for (int i = 0; i < seqhdr->rpls_l0_num; ++i) {
        dec_parse_rlp(bs, &seqhdr->rpls_l0[i], seqhdr->max_dpb_size);
    }

    if (!seqhdr->rpl1_same_as_rpl0_flag) {
        seqhdr->rpls_l1_num = (u32)dec_bs_read_ue(bs, 0, 64);
        for (int i = 0; i < seqhdr->rpls_l1_num; ++i) {
            dec_parse_rlp(bs, &seqhdr->rpls_l1[i], seqhdr->max_dpb_size);
        }
    } else {
        seqhdr->rpls_l1_num = seqhdr->rpls_l0_num;
        for (int i = 0; i < seqhdr->rpls_l1_num; i++) {
            seqhdr->rpls_l1[i].num = seqhdr->rpls_l0[i].num;
            for (int j = 0; j < seqhdr->rpls_l1[i].num; j++) {
                seqhdr->rpls_l1[i].delta_doi[j] = seqhdr->rpls_l0[i].delta_doi[j];
            }
        }
    }
    seqhdr->num_ref_default_active_minus1[0] = (u32)dec_bs_read_ue(bs, 0, 14); 
    seqhdr->num_ref_default_active_minus1[1] = (u32)dec_bs_read_ue(bs, 0, 14); 

    seqhdr->log2_max_cu_width_height = (u8)(dec_bs_read(bs, 3, 3, 5) + 2);          
    seqhdr->min_cu_size              = (u8)(1 << (dec_bs_read(bs, 2, 0, 0) + 2));   

    seqhdr->max_part_ratio_log2 = (u8)(dec_bs_read(bs, 2, 0, 3) + 2 );              
    seqhdr->max_split_times     = (u8)(dec_bs_read(bs, 3, 0, 7) + 6 );              
    seqhdr->min_qt_size         = (u8)(1 << ((dec_bs_read(bs, 3, 0, 5) + 2)));      
    seqhdr->max_bt_size         = (u8)(1 << ((dec_bs_read(bs, 3, 0, 5) + 2)));      
    seqhdr->max_eqt_size        = (u8)(1 << ((dec_bs_read(bs, 2, 0, 3) + 3)));      

    dec_bs_read1(bs, 1); //marker_bit
    seqhdr->wq_enable = (u8)dec_bs_read1(bs, -1);

    if (seqhdr->wq_enable) {
        seqhdr->seq_wq_mode = dec_bs_read1(bs, -1); 
        if (seqhdr->seq_wq_mode) {
            read_wq_matrix(bs, seqhdr->wq_4x4_matrix, seqhdr->wq_8x8_matrix);
        } else {
            memcpy(seqhdr->wq_4x4_matrix, g_tbl_wq_default_matrix_4x4, sizeof(g_tbl_wq_default_matrix_4x4));
            memcpy(seqhdr->wq_8x8_matrix, g_tbl_wq_default_matrix_8x8, sizeof(g_tbl_wq_default_matrix_8x8));
        }
    }
    seqhdr->secondary_transform_enable_flag         = (u8)dec_bs_read1(bs, -1);
    seqhdr->sample_adaptive_offset_enable_flag      = (u8)dec_bs_read1(bs, -1);
    seqhdr->adaptive_leveling_filter_enable_flag    = (u8)dec_bs_read1(bs, -1);
    seqhdr->affine_enable_flag                      = (u8)dec_bs_read1(bs, -1);
    seqhdr->smvd_enable_flag                        = (u8)dec_bs_read1(bs, -1);
    seqhdr->ipcm_enable_flag                        = (u8)dec_bs_read1(bs, -1);
    seqhdr->amvr_enable_flag                        = (u8)dec_bs_read1(bs, -1);
    seqhdr->num_of_hmvp_cand                        = (u8)dec_bs_read(bs, 4, 0, 8);
    seqhdr->umve_enable_flag                        = (u8)dec_bs_read1(bs, -1);

    if (seqhdr->amvr_enable_flag && seqhdr->num_of_hmvp_cand) {
        seqhdr->emvr_enable_flag = (u8)dec_bs_read1(bs, -1);
    } else {
        seqhdr->emvr_enable_flag = 0;
    }

    seqhdr->ipf_enable_flag   = (u8)dec_bs_read1(bs, -1); 
    seqhdr->tscpm_enable_flag = (u8)dec_bs_read1(bs, -1);

    dec_bs_read1(bs, 1); //marker_bit

    seqhdr->dt_intra_enable_flag = (u8)dec_bs_read1(bs, -1);

    if (seqhdr->dt_intra_enable_flag) {
        u32 log2_max_dt_size_minus4 = dec_bs_read(bs, 2, 0, 2); 
        seqhdr->max_dt_size = (u8)(1 << (log2_max_dt_size_minus4 + 4)); 
    }

    seqhdr->position_based_transform_enable_flag = (u8)dec_bs_read1(bs, -1);

    if (seqhdr->low_delay == 0) {
        seqhdr->output_reorder_delay = (u8)dec_bs_read(bs, 5, 0, COM_UINT32_MAX);
    } else {
        seqhdr->output_reorder_delay = 0;
    }

    seqhdr->cross_patch_loop_filter   = (u8)dec_bs_read1(bs, -1);      
    seqhdr->patch_ref_colocated       = (u8)dec_bs_read1(bs, -1);      
    seqhdr->patch_stable              = (u8)dec_bs_read1(bs,  1);      

    if (seqhdr->patch_stable) {
        seqhdr->patch_uniform = (u8)dec_bs_read1(bs, 1); 
        if (seqhdr->patch_uniform) {
            dec_bs_read1(bs, 1); //marker_bit
            seqhdr->patch_width  = (u8)dec_bs_read_ue(bs, 0, 255) + 1; 
            seqhdr->patch_height = (u8)dec_bs_read_ue(bs, 0, 143) + 1; 
        }
    }

    dec_bs_read(bs, 2, 0, COM_UINT32_MAX);//reserved bits

    dec_bs_read1(bs, 1); //marker_bit

    while (!DEC_BS_IS_ALIGN(bs)) {
        dec_bs_read1(bs, -1);
    }

    return RET_OK;
}

static int user_data(com_pic_header_t *pichdr, com_bs_t * bs)
{
    dec_bs_read(bs, 24, 1, 1);
    dec_bs_read(bs, 8, 0xB2, 0xB2); 

    while (dec_bs_next(bs, 24) != 0x1 && bs->cur <= bs->end) {
        int    i;
        u32 code;
   
        uavs3d_assert_return(DEC_BS_IS_ALIGN(bs), ERR_UNKNOWN);
        code = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);

        while (code != COM_UD_END) {
            switch (code) {
            case COM_UD_PIC_SIGNATURE:
                uavs3d_assert(pichdr);

                for (i = 0; i < 16; i++) {
                    pichdr->pic_md5[i] = (u8)dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
                    if (i % 2 == 1) {
                        dec_bs_read1(bs, 1); 
                    }
                }
                pichdr->pic_md5_exist = 1;
                break;
            default:
                return ERR_UNEXPECTED;
            }
            code = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
        }

    }
    return RET_OK;
}

static int sequence_display_extension(com_bs_t * bs, com_seqh_t *seqhdr)
{
    dec_bs_read(bs, 3, 0, COM_UINT32_MAX);             // video_format             u(3)
    dec_bs_read1(bs, -1);                              // sample_range             u(1)

    seqhdr->colour_description = dec_bs_read1(bs, -1);     // colour_description       u(1)

    if (seqhdr->colour_description) {
        seqhdr->colour_primaries         = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);   // colour_primaries         u(8)
        seqhdr->transfer_characteristics = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);   // transfer_characteristics u(8)
        seqhdr->matrix_coefficients      = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);   // matrix_coefficients      u(8)
    }                                                  
    seqhdr->display_horizontal_size = dec_bs_read(bs, 14, 0, COM_UINT32_MAX);            // display_horizontal_size  u(14)
    dec_bs_read1(bs, 1); //marker_bit                  
    seqhdr->display_vertical_size = dec_bs_read(bs, 14, 0, COM_UINT32_MAX);            // display_vertical_size    u(14)
    char td_mode_flag = dec_bs_read1(bs, -1);          // td_mode_flag             u(1)
                                                       
    if (td_mode_flag == 1) {                           
        dec_bs_read(bs, 8, 0, COM_UINT32_MAX);         // td_packing_mode          u(8)
        dec_bs_read1(bs, -1);                          // view_reverse_flag        u(1)
    }                                                  
                                                       
    dec_bs_read1(bs, 1); //marker_bit                  
                                                       
    while (!DEC_BS_IS_ALIGN(bs)) {
        dec_bs_read1(bs, -1);
    }
    while (dec_bs_next(bs, 24) != 0x1 && bs->cur <= bs->end) {
        int ret = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
        uavs3d_assert(ret == 0);
    }
    return 0;
}

static int temporal_scalability_extension(com_bs_t * bs)
{
    int num_of_temporal_level_minus1 = dec_bs_read(bs, 3, 0, COM_UINT32_MAX); // num_of_temporal_level_minus1 u(3)
    for (int i = 0; i < num_of_temporal_level_minus1; i++)
    {
        dec_bs_read(bs, 4, 0, COM_UINT32_MAX);                                // temporal_frame_rate_code[i]  u(4)
        dec_bs_read(bs, 18, 0, COM_UINT32_MAX);                               // temporal_bit_rate_lower[i]   u(18)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 12, 0, COM_UINT32_MAX);                               // temporal_bit_rate_upper[i]   u(12)
    }
    return 0;
}

static int copyright_extension(com_bs_t * bs)
{
    dec_bs_read1(bs, -1);                             // copyright_flag       u(1)
    dec_bs_read(bs, 8, 0, COM_UINT32_MAX);            // copyright_id         u(8)
    dec_bs_read1(bs, -1);                             // original_or_copy     u(1)
    dec_bs_read(bs, 7, 0, COM_UINT32_MAX);            // reserved_bits        r(7)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 20, 0, COM_UINT32_MAX);           // copyright_number_1   u(20)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // copyright_number_2   u(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // copyright_number_3   u(22)
    return 0;
}

static int mastering_display_and_content_metadata_extension(com_bs_t * bs)
{
    for (int c = 0; c < 3; c++) {
        dec_bs_read(bs, 16, 0, COM_UINT32_MAX);       // display_primaries_x[c]           u(16)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 16, 0, COM_UINT32_MAX);       // display_primaries_y[c]           u(16)
        dec_bs_read1(bs, 1); //marker_bit
    }
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // white_point_x                    u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // white_point_y                    u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // max_display_mastering_luminance  u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // min_display_mastering_luminance  u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // max_content_light_level          u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // max_picture_average_light_level  u(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // reserved_bits                    r(16)
    return 0;
}

static int camera_parameters_extension(com_bs_t * bs)
{
    dec_bs_read1(bs, -1);                             // reserved_bits            r(1)
    dec_bs_read(bs, 7, 0, COM_UINT32_MAX);            // camera_id                u(7)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // height_of_image_device   u(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // focal_length             u(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // f_number                 u(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // vertical_angle_of_view   u(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_x_upper  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_x_lower  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_y_upper  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_y_lower  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_z_upper  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // camera_position_z_lower  i(16)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // camera_direction_x       i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // camera_direction_y       i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // camera_direction_z       i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // image_plane_vertical_x   i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // image_plane_vertical_y   i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 22, 0, COM_UINT32_MAX);           // image_plane_vertical_z   i(22)
    dec_bs_read1(bs, 1); //marker_bit
    dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           //                          r(16)
    return 0;
}

static int roi_parameters_extension(com_bs_t * bs, int slice_type)
{
    int current_picture_roi_num = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);  // current_picture_roi_num  u(8)
    int roiIndex = 0;
    if (slice_type != SLICE_I) {
        int PrevPictureROINum = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);    // prev_picture_roi_num     u(8)
        for (int i = 0; i < PrevPictureROINum; i++) {
            int roi_skip_run = dec_bs_read_ue(bs, 0, COM_UINT32_MAX);
            if (roi_skip_run != 0) {
                for (int j = 0; j < roi_skip_run; j++) {
                    dec_bs_read1(bs, -1);                  //skip_roi_mode
                    if (j % 22 == 0)
                        dec_bs_read1(bs, 1); //marker_bit
                }
                dec_bs_read1(bs, -1);
            } else {
                dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);                    // roi_axisx_delta          se(v)
                dec_bs_read1(bs, 1); //marker_bit
                dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);                    // roi_axisy_delta          se(v)
                dec_bs_read1(bs, 1); //marker_bit
                dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);                    // roi_width_delta          se(v)
                dec_bs_read1(bs, 1); //marker_bit
                dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);                    // roi_height_delta         se(v)
                dec_bs_read1(bs, 1); //marker_bit
            }
        }
    }
    for (int i = roiIndex; i < current_picture_roi_num; i++) {
        dec_bs_read(bs, 6, 0, COM_UINT32_MAX);                            // roi_axisx                u(6)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 6, 0, COM_UINT32_MAX);                            // roi_axisy                u(6)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 6, 0, COM_UINT32_MAX);                            // roi_width                u(6)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 6, 0, COM_UINT32_MAX);                            // roi_height               u(6)
        dec_bs_read1(bs, 1); //marker_bit
    }
    return 0;
}

static int picture_display_extension(com_bs_t * bs, com_seqh_t* sqh, com_pic_header_t* pichdr)
{
    int num_frame_center_offsets;
    if (sqh->progressive_sequence == 1) {
        if (pichdr->repeat_first_field == 1) {
            num_frame_center_offsets = pichdr->top_field_first == 1 ? 3 : 2;
        } else {
            num_frame_center_offsets = 1;
        }
    } else {
        if (pichdr->picture_structure == 0) {
            num_frame_center_offsets = 1;
        } else {
            num_frame_center_offsets = pichdr->repeat_first_field == 1 ? 3 : 2;
        }
    }
    for (int i = 0; i < num_frame_center_offsets; i++) {
        dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // picture_centre_horizontal_offset i(16)
        dec_bs_read1(bs, 1); //marker_bit
        dec_bs_read(bs, 16, 0, COM_UINT32_MAX);           // picture_centre_vertical_offset   i(16)
        dec_bs_read1(bs, 1); //marker_bit
    }
    return 0;
}

static int extension_data(com_bs_t * bs, com_seqh_t *seqhdr, com_pic_header_t *pichdr, int i, int slice_type)
{
    while (dec_bs_next(bs, 32) == 0x1B5) {
        dec_bs_read(bs, 24, 1, 1);        
        dec_bs_read(bs, 8, 0xB5, 0xB5);  
        if (i == 0) {
            int ret = dec_bs_read(bs, 4, 0, COM_UINT32_MAX);
            if (ret == 2) {
                sequence_display_extension(bs, seqhdr);
            } else if (ret == 3) {
                temporal_scalability_extension(bs);
            } else if (ret == 4) {
                copyright_extension(bs);
            } else if (ret == 0xa) {
                mastering_display_and_content_metadata_extension(bs);
            } else if (ret == 0xb) {
                camera_parameters_extension(bs);
            } else {
                while (dec_bs_next(bs, 24) != 1 && bs->cur <= bs->end) {
                    dec_bs_read(bs, 8, 0, COM_UINT32_MAX);    // reserved_extension_data_byte u(8)
                }
            }
        } else {
            int ret = dec_bs_read(bs, 4, 0, COM_UINT32_MAX);
            if (ret == 4)  {
                copyright_extension(bs);
            } else if (ret == 7) {
                picture_display_extension(bs, seqhdr, pichdr);
            } else if (ret == 0xb) {
                camera_parameters_extension(bs);
            } else if (ret == 0xc) {
                roi_parameters_extension(bs, slice_type);
            } else {
                while (dec_bs_next(bs, 24) != 1 && bs->cur <= bs->end)
                {
                    dec_bs_read(bs, 8, 0, COM_UINT32_MAX);    // reserved_extension_data_byte u(8)
                }
            }
        }
    }
    return 0;
}

int dec_parse_ext_and_usr_data(com_bs_t * bs, com_seqh_t *seqhdr, com_pic_header_t * pichdr, int i, int slicetype)
{
    while ((dec_bs_next(bs, 32) == 0x1B5 || dec_bs_next(bs, 32) == 0x1B2) && bs->cur <= bs->end) {

        if (dec_bs_next(bs, 32) == 0x1B5) {
            extension_data(bs, seqhdr, pichdr, i, slicetype);
        }
        if (dec_bs_next(bs, 32) == 0x1B2) {
            user_data(pichdr, bs);
        }
    }
    return 0;
}

static int dec_parse_deblock_param(com_bs_t *bs, com_pic_header_t *pichdr)
{
    pichdr->loop_filter_disable_flag = (u8)dec_bs_read1(bs, -1);
    if (pichdr->loop_filter_disable_flag == 0) {
        pichdr->loop_filter_parameter_flag = (u8)dec_bs_read1(bs, -1);
        if (pichdr->loop_filter_parameter_flag) {
            pichdr->alpha_c_offset = dec_bs_read_se(bs, -8, 8);
            pichdr->beta_offset    = dec_bs_read_se(bs, -8, 8);
        } else {
            pichdr->alpha_c_offset = 0;
            pichdr->beta_offset    = 0;
        }
    }
    return  RET_OK;
}

static int dec_parse_alf_pic_param(com_bs_t *bs, com_pic_header_t *ph)
{
    int i, lac;
    com_alf_pic_param_t *alf_param = &ph->alf_param;
 
    alf_param->alf_flag[Y_C] = dec_bs_read1(bs, -1);
    alf_param->alf_flag[U_C] = dec_bs_read1(bs, -1);
    alf_param->alf_flag[V_C] = dec_bs_read1(bs, -1);

    alf_param->pic_alf_enable = (alf_param->alf_flag[Y_C] || alf_param->alf_flag[U_C] || alf_param->alf_flag[V_C]);

    if (alf_param->alf_flag[0]) {
        int pre_symbole, g, i;
        int filterPattern[16];
        int groups = dec_bs_read_ue(bs, 0, 15) + 1;
        memset(filterPattern, 0, NO_VAR_BINS * sizeof(int));
        pre_symbole = 0;

        for (g = 0; g < groups; g++) {
            int sum = 0;
            
            if (g > 0) {
                int symbol;
                if (groups != 16) {
                    symbol = dec_bs_read_ue(bs, 1, 15);
                } else {
                    symbol = 1;
                }
                symbol = COM_MIN(15, symbol + pre_symbole);
                filterPattern[symbol] = 1;
                pre_symbole = symbol;
            }
            
            for (i = 0; i < ALF_MAX_NUM_COEF - 1; i++) {
                alf_param->filterCoeff_luma[g][i] = dec_bs_read_se(bs, -64, 63);
                sum += (2 * alf_param->filterCoeff_luma[g][i]);
            }
            lac = dec_bs_read_se(bs, -1088, 1071);
            alf_param->filterCoeff_luma[g][ALF_MAX_NUM_COEF - 1] = (1 << 6) - sum + lac;
        }

        if (groups > 1) {
            alf_param->varIndTab[0] = 0;
            for (g = 1; g < NO_VAR_BINS; ++g) {
                if (filterPattern[g]) {
                    alf_param->varIndTab[g] = alf_param->varIndTab[g - 1] + 1;
                } else {
                    alf_param->varIndTab[g] = alf_param->varIndTab[g - 1];
                }
            }
        } else {
            memset(alf_param->varIndTab, 0, NO_VAR_BINS * sizeof(int));
        }
    }
    if (alf_param->alf_flag[1]) {
        int sum = 0;
        for (i = 0; i < (ALF_MAX_NUM_COEF - 1) * 2; i += 2) {
            alf_param->filterCoeff_chroma[i] = dec_bs_read_se(bs, -64, 63);
            sum += (2 * alf_param->filterCoeff_chroma[i]);
            
        }
        lac = dec_bs_read_se(bs, -1088, 1071);
        alf_param->filterCoeff_chroma[(ALF_MAX_NUM_COEF - 1) * 2] = (1 << 6) - sum + lac;
    }
    if (alf_param->alf_flag[2]) {
        int sum = 0;
        for (i = 1; i < (ALF_MAX_NUM_COEF - 1) * 2 + 1; i += 2) {
            alf_param->filterCoeff_chroma[i] = dec_bs_read_se(bs, -64, 63);
            sum += (2 * alf_param->filterCoeff_chroma[i]);

        }
        lac = dec_bs_read_se(bs, -1088, 1071);
        alf_param->filterCoeff_chroma[(ALF_MAX_NUM_COEF - 1) * 2 + 1] = (1 << 6) - sum + lac;
    }

    return RET_OK;
}

int dec_parse_pic_header(com_bs_t * bs, com_pic_header_t * pichdr, com_seqh_t * seqhdr, com_pic_manager_t *pm)
{
    unsigned int ret = dec_bs_read(bs, 24, 1, 1);
    unsigned int start_code = dec_bs_read(bs, 8, 0xB3, 0xB6);

    com_check_val2(start_code, 0xB3, 0xB6);

    if (start_code == 0xB3) {
        pichdr->slice_type = SLICE_I;
    } else { 
        pichdr->random_access_decodable_flag = dec_bs_read1(bs, -1);
    }

    pichdr->bbv_delay = dec_bs_read(bs, 8, 0, COM_UINT32_MAX) << 24 | 
                        dec_bs_read(bs, 8, 0, COM_UINT32_MAX) << 16 | 
                        dec_bs_read(bs, 8, 0, COM_UINT32_MAX) <<  8 | 
                        dec_bs_read(bs, 8, 0, COM_UINT32_MAX);

    if (start_code == 0xB6) { //picture_coding_type
        pichdr->slice_type = (dec_bs_read(bs, 2, 1, 2) == 1 ? SLICE_P : SLICE_B);
    }
    if (pichdr->slice_type == SLICE_I) {
        pichdr->time_code_flag = dec_bs_read1(bs, -1);
        if (pichdr->time_code_flag == 1) {
            pichdr->time_code = dec_bs_read(bs, 24, 0, COM_UINT32_MAX);
        }
    }
    pichdr->decode_order_index = dec_bs_read(bs, 8, 0, COM_UINT32_MAX);

    if (seqhdr->temporal_id_enable_flag == 1) {
        pichdr->temporal_id = (u8)dec_bs_read(bs, 3, 0, COM_UINT32_MAX);
    }
    if (seqhdr->low_delay == 0) {
        pichdr->output_delay = dec_bs_read_ue(bs, 0, 63); 
    } else {
        pichdr->output_delay = 0;
        pichdr->bbv_check_times = dec_bs_read_ue(bs, 0, COM_UINT32_MAX);
    }

    pichdr->progressive_frame = dec_bs_read1(bs, -1);

    if (pichdr->progressive_frame == 0) {
        pichdr->picture_structure = dec_bs_read1(bs, -1);
    } else {
        pichdr->picture_structure = 1;
    }
    pichdr->top_field_first = dec_bs_read1(bs, -1);
    pichdr->repeat_first_field = dec_bs_read1(bs, -1);

    if (seqhdr->field_coded_sequence == 1) {
        pichdr->top_field_picture_flag = dec_bs_read1(bs, -1);
        dec_bs_read1(bs, -1); // reserved_bits
    }
 
    if (pichdr->decode_order_index < pm->prev_doi) {
        pm->doi_cycles++; 

        for (int i = 0; i < pm->cur_pb_size; i++) {
            com_pic_t *pic = pm->list[i];
            if (pic != NULL) {
                pic->doi = pic->doi - DOI_CYCLE_LENGTH;
            }
        }
    } 
    pichdr->ptr = pichdr->decode_order_index + (DOI_CYCLE_LENGTH * pm->doi_cycles) + pichdr->output_delay - seqhdr->output_reorder_delay;
    pm->prev_doi = pichdr->decode_order_index;

    //initialization
    pichdr->rpl_l0_idx = pichdr->rpl_l1_idx = -1;
    pichdr->ref_pic_list_sps_flag[0] = pichdr->ref_pic_list_sps_flag[1] = 0;

    for (int i = 0; i < MAX_REFS; i++) {
        pichdr->rpl_l0.delta_doi[i] = 0;
        pichdr->rpl_l1.delta_doi[i] = 0;
    }

    pichdr->ref_pic_list_sps_flag[0] = dec_bs_read1(bs, -1);

    if (pichdr->ref_pic_list_sps_flag[0]) {
        if (seqhdr->rpls_l0_num) {
            if (seqhdr->rpls_l0_num > 1) {
                pichdr->rpl_l0_idx = dec_bs_read_ue(bs, 0, seqhdr->rpls_l0_num - 1);
            } else { 
                pichdr->rpl_l0_idx = 0;
            }
            memcpy(&pichdr->rpl_l0, &seqhdr->rpls_l0[pichdr->rpl_l0_idx], sizeof(pichdr->rpl_l0));
        }
    } else {
        dec_parse_rlp(bs, &pichdr->rpl_l0, seqhdr->max_dpb_size);
    }

    //L1 candidates signaling
    pichdr->ref_pic_list_sps_flag[1] = seqhdr->rpl1_index_exist_flag ? dec_bs_read1(bs, -1) : pichdr->ref_pic_list_sps_flag[0];

    if (pichdr->ref_pic_list_sps_flag[1]) {
        if (seqhdr->rpls_l1_num > 1 && seqhdr->rpl1_index_exist_flag) {
            pichdr->rpl_l1_idx = dec_bs_read_ue(bs, 0, seqhdr->rpls_l1_num - 1); 
        } else if (!seqhdr->rpl1_index_exist_flag) {
            pichdr->rpl_l1_idx = pichdr->rpl_l0_idx;
        } else { 
            uavs3d_assert(seqhdr->rpls_l1_num == 1);
            pichdr->rpl_l1_idx = 0;
        }
        memcpy(&pichdr->rpl_l1, &seqhdr->rpls_l1[pichdr->rpl_l1_idx], sizeof(pichdr->rpl_l1));
    } else {
        dec_parse_rlp(bs, &pichdr->rpl_l1, seqhdr->max_dpb_size);
    }

    if (pichdr->slice_type != SLICE_I) {
        pichdr->num_ref_idx_active_override_flag = dec_bs_read1(bs, -1);
        if (pichdr->num_ref_idx_active_override_flag) {
            pichdr->rpl_l0.active = (u32)dec_bs_read_ue(bs, 0, 14) + 1; 
            if (pichdr->slice_type == SLICE_B) {
                pichdr->rpl_l1.active = (u32)dec_bs_read_ue(bs, 0, 14) + 1;
            }
        } else {
            pichdr->rpl_l0.active = seqhdr->num_ref_default_active_minus1[0] + 1;

            if (pichdr->slice_type == SLICE_P) {
              pichdr->rpl_l1.active = 0;
            } else {
              pichdr->rpl_l1.active = seqhdr->num_ref_default_active_minus1[1] + 1;
            }
        }
    }
    if (pichdr->slice_type == SLICE_I) {
        pichdr->rpl_l0.active = 0;
        pichdr->rpl_l1.active = 0;
    }

    pichdr->fixed_picture_qp_flag = dec_bs_read1(bs, -1);
    pichdr->picture_qp = dec_bs_read(bs, 7, 0, MAX_QUANT_BASE + seqhdr->bit_depth_2_qp_offset);

    if (pichdr->slice_type != SLICE_I && !(pichdr->slice_type == SLICE_B && pichdr->picture_structure == 1)) {
        dec_bs_read1(bs, -1); // reserved_bits r(1)
    }
    dec_parse_deblock_param(bs, pichdr);

    pichdr->chroma_quant_param_disable_flag = (u8)dec_bs_read1(bs, -1);
    if (pichdr->chroma_quant_param_disable_flag == 0) {
        pichdr->chroma_quant_param_delta_cb = (u8)dec_bs_read_se(bs, -16, 16);
        pichdr->chroma_quant_param_delta_cr = (u8)dec_bs_read_se(bs, -16, 16);
    } else {
        pichdr->chroma_quant_param_delta_cb = pichdr->chroma_quant_param_delta_cr = 0;
    }

    if (seqhdr->wq_enable) {
        pichdr->pic_wq_enable = dec_bs_read1(bs, -1);

        if (pichdr->pic_wq_enable) {
            pichdr->pic_wq_data_idx = dec_bs_read(bs, 2, 0, 2);

            if (pichdr->pic_wq_data_idx == 0) {
                memcpy(pichdr->wq_4x4_matrix, seqhdr->wq_4x4_matrix, sizeof(seqhdr->wq_4x4_matrix));
                memcpy(pichdr->wq_8x8_matrix, seqhdr->wq_8x8_matrix, sizeof(seqhdr->wq_8x8_matrix));
            } else if (pichdr->pic_wq_data_idx == 1) {
                int delta, i;

                dec_bs_read1(bs, -1); //reserved_bits r(1)

                pichdr->wq_param = dec_bs_read(bs, 2, 0, 2);
                pichdr->wq_model = dec_bs_read(bs, 2, 0, 3);

                if (pichdr->wq_param == 0) {
                    memcpy(pichdr->wq_param_vector, g_tbl_wq_default_param[1], sizeof(pichdr->wq_param_vector));
                }  else if (pichdr->wq_param == 1) {
                    for (i = 0; i < 6; i++) {
                        delta = dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);
                        pichdr->wq_param_vector[i] = delta + g_tbl_wq_default_param[0][i];
                    }
                } else {
                    for (i = 0; i < 6; i++) {
                        delta = dec_bs_read_se(bs, COM_INT32_MIN, COM_INT32_MAX);
                        pichdr->wq_param_vector[i] = delta + g_tbl_wq_default_param[1][i];
                    }
                }
                for (i = 0; i < 16; i++) {
                    pichdr->wq_4x4_matrix[i] = pichdr->wq_param_vector[g_tbl_wq_model_4x4[pichdr->wq_model][i]];
                }
                for (i = 0; i < 64; i++) {
                    pichdr->wq_8x8_matrix[i] = pichdr->wq_param_vector[g_tbl_wq_model_8x8[pichdr->wq_model][i]];
                }
            } else {
                read_wq_matrix(bs, pichdr->wq_4x4_matrix, pichdr->wq_8x8_matrix);
            }
        } else {
            memset(pichdr->wq_4x4_matrix, 64, sizeof(pichdr->wq_4x4_matrix));
            memset(pichdr->wq_8x8_matrix, 64, sizeof(pichdr->wq_8x8_matrix));
        }
    } else {
        pichdr->pic_wq_enable = 0;
        memset(pichdr->wq_4x4_matrix, 64, sizeof(pichdr->wq_4x4_matrix));
        memset(pichdr->wq_8x8_matrix, 64, sizeof(pichdr->wq_8x8_matrix));
    }

    if (seqhdr->adaptive_leveling_filter_enable_flag) {
        dec_parse_alf_pic_param(bs, pichdr);
    }

    if (pichdr->slice_type != SLICE_I && seqhdr->affine_enable_flag) {
        pichdr->affine_subblock_size_idx = dec_bs_read1(bs, -1);
    }

    dec_bs_read1(bs, 1); //marker_bit

    /* byte align */
    while (!DEC_BS_IS_ALIGN(bs)) {
        dec_bs_read1(bs, -1);
    }

    return RET_OK;
}

int dec_parse_patch_header(com_bs_t * bs, com_seqh_t *seqhdr, com_pic_header_t * ph, com_patch_header_t * sh)
{
    int ret = dec_bs_read(bs, 24, 1, 1);
    int idx = dec_bs_read(bs, 8, 0, 0x8E);

    if (!ph->fixed_picture_qp_flag) {
        sh->fixed_slice_qp_flag = (u8)dec_bs_read1(bs, -1);
        sh->slice_qp = (u8)dec_bs_read(bs, 7, 0, MAX_QUANT_BASE + seqhdr->bit_depth_2_qp_offset);
    } else {
        sh->fixed_slice_qp_flag = 1;
        sh->slice_qp = (u8)ph->picture_qp;
    }
    if (seqhdr->sample_adaptive_offset_enable_flag) {
        sh->slice_sao_enable[Y_C] = (u8)dec_bs_read1(bs, -1);
        sh->slice_sao_enable[U_C] = (u8)dec_bs_read1(bs, -1);
        sh->slice_sao_enable[V_C] = (u8)dec_bs_read1(bs, -1);
    }
    /* byte align */
    while (!DEC_BS_IS_ALIGN(bs)) {
        dec_bs_read1(bs, -1);
    }

    /* push back */
    bs->code = 0;
    bs->cur -= bs->leftbits >> 3;
    bs->leftbits = 0;

    return idx;
}

int dec_parse_patch_end(com_bs_t * bs)
{
    int ret;

    while (dec_bs_next(bs, 24) != 0x1 && bs->cur <= bs->end) {
        dec_bs_read(bs, 8, 0, COM_UINT32_MAX);
    }
    ret = dec_bs_read(bs, 24, 1, 1);  
    ret = dec_bs_read(bs,  8, 0x8F, 0x8F); 

    return RET_OK;
}

/* ============ LBAC ============== */

static tab_s16 tbl_plps[4 * 4096] = {
    5750,   5751,   5752,   5753,   5754,   5755,   5756,   5757,   5758,   5759,   5760,   5761,   5762,   5763,   5764,   5765,   5766,   5767,   5768,   5769,   5770,   5771,   5772,   5773,   5774,   5775,   5776,   5777,   5778,   5779,   5780,   5781,
    5782,   5783,   5784,   5785,   5786,   5787,   5788,   5789,   5790,   5791,   5792,   5793,   5794,   5795,   5796,   5797,   5798,   5799,   5800,   5801,   5802,   5803,   5804,   5805,   5806,   5807,   5808,   5809,   5810,   5811,   5812,   5813,
    5814,   5815,   5816,   5817,   5818,   5819,   5820,   5821,   5822,   5823,   5824,   5825,   5826,   5827,   5828,   5829,   5830,   5831,   5832,   5833,   5834,   5835,   5836,   5837,   5838,   5839,   5840,   5841,   5842,   5843,   5844,   5845,
    5846,   5847,   5848,   5849,   5850,   5851,   5852,   5853,   5854,   5855,   5856,   5857,   5858,   5859,   5860,   5861,   5862,   5863,   5864,   5865,   5866,   5867,   5868,   5869,   5870,   5871,   5872,   5873,   5874,   5875,   5876,   5877,
    5878,   5879,   5880,   5881,   5882,   5883,   5884,   5885,   5886,   5887,   5888,   5889,   5890,   5891,   5892,   5893,   5894,   5895,   5896,   5897,   5898,   5899,   5900,   5901,   5902,   5903,   5904,   5905,   5906,   5907,   5908,   5909,
    5910,   5911,   5912,   5913,   5914,   5915,   5916,   5917,   5918,   5919,   5920,   5921,   5922,   5923,   5924,   5925,   5926,   5927,   5928,   5929,   5930,   5931,   5932,   5933,   5934,   5935,   5936,   5937,   5938,   5939,   5940,   5941,
    5942,   5943,   5944,   5945,   5946,   5947,   5948,   5949,   5950,   5951,   5952,   5953,   5954,   5955,   5956,   5957,   5958,   5959,   5960,   5961,   5962,   5963,   5964,   5965,   5966,   5967,   5968,   5969,   5970,   5971,   5972,   5973,
    5974,   5975,   5976,   5977,   5978,   5979,   5980,   5981,   5982,   5983,   5984,   5985,   5986,   5987,   5988,   5989,   5990,   5991,   5992,   5993,   5994,   5995,   5996,   5997,   5998,   5999,   6000,   6001,   6002,   6003,   6004,   6005,
    6006,   6007,   6008,   6009,   6010,   6011,   6012,   6013,   6014,   6015,   6016,   6017,   6018,   6019,   6020,   6021,   6022,   6023,   6024,   6025,   6026,   6027,   6028,   6029,   6030,   6031,   6032,   6033,   6034,   6035,   6036,   6037,
    6038,   6039,   6040,   6041,   6042,   6043,   6044,   6045,   6046,   6047,   6048,   6049,   6050,   6051,   6052,   6053,   6054,   6055,   6056,   6057,   6058,   6059,   6060,   6061,   6062,   6063,   6064,   6065,   6066,   6067,   6068,   6069,
    6070,   6071,   6072,   6073,   6074,   6075,   6076,   6077,   6078,   6079,   6080,   6081,   6082,   6083,   6084,   6085,   6086,   6087,   6088,   6089,   6090,   6091,   6092,   6093,   6094,   6095,   6096,   6097,   6098,   6099,   6100,   6101,
    6102,   6103,   6104,   6105,   6106,   6107,   6108,   6109,   6110,   6111,   6112,   6113,   6114,   6115,   6116,   6117,   6118,   6119,   6120,   6121,   6122,   6123,   6124,   6125,   6126,   6127,   6128,   6129,   6130,   6131,   6132,   6133,
    6134,   6135,   6136,   6137,   6138,   6139,   6140,   6141,   6142,   6143,   6143,   6142,   6141,   6140,   6139,   6138,   6137,   6136,   6135,   6134,   6133,   6132,   6131,   6130,   6129,   6128,   6127,   6126,   6125,   6124,   6123,   6122,
    6121,   6120,   6119,   6118,   6117,   6116,   6115,   6114,   6113,   6112,   6111,   6110,   6109,   6108,   6107,   6106,   6105,   6104,   6103,   6102,   6101,   6100,   6099,   6098,   6097,   6096,   6095,   6094,   6093,   6092,   6091,   6090,
    6089,   6088,   6087,   6086,   6085,   6084,   6083,   6082,   6081,   6080,   6079,   6078,   6077,   6076,   6075,   6074,   6073,   6072,   6071,   6070,   6069,   6068,   6067,   6066,   6065,   6064,   6063,   6062,   6061,   6060,   6059,   6058,
    6057,   6056,   6055,   6054,   6053,   6052,   6051,   6050,   6049,   6048,   6047,   6046,   6045,   6044,   6043,   6042,   6041,   6040,   6039,   6038,   6037,   6036,   6035,   6034,   6033,   6032,   6031,   6030,   6029,   6028,   6027,   6026,
    6025,   6024,   6023,   6022,   6021,   6020,   6019,   6018,   6017,   6016,   6015,   6014,   6013,   6012,   6011,   6010,   6009,   6008,   6007,   6006,   6005,   6004,   6003,   6002,   6001,   6000,   5999,   5998,   5997,   5996,   5995,   5994,
    5993,   5992,   5991,   5990,   5989,   5988,   5987,   5986,   5985,   5984,   5983,   5982,   5981,   5980,   5979,   5978,   5977,   5976,   5975,   5974,   5973,   5972,   5971,   5970,   5969,   5968,   5967,   5966,   5965,   5964,   5963,   5962,
    5961,   5960,   5959,   5958,   5957,   5956,   5955,   5954,   5953,   5952,   5951,   5950,   5949,   5948,   5947,   5946,   5945,   5944,   5943,   5942,   5941,   5940,   5939,   5938,   5937,   5936,   5935,   5934,   5933,   5932,   5931,   5930,
    5929,   5928,   5927,   5926,   5925,   5924,   5923,   5922,   5921,   5920,   5919,   5918,   5917,   5916,   5915,   5914,   5913,   5912,   5911,   5910,   5909,   5908,   5907,   5906,   5905,   5904,   5903,   5902,   5901,   5900,   5899,   5898,
    5897,   5896,   5895,   5894,   5893,   5892,   5891,   5890,   5889,   5888,   5887,   5886,   5885,   5884,   5883,   5882,   5881,   5880,   5879,   5878,   5877,   5876,   5875,   5874,   5873,   5872,   5871,   5870,   5869,   5868,   5867,   5866,
    5865,   5864,   5863,   5862,   5861,   5860,   5859,   5858,   5857,   5856,   5855,   5854,   5853,   5852,   5851,   5850,   5849,   5848,   5847,   5846,   5845,   5844,   5843,   5842,   5841,   5840,   5839,   5838,   5837,   5836,   5835,   5834,
    5833,   5832,   5831,   5830,   5829,   5828,   5827,   5826,   5825,   5824,   5823,   5822,   5821,   5820,   5819,   5818,   5817,   5816,   5815,   5814,   5813,   5812,   5811,   5810,   5809,   5808,   5807,   5806,   5805,   5804,   5803,   5802,
    5801,   5800,   5799,   5798,   5797,   5796,   5795,   5794,   5793,   5792,   5791,   5790,   5789,   5788,   5787,   5786,   5785,   5784,   5783,   5782,   5781,   5780,   5779,   5778,   5777,   5776,   5775,   5774,   5773,   5772,   5771,   5770,
    5769,   5768,   5767,   5766,   5765,   5764,   5763,   5762,   5761,   5760,   5759,   5758,   5757,   5756,   5755,   5754,   5753,   5752,   5751,   5750,   5749,   5748,   5747,   5746,   5745,   5744,   5743,   5742,   5741,   5740,   5739,   5738,
    5737,   5736,   5735,   5734,   5733,   5732,   5731,   5730,   5729,   5728,   5727,   5726,   5725,   5724,   5723,   5722,   5721,   5720,   5719,   5718,   5717,   5716,   5715,   5714,   5713,   5712,   5711,   5710,   5709,   5708,   5707,   5706,
    5705,   5704,   5703,   5702,   5701,   5700,   5699,   5698,   5697,   5696,   5695,   5694,   5693,   5692,   5691,   5690,   5689,   5688,   5687,   5686,   5685,   5684,   5683,   5682,   5681,   5680,   5679,   5678,   5677,   5676,   5675,   5674,
    5673,   5672,   5671,   5670,   5669,   5668,   5667,   5666,   5665,   5664,   5663,   5662,   5661,   5660,   5659,   5658,   5657,   5656,   5655,   5654,   5653,   5652,   5651,   5650,   5649,   5648,   5647,   5646,   5645,   5644,   5643,   5642,
    5641,   5640,   5639,   5638,   5637,   5636,   5635,   5634,   5633,   5632,   5631,   5630,   5629,   5628,   5627,   5626,   5625,   5624,   5623,   5622,   5621,   5620,   5619,   5618,   5617,   5616,   5615,   5614,   5613,   5612,   5611,   5610,
    5609,   5608,   5607,   5606,   5605,   5604,   5603,   5602,   5601,   5600,   5599,   5598,   5597,   5596,   5595,   5594,   5593,   5592,   5591,   5590,   5589,   5588,   5587,   5586,   5585,   5584,   5583,   5582,   5581,   5580,   5579,   5578,
    5577,   5576,   5575,   5574,   5573,   5572,   5571,   5570,   5569,   5568,   5567,   5566,   5565,   5564,   5563,   5562,   5561,   5560,   5559,   5558,   5557,   5556,   5555,   5554,   5553,   5552,   5551,   5550,   5549,   5548,   5547,   5546,
    5545,   5544,   5543,   5542,   5541,   5540,   5539,   5538,   5537,   5536,   5535,   5534,   5533,   5532,   5531,   5530,   5529,   5528,   5527,   5526,   5525,   5524,   5523,   5522,   5521,   5520,   5519,   5518,   5517,   5516,   5515,   5514,
    5513,   5512,   5511,   5510,   5509,   5508,   5507,   5506,   5505,   5504,   5503,   5502,   5501,   5500,   5499,   5498,   5497,   5496,   5495,   5494,   5493,   5492,   5491,   5490,   5489,   5488,   5487,   5486,   5485,   5484,   5483,   5482,
    5481,   5480,   5479,   5478,   5477,   5476,   5475,   5474,   5473,   5472,   5471,   5470,   5469,   5468,   5467,   5466,   5465,   5464,   5463,   5462,   5461,   5460,   5459,   5458,   5457,   5456,   5455,   5454,   5453,   5452,   5451,   5450,
    5449,   5448,   5447,   5446,   5445,   5444,   5443,   5442,   5441,   5440,   5439,   5438,   5437,   5436,   5435,   5434,   5433,   5432,   5431,   5430,   5429,   5428,   5427,   5426,   5425,   5424,   5423,   5422,   5421,   5420,   5419,   5418,
    5417,   5416,   5415,   5414,   5413,   5412,   5411,   5410,   5409,   5408,   5407,   5406,   5405,   5404,   5403,   5402,   5401,   5400,   5399,   5398,   5397,   5396,   5395,   5394,   5393,   5392,   5391,   5390,   5389,   5388,   5387,   5386,
    5385,   5384,   5383,   5382,   5381,   5380,   5379,   5378,   5377,   5376,   5375,   5374,   5373,   5372,   5371,   5370,   5369,   5368,   5367,   5366,   5365,   5364,   5363,   5362,   5361,   5360,   5359,   5358,   5357,   5356,   5355,   5354,
    5353,   5352,   5351,   5350,   5349,   5348,   5347,   5346,   5345,   5344,   5343,   5342,   5341,   5340,   5339,   5338,   5337,   5336,   5335,   5334,   5333,   5332,   5331,   5330,   5329,   5328,   5327,   5326,   5325,   5324,   5323,   5322,
    5321,   5320,   5319,   5318,   5317,   5316,   5315,   5314,   5313,   5312,   5311,   5310,   5309,   5308,   5307,   5306,   5305,   5304,   5303,   5302,   5301,   5300,   5299,   5298,   5297,   5296,   5295,   5294,   5293,   5292,   5291,   5290,
    5289,   5288,   5287,   5286,   5285,   5284,   5283,   5282,   5281,   5280,   5279,   5278,   5277,   5276,   5275,   5274,   5273,   5272,   5271,   5270,   5269,   5268,   5267,   5266,   5265,   5264,   5263,   5262,   5261,   5260,   5259,   5258,
    5257,   5256,   5255,   5254,   5253,   5252,   5251,   5250,   5249,   5248,   5247,   5246,   5245,   5244,   5243,   5242,   5241,   5240,   5239,   5238,   5237,   5236,   5235,   5234,   5233,   5232,   5231,   5230,   5229,   5228,   5227,   5226,
    5225,   5224,   5223,   5222,   5221,   5220,   5219,   5218,   5217,   5216,   5215,   5214,   5213,   5212,   5211,   5210,   5209,   5208,   5207,   5206,   5205,   5204,   5203,   5202,   5201,   5200,   5199,   5198,   5197,   5196,   5195,   5194,
    5193,   5192,   5191,   5190,   5189,   5188,   5187,   5186,   5185,   5184,   5183,   5182,   5181,   5180,   5179,   5178,   5177,   5176,   5175,   5174,   5173,   5172,   5171,   5170,   5169,   5168,   5167,   5166,   5165,   5164,   5163,   5162,
    5161,   5160,   5159,   5158,   5157,   5156,   5155,   5154,   5153,   5152,   5151,   5150,   5149,   5148,   5147,   5146,   5145,   5144,   5143,   5142,   5141,   5140,   5139,   5138,   5137,   5136,   5135,   5134,   5133,   5132,   5131,   5130,
    5129,   5128,   5127,   5126,   5125,   5124,   5123,   5122,   5121,   5120,   5119,   5118,   5117,   5116,   5115,   5114,   5113,   5112,   5111,   5110,   5109,   5108,   5107,   5106,   5105,   5104,   5103,   5102,   5101,   5100,   5099,   5098,
    5097,   5096,   5095,   5094,   5093,   5092,   5091,   5090,   5089,   5088,   5087,   5086,   5085,   5084,   5083,   5082,   5081,   5080,   5079,   5078,   5077,   5076,   5075,   5074,   5073,   5072,   5071,   5070,   5069,   5068,   5067,   5066,
    5065,   5064,   5063,   5062,   5061,   5060,   5059,   5058,   5057,   5056,   5055,   5054,   5053,   5052,   5051,   5050,   5049,   5048,   5047,   5046,   5045,   5044,   5043,   5042,   5041,   5040,   5039,   5038,   5037,   5036,   5035,   5034,
    5033,   5032,   5031,   5030,   5029,   5028,   5027,   5026,   5025,   5024,   5023,   5022,   5021,   5020,   5019,   5018,   5017,   5016,   5015,   5014,   5013,   5012,   5011,   5010,   5009,   5008,   5007,   5006,   5005,   5004,   5003,   5002,
    5001,   5000,   4999,   4998,   4997,   4996,   4995,   4994,   4993,   4992,   4991,   4990,   4989,   4988,   4987,   4986,   4985,   4984,   4983,   4982,   4981,   4980,   4979,   4978,   4977,   4976,   4975,   4974,   4973,   4972,   4971,   4970,
    4969,   4968,   4967,   4966,   4965,   4964,   4963,   4962,   4961,   4960,   4959,   4958,   4957,   4956,   4955,   4954,   4953,   4952,   4951,   4950,   4949,   4948,   4947,   4946,   4945,   4944,   4943,   4942,   4941,   4940,   4939,   4938,
    4937,   4936,   4935,   4934,   4933,   4932,   4931,   4930,   4929,   4928,   4927,   4926,   4925,   4924,   4923,   4922,   4921,   4920,   4919,   4918,   4917,   4916,   4915,   4914,   4913,   4912,   4911,   4910,   4909,   4908,   4907,   4906,
    4905,   4904,   4903,   4902,   4901,   4900,   4899,   4898,   4897,   4896,   4895,   4894,   4893,   4892,   4891,   4890,   4889,   4888,   4887,   4886,   4885,   4884,   4883,   4882,   4881,   4880,   4879,   4878,   4877,   4876,   4875,   4874,
    4873,   4872,   4871,   4870,   4869,   4868,   4867,   4866,   4865,   4864,   4863,   4862,   4861,   4860,   4859,   4858,   4857,   4856,   4855,   4854,   4853,   4852,   4851,   4850,   4849,   4848,   4847,   4846,   4845,   4844,   4843,   4842,
    4841,   4840,   4839,   4838,   4837,   4836,   4835,   4834,   4833,   4832,   4831,   4830,   4829,   4828,   4827,   4826,   4825,   4824,   4823,   4822,   4821,   4820,   4819,   4818,   4817,   4816,   4815,   4814,   4813,   4812,   4811,   4810,
    4809,   4808,   4807,   4806,   4805,   4804,   4803,   4802,   4801,   4800,   4799,   4798,   4797,   4796,   4795,   4794,   4793,   4792,   4791,   4790,   4789,   4788,   4787,   4786,   4785,   4784,   4783,   4782,   4781,   4780,   4779,   4778,
    4777,   4776,   4775,   4774,   4773,   4772,   4771,   4770,   4769,   4768,   4767,   4766,   4765,   4764,   4763,   4762,   4761,   4760,   4759,   4758,   4757,   4756,   4755,   4754,   4753,   4752,   4751,   4750,   4749,   4748,   4747,   4746,
    4745,   4744,   4743,   4742,   4741,   4740,   4739,   4738,   4737,   4736,   4735,   4734,   4733,   4732,   4731,   4730,   4729,   4728,   4727,   4726,   4725,   4724,   4723,   4722,   4721,   4720,   4719,   4718,   4717,   4716,   4715,   4714,
    4713,   4712,   4711,   4710,   4709,   4708,   4707,   4706,   4705,   4704,   4703,   4702,   4701,   4700,   4699,   4698,   4697,   4696,   4695,   4694,   4693,   4692,   4691,   4690,   4689,   4688,   4687,   4686,   4685,   4684,   4683,   4682,
    4681,   4680,   4679,   4678,   4677,   4676,   4675,   4674,   4673,   4672,   4671,   4670,   4669,   4668,   4667,   4666,   4665,   4664,   4663,   4662,   4661,   4660,   4659,   4658,   4657,   4656,   4655,   4654,   4653,   4652,   4651,   4650,
    4649,   4648,   4647,   4646,   4645,   4644,   4643,   4642,   4641,   4640,   4639,   4638,   4637,   4636,   4635,   4634,   4633,   4632,   4631,   4630,   4629,   4628,   4627,   4626,   4625,   4624,   4623,   4622,   4621,   4620,   4619,   4618,
    4617,   4616,   4615,   4614,   4613,   4612,   4611,   4610,   4609,   4608,   4607,   4606,   4605,   4604,   4603,   4602,   4601,   4600,   4599,   4598,   4597,   4596,   4595,   4594,   4593,   4592,   4591,   4590,   4589,   4588,   4587,   4586,
    4585,   4584,   4583,   4582,   4581,   4580,   4579,   4578,   4577,   4576,   4575,   4574,   4573,   4572,   4571,   4570,   4569,   4568,   4567,   4566,   4565,   4564,   4563,   4562,   4561,   4560,   4559,   4558,   4557,   4556,   4555,   4554,
    4553,   4552,   4551,   4550,   4549,   4548,   4547,   4546,   4545,   4544,   4543,   4542,   4541,   4540,   4539,   4538,   4537,   4536,   4535,   4534,   4533,   4532,   4531,   4530,   4529,   4528,   4527,   4526,   4525,   4524,   4523,   4522,
    4521,   4520,   4519,   4518,   4517,   4516,   4515,   4514,   4513,   4512,   4511,   4510,   4509,   4508,   4507,   4506,   4505,   4504,   4503,   4502,   4501,   4500,   4499,   4498,   4497,   4496,   4495,   4494,   4493,   4492,   4491,   4490,
    4096,   4097,   4098,   4099,   4100,   4101,   4102,   4103,   4104,   4105,   4106,   4107,   4108,   4109,   4110,   4111,   4110,   4111,   4112,   4113,   4114,   4115,   4116,   4117,   4118,   4119,   4120,   4121,   4122,   4123,   4124,   4125,
    4124,   4125,   4126,   4127,   4128,   4129,   4130,   4131,   4132,   4133,   4134,   4135,   4136,   4137,   4138,   4139,   4138,   4139,   4140,   4141,   4142,   4143,   4144,   4145,   4146,   4147,   4148,   4149,   4150,   4151,   4152,   4153,
    4150,   4151,   4152,   4153,   4154,   4155,   4156,   4157,   4158,   4159,   4160,   4161,   4162,   4163,   4164,   4165,   4164,   4165,   4166,   4167,   4168,   4169,   4170,   4171,   4172,   4173,   4174,   4175,   4176,   4177,   4178,   4179,
    4178,   4179,   4180,   4181,   4182,   4183,   4184,   4185,   4186,   4187,   4188,   4189,   4190,   4191,   4192,   4193,   4192,   4193,   4194,   4195,   4196,   4197,   4198,   4199,   4200,   4201,   4202,   4203,   4204,   4205,   4206,   4207,
    4204,   4205,   4206,   4207,   4208,   4209,   4210,   4211,   4212,   4213,   4214,   4215,   4216,   4217,   4218,   4219,   4218,   4219,   4220,   4221,   4222,   4223,   4224,   4225,   4226,   4227,   4228,   4229,   4230,   4231,   4232,   4233,
    4232,   4233,   4234,   4235,   4236,   4237,   4238,   4239,   4240,   4241,   4242,   4243,   4244,   4245,   4246,   4247,   4246,   4247,   4248,   4249,   4250,   4251,   4252,   4253,   4254,   4255,   4256,   4257,   4258,   4259,   4260,   4261,
    4258,   4259,   4260,   4261,   4262,   4263,   4264,   4265,   4266,   4267,   4268,   4269,   4270,   4271,   4272,   4273,   4272,   4273,   4274,   4275,   4276,   4277,   4278,   4279,   4280,   4281,   4282,   4283,   4284,   4285,   4286,   4287,
    4286,   4287,   4288,   4289,   4290,   4291,   4292,   4293,   4294,   4295,   4296,   4297,   4298,   4299,   4300,   4301,   4300,   4301,   4302,   4303,   4304,   4305,   4306,   4307,   4308,   4309,   4310,   4311,   4312,   4313,   4314,   4315,
    4312,   4313,   4314,   4315,   4316,   4317,   4318,   4319,   4320,   4321,   4322,   4323,   4324,   4325,   4326,   4327,   4326,   4327,   4328,   4329,   4330,   4331,   4332,   4333,   4334,   4335,   4336,   4337,   4338,   4339,   4340,   4341,
    4340,   4341,   4342,   4343,   4344,   4345,   4346,   4347,   4348,   4349,   4350,   4351,   4352,   4353,   4354,   4355,   4354,   4355,   4356,   4357,   4358,   4359,   4360,   4361,   4362,   4363,   4364,   4365,   4366,   4367,   4368,   4369,
    4366,   4367,   4368,   4369,   4370,   4371,   4372,   4373,   4374,   4375,   4376,   4377,   4378,   4379,   4380,   4381,   4380,   4381,   4382,   4383,   4384,   4385,   4386,   4387,   4388,   4389,   4390,   4391,   4392,   4393,   4394,   4395,
    4394,   4395,   4396,   4397,   4398,   4399,   4400,   4401,   4402,   4403,   4404,   4405,   4406,   4407,   4408,   4409,   4408,   4409,   4410,   4411,   4412,   4413,   4414,   4415,   4416,   4417,   4418,   4419,   4420,   4421,   4422,   4423,
    4420,   4421,   4422,   4423,   4424,   4425,   4426,   4427,   4428,   4429,   4430,   4431,   4432,   4433,   4434,   4435,   4434,   4435,   4436,   4437,   4438,   4439,   4440,   4441,   4442,   4443,   4444,   4445,   4446,   4447,   4448,   4449,
    4448,   4449,   4450,   4451,   4452,   4453,   4454,   4455,   4456,   4457,   4458,   4459,   4460,   4461,   4462,   4463,   4462,   4463,   4464,   4465,   4466,   4467,   4468,   4469,   4470,   4471,   4472,   4473,   4474,   4475,   4476,   4477,
    4474,   4475,   4476,   4477,   4478,   4479,   4480,   4481,   4482,   4483,   4484,   4485,   4486,   4487,   4488,   4489,   4488,   4489,   4490,   4491,   4492,   4493,   4494,   4495,   4496,   4497,   4498,   4499,   4500,   4501,   4502,   4503,
    4502,   4503,   4504,   4505,   4506,   4507,   4508,   4509,   4510,   4511,   4512,   4513,   4514,   4515,   4516,   4517,   4516,   4517,   4518,   4519,   4520,   4521,   4522,   4523,   4524,   4525,   4526,   4527,   4528,   4529,   4530,   4531,
    4528,   4529,   4530,   4531,   4532,   4533,   4534,   4535,   4536,   4537,   4538,   4539,   4540,   4541,   4542,   4543,   4542,   4543,   4544,   4545,   4546,   4547,   4548,   4549,   4550,   4551,   4552,   4553,   4554,   4555,   4556,   4557,
    4556,   4557,   4558,   4559,   4560,   4561,   4562,   4563,   4564,   4565,   4566,   4567,   4568,   4569,   4570,   4571,   4570,   4571,   4572,   4573,   4574,   4575,   4576,   4577,   4578,   4579,   4580,   4581,   4582,   4583,   4584,   4585,
    4582,   4583,   4584,   4585,   4586,   4587,   4588,   4589,   4590,   4591,   4592,   4593,   4594,   4595,   4596,   4597,   4596,   4597,   4598,   4599,   4600,   4601,   4602,   4603,   4604,   4605,   4606,   4607,   4608,   4609,   4610,   4611,
    4610,   4611,   4612,   4613,   4614,   4615,   4616,   4617,   4618,   4619,   4620,   4621,   4622,   4623,   4624,   4625,   4624,   4625,   4626,   4627,   4628,   4629,   4630,   4631,   4632,   4633,   4634,   4635,   4636,   4637,   4638,   4639,
    4636,   4637,   4638,   4639,   4640,   4641,   4642,   4643,   4644,   4645,   4646,   4647,   4648,   4649,   4650,   4651,   4650,   4651,   4652,   4653,   4654,   4655,   4656,   4657,   4658,   4659,   4660,   4661,   4662,   4663,   4664,   4665,
    4664,   4665,   4666,   4667,   4668,   4669,   4670,   4671,   4672,   4673,   4674,   4675,   4676,   4677,   4678,   4679,   4678,   4679,   4680,   4681,   4682,   4683,   4684,   4685,   4686,   4687,   4688,   4689,   4690,   4691,   4692,   4693,
    4690,   4691,   4692,   4693,   4694,   4695,   4696,   4697,   4698,   4699,   4700,   4701,   4702,   4703,   4704,   4705,   4704,   4705,   4706,   4707,   4708,   4709,   4710,   4711,   4712,   4713,   4714,   4715,   4716,   4717,   4718,   4719,
    4718,   4719,   4720,   4721,   4722,   4723,   4724,   4725,   4726,   4727,   4728,   4729,   4730,   4731,   4732,   4733,   4732,   4733,   4734,   4735,   4736,   4737,   4738,   4739,   4740,   4741,   4742,   4743,   4744,   4745,   4746,   4747,
    4744,   4745,   4746,   4747,   4748,   4749,   4750,   4751,   4752,   4753,   4754,   4755,   4756,   4757,   4758,   4759,   4758,   4759,   4760,   4761,   4762,   4763,   4764,   4765,   4766,   4767,   4768,   4769,   4770,   4771,   4772,   4773,
    4772,   4773,   4774,   4775,   4776,   4777,   4778,   4779,   4780,   4781,   4782,   4783,   4784,   4785,   4786,   4787,   4786,   4787,   4788,   4789,   4790,   4791,   4792,   4793,   4794,   4795,   4796,   4797,   4798,   4799,   4800,   4801,
    4798,   4799,   4800,   4801,   4802,   4803,   4804,   4805,   4806,   4807,   4808,   4809,   4810,   4811,   4812,   4813,   4812,   4813,   4814,   4815,   4816,   4817,   4818,   4819,   4820,   4821,   4822,   4823,   4824,   4825,   4826,   4827,
    4826,   4827,   4828,   4829,   4830,   4831,   4832,   4833,   4834,   4835,   4836,   4837,   4838,   4839,   4840,   4841,   4840,   4841,   4842,   4843,   4844,   4845,   4846,   4847,   4848,   4849,   4850,   4851,   4852,   4853,   4854,   4855,
    4852,   4853,   4854,   4855,   4856,   4857,   4858,   4859,   4860,   4861,   4862,   4863,   4864,   4865,   4866,   4867,   4866,   4867,   4868,   4869,   4870,   4871,   4872,   4873,   4874,   4875,   4876,   4877,   4878,   4879,   4880,   4881,
    4880,   4881,   4882,   4883,   4884,   4885,   4886,   4887,   4888,   4889,   4890,   4891,   4892,   4893,   4894,   4895,   4894,   4895,   4896,   4897,   4898,   4899,   4900,   4901,   4902,   4903,   4904,   4905,   4906,   4907,   4908,   4909,
    4906,   4907,   4908,   4909,   4910,   4911,   4912,   4913,   4914,   4915,   4916,   4917,   4918,   4919,   4920,   4921,   4920,   4921,   4922,   4923,   4924,   4925,   4926,   4927,   4928,   4929,   4930,   4931,   4932,   4933,   4934,   4935,
    4934,   4935,   4936,   4937,   4938,   4939,   4940,   4941,   4942,   4943,   4944,   4945,   4946,   4947,   4948,   4949,   4948,   4949,   4950,   4951,   4952,   4953,   4954,   4955,   4956,   4957,   4958,   4959,   4960,   4961,   4962,   4963,
    4960,   4961,   4962,   4963,   4964,   4965,   4966,   4967,   4968,   4969,   4970,   4971,   4972,   4973,   4974,   4975,   4974,   4975,   4976,   4977,   4978,   4979,   4980,   4981,   4982,   4983,   4984,   4985,   4986,   4987,   4988,   4989,
    4988,   4989,   4990,   4991,   4992,   4993,   4994,   4995,   4996,   4997,   4998,   4999,   5000,   5001,   5002,   5003,   5002,   5003,   5004,   5005,   5006,   5007,   5008,   5009,   5010,   5011,   5012,   5013,   5014,   5015,   5016,   5017,
    5014,   5015,   5016,   5017,   5018,   5019,   5020,   5021,   5022,   5023,   5024,   5025,   5026,   5027,   5028,   5029,   5028,   5029,   5030,   5031,   5032,   5033,   5034,   5035,   5036,   5037,   5038,   5039,   5040,   5041,   5042,   5043,
    5042,   5043,   5044,   5045,   5046,   5047,   5048,   5049,   5050,   5051,   5052,   5053,   5054,   5055,   5056,   5057,   5056,   5057,   5058,   5059,   5060,   5061,   5062,   5063,   5064,   5065,   5066,   5067,   5068,   5069,   5070,   5071,
    5068,   5069,   5070,   5071,   5072,   5073,   5074,   5075,   5076,   5077,   5078,   5079,   5080,   5081,   5082,   5083,   5082,   5083,   5084,   5085,   5086,   5087,   5088,   5089,   5090,   5091,   5092,   5093,   5094,   5095,   5096,   5097,
    5096,   5097,   5098,   5099,   5100,   5101,   5102,   5103,   5104,   5105,   5106,   5107,   5108,   5109,   5110,   5111,   5110,   5111,   5112,   5113,   5114,   5115,   5116,   5117,   5118,   5119,   5120,   5121,   5122,   5123,   5124,   5125,
    5122,   5123,   5124,   5125,   5126,   5127,   5128,   5129,   5130,   5131,   5132,   5133,   5134,   5135,   5136,   5137,   5136,   5137,   5138,   5139,   5140,   5141,   5142,   5143,   5144,   5145,   5146,   5147,   5148,   5149,   5150,   5151,
    5150,   5151,   5152,   5153,   5154,   5155,   5156,   5157,   5158,   5159,   5160,   5161,   5162,   5163,   5164,   5165,   5164,   5165,   5166,   5167,   5168,   5169,   5170,   5171,   5172,   5173,   5174,   5175,   5176,   5177,   5178,   5179,
    5176,   5177,   5178,   5179,   5180,   5181,   5182,   5183,   5184,   5185,   5186,   5187,   5188,   5189,   5190,   5191,   5190,   5191,   5192,   5193,   5194,   5195,   5196,   5197,   5198,   5199,   5200,   5201,   5202,   5203,   5204,   5205,
    5204,   5205,   5206,   5207,   5208,   5209,   5210,   5211,   5212,   5213,   5214,   5215,   5216,   5217,   5218,   5219,   5218,   5219,   5220,   5221,   5222,   5223,   5224,   5225,   5226,   5227,   5228,   5229,   5230,   5231,   5232,   5233,
    5230,   5231,   5232,   5233,   5234,   5235,   5236,   5237,   5238,   5239,   5240,   5241,   5242,   5243,   5244,   5245,   5244,   5245,   5246,   5247,   5248,   5249,   5250,   5251,   5252,   5253,   5254,   5255,   5256,   5257,   5258,   5259,
    5258,   5259,   5260,   5261,   5262,   5263,   5264,   5265,   5266,   5267,   5268,   5269,   5270,   5271,   5272,   5273,   5272,   5273,   5274,   5275,   5276,   5277,   5278,   5279,   5280,   5281,   5282,   5283,   5284,   5285,   5286,   5287,
    5284,   5285,   5286,   5287,   5288,   5289,   5290,   5291,   5292,   5293,   5294,   5295,   5296,   5297,   5298,   5299,   5298,   5299,   5300,   5301,   5302,   5303,   5304,   5305,   5306,   5307,   5308,   5309,   5310,   5311,   5312,   5313,
    5312,   5313,   5314,   5315,   5316,   5317,   5318,   5319,   5320,   5321,   5322,   5323,   5324,   5325,   5326,   5327,   5326,   5327,   5328,   5329,   5330,   5331,   5332,   5333,   5334,   5335,   5336,   5337,   5338,   5339,   5340,   5341,
    5338,   5339,   5340,   5341,   5342,   5343,   5344,   5345,   5346,   5347,   5348,   5349,   5350,   5351,   5352,   5353,   5352,   5353,   5354,   5355,   5356,   5357,   5358,   5359,   5360,   5361,   5362,   5363,   5364,   5365,   5366,   5367,
    5366,   5367,   5368,   5369,   5370,   5371,   5372,   5373,   5374,   5375,   5376,   5377,   5378,   5379,   5380,   5381,   5380,   5381,   5382,   5383,   5384,   5385,   5386,   5387,   5388,   5389,   5390,   5391,   5392,   5393,   5394,   5395,
    5392,   5393,   5394,   5395,   5396,   5397,   5398,   5399,   5400,   5401,   5402,   5403,   5404,   5405,   5406,   5407,   5406,   5407,   5408,   5409,   5410,   5411,   5412,   5413,   5414,   5415,   5416,   5417,   5418,   5419,   5420,   5421,
    5420,   5421,   5422,   5423,   5424,   5425,   5426,   5427,   5428,   5429,   5430,   5431,   5432,   5433,   5434,   5435,   5434,   5435,   5436,   5437,   5438,   5439,   5440,   5441,   5442,   5443,   5444,   5445,   5446,   5447,   5448,   5449,
    5446,   5447,   5448,   5449,   5450,   5451,   5452,   5453,   5454,   5455,   5456,   5457,   5458,   5459,   5460,   5461,   5460,   5461,   5462,   5463,   5464,   5465,   5466,   5467,   5468,   5469,   5470,   5471,   5472,   5473,   5474,   5475,
    5474,   5475,   5476,   5477,   5478,   5479,   5480,   5481,   5482,   5483,   5484,   5485,   5486,   5487,   5488,   5489,   5488,   5489,   5490,   5491,   5492,   5493,   5494,   5495,   5496,   5497,   5498,   5499,   5500,   5501,   5502,   5503,
    5500,   5501,   5502,   5503,   5504,   5505,   5506,   5507,   5508,   5509,   5510,   5511,   5512,   5513,   5514,   5515,   5514,   5515,   5516,   5517,   5518,   5519,   5520,   5521,   5522,   5523,   5524,   5525,   5526,   5527,   5528,   5529,
    5528,   5529,   5530,   5531,   5532,   5533,   5534,   5535,   5536,   5537,   5538,   5539,   5540,   5541,   5542,   5543,   5542,   5543,   5544,   5545,   5546,   5547,   5548,   5549,   5550,   5551,   5552,   5553,   5554,   5555,   5556,   5557,
    5554,   5555,   5556,   5557,   5558,   5559,   5560,   5561,   5562,   5563,   5564,   5565,   5566,   5567,   5568,   5569,   5568,   5569,   5570,   5571,   5572,   5573,   5574,   5575,   5576,   5577,   5578,   5579,   5580,   5581,   5582,   5583,
    5582,   5583,   5584,   5585,   5586,   5587,   5588,   5589,   5590,   5591,   5592,   5593,   5594,   5595,   5596,   5597,   5596,   5597,   5598,   5599,   5600,   5601,   5602,   5603,   5604,   5605,   5606,   5607,   5608,   5609,   5610,   5611,
    5608,   5609,   5610,   5611,   5612,   5613,   5614,   5615,   5616,   5617,   5618,   5619,   5620,   5621,   5622,   5623,   5622,   5623,   5624,   5625,   5626,   5627,   5628,   5629,   5630,   5631,   5632,   5633,   5634,   5635,   5636,   5637,
    5636,   5637,   5638,   5639,   5640,   5641,   5642,   5643,   5644,   5645,   5646,   5647,   5648,   5649,   5650,   5651,   5650,   5651,   5652,   5653,   5654,   5655,   5656,   5657,   5658,   5659,   5660,   5661,   5662,   5663,   5664,   5665,
    5662,   5663,   5664,   5665,   5666,   5667,   5668,   5669,   5670,   5671,   5672,   5673,   5674,   5675,   5676,   5677,   5676,   5677,   5678,   5679,   5680,   5681,   5682,   5683,   5684,   5685,   5686,   5687,   5688,   5689,   5690,   5691,
    5690,   5691,   5692,   5693,   5694,   5695,   5696,   5697,   5698,   5699,   5700,   5701,   5702,   5703,   5704,   5705,   5704,   5705,   5706,   5707,   5708,   5709,   5710,   5711,   5712,   5713,   5714,   5715,   5716,   5717,   5718,   5719,
    5716,   5717,   5718,   5719,   5720,   5721,   5722,   5723,   5724,   5725,   5726,   5727,   5728,   5729,   5730,   5731,   5730,   5731,   5732,   5733,   5734,   5735,   5736,   5737,   5738,   5739,   5740,   5741,   5742,   5743,   5744,   5745,
    5744,   5745,   5746,   5747,   5748,   5749,   5750,   5751,   5752,   5753,   5754,   5755,   5756,   5757,   5758,   5759,   5758,   5759,   5760,   5761,   5762,   5763,   5764,   5765,   5766,   5767,   5768,   5769,   5770,   5771,   5772,   5773,
    5770,   5771,   5772,   5773,   5774,   5775,   5776,   5777,   5778,   5779,   5780,   5781,   5782,   5783,   5784,   5785,   5784,   5785,   5786,   5787,   5788,   5789,   5790,   5791,   5792,   5793,   5794,   5795,   5796,   5797,   5798,   5799,
    5798,   5799,   5800,   5801,   5802,   5803,   5804,   5805,   5806,   5807,   5808,   5809,   5810,   5811,   5812,   5813,   5812,   5813,   5814,   5815,   5816,   5817,   5818,   5819,   5820,   5821,   5822,   5823,   5824,   5825,   5826,   5827,

    9846,   9847,   9848,   9849,   9850,   9851,   9852,   9853,   9854,   9855,   9856,   9857,   9858,   9859,   9860,   9861,   9862,   9863,   9864,   9865,   9866,   9867,   9868,   9869,   9870,   9871,   9872,   9873,   9874,   9875,   9876,   9877,
    9878,   9879,   9880,   9881,   9882,   9883,   9884,   9885,   9886,   9887,   9888,   9889,   9890,   9891,   9892,   9893,   9894,   9895,   9896,   9897,   9898,   9899,   9900,   9901,   9902,   9903,   9904,   9905,   9906,   9907,   9908,   9909,
    9910,   9911,   9912,   9913,   9914,   9915,   9916,   9917,   9918,   9919,   9920,   9921,   9922,   9923,   9924,   9925,   9926,   9927,   9928,   9929,   9930,   9931,   9932,   9933,   9934,   9935,   9936,   9937,   9938,   9939,   9940,   9941,
    9942,   9943,   9944,   9945,   9946,   9947,   9948,   9949,   9950,   9951,   9952,   9953,   9954,   9955,   9956,   9957,   9958,   9959,   9960,   9961,   9962,   9963,   9964,   9965,   9966,   9967,   9968,   9969,   9970,   9971,   9972,   9973,
    9974,   9975,   9976,   9977,   9978,   9979,   9980,   9981,   9982,   9983,   9984,   9985,   9986,   9987,   9988,   9989,   9990,   9991,   9992,   9993,   9994,   9995,   9996,   9997,   9998,   9999,  10000,  10001,  10002,  10003,  10004,  10005,
    10006,  10007,  10008,  10009,  10010,  10011,  10012,  10013,  10014,  10015,  10016,  10017,  10018,  10019,  10020,  10021,  10022,  10023,  10024,  10025,  10026,  10027,  10028,  10029,  10030,  10031,  10032,  10033,  10034,  10035,  10036,  10037,
    10038,  10039,  10040,  10041,  10042,  10043,  10044,  10045,  10046,  10047,  10048,  10049,  10050,  10051,  10052,  10053,  10054,  10055,  10056,  10057,  10058,  10059,  10060,  10061,  10062,  10063,  10064,  10065,  10066,  10067,  10068,  10069,
    10070,  10071,  10072,  10073,  10074,  10075,  10076,  10077,  10078,  10079,  10080,  10081,  10082,  10083,  10084,  10085,  10086,  10087,  10088,  10089,  10090,  10091,  10092,  10093,  10094,  10095,  10096,  10097,  10098,  10099,  10100,  10101,
    10102,  10103,  10104,  10105,  10106,  10107,  10108,  10109,  10110,  10111,  10112,  10113,  10114,  10115,  10116,  10117,  10118,  10119,  10120,  10121,  10122,  10123,  10124,  10125,  10126,  10127,  10128,  10129,  10130,  10131,  10132,  10133,
    10134,  10135,  10136,  10137,  10138,  10139,  10140,  10141,  10142,  10143,  10144,  10145,  10146,  10147,  10148,  10149,  10150,  10151,  10152,  10153,  10154,  10155,  10156,  10157,  10158,  10159,  10160,  10161,  10162,  10163,  10164,  10165,
    10166,  10167,  10168,  10169,  10170,  10171,  10172,  10173,  10174,  10175,  10176,  10177,  10178,  10179,  10180,  10181,  10182,  10183,  10184,  10185,  10186,  10187,  10188,  10189,  10190,  10191,  10192,  10193,  10194,  10195,  10196,  10197,
    10198,  10199,  10200,  10201,  10202,  10203,  10204,  10205,  10206,  10207,  10208,  10209,  10210,  10211,  10212,  10213,  10214,  10215,  10216,  10217,  10218,  10219,  10220,  10221,  10222,  10223,  10224,  10225,  10226,  10227,  10228,  10229,
    10230,  10231,  10232,  10233,  10234,  10235,  10236,  10237,  10238,  10239,  10239,  10238,  10237,  10236,  10235,  10234,  10233,  10232,  10231,  10230,  10229,  10228,  10227,  10226,  10225,  10224,  10223,  10222,  10221,  10220,  10219,  10218,
    10217,  10216,  10215,  10214,  10213,  10212,  10211,  10210,  10209,  10208,  10207,  10206,  10205,  10204,  10203,  10202,  10201,  10200,  10199,  10198,  10197,  10196,  10195,  10194,  10193,  10192,  10191,  10190,  10189,  10188,  10187,  10186,
    10185,  10184,  10183,  10182,  10181,  10180,  10179,  10178,  10177,  10176,  10175,  10174,  10173,  10172,  10171,  10170,  10169,  10168,  10167,  10166,  10165,  10164,  10163,  10162,  10161,  10160,  10159,  10158,  10157,  10156,  10155,  10154,
    10153,  10152,  10151,  10150,  10149,  10148,  10147,  10146,  10145,  10144,  10143,  10142,  10141,  10140,  10139,  10138,  10137,  10136,  10135,  10134,  10133,  10132,  10131,  10130,  10129,  10128,  10127,  10126,  10125,  10124,  10123,  10122,
    10121,  10120,  10119,  10118,  10117,  10116,  10115,  10114,  10113,  10112,  10111,  10110,  10109,  10108,  10107,  10106,  10105,  10104,  10103,  10102,  10101,  10100,  10099,  10098,  10097,  10096,  10095,  10094,  10093,  10092,  10091,  10090,
    10089,  10088,  10087,  10086,  10085,  10084,  10083,  10082,  10081,  10080,  10079,  10078,  10077,  10076,  10075,  10074,  10073,  10072,  10071,  10070,  10069,  10068,  10067,  10066,  10065,  10064,  10063,  10062,  10061,  10060,  10059,  10058,
    10057,  10056,  10055,  10054,  10053,  10052,  10051,  10050,  10049,  10048,  10047,  10046,  10045,  10044,  10043,  10042,  10041,  10040,  10039,  10038,  10037,  10036,  10035,  10034,  10033,  10032,  10031,  10030,  10029,  10028,  10027,  10026,
    10025,  10024,  10023,  10022,  10021,  10020,  10019,  10018,  10017,  10016,  10015,  10014,  10013,  10012,  10011,  10010,  10009,  10008,  10007,  10006,  10005,  10004,  10003,  10002,  10001,  10000,   9999,   9998,   9997,   9996,   9995,   9994,
    9993,   9992,   9991,   9990,   9989,   9988,   9987,   9986,   9985,   9984,   9983,   9982,   9981,   9980,   9979,   9978,   9977,   9976,   9975,   9974,   9973,   9972,   9971,   9970,   9969,   9968,   9967,   9966,   9965,   9964,   9963,   9962,
    9961,   9960,   9959,   9958,   9957,   9956,   9955,   9954,   9953,   9952,   9951,   9950,   9949,   9948,   9947,   9946,   9945,   9944,   9943,   9942,   9941,   9940,   9939,   9938,   9937,   9936,   9935,   9934,   9933,   9932,   9931,   9930,
    9929,   9928,   9927,   9926,   9925,   9924,   9923,   9922,   9921,   9920,   9919,   9918,   9917,   9916,   9915,   9914,   9913,   9912,   9911,   9910,   9909,   9908,   9907,   9906,   9905,   9904,   9903,   9902,   9901,   9900,   9899,   9898,
    9897,   9896,   9895,   9894,   9893,   9892,   9891,   9890,   9889,   9888,   9887,   9886,   9885,   9884,   9883,   9882,   9881,   9880,   9879,   9878,   9877,   9876,   9875,   9874,   9873,   9872,   9871,   9870,   9869,   9868,   9867,   9866,
    9865,   9864,   9863,   9862,   9861,   9860,   9859,   9858,   9857,   9856,   9855,   9854,   9853,   9852,   9851,   9850,   9849,   9848,   9847,   9846,   9845,   9844,   9843,   9842,   9841,   9840,   9839,   9838,   9837,   9836,   9835,   9834,
    9833,   9832,   9831,   9830,   9829,   9828,   9827,   9826,   9825,   9824,   9823,   9822,   9821,   9820,   9819,   9818,   9817,   9816,   9815,   9814,   9813,   9812,   9811,   9810,   9809,   9808,   9807,   9806,   9805,   9804,   9803,   9802,
    9801,   9800,   9799,   9798,   9797,   9796,   9795,   9794,   9793,   9792,   9791,   9790,   9789,   9788,   9787,   9786,   9785,   9784,   9783,   9782,   9781,   9780,   9779,   9778,   9777,   9776,   9775,   9774,   9773,   9772,   9771,   9770,
    9769,   9768,   9767,   9766,   9765,   9764,   9763,   9762,   9761,   9760,   9759,   9758,   9757,   9756,   9755,   9754,   9753,   9752,   9751,   9750,   9749,   9748,   9747,   9746,   9745,   9744,   9743,   9742,   9741,   9740,   9739,   9738,
    9737,   9736,   9735,   9734,   9733,   9732,   9731,   9730,   9729,   9728,   9727,   9726,   9725,   9724,   9723,   9722,   9721,   9720,   9719,   9718,   9717,   9716,   9715,   9714,   9713,   9712,   9711,   9710,   9709,   9708,   9707,   9706,
    9705,   9704,   9703,   9702,   9701,   9700,   9699,   9698,   9697,   9696,   9695,   9694,   9693,   9692,   9691,   9690,   9689,   9688,   9687,   9686,   9685,   9684,   9683,   9682,   9681,   9680,   9679,   9678,   9677,   9676,   9675,   9674,
    9673,   9672,   9671,   9670,   9669,   9668,   9667,   9666,   9665,   9664,   9663,   9662,   9661,   9660,   9659,   9658,   9657,   9656,   9655,   9654,   9653,   9652,   9651,   9650,   9649,   9648,   9647,   9646,   9645,   9644,   9643,   9642,
    9641,   9640,   9639,   9638,   9637,   9636,   9635,   9634,   9633,   9632,   9631,   9630,   9629,   9628,   9627,   9626,   9625,   9624,   9623,   9622,   9621,   9620,   9619,   9618,   9617,   9616,   9615,   9614,   9613,   9612,   9611,   9610,
    9609,   9608,   9607,   9606,   9605,   9604,   9603,   9602,   9601,   9600,   9599,   9598,   9597,   9596,   9595,   9594,   9593,   9592,   9591,   9590,   9589,   9588,   9587,   9586,   9585,   9584,   9583,   9582,   9581,   9580,   9579,   9578,
    9577,   9576,   9575,   9574,   9573,   9572,   9571,   9570,   9569,   9568,   9567,   9566,   9565,   9564,   9563,   9562,   9561,   9560,   9559,   9558,   9557,   9556,   9555,   9554,   9553,   9552,   9551,   9550,   9549,   9548,   9547,   9546,
    9545,   9544,   9543,   9542,   9541,   9540,   9539,   9538,   9537,   9536,   9535,   9534,   9533,   9532,   9531,   9530,   9529,   9528,   9527,   9526,   9525,   9524,   9523,   9522,   9521,   9520,   9519,   9518,   9517,   9516,   9515,   9514,
    9513,   9512,   9511,   9510,   9509,   9508,   9507,   9506,   9505,   9504,   9503,   9502,   9501,   9500,   9499,   9498,   9497,   9496,   9495,   9494,   9493,   9492,   9491,   9490,   9489,   9488,   9487,   9486,   9485,   9484,   9483,   9482,
    9481,   9480,   9479,   9478,   9477,   9476,   9475,   9474,   9473,   9472,   9471,   9470,   9469,   9468,   9467,   9466,   9465,   9464,   9463,   9462,   9461,   9460,   9459,   9458,   9457,   9456,   9455,   9454,   9453,   9452,   9451,   9450,
    9449,   9448,   9447,   9446,   9445,   9444,   9443,   9442,   9441,   9440,   9439,   9438,   9437,   9436,   9435,   9434,   9433,   9432,   9431,   9430,   9429,   9428,   9427,   9426,   9425,   9424,   9423,   9422,   9421,   9420,   9419,   9418,
    9417,   9416,   9415,   9414,   9413,   9412,   9411,   9410,   9409,   9408,   9407,   9406,   9405,   9404,   9403,   9402,   9401,   9400,   9399,   9398,   9397,   9396,   9395,   9394,   9393,   9392,   9391,   9390,   9389,   9388,   9387,   9386,
    9385,   9384,   9383,   9382,   9381,   9380,   9379,   9378,   9377,   9376,   9375,   9374,   9373,   9372,   9371,   9370,   9369,   9368,   9367,   9366,   9365,   9364,   9363,   9362,   9361,   9360,   9359,   9358,   9357,   9356,   9355,   9354,
    9353,   9352,   9351,   9350,   9349,   9348,   9347,   9346,   9345,   9344,   9343,   9342,   9341,   9340,   9339,   9338,   9337,   9336,   9335,   9334,   9333,   9332,   9331,   9330,   9329,   9328,   9327,   9326,   9325,   9324,   9323,   9322,
    9321,   9320,   9319,   9318,   9317,   9316,   9315,   9314,   9313,   9312,   9311,   9310,   9309,   9308,   9307,   9306,   9305,   9304,   9303,   9302,   9301,   9300,   9299,   9298,   9297,   9296,   9295,   9294,   9293,   9292,   9291,   9290,
    9289,   9288,   9287,   9286,   9285,   9284,   9283,   9282,   9281,   9280,   9279,   9278,   9277,   9276,   9275,   9274,   9273,   9272,   9271,   9270,   9269,   9268,   9267,   9266,   9265,   9264,   9263,   9262,   9261,   9260,   9259,   9258,
    9257,   9256,   9255,   9254,   9253,   9252,   9251,   9250,   9249,   9248,   9247,   9246,   9245,   9244,   9243,   9242,   9241,   9240,   9239,   9238,   9237,   9236,   9235,   9234,   9233,   9232,   9231,   9230,   9229,   9228,   9227,   9226,
    9225,   9224,   9223,   9222,   9221,   9220,   9219,   9218,   9217,   9216,   9215,   9214,   9213,   9212,   9211,   9210,   9209,   9208,   9207,   9206,   9205,   9204,   9203,   9202,   9201,   9200,   9199,   9198,   9197,   9196,   9195,   9194,
    9193,   9192,   9191,   9190,   9189,   9188,   9187,   9186,   9185,   9184,   9183,   9182,   9181,   9180,   9179,   9178,   9177,   9176,   9175,   9174,   9173,   9172,   9171,   9170,   9169,   9168,   9167,   9166,   9165,   9164,   9163,   9162,
    9161,   9160,   9159,   9158,   9157,   9156,   9155,   9154,   9153,   9152,   9151,   9150,   9149,   9148,   9147,   9146,   9145,   9144,   9143,   9142,   9141,   9140,   9139,   9138,   9137,   9136,   9135,   9134,   9133,   9132,   9131,   9130,
    9129,   9128,   9127,   9126,   9125,   9124,   9123,   9122,   9121,   9120,   9119,   9118,   9117,   9116,   9115,   9114,   9113,   9112,   9111,   9110,   9109,   9108,   9107,   9106,   9105,   9104,   9103,   9102,   9101,   9100,   9099,   9098,
    9097,   9096,   9095,   9094,   9093,   9092,   9091,   9090,   9089,   9088,   9087,   9086,   9085,   9084,   9083,   9082,   9081,   9080,   9079,   9078,   9077,   9076,   9075,   9074,   9073,   9072,   9071,   9070,   9069,   9068,   9067,   9066,
    9065,   9064,   9063,   9062,   9061,   9060,   9059,   9058,   9057,   9056,   9055,   9054,   9053,   9052,   9051,   9050,   9049,   9048,   9047,   9046,   9045,   9044,   9043,   9042,   9041,   9040,   9039,   9038,   9037,   9036,   9035,   9034,
    9033,   9032,   9031,   9030,   9029,   9028,   9027,   9026,   9025,   9024,   9023,   9022,   9021,   9020,   9019,   9018,   9017,   9016,   9015,   9014,   9013,   9012,   9011,   9010,   9009,   9008,   9007,   9006,   9005,   9004,   9003,   9002,
    9001,   9000,   8999,   8998,   8997,   8996,   8995,   8994,   8993,   8992,   8991,   8990,   8989,   8988,   8987,   8986,   8985,   8984,   8983,   8982,   8981,   8980,   8979,   8978,   8977,   8976,   8975,   8974,   8973,   8972,   8971,   8970,
    8969,   8968,   8967,   8966,   8965,   8964,   8963,   8962,   8961,   8960,   8959,   8958,   8957,   8956,   8955,   8954,   8953,   8952,   8951,   8950,   8949,   8948,   8947,   8946,   8945,   8944,   8943,   8942,   8941,   8940,   8939,   8938,
    8937,   8936,   8935,   8934,   8933,   8932,   8931,   8930,   8929,   8928,   8927,   8926,   8925,   8924,   8923,   8922,   8921,   8920,   8919,   8918,   8917,   8916,   8915,   8914,   8913,   8912,   8911,   8910,   8909,   8908,   8907,   8906,
    8905,   8904,   8903,   8902,   8901,   8900,   8899,   8898,   8897,   8896,   8895,   8894,   8893,   8892,   8891,   8890,   8889,   8888,   8887,   8886,   8885,   8884,   8883,   8882,   8881,   8880,   8879,   8878,   8877,   8876,   8875,   8874,
    8873,   8872,   8871,   8870,   8869,   8868,   8867,   8866,   8865,   8864,   8863,   8862,   8861,   8860,   8859,   8858,   8857,   8856,   8855,   8854,   8853,   8852,   8851,   8850,   8849,   8848,   8847,   8846,   8845,   8844,   8843,   8842,
    8841,   8840,   8839,   8838,   8837,   8836,   8835,   8834,   8833,   8832,   8831,   8830,   8829,   8828,   8827,   8826,   8825,   8824,   8823,   8822,   8821,   8820,   8819,   8818,   8817,   8816,   8815,   8814,   8813,   8812,   8811,   8810,
    8809,   8808,   8807,   8806,   8805,   8804,   8803,   8802,   8801,   8800,   8799,   8798,   8797,   8796,   8795,   8794,   8793,   8792,   8791,   8790,   8789,   8788,   8787,   8786,   8785,   8784,   8783,   8782,   8781,   8780,   8779,   8778,
    8777,   8776,   8775,   8774,   8773,   8772,   8771,   8770,   8769,   8768,   8767,   8766,   8765,   8764,   8763,   8762,   8761,   8760,   8759,   8758,   8757,   8756,   8755,   8754,   8753,   8752,   8751,   8750,   8749,   8748,   8747,   8746,
    8745,   8744,   8743,   8742,   8741,   8740,   8739,   8738,   8737,   8736,   8735,   8734,   8733,   8732,   8731,   8730,   8729,   8728,   8727,   8726,   8725,   8724,   8723,   8722,   8721,   8720,   8719,   8718,   8717,   8716,   8715,   8714,
    8713,   8712,   8711,   8710,   8709,   8708,   8707,   8706,   8705,   8704,   8703,   8702,   8701,   8700,   8699,   8698,   8697,   8696,   8695,   8694,   8693,   8692,   8691,   8690,   8689,   8688,   8687,   8686,   8685,   8684,   8683,   8682,
    8681,   8680,   8679,   8678,   8677,   8676,   8675,   8674,   8673,   8672,   8671,   8670,   8669,   8668,   8667,   8666,   8665,   8664,   8663,   8662,   8661,   8660,   8659,   8658,   8657,   8656,   8655,   8654,   8653,   8652,   8651,   8650,
    8649,   8648,   8647,   8646,   8645,   8644,   8643,   8642,   8641,   8640,   8639,   8638,   8637,   8636,   8635,   8634,   8633,   8632,   8631,   8630,   8629,   8628,   8627,   8626,   8625,   8624,   8623,   8622,   8621,   8620,   8619,   8618,
    8617,   8616,   8615,   8614,   8613,   8612,   8611,   8610,   8609,   8608,   8607,   8606,   8605,   8604,   8603,   8602,   8601,   8600,   8599,   8598,   8597,   8596,   8595,   8594,   8593,   8592,   8591,   8590,   8589,   8588,   8587,   8586,
    4096,   4097,   4098,   4099,   4100,   4101,   4102,   4103,   4104,   4105,   4106,   4107,   4108,   4109,   4110,   4111,   4110,   4111,   4112,   4113,   4114,   4115,   4116,   4117,   4118,   4119,   4120,   4121,   4122,   4123,   4124,   4125,
    4124,   4125,   4126,   4127,   4128,   4129,   4130,   4131,   4132,   4133,   4134,   4135,   4136,   4137,   4138,   4139,   4138,   4139,   4140,   4141,   4142,   4143,   4144,   4145,   4146,   4147,   4148,   4149,   4150,   4151,   4152,   4153,
    4150,   4151,   4152,   4153,   4154,   4155,   4156,   4157,   4158,   4159,   4160,   4161,   4162,   4163,   4164,   4165,   4164,   4165,   4166,   4167,   4168,   4169,   4170,   4171,   4172,   4173,   4174,   4175,   4176,   4177,   4178,   4179,
    4178,   4179,   4180,   4181,   4182,   4183,   4184,   4185,   4186,   4187,   4188,   4189,   4190,   4191,   4192,   4193,   4192,   4193,   4194,   4195,   4196,   4197,   4198,   4199,   4200,   4201,   4202,   4203,   4204,   4205,   4206,   4207,
    4204,   4205,   4206,   4207,   4208,   4209,   4210,   4211,   4212,   4213,   4214,   4215,   4216,   4217,   4218,   4219,   4218,   4219,   4220,   4221,   4222,   4223,   4224,   4225,   4226,   4227,   4228,   4229,   4230,   4231,   4232,   4233,
    4232,   4233,   4234,   4235,   4236,   4237,   4238,   4239,   4240,   4241,   4242,   4243,   4244,   4245,   4246,   4247,   4246,   4247,   4248,   4249,   4250,   4251,   4252,   4253,   4254,   4255,   4256,   4257,   4258,   4259,   4260,   4261,
    4258,   4259,   4260,   4261,   4262,   4263,   4264,   4265,   4266,   4267,   4268,   4269,   4270,   4271,   4272,   4273,   4272,   4273,   4274,   4275,   4276,   4277,   4278,   4279,   4280,   4281,   4282,   4283,   4284,   4285,   4286,   4287,
    4286,   4287,   4288,   4289,   4290,   4291,   4292,   4293,   4294,   4295,   4296,   4297,   4298,   4299,   4300,   4301,   4300,   4301,   4302,   4303,   4304,   4305,   4306,   4307,   4308,   4309,   4310,   4311,   4312,   4313,   4314,   4315,
    4312,   4313,   4314,   4315,   4316,   4317,   4318,   4319,   4320,   4321,   4322,   4323,   4324,   4325,   4326,   4327,   4326,   4327,   4328,   4329,   4330,   4331,   4332,   4333,   4334,   4335,   4336,   4337,   4338,   4339,   4340,   4341,
    4340,   4341,   4342,   4343,   4344,   4345,   4346,   4347,   4348,   4349,   4350,   4351,   4352,   4353,   4354,   4355,   4354,   4355,   4356,   4357,   4358,   4359,   4360,   4361,   4362,   4363,   4364,   4365,   4366,   4367,   4368,   4369,
    4366,   4367,   4368,   4369,   4370,   4371,   4372,   4373,   4374,   4375,   4376,   4377,   4378,   4379,   4380,   4381,   4380,   4381,   4382,   4383,   4384,   4385,   4386,   4387,   4388,   4389,   4390,   4391,   4392,   4393,   4394,   4395,
    4394,   4395,   4396,   4397,   4398,   4399,   4400,   4401,   4402,   4403,   4404,   4405,   4406,   4407,   4408,   4409,   4408,   4409,   4410,   4411,   4412,   4413,   4414,   4415,   4416,   4417,   4418,   4419,   4420,   4421,   4422,   4423,
    4420,   4421,   4422,   4423,   4424,   4425,   4426,   4427,   4428,   4429,   4430,   4431,   4432,   4433,   4434,   4435,   4434,   4435,   4436,   4437,   4438,   4439,   4440,   4441,   4442,   4443,   4444,   4445,   4446,   4447,   4448,   4449,
    4448,   4449,   4450,   4451,   4452,   4453,   4454,   4455,   4456,   4457,   4458,   4459,   4460,   4461,   4462,   4463,   4462,   4463,   4464,   4465,   4466,   4467,   4468,   4469,   4470,   4471,   4472,   4473,   4474,   4475,   4476,   4477,
    4474,   4475,   4476,   4477,   4478,   4479,   4480,   4481,   4482,   4483,   4484,   4485,   4486,   4487,   4488,   4489,   4488,   4489,   4490,   4491,   4492,   4493,   4494,   4495,   4496,   4497,   4498,   4499,   4500,   4501,   4502,   4503,
    4502,   4503,   4504,   4505,   4506,   4507,   4508,   4509,   4510,   4511,   4512,   4513,   4514,   4515,   4516,   4517,   4516,   4517,   4518,   4519,   4520,   4521,   4522,   4523,   4524,   4525,   4526,   4527,   4528,   4529,   4530,   4531,
    4528,   4529,   4530,   4531,   4532,   4533,   4534,   4535,   4536,   4537,   4538,   4539,   4540,   4541,   4542,   4543,   4542,   4543,   4544,   4545,   4546,   4547,   4548,   4549,   4550,   4551,   4552,   4553,   4554,   4555,   4556,   4557,
    4556,   4557,   4558,   4559,   4560,   4561,   4562,   4563,   4564,   4565,   4566,   4567,   4568,   4569,   4570,   4571,   4570,   4571,   4572,   4573,   4574,   4575,   4576,   4577,   4578,   4579,   4580,   4581,   4582,   4583,   4584,   4585,
    4582,   4583,   4584,   4585,   4586,   4587,   4588,   4589,   4590,   4591,   4592,   4593,   4594,   4595,   4596,   4597,   4596,   4597,   4598,   4599,   4600,   4601,   4602,   4603,   4604,   4605,   4606,   4607,   4608,   4609,   4610,   4611,
    4610,   4611,   4612,   4613,   4614,   4615,   4616,   4617,   4618,   4619,   4620,   4621,   4622,   4623,   4624,   4625,   4624,   4625,   4626,   4627,   4628,   4629,   4630,   4631,   4632,   4633,   4634,   4635,   4636,   4637,   4638,   4639,
    4636,   4637,   4638,   4639,   4640,   4641,   4642,   4643,   4644,   4645,   4646,   4647,   4648,   4649,   4650,   4651,   4650,   4651,   4652,   4653,   4654,   4655,   4656,   4657,   4658,   4659,   4660,   4661,   4662,   4663,   4664,   4665,
    4664,   4665,   4666,   4667,   4668,   4669,   4670,   4671,   4672,   4673,   4674,   4675,   4676,   4677,   4678,   4679,   4678,   4679,   4680,   4681,   4682,   4683,   4684,   4685,   4686,   4687,   4688,   4689,   4690,   4691,   4692,   4693,
    4690,   4691,   4692,   4693,   4694,   4695,   4696,   4697,   4698,   4699,   4700,   4701,   4702,   4703,   4704,   4705,   4704,   4705,   4706,   4707,   4708,   4709,   4710,   4711,   4712,   4713,   4714,   4715,   4716,   4717,   4718,   4719,
    4718,   4719,   4720,   4721,   4722,   4723,   4724,   4725,   4726,   4727,   4728,   4729,   4730,   4731,   4732,   4733,   4732,   4733,   4734,   4735,   4736,   4737,   4738,   4739,   4740,   4741,   4742,   4743,   4744,   4745,   4746,   4747,
    4744,   4745,   4746,   4747,   4748,   4749,   4750,   4751,   4752,   4753,   4754,   4755,   4756,   4757,   4758,   4759,   4758,   4759,   4760,   4761,   4762,   4763,   4764,   4765,   4766,   4767,   4768,   4769,   4770,   4771,   4772,   4773,
    4772,   4773,   4774,   4775,   4776,   4777,   4778,   4779,   4780,   4781,   4782,   4783,   4784,   4785,   4786,   4787,   4786,   4787,   4788,   4789,   4790,   4791,   4792,   4793,   4794,   4795,   4796,   4797,   4798,   4799,   4800,   4801,
    4798,   4799,   4800,   4801,   4802,   4803,   4804,   4805,   4806,   4807,   4808,   4809,   4810,   4811,   4812,   4813,   4812,   4813,   4814,   4815,   4816,   4817,   4818,   4819,   4820,   4821,   4822,   4823,   4824,   4825,   4826,   4827,
    4826,   4827,   4828,   4829,   4830,   4831,   4832,   4833,   4834,   4835,   4836,   4837,   4838,   4839,   4840,   4841,   4840,   4841,   4842,   4843,   4844,   4845,   4846,   4847,   4848,   4849,   4850,   4851,   4852,   4853,   4854,   4855,
    4852,   4853,   4854,   4855,   4856,   4857,   4858,   4859,   4860,   4861,   4862,   4863,   4864,   4865,   4866,   4867,   4866,   4867,   4868,   4869,   4870,   4871,   4872,   4873,   4874,   4875,   4876,   4877,   4878,   4879,   4880,   4881,
    4880,   4881,   4882,   4883,   4884,   4885,   4886,   4887,   4888,   4889,   4890,   4891,   4892,   4893,   4894,   4895,   4894,   4895,   4896,   4897,   4898,   4899,   4900,   4901,   4902,   4903,   4904,   4905,   4906,   4907,   4908,   4909,
    4906,   4907,   4908,   4909,   4910,   4911,   4912,   4913,   4914,   4915,   4916,   4917,   4918,   4919,   4920,   4921,   4920,   4921,   4922,   4923,   4924,   4925,   4926,   4927,   4928,   4929,   4930,   4931,   4932,   4933,   4934,   4935,
    4934,   4935,   4936,   4937,   4938,   4939,   4940,   4941,   4942,   4943,   4944,   4945,   4946,   4947,   4948,   4949,   4948,   4949,   4950,   4951,   4952,   4953,   4954,   4955,   4956,   4957,   4958,   4959,   4960,   4961,   4962,   4963,
    4960,   4961,   4962,   4963,   4964,   4965,   4966,   4967,   4968,   4969,   4970,   4971,   4972,   4973,   4974,   4975,   4974,   4975,   4976,   4977,   4978,   4979,   4980,   4981,   4982,   4983,   4984,   4985,   4986,   4987,   4988,   4989,
    4988,   4989,   4990,   4991,   4992,   4993,   4994,   4995,   4996,   4997,   4998,   4999,   5000,   5001,   5002,   5003,   5002,   5003,   5004,   5005,   5006,   5007,   5008,   5009,   5010,   5011,   5012,   5013,   5014,   5015,   5016,   5017,
    5014,   5015,   5016,   5017,   5018,   5019,   5020,   5021,   5022,   5023,   5024,   5025,   5026,   5027,   5028,   5029,   5028,   5029,   5030,   5031,   5032,   5033,   5034,   5035,   5036,   5037,   5038,   5039,   5040,   5041,   5042,   5043,
    5042,   5043,   5044,   5045,   5046,   5047,   5048,   5049,   5050,   5051,   5052,   5053,   5054,   5055,   5056,   5057,   5056,   5057,   5058,   5059,   5060,   5061,   5062,   5063,   5064,   5065,   5066,   5067,   5068,   5069,   5070,   5071,
    5068,   5069,   5070,   5071,   5072,   5073,   5074,   5075,   5076,   5077,   5078,   5079,   5080,   5081,   5082,   5083,   5082,   5083,   5084,   5085,   5086,   5087,   5088,   5089,   5090,   5091,   5092,   5093,   5094,   5095,   5096,   5097,
    5096,   5097,   5098,   5099,   5100,   5101,   5102,   5103,   5104,   5105,   5106,   5107,   5108,   5109,   5110,   5111,   5110,   5111,   5112,   5113,   5114,   5115,   5116,   5117,   5118,   5119,   5120,   5121,   5122,   5123,   5124,   5125,
    5122,   5123,   5124,   5125,   5126,   5127,   5128,   5129,   5130,   5131,   5132,   5133,   5134,   5135,   5136,   5137,   5136,   5137,   5138,   5139,   5140,   5141,   5142,   5143,   5144,   5145,   5146,   5147,   5148,   5149,   5150,   5151,
    5150,   5151,   5152,   5153,   5154,   5155,   5156,   5157,   5158,   5159,   5160,   5161,   5162,   5163,   5164,   5165,   5164,   5165,   5166,   5167,   5168,   5169,   5170,   5171,   5172,   5173,   5174,   5175,   5176,   5177,   5178,   5179,
    5176,   5177,   5178,   5179,   5180,   5181,   5182,   5183,   5184,   5185,   5186,   5187,   5188,   5189,   5190,   5191,   5190,   5191,   5192,   5193,   5194,   5195,   5196,   5197,   5198,   5199,   5200,   5201,   5202,   5203,   5204,   5205,
    5204,   5205,   5206,   5207,   5208,   5209,   5210,   5211,   5212,   5213,   5214,   5215,   5216,   5217,   5218,   5219,   5218,   5219,   5220,   5221,   5222,   5223,   5224,   5225,   5226,   5227,   5228,   5229,   5230,   5231,   5232,   5233,
    5230,   5231,   5232,   5233,   5234,   5235,   5236,   5237,   5238,   5239,   5240,   5241,   5242,   5243,   5244,   5245,   5244,   5245,   5246,   5247,   5248,   5249,   5250,   5251,   5252,   5253,   5254,   5255,   5256,   5257,   5258,   5259,
    5258,   5259,   5260,   5261,   5262,   5263,   5264,   5265,   5266,   5267,   5268,   5269,   5270,   5271,   5272,   5273,   5272,   5273,   5274,   5275,   5276,   5277,   5278,   5279,   5280,   5281,   5282,   5283,   5284,   5285,   5286,   5287,
    5284,   5285,   5286,   5287,   5288,   5289,   5290,   5291,   5292,   5293,   5294,   5295,   5296,   5297,   5298,   5299,   5298,   5299,   5300,   5301,   5302,   5303,   5304,   5305,   5306,   5307,   5308,   5309,   5310,   5311,   5312,   5313,
    5312,   5313,   5314,   5315,   5316,   5317,   5318,   5319,   5320,   5321,   5322,   5323,   5324,   5325,   5326,   5327,   5326,   5327,   5328,   5329,   5330,   5331,   5332,   5333,   5334,   5335,   5336,   5337,   5338,   5339,   5340,   5341,
    5338,   5339,   5340,   5341,   5342,   5343,   5344,   5345,   5346,   5347,   5348,   5349,   5350,   5351,   5352,   5353,   5352,   5353,   5354,   5355,   5356,   5357,   5358,   5359,   5360,   5361,   5362,   5363,   5364,   5365,   5366,   5367,
    5366,   5367,   5368,   5369,   5370,   5371,   5372,   5373,   5374,   5375,   5376,   5377,   5378,   5379,   5380,   5381,   5380,   5381,   5382,   5383,   5384,   5385,   5386,   5387,   5388,   5389,   5390,   5391,   5392,   5393,   5394,   5395,
    5392,   5393,   5394,   5395,   5396,   5397,   5398,   5399,   5400,   5401,   5402,   5403,   5404,   5405,   5406,   5407,   5406,   5407,   5408,   5409,   5410,   5411,   5412,   5413,   5414,   5415,   5416,   5417,   5418,   5419,   5420,   5421,
    5420,   5421,   5422,   5423,   5424,   5425,   5426,   5427,   5428,   5429,   5430,   5431,   5432,   5433,   5434,   5435,   5434,   5435,   5436,   5437,   5438,   5439,   5440,   5441,   5442,   5443,   5444,   5445,   5446,   5447,   5448,   5449,
    5446,   5447,   5448,   5449,   5450,   5451,   5452,   5453,   5454,   5455,   5456,   5457,   5458,   5459,   5460,   5461,   5460,   5461,   5462,   5463,   5464,   5465,   5466,   5467,   5468,   5469,   5470,   5471,   5472,   5473,   5474,   5475,
    5474,   5475,   5476,   5477,   5478,   5479,   5480,   5481,   5482,   5483,   5484,   5485,   5486,   5487,   5488,   5489,   5488,   5489,   5490,   5491,   5492,   5493,   5494,   5495,   5496,   5497,   5498,   5499,   5500,   5501,   5502,   5503,
    5500,   5501,   5502,   5503,   5504,   5505,   5506,   5507,   5508,   5509,   5510,   5511,   5512,   5513,   5514,   5515,   5514,   5515,   5516,   5517,   5518,   5519,   5520,   5521,   5522,   5523,   5524,   5525,   5526,   5527,   5528,   5529,
    5528,   5529,   5530,   5531,   5532,   5533,   5534,   5535,   5536,   5537,   5538,   5539,   5540,   5541,   5542,   5543,   5542,   5543,   5544,   5545,   5546,   5547,   5548,   5549,   5550,   5551,   5552,   5553,   5554,   5555,   5556,   5557,
    5554,   5555,   5556,   5557,   5558,   5559,   5560,   5561,   5562,   5563,   5564,   5565,   5566,   5567,   5568,   5569,   5568,   5569,   5570,   5571,   5572,   5573,   5574,   5575,   5576,   5577,   5578,   5579,   5580,   5581,   5582,   5583,
    5582,   5583,   5584,   5585,   5586,   5587,   5588,   5589,   5590,   5591,   5592,   5593,   5594,   5595,   5596,   5597,   5596,   5597,   5598,   5599,   5600,   5601,   5602,   5603,   5604,   5605,   5606,   5607,   5608,   5609,   5610,   5611,
    5608,   5609,   5610,   5611,   5612,   5613,   5614,   5615,   5616,   5617,   5618,   5619,   5620,   5621,   5622,   5623,   5622,   5623,   5624,   5625,   5626,   5627,   5628,   5629,   5630,   5631,   5632,   5633,   5634,   5635,   5636,   5637,
    5636,   5637,   5638,   5639,   5640,   5641,   5642,   5643,   5644,   5645,   5646,   5647,   5648,   5649,   5650,   5651,   5650,   5651,   5652,   5653,   5654,   5655,   5656,   5657,   5658,   5659,   5660,   5661,   5662,   5663,   5664,   5665,
    5662,   5663,   5664,   5665,   5666,   5667,   5668,   5669,   5670,   5671,   5672,   5673,   5674,   5675,   5676,   5677,   5676,   5677,   5678,   5679,   5680,   5681,   5682,   5683,   5684,   5685,   5686,   5687,   5688,   5689,   5690,   5691,
    5690,   5691,   5692,   5693,   5694,   5695,   5696,   5697,   5698,   5699,   5700,   5701,   5702,   5703,   5704,   5705,   5704,   5705,   5706,   5707,   5708,   5709,   5710,   5711,   5712,   5713,   5714,   5715,   5716,   5717,   5718,   5719,
    5716,   5717,   5718,   5719,   5720,   5721,   5722,   5723,   5724,   5725,   5726,   5727,   5728,   5729,   5730,   5731,   5730,   5731,   5732,   5733,   5734,   5735,   5736,   5737,   5738,   5739,   5740,   5741,   5742,   5743,   5744,   5745,
    5744,   5745,   5746,   5747,   5748,   5749,   5750,   5751,   5752,   5753,   5754,   5755,   5756,   5757,   5758,   5759,   5758,   5759,   5760,   5761,   5762,   5763,   5764,   5765,   5766,   5767,   5768,   5769,   5770,   5771,   5772,   5773,
    5770,   5771,   5772,   5773,   5774,   5775,   5776,   5777,   5778,   5779,   5780,   5781,   5782,   5783,   5784,   5785,   5784,   5785,   5786,   5787,   5788,   5789,   5790,   5791,   5792,   5793,   5794,   5795,   5796,   5797,   5798,   5799,
    5798,   5799,   5800,   5801,   5802,   5803,   5804,   5805,   5806,   5807,   5808,   5809,   5810,   5811,   5812,   5813,   5812,   5813,   5814,   5815,   5816,   5817,   5818,   5819,   5820,   5821,   5822,   5823,   5824,   5825,   5826,   5827,

    14146,  14147,  14148,  14149,  14150,  14151,  14152,  14153,  14154,  14155,  14156,  14157,  14158,  14159,  14160,  14161,  14162,  14163,  14164,  14165,  14166,  14167,  14168,  14169,  14170,  14171,  14172,  14173,  14174,  14175,  14176,  14177,
    14178,  14179,  14180,  14181,  14182,  14183,  14184,  14185,  14186,  14187,  14188,  14189,  14190,  14191,  14192,  14193,  14194,  14195,  14196,  14197,  14198,  14199,  14200,  14201,  14202,  14203,  14204,  14205,  14206,  14207,  14208,  14209,
    14210,  14211,  14212,  14213,  14214,  14215,  14216,  14217,  14218,  14219,  14220,  14221,  14222,  14223,  14224,  14225,  14226,  14227,  14228,  14229,  14230,  14231,  14232,  14233,  14234,  14235,  14236,  14237,  14238,  14239,  14240,  14241,
    14242,  14243,  14244,  14245,  14246,  14247,  14248,  14249,  14250,  14251,  14252,  14253,  14254,  14255,  14256,  14257,  14258,  14259,  14260,  14261,  14262,  14263,  14264,  14265,  14266,  14267,  14268,  14269,  14270,  14271,  14272,  14273,
    14274,  14275,  14276,  14277,  14278,  14279,  14280,  14281,  14282,  14283,  14284,  14285,  14286,  14287,  14288,  14289,  14290,  14291,  14292,  14293,  14294,  14295,  14296,  14297,  14298,  14299,  14300,  14301,  14302,  14303,  14304,  14305,
    14306,  14307,  14308,  14309,  14310,  14311,  14312,  14313,  14314,  14315,  14316,  14317,  14318,  14319,  14320,  14321,  14322,  14323,  14324,  14325,  14326,  14327,  14328,  14329,  14330,  14331,  14332,  14333,  14334,  14335,  14335,  14334,
    14333,  14332,  14331,  14330,  14329,  14328,  14327,  14326,  14325,  14324,  14323,  14322,  14321,  14320,  14319,  14318,  14317,  14316,  14315,  14314,  14313,  14312,  14311,  14310,  14309,  14308,  14307,  14306,  14305,  14304,  14303,  14302,
    14301,  14300,  14299,  14298,  14297,  14296,  14295,  14294,  14293,  14292,  14291,  14290,  14289,  14288,  14287,  14286,  14285,  14284,  14283,  14282,  14281,  14280,  14279,  14278,  14277,  14276,  14275,  14274,  14273,  14272,  14271,  14270,
    14269,  14268,  14267,  14266,  14265,  14264,  14263,  14262,  14261,  14260,  14259,  14258,  14257,  14256,  14255,  14254,  14253,  14252,  14251,  14250,  14249,  14248,  14247,  14246,  14245,  14244,  14243,  14242,  14241,  14240,  14239,  14238,
    14237,  14236,  14235,  14234,  14233,  14232,  14231,  14230,  14229,  14228,  14227,  14226,  14225,  14224,  14223,  14222,  14221,  14220,  14219,  14218,  14217,  14216,  14215,  14214,  14213,  14212,  14211,  14210,  14209,  14208,  14207,  14206,
    14205,  14204,  14203,  14202,  14201,  14200,  14199,  14198,  14197,  14196,  14195,  14194,  14193,  14192,  14191,  14190,  14189,  14188,  14187,  14186,  14185,  14184,  14183,  14182,  14181,  14180,  14179,  14178,  14177,  14176,  14175,  14174,
    14173,  14172,  14171,  14170,  14169,  14168,  14167,  14166,  14165,  14164,  14163,  14162,  14161,  14160,  14159,  14158,  14157,  14156,  14155,  14154,  14153,  14152,  14151,  14150,  14149,  14148,  14147,  14146,  14145,  14144,  14143,  14142,
    14141,  14140,  14139,  14138,  14137,  14136,  14135,  14134,  14133,  14132,  14131,  14130,  14129,  14128,  14127,  14126,  14125,  14124,  14123,  14122,  14121,  14120,  14119,  14118,  14117,  14116,  14115,  14114,  14113,  14112,  14111,  14110,
    14109,  14108,  14107,  14106,  14105,  14104,  14103,  14102,  14101,  14100,  14099,  14098,  14097,  14096,  14095,  14094,  14093,  14092,  14091,  14090,  14089,  14088,  14087,  14086,  14085,  14084,  14083,  14082,  14081,  14080,  14079,  14078,
    14077,  14076,  14075,  14074,  14073,  14072,  14071,  14070,  14069,  14068,  14067,  14066,  14065,  14064,  14063,  14062,  14061,  14060,  14059,  14058,  14057,  14056,  14055,  14054,  14053,  14052,  14051,  14050,  14049,  14048,  14047,  14046,
    14045,  14044,  14043,  14042,  14041,  14040,  14039,  14038,  14037,  14036,  14035,  14034,  14033,  14032,  14031,  14030,  14029,  14028,  14027,  14026,  14025,  14024,  14023,  14022,  14021,  14020,  14019,  14018,  14017,  14016,  14015,  14014,
    14013,  14012,  14011,  14010,  14009,  14008,  14007,  14006,  14005,  14004,  14003,  14002,  14001,  14000,  13999,  13998,  13997,  13996,  13995,  13994,  13993,  13992,  13991,  13990,  13989,  13988,  13987,  13986,  13985,  13984,  13983,  13982,
    13981,  13980,  13979,  13978,  13977,  13976,  13975,  13974,  13973,  13972,  13971,  13970,  13969,  13968,  13967,  13966,  13965,  13964,  13963,  13962,  13961,  13960,  13959,  13958,  13957,  13956,  13955,  13954,  13953,  13952,  13951,  13950,
    13949,  13948,  13947,  13946,  13945,  13944,  13943,  13942,  13941,  13940,  13939,  13938,  13937,  13936,  13935,  13934,  13933,  13932,  13931,  13930,  13929,  13928,  13927,  13926,  13925,  13924,  13923,  13922,  13921,  13920,  13919,  13918,
    13917,  13916,  13915,  13914,  13913,  13912,  13911,  13910,  13909,  13908,  13907,  13906,  13905,  13904,  13903,  13902,  13901,  13900,  13899,  13898,  13897,  13896,  13895,  13894,  13893,  13892,  13891,  13890,  13889,  13888,  13887,  13886,
    13885,  13884,  13883,  13882,  13881,  13880,  13879,  13878,  13877,  13876,  13875,  13874,  13873,  13872,  13871,  13870,  13869,  13868,  13867,  13866,  13865,  13864,  13863,  13862,  13861,  13860,  13859,  13858,  13857,  13856,  13855,  13854,
    13853,  13852,  13851,  13850,  13849,  13848,  13847,  13846,  13845,  13844,  13843,  13842,  13841,  13840,  13839,  13838,  13837,  13836,  13835,  13834,  13833,  13832,  13831,  13830,  13829,  13828,  13827,  13826,  13825,  13824,  13823,  13822,
    13821,  13820,  13819,  13818,  13817,  13816,  13815,  13814,  13813,  13812,  13811,  13810,  13809,  13808,  13807,  13806,  13805,  13804,  13803,  13802,  13801,  13800,  13799,  13798,  13797,  13796,  13795,  13794,  13793,  13792,  13791,  13790,
    13789,  13788,  13787,  13786,  13785,  13784,  13783,  13782,  13781,  13780,  13779,  13778,  13777,  13776,  13775,  13774,  13773,  13772,  13771,  13770,  13769,  13768,  13767,  13766,  13765,  13764,  13763,  13762,  13761,  13760,  13759,  13758,
    13757,  13756,  13755,  13754,  13753,  13752,  13751,  13750,  13749,  13748,  13747,  13746,  13745,  13744,  13743,  13742,  13741,  13740,  13739,  13738,  13737,  13736,  13735,  13734,  13733,  13732,  13731,  13730,  13729,  13728,  13727,  13726,
    13725,  13724,  13723,  13722,  13721,  13720,  13719,  13718,  13717,  13716,  13715,  13714,  13713,  13712,  13711,  13710,  13709,  13708,  13707,  13706,  13705,  13704,  13703,  13702,  13701,  13700,  13699,  13698,  13697,  13696,  13695,  13694,
    13693,  13692,  13691,  13690,  13689,  13688,  13687,  13686,  13685,  13684,  13683,  13682,  13681,  13680,  13679,  13678,  13677,  13676,  13675,  13674,  13673,  13672,  13671,  13670,  13669,  13668,  13667,  13666,  13665,  13664,  13663,  13662,
    13661,  13660,  13659,  13658,  13657,  13656,  13655,  13654,  13653,  13652,  13651,  13650,  13649,  13648,  13647,  13646,  13645,  13644,  13643,  13642,  13641,  13640,  13639,  13638,  13637,  13636,  13635,  13634,  13633,  13632,  13631,  13630,
    13629,  13628,  13627,  13626,  13625,  13624,  13623,  13622,  13621,  13620,  13619,  13618,  13617,  13616,  13615,  13614,  13613,  13612,  13611,  13610,  13609,  13608,  13607,  13606,  13605,  13604,  13603,  13602,  13601,  13600,  13599,  13598,
    13597,  13596,  13595,  13594,  13593,  13592,  13591,  13590,  13589,  13588,  13587,  13586,  13585,  13584,  13583,  13582,  13581,  13580,  13579,  13578,  13577,  13576,  13575,  13574,  13573,  13572,  13571,  13570,  13569,  13568,  13567,  13566,
    13565,  13564,  13563,  13562,  13561,  13560,  13559,  13558,  13557,  13556,  13555,  13554,  13553,  13552,  13551,  13550,  13549,  13548,  13547,  13546,  13545,  13544,  13543,  13542,  13541,  13540,  13539,  13538,  13537,  13536,  13535,  13534,
    13533,  13532,  13531,  13530,  13529,  13528,  13527,  13526,  13525,  13524,  13523,  13522,  13521,  13520,  13519,  13518,  13517,  13516,  13515,  13514,  13513,  13512,  13511,  13510,  13509,  13508,  13507,  13506,  13505,  13504,  13503,  13502,
    13501,  13500,  13499,  13498,  13497,  13496,  13495,  13494,  13493,  13492,  13491,  13490,  13489,  13488,  13487,  13486,  13485,  13484,  13483,  13482,  13481,  13480,  13479,  13478,  13477,  13476,  13475,  13474,  13473,  13472,  13471,  13470,
    13469,  13468,  13467,  13466,  13465,  13464,  13463,  13462,  13461,  13460,  13459,  13458,  13457,  13456,  13455,  13454,  13453,  13452,  13451,  13450,  13449,  13448,  13447,  13446,  13445,  13444,  13443,  13442,  13441,  13440,  13439,  13438,
    13437,  13436,  13435,  13434,  13433,  13432,  13431,  13430,  13429,  13428,  13427,  13426,  13425,  13424,  13423,  13422,  13421,  13420,  13419,  13418,  13417,  13416,  13415,  13414,  13413,  13412,  13411,  13410,  13409,  13408,  13407,  13406,
    13405,  13404,  13403,  13402,  13401,  13400,  13399,  13398,  13397,  13396,  13395,  13394,  13393,  13392,  13391,  13390,  13389,  13388,  13387,  13386,  13385,  13384,  13383,  13382,  13381,  13380,  13379,  13378,  13377,  13376,  13375,  13374,
    13373,  13372,  13371,  13370,  13369,  13368,  13367,  13366,  13365,  13364,  13363,  13362,  13361,  13360,  13359,  13358,  13357,  13356,  13355,  13354,  13353,  13352,  13351,  13350,  13349,  13348,  13347,  13346,  13345,  13344,  13343,  13342,
    13341,  13340,  13339,  13338,  13337,  13336,  13335,  13334,  13333,  13332,  13331,  13330,  13329,  13328,  13327,  13326,  13325,  13324,  13323,  13322,  13321,  13320,  13319,  13318,  13317,  13316,  13315,  13314,  13313,  13312,  13311,  13310,
    13309,  13308,  13307,  13306,  13305,  13304,  13303,  13302,  13301,  13300,  13299,  13298,  13297,  13296,  13295,  13294,  13293,  13292,  13291,  13290,  13289,  13288,  13287,  13286,  13285,  13284,  13283,  13282,  13281,  13280,  13279,  13278,
    13277,  13276,  13275,  13274,  13273,  13272,  13271,  13270,  13269,  13268,  13267,  13266,  13265,  13264,  13263,  13262,  13261,  13260,  13259,  13258,  13257,  13256,  13255,  13254,  13253,  13252,  13251,  13250,  13249,  13248,  13247,  13246,
    13245,  13244,  13243,  13242,  13241,  13240,  13239,  13238,  13237,  13236,  13235,  13234,  13233,  13232,  13231,  13230,  13229,  13228,  13227,  13226,  13225,  13224,  13223,  13222,  13221,  13220,  13219,  13218,  13217,  13216,  13215,  13214,
    13213,  13212,  13211,  13210,  13209,  13208,  13207,  13206,  13205,  13204,  13203,  13202,  13201,  13200,  13199,  13198,  13197,  13196,  13195,  13194,  13193,  13192,  13191,  13190,  13189,  13188,  13187,  13186,  13185,  13184,  13183,  13182,
    13181,  13180,  13179,  13178,  13177,  13176,  13175,  13174,  13173,  13172,  13171,  13170,  13169,  13168,  13167,  13166,  13165,  13164,  13163,  13162,  13161,  13160,  13159,  13158,  13157,  13156,  13155,  13154,  13153,  13152,  13151,  13150,
    13149,  13148,  13147,  13146,  13145,  13144,  13143,  13142,  13141,  13140,  13139,  13138,  13137,  13136,  13135,  13134,  13133,  13132,  13131,  13130,  13129,  13128,  13127,  13126,  13125,  13124,  13123,  13122,  13121,  13120,  13119,  13118,
    13117,  13116,  13115,  13114,  13113,  13112,  13111,  13110,  13109,  13108,  13107,  13106,  13105,  13104,  13103,  13102,  13101,  13100,  13099,  13098,  13097,  13096,  13095,  13094,  13093,  13092,  13091,  13090,  13089,  13088,  13087,  13086,
    13085,  13084,  13083,  13082,  13081,  13080,  13079,  13078,  13077,  13076,  13075,  13074,  13073,  13072,  13071,  13070,  13069,  13068,  13067,  13066,  13065,  13064,  13063,  13062,  13061,  13060,  13059,  13058,  13057,  13056,  13055,  13054,
    13053,  13052,  13051,  13050,  13049,  13048,  13047,  13046,  13045,  13044,  13043,  13042,  13041,  13040,  13039,  13038,  13037,  13036,  13035,  13034,  13033,  13032,  13031,  13030,  13029,  13028,  13027,  13026,  13025,  13024,  13023,  13022,
    13021,  13020,  13019,  13018,  13017,  13016,  13015,  13014,  13013,  13012,  13011,  13010,  13009,  13008,  13007,  13006,  13005,  13004,  13003,  13002,  13001,  13000,  12999,  12998,  12997,  12996,  12995,  12994,  12993,  12992,  12991,  12990,
    12989,  12988,  12987,  12986,  12985,  12984,  12983,  12982,  12981,  12980,  12979,  12978,  12977,  12976,  12975,  12974,  12973,  12972,  12971,  12970,  12969,  12968,  12967,  12966,  12965,  12964,  12963,  12962,  12961,  12960,  12959,  12958,
    12957,  12956,  12955,  12954,  12953,  12952,  12951,  12950,  12949,  12948,  12947,  12946,  12945,  12944,  12943,  12942,  12941,  12940,  12939,  12938,  12937,  12936,  12935,  12934,  12933,  12932,  12931,  12930,  12929,  12928,  12927,  12926,
    12925,  12924,  12923,  12922,  12921,  12920,  12919,  12918,  12917,  12916,  12915,  12914,  12913,  12912,  12911,  12910,  12909,  12908,  12907,  12906,  12905,  12904,  12903,  12902,  12901,  12900,  12899,  12898,  12897,  12896,  12895,  12894,
    12893,  12892,  12891,  12890,  12889,  12888,  12887,  12886,  12885,  12884,  12883,  12882,  12881,  12880,  12879,  12878,  12877,  12876,  12875,  12874,  12873,  12872,  12871,  12870,  12869,  12868,  12867,  12866,  12865,  12864,  12863,  12862,
    12861,  12860,  12859,  12858,  12857,  12856,  12855,  12854,  12853,  12852,  12851,  12850,  12849,  12848,  12847,  12846,  12845,  12844,  12843,  12842,  12841,  12840,  12839,  12838,  12837,  12836,  12835,  12834,  12833,  12832,  12831,  12830,
    12829,  12828,  12827,  12826,  12825,  12824,  12823,  12822,  12821,  12820,  12819,  12818,  12817,  12816,  12815,  12814,  12813,  12812,  12811,  12810,  12809,  12808,  12807,  12806,  12805,  12804,  12803,  12802,  12801,  12800,  12799,  12798,
    12797,  12796,  12795,  12794,  12793,  12792,  12791,  12790,  12789,  12788,  12787,  12786,  12785,  12784,  12783,  12782,  12781,  12780,  12779,  12778,  12777,  12776,  12775,  12774,  12773,  12772,  12771,  12770,  12769,  12768,  12767,  12766,
    12765,  12764,  12763,  12762,  12761,  12760,  12759,  12758,  12757,  12756,  12755,  12754,  12753,  12752,  12751,  12750,  12749,  12748,  12747,  12746,  12745,  12744,  12743,  12742,  12741,  12740,  12739,  12738,  12737,  12736,  12735,  12734,
    12733,  12732,  12731,  12730,  12729,  12728,  12727,  12726,  12725,  12724,  12723,  12722,  12721,  12720,  12719,  12718,  12717,  12716,  12715,  12714,  12713,  12712,  12711,  12710,  12709,  12708,  12707,  12706,  12705,  12704,  12703,  12702,
    12701,  12700,  12699,  12698,  12697,  12696,  12695,  12694,  12693,  12692,  12691,  12690,  12689,  12688,  12687,  12686,  12685,  12684,  12683,  12682,  12681,  12680,  12679,  12678,  12677,  12676,  12675,  12674,  12673,  12672,  12671,  12670,
    12669,  12668,  12667,  12666,  12665,  12664,  12663,  12662,  12661,  12660,  12659,  12658,  12657,  12656,  12655,  12654,  12653,  12652,  12651,  12650,  12649,  12648,  12647,  12646,  12645,  12644,  12643,  12642,  12641,  12640,  12639,  12638,
    12637,  12636,  12635,  12634,  12633,  12632,  12631,  12630,  12629,  12628,  12627,  12626,  12625,  12624,  12623,  12622,  12621,  12620,  12619,  12618,  12617,  12616,  12615,  12614,  12613,  12612,  12611,  12610,  12609,  12608,  12607,  12606,
    12605,  12604,  12603,  12602,  12601,  12600,  12599,  12598,  12597,  12596,  12595,  12594,  12593,  12592,  12591,  12590,  12589,  12588,  12587,  12586,  12585,  12584,  12583,  12582,  12581,  12580,  12579,  12578,  12577,  12576,  12575,  12574,
    12573,  12572,  12571,  12570,  12569,  12568,  12567,  12566,  12565,  12564,  12563,  12562,  12561,  12560,  12559,  12558,  12557,  12556,  12555,  12554,  12553,  12552,  12551,  12550,  12549,  12548,  12547,  12546,  12545,  12544,  12543,  12542,
    12541,  12540,  12539,  12538,  12537,  12536,  12535,  12534,  12533,  12532,  12531,  12530,  12529,  12528,  12527,  12526,  12525,  12524,  12523,  12522,  12521,  12520,  12519,  12518,  12517,  12516,  12515,  12514,  12513,  12512,  12511,  12510,
    12509,  12508,  12507,  12506,  12505,  12504,  12503,  12502,  12501,  12500,  12499,  12498,  12497,  12496,  12495,  12494,  12493,  12492,  12491,  12490,  12489,  12488,  12487,  12486,  12485,  12484,  12483,  12482,  12481,  12480,  12479,  12478,
    8192,   8193,   8194,   8195,   8196,   8197,   8198,   8199,   8200,   8201,   8202,   8203,   8204,   8205,   8206,   8207,   8208,   8209,   8210,   8211,   8212,   8213,   8214,   8215,   8216,   8217,   8218,   8219,   8220,   8221,   8222,   8223,
    8222,   8223,   8224,   8225,   8226,   8227,   8228,   8229,   8230,   8231,   8232,   8233,   8234,   8235,   8236,   8237,   8238,   8239,   8240,   8241,   8242,   8243,   8244,   8245,   8246,   8247,   8248,   8249,   8250,   8251,   8252,   8253,
    8252,   8253,   8254,   8255,   8256,   8257,   8258,   8259,   8260,   8261,   8262,   8263,   8264,   8265,   8266,   8267,   8268,   8269,   8270,   8271,   8272,   8273,   8274,   8275,   8276,   8277,   8278,   8279,   8280,   8281,   8282,   8283,
    8282,   8283,   8284,   8285,   8286,   8287,   8288,   8289,   8290,   8291,   8292,   8293,   8294,   8295,   8296,   8297,   8298,   8299,   8300,   8301,   8302,   8303,   8304,   8305,   8306,   8307,   8308,   8309,   8310,   8311,   8312,   8313,
    8310,   8311,   8312,   8313,   8314,   8315,   8316,   8317,   8318,   8319,   8320,   8321,   8322,   8323,   8324,   8325,   8326,   8327,   8328,   8329,   8330,   8331,   8332,   8333,   8334,   8335,   8336,   8337,   8338,   8339,   8340,   8341,
    8340,   8341,   8342,   8343,   8344,   8345,   8346,   8347,   8348,   8349,   8350,   8351,   8352,   8353,   8354,   8355,   8356,   8357,   8358,   8359,   8360,   8361,   8362,   8363,   8364,   8365,   8366,   8367,   8368,   8369,   8370,   8371,
    8370,   8371,   8372,   8373,   8374,   8375,   8376,   8377,   8378,   8379,   8380,   8381,   8382,   8383,   8384,   8385,   8386,   8387,   8388,   8389,   8390,   8391,   8392,   8393,   8394,   8395,   8396,   8397,   8398,   8399,   8400,   8401,
    8400,   8401,   8402,   8403,   8404,   8405,   8406,   8407,   8408,   8409,   8410,   8411,   8412,   8413,   8414,   8415,   8416,   8417,   8418,   8419,   8420,   8421,   8422,   8423,   8424,   8425,   8426,   8427,   8428,   8429,   8430,   8431,
    8428,   8429,   8430,   8431,   8432,   8433,   8434,   8435,   8436,   8437,   8438,   8439,   8440,   8441,   8442,   8443,   8444,   8445,   8446,   8447,   8448,   8449,   8450,   8451,   8452,   8453,   8454,   8455,   8456,   8457,   8458,   8459,
    8458,   8459,   8460,   8461,   8462,   8463,   8464,   8465,   8466,   8467,   8468,   8469,   8470,   8471,   8472,   8473,   8474,   8475,   8476,   8477,   8478,   8479,   8480,   8481,   8482,   8483,   8484,   8485,   8486,   8487,   8488,   8489,
    8488,   8489,   8490,   8491,   8492,   8493,   8494,   8495,   8496,   8497,   8498,   8499,   8500,   8501,   8502,   8503,   8504,   8505,   8506,   8507,   8508,   8509,   8510,   8511,   8512,   8513,   8514,   8515,   8516,   8517,   8518,   8519,
    8518,   8519,   8520,   8521,   8522,   8523,   8524,   8525,   8526,   8527,   8528,   8529,   8530,   8531,   8532,   8533,   8534,   8535,   8536,   8537,   8538,   8539,   8540,   8541,   8542,   8543,   8544,   8545,   8546,   8547,   8548,   8549,
    8546,   8547,   8548,   8549,   8550,   8551,   8552,   8553,   8554,   8555,   8556,   8557,   8558,   8559,   8560,   8561,   8562,   8563,   8564,   8565,   8566,   8567,   8568,   8569,   8570,   8571,   8572,   8573,   8574,   8575,   8576,   8577,
    8576,   8577,   8578,   8579,   8580,   8581,   8582,   8583,   8584,   8585,   8586,   8587,   8588,   8589,   8590,   8591,   8592,   8593,   8594,   8595,   8596,   8597,   8598,   8599,   8600,   8601,   8602,   8603,   8604,   8605,   8606,   8607,
    8606,   8607,   8608,   8609,   8610,   8611,   8612,   8613,   8614,   8615,   8616,   8617,   8618,   8619,   8620,   8621,   8622,   8623,   8624,   8625,   8626,   8627,   8628,   8629,   8630,   8631,   8632,   8633,   8634,   8635,   8636,   8637,
    8636,   8637,   8638,   8639,   8640,   8641,   8642,   8643,   8644,   8645,   8646,   8647,   8648,   8649,   8650,   8651,   8652,   8653,   8654,   8655,   8656,   8657,   8658,   8659,   8660,   8661,   8662,   8663,   8664,   8665,   8666,   8667,
    8664,   8665,   8666,   8667,   8668,   8669,   8670,   8671,   8672,   8673,   8674,   8675,   8676,   8677,   8678,   8679,   8680,   8681,   8682,   8683,   8684,   8685,   8686,   8687,   8688,   8689,   8690,   8691,   8692,   8693,   8694,   8695,
    8694,   8695,   8696,   8697,   8698,   8699,   8700,   8701,   8702,   8703,   8704,   8705,   8706,   8707,   8708,   8709,   8710,   8711,   8712,   8713,   8714,   8715,   8716,   8717,   8718,   8719,   8720,   8721,   8722,   8723,   8724,   8725,
    8724,   8725,   8726,   8727,   8728,   8729,   8730,   8731,   8732,   8733,   8734,   8735,   8736,   8737,   8738,   8739,   8740,   8741,   8742,   8743,   8744,   8745,   8746,   8747,   8748,   8749,   8750,   8751,   8752,   8753,   8754,   8755,
    8754,   8755,   8756,   8757,   8758,   8759,   8760,   8761,   8762,   8763,   8764,   8765,   8766,   8767,   8768,   8769,   8770,   8771,   8772,   8773,   8774,   8775,   8776,   8777,   8778,   8779,   8780,   8781,   8782,   8783,   8784,   8785,
    8782,   8783,   8784,   8785,   8786,   8787,   8788,   8789,   8790,   8791,   8792,   8793,   8794,   8795,   8796,   8797,   8798,   8799,   8800,   8801,   8802,   8803,   8804,   8805,   8806,   8807,   8808,   8809,   8810,   8811,   8812,   8813,
    8812,   8813,   8814,   8815,   8816,   8817,   8818,   8819,   8820,   8821,   8822,   8823,   8824,   8825,   8826,   8827,   8828,   8829,   8830,   8831,   8832,   8833,   8834,   8835,   8836,   8837,   8838,   8839,   8840,   8841,   8842,   8843,
    8842,   8843,   8844,   8845,   8846,   8847,   8848,   8849,   8850,   8851,   8852,   8853,   8854,   8855,   8856,   8857,   8858,   8859,   8860,   8861,   8862,   8863,   8864,   8865,   8866,   8867,   8868,   8869,   8870,   8871,   8872,   8873,
    8872,   8873,   8874,   8875,   8876,   8877,   8878,   8879,   8880,   8881,   8882,   8883,   8884,   8885,   8886,   8887,   8888,   8889,   8890,   8891,   8892,   8893,   8894,   8895,   8896,   8897,   8898,   8899,   8900,   8901,   8902,   8903,
    8900,   8901,   8902,   8903,   8904,   8905,   8906,   8907,   8908,   8909,   8910,   8911,   8912,   8913,   8914,   8915,   8916,   8917,   8918,   8919,   8920,   8921,   8922,   8923,   8924,   8925,   8926,   8927,   8928,   8929,   8930,   8931,
    8930,   8931,   8932,   8933,   8934,   8935,   8936,   8937,   8938,   8939,   8940,   8941,   8942,   8943,   8944,   8945,   8946,   8947,   8948,   8949,   8950,   8951,   8952,   8953,   8954,   8955,   8956,   8957,   8958,   8959,   8960,   8961,
    8960,   8961,   8962,   8963,   8964,   8965,   8966,   8967,   8968,   8969,   8970,   8971,   8972,   8973,   8974,   8975,   8976,   8977,   8978,   8979,   8980,   8981,   8982,   8983,   8984,   8985,   8986,   8987,   8988,   8989,   8990,   8991,
    8990,   8991,   8992,   8993,   8994,   8995,   8996,   8997,   8998,   8999,   9000,   9001,   9002,   9003,   9004,   9005,   9006,   9007,   9008,   9009,   9010,   9011,   9012,   9013,   9014,   9015,   9016,   9017,   9018,   9019,   9020,   9021,
    9018,   9019,   9020,   9021,   9022,   9023,   9024,   9025,   9026,   9027,   9028,   9029,   9030,   9031,   9032,   9033,   9034,   9035,   9036,   9037,   9038,   9039,   9040,   9041,   9042,   9043,   9044,   9045,   9046,   9047,   9048,   9049,
    9048,   9049,   9050,   9051,   9052,   9053,   9054,   9055,   9056,   9057,   9058,   9059,   9060,   9061,   9062,   9063,   9064,   9065,   9066,   9067,   9068,   9069,   9070,   9071,   9072,   9073,   9074,   9075,   9076,   9077,   9078,   9079,
    9078,   9079,   9080,   9081,   9082,   9083,   9084,   9085,   9086,   9087,   9088,   9089,   9090,   9091,   9092,   9093,   9094,   9095,   9096,   9097,   9098,   9099,   9100,   9101,   9102,   9103,   9104,   9105,   9106,   9107,   9108,   9109,
    9108,   9109,   9110,   9111,   9112,   9113,   9114,   9115,   9116,   9117,   9118,   9119,   9120,   9121,   9122,   9123,   9124,   9125,   9126,   9127,   9128,   9129,   9130,   9131,   9132,   9133,   9134,   9135,   9136,   9137,   9138,   9139,
    9136,   9137,   9138,   9139,   9140,   9141,   9142,   9143,   9144,   9145,   9146,   9147,   9148,   9149,   9150,   9151,   9152,   9153,   9154,   9155,   9156,   9157,   9158,   9159,   9160,   9161,   9162,   9163,   9164,   9165,   9166,   9167,
    9166,   9167,   9168,   9169,   9170,   9171,   9172,   9173,   9174,   9175,   9176,   9177,   9178,   9179,   9180,   9181,   9182,   9183,   9184,   9185,   9186,   9187,   9188,   9189,   9190,   9191,   9192,   9193,   9194,   9195,   9196,   9197,
    9196,   9197,   9198,   9199,   9200,   9201,   9202,   9203,   9204,   9205,   9206,   9207,   9208,   9209,   9210,   9211,   9212,   9213,   9214,   9215,   9216,   9217,   9218,   9219,   9220,   9221,   9222,   9223,   9224,   9225,   9226,   9227,
    9226,   9227,   9228,   9229,   9230,   9231,   9232,   9233,   9234,   9235,   9236,   9237,   9238,   9239,   9240,   9241,   9242,   9243,   9244,   9245,   9246,   9247,   9248,   9249,   9250,   9251,   9252,   9253,   9254,   9255,   9256,   9257,
    9254,   9255,   9256,   9257,   9258,   9259,   9260,   9261,   9262,   9263,   9264,   9265,   9266,   9267,   9268,   9269,   9270,   9271,   9272,   9273,   9274,   9275,   9276,   9277,   9278,   9279,   9280,   9281,   9282,   9283,   9284,   9285,
    9284,   9285,   9286,   9287,   9288,   9289,   9290,   9291,   9292,   9293,   9294,   9295,   9296,   9297,   9298,   9299,   9300,   9301,   9302,   9303,   9304,   9305,   9306,   9307,   9308,   9309,   9310,   9311,   9312,   9313,   9314,   9315,
    9314,   9315,   9316,   9317,   9318,   9319,   9320,   9321,   9322,   9323,   9324,   9325,   9326,   9327,   9328,   9329,   9330,   9331,   9332,   9333,   9334,   9335,   9336,   9337,   9338,   9339,   9340,   9341,   9342,   9343,   9344,   9345,
    9344,   9345,   9346,   9347,   9348,   9349,   9350,   9351,   9352,   9353,   9354,   9355,   9356,   9357,   9358,   9359,   9360,   9361,   9362,   9363,   9364,   9365,   9366,   9367,   9368,   9369,   9370,   9371,   9372,   9373,   9374,   9375,
    9372,   9373,   9374,   9375,   9376,   9377,   9378,   9379,   9380,   9381,   9382,   9383,   9384,   9385,   9386,   9387,   9388,   9389,   9390,   9391,   9392,   9393,   9394,   9395,   9396,   9397,   9398,   9399,   9400,   9401,   9402,   9403,
    9402,   9403,   9404,   9405,   9406,   9407,   9408,   9409,   9410,   9411,   9412,   9413,   9414,   9415,   9416,   9417,   9418,   9419,   9420,   9421,   9422,   9423,   9424,   9425,   9426,   9427,   9428,   9429,   9430,   9431,   9432,   9433,
    9432,   9433,   9434,   9435,   9436,   9437,   9438,   9439,   9440,   9441,   9442,   9443,   9444,   9445,   9446,   9447,   9448,   9449,   9450,   9451,   9452,   9453,   9454,   9455,   9456,   9457,   9458,   9459,   9460,   9461,   9462,   9463,
    9462,   9463,   9464,   9465,   9466,   9467,   9468,   9469,   9470,   9471,   9472,   9473,   9474,   9475,   9476,   9477,   9478,   9479,   9480,   9481,   9482,   9483,   9484,   9485,   9486,   9487,   9488,   9489,   9490,   9491,   9492,   9493,
    9490,   9491,   9492,   9493,   9494,   9495,   9496,   9497,   9498,   9499,   9500,   9501,   9502,   9503,   9504,   9505,   9506,   9507,   9508,   9509,   9510,   9511,   9512,   9513,   9514,   9515,   9516,   9517,   9518,   9519,   9520,   9521,
    9520,   9521,   9522,   9523,   9524,   9525,   9526,   9527,   9528,   9529,   9530,   9531,   9532,   9533,   9534,   9535,   9536,   9537,   9538,   9539,   9540,   9541,   9542,   9543,   9544,   9545,   9546,   9547,   9548,   9549,   9550,   9551,
    9550,   9551,   9552,   9553,   9554,   9555,   9556,   9557,   9558,   9559,   9560,   9561,   9562,   9563,   9564,   9565,   9566,   9567,   9568,   9569,   9570,   9571,   9572,   9573,   9574,   9575,   9576,   9577,   9578,   9579,   9580,   9581,
    9580,   9581,   9582,   9583,   9584,   9585,   9586,   9587,   9588,   9589,   9590,   9591,   9592,   9593,   9594,   9595,   9596,   9597,   9598,   9599,   9600,   9601,   9602,   9603,   9604,   9605,   9606,   9607,   9608,   9609,   9610,   9611,
    9608,   9609,   9610,   9611,   9612,   9613,   9614,   9615,   9616,   9617,   9618,   9619,   9620,   9621,   9622,   9623,   9624,   9625,   9626,   9627,   9628,   9629,   9630,   9631,   9632,   9633,   9634,   9635,   9636,   9637,   9638,   9639,
    9638,   9639,   9640,   9641,   9642,   9643,   9644,   9645,   9646,   9647,   9648,   9649,   9650,   9651,   9652,   9653,   9654,   9655,   9656,   9657,   9658,   9659,   9660,   9661,   9662,   9663,   9664,   9665,   9666,   9667,   9668,   9669,
    9668,   9669,   9670,   9671,   9672,   9673,   9674,   9675,   9676,   9677,   9678,   9679,   9680,   9681,   9682,   9683,   9684,   9685,   9686,   9687,   9688,   9689,   9690,   9691,   9692,   9693,   9694,   9695,   9696,   9697,   9698,   9699,
    9698,   9699,   9700,   9701,   9702,   9703,   9704,   9705,   9706,   9707,   9708,   9709,   9710,   9711,   9712,   9713,   9714,   9715,   9716,   9717,   9718,   9719,   9720,   9721,   9722,   9723,   9724,   9725,   9726,   9727,   9728,   9729,
    9726,   9727,   9728,   9729,   9730,   9731,   9732,   9733,   9734,   9735,   9736,   9737,   9738,   9739,   9740,   9741,   9742,   9743,   9744,   9745,   9746,   9747,   9748,   9749,   9750,   9751,   9752,   9753,   9754,   9755,   9756,   9757,
    9756,   9757,   9758,   9759,   9760,   9761,   9762,   9763,   9764,   9765,   9766,   9767,   9768,   9769,   9770,   9771,   9772,   9773,   9774,   9775,   9776,   9777,   9778,   9779,   9780,   9781,   9782,   9783,   9784,   9785,   9786,   9787,
    9786,   9787,   9788,   9789,   9790,   9791,   9792,   9793,   9794,   9795,   9796,   9797,   9798,   9799,   9800,   9801,   9802,   9803,   9804,   9805,   9806,   9807,   9808,   9809,   9810,   9811,   9812,   9813,   9814,   9815,   9816,   9817,
    9816,   9817,   9818,   9819,   9820,   9821,   9822,   9823,   9824,   9825,   9826,   9827,   9828,   9829,   9830,   9831,   9832,   9833,   9834,   9835,   9836,   9837,   9838,   9839,   9840,   9841,   9842,   9843,   9844,   9845,   9846,   9847,
    9844,   9845,   9846,   9847,   9848,   9849,   9850,   9851,   9852,   9853,   9854,   9855,   9856,   9857,   9858,   9859,   9860,   9861,   9862,   9863,   9864,   9865,   9866,   9867,   9868,   9869,   9870,   9871,   9872,   9873,   9874,   9875,
    9874,   9875,   9876,   9877,   9878,   9879,   9880,   9881,   9882,   9883,   9884,   9885,   9886,   9887,   9888,   9889,   9890,   9891,   9892,   9893,   9894,   9895,   9896,   9897,   9898,   9899,   9900,   9901,   9902,   9903,   9904,   9905,
    9904,   9905,   9906,   9907,   9908,   9909,   9910,   9911,   9912,   9913,   9914,   9915,   9916,   9917,   9918,   9919,   9920,   9921,   9922,   9923,   9924,   9925,   9926,   9927,   9928,   9929,   9930,   9931,   9932,   9933,   9934,   9935,
    9934,   9935,   9936,   9937,   9938,   9939,   9940,   9941,   9942,   9943,   9944,   9945,   9946,   9947,   9948,   9949,   9950,   9951,   9952,   9953,   9954,   9955,   9956,   9957,   9958,   9959,   9960,   9961,   9962,   9963,   9964,   9965,
    9962,   9963,   9964,   9965,   9966,   9967,   9968,   9969,   9970,   9971,   9972,   9973,   9974,   9975,   9976,   9977,   9978,   9979,   9980,   9981,   9982,   9983,   9984,   9985,   9986,   9987,   9988,   9989,   9990,   9991,   9992,   9993,
    9992,   9993,   9994,   9995,   9996,   9997,   9998,   9999,  10000,  10001,  10002,  10003,  10004,  10005,  10006,  10007,  10008,  10009,  10010,  10011,  10012,  10013,  10014,  10015,  10016,  10017,  10018,  10019,  10020,  10021,  10022,  10023,
    10022,  10023,  10024,  10025,  10026,  10027,  10028,  10029,  10030,  10031,  10032,  10033,  10034,  10035,  10036,  10037,  10038,  10039,  10040,  10041,  10042,  10043,  10044,  10045,  10046,  10047,  10048,  10049,  10050,  10051,  10052,  10053,
    10052,  10053,  10054,  10055,  10056,  10057,  10058,  10059,  10060,  10061,  10062,  10063,  10064,  10065,  10066,  10067,  10068,  10069,  10070,  10071,  10072,  10073,  10074,  10075,  10076,  10077,  10078,  10079,  10080,  10081,  10082,  10083,

    14244,  14245,  14246,  14247,  14248,  14249,  14250,  14251,  14252,  14253,  14254,  14255,  14256,  14257,  14258,  14259,  14260,  14261,  14262,  14263,  14264,  14265,  14266,  14267,  14268,  14269,  14270,  14271,  14272,  14273,  14274,  14275,
    14276,  14277,  14278,  14279,  14280,  14281,  14282,  14283,  14284,  14285,  14286,  14287,  14288,  14289,  14290,  14291,  14292,  14293,  14294,  14295,  14296,  14297,  14298,  14299,  14300,  14301,  14302,  14303,  14304,  14305,  14306,  14307,
    14308,  14309,  14310,  14311,  14312,  14313,  14314,  14315,  14316,  14317,  14318,  14319,  14320,  14321,  14322,  14323,  14324,  14325,  14326,  14327,  14328,  14329,  14330,  14331,  14332,  14333,  14334,  14335,  14335,  14334,  14333,  14332,
    14331,  14330,  14329,  14328,  14327,  14326,  14325,  14324,  14323,  14322,  14321,  14320,  14319,  14318,  14317,  14316,  14315,  14314,  14313,  14312,  14311,  14310,  14309,  14308,  14307,  14306,  14305,  14304,  14303,  14302,  14301,  14300,
    14299,  14298,  14297,  14296,  14295,  14294,  14293,  14292,  14291,  14290,  14289,  14288,  14287,  14286,  14285,  14284,  14283,  14282,  14281,  14280,  14279,  14278,  14277,  14276,  14275,  14274,  14273,  14272,  14271,  14270,  14269,  14268,
    14267,  14266,  14265,  14264,  14263,  14262,  14261,  14260,  14259,  14258,  14257,  14256,  14255,  14254,  14253,  14252,  14251,  14250,  14249,  14248,  14247,  14246,  14245,  14244,  14243,  14242,  14241,  14240,  14239,  14238,  14237,  14236,
    14235,  14234,  14233,  14232,  14231,  14230,  14229,  14228,  14227,  14226,  14225,  14224,  14223,  14222,  14221,  14220,  14219,  14218,  14217,  14216,  14215,  14214,  14213,  14212,  14211,  14210,  14209,  14208,  14207,  14206,  14205,  14204,
    14203,  14202,  14201,  14200,  14199,  14198,  14197,  14196,  14195,  14194,  14193,  14192,  14191,  14190,  14189,  14188,  14187,  14186,  14185,  14184,  14183,  14182,  14181,  14180,  14179,  14178,  14177,  14176,  14175,  14174,  14173,  14172,
    14171,  14170,  14169,  14168,  14167,  14166,  14165,  14164,  14163,  14162,  14161,  14160,  14159,  14158,  14157,  14156,  14155,  14154,  14153,  14152,  14151,  14150,  14149,  14148,  14147,  14146,  14145,  14144,  14143,  14142,  14141,  14140,
    14139,  14138,  14137,  14136,  14135,  14134,  14133,  14132,  14131,  14130,  14129,  14128,  14127,  14126,  14125,  14124,  14123,  14122,  14121,  14120,  14119,  14118,  14117,  14116,  14115,  14114,  14113,  14112,  14111,  14110,  14109,  14108,
    14107,  14106,  14105,  14104,  14103,  14102,  14101,  14100,  14099,  14098,  14097,  14096,  14095,  14094,  14093,  14092,  14091,  14090,  14089,  14088,  14087,  14086,  14085,  14084,  14083,  14082,  14081,  14080,  14079,  14078,  14077,  14076,
    14075,  14074,  14073,  14072,  14071,  14070,  14069,  14068,  14067,  14066,  14065,  14064,  14063,  14062,  14061,  14060,  14059,  14058,  14057,  14056,  14055,  14054,  14053,  14052,  14051,  14050,  14049,  14048,  14047,  14046,  14045,  14044,
    14043,  14042,  14041,  14040,  14039,  14038,  14037,  14036,  14035,  14034,  14033,  14032,  14031,  14030,  14029,  14028,  14027,  14026,  14025,  14024,  14023,  14022,  14021,  14020,  14019,  14018,  14017,  14016,  14015,  14014,  14013,  14012,
    14011,  14010,  14009,  14008,  14007,  14006,  14005,  14004,  14003,  14002,  14001,  14000,  13999,  13998,  13997,  13996,  13995,  13994,  13993,  13992,  13991,  13990,  13989,  13988,  13987,  13986,  13985,  13984,  13983,  13982,  13981,  13980,
    13979,  13978,  13977,  13976,  13975,  13974,  13973,  13972,  13971,  13970,  13969,  13968,  13967,  13966,  13965,  13964,  13963,  13962,  13961,  13960,  13959,  13958,  13957,  13956,  13955,  13954,  13953,  13952,  13951,  13950,  13949,  13948,
    13947,  13946,  13945,  13944,  13943,  13942,  13941,  13940,  13939,  13938,  13937,  13936,  13935,  13934,  13933,  13932,  13931,  13930,  13929,  13928,  13927,  13926,  13925,  13924,  13923,  13922,  13921,  13920,  13919,  13918,  13917,  13916,
    13915,  13914,  13913,  13912,  13911,  13910,  13909,  13908,  13907,  13906,  13905,  13904,  13903,  13902,  13901,  13900,  13899,  13898,  13897,  13896,  13895,  13894,  13893,  13892,  13891,  13890,  13889,  13888,  13887,  13886,  13885,  13884,
    13883,  13882,  13881,  13880,  13879,  13878,  13877,  13876,  13875,  13874,  13873,  13872,  13871,  13870,  13869,  13868,  13867,  13866,  13865,  13864,  13863,  13862,  13861,  13860,  13859,  13858,  13857,  13856,  13855,  13854,  13853,  13852,
    13851,  13850,  13849,  13848,  13847,  13846,  13845,  13844,  13843,  13842,  13841,  13840,  13839,  13838,  13837,  13836,  13835,  13834,  13833,  13832,  13831,  13830,  13829,  13828,  13827,  13826,  13825,  13824,  13823,  13822,  13821,  13820,
    13819,  13818,  13817,  13816,  13815,  13814,  13813,  13812,  13811,  13810,  13809,  13808,  13807,  13806,  13805,  13804,  13803,  13802,  13801,  13800,  13799,  13798,  13797,  13796,  13795,  13794,  13793,  13792,  13791,  13790,  13789,  13788,
    13787,  13786,  13785,  13784,  13783,  13782,  13781,  13780,  13779,  13778,  13777,  13776,  13775,  13774,  13773,  13772,  13771,  13770,  13769,  13768,  13767,  13766,  13765,  13764,  13763,  13762,  13761,  13760,  13759,  13758,  13757,  13756,
    13755,  13754,  13753,  13752,  13751,  13750,  13749,  13748,  13747,  13746,  13745,  13744,  13743,  13742,  13741,  13740,  13739,  13738,  13737,  13736,  13735,  13734,  13733,  13732,  13731,  13730,  13729,  13728,  13727,  13726,  13725,  13724,
    13723,  13722,  13721,  13720,  13719,  13718,  13717,  13716,  13715,  13714,  13713,  13712,  13711,  13710,  13709,  13708,  13707,  13706,  13705,  13704,  13703,  13702,  13701,  13700,  13699,  13698,  13697,  13696,  13695,  13694,  13693,  13692,
    13691,  13690,  13689,  13688,  13687,  13686,  13685,  13684,  13683,  13682,  13681,  13680,  13679,  13678,  13677,  13676,  13675,  13674,  13673,  13672,  13671,  13670,  13669,  13668,  13667,  13666,  13665,  13664,  13663,  13662,  13661,  13660,
    13659,  13658,  13657,  13656,  13655,  13654,  13653,  13652,  13651,  13650,  13649,  13648,  13647,  13646,  13645,  13644,  13643,  13642,  13641,  13640,  13639,  13638,  13637,  13636,  13635,  13634,  13633,  13632,  13631,  13630,  13629,  13628,
    13627,  13626,  13625,  13624,  13623,  13622,  13621,  13620,  13619,  13618,  13617,  13616,  13615,  13614,  13613,  13612,  13611,  13610,  13609,  13608,  13607,  13606,  13605,  13604,  13603,  13602,  13601,  13600,  13599,  13598,  13597,  13596,
    13595,  13594,  13593,  13592,  13591,  13590,  13589,  13588,  13587,  13586,  13585,  13584,  13583,  13582,  13581,  13580,  13579,  13578,  13577,  13576,  13575,  13574,  13573,  13572,  13571,  13570,  13569,  13568,  13567,  13566,  13565,  13564,
    13563,  13562,  13561,  13560,  13559,  13558,  13557,  13556,  13555,  13554,  13553,  13552,  13551,  13550,  13549,  13548,  13547,  13546,  13545,  13544,  13543,  13542,  13541,  13540,  13539,  13538,  13537,  13536,  13535,  13534,  13533,  13532,
    13531,  13530,  13529,  13528,  13527,  13526,  13525,  13524,  13523,  13522,  13521,  13520,  13519,  13518,  13517,  13516,  13515,  13514,  13513,  13512,  13511,  13510,  13509,  13508,  13507,  13506,  13505,  13504,  13503,  13502,  13501,  13500,
    13499,  13498,  13497,  13496,  13495,  13494,  13493,  13492,  13491,  13490,  13489,  13488,  13487,  13486,  13485,  13484,  13483,  13482,  13481,  13480,  13479,  13478,  13477,  13476,  13475,  13474,  13473,  13472,  13471,  13470,  13469,  13468,
    13467,  13466,  13465,  13464,  13463,  13462,  13461,  13460,  13459,  13458,  13457,  13456,  13455,  13454,  13453,  13452,  13451,  13450,  13449,  13448,  13447,  13446,  13445,  13444,  13443,  13442,  13441,  13440,  13439,  13438,  13437,  13436,
    13435,  13434,  13433,  13432,  13431,  13430,  13429,  13428,  13427,  13426,  13425,  13424,  13423,  13422,  13421,  13420,  13419,  13418,  13417,  13416,  13415,  13414,  13413,  13412,  13411,  13410,  13409,  13408,  13407,  13406,  13405,  13404,
    13403,  13402,  13401,  13400,  13399,  13398,  13397,  13396,  13395,  13394,  13393,  13392,  13391,  13390,  13389,  13388,  13387,  13386,  13385,  13384,  13383,  13382,  13381,  13380,  13379,  13378,  13377,  13376,  13375,  13374,  13373,  13372,
    13371,  13370,  13369,  13368,  13367,  13366,  13365,  13364,  13363,  13362,  13361,  13360,  13359,  13358,  13357,  13356,  13355,  13354,  13353,  13352,  13351,  13350,  13349,  13348,  13347,  13346,  13345,  13344,  13343,  13342,  13341,  13340,
    13339,  13338,  13337,  13336,  13335,  13334,  13333,  13332,  13331,  13330,  13329,  13328,  13327,  13326,  13325,  13324,  13323,  13322,  13321,  13320,  13319,  13318,  13317,  13316,  13315,  13314,  13313,  13312,  13311,  13310,  13309,  13308,
    13307,  13306,  13305,  13304,  13303,  13302,  13301,  13300,  13299,  13298,  13297,  13296,  13295,  13294,  13293,  13292,  13291,  13290,  13289,  13288,  13287,  13286,  13285,  13284,  13283,  13282,  13281,  13280,  13279,  13278,  13277,  13276,
    13275,  13274,  13273,  13272,  13271,  13270,  13269,  13268,  13267,  13266,  13265,  13264,  13263,  13262,  13261,  13260,  13259,  13258,  13257,  13256,  13255,  13254,  13253,  13252,  13251,  13250,  13249,  13248,  13247,  13246,  13245,  13244,
    13243,  13242,  13241,  13240,  13239,  13238,  13237,  13236,  13235,  13234,  13233,  13232,  13231,  13230,  13229,  13228,  13227,  13226,  13225,  13224,  13223,  13222,  13221,  13220,  13219,  13218,  13217,  13216,  13215,  13214,  13213,  13212,
    13211,  13210,  13209,  13208,  13207,  13206,  13205,  13204,  13203,  13202,  13201,  13200,  13199,  13198,  13197,  13196,  13195,  13194,  13193,  13192,  13191,  13190,  13189,  13188,  13187,  13186,  13185,  13184,  13183,  13182,  13181,  13180,
    13179,  13178,  13177,  13176,  13175,  13174,  13173,  13172,  13171,  13170,  13169,  13168,  13167,  13166,  13165,  13164,  13163,  13162,  13161,  13160,  13159,  13158,  13157,  13156,  13155,  13154,  13153,  13152,  13151,  13150,  13149,  13148,
    13147,  13146,  13145,  13144,  13143,  13142,  13141,  13140,  13139,  13138,  13137,  13136,  13135,  13134,  13133,  13132,  13131,  13130,  13129,  13128,  13127,  13126,  13125,  13124,  13123,  13122,  13121,  13120,  13119,  13118,  13117,  13116,
    13115,  13114,  13113,  13112,  13111,  13110,  13109,  13108,  13107,  13106,  13105,  13104,  13103,  13102,  13101,  13100,  13099,  13098,  13097,  13096,  13095,  13094,  13093,  13092,  13091,  13090,  13089,  13088,  13087,  13086,  13085,  13084,
    13083,  13082,  13081,  13080,  13079,  13078,  13077,  13076,  13075,  13074,  13073,  13072,  13071,  13070,  13069,  13068,  13067,  13066,  13065,  13064,  13063,  13062,  13061,  13060,  13059,  13058,  13057,  13056,  13055,  13054,  13053,  13052,
    13051,  13050,  13049,  13048,  13047,  13046,  13045,  13044,  13043,  13042,  13041,  13040,  13039,  13038,  13037,  13036,  13035,  13034,  13033,  13032,  13031,  13030,  13029,  13028,  13027,  13026,  13025,  13024,  13023,  13022,  13021,  13020,
    13019,  13018,  13017,  13016,  13015,  13014,  13013,  13012,  13011,  13010,  13009,  13008,  13007,  13006,  13005,  13004,  13003,  13002,  13001,  13000,  12999,  12998,  12997,  12996,  12995,  12994,  12993,  12992,  12991,  12990,  12989,  12988,
    12987,  12986,  12985,  12984,  12983,  12982,  12981,  12980,  12979,  12978,  12977,  12976,  12975,  12974,  12973,  12972,  12971,  12970,  12969,  12968,  12967,  12966,  12965,  12964,  12963,  12962,  12961,  12960,  12959,  12958,  12957,  12956,
    12955,  12954,  12953,  12952,  12951,  12950,  12949,  12948,  12947,  12946,  12945,  12944,  12943,  12942,  12941,  12940,  12939,  12938,  12937,  12936,  12935,  12934,  12933,  12932,  12931,  12930,  12929,  12928,  12927,  12926,  12925,  12924,
    12923,  12922,  12921,  12920,  12919,  12918,  12917,  12916,  12915,  12914,  12913,  12912,  12911,  12910,  12909,  12908,  12907,  12906,  12905,  12904,  12903,  12902,  12901,  12900,  12899,  12898,  12897,  12896,  12895,  12894,  12893,  12892,
    12891,  12890,  12889,  12888,  12887,  12886,  12885,  12884,  12883,  12882,  12881,  12880,  12879,  12878,  12877,  12876,  12875,  12874,  12873,  12872,  12871,  12870,  12869,  12868,  12867,  12866,  12865,  12864,  12863,  12862,  12861,  12860,
    12859,  12858,  12857,  12856,  12855,  12854,  12853,  12852,  12851,  12850,  12849,  12848,  12847,  12846,  12845,  12844,  12843,  12842,  12841,  12840,  12839,  12838,  12837,  12836,  12835,  12834,  12833,  12832,  12831,  12830,  12829,  12828,
    12827,  12826,  12825,  12824,  12823,  12822,  12821,  12820,  12819,  12818,  12817,  12816,  12815,  12814,  12813,  12812,  12811,  12810,  12809,  12808,  12807,  12806,  12805,  12804,  12803,  12802,  12801,  12800,  12799,  12798,  12797,  12796,
    12795,  12794,  12793,  12792,  12791,  12790,  12789,  12788,  12787,  12786,  12785,  12784,  12783,  12782,  12781,  12780,  12779,  12778,  12777,  12776,  12775,  12774,  12773,  12772,  12771,  12770,  12769,  12768,  12767,  12766,  12765,  12764,
    12763,  12762,  12761,  12760,  12759,  12758,  12757,  12756,  12755,  12754,  12753,  12752,  12751,  12750,  12749,  12748,  12747,  12746,  12745,  12744,  12743,  12742,  12741,  12740,  12739,  12738,  12737,  12736,  12735,  12734,  12733,  12732,
    12731,  12730,  12729,  12728,  12727,  12726,  12725,  12724,  12723,  12722,  12721,  12720,  12719,  12718,  12717,  12716,  12715,  12714,  12713,  12712,  12711,  12710,  12709,  12708,  12707,  12706,  12705,  12704,  12703,  12702,  12701,  12700,
    12699,  12698,  12697,  12696,  12695,  12694,  12693,  12692,  12691,  12690,  12689,  12688,  12687,  12686,  12685,  12684,  12683,  12682,  12681,  12680,  12679,  12678,  12677,  12676,  12675,  12674,  12673,  12672,  12671,  12670,  12669,  12668,
    12667,  12666,  12665,  12664,  12663,  12662,  12661,  12660,  12659,  12658,  12657,  12656,  12655,  12654,  12653,  12652,  12651,  12650,  12649,  12648,  12647,  12646,  12645,  12644,  12643,  12642,  12641,  12640,  12639,  12638,  12637,  12636,
    12635,  12634,  12633,  12632,  12631,  12630,  12629,  12628,  12627,  12626,  12625,  12624,  12623,  12622,  12621,  12620,  12619,  12618,  12617,  12616,  12615,  12614,  12613,  12612,  12611,  12610,  12609,  12608,  12607,  12606,  12605,  12604,
    12603,  12602,  12601,  12600,  12599,  12598,  12597,  12596,  12595,  12594,  12593,  12592,  12591,  12590,  12589,  12588,  12587,  12586,  12585,  12584,  12583,  12582,  12581,  12580,  12579,  12578,  12577,  12576,  12575,  12574,  12573,  12572,
    12571,  12570,  12569,  12568,  12567,  12566,  12565,  12564,  12563,  12562,  12561,  12560,  12559,  12558,  12557,  12556,  12555,  12554,  12553,  12552,  12551,  12550,  12549,  12548,  12547,  12546,  12545,  12544,  12543,  12542,  12541,  12540,
    12539,  12538,  12537,  12536,  12535,  12534,  12533,  12532,  12531,  12530,  12529,  12528,  12527,  12526,  12525,  12524,  12523,  12522,  12521,  12520,  12519,  12518,  12517,  12516,  12515,  12514,  12513,  12512,  12511,  12510,  12509,  12508,
    12507,  12506,  12505,  12504,  12503,  12502,  12501,  12500,  12499,  12498,  12497,  12496,  12495,  12494,  12493,  12492,  12491,  12490,  12489,  12488,  12487,  12486,  12485,  12484,  12483,  12482,  12481,  12480,  12479,  12478,  12477,  12476,
    12475,  12474,  12473,  12472,  12471,  12470,  12469,  12468,  12467,  12466,  12465,  12464,  12463,  12462,  12461,  12460,  12459,  12458,  12457,  12456,  12455,  12454,  12453,  12452,  12451,  12450,  12449,  12448,  12447,  12446,  12445,  12444,
    12443,  12442,  12441,  12440,  12439,  12438,  12437,  12436,  12435,  12434,  12433,  12432,  12431,  12430,  12429,  12428,  12427,  12426,  12425,  12424,  12423,  12422,  12421,  12420,  12419,  12418,  12417,  12416,  12415,  12414,  12413,  12412,
    12411,  12410,  12409,  12408,  12407,  12406,  12405,  12404,  12403,  12402,  12401,  12400,  12399,  12398,  12397,  12396,  12395,  12394,  12393,  12392,  12391,  12390,  12389,  12388,  12387,  12386,  12385,  12384,  12383,  12382,  12381,  12380,
    12288,  12289,  12290,  12291,  12292,  12293,  12294,  12295,  12296,  12297,  12298,  12299,  12300,  12301,  12302,  12303,  12304,  12305,  12306,  12307,  12308,  12309,  12310,  12311,  12312,  12313,  12314,  12315,  12316,  12317,  12318,  12319,
    12320,  12321,  12322,  12323,  12324,  12325,  12326,  12327,  12328,  12329,  12330,  12331,  12332,  12333,  12334,  12335,  12336,  12337,  12338,  12339,  12340,  12341,  12342,  12343,  12344,  12345,  12346,  12347,  12348,  12349,  12350,  12351,
    12350,  12351,  12352,  12353,  12354,  12355,  12356,  12357,  12358,  12359,  12360,  12361,  12362,  12363,  12364,  12365,  12366,  12367,  12368,  12369,  12370,  12371,  12372,  12373,  12374,  12375,  12376,  12377,  12378,  12379,  12380,  12381,
    12382,  12383,  12384,  12385,  12386,  12387,  12388,  12389,  12390,  12391,  12392,  12393,  12394,  12395,  12396,  12397,  12398,  12399,  12400,  12401,  12402,  12403,  12404,  12405,  12406,  12407,  12408,  12409,  12410,  12411,  12412,  12413,
    12412,  12413,  12414,  12415,  12416,  12417,  12418,  12419,  12420,  12421,  12422,  12423,  12424,  12425,  12426,  12427,  12428,  12429,  12430,  12431,  12432,  12433,  12434,  12435,  12436,  12437,  12438,  12439,  12440,  12441,  12442,  12443,
    12444,  12445,  12446,  12447,  12448,  12449,  12450,  12451,  12452,  12453,  12454,  12455,  12456,  12457,  12458,  12459,  12460,  12461,  12462,  12463,  12464,  12465,  12466,  12467,  12468,  12469,  12470,  12471,  12472,  12473,  12474,  12475,
    12474,  12475,  12476,  12477,  12478,  12479,  12480,  12481,  12482,  12483,  12484,  12485,  12486,  12487,  12488,  12489,  12490,  12491,  12492,  12493,  12494,  12495,  12496,  12497,  12498,  12499,  12500,  12501,  12502,  12503,  12504,  12505,
    12506,  12507,  12508,  12509,  12510,  12511,  12512,  12513,  12514,  12515,  12516,  12517,  12518,  12519,  12520,  12521,  12522,  12523,  12524,  12525,  12526,  12527,  12528,  12529,  12530,  12531,  12532,  12533,  12534,  12535,  12536,  12537,
    12534,  12535,  12536,  12537,  12538,  12539,  12540,  12541,  12542,  12543,  12544,  12545,  12546,  12547,  12548,  12549,  12550,  12551,  12552,  12553,  12554,  12555,  12556,  12557,  12558,  12559,  12560,  12561,  12562,  12563,  12564,  12565,
    12566,  12567,  12568,  12569,  12570,  12571,  12572,  12573,  12574,  12575,  12576,  12577,  12578,  12579,  12580,  12581,  12582,  12583,  12584,  12585,  12586,  12587,  12588,  12589,  12590,  12591,  12592,  12593,  12594,  12595,  12596,  12597,
    12596,  12597,  12598,  12599,  12600,  12601,  12602,  12603,  12604,  12605,  12606,  12607,  12608,  12609,  12610,  12611,  12612,  12613,  12614,  12615,  12616,  12617,  12618,  12619,  12620,  12621,  12622,  12623,  12624,  12625,  12626,  12627,
    12628,  12629,  12630,  12631,  12632,  12633,  12634,  12635,  12636,  12637,  12638,  12639,  12640,  12641,  12642,  12643,  12644,  12645,  12646,  12647,  12648,  12649,  12650,  12651,  12652,  12653,  12654,  12655,  12656,  12657,  12658,  12659,
    12658,  12659,  12660,  12661,  12662,  12663,  12664,  12665,  12666,  12667,  12668,  12669,  12670,  12671,  12672,  12673,  12674,  12675,  12676,  12677,  12678,  12679,  12680,  12681,  12682,  12683,  12684,  12685,  12686,  12687,  12688,  12689,
    12690,  12691,  12692,  12693,  12694,  12695,  12696,  12697,  12698,  12699,  12700,  12701,  12702,  12703,  12704,  12705,  12706,  12707,  12708,  12709,  12710,  12711,  12712,  12713,  12714,  12715,  12716,  12717,  12718,  12719,  12720,  12721,
    12720,  12721,  12722,  12723,  12724,  12725,  12726,  12727,  12728,  12729,  12730,  12731,  12732,  12733,  12734,  12735,  12736,  12737,  12738,  12739,  12740,  12741,  12742,  12743,  12744,  12745,  12746,  12747,  12748,  12749,  12750,  12751,
    12752,  12753,  12754,  12755,  12756,  12757,  12758,  12759,  12760,  12761,  12762,  12763,  12764,  12765,  12766,  12767,  12768,  12769,  12770,  12771,  12772,  12773,  12774,  12775,  12776,  12777,  12778,  12779,  12780,  12781,  12782,  12783,
    12780,  12781,  12782,  12783,  12784,  12785,  12786,  12787,  12788,  12789,  12790,  12791,  12792,  12793,  12794,  12795,  12796,  12797,  12798,  12799,  12800,  12801,  12802,  12803,  12804,  12805,  12806,  12807,  12808,  12809,  12810,  12811,
    12812,  12813,  12814,  12815,  12816,  12817,  12818,  12819,  12820,  12821,  12822,  12823,  12824,  12825,  12826,  12827,  12828,  12829,  12830,  12831,  12832,  12833,  12834,  12835,  12836,  12837,  12838,  12839,  12840,  12841,  12842,  12843,
    12842,  12843,  12844,  12845,  12846,  12847,  12848,  12849,  12850,  12851,  12852,  12853,  12854,  12855,  12856,  12857,  12858,  12859,  12860,  12861,  12862,  12863,  12864,  12865,  12866,  12867,  12868,  12869,  12870,  12871,  12872,  12873,
    12874,  12875,  12876,  12877,  12878,  12879,  12880,  12881,  12882,  12883,  12884,  12885,  12886,  12887,  12888,  12889,  12890,  12891,  12892,  12893,  12894,  12895,  12896,  12897,  12898,  12899,  12900,  12901,  12902,  12903,  12904,  12905,
    12904,  12905,  12906,  12907,  12908,  12909,  12910,  12911,  12912,  12913,  12914,  12915,  12916,  12917,  12918,  12919,  12920,  12921,  12922,  12923,  12924,  12925,  12926,  12927,  12928,  12929,  12930,  12931,  12932,  12933,  12934,  12935,
    12936,  12937,  12938,  12939,  12940,  12941,  12942,  12943,  12944,  12945,  12946,  12947,  12948,  12949,  12950,  12951,  12952,  12953,  12954,  12955,  12956,  12957,  12958,  12959,  12960,  12961,  12962,  12963,  12964,  12965,  12966,  12967,
    12966,  12967,  12968,  12969,  12970,  12971,  12972,  12973,  12974,  12975,  12976,  12977,  12978,  12979,  12980,  12981,  12982,  12983,  12984,  12985,  12986,  12987,  12988,  12989,  12990,  12991,  12992,  12993,  12994,  12995,  12996,  12997,
    12998,  12999,  13000,  13001,  13002,  13003,  13004,  13005,  13006,  13007,  13008,  13009,  13010,  13011,  13012,  13013,  13014,  13015,  13016,  13017,  13018,  13019,  13020,  13021,  13022,  13023,  13024,  13025,  13026,  13027,  13028,  13029,
    13026,  13027,  13028,  13029,  13030,  13031,  13032,  13033,  13034,  13035,  13036,  13037,  13038,  13039,  13040,  13041,  13042,  13043,  13044,  13045,  13046,  13047,  13048,  13049,  13050,  13051,  13052,  13053,  13054,  13055,  13056,  13057,
    13058,  13059,  13060,  13061,  13062,  13063,  13064,  13065,  13066,  13067,  13068,  13069,  13070,  13071,  13072,  13073,  13074,  13075,  13076,  13077,  13078,  13079,  13080,  13081,  13082,  13083,  13084,  13085,  13086,  13087,  13088,  13089,
    13088,  13089,  13090,  13091,  13092,  13093,  13094,  13095,  13096,  13097,  13098,  13099,  13100,  13101,  13102,  13103,  13104,  13105,  13106,  13107,  13108,  13109,  13110,  13111,  13112,  13113,  13114,  13115,  13116,  13117,  13118,  13119,
    13120,  13121,  13122,  13123,  13124,  13125,  13126,  13127,  13128,  13129,  13130,  13131,  13132,  13133,  13134,  13135,  13136,  13137,  13138,  13139,  13140,  13141,  13142,  13143,  13144,  13145,  13146,  13147,  13148,  13149,  13150,  13151,
    13150,  13151,  13152,  13153,  13154,  13155,  13156,  13157,  13158,  13159,  13160,  13161,  13162,  13163,  13164,  13165,  13166,  13167,  13168,  13169,  13170,  13171,  13172,  13173,  13174,  13175,  13176,  13177,  13178,  13179,  13180,  13181,
    13182,  13183,  13184,  13185,  13186,  13187,  13188,  13189,  13190,  13191,  13192,  13193,  13194,  13195,  13196,  13197,  13198,  13199,  13200,  13201,  13202,  13203,  13204,  13205,  13206,  13207,  13208,  13209,  13210,  13211,  13212,  13213,
    13212,  13213,  13214,  13215,  13216,  13217,  13218,  13219,  13220,  13221,  13222,  13223,  13224,  13225,  13226,  13227,  13228,  13229,  13230,  13231,  13232,  13233,  13234,  13235,  13236,  13237,  13238,  13239,  13240,  13241,  13242,  13243,
    13244,  13245,  13246,  13247,  13248,  13249,  13250,  13251,  13252,  13253,  13254,  13255,  13256,  13257,  13258,  13259,  13260,  13261,  13262,  13263,  13264,  13265,  13266,  13267,  13268,  13269,  13270,  13271,  13272,  13273,  13274,  13275,
    13272,  13273,  13274,  13275,  13276,  13277,  13278,  13279,  13280,  13281,  13282,  13283,  13284,  13285,  13286,  13287,  13288,  13289,  13290,  13291,  13292,  13293,  13294,  13295,  13296,  13297,  13298,  13299,  13300,  13301,  13302,  13303,
    13304,  13305,  13306,  13307,  13308,  13309,  13310,  13311,  13312,  13313,  13314,  13315,  13316,  13317,  13318,  13319,  13320,  13321,  13322,  13323,  13324,  13325,  13326,  13327,  13328,  13329,  13330,  13331,  13332,  13333,  13334,  13335,
    13334,  13335,  13336,  13337,  13338,  13339,  13340,  13341,  13342,  13343,  13344,  13345,  13346,  13347,  13348,  13349,  13350,  13351,  13352,  13353,  13354,  13355,  13356,  13357,  13358,  13359,  13360,  13361,  13362,  13363,  13364,  13365,
    13366,  13367,  13368,  13369,  13370,  13371,  13372,  13373,  13374,  13375,  13376,  13377,  13378,  13379,  13380,  13381,  13382,  13383,  13384,  13385,  13386,  13387,  13388,  13389,  13390,  13391,  13392,  13393,  13394,  13395,  13396,  13397,
    13396,  13397,  13398,  13399,  13400,  13401,  13402,  13403,  13404,  13405,  13406,  13407,  13408,  13409,  13410,  13411,  13412,  13413,  13414,  13415,  13416,  13417,  13418,  13419,  13420,  13421,  13422,  13423,  13424,  13425,  13426,  13427,
    13428,  13429,  13430,  13431,  13432,  13433,  13434,  13435,  13436,  13437,  13438,  13439,  13440,  13441,  13442,  13443,  13444,  13445,  13446,  13447,  13448,  13449,  13450,  13451,  13452,  13453,  13454,  13455,  13456,  13457,  13458,  13459,
    13458,  13459,  13460,  13461,  13462,  13463,  13464,  13465,  13466,  13467,  13468,  13469,  13470,  13471,  13472,  13473,  13474,  13475,  13476,  13477,  13478,  13479,  13480,  13481,  13482,  13483,  13484,  13485,  13486,  13487,  13488,  13489,
    13490,  13491,  13492,  13493,  13494,  13495,  13496,  13497,  13498,  13499,  13500,  13501,  13502,  13503,  13504,  13505,  13506,  13507,  13508,  13509,  13510,  13511,  13512,  13513,  13514,  13515,  13516,  13517,  13518,  13519,  13520,  13521,
    13518,  13519,  13520,  13521,  13522,  13523,  13524,  13525,  13526,  13527,  13528,  13529,  13530,  13531,  13532,  13533,  13534,  13535,  13536,  13537,  13538,  13539,  13540,  13541,  13542,  13543,  13544,  13545,  13546,  13547,  13548,  13549,
    13550,  13551,  13552,  13553,  13554,  13555,  13556,  13557,  13558,  13559,  13560,  13561,  13562,  13563,  13564,  13565,  13566,  13567,  13568,  13569,  13570,  13571,  13572,  13573,  13574,  13575,  13576,  13577,  13578,  13579,  13580,  13581,
    13580,  13581,  13582,  13583,  13584,  13585,  13586,  13587,  13588,  13589,  13590,  13591,  13592,  13593,  13594,  13595,  13596,  13597,  13598,  13599,  13600,  13601,  13602,  13603,  13604,  13605,  13606,  13607,  13608,  13609,  13610,  13611,
    13612,  13613,  13614,  13615,  13616,  13617,  13618,  13619,  13620,  13621,  13622,  13623,  13624,  13625,  13626,  13627,  13628,  13629,  13630,  13631,  13632,  13633,  13634,  13635,  13636,  13637,  13638,  13639,  13640,  13641,  13642,  13643,
    13642,  13643,  13644,  13645,  13646,  13647,  13648,  13649,  13650,  13651,  13652,  13653,  13654,  13655,  13656,  13657,  13658,  13659,  13660,  13661,  13662,  13663,  13664,  13665,  13666,  13667,  13668,  13669,  13670,  13671,  13672,  13673,
    13674,  13675,  13676,  13677,  13678,  13679,  13680,  13681,  13682,  13683,  13684,  13685,  13686,  13687,  13688,  13689,  13690,  13691,  13692,  13693,  13694,  13695,  13696,  13697,  13698,  13699,  13700,  13701,  13702,  13703,  13704,  13705,
    13704,  13705,  13706,  13707,  13708,  13709,  13710,  13711,  13712,  13713,  13714,  13715,  13716,  13717,  13718,  13719,  13720,  13721,  13722,  13723,  13724,  13725,  13726,  13727,  13728,  13729,  13730,  13731,  13732,  13733,  13734,  13735,
    13736,  13737,  13738,  13739,  13740,  13741,  13742,  13743,  13744,  13745,  13746,  13747,  13748,  13749,  13750,  13751,  13752,  13753,  13754,  13755,  13756,  13757,  13758,  13759,  13760,  13761,  13762,  13763,  13764,  13765,  13766,  13767,
    13764,  13765,  13766,  13767,  13768,  13769,  13770,  13771,  13772,  13773,  13774,  13775,  13776,  13777,  13778,  13779,  13780,  13781,  13782,  13783,  13784,  13785,  13786,  13787,  13788,  13789,  13790,  13791,  13792,  13793,  13794,  13795,
    13796,  13797,  13798,  13799,  13800,  13801,  13802,  13803,  13804,  13805,  13806,  13807,  13808,  13809,  13810,  13811,  13812,  13813,  13814,  13815,  13816,  13817,  13818,  13819,  13820,  13821,  13822,  13823,  13824,  13825,  13826,  13827,
    13826,  13827,  13828,  13829,  13830,  13831,  13832,  13833,  13834,  13835,  13836,  13837,  13838,  13839,  13840,  13841,  13842,  13843,  13844,  13845,  13846,  13847,  13848,  13849,  13850,  13851,  13852,  13853,  13854,  13855,  13856,  13857,
    13858,  13859,  13860,  13861,  13862,  13863,  13864,  13865,  13866,  13867,  13868,  13869,  13870,  13871,  13872,  13873,  13874,  13875,  13876,  13877,  13878,  13879,  13880,  13881,  13882,  13883,  13884,  13885,  13886,  13887,  13888,  13889,
    13888,  13889,  13890,  13891,  13892,  13893,  13894,  13895,  13896,  13897,  13898,  13899,  13900,  13901,  13902,  13903,  13904,  13905,  13906,  13907,  13908,  13909,  13910,  13911,  13912,  13913,  13914,  13915,  13916,  13917,  13918,  13919,
    13920,  13921,  13922,  13923,  13924,  13925,  13926,  13927,  13928,  13929,  13930,  13931,  13932,  13933,  13934,  13935,  13936,  13937,  13938,  13939,  13940,  13941,  13942,  13943,  13944,  13945,  13946,  13947,  13948,  13949,  13950,  13951,
    13950,  13951,  13952,  13953,  13954,  13955,  13956,  13957,  13958,  13959,  13960,  13961,  13962,  13963,  13964,  13965,  13966,  13967,  13968,  13969,  13970,  13971,  13972,  13973,  13974,  13975,  13976,  13977,  13978,  13979,  13980,  13981,
    13982,  13983,  13984,  13985,  13986,  13987,  13988,  13989,  13990,  13991,  13992,  13993,  13994,  13995,  13996,  13997,  13998,  13999,  14000,  14001,  14002,  14003,  14004,  14005,  14006,  14007,  14008,  14009,  14010,  14011,  14012,  14013,
    14010,  14011,  14012,  14013,  14014,  14015,  14016,  14017,  14018,  14019,  14020,  14021,  14022,  14023,  14024,  14025,  14026,  14027,  14028,  14029,  14030,  14031,  14032,  14033,  14034,  14035,  14036,  14037,  14038,  14039,  14040,  14041,
    14042,  14043,  14044,  14045,  14046,  14047,  14048,  14049,  14050,  14051,  14052,  14053,  14054,  14055,  14056,  14057,  14058,  14059,  14060,  14061,  14062,  14063,  14064,  14065,  14066,  14067,  14068,  14069,  14070,  14071,  14072,  14073,
    14072,  14073,  14074,  14075,  14076,  14077,  14078,  14079,  14080,  14081,  14082,  14083,  14084,  14085,  14086,  14087,  14088,  14089,  14090,  14091,  14092,  14093,  14094,  14095,  14096,  14097,  14098,  14099,  14100,  14101,  14102,  14103,
    14104,  14105,  14106,  14107,  14108,  14109,  14110,  14111,  14112,  14113,  14114,  14115,  14116,  14117,  14118,  14119,  14120,  14121,  14122,  14123,  14124,  14125,  14126,  14127,  14128,  14129,  14130,  14131,  14132,  14133,  14134,  14135,
    14134,  14135,  14136,  14137,  14138,  14139,  14140,  14141,  14142,  14143,  14144,  14145,  14146,  14147,  14148,  14149,  14150,  14151,  14152,  14153,  14154,  14155,  14156,  14157,  14158,  14159,  14160,  14161,  14162,  14163,  14164,  14165,
    14166,  14167,  14168,  14169,  14170,  14171,  14172,  14173,  14174,  14175,  14176,  14177,  14178,  14179,  14180,  14181,  14182,  14183,  14184,  14185,  14186,  14187,  14188,  14189,  14190,  14191,  14192,  14193,  14194,  14195,  14196,  14197,
    14196,  14197,  14198,  14199,  14200,  14201,  14202,  14203,  14204,  14205,  14206,  14207,  14208,  14209,  14210,  14211,  14212,  14213,  14214,  14215,  14216,  14217,  14218,  14219,  14220,  14221,  14222,  14223,  14224,  14225,  14226,  14227,
    14228,  14229,  14230,  14231,  14232,  14233,  14234,  14235,  14236,  14237,  14238,  14239,  14240,  14241,  14242,  14243,  14244,  14245,  14246,  14247,  14248,  14249,  14250,  14251,  14252,  14253,  14254,  14255,  14256,  14257,  14258,  14259
};

tab_s16 * const tbl_plps_base = tbl_plps + 2048;

static uavs3d_always_inline int lbac_get_shift(int v)
{
#ifdef _WIN32
    unsigned long index;
    _BitScanReverse(&index, v);
    return 8 - index;
#else
    return __builtin_clz(v) - 23;
#endif
}

static uavs3d_always_inline int lbac_get_log2(int v)
{
#ifdef _WIN32
    unsigned long index;
    _BitScanReverse(&index, v);
    return index;
#else
    return 31 - __builtin_clz(v);
#endif
}

static uavs3d_always_inline int lbac_refill2(com_lbac_t *lbac, u32 low)
{
    int x = low ^ (low - 1);
    int i = 7 - lbac_get_shift(x >> 15);

    x = -CABAC_MASK;

#if CHECK_RAND_STRM
    x += ((rand() & 0xFF) << 9) + ((rand() & 0xFF) << 1);
#else
    x += (lbac->cur[0] << 9) + (lbac->cur[1] << 1);
#endif
    lbac->cur = COM_MIN(lbac->cur + 2, lbac->end);

    return low + (x << i);
}

void lbac_init(com_lbac_t *lbac, u8 *cur, u8* end)
{
    lbac->range = MAX_RANGE;
    lbac->cur = cur + 3;
    lbac->end = end;
    lbac->low = (cur[0] << 18) | (cur[1] << 10) | (cur[2] << 2) | 2;
}

static u32 uavs3d_always_inline lbac_dec_bin_inline(com_lbac_t * lbac, lbac_ctx_model_t * model)
{
    s32 bin;
    s32 scaled_rMPS;
    s32 lps_mask;
    s32 RangeLPS = *model;
    s32 s = RangeLPS & PROB_MASK;
    s32 rMPS = lbac->range - (s >> (PROB_LPS_SHIFTNO + 1));
    int s_flag = rMPS < HALF_RANGE;

    rMPS |= 0x100;
    scaled_rMPS = rMPS << (CABAC_BITS + 1 - s_flag);

    lbac->range = (lbac->range << s_flag) - rMPS;
    lps_mask = (scaled_rMPS - lbac->low) >> 31;

    lbac->low -= scaled_rMPS & lps_mask;
    lbac->range += (rMPS - lbac->range) & (~lps_mask);

    s ^= lps_mask;
    *model = tbl_plps_base[(int)(RangeLPS & 0xf000) + s];

    bin = s & 1;

    lps_mask = lbac_get_shift(lbac->range);
    lbac->range <<= lps_mask;
    lbac->low <<= lps_mask + s_flag;

    if (!(lbac->low & CABAC_MASK)) {
        lbac->low = lbac_refill2(lbac, lbac->low);
    }

    return bin;
}

static u32 lbac_dec_bin(com_lbac_t * lbac, lbac_ctx_model_t * model)
{
    return lbac_dec_bin_inline(lbac, model);
}

static u32 uavs3d_always_inline lbac_dec_binW(com_lbac_t * lbac, lbac_ctx_model_t * model1, lbac_ctx_model_t * model2)
{
    s32 bin;
    s32 scaled_rMPS;
    s32 lps_mask;
    s32 RangeLPS1 = *model1;
    s32 RangeLPS2 = *model2;
    s32 s1 = RangeLPS1 & PROB_MASK;
    s32 s2 = RangeLPS2 & PROB_MASK;
    s32 rMPS;
    int s_flag;
    u16 prob_lps;
    u16 prob_lps1 = s1 >> 1;
    u16 prob_lps2 = s2 >> 1;
    u16 cmps;
    u16 cmps1 = (*model1) & 1;
    u16 cmps2 = (*model2) & 1;
  
    if (cmps1 == cmps2) {
        cmps = cmps1;
        prob_lps = (prob_lps1 + prob_lps2) >> 1;
    } else {
        if (prob_lps1 < prob_lps2) {
            cmps = cmps1;
            prob_lps = (256 << PROB_LPS_SHIFTNO) - 1 - ((prob_lps2 - prob_lps1) >> 1);
        } else {
            cmps = cmps2;
            prob_lps = (256 << PROB_LPS_SHIFTNO) - 1 - ((prob_lps1 - prob_lps2) >> 1);
        }
    }

    rMPS = lbac->range - (prob_lps >> PROB_LPS_SHIFTNO);
    s_flag = rMPS < HALF_RANGE;

    rMPS |= 0x100;
    scaled_rMPS = rMPS << (CABAC_BITS + 1 - s_flag);

    lbac->range = (lbac->range << s_flag) - rMPS;
    lps_mask = (scaled_rMPS - lbac->low) >> 31;

    lbac->low -= scaled_rMPS & lps_mask;
    lbac->range += (rMPS - lbac->range) & (~lps_mask);

    bin = cmps ^ (lps_mask & 1);
    s1 ^= (bin == cmps1) ? 0 : 0xFFFFFFFF;
    s2 ^= (bin == cmps2) ? 0 : 0xFFFFFFFF;
    *model1 = tbl_plps_base[(int)(RangeLPS1 & 0xf000) + s1];
    *model2 = tbl_plps_base[(int)(RangeLPS2 & 0xf000) + s2];

    lps_mask = lbac_get_shift(lbac->range);
    lbac->range <<= lps_mask;
    lbac->low <<= lps_mask + s_flag;

    if (!(lbac->low & CABAC_MASK)) {
        lbac->low = lbac_refill2(lbac, lbac->low);
    }

    return bin;
}

static uavs3d_always_inline u32 lbac_dec_bin_ep_inline(com_lbac_t * lbac)
{
    s32 scaled_rMPS = lbac->range << CABAC_BITS;
    s32 lps_mask = (scaled_rMPS - lbac->low) >> 31;
    lbac->low -= scaled_rMPS & lps_mask;
    lbac->low <<= 1;

    if (!(lbac->low & CABAC_MASK)) {
        lbac->low = lbac_refill2(lbac, lbac->low);
    }
    return lps_mask;
}

static u32 lbac_dec_bin_ep(com_lbac_t * lbac) 
{
    return lbac_dec_bin_ep_inline(lbac);
}

int lbac_dec_bin_trm(com_lbac_t * lbac)
{
    int bin;
    s32 scaled_rMPS;
    s32 lps_mask;
    s32 rMPS = lbac->range - 1;
    s32 s_flag = (rMPS < HALF_RANGE);

    rMPS |= 0x100;
    scaled_rMPS = rMPS << (CABAC_BITS + 1 - s_flag);
    lps_mask = (scaled_rMPS - lbac->low) >> 31;
    bin = lps_mask & 1;

    lbac->range = (lbac->range << s_flag) - rMPS;
    lbac->low -= scaled_rMPS & lps_mask;
    lbac->range += (rMPS - lbac->range) & (~lps_mask);

    lps_mask = lbac_get_shift(lbac->range);
    lbac->range <<= lps_mask;
    lbac->low <<= lps_mask + s_flag;

    if (!(lbac->low & CABAC_MASK)) {
        lbac->low = lbac_refill2(lbac, lbac->low);
    }

    return bin;
}

static uavs3d_always_inline u32 lbac_read_bins_ep_msb(com_lbac_t * lbac, int num_bin)
{
    int bins = 0;
    u32 val = 0;
    s32 range = lbac->range;
    s32 low = lbac->low;
    s32 scaled_rMPS = range << CABAC_BITS;

    for (bins = num_bin - 1; bins >= 0; bins--) {
        s32 lps_mask = (scaled_rMPS - low) >> 31;
        int bin = lps_mask & 1;
        low -= scaled_rMPS & lps_mask;
        low <<= 1;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }

        val = (val << 1) | bin;
    }

    lbac->range = range;
    lbac->low = low;

    return val;
}

static uavs3d_always_inline u32 lbac_read_bins_ep_msb_ext(com_lbac_t * lbac, int num_bin)
{
    int bins = 0;
    u32 val = 0;
    s32 range = lbac->range;
    s32 low = lbac->low;
    s32 scaled_rMPS = range << CABAC_BITS;

    for (bins = 0; bins < num_bin; bins++) {
        s32 lps_mask = (scaled_rMPS - low) >> 31;
        int bin = lps_mask & 1;
        low -= scaled_rMPS & lps_mask;
        low <<= 1;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }

        val |= bin << bins;
    }

    lbac->range = range;
    lbac->low = low;

    return val;
}

static uavs3d_always_inline u32 lbac_read_unary_sym_ep(com_lbac_t * lbac)
{
    u32 val = 0;
    u32 bin;
    s32 range = lbac->range;
    s32 low = lbac->low;
    s32 scaled_rMPS = range << CABAC_BITS;
    s32 lps_mask;

    do {
        lps_mask = (scaled_rMPS - low) >> 31;
        bin = !lps_mask;
        low -= scaled_rMPS & lps_mask;
        low <<= 1;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }
        val += bin;
    } while (bin && lbac->cur < lbac->end);

    lbac->range = range;
    lbac->low = low;

    return val;
}

static uavs3d_always_inline u32 lbac_read_truncate_unary_sym_ep(com_lbac_t * lbac, u32 max_num)
{
    u32 val = 0;
    u32 bin;
    s32 range = lbac->range;
    s32 low = lbac->low;
    s32 scaled_rMPS = range << CABAC_BITS;

    do {
        s32 lps_mask = (scaled_rMPS - low) >> 31;
        bin = !lps_mask;
        low -= scaled_rMPS & lps_mask;
        low <<= 1;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }

        val += bin;
    } while (val < max_num - 1 && bin);

    lbac->range = range;
    lbac->low = low;

    return val;
}

static uavs3d_always_inline u32 lbac_read_bins_msb(com_lbac_t * lbac, lbac_ctx_model_t *model, int num_bin)
{
    int bins = 0;
    u32 val = 0;
    s32 range = lbac->range;
    s32 low = lbac->low;

    for (bins = num_bin - 1; bins >= 0; bins--) {
        s32 scaled_rMPS;
        s32 lps_mask;
        s32 RangeLPS = *model;
        s32 s = RangeLPS & PROB_MASK;
        s32 rMPS = range - (s >> (PROB_LPS_SHIFTNO + 1));
        int s_flag = rMPS < HALF_RANGE;
        int bin;

        rMPS |= 0x100;
        scaled_rMPS = rMPS << (CABAC_BITS + 1 - s_flag);

        range = (range << s_flag) - rMPS;
        lps_mask = (scaled_rMPS - low) >> 31;

        low -= scaled_rMPS & lps_mask;
        range += (rMPS - range) & (~lps_mask);

        s ^= lps_mask;
        *model++ = tbl_plps_base[(int)(RangeLPS & 0xf000) + s];

        bin = (s & 1);

        lps_mask = lbac_get_shift(range);
        range <<= lps_mask;
        low <<= lps_mask + s_flag;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }
        val = (val << 1) | bin;
    }

    lbac->range = range;
    lbac->low = low;

    return val;
}

static uavs3d_always_inline u32 lbac_read_truncate_unary_sym(com_lbac_t * lbac, lbac_ctx_model_t * model, u32 num_ctx, u32 max_num)
{
    u32 val = 0;
    u32 bin;
    s32 range = lbac->range;
    s32 low = lbac->low;

    do {
        lbac_ctx_model_t *ctx = model + COM_MIN(val, num_ctx - 1);
        s32 scaled_rMPS;
        s32 lps_mask;
        s32 RangeLPS = *ctx;
        s32 s = RangeLPS & PROB_MASK;
        s32 rMPS = range - (s >> (PROB_LPS_SHIFTNO + 1));
        int s_flag = rMPS < HALF_RANGE;

        rMPS |= 0x100;
        scaled_rMPS = rMPS << (CABAC_BITS + 1 - s_flag);

        range = (range << s_flag) - rMPS;
        lps_mask = (scaled_rMPS - low) >> 31;

        low -= scaled_rMPS & lps_mask;
        range += (rMPS - range) & (~lps_mask);

        s ^= lps_mask;
        *ctx = tbl_plps_base[(int)(RangeLPS & 0xf000) + s];

        bin = !(s & 1);

        lps_mask = lbac_get_shift(range);
        range <<= lps_mask;
        low <<= lps_mask + s_flag;

        if (!(low & CABAC_MASK)) {
            low = lbac_refill2(lbac, low);
        }
        val += bin;
    } while (val < max_num - 1 && bin);

    lbac->range = range;
    lbac->low = low;

    return val;
}

void dec_parse_ipcm_start(com_lbac_t *lbac)
{
    int low = lbac->low;
    int x = low ^ (low - 1);
    int flag_idx = lbac_get_log2(x);

    while (flag_idx <= 8) {
        lbac->cur--;
        flag_idx += 8;
    }
}

int dec_parse_ipcm(com_lbac_t *lbac, int *bit_left, int bits)
{
    int t1 = lbac->cur[0] & ((1 << *bit_left) - 1);
    bits -= *bit_left;
    *bit_left = 8 - bits;

    lbac->cur = COM_MIN(lbac->cur + 1, lbac->end);

    if (bits) {
        int t2 = lbac->cur[0] >> *bit_left;
        if (*bit_left == 0) {
            lbac->cur = COM_MIN(lbac->cur + 1, lbac->end);
            *bit_left = 8;
        }
        return (t1 << bits) | t2;
    } else {
        return t1;
    }
}

static uavs3d_always_inline int dec_parse_cbf_uv(com_lbac_t * lbac, u8 *cbf)
{
    com_lbac_all_ctx_t * sbac_ctx = &lbac->ctx;

    cbf[0] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 1);
    cbf[1] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 2);

    return RET_OK;
}

static int dec_parse_cbf(com_core_t *core, com_lbac_t * lbac, int cu_log2w, int cu_log2h)
{
    com_lbac_all_ctx_t * sbac_ctx = &lbac->ctx;
    int i, part_num;
    u8 *cbfy = core->cbfy;
    u8 *cbfc = core->cbfc;
    u8 tree_status = core->tree_status;

    /* decode allcbf */
    if(core->cu_mode != MODE_INTRA) {
        if (core->cu_mode != MODE_DIR && tree_status == TREE_LC) {
            if (lbac_dec_bin(lbac, sbac_ctx->ctp_zero_flag + (cu_log2w > 6 || cu_log2h > 6))) {
                core->tb_part = SIZE_2Nx2N;
                return RET_OK;
            }
        }
        static tab_u8 tb_avaliable_tab[8][8] = {
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 1, 1, 0, 0, 0 },
            { 0, 0, 0, 1, 1, 1, 0, 0 },
            { 0, 0, 0, 0, 1, 1, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 }
        };
        if (core->seqhdr->position_based_transform_enable_flag && tb_avaliable_tab[cu_log2w][cu_log2h]) {
            core->tb_part = lbac_dec_bin(lbac, sbac_ctx->tb_split) ? SIZE_NxN : SIZE_2Nx2N;
        } else {
            core->tb_part = SIZE_2Nx2N;
        }
        if (tree_status == TREE_LC) {
            cbfc[0] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 1);
            cbfc[1] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 2);
        }
        if(!M16(cbfc) && core->tb_part == SIZE_2Nx2N && tree_status == TREE_LC) {
            cbfy[0] = 1;
        } else {
            part_num = dec_get_part_num(core->tb_part);
            for (i = 0; i < part_num; i++) {
                cbfy[i] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf);
            }
        }
        if (cu_log2w > 6 || cu_log2h > 6) {
            uavs3d_assert(cbfy[0] == 0);
            cbfy[0] = 0; 
        }
    } else {
        if (core->ipm[0] != IPD_IPCM) {
            core->tb_part = dec_get_tb_part_size_by_pb(core->pb_part);
            part_num = dec_get_part_num(core->tb_part);
            for (i = 0; i < part_num; i++) {
                cbfy[i] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf);
            }
        }
        if (tree_status == TREE_LC && (core->ipm[0] != IPD_IPCM || core->ipm_c != IPD_DM_C)) {
            cbfc[0] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 1);
            cbfc[1] = (u8)lbac_dec_bin(lbac, sbac_ctx->cbf + 2);
        }
    }
    return RET_OK;
}

static uavs3d_always_inline u32 dec_parse_run(com_lbac_t *lbac, lbac_ctx_model_t *model)
{
    u32 act_sym = lbac_read_truncate_unary_sym(lbac, model, 2, 17);

    if (act_sym == 16) {
        int golomb_order = lbac_read_unary_sym_ep(lbac);
        act_sym += (1 << golomb_order) - 1 + lbac_read_bins_ep_msb(lbac, golomb_order);
    }
    return act_sym;
}

static uavs3d_always_inline u32 dec_parse_level(com_lbac_t *lbac, lbac_ctx_model_t *model)
{
    u32 act_sym = lbac_read_truncate_unary_sym(lbac, model, 2, 9);

    if (act_sym == 8) {
        int golomb_order = lbac_read_unary_sym_ep(lbac);
        act_sym += (1 << golomb_order) - 1 + lbac_read_bins_ep_msb(lbac, golomb_order);
    }
    return act_sym;
}

static tab_u8 tbl_dq_shift[80] =   // [64 + 16]
{
    14, 14, 14, 14, 14, 14, 14, 14, 
    13, 13, 13, 13, 13, 13, 13, 13, 
    13, 12, 12, 12, 12, 12, 12, 12, 
    11, 11, 11, 11, 11, 11, 11, 11, 
    11, 10, 10, 10, 10, 10, 10, 10, 
    10,  9,  9,  9,  9,  9,  9,  9, 
     9,  8,  8,  8,  8,  8,  8,  8, 
     7,  7,  7,  7,  7,  7,  7,  7, 
     6,  6,  6,  6,  6,  6,  6,  6, 
     5,  5,  5,  5,  5,  5,  5,  5, 
};

static tab_u16 tbl_dq_scale[80] =   // [64 + 16]
{
    32768, 36061, 38968, 42495, 46341, 50535, 55437, 60424,
    32932, 35734, 38968, 42495, 46177, 50535, 55109, 59933,
    65535, 35734, 38968, 42577, 46341, 50617, 55027, 60097,
    32809, 35734, 38968, 42454, 46382, 50576, 55109, 60056,
    65535, 35734, 38968, 42495, 46320, 50515, 55109, 60076,
    65535, 35744, 38968, 42495, 46341, 50535, 55099, 60087,
    65535, 35734, 38973, 42500, 46341, 50535, 55109, 60097,
    32771, 35734, 38965, 42497, 46341, 50535, 55109, 60099,
    32768, 36061, 38968, 42495, 46341, 50535, 55437, 60424,
    32932, 35734, 38968, 42495, 46177, 50535, 55109, 59933
};

int dec_parse_run_length_cc(com_core_t *core, s16 *coef, int log2_w, int log2_h, int ch_type)
{
    com_lbac_t *lbac = &core->lbac;
    com_lbac_all_ctx_t  *sbac_ctx = &lbac->ctx;
    int            last_flag;
    const u16     *scanp = g_tbl_scan + g_tbl_scan_blkpos[log2_h - 1][log2_w - 1];
    int num_coeff = 1 << (log2_w + log2_h);
    int scan_pos_offset = 0;
    int prev_level = 5;
    lbac_ctx_model_t *ctx_last1 = sbac_ctx->last1 + (ch_type == Y_C ? 0 : LBAC_CTX_NUM_LAST1);
    lbac_ctx_model_t *ctx_last2 = sbac_ctx->last2 + (ch_type == Y_C ? 0 : LBAC_CTX_NUM_LAST2);
    lbac_ctx_model_t *ctx_run = sbac_ctx->run   + (ch_type == Y_C ? 0 : 12);
    lbac_ctx_model_t *ctx_lvl = sbac_ctx->level + (ch_type == Y_C ? 0 : 12);

    int qp = ch_type == Y_C ? core->lcu_qp_y : (ch_type == U_C ? core->lcu_qp_u : core->lcu_qp_v);
    int log2_size = (log2_w + log2_h) >> 1;
    int refix = (log2_w + log2_h) & 1;
    int scale = tbl_dq_scale[qp];
    int shift = tbl_dq_shift[qp] - GET_TRANS_SHIFT(core->seqhdr->bit_depth_internal, log2_size) + 1; // +1 is used to compensate for the mismatching of shifts in quantization and inverse quantization
    int offset = (shift == 0) ? 0 : (1 << (shift - 1));
    int mask = (1 << log2_w) - 1;
    com_pic_header_t *pichdr = core->pichdr;
    int weight_quant = pichdr->pic_wq_enable;
    u8* wq = NULL;
    int wq_width;
    int idx_shift;
    int idx_step;

    if (weight_quant) {
        if (log2_w == 2 && log2_h == 2) {
            wq = pichdr->wq_4x4_matrix;
            idx_shift = 0;
            idx_step = 1;
            wq_width = 4;
        } else {
            wq = pichdr->wq_8x8_matrix;
            idx_shift = max(log2_w, log2_h) - 3;
            idx_step = 1 << idx_shift;
            wq_width = 8;
        }
    }

    memset(coef, 0, sizeof(s16) * num_coeff);

    do {
        int ctx_add   = prev_level << 1;
        int run       = dec_parse_run  (lbac, ctx_run + ctx_add);                             /* Run   parsing */
        int abs_level = dec_parse_level(lbac, ctx_lvl + ctx_add) + 1;                         /* Level parsing */
        s16 level = lbac_dec_bin_ep_inline(lbac) ? -(s16)abs_level : (s16)abs_level;     /* Sign  parsing */
        int x, y, scan_idx;

        scan_pos_offset += run;
        scan_pos_offset = COM_MIN(scan_pos_offset, num_coeff);
        scan_idx = scanp[scan_pos_offset];
        x = scan_idx & mask;
        y = scan_idx >> log2_w;

        /* de-quant */
        if (!((x | y) & 0xE0)) {
            
#define DQUANT_WQ(c, wq, scale, offset, shift) (((((((c)*(wq))>>2)*(scale))>>4)+(offset))>>(shift))
#define DQUANT(c, scale, offset, shift) (((c)*(scale)+(offset))>>(shift))

            if (weight_quant) {
                int weight = wq[(y >> idx_shift) * wq_width + (x >> idx_shift)];
                int lev = DQUANT_WQ(level, weight, (long long)scale, offset, shift);
                lev = (s16)COM_CLIP(lev, -32768, 32767);
                if (refix) {
                    lev = (lev * 181 + 128) >> 8;
                }
                coef[scan_idx] = (s16)lev;
            } else {
                int lev = DQUANT(level, scale, offset, shift);
                lev = (s16)COM_CLIP(lev, -32768, 32767);
                if (refix) {
                    lev = (lev * 181 + 128) >> 8;
                }
                coef[scan_idx] = (s16)lev;
            }
        }

        if (scan_pos_offset >= num_coeff - 1) {
            break;
        }

        /* Last flag parsing */
        last_flag = lbac_dec_binW(lbac, ctx_last1 + prev_level, ctx_last2 + lbac_get_log2(scan_pos_offset + 1));

        prev_level = COM_MIN(abs_level - 1, 5);
        scan_pos_offset++;
    }
    while (!last_flag);

    return RET_OK;
}

static int uavs3d_always_inline decode_refidx(com_lbac_t * lbac, int num_refp)
{
    int ref_num = 0;

    if(num_refp > 1) {
        ref_num = lbac_read_truncate_unary_sym(lbac, lbac->ctx.refi, 3, num_refp);
    }
    return ref_num;
}

u8 dec_parse_cons_pred_mode_child(com_lbac_t * lbac)
{
    return lbac_dec_bin(lbac, lbac->ctx.cons_mode) ? ONLY_INTRA : ONLY_INTER;
}

static u8 uavs3d_always_inline decode_skip_flag(com_core_t *core, com_lbac_t * lbac, com_scu_t *map_scu, int i_scu, int cu_log2w, int cu_log2h)
{
    int ctx_inc;
    
    if (cu_log2w + cu_log2h < 6) {
        ctx_inc = 3;
    } else {
        ctx_inc = map_scu[- i_scu].skip + map_scu[- 1].skip;
    }

    return (u8)lbac_dec_bin_inline(lbac, lbac->ctx.skip_flag + ctx_inc);
}

static u8 uavs3d_always_inline decode_umve_flag(com_lbac_t * lbac)
{
    return lbac_dec_bin(lbac, &lbac->ctx.umve_flag);
}

static u8 uavs3d_always_inline decode_umve_idx(com_lbac_t * lbac)
{
    int base_idx = !lbac_dec_bin(lbac, lbac->ctx.umve_base_idx);
    int step_idx = 0, dir_idx;
 
    if (!lbac_dec_bin(lbac, lbac->ctx.umve_step_idx)) {
        step_idx = lbac_read_truncate_unary_sym_ep(lbac, UMVE_REFINE_STEP - 1) + 1;
    }
    dir_idx  = lbac_dec_bin(lbac, &lbac->ctx.umve_dir_idx[0]) << 1;
    dir_idx += lbac_dec_bin(lbac, &lbac->ctx.umve_dir_idx[1]);
   
    return (base_idx * UMVE_MAX_REFINE_NUM + step_idx * 4 + dir_idx);

}

static u8 decode_split_flag(com_core_t *core, com_lbac_t * lbac, int cu_width, int cu_height)
{
    int ctx = 0;
 
    if (core->slice_type == SLICE_I && cu_width == 128 && cu_height == 128) {
        ctx = 3;
    } else {
        int i_scu = core->seqhdr->i_scu;
        int scup = core->tree_scup;
        com_scu_t *scu = core->map.map_scu + scup;
        u32     *pos = core->map.map_pos + scup;

        if (scu[-i_scu].coded) {
            ctx += (1 << MCU_GET_LOGW(pos[-i_scu])) < cu_width;
        }
        if (scu[-1].coded) {
            ctx += (1 << MCU_GET_LOGH(pos[-1])) < cu_height;
        }
    }

    return lbac_dec_bin(lbac, lbac->ctx.split_flag + ctx);
}

static u8 uavs3d_always_inline decode_affine_flag(com_core_t *core, com_lbac_t * lbac, com_seqh_t *seqhdr, int cu_log2w, int cu_log2h)
{
    return cu_log2w >= AFF_BITSIZE && cu_log2h >= AFF_BITSIZE && seqhdr->affine_enable_flag && lbac_dec_bin_inline(lbac, lbac->ctx.affine_flag);
}

static u8 uavs3d_always_inline decode_affine_mrg_idx(com_lbac_t * lbac)
{
    return lbac_read_truncate_unary_sym(lbac, lbac->ctx.affine_mrg_idx, LBAC_CTX_NUM_AFFINE_MRG, AFF_MAX_NUM_MRG);
}

static int uavs3d_always_inline decode_smvd_flag(com_lbac_t * lbac)
{
    return lbac_dec_bin(lbac, lbac->ctx.smvd_flag );
}

static int decode_part_size(com_core_t *core, com_lbac_t * lbac)
{
    int part_size = SIZE_2Nx2N;
    int allowDT, dir;
    com_seqh_t *seqhdr = core->seqhdr;

    if (!seqhdr->dt_intra_enable_flag || core->cu_mode != MODE_INTRA || 
        !(allowDT = dec_dt_allow(core->cu_width, core->cu_height, core->cu_mode, seqhdr->max_dt_size))) {
        return SIZE_2Nx2N;
    }

    if (lbac_dec_bin(lbac, lbac->ctx.part_size + 0)) {
        if (allowDT == 0x3) {
            dir = lbac_dec_bin(lbac, lbac->ctx.part_size + 1);
        } else {
            dir = allowDT & 0x1;
        }
        if (dir) { //hori
            if (lbac_dec_bin(lbac, lbac->ctx.part_size + 2)) {
                part_size = SIZE_2NxhN;
            } else {
                part_size = lbac_dec_bin(lbac, lbac->ctx.part_size + 3) ? SIZE_2NxnD : SIZE_2NxnU;
            }
        } else { //vert
            if (lbac_dec_bin(lbac, lbac->ctx.part_size + 4)) {
                part_size = SIZE_hNx2N;
            } else {
                part_size = lbac_dec_bin(lbac, lbac->ctx.part_size + 5) ? SIZE_nRx2N : SIZE_nLx2N;
            }
        }
    } 

    return part_size;
}

static u8 uavs3d_always_inline decode_pred_mode(com_core_t *core, com_lbac_t * lbac, com_scu_t *map_scu, int i_scu, int cu_log2w, int cu_log2h)
{
    int ctx_inc;

    if (cu_log2w > 6 || cu_log2h > 6) {
        ctx_inc = 5;
    } else {
        ctx_inc = map_scu[- i_scu].intra + map_scu[- 1].intra;

        if (ctx_inc == 0) {
            int sample = 1 << (cu_log2w + cu_log2h);
            ctx_inc = (sample > 256) ? 0 : (sample > 64 ? 3 : 4);
        }
    }
    return lbac_dec_bin_inline(lbac, lbac->ctx.pred_mode + ctx_inc) ? MODE_INTRA : MODE_INTER;
}

static int uavs3d_always_inline decode_mvr_idx(com_lbac_t * lbac, BOOL is_affine)
{
    if (is_affine) {
        return lbac_read_truncate_unary_sym(lbac, lbac->ctx.affine_mvr_idx, LBAC_CTX_NUM_AFFINE_MVR_IDX, MAX_NUM_AFFINE_MVR);
    } else {
        return lbac_read_truncate_unary_sym(lbac, lbac->ctx.mvr_idx, LBAC_CTX_NUM_MVR_IDX, MAX_NUM_MVR);
    }
}

static int uavs3d_always_inline decode_extend_amvr_flag(com_lbac_t * lbac)
{
    return lbac_dec_bin_inline(lbac, lbac->ctx.mvp_from_hmvp_flag);
}

static int uavs3d_always_inline decode_ipf_flag(com_lbac_t * lbac)
{
    return lbac_dec_bin(lbac, lbac->ctx.ipf_flag);
}

static u32 dec_parse_abs_mvd(com_lbac_t *lbac, lbac_ctx_model_t *model)
{
    u32 act_sym;

    if (!lbac_dec_bin(lbac, model)) {
        act_sym = 0;
    } else if (!lbac_dec_bin(lbac, model + 1)) {
        act_sym = 1;
    } else if (!lbac_dec_bin(lbac, model + 2)) {
        act_sym = 2;
    } else {
        int add_one = lbac_dec_bin_ep(lbac) & 1;
        int golomb_order = lbac_read_unary_sym_ep(lbac);
        act_sym = (1 << golomb_order) - 1 + lbac_read_bins_ep_msb(lbac, golomb_order);
        act_sym = 3 + act_sym * 2 + add_one;
    }

    return act_sym;
}

static int uavs3d_always_inline decode_mvd(com_lbac_t * lbac, s16 mvd[MV_D])
{
    /* MV_X */
    mvd[MV_X] = (s16)dec_parse_abs_mvd(lbac, lbac->ctx.mvd[0]);

    if (mvd[MV_X]) {
        mvd[MV_X] = lbac_dec_bin_ep(lbac) ? -mvd[MV_X] : mvd[MV_X];
    }
    /* MV_Y */
    mvd[MV_Y] = (s16)dec_parse_abs_mvd(lbac, lbac->ctx.mvd[1]);

    if (mvd[MV_Y]) {
        mvd[MV_Y] = lbac_dec_bin_ep(lbac) ? -mvd[MV_Y] : mvd[MV_Y];
    }
  
    return RET_OK;
}

static int decode_intra_dir(com_lbac_t * lbac, u8 mpm[2])
{
    int ipm;

    if (lbac_dec_bin(lbac, lbac->ctx.intra_dir)) {
        ipm = lbac_dec_bin(lbac, lbac->ctx.intra_dir + 6) - 2;
    } else {
        ipm = lbac_read_bins_msb(lbac, lbac->ctx.intra_dir + 1, 5);
    }
    ipm = (ipm < 0) ? mpm[ipm + 2] : (ipm + (ipm >= mpm[0]) + ((ipm + 1) >= mpm[1]));

    return ipm;
}

static int decode_intra_dir_c(com_lbac_t * lbac, u8 ipm_l, u8 tscpm_enable_flag)
{
    int ipm = 0;

    if (!lbac_dec_bin(lbac, lbac->ctx.intra_dir + 7)) {
        u8 chk_bypass = COM_IPRED_CHK_CONV(ipm_l);

        if (chk_bypass) {
            ipm_l = (ipm_l == IPD_VER) ? IPD_VER_C : (ipm_l == IPD_HOR ? IPD_HOR_C : (ipm_l == IPD_DC ? IPD_DC_C : IPD_BI_C));
        }
        if (tscpm_enable_flag) {
            if (lbac_dec_bin(lbac, lbac->ctx.intra_dir + 9)) {
                return IPD_TSCPM_C;
            }
        }
        ipm = 1 + lbac_read_truncate_unary_sym(lbac, lbac->ctx.intra_dir + 8, 1, IPD_CHROMA_CNT - 1);

        if (chk_bypass &&  ipm >= ipm_l) {
            ipm++;
        }
    }
    return ipm;
}

static int uavs3d_always_inline decode_inter_dir(com_core_t *core, com_lbac_t * lbac, int part_size, int cu_log2w, int cu_log2h)
{
    if (cu_log2w + cu_log2h < 6) {
        lbac_dec_bin(lbac, lbac->ctx.inter_dir + 2);
        return lbac_dec_bin(lbac, lbac->ctx.inter_dir + 1) ? PRED_L1 : PRED_L0;
    } else {
        if (lbac_dec_bin_inline(lbac, lbac->ctx.inter_dir)) {
            return PRED_BI;
        }else {
            return lbac_dec_bin_inline(lbac, lbac->ctx.inter_dir + 1) ? PRED_L1 : PRED_L0;
        }
    }
}

static u8 uavs3d_always_inline decode_skip_idx(com_core_t *core, com_lbac_t * lbac, int slice_type, int num_hmvp_cands)
{
    const u32 max_skip_num = (slice_type == SLICE_P ? 2 : TRADITIONAL_SKIP_NUM) + num_hmvp_cands;
    int skip_idx = lbac_read_truncate_unary_sym(lbac, lbac->ctx.skip_idx_ctx, LBAC_CTX_NUM_SKIP_IDX, max_skip_num);
    return (u8)skip_idx + ((slice_type == SLICE_P && skip_idx > 0) << 1);
}

static u8 uavs3d_always_inline decode_direct_flag(com_core_t *core, com_lbac_t * lbac, int cu_log2w, int cu_log2h)
{
    int ctx_inc = (cu_log2w + cu_log2h < 6) || cu_log2w > 6 || cu_log2h > 6;
    return lbac_dec_bin_inline(lbac, lbac->ctx.direct_flag + ctx_inc);
}

s8 dec_parse_split_mode(com_core_t *core, com_lbac_t *lbac, int split_tab, int cu_width, int cu_height, int qt_depth, int bet_depth)
{
    if (split_tab & (1 << SPLIT_QUAD)) {
        if (split_tab == (1 << SPLIT_QUAD) || decode_split_flag(core, lbac, cu_width, cu_height)) {
            return SPLIT_QUAD;
        }
        if (core->slice_type == SLICE_I && cu_width == 128 && cu_height == 128) {
            uavs3d_assert(0);
            return SPLIT_QUAD; 
        }
    }
    if (split_tab & 0x1E) {
        int i_scu = core->seqhdr->i_scu;
        int scup = core->tree_scup;
        int s = cu_width * cu_height;
        com_scu_t *scu = core->map.map_scu + scup;
        u32     *pos = core->map.map_pos + scup;
        int ctx = (scu[-i_scu].coded && (1 << MCU_GET_LOGW(pos[-i_scu])) < cu_width ) +
                  (scu[-1    ].coded && (1 << MCU_GET_LOGH(pos[-1    ])) < cu_height);
        
        if ((split_tab & (1 << NO_SPLIT)) && !lbac_dec_bin_inline(lbac, lbac->ctx.bt_split_flag + ctx + ((s > 1024) ? 0 : (s > 256 ? 3 : 6)))) { 
            return NO_SPLIT;
        } else {
            int EnableBT  = split_tab & ((1 << SPLIT_BI_HOR ) | (1 << SPLIT_BI_VER ));
            int EnableEQT = split_tab & ((1 << SPLIT_EQT_HOR) | (1 << SPLIT_EQT_VER));
            u8 ctx_dir, split_dir = 0, split_typ = 0;

            if (EnableEQT && EnableBT) {
                split_typ = (u8)lbac_dec_bin(lbac, lbac->ctx.split_mode + ctx);
            } else if (EnableEQT) {
                split_typ = 1;
            }
            if ((cu_width == 128 && cu_height == 64) || (cu_width == 64 && cu_height == 128)) {
                uavs3d_assert(split_typ == 0);
                split_typ = 0;  
            }

            if (split_typ == 0) { // BT
                if (((split_tab >> SPLIT_BI_VER) & 3) == 3) {
                    if (cu_width == 64 && cu_height == 128) {
                        ctx_dir = 3;
                    } else if (cu_width == 128 && cu_height == 64) {
                        ctx_dir = 4;
                    } else {
                        ctx_dir = cu_width == cu_height ? 0 : (cu_width > cu_height ? 1 : 2);
                    }
                    split_dir = (u8)lbac_dec_bin(lbac, lbac->ctx.split_dir + ctx_dir);
                } else {
                    split_dir = split_tab & (1 << SPLIT_BI_VER);
                }
                if (cu_width == 128 && cu_height == 64) {
                    uavs3d_assert(split_dir == 1);
                    split_dir = 1; 
                } else if (cu_width == 64 && cu_height == 128) {
                    uavs3d_assert(split_dir == 0);
                    split_dir = 0; 
                }
                return split_dir ? SPLIT_BI_VER : SPLIT_BI_HOR;
            } else { // EQT
                if (((split_tab >> SPLIT_EQT_VER) & 3) == 3) {
                    ctx_dir = cu_width == cu_height ? 0 : (cu_width > cu_height ? 1 : 2);
                    split_dir = (u8)lbac_dec_bin(lbac, lbac->ctx.split_dir + ctx_dir);
                } else {
                    split_dir = split_tab & (1 << SPLIT_EQT_VER);
                }
                return split_dir ? SPLIT_EQT_VER : SPLIT_EQT_HOR;
            }
        }
    }
    return NO_SPLIT;
}

static void dec_parse_affine_mv(com_core_t *core, com_lbac_t *lbac, int mvr_idx, int inter_dir)
{
    s16 mvd[MV_D];
    int vertex;
    int vertex_num = core->affine_flag + 1;
    CPMV affine_mvp[VER_NUM][MV_D];

    /* forward */
    if (inter_dir == PRED_L0 || inter_dir == PRED_BI) {
        core->refi[REFP_0] = (s8)decode_refidx(lbac, core->num_refp[REFP_0]);
        dec_scale_affine_mvp(core, REFP_0, affine_mvp, mvr_idx);

        for (vertex = 0; vertex < vertex_num; vertex++) {
            decode_mvd(lbac, mvd);

            u8 amvr_shift = Tab_Affine_AMVR(mvr_idx);
            s32 mv_x = (s32)affine_mvp[vertex][MV_X] + ((s32)mvd[MV_X] << amvr_shift);
            s32 mv_y = (s32)affine_mvp[vertex][MV_Y] + ((s32)mvd[MV_Y] << amvr_shift);

            core->affine_mv[REFP_0][vertex][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, mv_x);
            core->affine_mv[REFP_0][vertex][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, mv_y);
        }
    } else {
        core->refi[REFP_0] = REFI_INVALID;
    }

    /* backward */
    if (inter_dir == PRED_L1 || inter_dir == PRED_BI) {
        core->refi[REFP_1] = (s8)decode_refidx(lbac, core->num_refp[REFP_1]);
        dec_scale_affine_mvp(core, REFP_1, affine_mvp, mvr_idx);

        for (vertex = 0; vertex < vertex_num; vertex++) {
            decode_mvd(lbac, mvd);

            u8 amvr_shift = Tab_Affine_AMVR(mvr_idx);
            s32 mv_x = (s32)affine_mvp[vertex][MV_X] + ((s32)mvd[MV_X] << amvr_shift);
            s32 mv_y = (s32)affine_mvp[vertex][MV_Y] + ((s32)mvd[MV_Y] << amvr_shift);

            core->affine_mv[REFP_1][vertex][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, mv_x);
            core->affine_mv[REFP_1][vertex][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, mv_y);
        }
    } else {
        core->refi[REFP_1] = REFI_INVALID;
    }
}

static void com_get_mpm(int x_scu, int y_scu, com_scu_t *map_scu, s8 *map_ipm, int scup, int i_scu, u8 mpm[2])
{
    u8 ipm_l = IPD_DC, ipm_u = IPD_DC;

    if (map_scu[scup - 1].intra) {
        ipm_l = map_ipm[scup - 1];
    }

    if (map_scu[scup - i_scu].intra) {
        ipm_u = map_ipm[scup - i_scu];
    }
    mpm[0] = COM_MIN(ipm_l, ipm_u);
    mpm[1] = COM_MAX(ipm_l, ipm_u);

    if (mpm[0] == mpm[1]) {
        mpm[0] = IPD_DC;
        mpm[1] = (mpm[1] == IPD_DC) ? IPD_BI : mpm[1];
    }
}

int dec_parse_cu_header_chroma(com_core_t * core)
{
    com_lbac_t *lbac = &core->lbac;
    com_scu_t *map_scu = core->map.map_scu;
    int scu_x = (core->cu_pix_x + core->cu_width  - 1) >> MIN_CU_LOG2;
    int scu_y = (core->cu_pix_y + core->cu_height - 1) >> MIN_CU_LOG2;
    int luma_scup = scu_x + scu_y * core->seqhdr->i_scu;

    if (map_scu[luma_scup].intra) {
        s8  *map_ipm = core->map.map_ipm;
        core->cu_mode = MODE_INTRA;
        core->ipm[PB0] = map_ipm[luma_scup];
        core->ipm_c = (s8)decode_intra_dir_c(lbac, core->ipm[PB0], core->seqhdr->tscpm_enable_flag);
    } else {
        s8(*map_refi)[REFP_NUM] = core->map.map_refi;
        s16(*map_mv)[REFP_NUM][MV_D] = core->map.map_mv;
        core->cu_mode = MODE_INTER;
        for (int lidx = 0; lidx < REFP_NUM; lidx++) {
            core->refi[lidx] = map_refi[luma_scup][lidx];
            CP32(core->mv[lidx], map_mv[luma_scup][lidx]);
        }
    }
    if ((core->cu_mode != MODE_INTRA || core->ipm[0] != IPD_IPCM || core->ipm_c != IPD_DM_C)) {
        dec_parse_cbf_uv(lbac, core->cbfc);
    }
    return RET_OK;
}

int dec_parse_cu_header(com_core_t * core) // this function can be optimized to better match with the text
{
    com_lbac_t *lbac = &core->lbac;
    com_seqh_t *seqhdr = core->seqhdr;
    com_scu_t *map_scu = core->map.map_scu;
    int scup = core->cu_scup;
    int i_scu = core->seqhdr->i_scu;
    int cu_log2w = core->cu_log2w;
    int cu_log2h = core->cu_log2h;

    core->tb_part = SIZE_2Nx2N;
    core->pb_part = SIZE_2Nx2N;

    if (core->slice_type != SLICE_I) {
        u8 skip_flag = 0;
        u8 direct_flag = 0;

        if (core->cons_pred_mode != ONLY_INTRA) {
            skip_flag = decode_skip_flag(core, lbac, map_scu + scup, i_scu, cu_log2w, cu_log2h);
            if (!skip_flag) {
                direct_flag = decode_direct_flag(core, lbac, cu_log2w, cu_log2h);
            }
        }
        if (skip_flag || direct_flag) {
            int umve_flag = 0;

            core->cu_mode = skip_flag ? MODE_SKIP : MODE_DIR;

            if (seqhdr->umve_enable_flag) {
                umve_flag = decode_umve_flag(lbac);
            }
            if (umve_flag) {
                core->affine_flag = 0;
                dec_derive_skip_mv_umve(core, decode_umve_idx(lbac));
            } else {
                core->affine_flag = decode_affine_flag(core, lbac, seqhdr, cu_log2w, cu_log2h);
                if (core->affine_flag) {
                    dec_derive_skip_mv_affine(core, decode_affine_mrg_idx(lbac));
                } else {
                    dec_derive_skip_mv(core, decode_skip_idx(core, lbac, core->slice_type, seqhdr->num_of_hmvp_cand));
                }
            }        
        } else {
            if (core->cons_pred_mode == NO_MODE_CONS) {
                core->cu_mode = decode_pred_mode(core, lbac, map_scu + scup, i_scu, cu_log2w, cu_log2h);
                if (cu_log2w > 6 || cu_log2h > 6) {
                    uavs3d_assert(core->cu_mode == MODE_INTER);
                    core->cu_mode = MODE_INTER; 
                }
            } else {
                core->cu_mode = core->cons_pred_mode == ONLY_INTRA ? MODE_INTRA : MODE_INTER;
            }
            core->affine_flag = 0;

            if (core->cu_mode != MODE_INTRA) {
                int inter_dir = PRED_L0;
                int mvp_from_hmvp_flag = 0;
                int mvr_idx = 0;

                core->affine_flag = decode_affine_flag(core, lbac, seqhdr, cu_log2w, cu_log2h);
                
                if (seqhdr->amvr_enable_flag) {
                    if (seqhdr->emvr_enable_flag && !core->affine_flag) { // also imply core->seqhdr->num_of_hmvp_cand is not zero
                        mvp_from_hmvp_flag = decode_extend_amvr_flag(lbac);
                    }
                    mvr_idx = (u8)decode_mvr_idx(lbac, core->affine_flag);
                }
                if (core->slice_type == SLICE_B) {
                    inter_dir = decode_inter_dir(core, lbac, core->pb_part, cu_log2w, cu_log2h);
                }
                if (core->affine_flag) {
                    dec_parse_affine_mv(core, lbac, mvr_idx, inter_dir);
                } else {
                    int smvd_flag = 0;
                    s16 mvp[MV_D], mvd[MV_D];

                    if (seqhdr->smvd_enable_flag && inter_dir == PRED_BI && !mvp_from_hmvp_flag && (core->refp[0][REFP_0].dist == -core->refp[0][REFP_1].dist)) {
                        smvd_flag = (u8)decode_smvd_flag(lbac);
                    } 

                    if (inter_dir == PRED_L0 || inter_dir == PRED_BI) {  /* forward */
                        if (smvd_flag == 1) {
                            core->refi[REFP_0] = 0;
                        } else {
                            core->refi[REFP_0] = (s8)decode_refidx(lbac, core->num_refp[REFP_0]);
                        }
                        dec_derive_mvp(core, REFP_0, mvp_from_hmvp_flag, mvr_idx, mvp);

                        decode_mvd(lbac, mvd);
                        s32 mv_x = (s32)mvp[MV_X] + ((s32)mvd[MV_X] << mvr_idx);
                        s32 mv_y = (s32)mvp[MV_Y] + ((s32)mvd[MV_Y] << mvr_idx);
                        core->mv[REFP_0][MV_X] = COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
                        core->mv[REFP_0][MV_Y] = COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
                    } else {
                        core->refi[REFP_0] = REFI_INVALID;
                    }
                    
                    if (inter_dir == PRED_L1 || inter_dir == PRED_BI) {  /* backward */
                        if (smvd_flag == 1) {
                            core->refi[REFP_1] = 0;
                        } else {
                            core->refi[REFP_1] = (s8)decode_refidx(lbac, core->num_refp[REFP_1]);
                        }
                        dec_derive_mvp(core, REFP_1, mvp_from_hmvp_flag, mvr_idx, mvp);

                        if (smvd_flag == 1) {
                            mvd[MV_X] = COM_MIN(COM_INT16_MAX, -mvd[MV_X]);
                            mvd[MV_Y] = COM_MIN(COM_INT16_MAX, -mvd[MV_Y]);
                        } else {
                            decode_mvd(lbac, mvd);

                        }
                        s32 mv_x = (s32)mvp[MV_X] + ((s32)mvd[MV_X] << mvr_idx);
                        s32 mv_y = (s32)mvp[MV_Y] + ((s32)mvd[MV_Y] << mvr_idx);

                        core->mv[REFP_1][MV_X] = COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
                        core->mv[REFP_1][MV_Y] = COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
                    } else {
                        core->refi[REFP_1] = REFI_INVALID;
                    }
                }

            }
        }
    } else { /* SLICE_I */
        core->cu_mode = MODE_INTRA;
    }

    if (core->cu_mode == MODE_INTRA) {
        com_part_info_t pb_info;
        int i_scu = seqhdr->i_scu;
        core->ipf_flag = 0;
        core->pb_part = decode_part_size(core, lbac);

        dec_get_part_info(core, &pb_info);

        for (int part_idx = 0; part_idx < pb_info.num_sub_part; part_idx++) {
            int pb_x = pb_info.sub_x[part_idx];
            int pb_y = pb_info.sub_y[part_idx];
            int pb_width  = pb_info.sub_w[part_idx];
            int pb_height = pb_info.sub_h[part_idx];
            int scu_x = pb_x >> MIN_CU_LOG2;
            int scu_y = pb_y >> MIN_CU_LOG2;
            u8 mpm[2];

            com_get_mpm(scu_x, scu_y, core->map.map_scu, core->map.map_ipm, scu_y * i_scu + scu_x, i_scu, mpm);
            core->ipm[part_idx] = (s8)decode_intra_dir(lbac, mpm);

            if (core->pb_part != SIZE_2Nx2N) {
                uavs3d_assert(core->ipm[part_idx] < IPD_IPCM);
                core->ipm[part_idx] = COM_MIN(core->ipm[part_idx], IPD_IPCM - 1);
            }

            M16(core->refi) = -1;
            dec_update_map_for_intra(core->map.map_scu, core->map.map_ipm, pb_x, pb_y, pb_width, pb_height, i_scu, core->ipm[part_idx]);
        }

        if (core->tree_status != TREE_L) {
            core->ipm_c = (s8)decode_intra_dir_c(lbac, core->ipm[PB0], seqhdr->tscpm_enable_flag);
        }
        if (!(core->tree_status == TREE_C && core->ipm[0] == IPD_IPCM  && core->ipm_c == IPD_DM_C) && 
            !(core->tree_status != TREE_C && core->ipm[0] == IPD_IPCM) && 
              seqhdr->ipf_enable_flag && 
              core->cu_width < MAX_CU_SIZE && core->cu_height < MAX_CU_SIZE && core->pb_part == SIZE_2Nx2N) {
            core->ipf_flag = decode_ipf_flag(lbac);
        }
        M16(core->refi) = -1;
    }

    /* parse coefficients */
    M32(core->cbfy) = M16(core->cbfc) = 0;

    if (core->cu_mode != MODE_SKIP) {
        dec_parse_cbf(core, lbac, cu_log2w, cu_log2h);
    }
    return RET_OK;
}

static int dec_parse_sao_mergeflag(com_lbac_t *lbac, int mergeleft_avail, int mergeup_avail)
{
    int act_ctx = mergeleft_avail + mergeup_avail;
    int act_sym = 0;

    if (act_ctx == 1)
    {
        act_sym = lbac_dec_bin(lbac, &lbac->ctx.sao_merge_flag[0]);
    }
    else if (act_ctx == 2)
    {
        act_sym = lbac_dec_bin(lbac, &lbac->ctx.sao_merge_flag[1]);
        if (act_sym != 1)
        {
            act_sym += (lbac_dec_bin(lbac, &lbac->ctx.sao_merge_flag[2]) << 1);
        }
    }

    /* Get SaoMergeMode by SaoMergeTypeIndex */
    if (mergeleft_avail && !mergeup_avail)
    {
        return act_sym ? 2 : 0;
    }
    if (mergeup_avail && !mergeleft_avail)
    {
        return act_sym ? 1 : 0;
    }
    return act_sym ? 3 - act_sym : 0;
}

static int dec_parse_sao_mode(com_lbac_t *lbac)
{
    if (!lbac_dec_bin(lbac, lbac->ctx.sao_mode)) {
        return 1 + !lbac_dec_bin_ep(lbac);
    } else {
        return 0;
    }
}

static int dec_parse_sao_offset(com_lbac_t *lbac, com_sao_param_t *saoBlkParam, int *offset)
{
    int i;

    static tab_s8 saoclip[NUM_SAO_OFFSET][3] = {
        { -1, 6, 7 }, //low bound, upper bound, threshold
        {  0, 1, 1 },
        {  0, 0, 0 },
        { -1, 0, 1 },
        { -6, 1, 7 },
        { -7, 7, 7 } //BO
    };

    offset[2] = 0;

    for (i = 0; i < 4; i++) {
        int act_sym;
        if (saoBlkParam->type == SAO_TYPE_BO) {
            act_sym = !lbac_dec_bin(lbac, lbac->ctx.sao_offset);
            if (act_sym) {
                act_sym += lbac_read_truncate_unary_sym_ep(lbac, saoclip[SAO_CLASS_BO][2]);

                if (lbac_dec_bin_ep(lbac)) {
                    act_sym = -act_sym;
                }
            }
            offset[i] = act_sym;
        } else {
            int idx = (i >= 2) ? (i + 1) : i;
            int maxvalue = saoclip[idx][2];
            int cnt = lbac_read_truncate_unary_sym_ep(lbac, maxvalue + 1);
            static tab_s8 EO_OFFSET_INV__MAP[] = { 1, 0, 2, -1, 3, 4, 5, 6 };

            if (idx == SAO_CLASS_EO_FULL_VALLEY) {
                act_sym = EO_OFFSET_INV__MAP[cnt];
            }
            else if (idx == SAO_CLASS_EO_FULL_PEAK) {
                act_sym = -EO_OFFSET_INV__MAP[cnt];
            }
            else if (idx == SAO_CLASS_EO_HALF_PEAK) {
                act_sym = -cnt;
            }
            else {
                act_sym = cnt;
            }
            offset[idx] = act_sym;
        }
        
    }
    return 1;
}

static int dec_parse_sao_eo_type(com_lbac_t *lbac, com_sao_param_t *saoBlkParam)
{
    return lbac_read_bins_ep_msb_ext(lbac, 2);
}

static void dec_parse_sao_bo_start(com_lbac_t *lbac, com_sao_param_t *saoBlkParam, int stBnd[4])
{
    int start = lbac_read_bins_ep_msb_ext(lbac, 5);
    int golomb_order = 1 + lbac_read_truncate_unary_sym_ep(lbac, 4);
    int act_sym = (1 << golomb_order) - 2 + lbac_read_bins_ep_msb(lbac, golomb_order % 4);

    stBnd[0] =  start;
    stBnd[1] = (start + 1) % NUM_BO_OFFSET;
    stBnd[2] = (start + 2 + act_sym) % NUM_BO_OFFSET;
    stBnd[3] = (start + 2 + act_sym + 1) % NUM_BO_OFFSET;;
}

static void copy_sao_param(com_sao_param_t *saopara_dst, com_sao_param_t *saopara_src)
{
    int i;
    for (i = 0; i < N_C; i++) {
        saopara_dst[i] = saopara_src[i];
    }
}

void dec_parse_sao_param(com_core_t* core, int lcu_idx, com_sao_param_t *sao_cur_param)
{
    com_patch_header_t *sh = &core->pathdr;

    if (!sh->slice_sao_enable[Y_C] && !sh->slice_sao_enable[U_C] && !sh->slice_sao_enable[V_C]) {
        sao_cur_param[0].enable = 0;
        sao_cur_param[1].enable = 0;
        sao_cur_param[2].enable = 0;
    } else {
        com_seqh_t *seqhdr = core->seqhdr;
        com_lbac_t *lbac = &core->lbac;
        int cur_patch = core->map.map_patch[lcu_idx];
        int MergeUpAvail   = (core->lcu_y == 0) ? 0 : cur_patch == core->map.map_patch[lcu_idx - seqhdr->pic_width_in_lcu];
        int MergeLeftAvail = (core->lcu_x == 0) ? 0 : cur_patch == core->map.map_patch[lcu_idx -                      1];
        int mergemode = 0;

        if (MergeLeftAvail + MergeUpAvail > 0) {
            mergemode = dec_parse_sao_mergeflag(lbac, MergeLeftAvail, MergeUpAvail);
        }
        if (mergemode) {
            if (mergemode == 2) {
                copy_sao_param(sao_cur_param, core->sao_param_map[lcu_idx - 1]);
            } else {
                copy_sao_param(sao_cur_param, core->sao_param_map[lcu_idx - seqhdr->pic_width_in_lcu]);
            }
        } else {
            int compIdx;

            for (compIdx = Y_C; compIdx < N_C; compIdx++) {
                if (!sh->slice_sao_enable[compIdx]) {
                    sao_cur_param[compIdx].enable = 0;
                } else {
                    switch (dec_parse_sao_mode(lbac)) {
                    case 0:
                        sao_cur_param[compIdx].enable = 0;
                        break;
                    case 1:
                        sao_cur_param[compIdx].enable = 1;
                        sao_cur_param[compIdx].type = SAO_TYPE_BO;
                        break;
                    case 2:
                        sao_cur_param[compIdx].enable = 1;
                        sao_cur_param[compIdx].type = SAO_TYPE_EO_0;
                        break;
                    default:
                        uavs3d_assert(0);
                        break;
                    }

                    if (sao_cur_param[compIdx].enable) {
                        dec_parse_sao_offset(lbac, &(sao_cur_param[compIdx]), sao_cur_param[compIdx].offset);

                        if (sao_cur_param[compIdx].type == SAO_TYPE_BO) {
                            dec_parse_sao_bo_start(lbac, &(sao_cur_param[compIdx]), sao_cur_param[compIdx].bandIdx);
                        } else {
                            sao_cur_param[compIdx].type = dec_parse_sao_eo_type(lbac, &(sao_cur_param[compIdx]));
                        }
                    }
                }
            }
        }
    }
}

u32 dec_parse_alf_enable(com_lbac_t * lbac, int compIdx)
{
    return lbac_dec_bin(lbac, lbac->ctx.alf_lcu_enable);
}

int dec_parse_lcu_delta_qp(com_lbac_t * lbac, int last_dqp)
{
    com_lbac_all_ctx_t * ctx = &lbac->ctx;
    int act_ctx;
    int act_sym;
    int dquant;

    act_ctx = ((last_dqp != 0) ? 1 : 0);
    act_sym = !lbac_dec_bin(lbac, ctx->lcu_qp_delta + act_ctx);

    if (act_sym != 0)
    {
        u32 bin;
        act_ctx = 2;

        do
        {
            bin = lbac_dec_bin(lbac, ctx->lcu_qp_delta + act_ctx);
            act_ctx = min(3, act_ctx + 1);
            act_sym += !bin;
        } while (!bin && lbac->cur < lbac->end);
    }

    dquant = (act_sym + 1) >> 1;

    // lsb is signed bit
    if ((act_sym & 0x01) == 0)
    {
        dquant = -dquant;
    }

    return dquant;
}
