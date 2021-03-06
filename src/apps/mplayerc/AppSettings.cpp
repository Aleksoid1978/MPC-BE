/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2021 see Authors.txt
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

#include "stdafx.h"
#include <atlpath.h>
#include <d3d9types.h>
#include "MiniDump.h"
#include "Misc.h"
#include "PlayerYouTube.h"
#include "PPageFormats.h"
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/std_helper.h"
#include "../../filters/switcher/AudioSwitcher/IAudioSwitcherFilter.h"
#include "AppSettings.h"

const LPCWSTR channel_mode_sets[] = {
	//         ID          Name
	L"1.0", // SPK_MONO   "Mono"
	L"2.0", // SPK_STEREO "Stereo"
	L"4.0", // SPK_4_0    "4.0"
	L"5.0", // SPK_5_0    "5.0"
	L"5.1", // SPK_5_1    "5.1"
	L"7.1", // SPK_7_1    "7.1"
};

// CFiltersPrioritySettings

void CFiltersPrioritySettings::SaveSettings()
{
	for (const auto& [format, guid] : values) {
		AfxGetProfile().WriteString(IDS_R_FILTERS_PRIORITY, format, CStringFromGUID(guid));
	}
};

void CFiltersPrioritySettings::LoadSettings()
{
	SetDefault();

	for (auto& [format, guid] : values) {
		CString str = CStringFromGUID(guid);
		AfxGetProfile().ReadString(IDS_R_FILTERS_PRIORITY, format, str);
		guid = GUIDFromCString(str);
	}
};

// CAppSettings

CAppSettings::CAppSettings()
	: bInitialized(false)
	, bKeepHistory(true)
	, iRecentFilesNumber(20)
	, MRU(0, L"Recent File List", L"File%d", iRecentFilesNumber)
	, MRUDub(0, L"Recent Dub List", L"Dub%d", iRecentFilesNumber)
	, hAccel(nullptr)
	, nCmdlnWebServerPort(-1)
	, bShowDebugInfo(false)
	, bResetSettings(false)
	, bSubSaveExternalStyleFile(false)
{
	ResetSettings();

	// Internal source filters
	SrcFiltersKeys[SRC_AC3]					= L"src_ac3";
	SrcFiltersKeys[SRC_AMR]					= L"src_amr";
	SrcFiltersKeys[SRC_AVI]					= L"src_avi";
	SrcFiltersKeys[SRC_APE]					= L"src_ape";
	SrcFiltersKeys[SRC_BINK]				= L"src_bink";
	SrcFiltersKeys[SRC_CDDA]				= L"src_cdda";
	SrcFiltersKeys[SRC_CDXA]				= L"src_cdxa";
	SrcFiltersKeys[SRC_DSD]					= L"src_dsd";
	SrcFiltersKeys[SRC_DSM]					= L"src_dsm";
	SrcFiltersKeys[SRC_DTS]					= L"src_dts";
	SrcFiltersKeys[SRC_VTS]					= L"src_vts";
	SrcFiltersKeys[SRC_FLIC]				= L"src_flic";
	SrcFiltersKeys[SRC_FLAC]				= L"src_flac";
	SrcFiltersKeys[SRC_FLV]					= L"src_flv";
	SrcFiltersKeys[SRC_MATROSKA]			= L"src_matroska";
	SrcFiltersKeys[SRC_MP4]					= L"src_mp4";
	SrcFiltersKeys[SRC_MPA]					= L"src_mpa";
	SrcFiltersKeys[SRC_MPEG]				= L"src_mpeg";
	SrcFiltersKeys[SRC_MUSEPACK]			= L"src_musepack";
	SrcFiltersKeys[SRC_OGG]					= L"src_ogg";
	SrcFiltersKeys[SRC_RAWVIDEO]			= L"src_rawvideo";
	SrcFiltersKeys[SRC_REAL]				= L"src_real";
	SrcFiltersKeys[SRC_ROQ]					= L"src_roq";
	SrcFiltersKeys[SRC_SHOUTCAST]			= L"src_shoutcast";
	SrcFiltersKeys[SRC_STDINPUT]			= L"src_stdinput";
	SrcFiltersKeys[SRC_TAK]					= L"src_tak";
	SrcFiltersKeys[SRC_TTA]					= L"src_tta";
	SrcFiltersKeys[SRC_WAV]					= L"src_wav";
	SrcFiltersKeys[SRC_WAVPACK]				= L"src_wavpack";
	SrcFiltersKeys[SRC_UDP]					= L"src_udp";
	SrcFiltersKeys[SRC_DVR]					= L"src_dvr";
	SrcFiltersKeys[SRC_AIFF]				= L"src_aiff";

	// Internal DXVA decoders
	DXVAFiltersKeys[VDEC_DXVA_H264]			= L"vdec_dxva_h264";
	DXVAFiltersKeys[VDEC_DXVA_HEVC]			= L"vdec_dxva_hevc";
	DXVAFiltersKeys[VDEC_DXVA_MPEG2]		= L"vdec_dxva_mpeg2";
	DXVAFiltersKeys[VDEC_DXVA_VC1]			= L"vdec_dxva_vc1";
	DXVAFiltersKeys[VDEC_DXVA_WMV3]			= L"vdec_dxva_wmv3";
	DXVAFiltersKeys[VDEC_DXVA_VP9]			= L"vdec_dxva_vp9";

	// Internal video decoders
	VideoFiltersKeys[VDEC_AMV]				= L"vdec_amv";
	VideoFiltersKeys[VDEC_PRORES]			= L"vdec_prores";
	VideoFiltersKeys[VDEC_DNXHD]			= L"vdec_dnxhd";
	VideoFiltersKeys[VDEC_BINK]				= L"vdec_bink";
	VideoFiltersKeys[VDEC_CANOPUS]			= L"vdec_canopus";
	VideoFiltersKeys[VDEC_CINEFORM]			= L"vdec_cineform";
	VideoFiltersKeys[VDEC_CINEPAK]			= L"vdec_cinepak";
	VideoFiltersKeys[VDEC_DIRAC]			= L"vdec_dirac";
	VideoFiltersKeys[VDEC_DIVX]				= L"vdec_divx";
	VideoFiltersKeys[VDEC_DV]				= L"vdec_dv";
	VideoFiltersKeys[VDEC_FLV]				= L"vdec_flv";
	VideoFiltersKeys[VDEC_H263]				= L"vdec_h263";
	VideoFiltersKeys[VDEC_H264]				= L"vdec_h264";
	VideoFiltersKeys[VDEC_H264_MVC]			= L"vdec_h264_mvc";
	VideoFiltersKeys[VDEC_HEVC]				= L"vdec_hevc";
	VideoFiltersKeys[VDEC_INDEO]			= L"vdec_indeo";
	VideoFiltersKeys[VDEC_LOSSLESS]			= L"vdec_lossless";
	VideoFiltersKeys[VDEC_MJPEG]			= L"vdec_mjpeg";
	VideoFiltersKeys[VDEC_MPEG1]			= L"vdec_mpeg1";
	VideoFiltersKeys[VDEC_MPEG2]			= L"vdec_mpeg2";
	VideoFiltersKeys[VDEC_DVD_LIBMPEG2]		= L"vdec_dvd_libmpeg2";
	VideoFiltersKeys[VDEC_MSMPEG4]			= L"vdec_msmpeg4";
	VideoFiltersKeys[VDEC_PNG]				= L"vdec_png";
	VideoFiltersKeys[VDEC_QT]				= L"vdec_qt";
	VideoFiltersKeys[VDEC_SCREEN]			= L"vdec_screen";
	VideoFiltersKeys[VDEC_SVQ]				= L"vdec_svq";
	VideoFiltersKeys[VDEC_THEORA]			= L"vdec_theora";
	VideoFiltersKeys[VDEC_UT]				= L"vdec_ut";
	VideoFiltersKeys[VDEC_VC1]				= L"vdec_vc1";
	VideoFiltersKeys[VDEC_VP356]			= L"vdec_vp356";
	VideoFiltersKeys[VDEC_VP789]			= L"vdec_vp789";
	VideoFiltersKeys[VDEC_WMV]				= L"vdec_wmv";
	VideoFiltersKeys[VDEC_XVID]				= L"vdec_xvid";
	VideoFiltersKeys[VDEC_REAL]				= L"vdec_real";
	VideoFiltersKeys[VDEC_HAP]				= L"vdec_hap";
	VideoFiltersKeys[VDEC_AV1]				= L"vdec_av1";
	VideoFiltersKeys[VDEC_SHQ]				= L"vdec_shq";
	VideoFiltersKeys[VDEC_UNCOMPRESSED]		= L"vdec_uncompressed";

	// Internal audio decoders
	AudioFiltersKeys[ADEC_AAC]				= L"adec_aac";
	AudioFiltersKeys[ADEC_AC3]				= L"adec_ac3";
	AudioFiltersKeys[ADEC_ALAC]				= L"adec_alac";
	AudioFiltersKeys[ADEC_ALS]				= L"adec_als";
	AudioFiltersKeys[ADEC_AMR]				= L"adec_amr";
	AudioFiltersKeys[ADEC_APE]				= L"adec_ape";
	AudioFiltersKeys[ADEC_BINK]				= L"adec_bink";
	AudioFiltersKeys[ADEC_TRUESPEECH]		= L"adec_truespeech";
	AudioFiltersKeys[ADEC_DTS]				= L"adec_dts";
	AudioFiltersKeys[ADEC_DSD]				= L"adec_dsd";
	AudioFiltersKeys[ADEC_FLAC]				= L"adec_flac";
	AudioFiltersKeys[ADEC_INDEO]			= L"adec_indeo";
	AudioFiltersKeys[ADEC_LPCM]				= L"adec_lpcm";
	AudioFiltersKeys[ADEC_MPA]				= L"adec_mpa";
	AudioFiltersKeys[ADEC_MUSEPACK]			= L"adec_musepack";
	AudioFiltersKeys[ADEC_NELLY]			= L"adec_nelly";
	AudioFiltersKeys[ADEC_OPUS]				= L"adec_opus";
	AudioFiltersKeys[ADEC_PS2]				= L"adec_ps2";
	AudioFiltersKeys[ADEC_QDMC]				= L"adec_qdmc";
	AudioFiltersKeys[ADEC_REAL]				= L"adec_real";
	AudioFiltersKeys[ADEC_SHORTEN]			= L"adec_shorten";
	AudioFiltersKeys[ADEC_SPEEX]			= L"adec_speex";
	AudioFiltersKeys[ADEC_TAK]				= L"adec_tak";
	AudioFiltersKeys[ADEC_TTA]				= L"adec_tta";
	AudioFiltersKeys[ADEC_VORBIS]			= L"adec_vorbis";
	AudioFiltersKeys[ADEC_VOXWARE]			= L"adec_voxware";
	AudioFiltersKeys[ADEC_WAVPACK]			= L"adec_wavpack";
	AudioFiltersKeys[ADEC_WMA]				= L"adec_wma";
	AudioFiltersKeys[ADEC_WMA9]				= L"adec_wma9";
	AudioFiltersKeys[ADEC_WMALOSSLESS]		= L"adec_wmalossless";
	AudioFiltersKeys[ADEC_WMAVOICE]			= L"adec_wmavoice";
	AudioFiltersKeys[ADEC_PCM_ADPCM]		= L"adec_pcm_adpcm";
}

void CAppSettings::CreateCommands()
{
	wmcmds.clear();

#define ADDCMD(cmd) wmcmds.emplace_back(wmcmd##cmd)
	ADDCMD((ID_FILE_OPENFILE,			IDS_AG_OPEN_FILE,			'Q', FCONTROL));
	ADDCMD((ID_FILE_OPENFILEURL,		IDS_AG_OPEN_FILEURL,		'O', FCONTROL));
	ADDCMD((ID_FILE_OPENDVD,			IDS_AG_OPEN_DVD,			'D', FCONTROL));
	ADDCMD((ID_FILE_OPENDEVICE,			IDS_AG_OPEN_DEVICE,			'V', FCONTROL));
	ADDCMD((ID_FILE_OPENDIRECTORY,		IDS_AG_OPEN_DIR));
	ADDCMD((ID_FILE_OPENISO,			IDS_AG_OPEN_ISO));
	ADDCMD((ID_FILE_REOPEN,				IDS_AG_REOPEN,				'E', FCONTROL));

	ADDCMD((ID_FILE_SAVE_COPY,			IDS_AG_SAVE_AS));
	ADDCMD((ID_FILE_SAVE_IMAGE,			IDS_AG_SAVE_IMAGE,			'I', FALT));
	ADDCMD((ID_FILE_AUTOSAVE_IMAGE,		IDS_AG_AUTOSAVE_IMAGE,		VK_F5));
	ADDCMD((ID_FILE_AUTOSAVE_DISPLAY,	IDS_AG_AUTOSAVE_DISPLAY,	VK_F5, FSHIFT));
	ADDCMD((ID_COPY_IMAGE,				IDS_AG_COPY_IMAGE));
	ADDCMD((ID_FILE_SAVE_THUMBNAILS,	IDS_FILE_SAVE_THUMBNAILS));

	ADDCMD((ID_FILE_LOAD_SUBTITLE,		IDS_AG_LOAD_SUBTITLE,		'L', FCONTROL));
	ADDCMD((ID_FILE_SAVE_SUBTITLE,		IDS_AG_SAVE_SUBTITLE,		'S', FCONTROL));
	ADDCMD((ID_FILE_CLOSEPLAYLIST,		IDS_AG_CLOSE,				'C', FCONTROL));
	ADDCMD((ID_FILE_PROPERTIES,			IDS_AG_PROPERTIES,			VK_F10, FSHIFT));
	ADDCMD((ID_FILE_EXIT,				IDS_AG_EXIT,				'X', FALT));
	ADDCMD((ID_PLAY_PLAYPAUSE,			IDS_AG_PLAYPAUSE,			VK_SPACE, 0,	APPCOMMAND_MEDIA_PLAY_PAUSE, wmcmd::LDOWN, wmcmd::LDOWN));
	ADDCMD((ID_PLAY_PLAY,				IDS_AG_PLAY,				0, 0,			APPCOMMAND_MEDIA_PLAY));
	ADDCMD((ID_PLAY_PAUSE,				IDS_AG_PAUSE,				0, 0,			APPCOMMAND_MEDIA_PAUSE));
	ADDCMD((ID_PLAY_STOP,				IDS_AG_STOP,				VK_OEM_PERIOD, 0, APPCOMMAND_MEDIA_STOP));

	ADDCMD((ID_MENU_SUBTITLELANG,		IDS_AG_MENU_SUBTITLELANG,	'S', FALT));
	ADDCMD((ID_MENU_AUDIOLANG,			IDS_AG_MENU_AUDIOLANG,		'A', FALT));
	ADDCMD((ID_MENU_JUMPTO,				IDS_AG_MENU_JUMPTO,			'G', FALT));

	ADDCMD((ID_PLAY_FRAMESTEP,			IDS_AG_FRAMESTEP,			VK_RIGHT, FCONTROL));
	ADDCMD((ID_PLAY_FRAMESTEPCANCEL,	IDS_MPLAYERC_16,			VK_LEFT, FCONTROL));
	ADDCMD((ID_PLAY_GOTO,				IDS_AG_GO_TO,				'G',     FCONTROL));
	ADDCMD((ID_PLAY_INCRATE,			IDS_AG_INCREASE_RATE,		VK_UP,   FCONTROL));
	ADDCMD((ID_PLAY_DECRATE,			IDS_AG_DECREASE_RATE,		VK_DOWN, FCONTROL));
	ADDCMD((ID_PLAY_RESETRATE,			IDS_AG_RESET_RATE,			'R',     FCONTROL));
	ADDCMD((ID_PLAY_AUDIODELAY_PLUS,	IDS_AG_AUDIODELAY_PLUS,		VK_ADD));
	ADDCMD((ID_PLAY_AUDIODELAY_MINUS,	IDS_AG_AUDIODELAY_MINUS,	VK_SUBTRACT));
	ADDCMD((ID_PLAY_SEEKFORWARDSMALL,	IDS_AG_JUMP_FORWARD_1));
	ADDCMD((ID_PLAY_SEEKBACKWARDSMALL,	IDS_AG_JUMP_BACKWARD_1));
	ADDCMD((ID_PLAY_SEEKFORWARDMED,		IDS_AG_JUMP_FORWARD_2,		VK_RIGHT));
	ADDCMD((ID_PLAY_SEEKBACKWARDMED,	IDS_AG_JUMP_BACKWARD_2,		VK_LEFT));
	ADDCMD((ID_PLAY_SEEKFORWARDLARGE,	IDS_AG_JUMP_FORWARD_3));
	ADDCMD((ID_PLAY_SEEKBACKWARDLARGE,	IDS_AG_JUMP_BACKWARD_3));
	ADDCMD((ID_PLAY_SEEKKEYFORWARD,		IDS_AG_JUMP_FORWARD_K,		VK_RIGHT, FSHIFT));
	ADDCMD((ID_PLAY_SEEKKEYBACKWARD,	IDS_AG_JUMP_BACKWARD_K,		VK_LEFT,  FSHIFT));
	ADDCMD((ID_PLAY_SEEKBEGIN,			IDS_SEEKBEGIN,				VK_HOME));
	ADDCMD((ID_NAVIGATE_SKIPFORWARD,	IDS_AG_NEXT,				VK_NEXT,  0, APPCOMMAND_MEDIA_NEXTTRACK, wmcmd::X2DOWN, wmcmd::X2DOWN));
	ADDCMD((ID_NAVIGATE_SKIPBACK,		IDS_AG_PREVIOUS,			VK_PRIOR, 0, APPCOMMAND_MEDIA_PREVIOUSTRACK, wmcmd::X1DOWN, wmcmd::X1DOWN));
	ADDCMD((ID_NAVIGATE_SKIPFORWARDFILE, IDS_AG_NEXT_FILE,			VK_NEXT,  FCONTROL));
	ADDCMD((ID_NAVIGATE_SKIPBACKFILE,	IDS_AG_PREVIOUS_FILE,		VK_PRIOR, FCONTROL));
	ADDCMD((ID_REPEAT_FOREVER,			IDS_REPEAT_FOREVER));
	ADDCMD((ID_PLAY_REPEAT_AB,			IDS_PLAYLOOPMODE_AB));
	ADDCMD((ID_PLAY_REPEAT_AB_MARK_A,	IDS_PLAYLOOPMODE_AB_MARK_A,	VK_OEM_4));
	ADDCMD((ID_PLAY_REPEAT_AB_MARK_B,	IDS_PLAYLOOPMODE_AB_MARK_B,	VK_OEM_6));
	ADDCMD((ID_NAVIGATE_TUNERSCAN,		IDS_NAVIGATE_TUNERSCAN,		'T', FSHIFT));

	ADDCMD((ID_FAVORITES_QUICKADD,		IDS_FAVORITES_QUICKADD,		'Q', FSHIFT));
	ADDCMD((ID_MENU_FAVORITES,			IDS_AG_MENU_FAVORITES));

	ADDCMD((ID_VIEW_CAPTIONMENU,		IDS_AG_TOGGLE_CAPTION,		'0', FCONTROL));
	ADDCMD((ID_VIEW_SEEKER,				IDS_AG_TOGGLE_SEEKER,		'1', FCONTROL));
	ADDCMD((ID_VIEW_CONTROLS,			IDS_AG_TOGGLE_CONTROLS,		'2', FCONTROL));
	ADDCMD((ID_VIEW_INFORMATION,		IDS_AG_TOGGLE_INFO,			'3', FCONTROL));
	ADDCMD((ID_VIEW_STATISTICS,			IDS_AG_TOGGLE_STATS,		'4', FCONTROL));
	ADDCMD((ID_VIEW_STATUS,				IDS_AG_TOGGLE_STATUS,		'5', FCONTROL));
	ADDCMD((ID_VIEW_SUBRESYNC,			IDS_AG_TOGGLE_SUBRESYNC,	'6', FCONTROL));
	ADDCMD((ID_VIEW_PLAYLIST,			IDS_AG_TOGGLE_PLAYLIST,		'7', FCONTROL));
	ADDCMD((ID_VIEW_CAPTURE,			IDS_AG_TOGGLE_CAPTURE,		'8', FCONTROL));
	ADDCMD((ID_VIEW_SHADEREDITOR,		IDS_AG_TOGGLE_SHADER,		'9', FCONTROL));
	ADDCMD((ID_VIEW_PRESETS_MINIMAL,	IDS_AG_VIEW_MINIMAL,		'1'));
	ADDCMD((ID_VIEW_PRESETS_COMPACT,	IDS_AG_VIEW_COMPACT,		'2'));
	ADDCMD((ID_VIEW_PRESETS_NORMAL,		IDS_AG_VIEW_NORMAL,			'3'));
	ADDCMD((ID_VIEW_FULLSCREEN,			IDS_AG_FULLSCREEN,			VK_RETURN, FCONTROL, 0, wmcmd::LDBLCLK, wmcmd::LDBLCLK));
	ADDCMD((ID_VIEW_FULLSCREEN_2,		IDS_AG_FULLSCREEN_2,		VK_RETURN, FALT));
	ADDCMD((ID_VIEW_ZOOM_50,			IDS_AG_ZOOM_50,				'1', FALT));
	ADDCMD((ID_VIEW_ZOOM_100,			IDS_AG_ZOOM_100,			'2', FALT));
	ADDCMD((ID_VIEW_ZOOM_200,			IDS_AG_ZOOM_200,			'3', FALT));
	ADDCMD((ID_VIEW_ZOOM_AUTOFIT,		IDS_AG_ZOOM_AUTO_FIT,		'4', FALT));
	ADDCMD((ID_ASPECTRATIO_NEXT,		IDS_AG_NEXT_AR_PRESET));
	ADDCMD((ID_VIEW_VF_HALF,			IDS_AG_VIDFRM_HALF));
	ADDCMD((ID_VIEW_VF_NORMAL,			IDS_AG_VIDFRM_NORMAL));
	ADDCMD((ID_VIEW_VF_DOUBLE,			IDS_AG_VIDFRM_DOUBLE));
	ADDCMD((ID_VIEW_VF_STRETCH,			IDS_AG_VIDFRM_STRETCH));
	ADDCMD((ID_VIEW_VF_FROMINSIDE,		IDS_AG_VIDFRM_INSIDE));
	ADDCMD((ID_VIEW_VF_ZOOM1,			IDS_AG_VIDFRM_ZOOM1));
	ADDCMD((ID_VIEW_VF_ZOOM2,			IDS_AG_VIDFRM_ZOOM2));
	ADDCMD((ID_VIEW_VF_FROMOUTSIDE,		IDS_AG_VIDFRM_OUTSIDE));
	ADDCMD((ID_VIEW_VF_SWITCHZOOM,		IDS_AG_VIDFRM_SWITCHZOOM,	'P'));
	ADDCMD((ID_ONTOP_ALWAYS,			IDS_AG_ALWAYS_ON_TOP,		'A', FCONTROL));
	ADDCMD((ID_VIEW_RESET,				IDS_AG_PNS_RESET,			VK_NUMPAD5));
	ADDCMD((ID_VIEW_INCSIZE,			IDS_AG_PNS_INC_SIZE,		VK_NUMPAD9));
	ADDCMD((ID_VIEW_INCWIDTH,			IDS_AG_PNS_INC_WIDTH,		VK_NUMPAD6));
	ADDCMD((ID_VIEW_INCHEIGHT,			IDS_AG_PNS_INC_HEIGHT,		VK_NUMPAD8));
	ADDCMD((ID_VIEW_DECSIZE,			IDS_AG_PNS_DEC_SIZE,		VK_NUMPAD1));
	ADDCMD((ID_VIEW_DECWIDTH,			IDS_AG_PNS_DEC_WIDTH,		VK_NUMPAD4));
	ADDCMD((ID_VIEW_DECHEIGHT,			IDS_AG_PNS_DEC_HEIGHT,		VK_NUMPAD2));
	ADDCMD((ID_PANSCAN_CENTER,			IDS_AG_PNS_CENTER,			VK_NUMPAD5, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVELEFT,		IDS_AG_PNS_LEFT,			VK_NUMPAD4, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVERIGHT,		IDS_AG_PNS_RIGHT,			VK_NUMPAD6, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEUP,			IDS_AG_PNS_UP,				VK_NUMPAD8, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEDOWN,		IDS_AG_PNS_DOWN,			VK_NUMPAD2, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEUPLEFT,		IDS_AG_PNS_UPLEFT,			VK_NUMPAD7, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEUPRIGHT,		IDS_AG_PNS_UPRIGHT,			VK_NUMPAD9, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEDOWNLEFT,	IDS_AG_PNS_DOWNLEFT,		VK_NUMPAD1, FCONTROL));
	ADDCMD((ID_PANSCAN_MOVEDOWNRIGHT,	IDS_AG_PNS_DOWNRIGHT,		VK_NUMPAD3, FCONTROL));
	ADDCMD((ID_PANSCAN_ROTATE_CCW,		IDS_AG_PNS_ROTATE_CCW,		VK_NUMPAD1, FALT));
	ADDCMD((ID_PANSCAN_ROTATE_CW,		IDS_AG_PNS_ROTATE_CW,		VK_NUMPAD3, FALT));
	ADDCMD((ID_PANSCAN_FLIP,			IDS_AG_PNS_FLIP));
	ADDCMD((ID_VOLUME_UP,				IDS_AG_VOLUME_UP,			VK_UP,   0, 0, wmcmd::WUP, wmcmd::WUP));
	ADDCMD((ID_VOLUME_DOWN,				IDS_AG_VOLUME_DOWN,			VK_DOWN, 0, 0, wmcmd::WDOWN, wmcmd::WDOWN));
	ADDCMD((ID_VOLUME_MUTE,				IDS_AG_VOLUME_MUTE,			'M', FCONTROL));
	ADDCMD((ID_VOLUME_GAIN_INC,			IDS_VOLUME_GAIN_INC));
	ADDCMD((ID_VOLUME_GAIN_DEC,			IDS_VOLUME_GAIN_DEC));
	ADDCMD((ID_VOLUME_GAIN_OFF,			IDS_VOLUME_GAIN_OFF));
	ADDCMD((ID_VOLUME_GAIN_MAX,			IDS_VOLUME_GAIN_MAX));
	ADDCMD((ID_AUDIO_CENTER_INC,		IDS_CENTER_LEVEL_INC));
	ADDCMD((ID_AUDIO_CENTER_DEC,		IDS_CENTER_LEVEL_DEC));
	ADDCMD((ID_NORMALIZE,				IDS_TOGGLE_AUTOVOLUMECONTROL));
	ADDCMD((ID_COLOR_BRIGHTNESS_INC,	IDS_BRIGHTNESS_INC));
	ADDCMD((ID_COLOR_BRIGHTNESS_DEC,	IDS_BRIGHTNESS_DEC));
	ADDCMD((ID_COLOR_CONTRAST_INC,		IDS_CONTRAST_INC));
	ADDCMD((ID_COLOR_CONTRAST_DEC,		IDS_CONTRAST_DEC));
	//ADDCMD((ID_COLOR_HUE_INC,			IDS_HUE_INC)); // nobody need this feature
	//ADDCMD((ID_COLOR_HUE_DEC,			IDS_HUE_DEC)); // nobody need this feature
	ADDCMD((ID_COLOR_SATURATION_INC,	IDS_SATURATION_INC));
	ADDCMD((ID_COLOR_SATURATION_DEC,	IDS_SATURATION_DEC));
	ADDCMD((ID_COLOR_RESET,				IDS_RESET_COLOR));

	ADDCMD((ID_NAVIGATE_TITLEMENU,		IDS_AG_DVD_TITLEMENU,		'T', FALT));
	ADDCMD((ID_NAVIGATE_ROOTMENU,		IDS_AG_DVD_ROOTMENU,		'R', FALT));
	ADDCMD((ID_NAVIGATE_SUBPICTUREMENU,	IDS_AG_DVD_SUBPICTUREMENU));
	ADDCMD((ID_NAVIGATE_AUDIOMENU,		IDS_AG_DVD_AUDIOMENU));
	ADDCMD((ID_NAVIGATE_ANGLEMENU,		IDS_AG_DVD_ANGLEMENU));
	ADDCMD((ID_NAVIGATE_CHAPTERMENU,	IDS_AG_DVD_CHAPTERMENU));
	ADDCMD((ID_NAVIGATE_MENU_LEFT,		IDS_AG_DVD_MENU_LEFT,		VK_LEFT, FALT));
	ADDCMD((ID_NAVIGATE_MENU_RIGHT,		IDS_AG_DVD_MENU_RIGHT,		VK_RIGHT, FALT));
	ADDCMD((ID_NAVIGATE_MENU_UP,		IDS_AG_DVD_MENU_UP,			VK_UP, FALT));
	ADDCMD((ID_NAVIGATE_MENU_DOWN,		IDS_AG_DVD_MENU_DOWN,		VK_DOWN, FALT));
	ADDCMD((ID_NAVIGATE_MENU_ACTIVATE,	IDS_AG_DVD_MENU_ACTIVATE));
	ADDCMD((ID_NAVIGATE_MENU_BACK,		IDS_AG_DVD_MENU_BACK));
	ADDCMD((ID_NAVIGATE_MENU_LEAVE,		IDS_AG_DVD_MENU_LEAVE));

	ADDCMD((ID_BOSS,					IDS_AG_BOSS_KEY,			'B'));
	ADDCMD((ID_MENU_PLAYER_LONG,		IDS_AG_MENU_PLAYER_L,		VK_APPS, 0, 0, wmcmd::RUP, wmcmd::RUP));
	ADDCMD((ID_MENU_PLAYER_SHORT,		IDS_AG_MENU_PLAYER_S));
	ADDCMD((ID_MENU_FILTERS,			IDS_AG_MENU_FILTERS));
	ADDCMD((ID_VIEW_OPTIONS,			IDS_AG_OPTIONS,				'O'));
	ADDCMD((ID_STREAM_AUDIO_NEXT,		IDS_AG_NEXT_AUDIO,			'A'));
	ADDCMD((ID_STREAM_AUDIO_PREV,		IDS_AG_PREV_AUDIO,			'A', FSHIFT));
	ADDCMD((ID_STREAM_SUB_NEXT,			IDS_AG_NEXT_SUBTITLE,		'S'));
	ADDCMD((ID_STREAM_SUB_PREV,			IDS_AG_PREV_SUBTITLE,		'S', FSHIFT));
	ADDCMD((ID_STREAM_SUB_ONOFF,		IDS_AG_SUBTITLE_ONOFF,		'W'));
	ADDCMD((ID_SUBTITLES_RELOAD,		IDS_AG_SUBTITLE_RELOAD));
	// subtitle position
	ADDCMD((ID_SUB_POS_UP,				IDS_SUB_POS_UP,				VK_UP,     FCONTROL|FSHIFT));
	ADDCMD((ID_SUB_POS_DOWN,			IDS_SUB_POS_DOWN,			VK_DOWN,   FCONTROL|FSHIFT));
	ADDCMD((ID_SUB_POS_LEFT,			IDS_SUB_POS_LEFT,			VK_LEFT,   FCONTROL|FSHIFT));
	ADDCMD((ID_SUB_POS_RIGHT,			IDS_SUB_POS_RIGHT,			VK_RIGHT,  FCONTROL|FSHIFT));
	ADDCMD((ID_SUB_POS_RESTORE,			IDS_SUB_POS_RESTORE,		VK_DELETE, FCONTROL|FSHIFT));
	//
	ADDCMD((ID_SUB_COPYTOCLIPBOARD,		IDS_SUB_COPYTOCLIPBOARD));
	ADDCMD((ID_FILE_ISDB_DOWNLOAD,		IDS_DOWNLOAD_SUBS,			'D'));

	ADDCMD((ID_STREAM_VIDEO_NEXT,		IDS_AG_NEXT_VIDEO));
	ADDCMD((ID_STREAM_VIDEO_PREV,		IDS_AG_PREV_VIDEO));
	ADDCMD((ID_VIEW_TEARING_TEST,		IDS_AG_TEARING_TEST,		'T', FCONTROL));
	ADDCMD((ID_VIEW_REMAINING_TIME,		IDS_MPLAYERC_98,			'I', FCONTROL));
	ADDCMD((ID_OSD_LOCAL_TIME,			IDS_AG_OSD_LOCAL_TIME,		'I'));
	ADDCMD((ID_OSD_FILE_NAME,			IDS_AG_OSD_FILE_NAME,		'I', FSHIFT));
	ADDCMD((ID_SHADERS_TOGGLE,			IDS_AT_TOGGLE_SHADER,		'P', FCONTROL));
	ADDCMD((ID_SHADERS_TOGGLE_SCREENSPACE, IDS_AT_TOGGLE_SHADERSCREENSPACE, 'P', FCONTROL|FALT));
	ADDCMD((ID_D3DFULLSCREEN_TOGGLE,	IDS_MPLAYERC_99,			'F', FCONTROL));
	ADDCMD((ID_GOTO_PREV_SUB,			IDS_MPLAYERC_100,			'Y', 0, APPCOMMAND_BROWSER_BACKWARD));
	ADDCMD((ID_GOTO_NEXT_SUB,			IDS_MPLAYERC_101,			'U', 0, APPCOMMAND_BROWSER_FORWARD));
	ADDCMD((ID_SHIFT_SUB_DOWN,			IDS_MPLAYERC_102,			VK_NEXT, FALT));
	ADDCMD((ID_SHIFT_SUB_UP,			IDS_MPLAYERC_103,			VK_PRIOR, FALT));
	ADDCMD((ID_VIEW_DISPLAYSTATS,		IDS_AG_DISPLAY_STATS,		'J', FCONTROL));
	ADDCMD((ID_VIEW_RESETSTATS,			IDS_AG_RESET_STATS,			'R', FCONTROL|FALT));
	ADDCMD((ID_VIEW_VSYNC,				IDS_AG_VSYNC,				'V'));
	ADDCMD((ID_VIEW_VSYNCINTERNAL,		IDS_AG_VSYNCINTERNAL,		'V', FCONTROL|FALT));
	ADDCMD((ID_VIEW_VSYNCOFFSET_DECREASE, IDS_AG_VSYNCOFFSET_DECREASE, VK_UP, FCONTROL|FALT));
	ADDCMD((ID_VIEW_VSYNCOFFSET_INCREASE, IDS_AG_VSYNCOFFSET_INCREASE, VK_DOWN, FCONTROL|FALT));
	ADDCMD((ID_VIEW_ENABLEFRAMETIMECORRECTION, IDS_AG_ENABLEFRAMETIMECORRECTION, 'C'));
	ADDCMD((ID_SUB_DELAY_DEC,			IDS_AG_SUBDELAY_DEC,		VK_F1));
	ADDCMD((ID_SUB_DELAY_INC,			IDS_AG_SUBDELAY_INC,		VK_F2));

	ADDCMD((ID_MENU_AFTERPLAYBACK,		IDS_AG_MENU_AFTERPLAYBACK));
	ADDCMD((ID_AFTERPLAYBACK_CLOSE,		IDS_AFTERPLAYBACK_CLOSE));
	ADDCMD((ID_AFTERPLAYBACK_STANDBY,	IDS_AFTERPLAYBACK_STANDBY));
	ADDCMD((ID_AFTERPLAYBACK_HIBERNATE,	IDS_AFTERPLAYBACK_HIBERNATE));
	ADDCMD((ID_AFTERPLAYBACK_SHUTDOWN,	IDS_AFTERPLAYBACK_SHUTDOWN));
	ADDCMD((ID_AFTERPLAYBACK_LOGOFF,	IDS_AFTERPLAYBACK_LOGOFF));
	ADDCMD((ID_AFTERPLAYBACK_LOCK,		IDS_AFTERPLAYBACK_LOCK));
	ADDCMD((ID_AFTERPLAYBACK_EXIT,		IDS_AFTERPLAYBACK_EXIT));
	ADDCMD((ID_AFTERPLAYBACK_DONOTHING,	IDS_AFTERPLAYBACK_DONOTHING));
	ADDCMD((ID_AFTERPLAYBACK_NEXT,		IDS_AFTERPLAYBACK_NEXT));

	ADDCMD((ID_WINDOW_TO_PRIMARYSCREEN,	IDS_AG_WINDOW_TO_PRIMARYSCREEN, '1', FCONTROL|FALT));
#undef ADDCMD

	ResetPositions();
}

CAppSettings::~CAppSettings()
{
	if (hAccel) {
		DestroyAcceleratorTable(hAccel);
	}
}

bool CAppSettings::IsD3DFullscreen() const
{
	if (m_VRSettings.iVideoRenderer == VIDRNDT_EVR_CP) {
		return fD3DFullscreen || (nCLSwitches & CLSW_D3DFULLSCREEN);
	}

	return false;
}

CString CAppSettings::SelectedAudioRenderer() const
{
	CString strResult;
	if (AfxGetMyApp()->m_AudioRendererDisplayName_CL.GetLength() > 0) {
		strResult = AfxGetMyApp()->m_AudioRendererDisplayName_CL;
	} else {
		strResult = AfxGetAppSettings().strAudioRendererDisplayName;
	}

	return strResult;
}

void CAppSettings::ResetPositions()
{
	nCurrentDvdPosition  = -1;
	nCurrentFilePosition = -1;
}

DVD_POSITION* CAppSettings::CurrentDVDPosition()
{
	if (nCurrentDvdPosition != -1) {
		return &DvdPosition[nCurrentDvdPosition];
	} else {
		return nullptr;
	}
}

bool CAppSettings::NewDvd(ULONGLONG llDVDGuid)
{
	// Look for the DVD position
	for (int i = 0; i < std::min(iRecentFilesNumber, MAX_DVD_POSITION); i++) {
		if (DvdPosition[i].llDVDGuid == llDVDGuid) {
			nCurrentDvdPosition = i;
			return false;
		}
	}

	// If DVD is unknown, we put it first
	for (int i = std::min(iRecentFilesNumber, MAX_DVD_POSITION) - 1; i > 0; i--) {
		memcpy(&DvdPosition[i], &DvdPosition[i - 1], sizeof(DVD_POSITION));
	}
	DvdPosition[0].llDVDGuid = llDVDGuid;
	nCurrentDvdPosition      = 0;
	return true;
}

FILE_POSITION* CAppSettings::CurrentFilePosition()
{
	if (nCurrentFilePosition != -1) {
		return &FilePosition[nCurrentFilePosition];
	} else {
		return nullptr;
	}
}

bool CAppSettings::NewFile(LPCTSTR strFileName)
{
	// Look for the file position
	for (int i = 0; i < std::min(iRecentFilesNumber, MAX_FILE_POSITION); i++) {
		if (FilePosition[i].strFile == strFileName) {
			nCurrentFilePosition = i;
			return false;
		}
	}

	// If it is unknown, we put it first
	for (int i = std::min(iRecentFilesNumber, MAX_FILE_POSITION) - 1; i > 0; i--) {
		FilePosition[i].strFile        = FilePosition[i - 1].strFile;
		FilePosition[i].llPosition     = FilePosition[i - 1].llPosition;
		FilePosition[i].nAudioTrack    = FilePosition[i - 1].nAudioTrack;
		FilePosition[i].nSubtitleTrack = FilePosition[i - 1].nSubtitleTrack;
	}
	FilePosition[0].strFile        = strFileName;
	FilePosition[0].llPosition     = 0;
	FilePosition[0].nAudioTrack    = -1;
	FilePosition[0].nSubtitleTrack = -1;

	nCurrentFilePosition = 0;
	return true;
}

bool CAppSettings::RemoveFile(LPCTSTR strFileName)
{
	// Look for the file position
	int idx = -1;

	for (int i = 0; i < std::min(iRecentFilesNumber, MAX_FILE_POSITION); i++) {
		if (FilePosition[i].strFile == strFileName) {
			idx = i;
			break;
		}
	}

	if (idx != -1) {
		for (int i = idx; i < MAX_FILE_POSITION - 1; i++) {
			FilePosition[i].strFile        = FilePosition[i + 1].strFile;
			FilePosition[i].llPosition     = FilePosition[i + 1].llPosition;
			FilePosition[i].nAudioTrack    = FilePosition[i + 1].nAudioTrack;
			FilePosition[i].nSubtitleTrack = FilePosition[i + 1].nSubtitleTrack;
		}

		return true;
	}

	return false;
}

void CAppSettings::DeserializeHex(LPCTSTR strVal, BYTE* pBuffer, int nBufSize)
{
	long lRes;

	for (int i = 0; i < nBufSize; i++) {
		swscanf_s(strVal+(i*2), L"%02x", &lRes);
		pBuffer[i] = (BYTE)lRes;
	}
}

CString CAppSettings::SerializeHex(BYTE* pBuffer, int nBufSize) const
{
	CString strTemp;
	CString strResult;

	for (int i = 0; i < nBufSize; i++) {
		strTemp.Format(L"%02x", pBuffer[i]);
		strResult += strTemp;
	}

	return strResult;
}

void CAppSettings::ResetSettings()
{
	iLanguage = -1;
	iCurrentLanguage = CMPlayerCApp::GetLanguageIndex(ID_LANGUAGE_ENGLISH);

	iCaptionMenuMode = MODE_SHOWCAPTIONMENU;
	fHideNavigation = false;
	nCS = CS_SEEKBAR | CS_TOOLBAR | CS_STATUSBAR;

	iDefaultVideoSize = DVS_FROMINSIDE;
	bNoSmallUpscale = false;
	bNoSmallDownscale = false;
	fKeepAspectRatio = true;
	fCompMonDeskARDiff = false;

	nVolume = 100;
	nBalance = 0;
	fMute = false;
	nLoops = 1;
	fLoopForever = false;
	fRewind = false;
	nVolumeStep = 5;
	nSpeedStep = 0; // auto
	bSpeedNotReset = false;

	strAudioRendererDisplayName.Empty();
	strSecondAudioRendererDisplayName.Empty();
	fDualAudioOutput = false;

	fAutoloadAudio = true;
	fPrioritizeExternalAudio = false;
	strAudioPaths = DEFAULT_AUDIO_PATHS;

	iSubtitleRenderer = SUBRNDT_ISR;
	strSubtitlesLanguageOrder.Empty();
	strAudiosLanguageOrder.Empty();
	fUseInternalSelectTrackLogic = true;

	bRememberSelectedTracks = false;

	nAudioWindowMode = 1;
	nAddSimilarFiles = 0;
	fEnableWorkerThreadForOpening = true;
	fReportFailedPins = true;

	iMultipleInst = 1;
	iTitleBarTextStyle = TEXTBAR_FILENAME;
	iSeekBarTextStyle = TEXTBAR_TITLE;

	iOnTop = 0;
	bTrayIcon = false;
	fShowBarsWhenFullScreen = true;
	nShowBarsWhenFullScreenTimeOut = 0;

	//Multi-monitor code
	strFullScreenMonitor.Empty();
	// DeviceID
	strFullScreenMonitorID.Empty();
	iDMChangeDelay = 0;

	// Prevent Minimize when in Fullscreen mode on non default monitor
	fPreventMinimize = false;

	bPauseMinimizedVideo = false;

	fUseWin7TaskBar = true;

	fExitAfterPlayback = false;
	bCloseFileAfterPlayback = false;
	fNextInDirAfterPlayback = false;
	fNextInDirAfterPlaybackLooped = false;
	fDontUseSearchInFolder = false;

	fUseTimeTooltip = true;
	nTimeTooltipPosition = TIME_TOOLTIP_ABOVE_SEEKBAR;
	nOSDSize = 18;
	strOSDFont = L"Segoe UI";

	// Associated types with icon or not...
	bAssociatedWithIcons = true;
	// file/dir context menu
	bSetContextFiles = false;
	bSetContextDir = false;

	// Last Open Dir
	strLastOpenDir = L"C:\\";
	// Last Saved Playlist Dir
	strLastSavedPlaylistDir = L"C:\\";

	fullScreenModes.res.clear();
	fullScreenModes.bEnabled = FALSE;
	fullScreenModes.bApplyDefault = false;

	ZeroMemory(&AccelTblColWidth, sizeof(AccelTbl));

	fExitFullScreenAtTheEnd = true;
	fExitFullScreenAtFocusLost = false;
	fRestoreResAfterExit = true;
	bSavePnSZoom = false;
	dZoomX = 1.0;
	dZoomY = 1.0;
	sizeAspectRatio.cx = 0;
	sizeAspectRatio.cy = 0;
	bKeepHistory = true;

	// Window size
	nStartupWindowMode = STARTUPWND_DEFAULT;
	szSpecifiedWndSize.SetSize(460, 390);
	nPlaybackWindowMode = PLAYBACKWND_NONE;
	nAutoScaleFactor = 100;
	nAutoFitFactor = 50;
	bResetWindowAfterClosingFile = false;
	bRememberWindowPos = false;
	ptLastWindowPos.SetPoint(0, 0);
	szLastWindowSize.SetSize(0, 0);
	nLastWindowType = SIZE_RESTORED;
	bLimitWindowProportions = false;
	bSnapToDesktopEdges = false;

	bShufflePlaylistItems = false;
	bRememberPlaylistItems = true;
	bHidePlaylistFullScreen = false;
	bShowPlaylistTooltip = true;
	bShowPlaylistSearchBar = true;
	bPlaylistNextOnError = true;
	bPlaylistDetermineDuration = false;

	bFavRememberPos = true;
	bFavRelativeDrive = false;

	strDVDPath.Empty();
	bUseDVDPath = false;

	LANGID langID = GetUserDefaultUILanguage();
	idMenuLang = langID;
	idAudioLang = langID;
	idSubtitlesLang = langID;

	bClosedCaptions = false;

	subdefstyle.SetDefault();
	subdefstyle.relativeTo = 1;
	fOverridePlacement = false;
	nHorPos = 50;
	nVerPos = 90;
	nSubDelayInterval = 500;

	fEnableSubtitles = true;
	fForcedSubtitles = false;
	fPrioritizeExternalSubtitles = true;
	fDisableInternalSubtitles = false;
	fAutoReloadExtSubtitles = false;
	fUseSybresync = false;
	strSubtitlePaths = DEFAULT_SUBTITLE_PATHS;

	fUseDefaultSubtitlesStyle = false;
	bAudioTimeShift = false;
	iAudioTimeShift = 0;

	iBufferDuration = APP_BUFDURATION_DEF;
	iNetworkTimeout = APP_NETTIMEOUT_DEF;

	bAudioMixer = false;
	nAudioMixerLayout = SPK_STEREO;
	bAudioStereoFromDecoder = false;
	bAudioBassRedirect = false;
	dAudioCenter_dB = 0.0;
	dAudioSurround_dB = 0.0;
	dAudioGain_dB = 0.0;
	bAudioAutoVolumeControl = false;
	bAudioNormBoost = true;
	iAudioNormLevel = 75;
	iAudioNormRealeaseTime = 8;
	iAudioSampleFormats = SFMT_MASK;

	m_filters.RemoveAll();

	// Keys
	strWinLircAddr = L"127.0.0.1:8765";
	bWinLirc = false;
	strUIceAddr = L"127.0.0.1:1234";
	bUIce = false;
	bGlobalMedia = true;

	// Mouse
	nMouseLeftClick     = ID_PLAY_PLAYPAUSE;
	nMouseLeftDblClick  = ID_VIEW_FULLSCREEN;
	nMouseRightClick    = 0;
	MouseMiddleClick    = { 0, 0, 0, 0 };
	MouseX1Click        = { ID_NAVIGATE_SKIPBACK, 0, 0, 0 };
	MouseX2Click        = { ID_NAVIGATE_SKIPFORWARD, 0, 0, 0 };
	MouseWheelUp        = { ID_VOLUME_UP, 0, 0, 0 };
	MouseWheelDown      = { ID_VOLUME_DOWN, 0, 0, 0 };
	MouseWheelLeft      = { 0, 0, 0, 0 };
	MouseWheelRight     = { 0, 0, 0, 0 };
	bMouseLeftClickOpenRecent = false;

	bUseDarkTheme = true;
	nThemeBrightness = 15;
	nThemeRed = 256;
	nThemeGreen = 256;
	nThemeBlue = 256;
	bDarkMenu = true;
	nOSDTransparent = 100;
	nOSDBorder = 1;

	clrFaceABGR = 0x00ffffff;
	clrOutlineABGR = 0x00868686;
	clrFontABGR = 0x00E0E0E0;
	clrGrad1ABGR = 0x00302820;
	clrGrad2ABGR = 0x00302820;

	nJumpDistS = DEFAULT_JUMPDISTANCE_1;
	nJumpDistM = DEFAULT_JUMPDISTANCE_2;
	nJumpDistL = DEFAULT_JUMPDISTANCE_3;

	iDlgPropX = 0;
	iDlgPropY = 0;

	// Internal filters
	for (auto& item : SrcFilters) {
		item = true;
	}
	for (auto& item : DXVAFilters) {
		item = true;
	}
	for (int f = 0; f < VDEC_COUNT; f++) {
		VideoFilters[f] = (f == VDEC_H264_MVC) ? false : true;
	}
	for (auto& item : AudioFilters) {
		item = true;
	}

	strLogoFileName.Empty();
	nLogoId = DEF_LOGO;
	bLogoExternal = false;

	bHideCDROMsSubMenu = false;

	dwPriority = NORMAL_PRIORITY_CLASS;
	fLaunchfullscreen = false;

	fEnableWebServer = false;
	nWebServerPort = APP_WEBSRVPORT_DEF;
	nWebServerQuality = APP_WEBSRVQUALITY_DEF;
	fWebServerPrintDebugInfo = false;
	fWebServerUseCompression = true;
	fWebServerLocalhostOnly = false;
	bWebUIEnablePreview = false;
	strWebRoot = L"*./webroot";
	strWebDefIndex = L"index.html;index.php";
	strWebServerCGI.Empty();

	CString MyPictures;
	WCHAR szPath[MAX_PATH] = { 0 };
	if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0, szPath))) {
		MyPictures = CString(szPath) + L"\\";
	}

	strSnapShotPath = MyPictures;
	strSnapShotExt = L".jpg";
	bSnapShotSubtitles = false;

	iThumbRows = 4;
	iThumbCols = 4;
	iThumbWidth = 1024;
	iThumbQuality = 85;
	iThumbLevelPNG = 7;

	bSubSaveExternalStyleFile = false;

	strISDb = L"www.opensubtitles.org/isdb";

	nLastUsedPage = 0;

	fD3DFullscreen = false;
	//fMonitorAutoRefreshRate = false;
	iStereo3DMode = STEREO3D_AUTO;
	bStereo3DSwapLR = false;

	iBrightness = 0;
	iContrast = 0;
	iHue = 0;
	iSaturation = 0;

	ShaderList.clear();
	ShaderListScreenSpace.clear();
	Shaders11PostScale.clear();
	bToggleShader = false;
	bToggleShaderScreenSpace = false;

	ShowOSD.Enable = 1;
	fFastSeek = true;
	bHideWindowedMousePointer = false;
	nMinMPlsDuration = 3;
	fMiniDump = false;
	CMiniDump::SetState(fMiniDump);

	fLCDSupport = false;
	fSmartSeek = false;
	iSmartSeekSize = 15;
	fChapterMarker = false;
	fFlybar = true;
	iPlsFontPercent = 100;
	fFlybarOnTop = false;
	fFontShadow = false;
	fFontAA = true;

	// Save analog capture settings
	iDefaultCaptureDevice = 0;
	strAnalogVideo = L"dummy";
	strAnalogAudio = L"dummy";
	iAnalogCountry = 1;

	strBDANetworkProvider.Empty();
	strBDATuner.Empty();
	strBDAReceiver.Empty();
	//sBDAStandard.Empty();
	iBDAScanFreqStart = 474000;
	iBDAScanFreqEnd = 858000;
	iBDABandwidth = 8;
	fBDAUseOffset = false;
	iBDAOffset = 166;
	nDVBLastChannel = 1;
	fBDAIgnoreEncryptedChannels = false;

	m_DVBChannels.clear();

	// playback positions for last played DVDs
	bRememberDVDPos = false;
	nCurrentDvdPosition = -1;
	memset(DvdPosition, 0, sizeof(DvdPosition));

	// playback positions for last played files
	bRememberFilePos = false;
	nCurrentFilePosition = -1;

	bStartMainTitle = false;
	bNormalStartDVD = true;

	fRemainingTime = false;

	strLastOpenFilterDir.Empty();

	bYoutubePageParser = true;
	YoutubeFormat.fmt = 0;
	YoutubeFormat.res = 720;
	YoutubeFormat.fps60 = false;
	YoutubeFormat.hdr = false;
	bYoutubeLoadPlaylist = false;

	bYDLEnable = true;
	iYDLMaxHeight = 720;
	bYDLMaximumQuality = false;

	strAceStreamAddress = L"http://127.0.0.1:6878/ace/getstream?id=%s";
	strTorrServerAddress = L"http://127.0.0.1:8090/torrent/play?link=%s&m3u=true";

	nLastFileInfoPage = 0;

	bUpdaterAutoCheck = false;
	nUpdaterDelay = 7;

	tUpdaterLastCheck = 0;

	bOSDRemainingTime = false;
	bOSDLocalTime = false;
	bOSDFileName = false;

	bPasteClipboardURL = false;

	youtubeSignatureCache.clear();

	strTabs.Empty();
}

void CAppSettings::LoadSettings(bool bForce/* = false*/)
{
	CMPlayerCApp* pApp = AfxGetMyApp();
	ASSERT(pApp);
	CProfile& profile = AfxGetProfile();

	UINT len;
	BYTE* ptr = nullptr;

	if (bInitialized && !bForce) {
		return;
	}

	// Set interface language first!
	CString str;
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_LANGUAGE, str);
	iLanguage = CMPlayerCApp::GetLanguageIndex(str);
	if (iLanguage < 0) {
		iLanguage = CMPlayerCApp::GetDefLanguage();
	}
	CMPlayerCApp::SetLanguage(iLanguage, false);

	FiltersPrioritySettings.LoadSettings();

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_HIDECAPTIONMENU, iCaptionMenuMode, MODE_SHOWCAPTIONMENU, MODE_BORDERLESS);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_HIDENAVIGATION, fHideNavigation);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_CONTROLSTATE, nCS);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_DEFAULTVIDEOFRAME, iDefaultVideoSize, DVS_HALF, DVS_ZOOM2);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_NOSMALLUPSCALE, bNoSmallUpscale);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_NOSMALLDOWNSCALE, bNoSmallDownscale);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_KEEPASPECTRATIO, fKeepAspectRatio);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_COMPMONDESKARDIFF, fCompMonDeskARDiff);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_VOLUME, nVolume, 0, 100);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_BALANCE, nBalance, -100, 100);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MUTE, fMute);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_LOOPNUM, nLoops);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_LOOP, fLoopForever);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REWIND, fRewind);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_VOLUME_STEP, nVolumeStep, 1, 10);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SPEED_STEP, nSpeedStep);
	nSpeedStep = discard(nSpeedStep, 0, { 10, 20, 25, 50, 100 });
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SPEED_NOTRESET, bSpeedNotReset);

	m_VRSettings.Load();

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_PLAYLISTTABSSETTINGS, strTabs);

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_AUDIORENDERERTYPE, strAudioRendererDisplayName);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_SECONDAUDIORENDERER, strSecondAudioRendererDisplayName);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_DUALAUDIOOUTPUT, fDualAudioOutput);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUTOLOADAUDIO, fAutoloadAudio);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PRIORITIZEEXTERNALAUDIO, fPrioritizeExternalAudio);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_AUDIOPATHS, strAudioPaths);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SUBTITLERENDERER, iSubtitleRenderer, SUBRNDT_NONE, SUBRNDT_XYSUBFILTER);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_SUBTITLESLANGORDER, strSubtitlesLanguageOrder);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_AUDIOSLANGORDER, strAudiosLanguageOrder);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_INTERNALSELECTTRACKLOGIC, fUseInternalSelectTrackLogic);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REMEMBERSELECTEDTRACKS, bRememberSelectedTracks);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUDIOWINDOWMODE, nAudioWindowMode, 0, 2);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_ADDSIMILARFILES, nAddSimilarFiles, 0, 2);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_ENABLEWORKERTHREADFOROPENING, fEnableWorkerThreadForOpening);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REPORTFAILEDPINS, fReportFailedPins);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_MULTIINST, iMultipleInst, 0, 2);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_TITLEBARTEXT, iTitleBarTextStyle, TEXTBAR_EMPTY, TEXTBAR_FULLPATH);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SEEKBARTEXT, iSeekBarTextStyle, TEXTBAR_EMPTY, TEXTBAR_FULLPATH);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_ONTOP, iOnTop, 0, 3);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_TRAYICON, bTrayIcon);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_FULLSCREENCTRLS, fShowBarsWhenFullScreen);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_FULLSCREENCTRLSTIMEOUT, nShowBarsWhenFullScreenTimeOut, -1, 10);

	//Multi-monitor code
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_FULLSCREENMONITOR, strFullScreenMonitor);
	// DeviceID
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_FULLSCREENMONITORID, strFullScreenMonitorID);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_DISPLAYMODECHANGEDELAY, iDMChangeDelay, 0, 9);

	// Prevent Minimize when in Fullscreen mode on non default monitor
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_PREVENT_MINIMIZE, fPreventMinimize);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PAUSEMINIMIZEDVIDEO, bPauseMinimizedVideo);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_WIN7TASKBAR, fUseWin7TaskBar);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_EXIT_AFTER_PB, fExitAfterPlayback);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_CLOSE_FILE_AFTER_PB, bCloseFileAfterPlayback);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_NEXT_AFTER_PB, fNextInDirAfterPlayback);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_NEXT_AFTER_PB_LOOPED, fNextInDirAfterPlaybackLooped);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MPC_NO_SEARCH_IN_FOLDER, fDontUseSearchInFolder);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USE_TIME_TOOLTIP, fUseTimeTooltip);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_TIME_TOOLTIP_POSITION, nTimeTooltipPosition, TIME_TOOLTIP_ABOVE_SEEKBAR, TIME_TOOLTIP_BELOW_SEEKBAR);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_OSD_SIZE, nOSDSize, 8, 26);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_OSD_FONT, strOSDFont);

	// Last Open Dir
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_LAST_OPEN_DIR, strLastOpenDir);
	// Last Saved Playlist Dir
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_LAST_SAVED_PLAYLIST_DIR, strLastSavedPlaylistDir);

	profile.ReadInt(IDS_RS_FULLSCREENRES, IDS_RS_FULLSCREENRES_ENABLE, fullScreenModes.bEnabled);
	profile.ReadBool(IDS_RS_FULLSCREENRES, IDS_RS_FULLSCREENRES_APPLY_DEF, fullScreenModes.bApplyDefault);
	fullScreenModes.res.clear();
	for (size_t cnt = 0; cnt < MaxMonitorId; cnt++) {
		fullScreenRes item;

		CString entry;
		entry.Format(L"MonitorId%u", cnt);
		if (!profile.ReadString(IDS_RS_FULLSCREENRES, entry, str) || str.IsEmpty()) {
			break;
		}

		entry.Format(L"Res%u", cnt);
		if (profile.ReadBinary(IDS_RS_FULLSCREENRES, entry, &ptr, len)) {
			if (len >= (sizeof(fpsmode) + 1)) {
				BYTE size = ptr[0];
				if (size && size <= MaxFullScreenModes && size * sizeof(fpsmode) == len - 1) {
					item.dmFullscreenRes.resize(size);
					memcpy(item.dmFullscreenRes.data(), ptr + 1, len - 1);

					item.monitorId = str;
					fullScreenModes.res.emplace_back(item);
				}
			}
			delete [] ptr;
		}
	}

	if (fullScreenModes.res.empty()) {
		fullScreenModes.bEnabled = FALSE;
		fullScreenModes.bApplyDefault = false;
	}

	if (profile.ReadBinary(IDS_R_SETTINGS, L"AccelTblColWidth", &ptr, len)) {
		if (len == sizeof(AccelTbl)) {
			memcpy(&AccelTblColWidth, ptr, sizeof(AccelTbl));
		} else {
			AccelTblColWidth.bEnable = false;
		}
		delete [] ptr;
	} else {
		AccelTblColWidth.bEnable = false;
	}

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_EXITFULLSCREENATTHEEND, fExitFullScreenAtTheEnd);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_EXITFULLSCREENATFOCUSLOST, fExitFullScreenAtFocusLost);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_RESTORERESAFTEREXIT, fRestoreResAfterExit);

	if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_PANSCANZOOM, str)
			&& swscanf_s(str, L"%f,%f", &dZoomX, &dZoomY) == 2
			&& dZoomX >= 0.2 && dZoomX <= 5.0
			&& dZoomY >= 0.2 && dZoomY <= 5.0) {
		bSavePnSZoom = true;
	} else {
		bSavePnSZoom = false;
		dZoomX = 1.0;
		dZoomY = 1.0;
	}

	// Window size
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_WINDOWMODESTARTUP, nStartupWindowMode, STARTUPWND_DEFAULT, STARTUPWND_SPECIFIED);
	CSize wndSize;
	if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_SPECIFIEDWINDOWSIZE, str) && swscanf_s(str, L"%d;%d", &wndSize.cx, &wndSize.cy) == 2) {
		if (wndSize.cx >= 480 && wndSize.cx <= 3840 && wndSize.cy >= 240 && wndSize.cy <= 2160) {
			szSpecifiedWndSize = wndSize;
		}
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REMEMBERWINDOWPOS, bRememberWindowPos);
	if (profile.ReadBinary(IDS_R_SETTINGS, IDS_RS_LASTWINDOWRECT, &ptr, len)) {
		if (len == sizeof(CRect)) {
			CRect rect;
			memcpy(&rect, ptr, sizeof(rect));
			ptLastWindowPos  = rect.TopLeft();
			szLastWindowSize = rect.Size();
		} else {
			bRememberWindowPos = false;
		}
		delete[] ptr;
	} else {
		bRememberWindowPos = false;
	}
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_LASTWINDOWTYPE, nLastWindowType);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_LIMITWINDOWPROPORTIONS, bLimitWindowProportions);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SNAPTODESKTOPEDGES, bSnapToDesktopEdges);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_WINDOWMODEPLAYBACK, nPlaybackWindowMode, PLAYBACKWND_NONE, PLAYBACKWND_FITSCREENLARGER);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUTOSCALEFACTOR, nAutoScaleFactor);
	nAutoScaleFactor = discard(nAutoScaleFactor, 100, { 50, 100, 200 });
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUTOFITFACTOR, nAutoFitFactor, 20, 80);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_RESETWINDOWAFTERCLOSINGFILE, bResetWindowAfterClosingFile);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_ASPECTRATIO_X, *(int*)&sizeAspectRatio.cx);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_ASPECTRATIO_Y, *(int*)&sizeAspectRatio.cy);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_KEEPHISTORY, bKeepHistory);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_RECENT_FILES_NUMBER, iRecentFilesNumber, APP_RECENTFILES_MIN, APP_RECENTFILES_MAX);
	MRU.SetSize(iRecentFilesNumber);
	MRUDub.SetSize(iRecentFilesNumber);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SHUFFLEPLAYLISTITEMS, bShufflePlaylistItems);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REMEMBERPLAYLISTITEMS, bRememberPlaylistItems);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_HIDEPLAYLISTFULLSCREEN, bHidePlaylistFullScreen);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SHOWPLAYLISTTOOLTIP, bShowPlaylistTooltip);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SHOWPLAYLISTSEARCHBAR, bShowPlaylistSearchBar);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PLAYLISTNEXTONERROR, bPlaylistNextOnError);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PLAYLISTDETERMINEDURATION, bPlaylistDetermineDuration);

	profile.ReadBool(IDS_R_FAVORITES, IDS_RS_FAV_REMEMBERPOS, bFavRememberPos);
	profile.ReadBool(IDS_R_FAVORITES, IDS_RS_FAV_RELATIVEDRIVE, bFavRelativeDrive);

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_DVDPATH, strDVDPath);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USEDVDPATH, bUseDVDPath);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_MENULANG, *(unsigned*)&idMenuLang);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_AUDIOLANG, *(unsigned*)&idAudioLang);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_SUBTITLESLANG, *(unsigned*)&idSubtitlesLang);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_CLOSEDCAPTIONS, bClosedCaptions);
	// TODO: rename subdefstyle -> defStyle
	{
		str.Empty();
		profile.ReadString(IDS_R_SETTINGS, IDS_RS_SPSTYLE, str);
		subdefstyle <<= str;
		if (str.IsEmpty()) {
			subdefstyle.relativeTo = 1;    //default "Position subtitles relative to the video frame" option is checked
		}
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SPOVERRIDEPLACEMENT, fOverridePlacement);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SPHORPOS, nHorPos, -10, 110);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SPVERPOS, nVerPos, -10, 110);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SUBDELAYINTERVAL, nSubDelayInterval);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_ENABLESUBTITLES, fEnableSubtitles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_FORCEDSUBTITLES, fForcedSubtitles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PRIORITIZEEXTERNALSUBTITLES, fPrioritizeExternalSubtitles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_DISABLEINTERNALSUBTITLES, fDisableInternalSubtitles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUTORELOADEXTSUBTITLES, fAutoReloadExtSubtitles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USE_SUBRESYNC, fUseSybresync);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_SUBTITLEPATHS, strSubtitlePaths);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USEDEFAULTSUBTITLESSTYLE, fUseDefaultSubtitlesStyle);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_ENABLEAUDIOTIMESHIFT, bAudioTimeShift);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUDIOTIMESHIFT, iAudioTimeShift, -600000, 600000);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_BUFFERDURATION, iBufferDuration, APP_BUFDURATION_MIN, APP_BUFDURATION_MAX);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_NETWORKTIMEOUT, iNetworkTimeout, APP_NETTIMEOUT_MIN, APP_NETTIMEOUT_MAX);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUDIOMIXER, bAudioMixer);
	if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_AUDIOMIXERLAYOUT, str)) {
		str.Trim();
		for (int i = SPK_MONO; i <= SPK_7_1; i++) {
			if (str == channel_mode_sets[i]) {
				nAudioMixerLayout = i;
				break;
			}
		}
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUDIOSTEREOFROMDECODER, bAudioStereoFromDecoder);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUDIOBASSREDIRECT, bAudioBassRedirect);
	profile.ReadDouble(IDS_R_SETTINGS, IDS_RS_AUDIOLEVELCENTER, dAudioCenter_dB, APP_AUDIOLEVEL_MIN, APP_AUDIOLEVEL_MAX);
	profile.ReadDouble(IDS_R_SETTINGS, IDS_RS_AUDIOLEVELSURROUND, dAudioSurround_dB, APP_AUDIOLEVEL_MIN, APP_AUDIOLEVEL_MAX);
	profile.ReadDouble(IDS_R_SETTINGS, IDS_RS_AUDIOGAIN, dAudioGain_dB, APP_AUDIOGAIN_MIN, APP_AUDIOGAIN_MAX);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUDIOAUTOVOLUMECONTROL, bAudioAutoVolumeControl);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_AUDIONORMBOOST, bAudioNormBoost);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUDIONORMLEVEL, iAudioNormLevel, 0, 100);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUDIONORMREALEASETIME, iAudioNormRealeaseTime, 5, 10);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_AUDIOSAMPLEFORMATS, iAudioSampleFormats);
	iAudioSampleFormats &= SFMT_MASK;

	{
		m_filters.RemoveAll();
		for (unsigned int i = 0; ; i++) {
			CString key;
			key.Format(L"%s\\%04u", IDS_R_FILTERS, i);
			CAutoPtr<FilterOverride> f(DNew FilterOverride);

			bool enabled = false;
			profile.ReadBool(key, L"Enabled", enabled);
			f->fDisabled = !enabled;

			int j = -1;
			profile.ReadInt(key, L"SourceType", j);
			if (j == 0) {
				f->type = FilterOverride::REGISTERED;
				profile.ReadString(key, L"DisplayName", f->dispname);
				profile.ReadString(key, L"Name", f->name);
				if (profile.ReadString(key, L"CLSID", str)) {
					f->clsid = GUIDFromCString(str);
				}
				if (f->clsid == CLSID_NULL) {
					CComPtr<IBaseFilter> pBF;
					CStringW FriendlyName;
					if (CreateFilter(f->dispname, &pBF, FriendlyName)) {
						f->clsid = GetCLSID(pBF);
						profile.WriteString(key, L"CLSID", CStringFromGUID(f->clsid));
					}
				}
			} else if (j == 1) {
				f->type = FilterOverride::EXTERNAL;
				profile.ReadString(key, L"Path", f->path);
				profile.ReadString(key, L"Name", f->name);
				if (profile.ReadString(key, L"CLSID", str)) {
					f->clsid = GUIDFromCString(str);
				}
			} else {
				profile.DeleteSection(key);
				break;
			}

			f->backup.clear();
			for (unsigned int i = 0; ; i++) {
				CString val;
				val.Format(L"org%04u", i);
				CString guid;
				profile.ReadString(key, val, guid);
				if (guid.IsEmpty()) {
					break;
				}
				f->backup.push_back(GUIDFromCString(guid));
			}
			if (f->backup.size() & 1) {
				f->backup.pop_back();
			}

			f->guids.clear();
			for (unsigned int i = 0; ; i++) {
				CString val;
				val.Format(L"mod%04u", i);
				CString guid;
				profile.ReadString(key, val, guid);
				if (guid.IsEmpty()) {
					break;
				}
				f->guids.push_back(GUIDFromCString(guid));
			}
			if (f->guids.size() & 1) {
				f->guids.pop_back();
			}

			f->iLoadType = -1;
			profile.ReadInt(key, L"LoadType", f->iLoadType);
			if (f->iLoadType < 0) {
				break;
			}

			f->dwMerit = MERIT_DO_NOT_USE + 1;
			profile.ReadUInt(key, L"Merit", *(unsigned*)&f->dwMerit);

			m_filters.AddTail(f);
		}
	}

	m_pnspresets.RemoveAll();
	for (int i = 0; i < (ID_PANNSCAN_PRESETS_END - ID_PANNSCAN_PRESETS_START); i++) {
		CString preset;
		preset.Format(L"Preset%d", i);
		if (!profile.ReadString(IDS_R_PNSPRESETS, preset, str) || str.IsEmpty()) {
			break;
		}
		m_pnspresets.Add(str);
	}

	if (m_pnspresets.IsEmpty()) {
		double _4p3		= 4.0/3.0;
		double _16p9	= 16.0/9.0;
		double _235p1	= 2.35/1.0;
		//double _185p1	= 1.85/1.0;
		//UNREFERENCED_PARAMETER(_185p1);

		CString str;
		str.Format(ResStr(IDS_SCALE_16_9), 0.5, 0.5, 1.0, _16p9 / _4p3);
		m_pnspresets.Add(str);
		str.Format(ResStr(IDS_SCALE_WIDESCREEN), 0.5, 0.5, _16p9 / _4p3, _16p9 / _4p3);
		m_pnspresets.Add(str);
		str.Format(ResStr(IDS_SCALE_ULTRAWIDE), 0.5, 0.5, _235p1 / _4p3, _235p1 / _4p3);
		m_pnspresets.Add(str);
	}

	// Keys
	CreateCommands();

	for (unsigned i = 0; i < wmcmds.size(); i++) {
		CString cmdmod;
		cmdmod.Format(L"CommandMod%u", i);
		if (!profile.ReadString(IDS_R_COMMANDS, cmdmod, str) || str.IsEmpty()) {
			break;
		}
		int cmd, fVirt, key, repcnt;
		UINT mouse, mouseFS, appcmd;
		WCHAR buff[128];
		int n;
		if (5 > (n = swscanf_s(str, L"%d %x %x %s %d %u %u %u", &cmd, &fVirt, &key, buff, _countof(buff), &repcnt, &mouse, &appcmd, &mouseFS))) {
			break;
		}

		for (auto& wc : wmcmds) {
			if (cmd == wc.cmd) {
				wc.cmd = cmd;
				wc.fVirt = fVirt;
				wc.key = key;
				if (n >= 6) {
					wc.mouse = mouse;
				}
				if (n >= 7) {
					wc.appcmd = appcmd;
				}
				// If there is no distinct bindings for windowed and
				// fullscreen modes we use the same for both.
				wc.mouseFS = (n >= 8) ? mouseFS : wc.mouse;
				wc.rmcmd = CStringA(buff).Trim('\"');
				wc.rmrepcnt = repcnt;

				break;
			}
		}
	}

	std::vector<ACCEL> Accel(wmcmds.size());
	for (size_t i = 0; i < Accel.size(); i++) {
		Accel[i] = wmcmds[i];
	}
	hAccel = CreateAcceleratorTableW(Accel.data(), Accel.size());

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_WINLIRCADDR, strWinLircAddr);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_WINLIRC, bWinLirc);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_UICEADDR, strUIceAddr);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_UICE, bUIce);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_GLOBALMEDIA, bGlobalMedia);

	// Mouse
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT, str)) {
		swscanf_s(str, L"%u", &nMouseLeftClick);
		if (nMouseLeftClick != 0 && nMouseLeftClick != ID_PLAY_PLAYPAUSE) {
			nMouseLeftClick = ID_PLAY_PLAYPAUSE;
		}
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT_DBLCLICK, str)) {
		swscanf_s(str, L"%u", &nMouseLeftDblClick);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_RIGHT, str)) {
		swscanf_s(str, L"%u", &nMouseRightClick);
		if (nMouseRightClick != ID_MENU_PLAYER_SHORT && nMouseRightClick != ID_MENU_PLAYER_LONG) {
			nMouseRightClick = 0;
		}
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_MIDDLE, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseMiddleClick.normal, &MouseMiddleClick.ctrl, &MouseMiddleClick.shift, &MouseMiddleClick.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_X1, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseX1Click.normal, &MouseX1Click.ctrl, &MouseX1Click.shift, &MouseX1Click.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_X2, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseX2Click.normal, &MouseX2Click.ctrl, &MouseX2Click.shift, &MouseX2Click.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_UP, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseWheelUp.normal, &MouseWheelUp.ctrl, &MouseWheelUp.shift, &MouseWheelUp.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_DOWN, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseWheelDown.normal, &MouseWheelDown.ctrl, &MouseWheelDown.shift, &MouseWheelDown.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_LEFT, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseWheelLeft.normal, &MouseWheelLeft.ctrl, &MouseWheelLeft.shift, &MouseWheelLeft.rbtn);
	}
	if (profile.ReadString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_RIGHT, str)) {
		swscanf_s(str, L"%u;%u;%u;%u", &MouseWheelRight.normal, &MouseWheelRight.ctrl, &MouseWheelRight.shift, &MouseWheelRight.rbtn);
	}
	profile.ReadBool(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT_OPENRECENT, bMouseLeftClickOpenRecent);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USEDARKTHEME, bUseDarkTheme);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THEMEBRIGHTNESS, nThemeBrightness);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THEMERED, nThemeRed);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THEMEGREEN, nThemeGreen);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THEMEBLUE, nThemeBlue);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_DARKMENU, bDarkMenu);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_OSD_TRANSPARENT, nOSDTransparent);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_OSD_BORDER, nOSDBorder);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_CLRFACEABGR, *(unsigned*)&clrFaceABGR);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_CLROUTLINEABGR, *(unsigned*)&clrOutlineABGR);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_OSD_FONTCOLOR, *(unsigned*)&clrFontABGR);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_OSD_GRAD1COLOR, *(unsigned*)&clrGrad1ABGR);
	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_OSD_GRAD2COLOR, *(unsigned*)&clrGrad2ABGR);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTS, nJumpDistS);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTM, nJumpDistM);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTL, nJumpDistL);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_DLGPROPX, iDlgPropX);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_DLGPROPY, iDlgPropY);

	// Internal filters
	for (int f = 0; f < SRC_COUNT; f++) {
		profile.ReadBool(IDS_R_INTERNAL_FILTERS, SrcFiltersKeys[f], SrcFilters[f]);
	}
	for (int f = 0; f < VDEC_DXVA_COUNT; f++) {
		profile.ReadBool(IDS_R_INTERNAL_FILTERS, DXVAFiltersKeys[f], DXVAFilters[f]);
	}
	for (int f = 0; f < VDEC_COUNT; f++) {
		profile.ReadBool(IDS_R_INTERNAL_FILTERS, VideoFiltersKeys[f], VideoFilters[f]);
	}
	for (int f = 0; f < ADEC_COUNT; f++) {
		profile.ReadBool(IDS_R_INTERNAL_FILTERS, AudioFiltersKeys[f], AudioFilters[f]);
	}

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_LOGOFILE, strLogoFileName);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_LOGOID, nLogoId);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_LOGOEXT, bLogoExternal);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_HIDECDROMSSUBMENU, bHideCDROMsSubMenu);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_PRIORITY, *(unsigned*)&dwPriority);
	::SetPriorityClass(::GetCurrentProcess(), dwPriority);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_LAUNCHFULLSCREEN, fLaunchfullscreen);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_ENABLEWEBSERVER, fEnableWebServer);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_WEBSERVERPORT, nWebServerPort, 1, 65535);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_WEBSERVERQUALITY, nWebServerQuality, APP_WEBSRVQUALITY_MIN, APP_WEBSRVQUALITY_MAX);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERPRINTDEBUGINFO, fWebServerPrintDebugInfo);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERUSECOMPRESSION, fWebServerUseCompression);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERLOCALHOSTONLY, fWebServerLocalhostOnly);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_WEBUI_ENABLE_PREVIEW, bWebUIEnablePreview);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_WEBROOT, strWebRoot);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_WEBDEFINDEX, strWebDefIndex);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_WEBSERVERCGI, strWebServerCGI);

	CString MyPictures;
	WCHAR szPath[MAX_PATH] = { 0 };
	if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0, szPath))) {
		MyPictures = CString(szPath) + L"\\";
	}

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_SNAPSHOTPATH, strSnapShotPath);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_SNAPSHOTEXT, strSnapShotExt);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SNAPSHOT_SUBTITLES, bSnapShotSubtitles);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THUMBROWS, iThumbRows, 1, 20);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THUMBCOLS, iThumbCols, 1, 10);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THUMBWIDTH, iThumbWidth, APP_THUMBWIDTH_MIN, APP_THUMBWIDTH_MAX);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THUMBQUALITY, iThumbQuality, 70, 100);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_THUMBLEVELPNG, iThumbLevelPNG, 1, 9);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SUBSAVEEXTERNALSTYLEFILE, bSubSaveExternalStyleFile);

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_ISDB, strISDb);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_LASTUSEDPAGE, nLastUsedPage);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_D3DFULLSCREEN, fD3DFullscreen);
	//profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MONITOR_AUTOREFRESHRATE, fMonitorAutoRefreshRate);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_STEREO3D_MODE, iStereo3DMode, STEREO3D_AUTO, STEREO3D_OVERUNDER);
	if (iStereo3DMode == ID_STEREO3D_ROW_INTERLEAVED) {
		GetRenderersSettings().iStereo3DTransform = STEREO3D_HalfOverUnder_to_Interlace;
	} else {
		GetRenderersSettings().iStereo3DTransform = STEREO3D_AsIs;
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_STEREO3D_SWAPLEFTRIGHT, bStereo3DSwapLR);

	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_COLOR_BRIGHTNESS, iBrightness, -100, 100);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_COLOR_CONTRAST, iContrast, -100, 100);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_COLOR_HUE, iHue, -180, 180);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_COLOR_SATURATION, iSaturation, -100, 100);

	{ // load shader list
		int curPos;
		CString token;

		if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_SHADERLIST, str)) {
			curPos = 0;
			token = str.Tokenize(L"|", curPos);
			while (token.GetLength()) {
				ShaderList.push_back(token);
				token = str.Tokenize(L"|", curPos);
			}
		}

		if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_SHADERLISTSCREENSPACE, str)) {
			curPos = 0;
			token = str.Tokenize(L"|", curPos);
			while (token.GetLength()) {
				ShaderListScreenSpace.push_back(token);
				token = str.Tokenize(L"|", curPos);
			}
		}

		if (profile.ReadString(IDS_R_SETTINGS, IDS_RS_SHADERS11POSTSCALE, str)) {
			curPos = 0;
			token = str.Tokenize(L"|", curPos);
			while (token.GetLength()) {
				Shaders11PostScale.push_back(token);
				token = str.Tokenize(L"|", curPos);
			}
		}
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_TOGGLESHADER, bToggleShader);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_TOGGLESHADERSSCREENSPACE, bToggleShaderScreenSpace);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_SHOWOSD, ShowOSD.value);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_FASTSEEK_KEYFRAME, fFastSeek);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_HIDE_WINDOWED_MOUSE_POINTER, bHideWindowedMousePointer);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_MIN_MPLS_DURATION, nMinMPlsDuration, 0, 20);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_MINI_DUMP, fMiniDump);
	CMiniDump::SetState(fMiniDump);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_LCD_SUPPORT, fLCDSupport);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_SMARTSEEK, fSmartSeek);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_SMARTSEEK_SIZE, iSmartSeekSize, 10, 30);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_CHAPTER_MARKER, fChapterMarker);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USE_FLYBAR, fFlybar);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_PLAYLISTFONTPERCENT, iPlsFontPercent, 100, 200);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_USE_FLYBAR_ONTOP, fFlybarOnTop);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_OSD_FONTSHADOW, fFontShadow);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_OSD_FONTAA, fFontAA);

	// Save analog capture settings
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_DEFAULT_CAPTURE, iDefaultCaptureDevice);
	profile.ReadString(IDS_R_CAPTURE, IDS_RS_VIDEO_DISP_NAME, strAnalogVideo);
	profile.ReadString(IDS_R_CAPTURE, IDS_RS_AUDIO_DISP_NAME, strAnalogAudio);
	profile.ReadInt(IDS_R_CAPTURE, IDS_RS_COUNTRY, iAnalogCountry);

	profile.ReadString(IDS_R_DVB, IDS_RS_BDA_NETWORKPROVIDER, strBDANetworkProvider);
	profile.ReadString(IDS_R_DVB, IDS_RS_BDA_TUNER, strBDATuner);
	profile.ReadString(IDS_R_DVB, IDS_RS_BDA_RECEIVER, strBDAReceiver);
	//profile.ReadString(IDS_R_DVB, IDS_RS_BDA_STANDARD, sBDAStandard);
	profile.ReadInt(IDS_R_DVB, IDS_RS_BDA_SCAN_FREQ_START, iBDAScanFreqStart);
	profile.ReadInt(IDS_R_DVB, IDS_RS_BDA_SCAN_FREQ_END, iBDAScanFreqEnd);
	profile.ReadInt(IDS_R_DVB, IDS_RS_BDA_BANDWIDTH, iBDABandwidth);
	profile.ReadBool(IDS_R_DVB, IDS_RS_BDA_USE_OFFSET, fBDAUseOffset);
	profile.ReadInt(IDS_R_DVB, IDS_RS_BDA_OFFSET, iBDAOffset);
	profile.ReadInt(IDS_R_DVB, IDS_RS_DVB_LAST_CHANNEL, nDVBLastChannel);
	profile.ReadBool(IDS_R_DVB, IDS_RS_BDA_IGNORE_ENCRYPTEDCHANNELS, fBDAIgnoreEncryptedChannels);

	m_DVBChannels.clear();
	for (int iChannel = 0; ; iChannel++) {
		CString strTemp;
		CString strChannel;
		CDVBChannel Channel;
		strTemp.Format(L"%d", iChannel);
		profile.ReadString(IDS_R_DVB, strTemp, strChannel);
		if (strChannel.IsEmpty()) {
			break;
		}
		Channel.FromString(strChannel);
		m_DVBChannels.push_back(Channel);
	}

	// playback positions for last played DVDs
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_DVDPOS, bRememberDVDPos);
	nCurrentDvdPosition = -1;
	memset(DvdPosition, 0, sizeof(DvdPosition));
	for (int i = 0; i < std::min(iRecentFilesNumber, MAX_DVD_POSITION); i++) {
		CString lpKeyName;
		CString lpString;

		lpKeyName.Format(L"DVD%d", i + 1);
		profile.ReadString(IDS_R_RECENTFILES, lpKeyName, lpString);
		if (lpString.GetLength() / 2 == sizeof(DVD_POSITION)) {
			DeserializeHex(lpString, (BYTE*)&DvdPosition[i], sizeof(DVD_POSITION));
		}
	}

	// playback positions for last played files
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_FILEPOS, bRememberFilePos);
	nCurrentFilePosition = -1;
	for (int i = 0; i < std::min(iRecentFilesNumber, MAX_FILE_POSITION); i++) {
		CString lpKeyName;
		CString lpString;

		lpKeyName.Format(L"File%d", i + 1);
		profile.ReadString(IDS_R_RECENTFILES, lpKeyName, lpString);

		std::list<CString> args;
		ExplodeEsc(lpString, args, L'|');
		auto it = args.begin();

		if (it != args.end()) {
			FilePosition[i].strFile = *it++;

			if (it != args.end()) {
				FilePosition[i].llPosition = _wtoi64(*it++);

				if (it != args.end()) {
					swscanf_s(*it++, L"%d", &FilePosition[i].nAudioTrack);

					if (it != args.end()) {
						swscanf_s(*it++, L"%d", &FilePosition[i].nSubtitleTrack);
					}
				}
			}
		}
	}

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_DVD_START_MAIN_TITLE, bStartMainTitle);
	bNormalStartDVD			= true;

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_REMAINING_TIME, fRemainingTime);

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_LAST_OPEN_FILTER_DIR, strLastOpenFilterDir);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_PAGEPARSER, bYoutubePageParser);
	str.Empty();
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_YOUTUBE_FORMAT, str);
	YoutubeFormat.fmt = (str == L"WEBM") ? 1 : 0;
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_YOUTUBE_RESOLUTION, YoutubeFormat.res);
	YoutubeFormat.res = discard(YoutubeFormat.res, 720, s_CommonVideoHeights);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_60FPS, YoutubeFormat.fps60);
	if (YoutubeFormat.fps60) {
		profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_HDR, YoutubeFormat.hdr);
	} else {
		YoutubeFormat.hdr = false;
	}
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_LOAD_PLAYLIST, bYoutubeLoadPlaylist);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YDL_ENABLE, bYDLEnable);
	profile.ReadInt(IDS_R_SETTINGS, IDS_RS_YDL_MAXHEIGHT, iYDLMaxHeight);
	iYDLMaxHeight = discard(iYDLMaxHeight, 720, s_CommonVideoHeights);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_YDL_MAXIMUM_QUALITY, bYDLMaximumQuality);

	profile.ReadString(IDS_R_SETTINGS, IDS_RS_ACESTREAM_ADDRESS, strAceStreamAddress);
	profile.ReadString(IDS_R_SETTINGS, IDS_RS_TORRSERVER_ADDRESS, strTorrServerAddress);

	profile.ReadUInt(IDS_R_SETTINGS, IDS_RS_LASTFILEINFOPAGE, *(unsigned*)&nLastFileInfoPage);

	profile.ReadBool(IDS_R_UPDATER, IDS_RS_UPDATER_AUTO_CHECK, bUpdaterAutoCheck);
	profile.ReadInt(IDS_R_UPDATER, IDS_RS_UPDATER_DELAY, nUpdaterDelay, 1, 365);
	profile.ReadInt64(IDS_R_UPDATER, IDS_RS_UPDATER_LAST_CHECK, tUpdaterLastCheck);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_OSD_REMAINING_TIME, bOSDRemainingTime);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_OSD_LOCAL_TIME, bOSDLocalTime);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_OSD_FILE_NAME, bOSDFileName);

	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_PASTECLIPBOARDURL, bPasteClipboardURL);

	std::vector<CStringW> valuenames;
	profile.EnumValueNames(IDS_R_YOUTUBECACHE, valuenames);
	for (const auto& name : valuenames) {
		CString value;
		profile.ReadString(IDS_R_YOUTUBECACHE, name, value);
		youtubeSignatureCache[name] = value;
	}

	if (fLaunchfullscreen && !IsD3DFullscreen()) {
		nCLSwitches |= CLSW_FULLSCREEN;
	}

	LoadFormats(false);

	bInitialized = true;
}

void CAppSettings::SaveSettings()
{
	CMPlayerCApp* pApp = AfxGetMyApp();
	ASSERT(pApp);
	CProfile& profile = AfxGetProfile();

	if (!bInitialized) {
		return;
	}

	CString str;

	FiltersPrioritySettings.SaveSettings();

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_HIDECAPTIONMENU, iCaptionMenuMode);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_HIDENAVIGATION, fHideNavigation);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_CONTROLSTATE, nCS);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_DEFAULTVIDEOFRAME, iDefaultVideoSize);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_NOSMALLUPSCALE, bNoSmallUpscale);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_NOSMALLDOWNSCALE, bNoSmallDownscale);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_KEEPASPECTRATIO, fKeepAspectRatio);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_COMPMONDESKARDIFF, fCompMonDeskARDiff);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_VOLUME, nVolume);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_BALANCE, nBalance);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MUTE, fMute);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_LOOPNUM, nLoops);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_LOOP, fLoopForever);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REWIND, fRewind);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_VOLUME_STEP, nVolumeStep);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SPEED_STEP, nSpeedStep);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SPEED_NOTRESET, bSpeedNotReset);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_MULTIINST, iMultipleInst);
	if (CPPageFormats::ShellExtExists()) {
		CRegKey key;
		if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, shellExtKeyName)) {
			key.SetDWORDValue(IDS_RS_MULTIINST, iMultipleInst);
		}
	}

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_TITLEBARTEXT, iTitleBarTextStyle);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SEEKBARTEXT, iSeekBarTextStyle);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_ONTOP, iOnTop);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_TRAYICON, bTrayIcon);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_FULLSCREENCTRLS, fShowBarsWhenFullScreen);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_FULLSCREENCTRLSTIMEOUT, nShowBarsWhenFullScreenTimeOut);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_EXITFULLSCREENATTHEEND, fExitFullScreenAtTheEnd);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_EXITFULLSCREENATFOCUSLOST, fExitFullScreenAtFocusLost);

	if (!fullScreenModes.res.empty()) {
		profile.DeleteSection(IDS_RS_FULLSCREENRES);
		profile.WriteInt(IDS_RS_FULLSCREENRES, IDS_RS_FULLSCREENRES_ENABLE, fullScreenModes.bEnabled);
		profile.WriteBool(IDS_RS_FULLSCREENRES, IDS_RS_FULLSCREENRES_APPLY_DEF, fullScreenModes.bApplyDefault);
		size_t cnt = 0;
		std::vector<BYTE> value;
		for (const auto& item : fullScreenModes.res) {
			if (!item.monitorId.IsEmpty()) {
				CString entry;
				entry.Format(L"MonitorId%u", cnt);
				profile.WriteString(IDS_RS_FULLSCREENRES, entry, item.monitorId);

				entry.Format(L"Res%u", cnt++);
				value.resize(1 + item.dmFullscreenRes.size() * sizeof(fpsmode));
				value[0] = (BYTE)item.dmFullscreenRes.size();
				memcpy(&value[1], item.dmFullscreenRes.data(), item.dmFullscreenRes.size() * sizeof(fpsmode));
				profile.WriteBinary(IDS_RS_FULLSCREENRES, entry, value.data(), value.size());
			}
		}
	}

	profile.WriteBinary(IDS_R_SETTINGS, L"AccelTblColWidth", (BYTE*)&AccelTblColWidth, sizeof(AccelTblColWidth));
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_DISPLAYMODECHANGEDELAY, iDMChangeDelay);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_RESTORERESAFTEREXIT, fRestoreResAfterExit);

	if (bSavePnSZoom) {
		str.Format(L"%.3f,%.3f", dZoomX, dZoomY);
		profile.WriteString(IDS_R_SETTINGS, IDS_RS_PANSCANZOOM, str);
	} else {
		profile.DeleteValue(IDS_R_SETTINGS, IDS_RS_PANSCANZOOM);
	}

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_ASPECTRATIO_X, sizeAspectRatio.cx);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_ASPECTRATIO_Y, sizeAspectRatio.cy);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_KEEPHISTORY, bKeepHistory);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_RECENT_FILES_NUMBER, iRecentFilesNumber);

	// Window size
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_WINDOWMODESTARTUP, nStartupWindowMode);
	str.Format(L"%d;%d", szSpecifiedWndSize.cx, szSpecifiedWndSize.cy);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SPECIFIEDWINDOWSIZE, str);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REMEMBERWINDOWPOS, bRememberWindowPos);
	CRect rect(ptLastWindowPos, szLastWindowSize);
	profile.WriteBinary(IDS_R_SETTINGS, IDS_RS_LASTWINDOWRECT, (BYTE*)&rect, sizeof(rect));
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_LASTWINDOWTYPE, (nLastWindowType == SIZE_MINIMIZED) ? SIZE_RESTORED : nLastWindowType);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_LIMITWINDOWPROPORTIONS, bLimitWindowProportions);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SNAPTODESKTOPEDGES, bSnapToDesktopEdges);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_WINDOWMODEPLAYBACK, nPlaybackWindowMode);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUTOSCALEFACTOR, nAutoScaleFactor);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUTOFITFACTOR, nAutoFitFactor);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_RESETWINDOWAFTERCLOSINGFILE, bResetWindowAfterClosingFile);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SHUFFLEPLAYLISTITEMS, bShufflePlaylistItems);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REMEMBERPLAYLISTITEMS, bRememberPlaylistItems);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_HIDEPLAYLISTFULLSCREEN, bHidePlaylistFullScreen);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SHOWPLAYLISTTOOLTIP, bShowPlaylistTooltip);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SHOWPLAYLISTSEARCHBAR, bShowPlaylistSearchBar);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PLAYLISTNEXTONERROR, bPlaylistNextOnError);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PLAYLISTDETERMINEDURATION, bPlaylistDetermineDuration);

	profile.WriteBool(IDS_R_FAVORITES, IDS_RS_FAV_REMEMBERPOS, bFavRememberPos);
	profile.WriteBool(IDS_R_FAVORITES, IDS_RS_FAV_RELATIVEDRIVE, bFavRelativeDrive);

	m_VRSettings.Save();

	SavePlaylistTabSetting();

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_AUDIORENDERERTYPE, strAudioRendererDisplayName);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SECONDAUDIORENDERER, strSecondAudioRendererDisplayName);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_DUALAUDIOOUTPUT, fDualAudioOutput);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUTOLOADAUDIO, fAutoloadAudio);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PRIORITIZEEXTERNALAUDIO, fPrioritizeExternalAudio);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_AUDIOPATHS, strAudioPaths);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SUBTITLERENDERER, iSubtitleRenderer);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SUBTITLESLANGORDER, CString(strSubtitlesLanguageOrder));
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_AUDIOSLANGORDER, CString(strAudiosLanguageOrder));
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_INTERNALSELECTTRACKLOGIC, fUseInternalSelectTrackLogic);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REMEMBERSELECTEDTRACKS, bRememberSelectedTracks);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUDIOWINDOWMODE, nAudioWindowMode);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_ADDSIMILARFILES, nAddSimilarFiles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_ENABLEWORKERTHREADFOROPENING, fEnableWorkerThreadForOpening);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REPORTFAILEDPINS, fReportFailedPins);

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_DVDPATH, strDVDPath);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USEDVDPATH, bUseDVDPath);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_MENULANG, idMenuLang);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_AUDIOLANG, idAudioLang);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_SUBTITLESLANG, idSubtitlesLang);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_CLOSEDCAPTIONS, bClosedCaptions);

	CString style;
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SPSTYLE, style <<= subdefstyle);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SPOVERRIDEPLACEMENT, fOverridePlacement);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SPHORPOS, nHorPos);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SPVERPOS, nVerPos);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SUBDELAYINTERVAL, nSubDelayInterval);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_ENABLESUBTITLES, fEnableSubtitles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_FORCEDSUBTITLES, fForcedSubtitles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PRIORITIZEEXTERNALSUBTITLES, fPrioritizeExternalSubtitles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_DISABLEINTERNALSUBTITLES, fDisableInternalSubtitles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUTORELOADEXTSUBTITLES, fAutoReloadExtSubtitles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USE_SUBRESYNC, fUseSybresync);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SUBTITLEPATHS, strSubtitlePaths);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USEDEFAULTSUBTITLESSTYLE, fUseDefaultSubtitlesStyle);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUDIOMIXER, bAudioMixer);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_AUDIOMIXERLAYOUT, channel_mode_sets[nAudioMixerLayout]);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUDIOSTEREOFROMDECODER, bAudioStereoFromDecoder);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUDIOBASSREDIRECT, bAudioBassRedirect);
	profile.WriteDouble(IDS_R_SETTINGS, IDS_RS_AUDIOLEVELCENTER, dAudioCenter_dB);
	profile.WriteDouble(IDS_R_SETTINGS, IDS_RS_AUDIOLEVELSURROUND, dAudioSurround_dB);
	profile.WriteDouble(IDS_R_SETTINGS, IDS_RS_AUDIOGAIN, dAudioGain_dB);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUDIOAUTOVOLUMECONTROL, bAudioAutoVolumeControl);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_AUDIONORMBOOST, bAudioNormBoost);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUDIONORMLEVEL, iAudioNormLevel);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUDIONORMREALEASETIME, iAudioNormRealeaseTime);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUDIOSAMPLEFORMATS, iAudioSampleFormats);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_ENABLEAUDIOTIMESHIFT, bAudioTimeShift);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_AUDIOTIMESHIFT, iAudioTimeShift);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_BUFFERDURATION, iBufferDuration);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_NETWORKTIMEOUT, iNetworkTimeout);

	// Multi-monitor code
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_FULLSCREENMONITOR, CString(strFullScreenMonitor));
	// DeviceID
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_FULLSCREENMONITORID, CString(strFullScreenMonitorID));

	// Prevent Minimize when in Fullscreen mode on non default monitor
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_PREVENT_MINIMIZE, fPreventMinimize);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PAUSEMINIMIZEDVIDEO, bPauseMinimizedVideo);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_WIN7TASKBAR, fUseWin7TaskBar);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_EXIT_AFTER_PB, fExitAfterPlayback);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_CLOSE_FILE_AFTER_PB, bCloseFileAfterPlayback);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_NEXT_AFTER_PB, fNextInDirAfterPlayback);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_NEXT_AFTER_PB_LOOPED, fNextInDirAfterPlaybackLooped);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MPC_NO_SEARCH_IN_FOLDER, fDontUseSearchInFolder);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USE_TIME_TOOLTIP, fUseTimeTooltip);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_TIME_TOOLTIP_POSITION, nTimeTooltipPosition);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_OSD_SIZE, nOSDSize);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_OSD_FONT, strOSDFont);

	// Last Open Dir
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_LAST_OPEN_DIR, strLastOpenDir);
	// Last Saved Playlist Dir
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_LAST_SAVED_PLAYLIST_DIR, strLastSavedPlaylistDir);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_D3DFULLSCREEN, fD3DFullscreen);
	//profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MONITOR_AUTOREFRESHRATE, fMonitorAutoRefreshRate);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_STEREO3D_MODE, iStereo3DMode);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_STEREO3D_SWAPLEFTRIGHT, bStereo3DSwapLR);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_COLOR_BRIGHTNESS, iBrightness);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_COLOR_CONTRAST, iContrast);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_COLOR_HUE, iHue);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_COLOR_SATURATION, iSaturation);

	{ // save shader list
		str.Empty();
		for (const auto& shader : ShaderList) {
			str += shader + "|";
		}
		profile.WriteString(IDS_R_SETTINGS, IDS_RS_SHADERLIST, str);

		str.Empty();
		for (const auto& shader : ShaderListScreenSpace) {
			str += shader + "|";
		}
		profile.WriteString(IDS_R_SETTINGS, IDS_RS_SHADERLISTSCREENSPACE, str);

		str.Empty();
		for (const auto& shader : Shaders11PostScale) {
			str += shader + "|";
		}
		profile.WriteString(IDS_R_SETTINGS, IDS_RS_SHADERS11POSTSCALE, str);
	}
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_TOGGLESHADER, bToggleShader);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_TOGGLESHADERSSCREENSPACE, bToggleShaderScreenSpace);

	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_SHOWOSD, ShowOSD.value);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_LANGUAGE, CMPlayerCApp::languageResources[iLanguage].strcode);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_FASTSEEK_KEYFRAME, fFastSeek);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_HIDE_WINDOWED_MOUSE_POINTER, bHideWindowedMousePointer);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_MIN_MPLS_DURATION, nMinMPlsDuration);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_MINI_DUMP, fMiniDump);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_LCD_SUPPORT, fLCDSupport);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SMARTSEEK, fSmartSeek);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_SMARTSEEK_SIZE, iSmartSeekSize);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_CHAPTER_MARKER, fChapterMarker);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USE_FLYBAR, fFlybar);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_PLAYLISTFONTPERCENT, iPlsFontPercent);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USE_FLYBAR_ONTOP, fFlybarOnTop);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_OSD_FONTSHADOW, fFontShadow);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_OSD_FONTAA, fFontAA);

	// Save analog capture settings
	profile.WriteInt   (IDS_R_SETTINGS, IDS_RS_DEFAULT_CAPTURE, iDefaultCaptureDevice);
	profile.WriteString(IDS_R_CAPTURE, IDS_RS_VIDEO_DISP_NAME, strAnalogVideo);
	profile.WriteString(IDS_R_CAPTURE, IDS_RS_AUDIO_DISP_NAME, strAnalogAudio);
	profile.WriteInt   (IDS_R_CAPTURE, IDS_RS_COUNTRY,		 iAnalogCountry);

	// Save digital capture settings (BDA)
	profile.WriteString(IDS_R_DVB, IDS_RS_BDA_NETWORKPROVIDER, strBDANetworkProvider);
	profile.WriteString(IDS_R_DVB, IDS_RS_BDA_TUNER, strBDATuner);
	profile.WriteString(IDS_R_DVB, IDS_RS_BDA_RECEIVER, strBDAReceiver);
	//profile.WriteString(IDS_R_DVB, IDS_RS_BDA_STANDARD, strBDAStandard);
	profile.WriteInt(IDS_R_DVB, IDS_RS_BDA_SCAN_FREQ_START, iBDAScanFreqStart);
	profile.WriteInt(IDS_R_DVB, IDS_RS_BDA_SCAN_FREQ_END, iBDAScanFreqEnd);
	profile.WriteInt(IDS_R_DVB, IDS_RS_BDA_BANDWIDTH, iBDABandwidth);
	profile.WriteBool(IDS_R_DVB, IDS_RS_BDA_USE_OFFSET, fBDAUseOffset);
	profile.WriteInt(IDS_R_DVB, IDS_RS_BDA_OFFSET, iBDAOffset);
	profile.WriteBool(IDS_R_DVB, IDS_RS_BDA_IGNORE_ENCRYPTEDCHANNELS, fBDAIgnoreEncryptedChannels);
	profile.WriteInt(IDS_R_DVB, IDS_RS_DVB_LAST_CHANNEL, nDVBLastChannel);

	int iChannel = 0;
	for (auto& Channel : m_DVBChannels) {
		CString strTemp;
		strTemp.Format(L"%d", iChannel);
		profile.WriteString(IDS_R_DVB, strTemp, Channel.ToString());
		iChannel++;
	}

	// playback positions for last played DVDs
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_DVDPOS, bRememberDVDPos);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_FILEPOS, bRememberFilePos);
	if (bKeepHistory) {
		if (bRememberDVDPos) {
			for (int i = 0; i < std::min(iRecentFilesNumber, MAX_DVD_POSITION); i++) {
				CString strDVDPos;
				CString strValue;

				strDVDPos.Format(L"DVD%d", i + 1);
				strValue = SerializeHex((BYTE*)&DvdPosition[i], sizeof(DVD_POSITION));
				profile.WriteString(IDS_R_RECENTFILES, strDVDPos, strValue);
			}
		}

		if (bRememberFilePos) {
			for (int i = 0; i < std::min(iRecentFilesNumber, MAX_FILE_POSITION); i++) {
				CString lpKeyName;
				CString lpString;

				lpKeyName.Format(L"File%d", i + 1);
				lpString.Format(L"%s|%I64d|%d|%d", FilePosition[i].strFile, FilePosition[i].llPosition, FilePosition[i].nAudioTrack, FilePosition[i].nSubtitleTrack);
				profile.WriteString(IDS_R_RECENTFILES, lpKeyName, lpString);
			}
		}
	}

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_DVD_START_MAIN_TITLE, bStartMainTitle);

	profile.DeleteSection(IDS_R_PNSPRESETS);
	for (int i = 0, j = m_pnspresets.GetCount(); i < j; i++) {
		str.Format(L"Preset%d", i);
		profile.WriteString(IDS_R_PNSPRESETS, str, m_pnspresets[i]);
	}

	// Keys

	profile.DeleteSection(IDS_R_COMMANDS);
	unsigned n = 0;
	for (const auto& wc : wmcmds) {
		if (wc.IsModified()) {
			str.Format(L"CommandMod%u", n);
			CString str2;
			str2.Format(L"%d %x %x \"%S\" %d %u %u %u",
						wc.cmd, wc.fVirt, wc.key,
						wc.rmcmd, wc.rmrepcnt,
						wc.mouse, wc.appcmd, wc.mouseFS);
			profile.WriteString(IDS_R_COMMANDS, str, str2);
			n++;
		}
	}

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_WINLIRC, bWinLirc);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_WINLIRCADDR, strWinLircAddr);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_UICE, bUIce);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_UICEADDR, strUIceAddr);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_GLOBALMEDIA, bGlobalMedia);

	// Mouse
	str.Format(L"%u", nMouseLeftClick);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT, str);
	str.Format(L"%u", nMouseLeftDblClick);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT_DBLCLICK, str);
	str.Format(L"%u", nMouseRightClick);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_RIGHT, str);
	str.Format(L"%u;%u;%u;%u", MouseMiddleClick.normal, MouseMiddleClick.ctrl, MouseMiddleClick.shift, MouseMiddleClick.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_MIDDLE, str);
	str.Format(L"%u;%u;%u;%u", MouseX1Click.normal, MouseX1Click.ctrl, MouseX1Click.shift, MouseX1Click.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_X1, str);
	str.Format(L"%u;%u;%u;%u", MouseX2Click.normal, MouseX2Click.ctrl, MouseX2Click.shift, MouseX2Click.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_X2, str);
	str.Format(L"%u;%u;%u;%u", MouseWheelUp.normal, MouseWheelUp.ctrl, MouseWheelUp.shift, MouseWheelUp.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_UP, str);
	str.Format(L"%u;%u;%u;%u", MouseWheelDown.normal, MouseWheelDown.ctrl, MouseWheelDown.shift, MouseWheelDown.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_DOWN, str);
	str.Format(L"%u;%u;%u;%u", MouseWheelLeft.normal, MouseWheelLeft.ctrl, MouseWheelLeft.shift, MouseWheelLeft.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_LEFT, str);
	str.Format(L"%u;%u;%u;%u", MouseWheelRight.normal, MouseWheelRight.ctrl, MouseWheelRight.shift, MouseWheelRight.rbtn);
	profile.WriteString(IDS_R_MOUSE, IDS_RS_MOUSE_WHEEL_RIGHT, str);
	profile.WriteBool(IDS_R_MOUSE, IDS_RS_MOUSE_BTN_LEFT_OPENRECENT, bMouseLeftClickOpenRecent);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_USEDARKTHEME, bUseDarkTheme);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THEMEBRIGHTNESS, nThemeBrightness);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THEMERED, nThemeRed);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THEMEGREEN, nThemeGreen);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THEMEBLUE, nThemeBlue);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_DARKMENU, bDarkMenu);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_OSD_TRANSPARENT, nOSDTransparent);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_OSD_BORDER, nOSDBorder);

	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_CLRFACEABGR, clrFaceABGR);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_CLROUTLINEABGR, clrOutlineABGR);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_OSD_FONTCOLOR, clrFontABGR);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_OSD_GRAD1COLOR, clrGrad1ABGR);
	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_OSD_GRAD2COLOR, clrGrad2ABGR);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTS, nJumpDistS);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTM, nJumpDistM);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_JUMPDISTL, nJumpDistL);

	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_LASTUSEDPAGE, nLastUsedPage);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_DLGPROPX, iDlgPropX);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_DLGPROPY, iDlgPropY);

	// Internal filters
	for (int f = 0; f < SRC_COUNT; f++) {
		profile.WriteBool(IDS_R_INTERNAL_FILTERS, SrcFiltersKeys[f], SrcFilters[f]);
	}
	for (int f = 0; f < VDEC_DXVA_COUNT; f++) {
		profile.WriteBool(IDS_R_INTERNAL_FILTERS, DXVAFiltersKeys[f], DXVAFilters[f]);
	}
	for (int f = 0; f < VDEC_COUNT; f++) {
		profile.WriteBool(IDS_R_INTERNAL_FILTERS, VideoFiltersKeys[f], VideoFilters[f]);
	}
	for (int f = 0; f < ADEC_COUNT; f++) {
		profile.WriteBool(IDS_R_INTERNAL_FILTERS, AudioFiltersKeys[f], AudioFilters[f]);
	}

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_LOGOFILE, strLogoFileName);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_LOGOID, nLogoId);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_LOGOEXT, bLogoExternal);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_HIDECDROMSSUBMENU, bHideCDROMsSubMenu);

	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_PRIORITY, dwPriority);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_LAUNCHFULLSCREEN, fLaunchfullscreen);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_ENABLEWEBSERVER, fEnableWebServer);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_WEBSERVERPORT, nWebServerPort);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_WEBSERVERQUALITY, nWebServerQuality);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERPRINTDEBUGINFO, fWebServerPrintDebugInfo);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERUSECOMPRESSION, fWebServerUseCompression);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_WEBSERVERLOCALHOSTONLY, fWebServerLocalhostOnly);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_WEBUI_ENABLE_PREVIEW, bWebUIEnablePreview);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_WEBROOT, strWebRoot);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_WEBDEFINDEX, strWebDefIndex);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_WEBSERVERCGI, strWebServerCGI);

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SNAPSHOTPATH, strSnapShotPath);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_SNAPSHOTEXT, strSnapShotExt);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SNAPSHOT_SUBTITLES, bSnapShotSubtitles);

	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THUMBROWS, iThumbRows);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THUMBCOLS, iThumbCols);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THUMBWIDTH, iThumbWidth);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THUMBQUALITY, iThumbQuality);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_THUMBLEVELPNG, iThumbLevelPNG);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_SUBSAVEEXTERNALSTYLEFILE, bSubSaveExternalStyleFile);

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_ISDB, strISDb);

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_LAST_OPEN_FILTER_DIR, strLastOpenFilterDir);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_PAGEPARSER, bYoutubePageParser);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_YOUTUBE_FORMAT, YoutubeFormat.fmt == 1 ? L"WEBM" : L"MP4");
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_YOUTUBE_RESOLUTION, YoutubeFormat.res);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_60FPS, YoutubeFormat.fps60);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_HDR, YoutubeFormat.hdr);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YOUTUBE_LOAD_PLAYLIST, bYoutubeLoadPlaylist);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YDL_ENABLE, bYDLEnable);
	profile.WriteInt(IDS_R_SETTINGS, IDS_RS_YDL_MAXHEIGHT, iYDLMaxHeight);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_YDL_MAXIMUM_QUALITY, bYDLMaximumQuality);

	profile.WriteString(IDS_R_SETTINGS, IDS_RS_ACESTREAM_ADDRESS, strAceStreamAddress);
	profile.WriteString(IDS_R_SETTINGS, IDS_RS_TORRSERVER_ADDRESS, strTorrServerAddress);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_REMAINING_TIME, fRemainingTime);

	profile.WriteUInt(IDS_R_SETTINGS, IDS_RS_LASTFILEINFOPAGE, nLastFileInfoPage);

	profile.WriteBool(IDS_R_UPDATER, IDS_RS_UPDATER_AUTO_CHECK, bUpdaterAutoCheck);
	profile.WriteInt(IDS_R_UPDATER, IDS_RS_UPDATER_DELAY, nUpdaterDelay);
	profile.WriteInt64(IDS_R_UPDATER, IDS_RS_UPDATER_LAST_CHECK, tUpdaterLastCheck);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_OSD_REMAINING_TIME, bOSDRemainingTime);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_OSD_LOCAL_TIME, bOSDLocalTime);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_OSD_FILE_NAME, bOSDFileName);

	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_PASTECLIPBOARDURL, bPasteClipboardURL);

	for (const auto& [name, value] : youtubeSignatureCache) {
		profile.WriteString(IDS_R_YOUTUBECACHE, name, value);
	}

	SaveFormats();

	if (pApp->m_pszRegistryKey) {
		// WINBUG: on win2k this would crash WritePrivateProfileString
		int v = 0;
		profile.ReadInt(L"", L"", v);
		profile.WriteInt(L"", L"", v ? 0 : 1);
	}

	profile.Flush(true);
}

void CAppSettings::SaveExternalFilters()
{
	// External Filter settings are saved for a long time. Use only when really necessary.
	CProfile& profile = AfxGetProfile();

	if (!bInitialized) {
		return;
	}

	for (unsigned int i = 0; ; i++) {
		CString key;
		key.Format(L"%s\\%04u", IDS_R_FILTERS, i);
		int j = -1;
		profile.ReadInt(key, L"Enabled", j);
		profile.DeleteSection(key);
		if (j < 0) {
			break;
		}
	}

	unsigned int k = 0;
	POSITION pos = m_filters.GetHeadPosition();
	while (pos) {
		FilterOverride* f = m_filters.GetNext(pos);

		if (f->fTemporary) {
			continue;
		}

		CString key;
		key.Format(L"%s\\%04u", IDS_R_FILTERS, k);

		profile.WriteInt(key, L"SourceType", (int)f->type);
		profile.WriteInt(key, L"Enabled", (int)!f->fDisabled);
		if (f->type == FilterOverride::REGISTERED) {
			profile.WriteString(key, L"DisplayName", CString(f->dispname));
			profile.WriteString(key, L"Name", f->name);
			if (f->clsid == CLSID_NULL) {
				CComPtr<IBaseFilter> pBF;
				CStringW FriendlyName;
				if (CreateFilter(f->dispname, &pBF, FriendlyName)) {
					f->clsid = GetCLSID(pBF);
				}
			}
			profile.WriteString(key, L"CLSID", CStringFromGUID(f->clsid));
		} else if (f->type == FilterOverride::EXTERNAL) {
			profile.WriteString(key, L"Path", f->path);
			profile.WriteString(key, L"Name", f->name);
			profile.WriteString(key, L"CLSID", CStringFromGUID(f->clsid));
		}
		unsigned i = 0;
		for (const auto& item : f->backup) {
			CString val;
			val.Format(L"org%04u", i++);
			profile.WriteString(key, val, CStringFromGUID(item));
		}
		i = 0;
		for (const auto& item : f->guids) {
			CString val;
			val.Format(L"mod%04u", i++);
			profile.WriteString(key, val, CStringFromGUID(item));
		}
		profile.WriteInt(key, L"LoadType", f->iLoadType);
		profile.WriteUInt(key, L"Merit", f->dwMerit);

		k++;
	}
}

int CAppSettings::GetMultiInst()
{
	int multiinst = 1;
	AfxGetProfile().ReadInt(IDS_R_SETTINGS, IDS_RS_MULTIINST, multiinst, 0, 2);

	return multiinst;
}

engine_t CAppSettings::GetFileEngine(CString path)
{
	CString ext = CPath(path).GetExtension().MakeLower();

	if (!ext.IsEmpty()) {
		for (auto& mfc : m_Formats) {
			if (mfc.GetLabel() == L"swf") {
				if (mfc.FindExt(ext)) {
					return ShockWave;
				}
				break;
			}
		}
	}

	return DirectShow;
}

void CAppSettings::SaveCurrentFilePosition()
{
	CProfile& profile = AfxGetProfile();
	int i = nCurrentFilePosition;

	CString lpKeyName;
	CString lpString;

	lpKeyName.Format(L"File%d", i + 1);
	lpString.Format(L"%s|%I64d|%d|%d", FilePosition[i].strFile, FilePosition[i].llPosition, FilePosition[i].nAudioTrack, FilePosition[i].nSubtitleTrack);
	profile.WriteString(IDS_R_RECENTFILES, lpKeyName, lpString);
}

void CAppSettings::ClearFilePositions()
{
	CProfile& profile = AfxGetProfile();
	CString lpKeyName;

	for (int i = 0; i < MAX_FILE_POSITION; i++) {
		FilePosition[i].strFile.Empty();
		FilePosition[i].llPosition     = 0;
		FilePosition[i].nAudioTrack    = -1;
		FilePosition[i].nSubtitleTrack = -1;

		lpKeyName.Format(L"File%d", i + 1);
		profile.WriteString(IDS_R_RECENTFILES, lpKeyName, L"");
	}
}

void CAppSettings::SaveCurrentDVDPosition()
{
	CProfile& profile = AfxGetProfile();
	int i = nCurrentDvdPosition;

	CString lpKeyName;
	CString lpString;

	lpKeyName.Format(L"DVD%d", i + 1);
	lpString = SerializeHex((BYTE*)&DvdPosition[i], sizeof(DVD_POSITION));
	profile.WriteString(IDS_R_RECENTFILES, lpKeyName, lpString);
}

void CAppSettings::ClearDVDPositions()
{
	CProfile& profile = AfxGetProfile();
	CString lpKeyName;

	for (int i = 0; i < MAX_DVD_POSITION; i++) {
		DvdPosition[i].llDVDGuid = 0;
		DvdPosition[i].lTitle    = 0;
		memset(&DvdPosition[i].Timecode, 0, sizeof(DVD_HMSF_TIMECODE));

		lpKeyName.Format(L"DVD%d", i + 1);
		profile.WriteString(IDS_R_RECENTFILES, lpKeyName, L"");
	}
}

__int64 CAppSettings::ConvertTimeToMSec(CString& time) const
{
	__int64 Sec = 0;
	__int64 mSec = 0;
	__int64 mult = 1;

	int pos = time.GetLength() - 1;
	if (pos < 3) {
		return 0;
	}

	while (pos >= 0) {
		WCHAR ch = time[pos];
		if (ch == '.') {
			mSec = Sec * 1000 / mult;
			Sec = 0;
			mult = 1;
		} else if (ch == ':') {
			mult = mult * 6 / 10;
		} else if (ch >= '0' && ch <= '9') {
			Sec += (ch - '0') * mult;
			mult *= 10;
		} else {
			mSec = Sec = 0;
			break;
		}
		pos--;
	}
	return Sec*1000 + mSec;
}

void CAppSettings::ExtractDVDStartPos(CString& strParam)
{
	int i = 0, j = 0;
	for (CString token = strParam.Tokenize(L"#", i);
			j < 3 && !token.IsEmpty();
			token = strParam.Tokenize(L"#", i), j++) {
		switch (j) {
			case 0 :
				lDVDTitle = token.IsEmpty() ? 0 : (ULONG)_wtol(token);
				break;
			case 1 :
				if (token.Find(':') > 0) {
					swscanf_s(token, L"%02d:%02d:%02d.%03d", &DVDPosition.bHours, &DVDPosition.bMinutes, &DVDPosition.bSeconds, &DVDPosition.bFrames);
					/* Hack by Ron.  If bFrames >= 30, PlayTime commands fail due to invalid arg */
					DVDPosition.bFrames = 0;
				} else {
					lDVDChapter = token.IsEmpty() ? 0 : (ULONG)_wtol(token);
				}
				break;
		}
	}
}

CString CAppSettings::ParseFileName(const CString& param)
{
	CString fullPathName;

	// Try to transform relative pathname into full pathname
	if (param.Find(L":") < 0) {
		fullPathName.ReleaseBuffer(GetFullPathNameW(param, MAX_PATH, fullPathName.GetBuffer(MAX_PATH), nullptr));

		CFileStatus fs;
		if (!fullPathName.IsEmpty() && CFileGetStatus(fullPathName, fs)) {
			return fullPathName;
		}
	}

	return param;
}

void CAppSettings::ParseCommandLine(cmdLine& cmdln)
{
	nCLSwitches = 0;
	slFilters.clear();
	slFiles.clear();
	slDubs.clear();
	slSubs.clear();
	rtStart = INVALID_TIME;
	rtShift = 0;
	lDVDTitle = 0;
	lDVDChapter = 0;
	memset(&DVDPosition, 0, sizeof(DVDPosition));
	iAdminOption = 0;
	sizeFixedWindow.SetSize(0, 0);
	iMonitor = 0;
	hMasterWnd = 0;
	strPnSPreset.Empty();

	auto it = cmdln.begin();

	while (it != cmdln.end()) {
		CString param = *it++;
		if (param.IsEmpty()) {
			continue;
		}

		if ((param[0] == '-' || param[0] == '/') && param.GetLength() > 1) {
			CString sw = param.Mid(1).MakeLower();
			bool next_available = (it != cmdln.end());

			if (sw == L"open") {
				nCLSwitches |= CLSW_OPEN;
			}
			else if (sw == L"play") {
				nCLSwitches |= CLSW_PLAY;
			}
			else if (sw == L"fullscreen") {
				nCLSwitches |= CLSW_FULLSCREEN;
			}
			else if (sw == L"minimized") {
				nCLSwitches |= CLSW_MINIMIZED;
			}
			else if (sw == L"new") {
				nCLSwitches |= CLSW_NEW;
			}
			else if (sw == L"help" || sw == L"h" || sw == L"?") {
				nCLSwitches |= CLSW_HELP;
			}
			else if (sw == L"dub" && next_available) {
				slDubs.push_back(ParseFileName(*it++));
			}
			else if (sw == L"dubdelay" && next_available) {
				CString strFile = ParseFileName(*it++);
				int nPos = strFile.Find (L"DELAY");
				if (nPos != -1) {
					rtShift = 10000 * _wtol(strFile.Mid(nPos + 6));
				}
				slDubs.push_back(strFile);
			}
			else if (sw == L"sub" && next_available) {
				slSubs.push_back(ParseFileName(*it++));
			}
			else if (sw == L"filter" && next_available) {
				slFilters.push_back(*it++);
			}
			else if (sw == L"dvd") {
				nCLSwitches |= CLSW_DVD;
			}
			else if (sw == L"dvdpos" && next_available) {
				ExtractDVDStartPos(*it++);
			}
			else if (sw == L"cd") {
				nCLSwitches |= CLSW_CD;
			}
			else if (sw == L"add") {
				nCLSwitches |= CLSW_ADD;
			}
			else if (sw == L"regvid") {
				nCLSwitches |= CLSW_REGEXTVID;
			}
			else if (sw == L"regaud") {
				nCLSwitches |= CLSW_REGEXTAUD;
			}
			else if (sw == L"regpl") {
				nCLSwitches |= CLSW_REGEXTPL;
			}
			else if (sw == L"regall") {
				nCLSwitches |= (CLSW_REGEXTVID | CLSW_REGEXTAUD | CLSW_REGEXTPL);
			}
			else if (sw == L"unregall") {
				nCLSwitches |= CLSW_UNREGEXT;
			}
			else if (sw == L"unregvid") {
				nCLSwitches |= CLSW_UNREGEXT;    /* keep for compatibility with old versions */
			}
			else if (sw == L"unregaud") {
				nCLSwitches |= CLSW_UNREGEXT;    /* keep for compatibility with old versions */
			}
			else if (sw == L"start" && next_available) {
				rtStart = 10000i64 * wcstol(*it++, nullptr, 10);
				nCLSwitches |= CLSW_STARTVALID;
			}
			else if (sw == L"startpos" && next_available) {
				rtStart = 10000i64 * ConvertTimeToMSec(*it++);
				nCLSwitches |= CLSW_STARTVALID;
			}
			else if (sw == L"nofocus") {
				nCLSwitches |= CLSW_NOFOCUS;
			}
			else if (sw == L"close") {
				nCLSwitches |= CLSW_CLOSE;
			}
			else if (sw == L"standby") {
				nCLSwitches |= CLSW_STANDBY;
			}
			else if (sw == L"hibernate") {
				nCLSwitches |= CLSW_HIBERNATE;
			}
			else if (sw == L"shutdown") {
				nCLSwitches |= CLSW_SHUTDOWN;
			}
			else if (sw == L"logoff") {
				nCLSwitches |= CLSW_LOGOFF;
			}
			else if (sw == L"lock") {
				nCLSwitches |= CLSW_LOCK;
			}
			else if (sw == L"d3dfs") {
				nCLSwitches |= CLSW_D3DFULLSCREEN;
			}
			else if (sw == L"adminoption" && next_available) {
				nCLSwitches |= CLSW_ADMINOPTION;
				iAdminOption = _wtoi(*it++);
			}
			else if (sw == L"slave" && next_available) {
				nCLSwitches |= CLSW_SLAVE;
				hMasterWnd = (HWND)IntToPtr(_wtoi(*it++));
			}
			else if (sw == L"fixedsize" && next_available) {
				std::list<CString> sl;
				Explode(*it++, sl, L',', 2);
				if (sl.size() == 2) {
					sizeFixedWindow.SetSize(_wtol(sl.front()), _wtol(sl.back()));
					if (sizeFixedWindow.cx > 0 && sizeFixedWindow.cy > 0) {
						nCLSwitches |= CLSW_FIXEDSIZE;
					}
				}
			}
			else if (sw == L"monitor" && next_available) {
				iMonitor = wcstol(*it++, nullptr, 10);
				nCLSwitches |= CLSW_MONITOR;
			}
			else if (sw == L"pns" && next_available) {
				strPnSPreset = *it++;
			}
			else if (sw == L"webport" && next_available) {
				int tmpport = wcstol(*it++, nullptr, 10);
				if ( tmpport >= 0 && tmpport <= 65535 ) {
					nCmdlnWebServerPort = tmpport;
				}
			}
			else if (sw == L"debug") {
				bShowDebugInfo = true;
			}
			else if (sw == L"nominidump") {
				CMiniDump::SetState(false);
			}
			else if (sw == L"audiorenderer" && next_available) {
				SetAudioRenderer(_wtoi(*it++));
			}
			else if (sw == L"reset") {
				nCLSwitches |= CLSW_RESET;
			}
			else if (sw == L"randomize") {
				nCLSwitches |= CLSW_RANDOMIZE;
			}
			else if (sw == L"clipboard") {
				nCLSwitches |= CLSW_CLIPBOARD;
			}
			else {
				nCLSwitches |= CLSW_HELP|CLSW_UNRECOGNIZEDSWITCH;
			}
		}
		else if (param == L"-") { // Special case: standard input
			slFiles.push_back(L"pipe://stdin");
		} else {
			slFiles.push_back(ParseFileName(param));
		}
	}
}

void CAppSettings::GetFav(favtype ft, std::list<CString>& sl)
{
	sl.clear();

	CString root;
	int maxcount;

	switch (ft) {
	case FAV_FILE:
		root = IDS_R_FAVFILES;
		maxcount = ID_FAVORITES_FILE_END - ID_FAVORITES_FILE_START + 1;
		break;
	case FAV_DVD:
		root = IDS_R_FAVDVDS;
		maxcount = ID_FAVORITES_DVD_END - ID_FAVORITES_DVD_START + 1;
		break;
	case FAV_DEVICE:
		root = IDS_R_FAVDEVICES;
		maxcount = ID_FAVORITES_DEVICE_END - ID_FAVORITES_DEVICE_START + 1;
		break;
	default:
		return;
	}

	CProfile& profile = AfxGetProfile();
	for (int i = 0; i < maxcount; i++) {
		CString fav;
		fav.Format(L"Fav%02d", i);
		CString s;
		profile.ReadString(root, fav, s);
		if (s.IsEmpty()) {
			break;
		}

		std::list<CString> args;
		ExplodeEsc(s, args, L'|');
		if (args.size() < 4) {
			ASSERT(FALSE);
			continue;
		}
		sl.push_back(s);
	}
}

void CAppSettings::SetFav(favtype ft, std::list<CString>& sl)
{
	CString root;

	switch (ft) {
	case FAV_FILE:   root = IDS_R_FAVFILES;   break;
	case FAV_DVD:    root = IDS_R_FAVDVDS;    break;
	case FAV_DEVICE: root = IDS_R_FAVDEVICES; break;
	default:
		return;
	}

	CProfile& profile = AfxGetProfile();
	profile.DeleteSection(root);

	int i = 0;
	for (const auto& item : sl) {
		CString s;
		s.Format(L"Fav%02d", i++);
		profile.WriteString(root, s, item);
	}
}

void CAppSettings::AddFav(favtype ft, CString s)
{
	std::list<CString> sl;
	GetFav(ft, sl);
	if (sl.size() < 100 && !Contains(sl, s)) {
		sl.push_back(s);
		SetFav(ft, sl);
	}
}

CDVBChannel* CAppSettings::FindChannelByPref(int nPrefNumber)
{
	for (auto& Channel : m_DVBChannels) {
		if (Channel.GetPrefNumber() == nPrefNumber) {
			return &Channel;
		}
	}

	return nullptr;
}

bool CAppSettings::IsISRSelect() const
{
	return (m_VRSettings.iVideoRenderer == VIDRNDT_EVR_CP ||
			m_VRSettings.iVideoRenderer == VIDRNDT_DXR    ||
			m_VRSettings.iVideoRenderer == VIDRNDT_SYNC   ||
			m_VRSettings.iVideoRenderer == VIDRNDT_MADVR  ||
			m_VRSettings.iVideoRenderer == VIDRNDT_MPCVR);
}

bool CAppSettings::IsISRAutoLoadEnabled() const
{
#if ENABLE_ASSFILTERMOD
	return (iSubtitleRenderer == SUBRNDT_ISR || iSubtitleRenderer == SUBRNDT_ASSFILTERMOD) && IsISRSelect(); // hmmm
#else
	return iSubtitleRenderer == SUBRNDT_ISR && IsISRSelect();
#endif
}

void CAppSettings::SavePlaylistTabSetting()
{
	AfxGetProfile().WriteString(IDS_R_SETTINGS, IDS_RS_PLAYLISTTABSSETTINGS, strTabs);
}

void CAppSettings::LoadFormats(const bool bLoadLanguage)
{
	auto& profile = AfxGetProfile();

	if (bLoadLanguage) {
		// Set interface language first!
		CString str;
		profile.ReadString(IDS_R_SETTINGS, IDS_RS_LANGUAGE, str);
		iLanguage = CMPlayerCApp::GetLanguageIndex(str);
		if (iLanguage < 0) {
			iLanguage = CMPlayerCApp::GetDefLanguage();
		}
		CMPlayerCApp::SetLanguage(iLanguage, false);
	}

	// Associated types with icon or not...
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_ASSOCIATED_WITH_ICON, bAssociatedWithIcons);
	// file/dir context menu
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_CONTEXTFILES, bSetContextFiles);
	profile.ReadBool(IDS_R_SETTINGS, IDS_RS_CONTEXTDIR, bSetContextDir);

	m_Formats.UpdateData(false);
}

void CAppSettings::SaveFormats()
{
	auto& profile = AfxGetProfile();

	// Associated types with icon or not...
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_ASSOCIATED_WITH_ICON, bAssociatedWithIcons);
	// file/dir context menu
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_CONTEXTFILES, bSetContextFiles);
	profile.WriteBool(IDS_R_SETTINGS, IDS_RS_CONTEXTDIR, bSetContextDir);

	m_Formats.UpdateData(true);
}

// Settings::CRecentFileAndURLList
CAppSettings::CRecentFileAndURLList::CRecentFileAndURLList(UINT nStart, LPCTSTR lpszSection,
		LPCTSTR lpszEntryFormat, int nSize,
		int nMaxDispLen)
	: CRecentFileList(nStart, lpszSection, lpszEntryFormat, nSize, nMaxDispLen)
{
}

extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

void CAppSettings::CRecentFileAndURLList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != nullptr);
	ASSERT(lpszPathName != nullptr);
	ASSERT(AfxIsValidString(lpszPathName));

	CString pathName = lpszPathName;
	if (CString(pathName).MakeLower().Find(L"@device:") >= 0) {
		return;
	}

	bool isURL = (pathName.Find(L"://") >= 0 || Youtube::CheckURL(lpszPathName));

	// fully qualify the path name
	if (!isURL) {
		AfxFullPath(pathName.GetBuffer(pathName.GetLength() + 1024), lpszPathName);
		pathName.ReleaseBuffer();
	}

	// update the MRU list, if an existing MRU string matches file name
	int iMRU;
	for (iMRU = 0; iMRU < m_nSize-1; iMRU++) {
		if ((isURL && !wcscmp(m_arrNames[iMRU], pathName))
				|| AfxComparePath(m_arrNames[iMRU], pathName)) {
			break;    // iMRU will point to matching entry
		}
	}
	// move MRU strings before this one down
	for (; iMRU > 0; iMRU--) {
		ASSERT(iMRU > 0);
		ASSERT(iMRU < m_nSize);
		m_arrNames[iMRU] = m_arrNames[iMRU-1];
	}
	// place this one at the beginning
	m_arrNames[0] = pathName;
}

void CAppSettings::CRecentFileAndURLList::SetSize(int nSize)
{
	ENSURE_ARG(nSize >= 0);

	if (m_nSize != nSize) {
		CString* arrNames = DNew CString[nSize];
		int nSizeToCopy = std::min(m_nSize, nSize);
		for (int i = 0; i < nSizeToCopy; i++) {
			arrNames[i] = m_arrNames[i];
		}
		delete [] m_arrNames;
		m_arrNames = arrNames;
		m_nSize = nSize;
	}
}
