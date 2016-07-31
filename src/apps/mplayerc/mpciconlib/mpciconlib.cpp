/*
 * (C) 2008-2016 see Authors.txt
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

#include <Windows.h>
#include <tchar.h>
#include "resource.h"

extern "C" __declspec(dllexport) int get_icon_index(LPCTSTR ext)
{
	int iconindex = IDI_NONE;

	// icons by type
	if (_tcsicmp(ext, _T(":video")) == 0) {
		iconindex = IDI_DEFAULT_VIDEO_ICON;
	} else if (_tcsicmp(ext, _T(":audio")) == 0) {
		iconindex = IDI_DEFAULT_AUDIO_ICON;
	} else if (_tcsicmp(ext, _T(":playlist")) == 0) {
		iconindex = IDI_PLAYLIST_ICON;
	}
	// icons by extension
	else if (_tcsicmp(ext, _T(".3g2")) == 0) {
		iconindex = IDI_3G2_ICON;
	} else if (_tcsicmp(ext, _T(".3ga")) == 0
			|| _tcsicmp(ext, _T(".3gp")) == 0) {
		iconindex = IDI_3GP_ICON;
	} else if (_tcsicmp(ext, _T(".3gp2")) == 0) {
		iconindex = IDI_3GP2_ICON;
	} else if (_tcsicmp(ext, _T(".3gpp")) == 0) {
		iconindex = IDI_3GPP_ICON;
	} else if (_tcsicmp(ext, _T(".aac")) == 0) {
		iconindex = IDI_AAC_ICON;
	} else if (_tcsicmp(ext, _T(".ac3")) == 0) {
		iconindex = IDI_AC3_ICON;
	} else if (_tcsicmp(ext, _T(".aif")) == 0) {
		iconindex = IDI_AIF_ICON;
	} else if (_tcsicmp(ext, _T(".aifc")) == 0) {
		iconindex = IDI_AIFC_ICON;
	} else if (_tcsicmp(ext, _T(".aiff")) == 0) {
		iconindex = IDI_AIFF_ICON;
	} else if (_tcsicmp(ext, _T(".alac")) == 0) {
		iconindex = IDI_ALAC_ICON;
	} else if (_tcsicmp(ext, _T(".amr")) == 0
			|| _tcsicmp(ext, _T(".awb")) == 0) {
		iconindex = IDI_AMR_ICON;
	} else if (_tcsicmp(ext, _T(".amv")) == 0) {
		iconindex = IDI_AMV_ICON;
	} else if (_tcsicmp(ext, _T(".aob")) == 0) {
		iconindex = IDI_AOB_ICON;
	} else if (_tcsicmp(ext, _T(".ape")) == 0) {
		iconindex = IDI_APE_ICON;
	} else if (_tcsicmp(ext, _T(".apl")) == 0) {
		iconindex = IDI_APL_ICON;
	} else if (_tcsicmp(ext, _T(".asf")) == 0) {
		iconindex = IDI_ASF_ICON;
	} else if (_tcsicmp(ext, _T(".au")) == 0) {
		iconindex = IDI_AU_ICON;
	} else if (_tcsicmp(ext, _T(".avi")) == 0) {
		iconindex = IDI_AVI_ICON;
	} else if (_tcsicmp(ext, _T(".bik")) == 0) {
		iconindex = IDI_BIK_ICON;
	} else if (_tcsicmp(ext, _T(".cda")) == 0) {
		iconindex = IDI_CDA_ICON;
	} else if (_tcsicmp(ext, _T(".divx")) == 0) {
		iconindex = IDI_DIVX_ICON;
	} else if (_tcsicmp(ext, _T(".dsa")) == 0) {
		iconindex = IDI_DSA_ICON;
	} else if (_tcsicmp(ext, _T(".dsm")) == 0) {
		iconindex = IDI_DSM_ICON;
	} else if (_tcsicmp(ext, _T(".dss")) == 0) {
		iconindex = IDI_DSS_ICON;
	} else if (_tcsicmp(ext, _T(".dsv")) == 0) {
		iconindex = IDI_DSV_ICON;
	} else if (_tcsicmp(ext, _T(".dts")) == 0
			|| _tcsicmp(ext, _T(".dtshd")) == 0) {
		iconindex = IDI_DTS_ICON;
	} else if (_tcsicmp(ext, _T(".evo")) == 0) {
		iconindex = IDI_EVO_ICON;
	} else if (_tcsicmp(ext, _T(".f4v")) == 0) {
		iconindex = IDI_F4V_ICON;
	} else if (_tcsicmp(ext, _T(".flac")) == 0) {
		iconindex = IDI_FLAC_ICON;
	} else if (_tcsicmp(ext, _T(".flc")) == 0) {
		iconindex = IDI_FLC_ICON;
	} else if (_tcsicmp(ext, _T(".fli")) == 0) {
		iconindex = IDI_FLI_ICON;
	} else if (_tcsicmp(ext, _T(".flic")) == 0) {
		iconindex = IDI_FLIC_ICON;
	} else if (_tcsicmp(ext, _T(".flv")) == 0
			|| _tcsicmp(ext, _T(".iflv")) == 0) {
		iconindex = IDI_FLV_ICON;
	} else if (_tcsicmp(ext, _T(".hdmov")) == 0) {
		iconindex = IDI_HDMOV_ICON;
	} else if (_tcsicmp(ext, _T(".ifo")) == 0) {
		iconindex = IDI_IFO_ICON;
	} else if (_tcsicmp(ext, _T(".ivf")) == 0) {
		iconindex = IDI_IVF_ICON;
	} else if (_tcsicmp(ext, _T(".m1a")) == 0) {
		iconindex = IDI_M1A_ICON;
	} else if (_tcsicmp(ext, _T(".m1v")) == 0) {
		iconindex = IDI_M1V_ICON;
	} else if (_tcsicmp(ext, _T(".m2a")) == 0) {
		iconindex = IDI_M2A_ICON;
	} else if (_tcsicmp(ext, _T(".m2p")) == 0) {
		iconindex = IDI_M2P_ICON;
	} else if (_tcsicmp(ext, _T(".m2t")) == 0
			|| _tcsicmp(ext, _T(".ssif")) == 0) {
		iconindex = IDI_M2T_ICON;
	} else if (_tcsicmp(ext, _T(".m2ts")) == 0) {
		iconindex = IDI_M2TS_ICON;
	} else if (_tcsicmp(ext, _T(".m2v")) == 0) {
		iconindex = IDI_M2V_ICON;
	} else if (_tcsicmp(ext, _T(".m4a")) == 0) {
		iconindex = IDI_M4A_ICON;
	} else if (_tcsicmp(ext, _T(".m4b")) == 0) {
		iconindex = IDI_M4B_ICON;
	} else if (_tcsicmp(ext, _T(".m4v")) == 0) {
		iconindex = IDI_M4V_ICON;
	} else if (_tcsicmp(ext, _T(".mid")) == 0) {
		iconindex = IDI_MID_ICON;
	} else if (_tcsicmp(ext, _T(".midi")) == 0) {
		iconindex = IDI_MIDI_ICON;
	} else if (_tcsicmp(ext, _T(".mka")) == 0) {
		iconindex = IDI_MKA_ICON;
	} else if (_tcsicmp(ext, _T(".mkv")) == 0
			|| _tcsicmp(ext, _T(".mk3d")) == 0) {
		iconindex = IDI_MKV_ICON;
	} else if (_tcsicmp(ext, _T(".mlp")) == 0) {
		iconindex = IDI_MLP_ICON;
	} else if (_tcsicmp(ext, _T(".mov")) == 0
			|| _tcsicmp(ext, _T(".qt")) == 0) {
		iconindex = IDI_MOV_ICON;
	} else if (_tcsicmp(ext, _T(".mp2")) == 0) {
		iconindex = IDI_MP2_ICON;
	} else if (_tcsicmp(ext, _T(".mp2v")) == 0) {
		iconindex = IDI_MP2V_ICON;
	} else if (_tcsicmp(ext, _T(".mp3")) == 0) {
		iconindex = IDI_MP3_ICON;
	} else if (_tcsicmp(ext, _T(".mp4")) == 0) {
		iconindex = IDI_MP4_ICON;
	} else if (_tcsicmp(ext, _T(".mp4v")) == 0) {
		iconindex = IDI_MP4V_ICON;
	} else if (_tcsicmp(ext, _T(".mpa")) == 0) {
		iconindex = IDI_MPA_ICON;
	} else if (_tcsicmp(ext, _T(".mpc")) == 0) {
		iconindex = IDI_MPC_ICON;
	} else if (_tcsicmp(ext, _T(".mpe")) == 0) {
		iconindex = IDI_MPE_ICON;
	} else if (_tcsicmp(ext, _T(".mpeg")) == 0) {
		iconindex = IDI_MPEG_ICON;
	} else if (_tcsicmp(ext, _T(".mpg")) == 0
			|| _tcsicmp(ext, _T(".pss")) == 0) {
		iconindex = IDI_MPG_ICON;
	} else if (_tcsicmp(ext, _T(".mpv2")) == 0) {
		iconindex = IDI_MPV2_ICON;
	} else if (_tcsicmp(ext, _T(".mpv4")) == 0) {
		iconindex = IDI_MPV4_ICON;
	} else if (_tcsicmp(ext, _T(".mts")) == 0) {
		iconindex = IDI_MTS_ICON;
	} else if (_tcsicmp(ext, _T(".ofr")) == 0) {
		iconindex = IDI_OFR_ICON;
	} else if (_tcsicmp(ext, _T(".ofs")) == 0) {
		iconindex = IDI_OFS_ICON;
	} else if (_tcsicmp(ext, _T(".oga")) == 0) {
		iconindex = IDI_OGA_ICON;
	} else if (_tcsicmp(ext, _T(".ogg")) == 0) {
		iconindex = IDI_OGG_ICON;
	} else if (_tcsicmp(ext, _T(".ogm")) == 0) {
		iconindex = IDI_OGM_ICON;
	} else if (_tcsicmp(ext, _T(".ogv")) == 0) {
		iconindex = IDI_OGV_ICON;
//	} else if (_tcsicmp(ext, _T(".plc")) == 0) {
//		iconindex = IDI_PLC_ICON;
	} else if (_tcsicmp(ext, _T(".pva")) == 0) {
		iconindex = IDI_PVA_ICON;
	} else if (_tcsicmp(ext, _T(".ra")) == 0) {
		iconindex = IDI_RA_ICON;
	} else if (_tcsicmp(ext, _T(".ram")) == 0) {
		iconindex = IDI_RAM_ICON;
	} else if (_tcsicmp(ext, _T(".rat")) == 0
			|| _tcsicmp(ext, _T(".ratdvd")) == 0) {
		iconindex = IDI_RATDVD_ICON;
	} else if (_tcsicmp(ext, _T(".rec")) == 0) {
		iconindex = IDI_REC_ICON;
	} else if (_tcsicmp(ext, _T(".rm")) == 0
			|| _tcsicmp(ext, _T(".rmvb")) == 0) {
		iconindex = IDI_RM_ICON;
	} else if (_tcsicmp(ext, _T(".rmi")) == 0) {
		iconindex = IDI_RMI_ICON;
	} else if (_tcsicmp(ext, _T(".rmm")) == 0) {
		iconindex = IDI_RMM_ICON;
	} else if (_tcsicmp(ext, _T(".roq")) == 0) {
		iconindex = IDI_ROQ_ICON;
	} else if (_tcsicmp(ext, _T(".rp")) == 0) {
		iconindex = IDI_RP_ICON;
	} else if (_tcsicmp(ext, _T(".rt")) == 0) {
		iconindex = IDI_RT_ICON;
	} else if (_tcsicmp(ext, _T(".smil")) == 0) {
		iconindex = IDI_SMIL_ICON;
	} else if (_tcsicmp(ext, _T(".smk")) == 0) {
		iconindex = IDI_SMK_ICON;
	} else if (_tcsicmp(ext, _T(".snd")) == 0) {
		iconindex = IDI_SND_ICON;
	} else if (_tcsicmp(ext, _T(".swf")) == 0) {
		iconindex = IDI_SWF_ICON;
	} else if (_tcsicmp(ext, _T(".tak")) == 0) {
		iconindex = IDI_TAK_ICON;
	} else if (_tcsicmp(ext, _T(".tp")) == 0) {
		iconindex = IDI_TP_ICON;
	} else if (_tcsicmp(ext, _T(".trp")) == 0) {
		iconindex = IDI_TRP_ICON;
	} else if (_tcsicmp(ext, _T(".ts")) == 0) {
		iconindex = IDI_TS_ICON;
	} else if (_tcsicmp(ext, _T(".tta")) == 0) {
		iconindex = IDI_TTA_ICON;
	} else if (_tcsicmp(ext, _T(".vob")) == 0) {
		iconindex = IDI_VOB_ICON;
//	} else if (_tcsicmp(ext, _T(".vp6")) == 0) {
//		iconindex = IDI_VP6_ICON;
	} else if (_tcsicmp(ext, _T(".wav")) == 0) {
		iconindex = IDI_WAV_ICON;
//	} else if (_tcsicmp(ext, _T(".wbm")) == 0) {
//		iconindex = IDI_WBM_ICON;
	} else if (_tcsicmp(ext, _T(".webm")) == 0) {
		iconindex = IDI_WEBM_ICON;
	} else if (_tcsicmp(ext, _T(".wm")) == 0) {
		iconindex = IDI_WM_ICON;
	} else if (_tcsicmp(ext, _T(".wma")) == 0) {
		iconindex = IDI_WMA_ICON;
	} else if (_tcsicmp(ext, _T(".wmp")) == 0) {
		iconindex = IDI_WMP_ICON;
	} else if (_tcsicmp(ext, _T(".wmv")) == 0) {
		iconindex = IDI_WMV_ICON;
	} else if (_tcsicmp(ext, _T(".wv")) == 0) {
		iconindex = IDI_WV_ICON;
	} else if (_tcsicmp(ext, _T(".opus")) == 0) {
		iconindex = IDI_OPUS_ICON;
	}
	// playlists (need for compatibility with older versions)
	else if (_tcsicmp(ext, _T(".asx")) == 0
			|| _tcsicmp(ext, _T(".bdmv")) == 0
			|| _tcsicmp(ext, _T(".m3u")) == 0
			|| _tcsicmp(ext, _T(".m3u8")) == 0
			|| _tcsicmp(ext, _T(".mpcpl")) == 0
			|| _tcsicmp(ext, _T(".mpls")) == 0
			|| _tcsicmp(ext, _T(".pls")) == 0
			|| _tcsicmp(ext, _T(".wpl")) == 0
			|| _tcsicmp(ext, _T(".xspf")) == 0
			|| _tcsicmp(ext, _T(".cue")) == 0) {
		iconindex = IDI_PLAYLIST_ICON;
	}

	return iconindex;
}
