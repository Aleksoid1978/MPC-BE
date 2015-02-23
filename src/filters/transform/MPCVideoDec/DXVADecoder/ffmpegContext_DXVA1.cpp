/*
 * (C) 2006-2015 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define HAVE_AV_CONFIG_H

#include <Windows.h>
#include <time.h>
#include "ffmpegContext_DXVA1.h"

extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
#define class classFFMPEG
	#include <ffmpeg/libavcodec/mpegvideo.h>
#undef class
#define new newFFMPEG
	#include <ffmpeg/libavcodec/h264.h>
	#include <ffmpeg/libavcodec/h264data.h>
#undef new
	#include <ffmpeg/libavcodec/vc1.h>
}

HRESULT FFH264DecodeFrame(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, BYTE* pBuffer, UINT nSize, REFERENCE_TIME rtStart,
						  int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart,
						  UINT* SecondFieldOffset, int* Sync, int* NALLength)
{
	HRESULT hr = E_FAIL;
	if (pBuffer != NULL) {
		H264Context* h	= (H264Context*)pAVCtx->priv_data;
		int				got_picture	= 0;
		AVPacket		avpkt;
		av_init_packet(&avpkt);
		avpkt.data		= pBuffer;
		avpkt.size		= nSize;
		avpkt.pts		= rtStart;
		avpkt.flags		= AV_PKT_FLAG_KEY;
		int used_bytes	= avcodec_decode_video2(pAVCtx, pFrame, &got_picture, &avpkt);
		
#if defined(_DEBUG) && 0
		av_log(pAVCtx, AV_LOG_INFO, "FFH264DecodeFrame() : used_bytes - %d, got_picture - %d\n", used_bytes, got_picture);
#endif

		if (used_bytes < 0) {
			return hr;
		}

		hr = S_OK;
		if (h->cur_pic_ptr) {
			if (pOutPOC) {
				*pOutPOC = h->next_output_pic ? h->next_output_pic->poc : INT_MIN;
			}
			if (pOutrtStart) {
				*pOutrtStart = pFrame->pkt_pts;
			}
			if (pFramePOC) {
				*pFramePOC = h->cur_pic_ptr->poc;
				if (*pFramePOC == INT_MIN) {
					return E_FAIL;
				}
			}
			if (SecondFieldOffset) {
				*SecondFieldOffset = 0;
				if (h->second_field_offset && (h->second_field_offset < (int)nSize)) {
					*SecondFieldOffset = h->second_field_offset;
				}
			}
			if (Sync) {
				*Sync = h->frame_recovered;
			}
			if (NALLength) {
				*NALLength = h->nal_length_size;
			}
		}
	}
	return hr;
}

static void CopyScalingMatrix(DXVA_Qmatrix_H264* pDest, PPS* pps, DWORD nPCIVendor, DWORD nPCIDevice, bool IsATIUVD)
{
	memset(pDest, 0, sizeof(DXVA_Qmatrix_H264));

	if (IsATIUVD) {
		for (int i = 0; i < 6; i++)
			for (int j = 0; j < 16; j++)
				pDest->bScalingLists4x4[i][j] = pps->scaling_matrix4[i][j];

		for (int i = 0; i < 64; i++) {
			pDest->bScalingLists8x8[0][i] = pps->scaling_matrix8[0][i];
			pDest->bScalingLists8x8[1][i] = pps->scaling_matrix8[3][i];
		}
	} else {
		for (int i = 0; i < 6; i++)
			for (int j = 0; j < 16; j++)
				pDest->bScalingLists4x4[i][j] = pps->scaling_matrix4[i][zigzag_scan[j]];

		for (int i = 0; i < 64; i++) {
			pDest->bScalingLists8x8[0][i] = pps->scaling_matrix8[0][ff_zigzag_direct[i]];
			pDest->bScalingLists8x8[1][i] = pps->scaling_matrix8[3][ff_zigzag_direct[i]];
		}
	}
}

USHORT FFH264FindRefFrameIndex(USHORT num_frame, DXVA_PicParams_H264* pDXVAPicParams)
{
	for (unsigned i = 0; i < pDXVAPicParams->num_ref_frames; i++) {
		if (pDXVAPicParams->FrameNumList[i] == num_frame) {
			return pDXVAPicParams->RefFrameList[i].Index7Bits;
		}
	}

	return 127;
}

HRESULT FFH264BuildPicParams(struct AVCodecContext* pAVCtx, DWORD nPCIVendor, DWORD nPCIDevice, DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix, int* nPictStruct, bool IsATIUVD)
{
	H264Context*	h = (H264Context*)pAVCtx->priv_data;
	SPS*			cur_sps;
	PPS*			cur_pps;
	int				field_pic_flag;
	HRESULT			hr = E_FAIL;
	const			H264Picture *current_picture = h->cur_pic_ptr;

	field_pic_flag = (h->picture_structure != PICT_FRAME);

	cur_sps	= &h->sps;
	cur_pps = &h->pps;

	if (cur_sps && cur_pps) {
		*nPictStruct = h->picture_structure;

		if (!cur_sps->mb_width || !cur_sps->mb_height) {
			return E_FAIL;
		}

		pDXVAPicParams->wFrameWidthInMbsMinus1					= cur_sps->mb_width  - 1;
		pDXVAPicParams->wFrameHeightInMbsMinus1					= cur_sps->mb_height * (2 - cur_sps->frame_mbs_only_flag) - 1;
		pDXVAPicParams->num_ref_frames							= cur_sps->ref_frame_count;
		pDXVAPicParams->field_pic_flag							= field_pic_flag;
		pDXVAPicParams->MbaffFrameFlag							= (h->sps.mb_aff && (field_pic_flag == 0));
		pDXVAPicParams->residual_colour_transform_flag			= cur_sps->residual_color_transform_flag;
		pDXVAPicParams->chroma_format_idc						= cur_sps->chroma_format_idc;
		pDXVAPicParams->RefPicFlag								= h->ref_pic_flag;
		pDXVAPicParams->constrained_intra_pred_flag				= cur_pps->constrained_intra_pred;
		pDXVAPicParams->weighted_pred_flag						= cur_pps->weighted_pred;
		pDXVAPicParams->weighted_bipred_idc						= cur_pps->weighted_bipred_idc;
		pDXVAPicParams->frame_mbs_only_flag						= cur_sps->frame_mbs_only_flag;
		pDXVAPicParams->transform_8x8_mode_flag					= cur_pps->transform_8x8_mode;
		pDXVAPicParams->MinLumaBipredSize8x8Flag				= h->sps.level_idc >= 31;
		pDXVAPicParams->IntraPicFlag							= (h->slice_type == AV_PICTURE_TYPE_I || h->slice_type == AV_PICTURE_TYPE_SI);

		pDXVAPicParams->bit_depth_luma_minus8					= cur_sps->bit_depth_luma   - 8;
		pDXVAPicParams->bit_depth_chroma_minus8					= cur_sps->bit_depth_chroma - 8;

		//	pDXVAPicParams->StatusReportFeedbackNumber				= SET IN DecodeFrame;
		//	pDXVAPicParams->CurrFieldOrderCnt						= SET IN UpdateRefFramesList;
		//	pDXVAPicParams->FieldOrderCntList						= SET IN UpdateRefFramesList;
		//	pDXVAPicParams->FrameNumList							= SET IN UpdateRefFramesList;
		//	pDXVAPicParams->UsedForReferenceFlags					= SET IN UpdateRefFramesList;
		//	pDXVAPicParams->NonExistingFrameFlags

		pDXVAPicParams->frame_num								= h->frame_num;

		pDXVAPicParams->log2_max_frame_num_minus4				= cur_sps->log2_max_frame_num - 4;
		pDXVAPicParams->pic_order_cnt_type						= cur_sps->poc_type;

		pDXVAPicParams->log2_max_pic_order_cnt_lsb_minus4		= 0;
		pDXVAPicParams->delta_pic_order_always_zero_flag		= 0;
		if (cur_sps->poc_type == 0)
			pDXVAPicParams->log2_max_pic_order_cnt_lsb_minus4	= cur_sps->log2_max_poc_lsb - 4;
		else if (cur_sps->poc_type == 1)
			pDXVAPicParams->delta_pic_order_always_zero_flag	= cur_sps->delta_pic_order_always_zero_flag;
		pDXVAPicParams->direct_8x8_inference_flag				= cur_sps->direct_8x8_inference_flag;
		pDXVAPicParams->entropy_coding_mode_flag				= cur_pps->cabac;
		pDXVAPicParams->pic_order_present_flag					= cur_pps->pic_order_present;
		pDXVAPicParams->num_slice_groups_minus1					= cur_pps->slice_group_count - 1;
		pDXVAPicParams->slice_group_map_type					= cur_pps->mb_slice_group_map_type;
		pDXVAPicParams->deblocking_filter_control_present_flag	= cur_pps->deblocking_filter_parameters_present;
		pDXVAPicParams->redundant_pic_cnt_present_flag			= cur_pps->redundant_pic_cnt_present;

		pDXVAPicParams->chroma_qp_index_offset					= cur_pps->chroma_qp_index_offset[0];
		pDXVAPicParams->second_chroma_qp_index_offset			= cur_pps->chroma_qp_index_offset[1];
		pDXVAPicParams->num_ref_idx_l0_active_minus1			= cur_pps->ref_count[0] - 1;
		pDXVAPicParams->num_ref_idx_l1_active_minus1			= cur_pps->ref_count[1] - 1;
		pDXVAPicParams->pic_init_qp_minus26						= cur_pps->init_qp - 26;
		pDXVAPicParams->pic_init_qs_minus26						= cur_pps->init_qs - 26;

		pDXVAPicParams->CurrPic.AssociatedFlag					= field_pic_flag && (h->picture_structure == PICT_BOTTOM_FIELD);
		pDXVAPicParams->CurrFieldOrderCnt[0]					= 0;
		if ((h->picture_structure & PICT_TOP_FIELD) && current_picture->field_poc[0] != INT_MAX) {
			pDXVAPicParams->CurrFieldOrderCnt[0]				= current_picture->field_poc[0];
		}
		pDXVAPicParams->CurrFieldOrderCnt[1]					= 0;
		if ((h->picture_structure & PICT_BOTTOM_FIELD) && current_picture->field_poc[1] != INT_MAX) {
			pDXVAPicParams->CurrFieldOrderCnt[1]				= current_picture->field_poc[1];
		}

		CopyScalingMatrix(pDXVAScalingMatrix, cur_pps, nPCIVendor, nPCIDevice, IsATIUVD);

		hr = S_OK;
	}

	return hr;
}

void FFH264SetCurrentPicture(int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx)
{
	H264Context* h = (H264Context*)pAVCtx->priv_data;

	pDXVAPicParams->CurrPic.Index7Bits = nIndex;
	if (h->cur_pic_ptr) {
		h->cur_pic_ptr->f.opaque = (void*)nIndex;
	}
}

void FFH264UpdateRefFramesList(DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx)
{
	H264Context*	h = (H264Context*)pAVCtx->priv_data;
	UINT			nUsedForReferenceFlags = 0;
	int				i, j;
	H264Picture*	pic;
	UCHAR			AssociatedFlag;

	for (i = 0, j = 0; i < 16; i++) {
		if (i < h->short_ref_count) {
			// Short list reference frames
			pic				= h->short_ref[h->short_ref_count - i - 1];
			AssociatedFlag	= pic->long_ref != 0;
		} else {
			// Long list reference frames
			pic = NULL;
			while (!pic && j < h->short_ref_count + 16) {
				pic = h->long_ref[j++ - h->short_ref_count];
			}
			AssociatedFlag	= 1;
		}

		if (pic != NULL) {
			pDXVAPicParams->FrameNumList[i]					= pic->long_ref ? pic->pic_id : pic->frame_num;
			pDXVAPicParams->FieldOrderCntList[i][0]			= 0;
			pDXVAPicParams->FieldOrderCntList[i][1]			= 0;

			if (pic->field_poc[0] != INT_MAX) {
				pDXVAPicParams->FieldOrderCntList[i][0]		= pic->field_poc [0];
				nUsedForReferenceFlags						|= 1<<(i*2);
			}

			if (pic->field_poc[1] != INT_MAX) {
				pDXVAPicParams->FieldOrderCntList[i][1]		= pic->field_poc [1];
				nUsedForReferenceFlags						|= 2<<(i*2);
			}

			pDXVAPicParams->RefFrameList[i].AssociatedFlag	= AssociatedFlag;
			pDXVAPicParams->RefFrameList[i].Index7Bits		= (UCHAR)pic->f.opaque;
		} else {
			pDXVAPicParams->RefFrameList[i].bPicEntry		= 255;
			pDXVAPicParams->FrameNumList[i]					= 0;
			pDXVAPicParams->FieldOrderCntList[i][0]			= 0;
			pDXVAPicParams->FieldOrderCntList[i][1]			= 0;
		}
	}

	pDXVAPicParams->UsedForReferenceFlags = nUsedForReferenceFlags;
}

BOOL FFH264IsRefFrameInUse(int nFrameNum, struct AVCodecContext* pAVCtx)
{
	H264Context* h = (H264Context*)pAVCtx->priv_data;

	for (int i = 0; i < h->short_ref_count; i++) {
		if ((int)h->short_ref[i]->f.opaque == nFrameNum) {
			return TRUE;
		}
	}

	for (int i = 0; i < h->long_ref_count; i++) {
		if ((int)h->long_ref[i]->f.opaque == nFrameNum) {
			return TRUE;
		}
	}

	return FALSE;
}

void FF264UpdateRefFrameSliceLong(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx)
{
	H264Context* h = (H264Context*)pAVCtx->priv_data;

	for (unsigned i = 0; i < _countof(pSlice->RefPicList[0]); i++) {
		pSlice->RefPicList[0][i].bPicEntry = 255;
		pSlice->RefPicList[1][i].bPicEntry = 255;
	}

	for (unsigned list = 0; list < 2; list++) {
		for (unsigned i = 0; i < _countof(pSlice->RefPicList[list]); i++) {
			if (list < h->list_count && i < h->ref_count[list]) {
				const H264Picture *r = &h->ref_list[list][i];
				pSlice->RefPicList[list][i].Index7Bits		= FFH264FindRefFrameIndex(h->ref_list[list][i].frame_num, pDXVAPicParams);
				pSlice->RefPicList[list][i].AssociatedFlag	= (r->reference == PICT_BOTTOM_FIELD);
			}
		}
	}
}

void FFH264SetDxvaSliceLong(struct AVCodecContext* pAVCtx, void* pSliceLong)
{
	((H264Context*)pAVCtx->priv_data)->dxva_slice_long = pSliceLong;
}

HRESULT FFVC1DecodeFrame(DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, REFERENCE_TIME rtStart,
						 BYTE* pBuffer, UINT nSize,
						 UINT* nFrameSize, BOOL b_SecondField)
{
	HRESULT	hr			= E_FAIL;
	VC1Context* vc1		= (VC1Context*)pAVCtx->priv_data;

	if (pBuffer && !b_SecondField) {
		int				got_picture	= 0;
		AVPacket		avpkt;
		av_init_packet(&avpkt);
		avpkt.data		= pBuffer;
		avpkt.size		= nSize;
		avpkt.pts		= rtStart;
		avpkt.flags		= AV_PKT_FLAG_KEY;
		int used_bytes	= avcodec_decode_video2(pAVCtx, pFrame, &got_picture, &avpkt);
		
#if defined(_DEBUG) && 0
		av_log(pAVCtx, AV_LOG_INFO, "FFVC1DecodeFrame() : used_bytes - %d, got_picture - %d\n", used_bytes, got_picture);
#endif

		if (used_bytes < 0) {
			return hr;
		}
	}

	hr = S_OK;

	if (b_SecondField) {
		vc1->second_field = 1;
		vc1->s.picture_structure = PICT_TOP_FIELD + vc1->tff;
		vc1->s.pict_type = (vc1->fptype & 1) ? AV_PICTURE_TYPE_P : AV_PICTURE_TYPE_I;
		if (vc1->fptype & 4)
			vc1->s.pict_type = (vc1->fptype & 1) ? AV_PICTURE_TYPE_BI : AV_PICTURE_TYPE_B;
	}

	if (nFrameSize) {
		*nFrameSize = vc1->second_field_offset;
	}

	if (vc1->profile == PROFILE_ADVANCED) {
		/* It is the cropped width/height -1 of the frame */
		pPicParams->wPicWidthInMBminus1		= pAVCtx->width  - 1;
		pPicParams->wPicHeightInMBminus1	= pAVCtx->height - 1;
	} else {
		/* It is the coded width/height in macroblock -1 of the frame */
		pPicParams->wPicWidthInMBminus1		= vc1->s.mb_width  - 1;
		pPicParams->wPicHeightInMBminus1	= vc1->s.mb_height - 1;
	}

	pPicParams->bSecondField			= (vc1->interlace && vc1->fcm == ILACE_FIELD && vc1->second_field);
	pPicParams->bPicIntra               = vc1->s.pict_type == AV_PICTURE_TYPE_I;
	pPicParams->bPicBackwardPrediction  = vc1->s.pict_type == AV_PICTURE_TYPE_B;

	// Init    Init    Init    Todo
	// iWMV9 - i9IRU - iOHIT - iINSO - iWMVA - 0 - 0 - 0		| Section 3.2.5
	pPicParams->bBidirectionalAveragingMode	= (pPicParams->bBidirectionalAveragingMode & 0xE0) |	// init in Init()
										   ((vc1->lumshift!=0 || vc1->lumscale!=32) ? 0x10 : 0)|	// iINSO
										   ((vc1->profile == PROFILE_ADVANCED)	 <<3 );				// iWMVA

	// Section 3.2.20.3
	pPicParams->bPicSpatialResid8	= (vc1->panscanflag   << 7) | (vc1->refdist_flag << 6) |
									  (vc1->s.loop_filter << 5) | (vc1->fastuvmc     << 4) |
									  (vc1->extended_mv   << 3) | (vc1->dquant       << 1) |
									  (vc1->vstransform);

	// Section 3.2.20.4
	pPicParams->bPicOverflowBlocks  = (vc1->quantizer_mode  << 6) | (vc1->multires << 5) |
									  (vc1->resync_marker << 4) | (vc1->rangered << 3) |
									  (vc1->s.max_b_frames);

	// Section 3.2.20.2
	pPicParams->bPicDeblockConfined	= (vc1->postprocflag << 7) | (vc1->broadcast  << 6) |
									  (vc1->interlace    << 5) | (vc1->tfcntrflag << 4) |
									  (vc1->finterpflag  << 3) | // (refpic << 2) set in DecodeFrame !
									  (vc1->psf << 1)		   | vc1->extended_dmv;

	pPicParams->bPicStructure = 0;
	if (vc1->s.picture_structure & PICT_TOP_FIELD) {
		pPicParams->bPicStructure |= 0x01;
	}
	if (vc1->s.picture_structure & PICT_BOTTOM_FIELD) {
		pPicParams->bPicStructure |= 0x02;
	}

	pPicParams->bMVprecisionAndChromaRelation =	((vc1->mv_mode == MV_PMODE_1MV_HPEL_BILIN) << 3) |
												  (1                                       << 2) |
												  (0                                       << 1) |
												  (!vc1->s.quarter_sample					   );

	// Cf page 17 : 2 for interlaced, 0 for progressive
	pPicParams->bPicExtrapolation = (!vc1->interlace || vc1->fcm == PROGRESSIVE) ? 1 : 2;

	if (vc1->s.picture_structure == PICT_FRAME) {
		pPicParams->wBitstreamFcodes        = vc1->lumscale;
		pPicParams->wBitstreamPCEelements   = vc1->lumshift;
	} else {
		/* Syntax: (top_field_param << 8) | bottom_field_param */
		pPicParams->wBitstreamFcodes        = (vc1->lumscale << 8) | vc1->lumscale;
		pPicParams->wBitstreamPCEelements   = (vc1->lumshift << 8) | vc1->lumshift;
	}

	if (vc1->profile == PROFILE_ADVANCED) {
		pPicParams->bPicOBMC = (vc1->range_mapy_flag  << 7) |
							   (vc1->range_mapy       << 4) |
							   (vc1->range_mapuv_flag << 3) |
							   (vc1->range_mapuv          );
	}

	// Cf section 7.1.1.25 in VC1 specification, section 3.2.14.3 in DXVA spec
	pPicParams->bRcontrol	= vc1->rnd;

	pPicParams->bPicDeblocked	= ((vc1->overlap == 1 && pPicParams->bPicBackwardPrediction == 0)	<< 6) |
								  ((vc1->profile != PROFILE_ADVANCED && vc1->rangeredfrm)			<< 5) |
								  (vc1->s.loop_filter												<< 1);

	return S_OK;
}
