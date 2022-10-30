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

#include "avx2.h"

#if (BIT_DEPTH == 8)
void uavs3d_funs_init_avx2()
{
    int i;
    uavs3d_funs_handle.ipcpy[3] = uavs3d_if_cpy_w32_avx2;
    uavs3d_funs_handle.ipcpy[4] = uavs3d_if_cpy_w64_avx2;
    uavs3d_funs_handle.ipcpy[5] = uavs3d_if_cpy_w128_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_H_4][1] = uavs3d_if_hor_chroma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][2] = uavs3d_if_hor_chroma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][3] = uavs3d_if_hor_chroma_w32_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][4] = uavs3d_if_hor_chroma_w32x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][5] = uavs3d_if_hor_chroma_w32x_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_H_8][0] = uavs3d_if_hor_luma_w4_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][1] = uavs3d_if_hor_luma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][2] = uavs3d_if_hor_luma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][3] = uavs3d_if_hor_luma_w32_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][4] = uavs3d_if_hor_luma_w32x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][5] = uavs3d_if_hor_luma_w32x_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_V_4][1] = uavs3d_if_ver_chroma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][2] = uavs3d_if_ver_chroma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][3] = uavs3d_if_ver_chroma_w32_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][4] = uavs3d_if_ver_chroma_w64_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][5] = uavs3d_if_ver_chroma_w128_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_V_8][0] = uavs3d_if_ver_luma_w4_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][1] = uavs3d_if_ver_luma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][2] = uavs3d_if_ver_luma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][3] = uavs3d_if_ver_luma_w32_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][4] = uavs3d_if_ver_luma_w64_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][5] = uavs3d_if_ver_luma_w128_avx2;

    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][1] = uavs3d_if_hor_ver_chroma_w8_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][2] = uavs3d_if_hor_ver_chroma_w16_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][3] = uavs3d_if_hor_ver_chroma_w32x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][4] = uavs3d_if_hor_ver_chroma_w32x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][5] = uavs3d_if_hor_ver_chroma_w32x_avx2;

    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][0] = uavs3d_if_hor_ver_luma_w4_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][1] = uavs3d_if_hor_ver_luma_w8_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][2] = uavs3d_if_hor_ver_luma_w16_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][3] = uavs3d_if_hor_ver_luma_w32_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][4] = uavs3d_if_hor_ver_luma_w32x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][5] = uavs3d_if_hor_ver_luma_w32x_avx2;

    uavs3d_funs_handle.avg_pel[1] = uavs3d_avg_pel_w8_avx2;
    uavs3d_funs_handle.avg_pel[2] = uavs3d_avg_pel_w16_avx2;
    uavs3d_funs_handle.avg_pel[3] = uavs3d_avg_pel_w32_avx2;
    uavs3d_funs_handle.avg_pel[4] = uavs3d_avg_pel_w64_avx2;
    uavs3d_funs_handle.avg_pel[5] = uavs3d_avg_pel_w128_avx2;
    uavs3d_funs_handle.conv_fmt_8bit = uavs3d_conv_fmt_8bit_avx2;
    uavs3d_funs_handle.conv_fmt_16bit = uavs3d_conv_fmt_16bit_avx2;
    uavs3d_funs_handle.conv_fmt_16to8bit = uavs3d_conv_fmt_16to8bit_avx2;

    uavs3d_funs_handle.sao[ Y_C] = uavs3d_sao_on_lcu_avx2;
    uavs3d_funs_handle.sao[UV_C] = uavs3d_sao_on_lcu_chroma_avx2;
    uavs3d_funs_handle.alf[ Y_C] = uavs3d_alf_one_lcu_avx2;
    uavs3d_funs_handle.alf[UV_C] = uavs3d_alf_one_lcu_chroma_avx2;
    uavs3d_funs_handle.alf[2] = uavs3d_alf_one_lcu_one_chroma_avx2;

    uavs3d_funs_handle.intra_pred_hor[Y_C] = uavs3d_ipred_hor_avx2;
    uavs3d_funs_handle.intra_pred_ver[Y_C] = uavs3d_ipred_ver_avx2;
    uavs3d_funs_handle.intra_pred_dc [Y_C] = uavs3d_ipred_dc_avx2;

    uavs3d_ipred_offsets_seteps_init();
    for (i = IPD_BI + 1; i < IPD_VER; i++) {
        uavs3d_funs_handle.intra_pred_ang[i] = uavs3d_ipred_ang_x_avx2;
    }
    for (i = IPD_HOR + 1; i < IPD_CNT - 2; i++) {
        uavs3d_funs_handle.intra_pred_ang[i] = uavs3d_ipred_ang_y_avx2;
    }
    uavs3d_funs_handle.intra_pred_ang[4] = uavs3d_ipred_ang_x_4_avx2;
    uavs3d_funs_handle.intra_pred_ang[6] = uavs3d_ipred_ang_x_6_avx2;
    uavs3d_funs_handle.intra_pred_ang[8] = uavs3d_ipred_ang_x_8_avx2;
    uavs3d_funs_handle.intra_pred_ang[10] = uavs3d_ipred_ang_x_10_avx2;
    uavs3d_funs_handle.intra_pred_ang[26] = uavs3d_ipred_ang_y_26_avx2;
    uavs3d_funs_handle.intra_pred_ang[28] = uavs3d_ipred_ang_y_28_avx2;
    uavs3d_funs_handle.intra_pred_ang[30] = uavs3d_ipred_ang_y_30_avx2;
    uavs3d_funs_handle.intra_pred_ang[32] = uavs3d_ipred_ang_y_32_avx2;
    uavs3d_funs_handle.intra_pred_ang[13] = uavs3d_ipred_ang_xy_13_avx2;
    uavs3d_funs_handle.intra_pred_ang[14] = uavs3d_ipred_ang_xy_14_avx2;
    uavs3d_funs_handle.intra_pred_ang[15] = uavs3d_ipred_ang_xy_15_avx2;
    uavs3d_funs_handle.intra_pred_ang[16] = uavs3d_ipred_ang_xy_16_avx2;
    uavs3d_funs_handle.intra_pred_ang[17] = uavs3d_ipred_ang_xy_17_avx2;
    uavs3d_funs_handle.intra_pred_ang[18] = uavs3d_ipred_ang_xy_18_avx2;
    uavs3d_funs_handle.intra_pred_ang[19] = uavs3d_ipred_ang_xy_19_avx2;
    uavs3d_funs_handle.intra_pred_ang[20] = uavs3d_ipred_ang_xy_20_avx2;
    uavs3d_funs_handle.intra_pred_ang[21] = uavs3d_ipred_ang_xy_21_avx2;
    uavs3d_funs_handle.intra_pred_ang[22] = uavs3d_ipred_ang_xy_22_avx2;
    uavs3d_funs_handle.intra_pred_ang[23] = uavs3d_ipred_ang_xy_23_avx2;

    uavs3d_funs_handle.recon_luma[2] = uavs3d_recon_luma_w16_avx2;
    uavs3d_funs_handle.recon_luma[3] = uavs3d_recon_luma_w32_avx2;
    uavs3d_funs_handle.recon_luma[4] = uavs3d_recon_luma_w64_avx2;

    uavs3d_funs_handle.recon_chroma[2] = uavs3d_recon_chroma_w16_avx2;
    uavs3d_funs_handle.recon_chroma[3] = uavs3d_recon_chroma_w16x_avx2;
    uavs3d_funs_handle.recon_chroma[4] = uavs3d_recon_chroma_w16x_avx2;
    uavs3d_funs_handle.recon_chroma[5] = uavs3d_recon_chroma_w16x_avx2;

    uavs3d_funs_handle.itrans_dct2[1][2] = itrans_dct2_h4_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[1][3] = itrans_dct2_h4_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[1][4] = itrans_dct2_h4_w32_avx2;

    uavs3d_funs_handle.itrans_dct2[2][1] = itrans_dct2_h8_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[2][2] = itrans_dct2_h8_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[2][3] = itrans_dct2_h8_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[2][4] = itrans_dct2_h8_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[2][5] = itrans_dct2_h8_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[3][1] = itrans_dct2_h16_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[3][2] = itrans_dct2_h16_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[3][3] = itrans_dct2_h16_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[3][4] = itrans_dct2_h16_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[3][5] = itrans_dct2_h16_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[4][1] = itrans_dct2_h32_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[4][2] = itrans_dct2_h32_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[4][3] = itrans_dct2_h32_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[4][4] = itrans_dct2_h32_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[4][5] = itrans_dct2_h32_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[5][2] = itrans_dct2_h64_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[5][3] = itrans_dct2_h64_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[5][4] = itrans_dct2_h64_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[5][5] = itrans_dct2_h64_w64_avx2;

    uavs3d_funs_handle.itrans_dct8[0] = itrans_dct8_pb4_avx2;
    uavs3d_funs_handle.itrans_dct8[1] = itrans_dct8_pb8_avx2;
    uavs3d_funs_handle.itrans_dct8[2] = itrans_dct8_pb16_avx2;

    uavs3d_funs_handle.itrans_dst7[0] = itrans_dst7_pb4_avx2;
    uavs3d_funs_handle.itrans_dst7[1] = itrans_dst7_pb8_avx2;
    uavs3d_funs_handle.itrans_dst7[2] = itrans_dst7_pb16_avx2;
}

#else
void uavs3d_funs_init_avx2()
{
    uavs3d_funs_handle.ipcpy[2] = uavs3d_if_cpy_w16_avx2;
    uavs3d_funs_handle.ipcpy[3] = uavs3d_if_cpy_w32_avx2;
    uavs3d_funs_handle.ipcpy[4] = uavs3d_if_cpy_w64_avx2;
    uavs3d_funs_handle.ipcpy[5] = uavs3d_if_cpy_w128_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_H_4][1] = uavs3d_if_hor_chroma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][2] = uavs3d_if_hor_chroma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][3] = uavs3d_if_hor_chroma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][4] = uavs3d_if_hor_chroma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_4][5] = uavs3d_if_hor_chroma_w16x_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_H_8][1] = uavs3d_if_hor_luma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][2] = uavs3d_if_hor_luma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][3] = uavs3d_if_hor_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][4] = uavs3d_if_hor_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_H_8][5] = uavs3d_if_hor_luma_w16x_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_V_4][2] = uavs3d_if_ver_chroma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][3] = uavs3d_if_ver_chroma_w32_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][4] = uavs3d_if_ver_chroma_w32x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_4][5] = uavs3d_if_ver_chroma_w32x_avx2;

    uavs3d_funs_handle.ipflt[IPFILTER_V_8][1] = uavs3d_if_ver_luma_w8_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][2] = uavs3d_if_ver_luma_w16_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][3] = uavs3d_if_ver_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][4] = uavs3d_if_ver_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt[IPFILTER_V_8][5] = uavs3d_if_ver_luma_w16x_avx2;

    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][1] = uavs3d_if_hor_ver_chroma_w8_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][2] = uavs3d_if_hor_ver_chroma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][3] = uavs3d_if_hor_ver_chroma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][4] = uavs3d_if_hor_ver_chroma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][5] = uavs3d_if_hor_ver_chroma_w16x_avx2;

    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][0] = uavs3d_if_hor_ver_luma_w4_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][1] = uavs3d_if_hor_ver_luma_w8_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][2] = uavs3d_if_hor_ver_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][3] = uavs3d_if_hor_ver_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][4] = uavs3d_if_hor_ver_luma_w16x_avx2;
    uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][5] = uavs3d_if_hor_ver_luma_w16x_avx2;

    uavs3d_funs_handle.avg_pel[0] = uavs3d_avg_pel_w4_avx2;
    uavs3d_funs_handle.avg_pel[1] = uavs3d_avg_pel_w8_avx2;
    uavs3d_funs_handle.avg_pel[2] = uavs3d_avg_pel_w16_avx2;
    uavs3d_funs_handle.avg_pel[3] = uavs3d_avg_pel_w32_avx2;
    uavs3d_funs_handle.avg_pel[4] = uavs3d_avg_pel_w64_avx2;
    uavs3d_funs_handle.avg_pel[5] = uavs3d_avg_pel_w128_avx2;
    uavs3d_funs_handle.padding_rows_luma = uavs3d_padding_rows_luma_avx2;
    uavs3d_funs_handle.padding_rows_chroma = uavs3d_padding_rows_chroma_avx2;
    uavs3d_funs_handle.conv_fmt_8bit = uavs3d_conv_fmt_8bit_avx2;
    uavs3d_funs_handle.conv_fmt_16bit = uavs3d_conv_fmt_16bit_avx2;
    uavs3d_funs_handle.conv_fmt_16to8bit = uavs3d_conv_fmt_16to8bit_avx2;

    uavs3d_funs_handle.recon_luma[1] = uavs3d_recon_luma_w8_avx2;
    uavs3d_funs_handle.recon_luma[2] = uavs3d_recon_luma_w16_avx2;
    uavs3d_funs_handle.recon_luma[3] = uavs3d_recon_luma_w32_avx2;
    uavs3d_funs_handle.recon_luma[4] = uavs3d_recon_luma_w64_avx2;

    uavs3d_funs_handle.recon_chroma[2] = uavs3d_recon_chroma_w16_avx2;
    uavs3d_funs_handle.recon_chroma[3] = uavs3d_recon_chroma_w16x_avx2;
    uavs3d_funs_handle.recon_chroma[4] = uavs3d_recon_chroma_w16x_avx2;
    uavs3d_funs_handle.recon_chroma[5] = uavs3d_recon_chroma_w16x_avx2;

    uavs3d_funs_handle.itrans_dct2[1][2] = itrans_dct2_h4_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[1][3] = itrans_dct2_h4_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[1][4] = itrans_dct2_h4_w32_avx2;

    uavs3d_funs_handle.itrans_dct2[2][1] = itrans_dct2_h8_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[2][2] = itrans_dct2_h8_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[2][3] = itrans_dct2_h8_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[2][4] = itrans_dct2_h8_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[2][5] = itrans_dct2_h8_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[3][1] = itrans_dct2_h16_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[3][2] = itrans_dct2_h16_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[3][3] = itrans_dct2_h16_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[3][4] = itrans_dct2_h16_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[3][5] = itrans_dct2_h16_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[4][1] = itrans_dct2_h32_w4_avx2;
    uavs3d_funs_handle.itrans_dct2[4][2] = itrans_dct2_h32_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[4][3] = itrans_dct2_h32_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[4][4] = itrans_dct2_h32_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[4][5] = itrans_dct2_h32_w64_avx2;

    uavs3d_funs_handle.itrans_dct2[5][2] = itrans_dct2_h64_w8_avx2;
    uavs3d_funs_handle.itrans_dct2[5][3] = itrans_dct2_h64_w16_avx2;
    uavs3d_funs_handle.itrans_dct2[5][4] = itrans_dct2_h64_w32_avx2;
    uavs3d_funs_handle.itrans_dct2[5][5] = itrans_dct2_h64_w64_avx2;

    uavs3d_funs_handle.sao[ Y_C] = uavs3d_sao_on_lcu_avx2;
    uavs3d_funs_handle.sao[UV_C] = uavs3d_sao_on_lcu_chroma_avx2;
    uavs3d_funs_handle.alf[ Y_C] = uavs3d_alf_one_lcu_avx2;
    uavs3d_funs_handle.alf[UV_C] = uavs3d_alf_one_lcu_chroma_avx2;
    uavs3d_funs_handle.alf[2] = uavs3d_alf_one_lcu_one_chroma_avx2;

    uavs3d_funs_handle.intra_pred_hor[Y_C] = uavs3d_ipred_hor_avx2;
    uavs3d_funs_handle.intra_pred_ver[Y_C] = uavs3d_ipred_ver_avx2;
    uavs3d_funs_handle.intra_pred_dc[Y_C] = uavs3d_ipred_dc_avx2;
}

#endif