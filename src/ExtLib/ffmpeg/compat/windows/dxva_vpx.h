//------------------------------------------------------------------------------
// Copyright (c) 1999 - 2002, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef __DIRECTX_VA_VPX__
#define __DIRECTX_VA_VPX__

#pragma pack(push, 1)

/* VPx picture entry data structure */
typedef struct _DXVA_PicEntry_VPx {
    union {
        struct {
            UCHAR Index7Bits : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
} DXVA_PicEntry_VPx, *LPDXVA_PicEntry_VPx;

/* VP9 segmentation structure */
typedef struct _segmentation_VP9 {
    union {
        struct {
            UCHAR enabled : 1;
            UCHAR update_map : 1;
            UCHAR temporal_update : 1;
            UCHAR abs_delta : 1;
            UCHAR ReservedSegmentFlags4Bits : 4;
        };
        UCHAR wSegmentInfoFlags;
    };
    UCHAR tree_probs[7];
    UCHAR pred_probs[3];
    SHORT feature_data[8][4];
    UCHAR feature_mask[8];
} DXVA_segmentation_VP9;

/* VP9 picture parameters structure */
typedef struct _DXVA_PicParams_VP9 {
    DXVA_PicEntry_VPx    CurrPic;
    UCHAR                profile;
    union {
        struct {
            USHORT frame_type : 1;
            USHORT show_frame : 1;
            USHORT error_resilient_mode : 1;
            USHORT subsampling_x : 1;
            USHORT subsampling_y : 1;
            USHORT extra_plane : 1;
            USHORT refresh_frame_context : 1;
            USHORT frame_parallel_decoding_mode : 1;
            USHORT intra_only : 1;
            USHORT frame_context_idx : 2;
            USHORT reset_frame_context : 2;
            USHORT allow_high_precision_mv : 1;
            USHORT ReservedFormatInfo2Bits : 2;
        };
        USHORT wFormatAndPictureInfoFlags;
    };
    UINT  width;
    UINT  height;
    UCHAR BitDepthMinus8Luma;
    UCHAR BitDepthMinus8Chroma;
    UCHAR interp_filter;
    UCHAR Reserved8Bits;
    DXVA_PicEntry_VPx  ref_frame_map[8];
    UINT  ref_frame_coded_width[8];
    UINT  ref_frame_coded_height[8];
    DXVA_PicEntry_VPx  frame_refs[3];
    CHAR  ref_frame_sign_bias[4];
    CHAR  filter_level;
    CHAR  sharpness_level;
    union {
        struct {
            UCHAR mode_ref_delta_enabled : 1;
            UCHAR mode_ref_delta_update : 1;
            UCHAR use_prev_in_find_mv_refs : 1;
            UCHAR ReservedControlInfo5Bits : 5;
        };
        UCHAR wControlInfoFlags;
    };
    CHAR   ref_deltas[4];
    CHAR   mode_deltas[2];
    SHORT  base_qindex;
    CHAR   y_dc_delta_q;
    CHAR   uv_dc_delta_q;
    CHAR   uv_ac_delta_q;
    DXVA_segmentation_VP9 stVP9Segments;
    UCHAR  log2_tile_cols;
    UCHAR  log2_tile_rows;
    USHORT uncompressed_header_size_byte_aligned;
    USHORT first_partition_size;
    USHORT Reserved16Bits;
    UINT   Reserved32Bits;
    UINT   StatusReportFeedbackNumber;
} DXVA_PicParams_VP9, *LPDXVA_PicParams_VP9;

/* VP8 segmentation structure */
typedef struct _segmentation_VP8 {
    union {
        struct {
            UCHAR segmentation_enabled : 1;
            UCHAR update_mb_segmentation_map : 1;
            UCHAR update_mb_segmentation_data : 1;
            UCHAR mb_segement_abs_delta : 1;
            UCHAR ReservedSegmentFlags4Bits : 4;
        };
        UCHAR wSegmentFlags;
    };
    CHAR  segment_feature_data[2][4];
    UCHAR mb_segment_tree_probs[3];
} DXVA_segmentation_VP8;

/* VP8 picture parameters structure */
typedef struct _DXVA_PicParams_VP8 {
    UINT first_part_size;
    UINT width;
    UINT height;
    DXVA_PicEntry_VPx  CurrPic;
    union {
        struct {
            UCHAR frame_type : 1;
            UCHAR version : 3;
            UCHAR show_frame : 1;
            UCHAR clamp_type : 1;
            UCHAR ReservedFrameTag3Bits : 2;
        };
        UCHAR wFrameTagFlags;
    };
    DXVA_segmentation_VP8  stVP8Segments;
    UCHAR filter_type;
    UCHAR filter_level;
    UCHAR sharpness_level;
    UCHAR mode_ref_lf_delta_enabled;
    UCHAR mode_ref_lf_delta_update;
    CHAR  ref_lf_deltas[4];
    CHAR  mode_lf_deltas[4];
    UCHAR log2_nbr_of_dct_partitions;
    UCHAR base_qindex;
    CHAR  y1dc_delta_q;
    CHAR  y2dc_delta_q;
    CHAR  y2ac_delta_q;
    CHAR  uvdc_delta_q;
    CHAR  uvac_delta_q;
    DXVA_PicEntry_VPx alt_fb_idx;
    DXVA_PicEntry_VPx gld_fb_idx;
    DXVA_PicEntry_VPx lst_fb_idx;
    UCHAR  ref_frame_sign_bias_golden;
    UCHAR  ref_frame_sign_bias_altref;
    UCHAR  refresh_entropy_probs;
    UCHAR  vp8_coef_update_probs[4][8][3][11];
    UCHAR  mb_no_coeff_skip;
    UCHAR  prob_skip_false;
    UCHAR  prob_intra;
    UCHAR  prob_last;
    UCHAR  prob_golden;
    UCHAR  intra_16x16_prob[4];
    UCHAR  intra_chroma_prob[3];
    UCHAR  vp8_mv_update_probs[2][19];
    USHORT ReservedBits1;
    USHORT ReservedBits2;
    USHORT ReservedBits3;
    UINT   StatusReportFeedbackNumber;
} DXVA_PicParams_VP8, *LPDXVA_PicParams_VP8;

/* VPx slice control data structure - short form */
typedef struct _DXVA_Slice_VPx_Short {
    UINT   BSNALunitDataLocation;
    UINT   SliceBytesInBuffer;
    USHORT wBadSliceChopping;
} DXVA_Slice_VPx_Short, *LPDXVA_Slice_VPx_Short;

/* VPx status reporting data structure */
typedef struct _DXVA_Status_VPx {
    UINT  StatusReportFeedbackNumber;
    DXVA_PicEntry_VPx CurrPic;
    UCHAR  bBufType;
    UCHAR  bStatus;
    UCHAR  bReserved8Bits;
    USHORT wNumMbsAffected;
} DXVA_Status_VPx, *LPDXVA_Status_VPx;

#pragma pack(pop)
#endif
