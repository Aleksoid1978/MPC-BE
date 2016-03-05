/*
 * (C) 2010-2015 see Authors.txt
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

#pragma once

enum {
	SOURCE_FILTER,
	DXVA_DECODER,
	VIDEO_DECODER,
	AUDIO_DECODER,
	FILTER_TYPE_NB
};

enum SOURCE_FILTER {
	SRC_AMR,
	SRC_AVI,
	SRC_APE,
	SRC_CDDA,
	SRC_CDXA,
	SRC_DSD,
	SRC_DSM,
	SRC_DTSAC3,
	SRC_VTS,
	SRC_FLIC,
	SRC_FLAC,
	SRC_FLV,
	SRC_MATROSKA,
	SRC_MP4,
	SRC_MPA,
	SRC_MPEG,
	SRC_MUSEPACK,
	SRC_OGG,
	SRC_RAWVIDEO,
	SRC_REAL,
	SRC_ROQ,
	SRC_SHOUTCAST,
	SRC_STDINPUT,
	SRC_TAK,
	SRC_TTA,
	SRC_WAV,
	SRC_WAVPACK,
	SRC_UDP,

	SRC_LAST
};

enum DXVA_FILTER {
	VDEC_DXVA_H264,
	VDEC_DXVA_HEVC,
	VDEC_DXVA_MPEG2,
	VDEC_DXVA_VC1,
	VDEC_DXVA_WMV3,
	VDEC_DXVA_VP9,

	VDEC_DXVA_LAST
};

enum VIDEO_FILTER {
	VDEC_AMV,
	VDEC_PRORES,
	VDEC_DNXHD,
	VDEC_BINK,
	VDEC_CANOPUS,
	VDEC_CINEFORM,
	VDEC_CINEPAK,
	VDEC_DIRAC,
	VDEC_DIVX,
	VDEC_DV,
	VDEC_FLV,
	VDEC_H263,
	VDEC_H264,
	VDEC_HEVC,
	VDEC_INDEO,
	VDEC_LOSSLESS,
	VDEC_MJPEG,
	VDEC_MPEG1,
	VDEC_MPEG2,
	VDEC_LIBMPEG2_MPEG1,
	VDEC_LIBMPEG2_MPEG2,
	VDEC_MSMPEG4,
	VDEC_PNG,
	VDEC_QT,
	VDEC_SCREEN,
	VDEC_SVQ,
	VDEC_THEORA,
	VDEC_UT,
	VDEC_VC1,
	VDEC_VP356,
	VDEC_VP789,
	VDEC_WMV,
	VDEC_XVID,
	VDEC_REAL,
	VDEC_UNCOMPRESSED,

	VDEC_LAST
};

enum AUDIO_FILTER {
	ADEC_AAC,
	ADEC_AC3,
	ADEC_ALAC,
	ADEC_ALS,
	ADEC_AMR,
	ADEC_APE,
	ADEC_BINK,
	ADEC_TRUESPEECH,
	ADEC_DTS,
	ADEC_DSD,
	ADEC_FLAC,
	ADEC_INDEO,
	ADEC_LPCM,
	ADEC_MPA,
	ADEC_MUSEPACK,
	ADEC_NELLY,
	ADEC_OPUS,
	ADEC_PS2,
	ADEC_QDM2,
	ADEC_REAL,
	ADEC_SHORTEN,
	ADEC_SPEEX,
	ADEC_TAK,
	ADEC_TTA,
	ADEC_VORBIS,
	ADEC_VOXWARE,
	ADEC_WAVPACK,
	ADEC_WMA,
	ADEC_WMA9,
//	ADEC_WMALOSSLESS, // bad support in FFmpeg
	ADEC_WMAVOICE,
	ADEC_PCM_ADPCM,

	ADEC_LAST
};
