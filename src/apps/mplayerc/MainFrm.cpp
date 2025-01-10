/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "MainFrm.h"
#include <afxglobals.h>
#include <..\src\mfc\afximpl.h>

#include "DSUtil/SysVersion.h"
#include "DSUtil/Filehandle.h"
#include "DSUtil/FileVersion.h"
#include "DSUtil/DXVAState.h"
#include "DSUtil/std_helper.h"
#include "DSUtil/UrlParser.h"
#include "DSUtil/NullRenderers.h"
#include "OpenDlg.h"
#include "SaveTaskDlg.h"
#include "GoToDlg.h"
#include "PnSPresetsDlg.h"
#include "MediaTypesDlg.h"
#include "SaveTextFileDialog.h"
#include "SaveImageDialog.h"
#include "FavoriteAddDlg.h"
#include "FavoriteOrganizeDlg.h"
#include "ShaderCombineDlg.h"
#include "FullscreenWnd.h"
#include "TunerScanDlg.h"
#include "UpdateChecker.h"

#include <ExtLib/BaseClasses/mtype.h>
#include <Mpconfig.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdevcod.h>
#include <dsound.h>
#include <moreuuids.h>
#include <clsids.h>
#include <psapi.h>

#include "GraphThread.h"
#include "FGManager.h"
#include "FGManagerBDA.h"
#include "filters/TextPassThruFilter.h"
#include "filters/ChaptersSouce.h"
#include "filters/parser/MpegSplitter/MpegSplitter.h"
#include "filters/parser/MatroskaSplitter/IMatroskaSplitter.h"
#include "filters/switcher/AudioSwitcher/AudioSwitcher.h"
#include "filters/transform/MPCVideoDec/MPCVideoDec.h"
#include "filters/filters/InternalPropertyPage.h"
#include <filters/renderer/VideoRenderers/IPinHook.h>
#include "filters/ffmpeg_link_fix.h"
#include "ComPropertySheet.h"
#include <comdef.h>
#include <dwmapi.h>

#include "DIB.h"

#include "Subtitles/RenderedHdmvSubtitle.h"
#include "Subtitles/XSUBSubtitle.h"
#include "SubPic/MemSubPic.h"

#include "MultiMonitor.h"

#include <random>

#include "ThumbsTaskDlg.h"
#include "Content.h"

#include <SubRenderIntf.h>
namespace LAVVideo
{
#include <LAVVideoSettings.h>
}
#include "filters/renderer/VideoRenderers/Variables.h"

#include "PlayerYouTubeDL.h"
#include "./Controls/MenuEx.h"

#include "Version.h"
#include "Win10Api.h"

#define DEFCLIENTW		292
#define DEFCLIENTH		200
#define MENUBARBREAK	30

static UINT s_uTaskbarRestart		= RegisterWindowMessageW(L"TaskbarCreated");
static UINT s_uTBBC					= RegisterWindowMessageW(L"TaskbarButtonCreated");
static UINT WM_NOTIFYICON			= RegisterWindowMessageW(L"MYWM_NOTIFYICON");
static UINT s_uQueryCancelAutoPlay	= RegisterWindowMessageW(L"QueryCancelAutoPlay");

class CSubClock : public CUnknown, public ISubClock
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
		return
			QI(ISubClock)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	REFERENCE_TIME m_rt;

public:
	CSubClock() : CUnknown(L"CSubClock", nullptr) {
		m_rt = 0;
	}

	DECLARE_IUNKNOWN;

	// ISubClock
	STDMETHODIMP SetTime(REFERENCE_TIME rt) {
		m_rt = rt;
		return S_OK;
	}
	STDMETHODIMP_(REFERENCE_TIME) GetTime() {
		return(m_rt);
	}
};


static LPCWSTR s_strPlayerTitle = "MPC-BE "
#ifdef _WIN64
	L"x64 "
#endif
	MPC_VERSION_WSTR
#if (MPC_VERSION_STATUS == 0)
	" dev"
#endif
	;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_MESSAGE(WM_MPCVR_SWITCH_FULLSCREEN, OnMPCVRSwitchFullscreen)
	ON_MESSAGE(WM_DXVASTATE_CHANGE, OnDXVAStateChange)

	ON_MESSAGE(WM_HANDLE_CMDLINE, HandleCmdLine)

	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()

	ON_REGISTERED_MESSAGE(s_uTaskbarRestart, OnTaskBarRestart)
	ON_REGISTERED_MESSAGE(s_uTBBC, OnTaskBarThumbnailsCreate)
	ON_REGISTERED_MESSAGE(WM_NOTIFYICON, OnNotifyIcon)
	ON_REGISTERED_MESSAGE(s_uQueryCancelAutoPlay, OnQueryCancelAutoPlay)

	ON_MESSAGE(WM_DWMSENDICONICTHUMBNAIL, OnDwmSendIconicThumbnail)
	ON_MESSAGE(WM_DWMSENDICONICLIVEPREVIEWBITMAP, OnDwmSendIconicLivePreviewBitmap)

	ON_WM_SETFOCUS()
	ON_WM_GETMINMAXINFO()
	ON_WM_ENTERSIZEMOVE()
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_MESSAGE_VOID(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_WINDOWPOSCHANGING()

	ON_WM_QUERYENDSESSION()

	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_MESSAGE(WM_DWMCOMPOSITIONCHANGED, OnDwmCompositionChanged)

	ON_WM_SYSCOMMAND()
	ON_WM_ACTIVATEAPP()
	ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)
	ON_WM_INPUT()
	ON_MESSAGE(WM_HOTKEY, OnHotKey)

	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()

	ON_WM_TIMER()

	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_MESSAGE(WM_RESIZE_DEVICE, OnResizeDevice)
	ON_MESSAGE(WM_RESET_DEVICE, OnResetDevice)

	ON_MESSAGE(WM_POSTOPEN, OnPostOpen)

	ON_MESSAGE_VOID(WM_SAVESETTINGS, SaveAppSettings)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_MESSAGE(WM_XBUTTONDOWN, OnXButtonDown)
	ON_MESSAGE(WM_XBUTTONUP, OnXButtonUp)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEMOVE()

	ON_WM_NCHITTEST()

	ON_WM_HSCROLL()

	ON_WM_INITMENU()
	ON_WM_INITMENUPOPUP()
	ON_WM_UNINITMENUPOPUP()

	ON_COMMAND(ID_MENU_PLAYER_SHORT, OnMenuPlayerShort)
	ON_COMMAND(ID_MENU_PLAYER_LONG, OnMenuPlayerLong)
	ON_COMMAND(ID_MENU_FILTERS, OnMenuFilters)
	ON_COMMAND(ID_MENU_AFTERPLAYBACK, OnMenuAfterPlayback)
	ON_COMMAND(ID_MENU_AUDIOLANG, OnMenuNavAudio)
	ON_COMMAND(ID_MENU_SUBTITLELANG, OnMenuNavSubtitle)
	ON_COMMAND(ID_MENU_JUMPTO, OnMenuNavJumpTo)
	ON_COMMAND(ID_MENU_RECENT_FILES, OnMenuRecentFiles)
	ON_COMMAND(ID_MENU_FAVORITES, OnMenuFavorites)

	ON_UPDATE_COMMAND_UI(IDC_PLAYERSTATUS, OnUpdatePlayerStatus)

	ON_COMMAND(ID_BOSS, OnBossKey)

	ON_COMMAND_RANGE(ID_STREAM_AUDIO_NEXT, ID_STREAM_AUDIO_PREV, OnStreamAudio)
	ON_COMMAND_RANGE(ID_STREAM_SUB_NEXT, ID_STREAM_SUB_PREV, OnStreamSub)
	ON_COMMAND(ID_STREAM_SUB_ONOFF, OnStreamSubOnOff)
	ON_COMMAND_RANGE(ID_STREAM_VIDEO_NEXT, ID_STREAM_VIDEO_PREV, OnStreamVideo)

	ON_COMMAND(ID_FILE_OPENFILE, OnFileOpenQuick)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENFILE, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENFILEURL, OnFileOpenMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENFILEURL, OnUpdateFileOpen)
	ON_WM_COPYDATA()
	ON_COMMAND(ID_FILE_OPENDVD, OnFileOpenDVD)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDVD, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENISO, OnFileOpenIso)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENISO, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENDEVICE, OnFileOpenDevice)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDEVICE, OnUpdateFileOpen)
	ON_COMMAND_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnFileOpenCD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_REOPEN, OnFileReOpen)
	ON_COMMAND(ID_FILE_SAVE_COPY, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_COPY, OnUpdateFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE_IMAGE, OnFileSaveImage)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_IMAGE, OnUpdateFileSaveImage)
	ON_COMMAND(ID_FILE_AUTOSAVE_IMAGE, OnAutoSaveImage)
	ON_COMMAND(ID_FILE_AUTOSAVE_DISPLAY, OnAutoSaveDisplay)
	ON_COMMAND(ID_COPY_IMAGE, OnCopyImage)
	ON_COMMAND(ID_FILE_SAVE_THUMBNAILS, OnFileSaveThumbnails)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_THUMBNAILS, OnUpdateFileSaveThumbnails)
	ON_COMMAND(ID_FILE_LOAD_SUBTITLE, OnFileLoadSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_SUBTITLE, OnUpdateFileLoadSubtitle)
	ON_COMMAND(ID_FILE_SAVE_SUBTITLE, OnFileSaveSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_SUBTITLE, OnUpdateFileSaveSubtitle)
	ON_COMMAND(ID_FILE_LOAD_AUDIO, OnFileLoadAudio)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_AUDIO, OnUpdateFileLoadAudio)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPLAYLIST, OnUpdateFileClose)
	ON_COMMAND(ID_FILE_CLOSEMEDIA, OnFileCloseMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEMEDIA, OnUpdateFileClose)
	ON_COMMAND(ID_REPEAT_FOREVER, OnRepeatForever)
	ON_UPDATE_COMMAND_UI(ID_REPEAT_FOREVER, OnUpdateRepeatForever)
	ON_COMMAND_RANGE(ID_PLAY_REPEAT_AB, ID_PLAY_REPEAT_AB_MARK_B, OnABRepeat)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_REPEAT_AB, ID_PLAY_REPEAT_AB_MARK_B, OnUpdateABRepeat)

	ON_COMMAND(ID_VIEW_CAPTIONMENU, OnViewCaptionmenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTIONMENU, OnUpdateViewCaptionmenu)
	ON_COMMAND_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnViewControlBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnUpdateViewControlBar)
	ON_COMMAND(ID_VIEW_SUBRESYNC, OnViewSubresync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUBRESYNC, OnUpdateViewSubresync)
	ON_COMMAND(ID_VIEW_PLAYLIST, OnViewPlaylist)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAYLIST, OnUpdateViewPlaylist)
	ON_COMMAND(ID_VIEW_CAPTURE, OnViewCapture)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTURE, OnUpdateViewCapture)
	ON_COMMAND(ID_VIEW_SHADEREDITOR, OnViewShaderEditor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHADEREDITOR, OnUpdateViewShaderEditor)
	ON_COMMAND(ID_VIEW_PRESETS_MINIMAL, OnViewMinimal)
	ON_COMMAND(ID_VIEW_PRESETS_COMPACT, OnViewCompact)
	ON_COMMAND(ID_VIEW_PRESETS_NORMAL, OnViewNormal)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_COMMAND(ID_VIEW_FULLSCREEN_2, OnViewFullscreenSecondary)

	ON_COMMAND(ID_WINDOW_TO_PRIMARYSCREEN, OnMoveWindowToPrimaryScreen)

	ON_COMMAND_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnViewZoom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnUpdateViewZoom)
	ON_COMMAND(ID_VIEW_ZOOM_AUTOFIT, OnViewZoomAutoFit)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_AUTOFIT, OnUpdateViewZoom)
	ON_COMMAND_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_ZOOM2, OnViewDefaultVideoFrame)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_ZOOM2, OnUpdateViewDefaultVideoFrame)
	ON_COMMAND(ID_VIEW_VF_SWITCHZOOM, OnViewSwitchVideoFrame)
	ON_COMMAND(ID_VIEW_VF_KEEPASPECTRATIO, OnViewKeepaspectratio)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_KEEPASPECTRATIO, OnUpdateViewKeepaspectratio)
	ON_COMMAND(ID_VIEW_VF_COMPMONDESKARDIFF, OnViewCompMonDeskARDiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_COMPMONDESKARDIFF, OnUpdateViewCompMonDeskARDiff)
	ON_COMMAND_RANGE(ID_VIEW_RESET, ID_PANSCAN_CENTER, OnViewPanNScan)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RESET, ID_PANSCAN_CENTER, OnUpdateViewPanNScan)
	ON_COMMAND_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnViewPanNScanPresets)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnUpdateViewPanNScanPresets)
	ON_COMMAND_RANGE(ID_PANSCAN_ROTATE_CCW, ID_PANSCAN_ROTATE_CW, OnViewRotate)
	ON_COMMAND(ID_PANSCAN_FLIP, OnViewFlip)
	ON_COMMAND_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnViewAspectRatio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnUpdateViewAspectRatio)
	ON_COMMAND(ID_ASPECTRATIO_NEXT, OnViewAspectRatioNext)
	ON_COMMAND_RANGE(ID_STEREO3D_AUTO, ID_STEREO3D_OVERUNDER, OnViewStereo3DMode)
	ON_UPDATE_COMMAND_UI_RANGE(ID_STEREO3D_AUTO, ID_STEREO3D_OVERUNDER, OnUpdateViewStereo3DMode)
	ON_UPDATE_COMMAND_UI(ID_STEREO3D_SWAP_LEFTRIGHT, OnUpdateViewSwapLeftRight)
	ON_COMMAND(ID_STEREO3D_SWAP_LEFTRIGHT, OnViewSwapLeftRight)
	ON_COMMAND_RANGE(ID_ONTOP_NEVER, ID_ONTOP_WHILEPLAYINGVIDEO, OnViewOntop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ONTOP_NEVER, ID_ONTOP_WHILEPLAYINGVIDEO, OnUpdateViewOntop)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)

	ON_UPDATE_COMMAND_UI(ID_VIEW_TEARING_TEST, OnUpdateViewTearingTest)
	ON_COMMAND(ID_VIEW_TEARING_TEST, OnViewTearingTest)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DISPLAYSTATS, OnUpdateViewDisplayStats)
	ON_COMMAND(ID_VIEW_RESETSTATS, OnViewResetStats)
	ON_COMMAND(ID_VIEW_DISPLAYSTATS, OnViewDisplayStatsSC)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENABLEFRAMETIMECORRECTION, OnUpdateViewEnableFrameTimeCorrection)

	ON_COMMAND(ID_VIEW_ENABLEFRAMETIMECORRECTION, OnViewEnableFrameTimeCorrection)
	ON_COMMAND(ID_VIEW_VSYNC, OnViewVSync)
	ON_COMMAND(ID_VIEW_VSYNCINTERNAL, OnViewVSyncInternal)

	ON_COMMAND(ID_VIEW_EXCLUSIVE_FULLSCREEN, OnViewD3DFullScreen)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EXCLUSIVE_FULLSCREEN, OnUpdateViewD3DFullscreen)

	ON_COMMAND(ID_VIEW_RESET_DEFAULT, OnViewResetDefault)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RESET_DEFAULT, OnUpdateViewResetDefault)

	ON_COMMAND(ID_VIEW_VSYNCOFFSET_INCREASE, OnViewVSyncOffsetIncrease)
	ON_COMMAND(ID_VIEW_VSYNCOFFSET_DECREASE, OnViewVSyncOffsetDecrease)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_1_ENABLE, OnUpdateShaderToggle1)
	ON_COMMAND(ID_SHADERS_1_ENABLE, OnShaderToggle1)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_2_ENABLE, OnUpdateShaderToggle2)
	ON_COMMAND(ID_SHADERS_2_ENABLE, OnShaderToggle2)

	ON_UPDATE_COMMAND_UI(ID_OSD_LOCAL_TIME, OnUpdateViewOSDLocalTime)
	ON_UPDATE_COMMAND_UI(ID_OSD_FILE_NAME, OnUpdateViewOSDFileName)
	ON_COMMAND(ID_OSD_LOCAL_TIME, OnViewOSDLocalTime)
	ON_COMMAND(ID_OSD_FILE_NAME, OnViewOSDFileName)

	ON_COMMAND(ID_VIEW_REMAINING_TIME, OnViewRemainingTime)
	ON_COMMAND(ID_D3DFULLSCREEN_TOGGLE, OnD3DFullscreenToggle)
	ON_COMMAND_RANGE(ID_GOTO_PREV_SUB, ID_GOTO_NEXT_SUB, OnGotoSubtitle)
	ON_COMMAND_RANGE(ID_SHIFT_SUB_DOWN, ID_SHIFT_SUB_UP, OnShiftSubtitle)
	ON_COMMAND_RANGE(ID_SUB_DELAY_DEC, ID_SUB_DELAY_INC, OnSubtitleDelay)
	ON_COMMAND_RANGE(ID_LANGUAGE_ENGLISH, ID_LANGUAGE_LAST, OnLanguage)

	ON_COMMAND(ID_PLAY_PLAY, OnPlayPlay)
	ON_COMMAND(ID_PLAY_PAUSE, OnPlayPause)
	ON_COMMAND(ID_PLAY_PLAYPAUSE, OnPlayPlayPause)
	ON_COMMAND(ID_PLAY_STOP, OnPlayStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAY, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAYPAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STOP, OnUpdatePlayPauseStop)

	ON_COMMAND(ID_NAVIGATE_SUBTITLES, OnMenuNavSubtitle)
	ON_COMMAND(ID_NAVIGATE_AUDIO, OnMenuNavAudio)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_SUBTITLES, OnUpdateMenuNavSubtitle)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_AUDIO, OnUpdateMenuNavAudio)

	ON_COMMAND_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEP_BACK, OnPlayFrameStep)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEP_BACK, OnUpdatePlayFrameStep)
	ON_COMMAND_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE, OnPlaySeek)
	ON_COMMAND(ID_PLAY_SEEKBEGIN, OnPlaySeekBegin)
	ON_COMMAND_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnPlaySeekKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE, OnUpdatePlaySeek)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnUpdatePlaySeek)
	ON_COMMAND(ID_PLAY_GOTO, OnPlayGoto)
	ON_UPDATE_COMMAND_UI(ID_PLAY_GOTO, OnUpdateGoto)
	ON_COMMAND_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnPlayChangeRate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnUpdatePlayChangeRate)
	ON_COMMAND(ID_PLAY_RESETRATE, OnPlayResetRate)
	ON_UPDATE_COMMAND_UI(ID_PLAY_RESETRATE, OnUpdatePlayResetRate)
	ON_COMMAND_RANGE(ID_PLAY_AUDIODELAY_PLUS, ID_PLAY_AUDIODELAY_MINUS, OnPlayChangeAudDelay)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_AUDIODELAY_PLUS, ID_PLAY_AUDIODELAY_MINUS, OnUpdatePlayChangeAudDelay)
	ON_COMMAND(ID_FILTERS_COPY_TO_CLIPBOARD, OnPlayFiltersCopyToClipboard)
	ON_COMMAND_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnPlayFilters)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnUpdatePlayFilters)
	ON_COMMAND(ID_SHADERS_SELECT, OnSelectShaders)

	ON_COMMAND(ID_AUDIO_OPTIONS, OnMenuAudioOption)
	ON_COMMAND(ID_SUBTITLES_OPTIONS, OnMenuSubtitlesOption)
	ON_COMMAND(ID_SUBTITLES_ENABLE, OnMenuSubtitlesEnable)
	ON_UPDATE_COMMAND_UI(ID_SUBTITLES_ENABLE, OnUpdateSubtitlesEnable)
	ON_COMMAND(ID_SUBTITLES_STYLES, OnMenuSubtitlesStyle)
	ON_UPDATE_COMMAND_UI(ID_SUBTITLES_STYLES, OnUpdateSubtitlesStyle)
	ON_COMMAND(ID_SUBTITLES_RELOAD, OnMenuSubtitlesReload)
	ON_UPDATE_COMMAND_UI(ID_SUBTITLES_RELOAD, OnUpdateSubtitlesReload)
	ON_COMMAND(ID_SUBTITLES_DEFSTYLE, OnMenuSubtitlesDefStyle)
	ON_UPDATE_COMMAND_UI(ID_SUBTITLES_DEFSTYLE, OnUpdateSubtitlesDefStyle)
	ON_COMMAND(ID_SUBTITLES_FORCEDONLY, OnMenuSubtitlesForcedOnly)
	ON_UPDATE_COMMAND_UI(ID_SUBTITLES_FORCEDONLY, OnUpdateSubtitlesForcedOnly)
	ON_COMMAND_RANGE(ID_SUBTITLES_STEREO_DONTUSE, ID_SUBTITLES_STEREO_TOPBOTTOM, OnStereoSubtitles)

	ON_COMMAND_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnSelectStream)
	ON_COMMAND_RANGE(ID_VOLUME_UP, ID_VOLUME_MUTE, OnPlayVolume)
	ON_COMMAND_RANGE(ID_VOLUME_GAIN_INC, ID_VOLUME_GAIN_MAX, OnPlayVolumeGain)
	ON_COMMAND(ID_NORMALIZE, OnAutoVolumeControl)
	ON_COMMAND_RANGE(ID_AUDIO_CENTER_INC, ID_AUDIO_CENTER_DEC, OnPlayCenterLevel)

	ON_COMMAND_RANGE(ID_COLOR_BRIGHTNESS_INC, ID_COLOR_RESET, OnPlayColor)

	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_LOCK, OnAfterplayback)
	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_NEXT, ID_AFTERPLAYBACK_DONOTHING, OnAfterplayback)
	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, OnAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_LOCK, OnUpdateAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_NEXT, ID_AFTERPLAYBACK_DONOTHING, OnUpdateAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, OnUpdateAfterplayback)

	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnNavigateSkip)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnUpdateNavigateSkip)
	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACKFILE, ID_NAVIGATE_SKIPFORWARDFILE, OnNavigateSkipFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACKFILE, ID_NAVIGATE_SKIPFORWARDFILE, OnUpdateNavigateSkipFile)
	ON_COMMAND_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnNavigateMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnUpdateNavigateMenu)

	ON_COMMAND_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnNavigateAudio)
	ON_COMMAND_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnNavigateSubpic)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnUpdateNavMixSubtitles)
	ON_COMMAND_RANGE(ID_VIDEO_STREAMS_SUBITEM_START, ID_VIDEO_STREAMS_SUBITEM_END, OnNavigateAngle)

	ON_COMMAND_RANGE(ID_NAVIGATE_CHAP_SUBITEM_START, ID_NAVIGATE_CHAP_SUBITEM_END, OnNavigateChapters)
	ON_COMMAND_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnNavigateMenuItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnUpdateNavigateMenuItem)
	ON_COMMAND(ID_NAVIGATE_TUNERSCAN, OnTunerScan)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_TUNERSCAN, OnUpdateTunerScan)

	ON_COMMAND(ID_FAVORITES_ADD, OnFavoritesAdd)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ADD, OnUpdateFavoritesAdd)
	ON_COMMAND(ID_FAVORITES_QUICKADD, OnFavoritesQuickAdd)
	ON_COMMAND(ID_FAVORITES_ORGANIZE, OnFavoritesOrganize)
	ON_COMMAND_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnFavoritesFile)
	ON_COMMAND_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnFavoritesDVD)

	ON_COMMAND(ID_SHOW_HISTORY, OnShowHistory)
	ON_COMMAND(ID_RECENT_FILES_CLEAR, OnRecentFileClear)
	ON_UPDATE_COMMAND_UI(ID_RECENT_FILES_CLEAR, OnUpdateRecentFileClear)
	ON_COMMAND_RANGE(ID_RECENT_FILE_START, ID_RECENT_FILE_END, OnRecentFile)

	ON_COMMAND(ID_HELP_HOMEPAGE, OnHelpHomepage)
	ON_COMMAND(ID_HELP_CHECKFORUPDATE, OnHelpCheckForUpdate)
	//ON_COMMAND(ID_HELP_DOCUMENTATION, OnHelpDocumentation)
	ON_COMMAND(ID_HELP_TOOLBARIMAGES, OnHelpToolbarImages)
	//ON_COMMAND(ID_HELP_DONATE, OnHelpDonate)

	// Open Dir incl. SubDir
	ON_COMMAND(ID_FILE_OPENDIRECTORY, OnFileOpenDirectory)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDIRECTORY, OnUpdateFileOpen)
	ON_WM_POWERBROADCAST()

	// Navigation panel
	ON_COMMAND(ID_VIEW_NAVIGATION, OnViewNavigation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_NAVIGATION, OnUpdateViewNavigation)

	// Subtitle position
	ON_COMMAND_RANGE(ID_SUB_POS_UP, ID_SUB_POS_RESTORE, OnSubtitlePos)

	ON_COMMAND(ID_SUB_COPYTOCLIPBOARD, OnSubCopyClipboard)

	ON_COMMAND_RANGE(ID_SUB_SIZE_DEC, ID_SUB_SIZE_INC, OnSubtitleSize)

	ON_COMMAND(ID_ADDTOPLAYLISTROMCLIPBOARD, OnAddToPlaylistFromClipboard)

	ON_COMMAND(ID_MOVEWINDOWBYVIDEO_ONOFF, OnChangeMouseEasyMove)

	ON_COMMAND(ID_PLAYLIST_OPENFOLDER, OnPlaylistOpenFolder)

	ON_WM_WTSSESSION_CHANGE()

	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

#ifdef DEBUG_OR_LOG
const WCHAR* GetEventString(LONG evCode)
{
#define UNPACK_VALUE(VALUE) case VALUE: return L#VALUE;
	switch (evCode) {
		UNPACK_VALUE(EC_COMPLETE);
		UNPACK_VALUE(EC_USERABORT);
		UNPACK_VALUE(EC_ERRORABORT);
		UNPACK_VALUE(EC_TIME);
		UNPACK_VALUE(EC_REPAINT);
		UNPACK_VALUE(EC_STREAM_ERROR_STOPPED);
		UNPACK_VALUE(EC_STREAM_ERROR_STILLPLAYING);
		UNPACK_VALUE(EC_ERROR_STILLPLAYING);
		UNPACK_VALUE(EC_PALETTE_CHANGED);
		UNPACK_VALUE(EC_VIDEO_SIZE_CHANGED);
		UNPACK_VALUE(EC_QUALITY_CHANGE);
		UNPACK_VALUE(EC_SHUTTING_DOWN);
		UNPACK_VALUE(EC_CLOCK_CHANGED);
		UNPACK_VALUE(EC_PAUSED);
		UNPACK_VALUE(EC_OPENING_FILE);
		UNPACK_VALUE(EC_BUFFERING_DATA);
		UNPACK_VALUE(EC_FULLSCREEN_LOST);
		UNPACK_VALUE(EC_ACTIVATE);
		UNPACK_VALUE(EC_NEED_RESTART);
		UNPACK_VALUE(EC_WINDOW_DESTROYED);
		UNPACK_VALUE(EC_DISPLAY_CHANGED);
		UNPACK_VALUE(EC_STARVATION);
		UNPACK_VALUE(EC_OLE_EVENT);
		UNPACK_VALUE(EC_NOTIFY_WINDOW);
		UNPACK_VALUE(EC_STREAM_CONTROL_STOPPED);
		UNPACK_VALUE(EC_STREAM_CONTROL_STARTED);
		UNPACK_VALUE(EC_END_OF_SEGMENT);
		UNPACK_VALUE(EC_SEGMENT_STARTED);
		UNPACK_VALUE(EC_LENGTH_CHANGED);
		UNPACK_VALUE(EC_DEVICE_LOST);
		UNPACK_VALUE(EC_SAMPLE_NEEDED);
		UNPACK_VALUE(EC_PROCESSING_LATENCY);
		UNPACK_VALUE(EC_SAMPLE_LATENCY);
		UNPACK_VALUE(EC_SCRUB_TIME);
		UNPACK_VALUE(EC_STEP_COMPLETE);
		UNPACK_VALUE(EC_TIMECODE_AVAILABLE);
		UNPACK_VALUE(EC_EXTDEVICE_MODE_CHANGE);
		UNPACK_VALUE(EC_STATE_CHANGE);
		UNPACK_VALUE(EC_GRAPH_CHANGED);
		UNPACK_VALUE(EC_CLOCK_UNSET);
		UNPACK_VALUE(EC_VMR_RENDERDEVICE_SET);
		UNPACK_VALUE(EC_VMR_SURFACE_FLIPPED);
		UNPACK_VALUE(EC_VMR_RECONNECTION_FAILED);
		UNPACK_VALUE(EC_PREPROCESS_COMPLETE);
		UNPACK_VALUE(EC_CODECAPI_EVENT);
		UNPACK_VALUE(EC_WMT_INDEX_EVENT);
		UNPACK_VALUE(EC_WMT_EVENT);
		UNPACK_VALUE(EC_BUILT);
		UNPACK_VALUE(EC_UNBUILT);
		UNPACK_VALUE(EC_SKIP_FRAMES);
		UNPACK_VALUE(EC_PLEASE_REOPEN);
		UNPACK_VALUE(EC_STATUS);
		UNPACK_VALUE(EC_MARKER_HIT);
		UNPACK_VALUE(EC_LOADSTATUS);
		UNPACK_VALUE(EC_FILE_CLOSED);
		UNPACK_VALUE(EC_ERRORABORTEX);
		//UNPACK_VALUE(EC_NEW_PIN);
		//UNPACK_VALUE(EC_RENDER_FINISHED);
		UNPACK_VALUE(EC_EOS_SOON);
		UNPACK_VALUE(EC_CONTENTPROPERTY_CHANGED);
		UNPACK_VALUE(EC_BANDWIDTHCHANGE);
		UNPACK_VALUE(EC_VIDEOFRAMEREADY);

		UNPACK_VALUE(EC_BG_AUDIO_CHANGED);
		UNPACK_VALUE(EC_BG_ERROR);
	};
#undef UNPACK_VALUE
	return L"UNKNOWN";
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	m_eMediaLoadState(MLS_CLOSED),
	m_ePlaybackMode(PM_NONE),
	m_PlaybackRate(1.0),
	m_rtDurationOverride(-1),
	m_bFullScreen(false),
	m_bFullScreenChangingMode(false),
	m_bFirstFSAfterLaunchOnFullScreen(false),
	m_bStartInD3DFullscreen(false),
	m_bHideCursor(false),
	m_lastMouseMove(-1, -1),
	m_lastMouseMoveFullScreen(-1, -1),
	m_pLastBar(nullptr),
	m_nLoops(0),
	m_iSubtitleSel(-1),
	m_iVideoSize(DVS_FROMINSIDE),
	m_ZoomX(1), m_ZoomY(1), m_PosX(0.5), m_PosY(0.5),
	m_iDefRotation(0),
	m_bCustomGraph(false),
	m_bShockwaveGraph(false),
	m_bFrameSteppingActive(false),
	m_bEndOfStream(false),
	m_bGraphEventComplete(false),
	m_bCapturing(false),
	m_bLiveWM(false),
	m_fOpeningAborted(false),
	m_bBuffering(false),
	m_fileDropTarget(this),
	m_bTrayIcon(false),
	m_pFullscreenWnd(nullptr),
	m_pVideoWnd(nullptr),
	m_pOSDWnd(nullptr),
	m_nCurSubtitle(-1),
	m_lSubtitleShift(0),
	m_bToggleShader(false),
	m_nStepForwardCount(0),
	m_rtStepForwardStart(0),
	m_bToggleShaderScreenSpace(false),
	m_bInOptions(false),
	m_lCurrentChapter(0),
	m_lChapterStartTime(0xFFFFFFFF),
	m_pTaskbarList(nullptr),
	m_pGraphThread(nullptr),
	m_bOpenedThruThread(false),
	m_bWasSnapped(false),
	m_bIsBDPlay(FALSE),
	m_bClosingState(false),
	m_flastnID(0),
	m_bfirstPlay(false),
	m_pBFmadVR(nullptr),
	m_hWtsLib(0),
	m_CaptureWndBitmap(nullptr),
	m_ThumbCashedBitmap(nullptr),
	m_nSelSub2(-1),
	m_hGraphThreadEventOpen(FALSE, TRUE),
	m_hGraphThreadEventClose(FALSE, TRUE),
	m_pfnDwmSetIconicThumbnail(nullptr),
	m_pfnDwmSetIconicLivePreviewBitmap(nullptr),
	m_pfnDwmInvalidateIconicBitmaps(nullptr),
	m_OSD(this),
	m_wndView(this),
	m_wndToolBar(this),
	m_wndSeekBar(this),
	m_wndInfoBar(this),
	m_wndStatsBar(this),
	m_wndStatusBar(this),
	m_wndFlyBar(this),
	m_wndPreView(this),
	m_wndPlaylistBar(this),
	m_wndCaptureBar(this),
	m_wndSubresyncBar(this),
	m_dMediaInfoFPS(0.0),
	m_bAudioOnly(true),
	m_bMainIsMPEGSplitter(false),
	m_abRepeatPositionAEnabled(false),
	m_abRepeatPositionBEnabled(false)
{
	m_Lcd.SetVolumeRange(0, 100);

	// Remove the lines-splitters between bars
	afxData.cyBorder2 = 0;

	CMenuEx::Hook();

	m_pThis = this;
	m_MenuHook = ::SetWindowsHookExW(WH_CALLWNDPROC, MenuHookProc, nullptr, ::GetCurrentThreadId());
	ASSERT(m_MenuHook);
}

CMainFrame::~CMainFrame()
{
	if (m_MenuHook) {
		VERIFY(::UnhookWindowsHookEx(m_MenuHook));
	}

	CMenuEx::UnHook();

	if (m_hMainMenuBrush) {
		::DeleteObject(m_hMainMenuBrush);
	}

	if (m_hPopupMenuBrush) {
		::DeleteObject(m_hPopupMenuBrush);
	}
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	UseCurentMonitorDPI(m_hWnd);

	CAppSettings& s = AfxGetAppSettings();

	CMenuEx::SetMain(this);
	CMenuEx::EnableHook(s.bUseDarkTheme && s.bDarkMenu);
	CMenuEx::ScaleFont();

	if (s.bUseDarkTheme && s.bDarkMenu) {
		SetColorMenu();
	}

	SetColorTitle();

	m_popupMenu.LoadMenuW(IDR_POPUP);
	m_popupMainMenu.LoadMenuW(IDR_POPUPMAIN);

	m_VideoFrameMenu.LoadMenuW(IDR_POPUP_VIDEOFRAME);
	m_PanScanMenu.LoadMenuW(IDR_POPUP_PANSCAN);
	m_ShadersMenu.LoadMenuW(IDR_POPUP_SHADERS);
	m_AfterPlaybackMenu.LoadMenuW(IDR_POPUP_AFTERPLAYBACK);
	m_NavigateMenu.LoadMenuW(IDR_POPUP_NAVIGATE);

	// create a Main View Window
	if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						  RECT{0,0,0,0}, this, AFX_IDW_PANE_FIRST)) {
		DLog(L"Failed to create Main View Window");
		return -1;
	}

	// Should never be RTLed
	m_wndView.ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	// Create FlyBar Window
	CreateFlyBar();
	// Create OSD Window
	CreateOSDBar();

	// Create Preview Window
	if (!m_wndPreView.CreateEx(WS_EX_TOPMOST, AfxRegisterWndClass(0), nullptr, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CRect(0, 0, 160, 109), this, 0)) {
		DLog(L"Failed to create Preview Window");
		m_wndView.DestroyWindow();
		return -1;
	} else {
		m_wndPreView.ShowWindow(SW_HIDE);
		m_wndPreView.SetRelativeSize(s.iSmartSeekSize);
	}

	CComPtr<IWICBitmap> pBitmap;
	HRESULT hr = WicLoadImage(&pBitmap, false, (GetProgramDir() + L"background.png").GetString());
	if (SUCCEEDED(hr)) {
		m_BackGroundGradient.Create(pBitmap);
	}

	// static bars
	BOOL bResult = m_wndStatusBar.Create(this);
	if (bResult) {
		bResult = m_wndStatsBar.Create(this);
	}
	if (bResult) {
		bResult = m_wndInfoBar.Create(this);
	}
	if (bResult) {
		bResult = m_wndToolBar.Create(this);
	}
	if (bResult) {
		bResult = m_wndSeekBar.Create(this);
	}
	if (!bResult) {
		DLog(L"Failed to create all control bars");
		return -1;      // fail to create
	}

	m_pFullscreenWnd = DNew CFullscreenWnd(this);

	m_bars.emplace_back(&m_wndSeekBar);
	m_bars.emplace_back(&m_wndToolBar);
	m_bars.emplace_back(&m_wndInfoBar);
	m_bars.emplace_back(&m_wndStatsBar);
	m_bars.emplace_back(&m_wndStatusBar);

	m_wndSeekBar.Enable(false);

	// dockable bars

	EnableDocking(CBRS_ALIGN_ANY);

	m_dockingbars.clear();

	m_wndSubresyncBar.Create(this, AFX_IDW_DOCKBAR_TOP, &m_csSubLock);
	m_wndSubresyncBar.SetBarStyle(m_wndSubresyncBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSubresyncBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndSubresyncBar.SetHeight(200);
	m_dockingbars.emplace_back(&m_wndSubresyncBar);

	m_wndPlaylistBar.Create(this, AFX_IDW_DOCKBAR_RIGHT);
	m_wndPlaylistBar.SetBarStyle(m_wndPlaylistBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPlaylistBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndPlaylistBar.SetHeight(100);
	m_dockingbars.emplace_back(&m_wndPlaylistBar);

	std::vector<CStringW> recentPath;
	AfxGetMyApp()->m_HistoryFile.GetRecentPaths(recentPath, 1);
	if (recentPath.size()) {
		m_wndPlaylistBar.LoadPlaylist(recentPath.front());
		if (s.bKeepHistory) {
			s.strLastOpenFile = recentPath.front();
		}
	} else {
		m_wndPlaylistBar.LoadPlaylist(L"");
	}

	m_wndCaptureBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndCaptureBar.SetBarStyle(m_wndCaptureBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCaptureBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.emplace_back(&m_wndCaptureBar);

	m_wndNavigationBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndNavigationBar.SetBarStyle(m_wndNavigationBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndNavigationBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.emplace_back(&m_wndNavigationBar);

	m_wndShaderEditorBar.Create(this, AFX_IDW_DOCKBAR_TOP);
	m_wndShaderEditorBar.SetBarStyle(m_wndShaderEditorBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndShaderEditorBar.EnableDocking(CBRS_ALIGN_ANY);
	m_dockingbars.emplace_back(&m_wndShaderEditorBar);

	// Hide all dockable bars by default
	for (const auto& pDockingBar : m_dockingbars) {
		pDockingBar->ShowWindow(SW_HIDE);
	}

	m_fileDropTarget.Register(this);

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);

	ShowControls(s.nCS);

	SetAlwaysOnTop(s.iOnTop);

	ShowTrayIcon(s.bTrayIcon);

	SetFocus();

	m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));
	if (m_pGraphThread) {
		m_pGraphThread->SetMainFrame(this);
	}

	if (s.nCmdlnWebServerPort != 0) {
		if (s.nCmdlnWebServerPort > 0) {
			StartWebServer(s.nCmdlnWebServerPort);
		} else if (s.fEnableWebServer) {
			StartWebServer(s.nWebServerPort);
		}
	}

	m_iVideoSize = s.iDefaultVideoSize;
	m_bToggleShader = s.bToggleShader;
	m_bToggleShaderScreenSpace = s.bToggleShaderScreenSpace;

	SetWindowTextW(s_strPlayerTitle);
	m_Lcd.SetMediaTitle(s_strPlayerTitle);

	m_hWnd_toolbar = m_wndToolBar.GetSafeHwnd();

	WTSRegisterSessionNotification();

	s.strFSMonOnLaunch = s.strFullScreenMonitor;
	GetCurDispMode(s.dmFSMonOnLaunch, s.strFSMonOnLaunch);

	if (!SysVersion::IsWin10orLater()) {
		// do not use these functions for Windows 10 due to some problems
		(FARPROC &)m_pfnDwmSetIconicThumbnail         = GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), "DwmSetIconicThumbnail");
		(FARPROC &)m_pfnDwmSetIconicLivePreviewBitmap = GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), "DwmSetIconicLivePreviewBitmap");
		(FARPROC &)m_pfnDwmInvalidateIconicBitmaps    = GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), "DwmInvalidateIconicBitmaps");
	}

	if (!SysVersion::IsWin8orLater()) {
		DwmIsCompositionEnabled(&m_bDesktopCompositionEnabled);
	}

	bool ok = m_svgFlybar.Load(::GetProgramDir() + L"flybar.svg");
	if (!ok) {
		ok = m_svgFlybar.Load(IDF_SVG_FLYBAR);
	}
	m_wndFlyBar.UpdateButtonImages();
	m_OSD.UpdateButtonImages();

	ok = m_svgTaskbarButtons.Load(IDF_SVG_TASKBAR_BUTTONS);

	// Windows 8 - if app is not pinned on the taskbar, it's not receive "TaskbarButtonCreated" message. Bug ???
	if (SysVersion::IsWin8orLater()) {
		CreateThumbnailToolbar();
	}

	m_DiskImage.Init();

	if (s.fAutoReloadExtSubtitles) {
		subChangeNotifyThreadStart();
	}

	// Setup 'CONSENT' cookie for internal Youtube parser
	constexpr auto lpszUrl = L"https://www.youtube.com";

	CString lpszCookieData;
	DWORD dwSize = 2048;

	InternetGetCookieExW(lpszUrl, nullptr, lpszCookieData.GetBuffer(dwSize), &dwSize, INTERNET_COOKIE_HTTPONLY, nullptr);
	lpszCookieData.ReleaseBuffer(dwSize);

	int consentId = 0;
	bool bSetCookies = true;

	if (!lpszCookieData.IsEmpty()) {
		std::list<CString> cookies;
		Explode(lpszCookieData, cookies, L';');
		for (auto& cookie : cookies) {
			cookie.Trim();
			const auto pos = cookie.Find(L'=');
			if (pos > 0) {
				const auto param = cookie.Left(pos);
				const auto value = cookie.Mid(pos + 1);

				if (param == L"__Secure-3PSID") {
					bSetCookies = false;
					break;
				} else if (param == L"CONSENT") {
					if (value.Find(L"YES") != -1) {
						bSetCookies = false;
						break;
					}
					consentId = _wtoi(RegExpParse(value.GetString(), LR"(PENDING\+(\d+))"));
				}
			}
		}
	}

	if (bSetCookies) {
		if (!consentId) {
			std::random_device random_device;
			std::mt19937 generator(random_device());
			std::uniform_int_distribution<> distribution(100, 999);

			consentId = distribution(generator);
		}

		SYSTEMTIME st;
		::GetLocalTime(&st);

		lpszCookieData.Format(L"YES+cb.%04u%02u%02u-17-p0.en+FX+%d", st.wYear, st.wMonth, st.wDay, consentId);
		InternetSetCookieExW(lpszUrl, L"CONSENT", lpszCookieData.GetString(), INTERNET_COOKIE_HTTPONLY, NULL);
	}

	if (s.bWinMediaControls) {
		m_CMediaControls.Init(this);
	}

	RegisterApplicationRestart(L"", RESTART_NO_CRASH | RESTART_NO_HANG);

	cmdLineThread = std::thread([this] { cmdLineThreadFunction(); });

	return 0;
}

void CMainFrame::OnDestroy()
{
	WTSUnRegisterSessionNotification();

	ShowTrayIcon(false);

	m_fileDropTarget.Revoke();

	if (m_pGraphThread) {
		CAMMsgEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_EXIT, 0, (LPARAM)&e);
		if (!e.Wait(10000)) {
			DLog(L"CRITICAL ERROR: Failed to abort graph creation.");
			TerminateThread(m_pGraphThread->m_hThread, 0xDEAD);
		}
	}

	if (m_pFullscreenWnd) {
		if (m_pFullscreenWnd->IsWindow()) {
			m_pFullscreenWnd->DestroyWindow();
		}
		delete m_pFullscreenWnd;
	}

	m_wndPreView.DestroyWindow();

	if (subChangeNotifyThread.joinable()) {
		m_EventSubChangeRefreshNotify.Reset();
		m_EventSubChangeStopNotify.Set();
		subChangeNotifyThread.join();
	}

	m_ExtSubFiles.clear();
	m_ExtSubPaths.clear();

	__super::OnDestroy();
}

void CMainFrame::OnClose()
{
	DLog(L"CMainFrame::OnClose() : start");

	m_bClosingState = true;

	m_EventCmdLineQueue.Reset();
	m_ExitCmdLineQueue.Set();
	if (cmdLineThread.joinable()) {
		cmdLineThread.join();
	}

	KillTimer(TIMER_FLYBARWINDOWHIDER);
	DestroyOSDBar();
	DestroyFlyBar();

	CAppSettings& s = AfxGetAppSettings();

	s.bToggleShader = m_bToggleShader;
	s.bToggleShaderScreenSpace = m_bToggleShaderScreenSpace;

	s.dZoomX = m_ZoomX;
	s.dZoomY = m_ZoomY;

	m_wndPlaylistBar.SavePlaylist();

	if (!s.bResetSettings) {
		for (const auto& pDockingBar : m_dockingbarsVisible) {
			ShowControlBar(pDockingBar, TRUE, FALSE);
		}

		SaveControlBars();
	}

	CloseMedia();

	ShowWindow(SW_HIDE);

	s.WinLircClient.DisConnect();
	s.UIceClient.DisConnect();

	SendAPICommand(CMD_DISCONNECT, L"\0"); // according to CMD_NOTIFYENDOFSTREAM (ctrl+f it here), you're not supposed to send NULL here

	if (s.fullScreenModes.bEnabled && s.fRestoreResAfterExit) {
		SetDispMode(s.dmFSMonOnLaunch, s.strFSMonOnLaunch, TRUE);
	}

	m_pMainBitmap.Release();
	m_pMainBitmapSmall.Release();
	if (m_ThumbCashedBitmap) {
		::DeleteObject(m_ThumbCashedBitmap);
	}
	m_wndView.ClearResizedImage();

	for (const auto& tmpFile : s.slTMPFilesList) {
		if (::PathFileExistsW(tmpFile)) {
			DeleteFileW(tmpFile);
		}
	}
	s.slTMPFilesList.clear();

	if (m_CaptureWndBitmap) {
		DeleteObject(m_CaptureWndBitmap);
		m_CaptureWndBitmap = nullptr;
	}

	DLog(L"CMainFrame::OnClose() : end");
	__super::OnClose();
}

DROPEFFECT CMainFrame::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

DROPEFFECT CMainFrame::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return (pDataObject->IsDataAvailable(CF_HDROP) ||
			pDataObject->IsDataAvailable(CF_URLW) ||
			pDataObject->IsDataAvailable(CF_URLA) ||
			pDataObject->IsDataAvailable(CF_UNICODETEXT) ||
			pDataObject->IsDataAvailable(CF_TEXT)) ?
			DROPEFFECT_COPY :
			DROPEFFECT_NONE;
}

BOOL CMainFrame::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	BOOL bResult = FALSE;
	CLIPFORMAT cfFormat = 0;

	if (pDataObject->IsDataAvailable(CF_URLW)) {
		cfFormat = CF_URLW;
	} else if (pDataObject->IsDataAvailable(CF_URLA)) {
		cfFormat = CF_URLA;
	} else if (pDataObject->IsDataAvailable(CF_HDROP)) {
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_HDROP)) {
			if (HDROP hDrop = (HDROP)GlobalLock(hGlobal)) {
				std::list<CString> slFiles;
				UINT nFiles = ::DragQueryFileW(hDrop, UINT_MAX, nullptr, 0);
				for (UINT iFile = 0; iFile < nFiles; iFile++) {
					CString fn = GetDragQueryFileName(hDrop, iFile);
					slFiles.emplace_back(ParseFileName(fn));
				}
				::DragFinish(hDrop);
				DropFiles(slFiles);
				bResult = TRUE;
			}
			GlobalUnlock(hGlobal);
		}
	} else if (pDataObject->IsDataAvailable(CF_UNICODETEXT)) {
		cfFormat = CF_UNICODETEXT;
	} else if (pDataObject->IsDataAvailable(CF_TEXT)) {
		cfFormat = CF_TEXT;
	}

	if (cfFormat) {
		FORMATETC fmt = { cfFormat, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(cfFormat, &fmt)) {
			LPVOID LockData = GlobalLock(hGlobal);
			auto pUnicodeText = reinterpret_cast<LPCWSTR>(LockData);
			auto pAnsiText = reinterpret_cast<LPCSTR>(LockData);
			bool bUnicode = cfFormat == CF_URLW || cfFormat == CF_UNICODETEXT;
			if (bUnicode ? AfxIsValidString(pUnicodeText) : AfxIsValidString(pAnsiText)) {
				CStringW text = bUnicode ? CStringW(pUnicodeText) : CStringW(pAnsiText);
				if (!text.IsEmpty()) {
					std::list<CString> slFiles;
					if (cfFormat == CF_URLW || cfFormat == CF_URLA) {
						slFiles.emplace_back(text);
					} else {
						std::list<CString> lines;
						Explode(text, lines, L'\n');
						for (const auto& line : lines) {
							auto path = ParseFileName(line);
							if (::PathIsURLW(path) || ::PathFileExistsW(path)) {
								slFiles.emplace_back(path);
							}
						}
					}

					if (!slFiles.empty()) {
						DropFiles(slFiles);
						bResult = TRUE;
					}
				}
			}
			GlobalUnlock(hGlobal);
		}
	}

	return bResult;
}

DROPEFFECT CMainFrame::OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
	if (OnDrop(pDataObject, dropDefault, point)) {
		return dropDefault;
	}

	return DROPEFFECT_NONE;
}

void CMainFrame::OnDragLeave()
{
}

DROPEFFECT CMainFrame::OnDragScroll(DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

void CMainFrame::RestoreControlBars()
{
	for (const auto& pDockingBar : m_dockingbars) {
		CPlayerBar* pBar = dynamic_cast<CPlayerBar*>(pDockingBar);

		if (pBar) {
			pBar->LoadState(this);
		}
	}
}

void CMainFrame::SaveControlBars()
{
	for (const auto& pDockingBar : m_dockingbars) {
		CPlayerBar* pBar = dynamic_cast<CPlayerBar*>(pDockingBar);

		if (pBar) {
			pBar->SaveState();
		}
	}
}

LRESULT CMainFrame::OnTaskBarRestart(WPARAM, LPARAM)
{
	m_bTrayIcon = false;
	ShowTrayIcon(AfxGetAppSettings().bTrayIcon);

	return 0;
}

LRESULT CMainFrame::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
	if ((UINT)wParam != IDR_MAINFRAME) {
		return -1;
	}

	switch ((UINT)lParam) {
		case WM_LBUTTONDOWN:
			ShowWindow(SW_SHOW);
			CreateThumbnailToolbar();
			MoveVideoWindow();
			SetForegroundWindow();

			for (const auto& pDockingBar : m_dockingbarsVisible) {
				ShowControlBar(pDockingBar, TRUE, FALSE);
			}
			m_dockingbarsVisible.clear();
			break;

		case WM_LBUTTONDBLCLK:
			PostMessageW(WM_COMMAND, ID_FILE_OPENFILEURL);
			break;

		case WM_RBUTTONDOWN: {
			POINT p;
			GetCursorPos(&p);
			SetForegroundWindow();
			m_popupMainMenu.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NOANIMATION, p.x, p.y, GetModalParent());
			PostMessageW(WM_NULL);
			break;
		}

		case WM_MOUSEMOVE: {
			CString str;
			GetWindowTextW(str);
			SetTrayTip(str);
			break;
		}

		default:
			break;
	}

	return 0;
}

LRESULT CMainFrame::OnTaskBarThumbnailsCreate(WPARAM, LPARAM)
{
	return CreateThumbnailToolbar();
}

LRESULT CMainFrame::OnQueryCancelAutoPlay(WPARAM wParam, LPARAM lParam)
{
	if (wParam == m_DiskImage.GetDriveLetter() - 'A') {
		DLog(L"CMainFrame: block autopay for %C:\\", (PCHAR)wParam + 'A');
		return TRUE;
	} else {
		DLog(L"CMainFrame: allow autopay for %C:\\", (PCHAR)wParam + 'A');
		return FALSE;
	}
}

void CMainFrame::ShowTrayIcon(bool fShow)
{
	NOTIFYICONDATAW tnid = { sizeof(NOTIFYICONDATAW), m_hWnd, IDR_MAINFRAME };

	if (fShow) {
		if (!m_bTrayIcon) {
			tnid.hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
			tnid.uCallbackMessage = WM_NOTIFYICON;
			StringCchCopyW(tnid.szTip, std::size(tnid.szTip), L"MPC-BE");
			Shell_NotifyIconW(NIM_ADD, &tnid);

			m_bTrayIcon = true;
		}
	} else {
		if (m_bTrayIcon) {
			Shell_NotifyIconW(NIM_DELETE, &tnid);

			m_bTrayIcon = false;
		}
	}
}

void CMainFrame::SetTrayTip(CString str)
{
	NOTIFYICONDATAW tnid;
	tnid.cbSize = sizeof(NOTIFYICONDATAW);
	tnid.hWnd = m_hWnd;
	tnid.uID = IDR_MAINFRAME;
	tnid.uFlags = NIF_TIP;
	StringCchCopyW(tnid.szTip, std::size(tnid.szTip), str);
	Shell_NotifyIconW(NIM_MODIFY, &tnid);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!__super::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	cs.lpszClass = MPC_WND_CLASS_NAMEW;

	return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_ESCAPE) {
			const bool bEscapeNotAssigned = !AssignedKeyToCmd(VK_ESCAPE);
			if (bEscapeNotAssigned
					&& (m_bFullScreen || IsD3DFullScreenMode())) {
				OnViewFullscreen();
				if (m_eMediaLoadState == MLS_LOADED) {
					PostMessageW(WM_COMMAND, ID_PLAY_PAUSE);
				}
				return TRUE;
			}
		} else if (pMsg->wParam == VK_LEFT && m_pAMTuner) {
			PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			return TRUE;
		} else if (pMsg->wParam == VK_RIGHT && m_pAMTuner) {
			PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			return TRUE;
		}
	} else if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_MENU
			&& (m_bIsMadVRExclusiveMode || m_bIsMPCVRExclusiveMode || CursorOnD3DFullScreenWindow())) {
		// disable pressing Alt on D3D FullScreen window/madVR Exclusive/MPCVR Exclusive.
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN) {
		m_bAltDownClean = false;
	}
	if (pMsg->message == WM_SYSKEYDOWN) {
		m_bAltDownClean = (pMsg->wParam == VK_MENU);
	}
	if ((m_dwMenuBarVisibility & AFX_MBV_DISPLAYONFOCUS) && pMsg->message == WM_SYSKEYUP && pMsg->wParam == VK_MENU
			&& m_dwMenuBarState == AFX_MBS_HIDDEN) {
		// mfc shows menubar when Ctrl+Alt+X is released in reverse order, but we don't want to
		if (m_bAltDownClean) {
			VERIFY(SetMenuBarState(AFX_MBS_VISIBLE));
			return FALSE;
		}
		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	__super::RecalcLayout(bNotify);

	m_wndSeekBar.HideToolTip();

	CRect r;
	GetWindowRect(&r);
	MINMAXINFO mmi = { 0 };
	SendMessageW(WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
	const POINT& min = mmi.ptMinTrackSize;
	if (r.Height() < min.y || r.Width() < min.x) {
		r |= CRect(r.TopLeft(), CSize(min));
		MoveWindow(r);
	}

	FlyBarSetPos();
	OSDBarSetPos();
}

static const bool IsMoveX(LONG x1, LONG x2, LONG y1, LONG y2)
{
	return (labs(x1 - x2) > 3 * labs(y1 - y2));
}

static const bool IsMoveY(LONG x1, LONG x2, LONG y1, LONG y2)
{
	return (labs(y1 - y2) > 3 * labs(x1 - x2));
}

BOOL CMainFrame::OnTouchInput(CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput)
{
	if (m_bFullScreen || IsD3DFullScreenMode()) {
		if ((pInput->dwFlags & TOUCHEVENTF_DOWN) == TOUCHEVENTF_DOWN) {
			if (!m_touchScreen.moving) {
				m_touchScreen.Add(pInput->dwID, pt.x, pt.y);
			}
		} else if ((pInput->dwFlags & TOUCHEVENTF_MOVE) == TOUCHEVENTF_MOVE) {
			const int index = m_touchScreen.FindById(pInput->dwID);
			if (index != -1) {
				touchPoint& point = m_touchScreen.point[index];
				if (labs(point.x_start - pt.x) > 5
						|| labs(point.y_start - pt.y) > 5) {
					m_touchScreen.moving = true;
				}

				if (m_touchScreen.moving && m_touchScreen.Count() == 1) {
					if (IsMoveX(pt.y, point.y_start, pt.x, point.x_start)) {
						CRect rc;
						m_wndView.GetWindowRect(&rc);

						const int height  = rc.Height();
						const int diff    = pt.y - point.y_start;
						const int percent = 100 * diff / height;

						if (abs(percent) >= 1) {
							int pos = m_wndToolBar.m_volctrl.GetPos();
							pos += (-percent);
							m_wndToolBar.m_volctrl.SetPosInternal(pos);

							point.x_start = pt.x;
							point.y_start = pt.y;
						}
					} else if (m_eMediaLoadState == MLS_LOADED
							&& IsMoveX(pt.x, point.x_start, pt.y, point.y_start)
							&& AfxGetAppSettings().ShowOSD.SeekTime) {

						REFERENCE_TIME stop = m_wndSeekBar.GetRange();
						if (stop > 0) {
							CRect rc;
							m_wndView.GetWindowRect(&rc);

							const int widht   = rc.Width();
							const int diff    = pt.x - point.x_start;
							const int percent = 100 * diff / widht;

							if (abs(percent) >= 1) {
								const REFERENCE_TIME rtDiff = stop * percent / 100;
								if (abs(rtDiff) >= UNITS) {
									const REFERENCE_TIME rtPos = m_wndSeekBar.GetPos();
									REFERENCE_TIME rtNewPos = rtPos + rtDiff;
									rtNewPos = std::clamp(rtNewPos, 0LL, stop);
									const bool bShowMilliSecs = m_bShowMilliSecs || m_wndSubresyncBar.IsWindowVisible();

									m_wndStatusBar.SetStatusTimer(rtNewPos, stop, bShowMilliSecs, GetTimeFormat());
									m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 1000);
									m_wndStatusBar.SetStatusTimer(rtPos, stop, bShowMilliSecs, GetTimeFormat());
								}
							}
						}
					}
				}
			}
		} else if ((pInput->dwFlags & TOUCHEVENTF_UP) == TOUCHEVENTF_UP) {
			const int index = m_touchScreen.Down(pInput->dwID, pt.x, pt.y);
			if (index != -1 && m_touchScreen.Count() == m_touchScreen.CountEnd()) {
				if (m_eMediaLoadState == MLS_LOADED) {
					if (m_touchScreen.Count() == 1) {
						touchPoint& point = m_touchScreen.point[index];
						if (m_touchScreen.moving) {
							if (IsMoveX(point.x_end, point.x_start, point.y_end, point.y_start)) {
								REFERENCE_TIME stop = m_wndSeekBar.GetRange();
								if (stop > 0) {
									CRect rc;
									m_wndView.GetWindowRect(&rc);

									const int widht   = rc.Width();
									const int diff    = point.x_end - point.x_start;
									const int percent = 100 * diff / widht;

									if (abs(percent) >= 1) {
										const REFERENCE_TIME rtDiff = stop * percent / 100;
										if (abs(rtDiff) >= UNITS) {
											SeekTo(m_wndSeekBar.GetPos() + rtDiff);
										}
									}
								}
							}
						} else {
							auto peekMouseMessage = [&]() {
								MSG msg;
								while (PeekMessageW(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, PM_REMOVE));
							};

							CRect rc;
							m_wndView.GetWindowRect(&rc);

							const int percentX = 100 * point.x_end / rc.Width();
							const int percentY = 100 * point.y_end / rc.Height();
							if (percentX <= 10) {
								peekMouseMessage();
								PostMessageW(WM_COMMAND, ID_PLAY_SEEKBACKWARDMED);
							} else if (percentX >= 90) {
								peekMouseMessage();
								PostMessageW(WM_COMMAND, ID_PLAY_SEEKFORWARDMED);
							} else if (percentX >= 15 && percentX <= 45
									&& percentY <= 15) {
								peekMouseMessage();
								PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
							} else if (percentX >= 55 && percentX <= 85
									&& percentY <= 15) {
								peekMouseMessage();
								PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
							}
						}
					} else if (m_touchScreen.Count() == 2) {
						if (!m_touchScreen.moving) {
							PostMessageW(WM_COMMAND, ID_PLAY_PLAYPAUSE);
						} else {
							touchPoint points[2];
							unsigned index = 0;
							for (unsigned i = 0; i < std::size(m_touchScreen.point); i++) {
								if (m_touchScreen.point[i].dwID != DWORD_MAX && m_touchScreen.point[i].x_end != 0) {
									points[index++] = m_touchScreen.point[i];
								}
							}

							if (IsMoveY(points[0].x_end, points[0].x_start, points[0].y_end, points[0].y_start)
									&& IsMoveY(points[1].x_end, points[1].x_start, points[1].y_end, points[1].y_start)) {
								if (points[0].y_end > points[0].y_start
										&& points[1].y_end > points[1].y_start) {
									PostMessageW(WM_COMMAND, ID_STREAM_AUDIO_NEXT);
								} else if (points[0].y_end < points[0].y_start
										&& points[1].y_end < points[1].y_start) {
									PostMessageW(WM_COMMAND, ID_STREAM_AUDIO_PREV);
								}
							} else if (IsMoveX(points[0].x_end, points[0].x_start, points[0].y_end, points[0].y_start)
									&& IsMoveX(points[1].x_end, points[1].x_start, points[1].y_end, points[1].y_start)) {
								if (points[0].x_end > points[0].x_start
										&& points[1].x_end > points[1].x_start) {
									PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
								} else if (points[0].x_end < points[0].x_start
										&& points[1].x_end < points[1].x_start) {
									PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
								}
							}
						}
					}
				}

				m_touchScreen.Empty();
			}
		}
	} else {
		m_touchScreen.Empty();
	}

	return TRUE;
}

LPCWSTR CMainFrame::GetTextForBar(int style)
{
	if (style == TEXTBAR_FILENAME) {
		return GetFileNameOrTitleOrPath();
	}
	if (style == TEXTBAR_TITLE) {
		return GetTitleOrFileNameOrPath();
	}
	if (style == TEXTBAR_FULLPATH) {
		return m_SessionInfo.Path.GetString();
	}

	return L"";
}

void CMainFrame::UpdateTitle()
{
	if (m_eMediaLoadState == MLS_LOADED && m_bUpdateTitle && m_pAMMC[0]) {
		for (const auto& pAMMC : m_pAMMC) {
			if (pAMMC) {
				CComBSTR bstr;
				if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
					if (m_SessionInfo.Title != bstr.m_str) {
						m_SessionInfo.Title = bstr.m_str;

						UpdateWindowTitle();
						m_wndSeekBar.Invalidate();

						m_wndInfoBar.RemoveAllLines();
						m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_TITLE), m_SessionInfo.Title);

						if (SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && bstr.Length()) {
							m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUTHOR), bstr.m_str);
							bstr.Empty();
						}
						if (SUCCEEDED(pAMMC->get_Copyright(&bstr)) && bstr.Length()) {
							m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_COPYRIGHT), bstr.m_str);
							bstr.Empty();
						}
						if (SUCCEEDED(pAMMC->get_Rating(&bstr)) && bstr.Length()) {
							m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_RATING), bstr.m_str);
							bstr.Empty();
						}
						if (SUCCEEDED(pAMMC->get_Description(&bstr)) && bstr.Length()) {
							// show only the first description line
							CString str(bstr.m_str);
							int k = str.Find(L"\r\n");
							if (k > 0) {
								str.Truncate(k);
								str.Append(L" \x2026");
							}
							m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DESCRIPTION), str);
							bstr.Empty();
						}
					}
					break;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	__super::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	// forward focus to the view window
	if (IsWindow(m_wndView.m_hWnd)) {
		m_wndView.SetFocus();
	}
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) {
		return TRUE;
	}

	for (const auto& pBar : m_bars) {
		if (pBar->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) {
			return TRUE;
		}
	}

	for (const auto& pDockingBar : m_dockingbars) {
		if (pDockingBar->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) {
			return TRUE;
		}
	}

	// otherwise, do default handling
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	const BOOL bMenuVisible = IsMainMenuVisible();

	// minimum video area size 32 x 32
	lpMMI->ptMinTrackSize.x = 32;
	if (m_eMediaLoadState == MLS_LOADED && (!m_bAudioOnly || AfxGetAppSettings().nAudioWindowMode == 1)) {
		// always show a video and album cover if its display is set in the settings
		lpMMI->ptMinTrackSize.y = 32;
	} else {
		lpMMI->ptMinTrackSize.y = 0;
	}

	CSize cSize;
	CalcControlsSize(cSize);
	lpMMI->ptMinTrackSize.x += cSize.cx;
	lpMMI->ptMinTrackSize.y += cSize.cy;

	if (bMenuVisible) {
		MENUBARINFO mbi = { sizeof(mbi) };
		GetMenuBarInfo(OBJID_MENU, 0, &mbi);

		// Calculate menu's horizontal length in pixels
		long x = GetSystemMetrics(SM_CYMENU) / 2; // free space after menu
		CRect r;
		for (UINT uItem = 0; ::GetMenuItemRect(m_hWnd, mbi.hMenu, uItem, &r); uItem++) {
			x += r.Width();
		}
		lpMMI->ptMinTrackSize.x = std::max(lpMMI->ptMinTrackSize.x, x);
	}

	if (IsWindow(m_wndToolBar.m_hWnd) && m_wndToolBar.IsVisible()) {
		lpMMI->ptMinTrackSize.x = std::max((LONG)m_wndToolBar.GetMinWidth(), lpMMI->ptMinTrackSize.x);
	}

	for (const auto& pDockingBar : m_dockingbars) {
		if (IsWindow(pDockingBar->m_hWnd)) {
			auto* pPlaylistBar = dynamic_cast<CPlayerPlaylistBar*>(pDockingBar);
			if (pPlaylistBar) {
				if (pPlaylistBar->IsPlaylistVisible() && !pPlaylistBar->IsFloating()) {
					lpMMI->ptMinTrackSize.y += pPlaylistBar->GetMinSize().cy;
				}
				break;
			}
		}
	}

	// Ensure that window decorations will fit
	CRect decorationsRect;
	VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), bMenuVisible, GetWindowExStyle(m_hWnd)));
	lpMMI->ptMinTrackSize.x += decorationsRect.Width();
	lpMMI->ptMinTrackSize.y += decorationsRect.Height();

	// Final phase
	lpMMI->ptMinTrackSize.x = std::max(lpMMI->ptMinTrackSize.x, (LONG)GetSystemMetrics(SM_CXMIN));
	lpMMI->ptMinTrackSize.y = std::max(lpMMI->ptMinTrackSize.y, (LONG)GetSystemMetrics(SM_CYMIN));

	lpMMI->ptMaxTrackSize.x = GetSystemMetrics(SM_CXVIRTUALSCREEN) + decorationsRect.Width();
	lpMMI->ptMaxTrackSize.y = GetSystemMetrics(SM_CYVIRTUALSCREEN)
							  + ((GetStyle() & WS_THICKFRAME) ? GetSystemMetrics(SM_CYSIZEFRAME) : 0);

	FlyBarSetPos();
	OSDBarSetPos();

	__super::OnGetMinMaxInfo(lpMMI);
}

void CMainFrame::CreateFlyBar()
{
	if (AfxGetAppSettings().fFlybar) {
		if (SUCCEEDED(m_wndFlyBar.Create(this))) {
			SetTimer(TIMER_FLYBARWINDOWHIDER, 250, nullptr);
		}
	}
}

bool CMainFrame::FlyBarSetPos()
{
	if (!m_wndFlyBar || !(::IsWindow(m_wndFlyBar.GetSafeHwnd()))) {
		return false;
	}

	if (m_bIsMadVRExclusiveMode || m_bIsMPCVRExclusiveMode || !m_wndView.IsWindowVisible()) {
		if (m_wndFlyBar.IsWindowVisible()) {
			m_wndFlyBar.ShowWindow(SW_HIDE);
		}
		return false;
	}

	if (m_pBFmadVR) {
		CComQIPtr<IMadVRSettings> pMVS = m_pBFmadVR.p;
		BOOL boolVal = FALSE;
		if (pMVS) {
			pMVS->SettingsGetBoolean(L"enableExclusive", &boolVal);
		}
		if (m_bFullScreen && boolVal) {
			if (m_wndFlyBar.IsWindowVisible()) {
				m_wndFlyBar.ShowWindow(SW_HIDE);
			}
			return false;
		}

		CComQIPtr<IMadVRInfo> pMVRInfo = m_pBFmadVR.p;
		if (pMVRInfo) {
			bool value = false;
			pMVRInfo->GetBool("ExclusiveModeActive", &value);
			if (value) {
				if (m_wndFlyBar.IsWindowVisible()) {
					m_wndFlyBar.ShowWindow(SW_HIDE);
				}
				return false;
			}
		}
	}

	CRect r_wndView;
	m_wndView.GetWindowRect(&r_wndView);

	const CAppSettings& s = AfxGetAppSettings();
	if (s.iCaptionMenuMode == MODE_FRAMEONLY || s.iCaptionMenuMode == MODE_BORDERLESS || m_bFullScreen) {

		int pos = (m_wndFlyBar.iw * 9) + 10;
		m_wndFlyBar.MoveWindow(r_wndView.right - 10 - pos, r_wndView.top + 10, pos, m_wndFlyBar.iw + 10);
		m_wndFlyBar.CalcButtonsRect();
		if (r_wndView.Height() > 40 && r_wndView.Width() > 236) {
			if (s.fFlybarOnTop && !m_wndFlyBar.IsWindowVisible()) {
				m_wndFlyBar.ShowWindow(SW_SHOWNOACTIVATE);
			}
			return true;
		} else {
			if (m_wndFlyBar.IsWindowVisible()) {
				m_wndFlyBar.ShowWindow(SW_HIDE);
			}
		}
	} else {
		if (m_wndFlyBar.IsWindowVisible()) {
			m_wndFlyBar.ShowWindow(SW_HIDE);
		}
	}

	return false;
}

void CMainFrame::DestroyFlyBar()
{
	if (m_wndFlyBar) {
		KillTimer(TIMER_FLYBARWINDOWHIDER);
		if (m_wndFlyBar.IsWindowVisible()) {
			m_wndFlyBar.ShowWindow(SW_HIDE);
		}
		m_wndFlyBar.DestroyWindow();
	}
}


void CMainFrame::CreateOSDBar()
{
	if (SUCCEEDED(m_OSD.Create(&m_wndView))) {
		m_pOSDWnd = &m_wndView;
	}
}

bool CMainFrame::OSDBarSetPos()
{
	if (!m_OSD || !(::IsWindow(m_OSD.GetSafeHwnd())) || m_OSD.GetOSDType() != OSD_TYPE_GDI) {
		return false;
	}

	if (m_pBFmadVR || !m_wndView.IsWindowVisible()) {
		if (m_OSD.IsWindowVisible()) {
			m_OSD.ShowWindow(SW_HIDE);
		}
		return false;
	}

	CRect r_wndView;
	m_wndView.GetWindowRect(&r_wndView);

	int pos = (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) ? (m_wndFlyBar.iw * 9 + 20) : 0;

	CRect MainWndRect;
	m_wndView.GetWindowRect(&MainWndRect);
	MainWndRect.right -= pos;
	m_OSD.SetWndRect(MainWndRect);
	if (m_OSD.IsWindowVisible()) {
		::PostMessageW(m_OSD.m_hWnd, WM_OSD_DRAW, WPARAM(0), LPARAM(0));
	}

	/*
	// we need to auto-hide or not ???
	if (r_wndView.Height() > 40 && r_wndView.Width() > 100) {
		m_OSD.EnableShowMessage();
		m_OSD.HideMessage(false);
		return true;
	} else {
		m_OSD.EnableShowMessage(false);
		m_OSD.HideMessage(true);
	}
	*/

	return false;
}

void CMainFrame::DestroyOSDBar()
{
	if (m_OSD) {
		m_OSD.Stop();
		m_OSD.DestroyWindow();
	}
}

void CMainFrame::OnEnterSizeMove()
{
	m_bWndZoomed = false;

	POINT cur_pos;
	RECT rcWindow;
	GetWindowRect(&rcWindow);
	GetCursorPos(&cur_pos);

	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	RECT rcWork = mi.rcWork;

	if (IsZoomed() // window is maximized
		|| (rcWindow.top == rcWork.top && rcWindow.bottom == rcWork.bottom) // window is aero snapped (???)
		|| m_bFullScreen) { // window is fullscreen

		m_bWndZoomed = true;
	}

	if (!m_bWndZoomed) {
		WINDOWPLACEMENT wp;
		GetWindowPlacement(&wp);
		RECT rcNormalPosition = wp.rcNormalPosition;
		snap_x = cur_pos.x - rcNormalPosition.left;
		snap_y = cur_pos.y - rcNormalPosition.top;
	}
}

BOOL CMainFrame::isSnapClose(int a, int b)
{
	snap_Margin = GetSystemMetrics(SM_CYCAPTION);
	return (abs(a - b) < snap_Margin);
}

void CMainFrame::OnMove(int x, int y)
{
	__super::OnMove(x, y);

	//MoveVideoWindow(); // This isn't needed, based on my limited tests. If it is needed then please add a description the scenario(s) where it is needed.
	m_wndView.Invalidate();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if (!m_bFirstFSAfterLaunchOnFullScreen && !m_bFullScreen
			&& IsWindowVisible() && wp.flags != WPF_RESTORETOMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED) {

		CAppSettings& s = AfxGetAppSettings();
		CRect rect;
		GetWindowRect(&rect);

		s.ptLastWindowPos = rect.TopLeft();

		if (m_bAudioOnly && IsSomethingLoaded() && s.nAudioWindowMode == 2) {
			s.szLastWindowSize.cx = rect.Width();
		} else {
			s.szLastWindowSize = rect.Size();
		}
	}

	if (m_wndToolBar && ::IsWindow(m_wndToolBar.GetSafeHwnd())) {
		m_wndToolBar.Invalidate();
	}

	FlyBarSetPos();
	OSDBarSetPos();
}

void CMainFrame::ClipRectToMonitor(LPRECT prc)
{
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	RECT rcNormalPosition = wp.rcNormalPosition;

	int w = rcNormalPosition.right - rcNormalPosition.left;
	int h = rcNormalPosition.bottom - rcNormalPosition.top;

	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoW(MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST), &mi);
	RECT rcWork = mi.rcWork;

	POINT cur_pos;
	GetCursorPos(&cur_pos);

	// by cursor position
	// prc->left   = max(rcWork.left, min(rcWork.right-w, cur_pos.x - (double)((rnp.right-rnp.left)/((double)(rcWork.right-rcWork.left)/cur_pos.x))));
	// prc->top    = max(rcWork.top,  min(rcWork.bottom-h, cur_pos.y - (double)((rnp.bottom-rnp.top)/((double)(rcWork.bottom-rcWork.top)/cur_pos.y))));

	prc->left   = std::max(rcWork.left, std::min(rcWork.right-w, cur_pos.x - (w/2)));
	prc->top    = std::max(rcWork.top,  std::min(rcWork.bottom-h, cur_pos.y - (h/2)));
	prc->right  = prc->left + w;
	prc->bottom = prc->top  + h;

	rc_forceNP.left		= prc->left;
	rc_forceNP.right	= prc->right;
	rc_forceNP.top		= prc->top;
	rc_forceNP.bottom	= prc->bottom;
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	m_bWasSnapped = false;
	const bool bCtrl = !!(GetAsyncKeyState(VK_CONTROL) & 0x80000000);

	POINT cur_pos;
	GetCursorPos(&cur_pos);

	if (m_bWndZoomed){
		ClipRectToMonitor(pRect);
		SetWindowPos(nullptr, pRect->left, pRect->top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		m_bWndZoomed = false;
		snap_x = cur_pos.x - pRect->left;
		snap_y = cur_pos.y - pRect->top;

		return;
	}

	if (AfxGetAppSettings().bSnapToDesktopEdges && !bCtrl) {

		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		CRect rcWork(mi.rcWork);
		if (SysVersion::IsWin10orLater()) {
			rcWork.InflateRect(GetInvisibleBorderSize());
		}

		RECT rcMonitor = mi.rcMonitor;

		OffsetRect(pRect, cur_pos.x - (pRect->left + snap_x), cur_pos.y - (pRect->top + snap_y));

		if (isSnapClose(pRect->left, rcWork.left)) { // left screen snap
			OffsetRect(pRect, rcWork.left - pRect->left, 0);
			m_bWasSnapped = true;
		} else if (isSnapClose(rcWork.right, pRect->right)) { // right screen snap
			OffsetRect(pRect, rcWork.right - pRect->right, 0);
			m_bWasSnapped = true;
		}

		if (isSnapClose(pRect->top, rcWork.top)) { // top screen snap
			OffsetRect(pRect, 0, rcWork.top - pRect->top);
			m_bWasSnapped = true;
		} else if (isSnapClose(rcWork.bottom, pRect->bottom)) { // bottom taskbar snap
			OffsetRect(pRect, 0, rcWork.bottom - pRect->bottom);
			m_bWasSnapped = true;
		} else if (isSnapClose(pRect->bottom, rcMonitor.bottom)) { // bottom screen snap
			OffsetRect(pRect, 0, rcMonitor.bottom - pRect->bottom);
			m_bWasSnapped = true;
		}

	}

	FlyBarSetPos();
	OSDBarSetPos();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_OSD && IsD3DFullScreenMode()) {
		m_OSD.OnSize(nType, cx, cy);
	}

	CAppSettings& s = AfxGetAppSettings();
	if (!m_bFirstFSAfterLaunchOnFullScreen && IsWindowVisible() && !m_bFullScreen) {
		if (nType != SIZE_MAXIMIZED && nType != SIZE_MINIMIZED) {
			CRect rect;
			GetWindowRect(&rect);

			s.ptLastWindowPos = rect.TopLeft();

			if (m_bAudioOnly && IsSomethingLoaded() && s.nAudioWindowMode == 2) {
				s.szLastWindowSize.cx = rect.Width();
			} else {
				s.szLastWindowSize = rect.Size();
			}
		}
		s.nLastWindowType = nType;

		// maximized window in MODE_FRAMEONLY | MODE_BORDERLESS is not correct
		if (nType == SIZE_MAXIMIZED && (s.iCaptionMenuMode == MODE_FRAMEONLY || s.iCaptionMenuMode == MODE_BORDERLESS)) {
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
			CRect wr(mi.rcWork);
			if (s.iCaptionMenuMode == MODE_FRAMEONLY && SysVersion::IsWin10orLater()) {
				wr.InflateRect(GetInvisibleBorderSize());
			}
			SetWindowPos(nullptr, wr.left, wr.top, wr.Width(), wr.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	if (nType != SIZE_MINIMIZED) {
		FlyBarSetPos();
		OSDBarSetPos();
	}

	if (nType == SIZE_MINIMIZED
			&& m_eMediaLoadState == MLS_LOADED
			&& s.bPauseMinimizedVideo
			&& !IsD3DFullScreenMode()) {
		OAFilterState fs = GetMediaState();
		if (fs == State_Running) {
			m_bWasPausedOnMinimizedVideo = true;
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}
	}

	if ((nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED)
			&& m_bWasPausedOnMinimizedVideo) {
		if (m_eMediaLoadState == MLS_LOADED) {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
		}
		m_bWasPausedOnMinimizedVideo = false;
	}

	if (nType == SIZE_RESTORED || nType == SIZE_MINIMIZED || nType == SIZE_MAXIMIZED) {
		m_bLeftMouseDown = FALSE;
	}
}

void CMainFrame::OnSizing(UINT nSide, LPRECT pRect)
{
	__super::OnSizing(nSide, pRect);

	const CAppSettings& s = AfxGetAppSettings();
	const bool bCtrl = !!(GetAsyncKeyState(VK_CONTROL) & 0x80000000);

	if (m_eMediaLoadState != MLS_LOADED || m_bFullScreen
			|| m_iVideoSize == DVS_STRETCH
			|| (bCtrl == s.bLimitWindowProportions)) {
		return;
	}

	const CSize videoSize = GetVideoSize();
	if (!videoSize.cx || !videoSize.cy) {
		return;
	}

	CSize decorationsSize;
	CRect decorationsRect;
	CalcControlsSize(decorationsSize);
	VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), IsMainMenuVisible(), GetWindowExStyle(m_hWnd)));
	decorationsSize += decorationsRect.Size();

	const CSize videoAreaSize(pRect->right - pRect->left - decorationsSize.cx, pRect->bottom - pRect->top - decorationsSize.cy);
	const bool bWider = videoAreaSize.cy < videoAreaSize.cx;

	// new proportional width and height of the window. only one of these values is used.
	long w = MulDiv(videoAreaSize.cy, videoSize.cx, videoSize.cy) + decorationsSize.cx;
	long h = MulDiv(videoAreaSize.cx, videoSize.cy, videoSize.cx) + decorationsSize.cy;

	if (nSide == WMSZ_TOP || nSide == WMSZ_BOTTOM || (!bWider && (nSide == WMSZ_TOPRIGHT || nSide == WMSZ_BOTTOMRIGHT))) {
		pRect->right = pRect->left + w;
	}
	else if (nSide == WMSZ_LEFT || nSide == WMSZ_RIGHT || (bWider && (nSide == WMSZ_BOTTOMLEFT || nSide == WMSZ_BOTTOMRIGHT))) {
		pRect->bottom = pRect->top + h;
	}
	else if (!bWider && (nSide == WMSZ_TOPLEFT || nSide == WMSZ_BOTTOMLEFT)) {
		pRect->left = pRect->right - w;
	}
	else if (bWider && (nSide == WMSZ_TOPLEFT || nSide == WMSZ_TOPRIGHT)) {
		pRect->top = pRect->bottom - h;
	}

	FlyBarSetPos();
	OSDBarSetPos();

#if _DEBUG
	CString msg;
	msg.Format(L"W x H = %d x %d", pRect->right - pRect->left - decorationsSize.cx, pRect->bottom - pRect->top - decorationsSize.cy);
	SetStatusMessage(msg);
#endif
}

void CMainFrame::OnDisplayChange() // untested, not sure if it's working...
{
	DLog(L"CMainFrame::OnDisplayChange() : start");
	if (m_eMediaLoadState == MLS_LOADED) {
		if (m_pGraphThread) {
			CAMMsgEvent e;
			m_pGraphThread->PostThreadMessage(CGraphThread::TM_DISPLAY_CHANGE, 0, (LPARAM)&e);
			e.WaitMsg();
		} else {
			DisplayChange();
		}
	}

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);
	if (m_bFullScreen || IsD3DFullScreenMode()) {
		CWnd* cwnd = m_bFullScreen ? this : static_cast<CWnd*>(m_pFullscreenWnd);

		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		HMONITOR hMonitor = MonitorFromWindow(cwnd->m_hWnd, MONITOR_DEFAULTTONULL);
		if (GetMonitorInfoW(hMonitor, &MonitorInfo)) {
			CRect MonitorRect = CRect(MonitorInfo.rcMonitor);
			cwnd->SetWindowPos(nullptr,
							   MonitorRect.left,
							   MonitorRect.top,
							   MonitorRect.Width(),
							   MonitorRect.Height(),
							   SWP_NOZORDER);
			MoveVideoWindow();
		}
	}

	DLog(L"CMainFrame::OnDisplayChange() : end");
}

void CMainFrame::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if (m_bFullScreen && !(lpwndpos->flags & SWP_NOMOVE)) {
		const HMONITOR hm = MonitorFromPoint(CPoint(lpwndpos->x, lpwndpos->y), MONITOR_DEFAULTTONULL);
		MONITORINFO mi = { sizeof(mi) };
		if (GetMonitorInfoW(hm, &mi)) {
			lpwndpos->flags &= ~SWP_NOSIZE;
			lpwndpos->cx = mi.rcMonitor.right - mi.rcMonitor.left;
			lpwndpos->cy = mi.rcMonitor.bottom - mi.rcMonitor.top;
			lpwndpos->x = mi.rcMonitor.left;
			lpwndpos->y = mi.rcMonitor.top;
		}
	}

	__super::OnWindowPosChanging(lpwndpos);
}

BOOL CMainFrame::OnQueryEndSession()
{
	CloseMedia();

	return TRUE;
}

LRESULT CMainFrame::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	const int dpix = LOWORD(wParam);
	const int dpiy = HIWORD(wParam);
	OverrideDPI(dpix, dpiy);
	if (m_OSD) {
		m_OSD.OverrideDPI(dpix, dpiy);
	}

	m_wndToolBar.ScaleToolbar();
	m_wndInfoBar.ScaleFont();
	m_wndStatsBar.ScaleFont();
	m_wndSeekBar.ScaleFont();
	m_wndPlaylistBar.ScaleFont();
	m_wndStatusBar.ScaleFont();
	m_wndPreView.ScaleFont();
	m_wndFlyBar.Scale();

	CMenuEx::ScaleFont();
	const auto& s = AfxGetAppSettings();
	if (s.bUseDarkTheme && s.bDarkMenu) {
		m_popupMenu.DestroyMenu();
		m_popupMainMenu.DestroyMenu();
		m_VideoFrameMenu.DestroyMenu();
		m_PanScanMenu.DestroyMenu();
		m_ShadersMenu.DestroyMenu();
		m_AfterPlaybackMenu.DestroyMenu();
		m_NavigateMenu.DestroyMenu();

		m_popupMenu.LoadMenuW(IDR_POPUP);
		m_popupMainMenu.LoadMenuW(IDR_POPUPMAIN);
		m_VideoFrameMenu.LoadMenuW(IDR_POPUP_VIDEOFRAME);
		m_PanScanMenu.LoadMenuW(IDR_POPUP_PANSCAN);
		m_ShadersMenu.LoadMenuW(IDR_POPUP_SHADERS);
		m_AfterPlaybackMenu.LoadMenuW(IDR_POPUP_AFTERPLAYBACK);
		m_NavigateMenu.LoadMenuW(IDR_POPUP_NAVIGATE);

		CMenu defaultMenu;
		defaultMenu.LoadMenuW(IDR_MAINFRAME);
		CMenu* oldMenu = GetMenu();
		if (oldMenu) {
			// Attach the new menu to the window only if there was a menu before
			SetMenu(&defaultMenu);
			// and then destroy the old one
			oldMenu->DestroyMenu();
		}
		m_hMenuDefault = defaultMenu.Detach();

		SetColorMenu();
	}

	if (!m_bFullScreenChangingMode) {
		MoveWindow(reinterpret_cast<RECT*>(lParam));
	}
	RecalcLayout();

	return 0;
}

LRESULT CMainFrame::OnDwmCompositionChanged(WPARAM wParam, LPARAM lParam)
{
	if (!SysVersion::IsWin8orLater()) {
		DwmIsCompositionEnabled(&m_bDesktopCompositionEnabled);
	}

	return 0;
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	// Only stop screensaver if video playing; allow for audio only
	if ((GetMediaState() == State_Running && !m_bAudioOnly) && (((nID & 0xFFF0) == SC_SCREENSAVE) || ((nID & 0xFFF0) == SC_MONITORPOWER))) {
		DLog(L"SC_SCREENSAVE, nID = %d, lParam = %d", nID, lParam);
		return;
	} else if ((nID & 0xFFF0) == SC_MINIMIZE && m_bTrayIcon) {
		if (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) {
			m_wndFlyBar.ShowWindow(SW_HIDE);
		}

		m_dockingbarsVisible.clear();
		for (const auto& pDockingBar : m_dockingbars) {
			if (IsWindow(pDockingBar->m_hWnd) && pDockingBar->IsFloating() && pDockingBar->IsWindowVisible()) {
				ShowControlBar(pDockingBar, FALSE, FALSE);
				m_dockingbarsVisible.emplace_back(pDockingBar);
			}
		}
		ShowWindow(SW_HIDE);
		return;
	} else if ((nID & 0xFFF0) == SC_MINIMIZE) {
		if (IsSomethingLoaded() && m_bAudioOnly && m_pfnDwmSetIconicLivePreviewBitmap) {
			m_isWindowMinimized = true;
			CreateCaptureWindow();
		}
	} else if ((nID & 0xFFF0) == SC_RESTORE) {
		if (m_bAudioOnly && m_pfnDwmSetIconicLivePreviewBitmap) {
			m_isWindowMinimized = false;
			if (m_CaptureWndBitmap) {
				DeleteObject(m_CaptureWndBitmap);
				m_CaptureWndBitmap = nullptr;
			}
		}
	} else if ((nID & 0xFFF0) == SC_MAXIMIZE && m_bFullScreen) {
		ToggleFullscreen(true, true);
	}

	__super::OnSysCommand(nID, lParam);

	if ((nID & 0xFFF0) == SC_RESTORE) {
		RepaintVideo();
		StartAutoHideCursor();
	}
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	__super::OnActivateApp(bActive, dwThreadID);

	if (bActive) {
		if (!m_bHideCursor) {
			StartAutoHideCursor();
		}
		m_lastMouseMove.x = m_lastMouseMove.y = -1;
		RepaintVideo();
	} else {
		if (!m_bHideCursor) {
			StopAutoHideCursor();
		}
	}

	if (m_bFullScreen) {
		if (bActive) {
			if (!m_bInOptions) {
				SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
		} else {
			const CAppSettings& s = AfxGetAppSettings();
			const int i = s.iOnTop;
			bool bTopMost = false;
			if (i == 0) {
				bTopMost = false;
			} else if (i == 1) {
				bTopMost = true;
			} else if (i == 2) {
				bTopMost = (GetMediaState() == State_Running) ? true : false;
			} else {
				bTopMost = (GetMediaState() == State_Running && !m_bAudioOnly) ? true : false;
			}

			if (bTopMost) {
				return;
			}

			const auto GetForegroundCWnd = []() -> CWnd* {
				for (unsigned i = 0; i < 20; i++) {
					if (auto pActiveCWnd = GetForegroundWindow()) {
						return pActiveCWnd;
					}

					Sleep(10);
				}

				return nullptr;
			};

			if (auto pActiveCWnd = GetForegroundCWnd()) {
				bool bExitFullscreen = s.fExitFullScreenAtFocusLost;
				if (bExitFullscreen) {
					HMONITOR hMonitor1 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
					HMONITOR hMonitor2 = MonitorFromWindow(pActiveCWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
					if (hMonitor1 && hMonitor2 && hMonitor1 != hMonitor2) {
						bExitFullscreen = false;
					}
				}

				CString title;
				pActiveCWnd->GetWindowTextW(title);

				CString module;

				DWORD pid;
				GetWindowThreadProcessId(pActiveCWnd->m_hWnd, &pid);

				if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
					HMODULE hModule;
					DWORD cbNeeded;

					if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
						module.ReleaseBufferSetLength(GetModuleFileNameExW(hProcess, hModule, module.GetBuffer(MAX_PATH), MAX_PATH));
					}

					CloseHandle(hProcess);
				}

				if (!title.IsEmpty() && !module.IsEmpty()) {
					CString str;
					str.Format(ResStr(IDS_MAINFRM_2), GetFileName(module).MakeLower(), title);
					SendStatusMessage(str, 5000);
				}

				if (bExitFullscreen) {
					ToggleFullscreen(true, true);
				} else {
					SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				}
			}
		}
	}
}

LRESULT CMainFrame::OnAppCommand(WPARAM wParam, LPARAM lParam)
{
	UINT cmd  = GET_APPCOMMAND_LPARAM(lParam);
	UINT uDevice = GET_DEVICE_LPARAM(lParam);

	if (uDevice != FAPPCOMMAND_OEM ||
			cmd == APPCOMMAND_MEDIA_PLAY || cmd == APPCOMMAND_MEDIA_PAUSE || cmd == APPCOMMAND_MEDIA_CHANNEL_UP ||
			cmd == APPCOMMAND_MEDIA_CHANNEL_DOWN || cmd == APPCOMMAND_MEDIA_RECORD ||
			cmd == APPCOMMAND_MEDIA_FAST_FORWARD || cmd == APPCOMMAND_MEDIA_REWIND ) {
		const CAppSettings& s = AfxGetAppSettings();

		BOOL fRet = FALSE;

		for (const auto& wc : s.wmcmds) {
			if (wc.appcmd == cmd && TRUE == SendMessageW(WM_COMMAND, wc.cmd)) {
				fRet = TRUE;
			}
		}

		if (fRet) {
			return TRUE;
		}
	}

	return Default();
}

void CMainFrame::OnRawInput(UINT nInputcode, HRAWINPUT hRawInput)
{
	const CAppSettings& s = AfxGetAppSettings();
	UINT nMceCmd = 0;

	nMceCmd = AfxGetMyApp()->GetRemoteControlCode(nInputcode, hRawInput);
	switch (nMceCmd) {
		case MCE_DETAILS :
		case MCE_GUIDE :
		case MCE_TVJUMP :
		case MCE_STANDBY :
		case MCE_OEM1 :
		case MCE_OEM2 :
		case MCE_MYTV :
		case MCE_MYVIDEOS :
		case MCE_MYPICTURES :
		case MCE_MYMUSIC :
		case MCE_RECORDEDTV :
		case MCE_DVDANGLE :
		case MCE_DVDAUDIO :
		case MCE_DVDMENU :
		case MCE_DVDSUBTITLE :
		case MCE_RED :
		case MCE_GREEN :
		case MCE_YELLOW :
		case MCE_BLUE :
		case MCE_MEDIA_NEXTTRACK :
		case MCE_MEDIA_PREVIOUSTRACK :
			for (const auto& wc : s.wmcmds) {
				if (wc.appcmd == nMceCmd) {
					SendMessageW(WM_COMMAND, wc.cmd);
					break;
				}
			}
			break;
	}
}

LRESULT CMainFrame::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	const CAppSettings& s = AfxGetAppSettings();
	BOOL fRet = FALSE;

	if (GetActiveWindow() == this || s.bGlobalMedia == true) {
		for (const auto& wc : s.wmcmds) {
			if (wc.appcmd == wParam && TRUE == SendMessageW(WM_COMMAND, wc.cmd)) {
				fRet = TRUE;
			}
		}
	}

	return fRet;
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent) {
		case TIMER_FLYBARWINDOWHIDER:
			if (!m_bInMenu) {
				if (m_wndView &&
							(AfxGetAppSettings().iCaptionMenuMode == MODE_FRAMEONLY
							|| AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS
							|| m_bFullScreen)) {

					CPoint p;
					GetCursorPos(&p);

					CRect r_wndView, r_wndFlybar, r_ShowFlybar;
					m_wndView.GetWindowRect(&r_wndView);
					m_wndFlyBar.GetWindowRect(&r_wndFlybar);

					r_ShowFlybar = r_wndView;
					r_ShowFlybar.bottom = r_wndFlybar.bottom + r_wndFlybar.Height();

					if (FlyBarSetPos() == 0) break;

					if (r_ShowFlybar.PtInRect(p) && !m_bHideCursor && !m_wndFlyBar.IsWindowVisible()) {
						m_wndFlyBar.ShowWindow(SW_SHOWNOACTIVATE);
					} else if (!r_ShowFlybar.PtInRect(p) && m_wndFlyBar.IsWindowVisible() && !AfxGetAppSettings().fFlybarOnTop) {
						m_wndFlyBar.ShowWindow(SW_HIDE);
					}
					OSDBarSetPos();

				} else if (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) {
					m_wndFlyBar.ShowWindow(SW_HIDE);
					OSDBarSetPos();
				}
			}
			break;
		case TIMER_STREAMPOSPOLLER:
			if (m_eMediaLoadState == MLS_LOADED) {
				if (m_bFullScreen) {
					// Get the position of the cursor outside the window in fullscreen mode
					POINT pos;
					GetCursorPos(&pos);
					ScreenToClient(&pos);
					CRect r;
					GetClientRect(r);

					if (!r.PtInRect(pos) && (m_lastMouseMoveFullScreen.x != pos.x || m_lastMouseMoveFullScreen.y != pos.y)) {
						m_lastMouseMoveFullScreen.x = pos.x;
						m_lastMouseMoveFullScreen.y = pos.y;
						OnMouseMove(0, pos);
					}
				}

				g_bForcedSubtitle = AfxGetAppSettings().fForcedSubtitles;
				REFERENCE_TIME rtNow = 0, rtDur = 0;

				switch (GetPlaybackMode()) {
					case PM_FILE:
						g_bExternalSubtitleTime = false;

						m_pMS->GetDuration(&rtDur);
						m_pMS->GetCurrentPosition(&rtNow);

						if (m_abRepeatPositionBEnabled && rtNow >= m_abRepeatPositionB) {
							PerformABRepeat();
							return;
						}

						// autosave subtitle sync after play
						if ((m_nCurSubtitle >= 0) && (m_rtCurSubPos != rtNow)) {
							if (m_lSubtitleShift != 0) {
								if (m_wndSubresyncBar.SaveToDisk()) {
									m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_AG_SUBTITLES_SAVED), 500);
								} else {
									m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_MAINFRM_4));
								}
							}
							m_nCurSubtitle   = -1;
							m_lSubtitleShift = 0;
						}
						m_wndStatusBar.SetStatusTimer(rtNow, rtDur, m_bShowMilliSecs || m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
						break;
					case PM_DVD:
						g_bExternalSubtitleTime = true;
						if (m_pDVDI) {
							DVD_PLAYBACK_LOCATION2 Location;
							if (m_pDVDI->GetCurrentLocation(&Location) == S_OK) {
								double fps = Location.TimeCodeFlags == DVD_TC_FLAG_25fps ? 25.0
												: Location.TimeCodeFlags == DVD_TC_FLAG_30fps ? 30.0
												: Location.TimeCodeFlags == DVD_TC_FLAG_DropFrame ? 30 / 1.001
												: 25.0;

								rtNow = HMSF2RT(Location.TimeCode, fps);

								if (m_abRepeatPositionBEnabled && rtNow >= m_abRepeatPositionB) {
									PerformABRepeat();
									return;
								}

								DVD_HMSF_TIMECODE tcDur;
								ULONG ulFlags;
								if (SUCCEEDED(m_pDVDI->GetTotalTitleTime(&tcDur, &ulFlags))) {
									rtDur = HMSF2RT(tcDur, fps);
								}
								if (m_pSubClock) {
									m_pSubClock->SetTime(rtNow);
								}
							}
						}
						m_wndStatusBar.SetStatusTimer(rtNow, rtDur, m_bShowMilliSecs || m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
						break;
					case PM_CAPTURE:
						g_bExternalSubtitleTime = true;
						if (m_bCapturing) {
							if (m_wndCaptureBar.m_capdlg.m_pMux) {
								CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux.p;
								if (!pMuxMS || FAILED(pMuxMS->GetCurrentPosition(&rtNow))) {
									m_pMS->GetCurrentPosition(&rtNow);
								}
							}
							if (m_rtDurationOverride >= 0) {
								rtDur = m_rtDurationOverride;
							}
						}
						break;
				}

				const GUID timeFormat = GetTimeFormat();
				g_bNoDuration = ((timeFormat == TIME_FORMAT_MEDIA_TIME ) && rtDur < UNITS/2) || rtDur <= 0;
				m_wndSeekBar.Enable(!g_bNoDuration);
				m_wndSeekBar.SetRange(rtDur);
				m_wndSeekBar.SetPos(rtNow);
				m_OSD.SetPosAndRange(rtNow, rtDur);
				m_Lcd.SetMediaRange(0, rtDur);
				m_Lcd.SetMediaPos(rtNow);

				if (m_pCAP) {
					if (g_bExternalSubtitleTime) {
						m_pCAP->SetTime(rtNow);
					}
					m_wndSubresyncBar.SetTime(rtNow);
					m_wndSubresyncBar.SetFPS(m_pCAP->GetFPS());
				}

				if (m_pfnDwmInvalidateIconicBitmaps) {
					m_pfnDwmInvalidateIconicBitmaps(m_hWnd);
				}
			}
			break;
		case TIMER_STREAMPOSPOLLER2:
			if (m_eMediaLoadState == MLS_LOADED) {
				auto& s = AfxGetAppSettings();
				if (s.nCS < CS_STATUSBAR) {
					s.bStatusBarIsVisible = false;
				} else {
					s.bStatusBarIsVisible = true;
				}

				if (GetPlaybackMode() == PM_CAPTURE && !m_bCapturing) {
					CString str = ResStr(IDS_CAPTURE_LIVE);

					long lChannel = 0, lVivSub = 0, lAudSub = 0;
					if (m_pAMTuner
							&& m_wndCaptureBar.m_capdlg.IsTunerActive()
							&& SUCCEEDED(m_pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub))) {
						CString ch;
						ch.Format(L" (ch%d)", lChannel);
						str += ch;
					}

					m_wndStatusBar.SetStatusTimer(str);
				} else {
					auto FormatString = [](CString& strOSD, LPCWSTR message) {
						if (strOSD.IsEmpty()) {
							strOSD = message;
						} else {
							strOSD.AppendFormat(L"\n%s", message);
						}
					};

					CString strOSD;
					if (s.bOSDLocalTime) {
						FormatString(strOSD, GetSystemLocalTime());
					}

					if (s.bOSDRemainingTime) {
						FormatString(strOSD, m_wndStatusBar.GetStatusTimer());
					}

					if (s.bOSDFileName) {
						FormatString(strOSD, GetFileNameOrTitleOrPath());
					}

					if (m_pBFmadVR) {
						strOSD.Replace(L"\n", L" / "); // MadVR support only singleline OSD message
					}

					if (!strOSD.IsEmpty()) {
						m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 1000, true);
					}
				}
			}
			break;
		case TIMER_FULLSCREENCONTROLBARHIDER: {
			CPoint p;
			GetCursorPos(&p);

			CRect r;
			GetWindowRect(r);
			bool fCursorOutside = !r.PtInRect(p);

			CWnd* pWnd = WindowFromPoint(p);
			if (pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside)) {
				if (AfxGetAppSettings().nShowBarsWhenFullScreenTimeOut >= 0) {
					ShowControls(CS_NONE, false);
				}
			}
		}
		break;
		case TIMER_MOUSEHIDER: {
			if (!m_bInOptions && !m_bInMenu) {
				CPoint p;
				GetCursorPos(&p);

				CWnd* pWnd = WindowFromPoint(p);

				if (IsD3DFullScreenMode()) {
					if (*pWnd == *m_pFullscreenWnd) {
						StopAutoHideCursor();
						m_pFullscreenWnd->ShowCursor(false);
					}
				} else {
					if (pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd))) {
						m_bHideCursor = true;
						SetCursor(nullptr);
					}
				}

				KillTimer(TIMER_MOUSEHIDER);
			}
		}
		break;
		case TIMER_STATS:
			UpdateTitle();
			if (m_wndStatsBar.IsWindowVisible()) {
				if (m_pQP) {
					CString rate;
					rate.Format(L" (%sx)", Rate2String(m_PlaybackRate));

					CString info;
					int val = 0;

					m_pQP->get_AvgFrameRate(&val); // We hang here due to a lock that never gets released.
					info.Format(L"%d.%02d %s", val / 100, val % 100, rate);
					m_wndStatsBar.SetLine(ResStr(IDS_AG_FRAMERATE), info);

					int avg, dev;
					m_pQP->get_AvgSyncOffset(&avg);
					m_pQP->get_DevSyncOffset(&dev);
					info.Format(ResStr(IDS_STATSBAR_SYNC_OFFSET_FORMAT), avg, dev);
					m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_SYNC_OFFSET), info);

					int drawn, dropped;
					m_pQP->get_FramesDrawn(&drawn);
					m_pQP->get_FramesDroppedInRenderer(&dropped);
					info.Format(ResStr(IDS_MAINFRM_6), drawn, dropped);
					m_wndStatsBar.SetLine(ResStr(IDS_AG_FRAMES), info);

					m_pQP->get_Jitter(&val);
					info.Format(L"%d ms", val);
					m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_JITTER), info);
				}

				if (GetPlaybackMode() != PM_DVD) {
					std::list<CComQIPtr<IBaseFilter>> pBFs;
					std::list<CComQIPtr<IBaseFilter>> pBFsVideo;
					BeginEnumFilters(m_pGB, pEF, pBF) {
						if (CComQIPtr<IBufferInfo> pBI = pBF.p) {
							bool bHasVideo = false;
							BeginEnumPins(pBF, pEP, pPin) {
								CMediaType mt;
								PIN_DIRECTION dir;
								if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
										|| FAILED(pPin->ConnectionMediaType(&mt))) {
									continue;
								}
								if (mt.majortype == MEDIATYPE_Video || mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
									bHasVideo = true;
									break;
								}
							}
							EndEnumPins;

							if (bHasVideo) {
								pBFsVideo.emplace_back(pBF);
							} else {
								pBFs.emplace_front(pBF);
							}
						}
					}
					EndEnumFilters;

					for (const auto& pBF : pBFsVideo) {
						pBFs.emplace_front(pBF);
					}

					if (!pBFs.empty()) {
						std::list<CString> sl;
						int cnt = 0;
						for (const auto& pBF : pBFs) {
							CComQIPtr<IBufferInfo> pBI = pBF.p;
							for (int i = 0, j = pBI->GetCount(); i < j; i++) {
								int samples, size;
								if (S_OK == pBI->GetStatus(i, samples, size)) {
									CString str;
									str.Format(L"[%d]: %03d/%d KB", cnt, samples, size / 1024);
									sl.emplace_back(str);
								}

								cnt++;
							}
						}

						if (!sl.empty()) {
							m_wndStatsBar.SetLine(ResStr(IDS_AG_BUFFERS), Implode(sl, L' '));
						}

						std::list<CComQIPtr<IBitRateInfo>> pBRIs;
						for (const auto& pBF : pBFs) {
							BeginEnumPins(pBF, pEP, pPin) {
								if (CComQIPtr<IBitRateInfo> pBRI = pPin.p) {
									pBRIs.emplace_back(pBRI);
								}
							}
							EndEnumPins;
						}

						if (!pBRIs.empty()) {
							sl.clear();
							cnt = 0;
							for (const auto& pBRI : pBRIs) {
								const DWORD cur = pBRI->GetCurrentBitRate() / 1000;
								const DWORD avg = pBRI->GetAverageBitRate() / 1000;

								CString str;
								if (cur != avg) {
									str.Format(L"[%d]: %u/%u Kb/s", cnt, avg, cur);
								} else {
									str.Format(L"[%d]: %u Kb/s", cnt, avg);
								}
								sl.emplace_back(str);

								cnt++;
							}

							if (!sl.empty()) {
								m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_BITRATE), Implode(sl, L' ') + ResStr(IDS_STATSBAR_BITRATE_AVG_CUR));
							}
						}
					}
				}
			}

			if (m_wndInfoBar.IsWindowVisible() && GetPlaybackMode() == PM_DVD) { // we also use this timer to update the info panel for DVD playback
				ULONG ulAvailable, ulCurrent;

				// Location

				CString Location('-');

				DVD_PLAYBACK_LOCATION2 loc;
				ULONG ulNumOfVolumes, ulVolume;
				DVD_DISC_SIDE Side;
				ULONG ulNumOfTitles;
				ULONG ulNumOfChapters;

				if (SUCCEEDED(m_pDVDI->GetCurrentLocation(&loc))
						&& SUCCEEDED(m_pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters))
						&& SUCCEEDED(m_pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles))) {
					Location.Format(ResStr(IDS_MAINFRM_9),
									ulVolume, ulNumOfVolumes,
									loc.TitleNum, ulNumOfTitles,
									loc.ChapterNum, ulNumOfChapters);
					ULONG tsec = (loc.TimeCode.bHours*3600)
								 + (loc.TimeCode.bMinutes*60)
								 + (loc.TimeCode.bSeconds);
					/* This might not always work, such as on resume */
					if ( loc.ChapterNum != m_lCurrentChapter ) {
						m_lCurrentChapter = loc.ChapterNum;
						m_lChapterStartTime = tsec;
					} else {
						/* If a resume point was used, and the user chapter jumps,
						then it might do some funky time jumping.  Try to 'fix' the
						chapter start time if this happens */
						if ( m_lChapterStartTime > tsec ) {
							m_lChapterStartTime = tsec;
						}
					}
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_LOCATION), Location);

				// Video

				CString Video('-');

				DVD_VideoAttributes VATR;

				if (SUCCEEDED(m_pDVDI->GetCurrentAngle(&ulAvailable, &ulCurrent))
						&& SUCCEEDED(m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
					Video.Format(ResStr(IDS_MAINFRM_10),
								 ulCurrent, ulAvailable,
								 VATR.ulSourceResolutionX, VATR.ulSourceResolutionY, VATR.ulFrameRate,
								 VATR.ulAspectX, VATR.ulAspectY);
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_VIDEO), Video);

				// Audio

				CString Audio('-');

				DVD_AudioAttributes AATR;

				if (SUCCEEDED(m_pDVDI->GetCurrentAudio(&ulAvailable, &ulCurrent))
						&& SUCCEEDED(m_pDVDI->GetAudioAttributes(ulCurrent, &AATR))) {
					CString lang;
					if (AATR.Language) {
						int len = GetLocaleInfoW(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
						lang.ReleaseBufferSetLength(std::max(len-1, 0));
					} else {
						lang.Format(ResStr(IDS_AG_UNKNOWN), ulCurrent+1);
					}

					switch (AATR.LanguageExtension) {
						case DVD_AUD_EXT_NotSpecified:
						default:
							break;
						case DVD_AUD_EXT_Captions:
							lang += L" (Captions)";
							break;
						case DVD_AUD_EXT_VisuallyImpaired:
							lang += L" (Visually Impaired)";
							break;
						case DVD_AUD_EXT_DirectorComments1:
							lang += L" (Director Comments 1)";
							break;
						case DVD_AUD_EXT_DirectorComments2:
							lang += L" (Director Comments 2)";
							break;
					}

					CString format = GetDVDAudioFormatName(AATR);

					Audio.Format(ResStr(IDS_MAINFRM_11),
								 lang,
								 format,
								 AATR.dwFrequency,
								 AATR.bQuantization,
								 AATR.bNumberOfChannels,
								 (AATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));

					m_wndStatusBar.SetStatusBitmap(
						AATR.bNumberOfChannels == 1 ? IDB_MONO
						: AATR.bNumberOfChannels >= 2 ? IDB_STEREO
						: IDB_NOAUDIO);
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUDIO), Audio);

				// Subtitles

				CString Subtitles('-');

				BOOL bIsDisabled;
				DVD_SubpictureAttributes SATR;

				if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulAvailable, &ulCurrent, &bIsDisabled))
						&& SUCCEEDED(m_pDVDI->GetSubpictureAttributes(ulCurrent, &SATR))) {
					CString lang;
					int len = GetLocaleInfoW(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(std::max(len-1, 0));

					switch (SATR.LanguageExtension) {
						case DVD_SP_EXT_NotSpecified:
						default:
							break;
						case DVD_SP_EXT_Caption_Normal:
							lang += L"";
							break;
						case DVD_SP_EXT_Caption_Big:
							lang += L" (Big)";
							break;
						case DVD_SP_EXT_Caption_Children:
							lang += L" (Children)";
							break;
						case DVD_SP_EXT_CC_Normal:
							lang += L" (CC)";
							break;
						case DVD_SP_EXT_CC_Big:
							lang += L" (CC Big)";
							break;
						case DVD_SP_EXT_CC_Children:
							lang += L" (CC Children)";
							break;
						case DVD_SP_EXT_Forced:
							lang += L" (Forced)";
							break;
						case DVD_SP_EXT_DirectorComments_Normal:
							lang += L" (Director Comments)";
							break;
						case DVD_SP_EXT_DirectorComments_Big:
							lang += L" (Director Comments, Big)";
							break;
						case DVD_SP_EXT_DirectorComments_Children:
							lang += L" (Director Comments, Children)";
							break;
					}

					if (bIsDisabled) {
						lang = L"-";
					}

					Subtitles.Format(L"%s", lang);
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), Subtitles);
			}
		break;
		case TIMER_STATUSERASER: {
			KillTimer(TIMER_STATUSERASER);
			m_playingmsg.Empty();
		}
		break;
		case TIMER_DM_AUTOCHANGING: {
			KillTimer(TIMER_DM_AUTOCHANGING);

			ASSERT(GetMediaState() != State_Stopped);
			OnPlayPlay();
		}
		break;
		case TIMER_PAUSE: {
			KillTimer(TIMER_PAUSE);
			SaveHistory();
		}
		break;
	}

	__super::OnTimer(nIDEvent);
}

bool CMainFrame::DoAfterPlaybackEvent()
{
	const CAppSettings& s = AfxGetAppSettings();

	bool bExit = (s.nCLSwitches & CLSW_CLOSE) || s.fExitAfterPlayback;

	if (s.nCLSwitches & CLSW_STANDBY) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		SetSystemPowerState(TRUE, FALSE);
		bExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	} else if (s.nCLSwitches & CLSW_HIBERNATE) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		SetSystemPowerState(FALSE, FALSE);
		bExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	} else if (s.nCLSwitches & CLSW_SHUTDOWN) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		UINT uFlags = EWX_SHUTDOWN | EWX_POWEROFF | EWX_FORCEIFHUNG;
		if (SysVersion::IsWin8orLater()) {
			uFlags |= EWX_HYBRID_SHUTDOWN;
		}
		ExitWindowsEx(uFlags, 0);
		bExit = true;
	} else if (s.nCLSwitches & CLSW_LOGOFF) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG, 0);
		bExit = true;
	} else if (s.nCLSwitches & CLSW_LOCK) {
		LockWorkStation();
	}

	if (bExit) {
		SendMessageW(WM_COMMAND, ID_FILE_EXIT);
	} else if (s.bCloseFileAfterPlayback) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		return true;
	}

	return bExit;
}

//
// graph event EC_COMPLETE handler
//
bool CMainFrame::GraphEventComplete()
{
	m_bGraphEventComplete = true;

	CAppSettings& s = AfxGetAppSettings();

	if (m_abRepeatPositionAEnabled || m_abRepeatPositionBEnabled) {
		PerformABRepeat();
	}
	else if (m_wndPlaylistBar.GetCount(true) <= 1) {
		m_nLoops++;

		if (DoAfterPlaybackEvent()) {
			return false;
		}

		if (s.fLoopForever || m_nLoops < s.nLoops) {
			if (GetMediaState() == State_Stopped) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			} else {
				LONGLONG pos = 0;
				m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);

				if (GetMediaState() == State_Paused) {
					SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
				}
			}
		} else {
			int NextMediaExist = 0;
			if (s.fNextInDirAfterPlayback) {
				NextMediaExist = SearchInDir(true);
			}
			if (!s.fNextInDirAfterPlayback || !(NextMediaExist > 1)) {
				m_bEndOfStream = true;
				if (s.fRewind) {
					SendMessageW(WM_COMMAND, ID_PLAY_STOP);
				} else {
					SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
				}

				if ((m_bFullScreen || IsD3DFullScreenMode()) && s.fExitFullScreenAtTheEnd) {
					OnViewFullscreen();
				}
			}
			if (s.fNextInDirAfterPlayback && !NextMediaExist) {
				m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_NO_MORE_MEDIA));
				// Don't move it. Else OSD message "Pause" will rewrite this message.
			}
		}
	} else {
		if (m_wndPlaylistBar.IsAtEnd()) {
			if (DoAfterPlaybackEvent()) {
				return false;
			}

			m_nLoops++;
		}

		if (s.fLoopForever || m_nLoops < s.nLoops || m_wndPlaylistBar.IsShuffle()) {
			int nLoops = m_nLoops;
			SendMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			m_nLoops = nLoops;
		} else {
			m_bEndOfStream = true;
			if (s.fRewind) {
				SendMessageW(WM_COMMAND, ID_PLAY_STOP);
			} else {
				SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
			}

			if ((m_bFullScreen || IsD3DFullScreenMode()) && s.fExitFullScreenAtTheEnd) {
				OnViewFullscreen();
			}
		}
	}
	return true;
}

//
// our WM_GRAPHNOTIFY handler
//

LRESULT CMainFrame::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
	CAppSettings& s = AfxGetAppSettings();
	HRESULT hr = S_OK;

	LONG evCode = 0;
	LONG_PTR evParam1, evParam2;
	while (m_pME && SUCCEEDED(m_pME->GetEvent(&evCode, &evParam1, &evParam2, 0))) {
		DLogIf(evCode != 0x0000011a, L"--> CMainFrame::OnGraphNotify on thread: %d; event: 0x%08x (%s)", GetCurrentThreadId(), evCode, GetEventString(evCode));

		CString str;

		if (m_bCustomGraph) {
			if (EC_BG_ERROR == evCode) {
				str = CString((char*)evParam1);
			}
		}

		if (!m_bFrameSteppingActive) {
			m_nStepForwardCount = 0;
		}

		hr = m_pME->FreeEventParams(evCode, evParam1, evParam2);

		switch (evCode) {
			case EC_ERRORABORT:
				DLog(L"OnGraphNotify: EC_ERRORABORT -> hr = %08x", (HRESULT)evParam1);
				break;
			case EC_BUFFERING_DATA:
				DLog(L"OnGraphNotify: EC_BUFFERING_DATA -> %d, %d", evParam1, evParam2);

				m_bBuffering = ((HRESULT)evParam1 != S_OK);
				break;
			case EC_COMPLETE:
				if (!GraphEventComplete()) {
					return hr;
				}
				// No break here. Need to check and reset m_fFrameSteppingActive.
				// This is necessary in cases where the video ends earlier than other tracks.
			case EC_STEP_COMPLETE:
				if (m_bFrameSteppingActive) {
					m_nStepForwardCount++;
					m_bFrameSteppingActive = false;
					m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
				}
				break;
			case EC_DEVICE_LOST:
				if (GetPlaybackMode() == PM_CAPTURE && evParam2 == 0) {
					CComQIPtr<IBaseFilter> pBF = (IUnknown*)evParam1;
					if (!m_pVidCap && m_pVidCap == pBF || !m_pAudCap && m_pAudCap == pBF) {
						SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					}
				}
				break;
			case EC_DVD_TITLE_CHANGE:
				if (m_pDVDC) {
					if (m_PlaybackRate != 1.0) {
						OnPlayResetRate();
					}

					// Save current Title
					m_iDVDTitle = (DWORD)evParam1;

					if (m_iDVDDomain == DVD_DOMAIN_Title) {
						CString Domain;
						Domain.Format(ResStr(IDS_AG_TITLE), m_iDVDTitle);
						m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), Domain);
					}
					SetupChapters();
					SetToolBarSubtitleButton();
				}
				break;
			case EC_DVD_DOMAIN_CHANGE:
				if (m_pDVDC) {
					m_iDVDDomain = (DVD_DOMAIN)evParam1;
					OpenDVDData* pDVDData = dynamic_cast<OpenDVDData*>(m_lastOMD.get());
					ASSERT(pDVDData);

					CString Domain(L'-');

					switch (m_iDVDDomain) {
						case DVD_DOMAIN_FirstPlay:

							Domain = L"First Play";

							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}

							{
								m_bValidDVDOpen = true;

								if (s.bShowDebugInfo) {
									m_OSD.DebugMessage(L"DVD Title: %d", s.lDVDTitle);
								}

								if (s.lDVDTitle != 0) {
									// Set command line position
									hr = m_pDVDC->PlayTitle(s.lDVDTitle, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

									if (s.bShowDebugInfo) {
										m_OSD.DebugMessage(L"PlayTitle: 0x%08X", hr);
										m_OSD.DebugMessage(L"DVD Chapter: %d", s.lDVDChapter);
									}

									if (s.lDVDChapter > 1) {
										hr = m_pDVDC->PlayChapterInTitle(s.lDVDTitle, s.lDVDChapter, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

										if (s.bShowDebugInfo) {
											m_OSD.DebugMessage(L"PlayChapterInTitle: 0x%08X", hr);
										}
									} else {
										// Trick: skip trailers with some DVDs
										hr = m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

										if (s.bShowDebugInfo) {
											m_OSD.DebugMessage(L"Resume: 0x%08X", hr);
										}

										// If the resume call succeeded, then we skip PlayChapterInTitle
										// and PlayAtTimeInTitle.
										if ( hr == S_OK ) {
											// This might fail if the Title is not available yet?
											hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																   DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

											if (s.bShowDebugInfo) {
												m_OSD.DebugMessage(L"PlayAtTime: 0x%08X", hr);
											}
										} else {
											if (s.bShowDebugInfo)
												m_OSD.DebugMessage(L"Timecode requested: %02d:%02d:%02d.%03d",
																   s.DVDPosition.bHours, s.DVDPosition.bMinutes,
																   s.DVDPosition.bSeconds, s.DVDPosition.bFrames);

											// Always play chapter 1 (for now, until something else dumb happens)
											hr = m_pDVDC->PlayChapterInTitle(s.lDVDTitle, 1,
																			 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

											if (s.bShowDebugInfo) {
												m_OSD.DebugMessage(L"PlayChapterInTitle: 0x%08X", hr);
											}

											// This might fail if the Title is not available yet?
											hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																	 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

											if (s.bShowDebugInfo) {
												m_OSD.DebugMessage(L"PlayAtTime: 0x%08X", hr);
											}

											if ( hr != S_OK ) {
												hr = m_pDVDC->PlayAtTimeInTitle(s.lDVDTitle, &s.DVDPosition,
																				DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

												if (s.bShowDebugInfo) {
													m_OSD.DebugMessage(L"PlayAtTimeInTitle: 0x%08X", hr);
												}
											}
										} // Resume

										hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

										if (s.bShowDebugInfo) {
											m_OSD.DebugMessage(L"PlayAtTime: %d", hr);
										}
									}

									m_iDVDTitle	  = s.lDVDTitle;
									s.lDVDTitle   = 0;
									s.lDVDChapter = 0;
								} else if (pDVDData->pDvdState) {
									// Set position from favorite
									VERIFY(SUCCEEDED(m_pDVDC->SetState(pDVDData->pDvdState, DVD_CMD_FLAG_Block, nullptr)));
									// We don't want to restore the position from the favorite
									// if the playback is reinitialized so we clear the saved state
									pDVDData->pDvdState.Release();
								}
								else if (s.bKeepHistory && s.bRememberDVDPos && m_SessionInfo.DVDTitle > 0) {
									// restore DVD-Video position
									hr = m_pDVDC->PlayTitle(m_SessionInfo.DVDTitle, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
									if (SUCCEEDED(hr)) {
										if (m_SessionInfo.DVDTimecode.bSeconds > 0 || m_SessionInfo.DVDTimecode.bMinutes > 0 || m_SessionInfo.DVDTimecode.bHours > 0 || m_SessionInfo.DVDTimecode.bFrames > 0) {
											hr = m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
											if (SUCCEEDED(hr)) {
												hr = m_pDVDC->PlayAtTime(&m_SessionInfo.DVDTimecode, DVD_CMD_FLAG_Flush, nullptr);
											} else {
												hr = m_pDVDC->PlayChapterInTitle(m_SessionInfo.DVDTitle, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
												if (SUCCEEDED(hr)) {
													hr = m_pDVDC->PlayAtTime(&m_SessionInfo.DVDTimecode, DVD_CMD_FLAG_Flush, nullptr);
													if (FAILED(hr)) {
														hr = m_pDVDC->PlayAtTimeInTitle(m_SessionInfo.DVDTitle, &m_SessionInfo.DVDTimecode, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
													}
												}
											}
										}
									}

									if (SUCCEEDED(hr)) {
										m_iDVDTitle = m_SessionInfo.DVDTitle;
									}
								}
								else if (s.bStartMainTitle && s.bNormalStartDVD) {
									m_pDVDC->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
								}
								s.bNormalStartDVD = true;
								if (s.nPlaybackWindowMode && !m_bFullScreen && !IsD3DFullScreenMode()) { // Hack to the normal initial zoom for DVD + DXVA ...
									ZoomVideoWindow();
								}
							}
							break;
						case DVD_DOMAIN_VideoManagerMenu:
							Domain = L"Video Manager Menu";
							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}
							break;
						case DVD_DOMAIN_VideoTitleSetMenu:
							Domain = L"Video Title Set Menu";
							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}
							break;
						case DVD_DOMAIN_Title:
							Domain.Format(ResStr(IDS_AG_TITLE), m_iDVDTitle);
							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}

							m_iDVDTitleForHistory = m_iDVDTitle;

							if (!m_bValidDVDOpen) {
								m_bValidDVDOpen = true;
								m_pDVDC->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
							}

							break;
						case DVD_DOMAIN_Stop:
							Domain = ResStr(IDS_AG_STOP);
							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}
							break;
						default:
							Domain = L"-";
							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}
							break;
					}

					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), Domain);

	#if 0	// UOPs debug traces
					if (hr == VFW_E_DVD_OPERATION_INHIBITED) {
						ULONG UOPfields = 0;
						pDVDI->GetCurrentUOPS(&UOPfields);
						CString message;
						message.Format( L"UOP bitfield: 0x%08X; domain: %s", UOPfields, Domain);
						m_OSD.DisplayMessage( OSD_TOPLEFT, message );
					} else {
						m_OSD.DisplayMessage( OSD_TOPRIGHT, Domain );
					}
	#endif

					MoveVideoWindow(); // AR might have changed
					SetupChapters();
					SetToolBarSubtitleButton();
				}
				break;
			case EC_DVD_CURRENT_HMSF_TIME:
				if (m_pDVDC) {
					// Save current position in the chapter
					memcpy(&m_SessionInfo.DVDTimecode, (void*)&evParam1, sizeof(DVD_HMSF_TIMECODE));
				}
				break;
			case EC_DVD_ERROR:
				if (m_pDVDC) {
					DLog(L"OnGraphNotify: EC_DVD_ERROR -> %d, %d", evParam1, evParam2);

					CString err;

					switch (evParam1) {
						case DVD_ERROR_Unexpected:
						default:
							err = ResStr(IDS_MAINFRM_16);
							break;
						case DVD_ERROR_CopyProtectFail:
							err = ResStr(IDS_MAINFRM_17);
							break;
						case DVD_ERROR_InvalidDVD1_0Disc:
							err = ResStr(IDS_MAINFRM_18);
							break;
						case DVD_ERROR_InvalidDiscRegion:
							err = ResStr(IDS_MAINFRM_19);
							err.AppendFormat(L" (%u \x2260 %u)", LOWORD(evParam2), HIWORD(evParam2));
							break;
						case DVD_ERROR_LowParentalLevel:
							err = ResStr(IDS_MAINFRM_20);
							break;
						case DVD_ERROR_MacrovisionFail:
							err = ResStr(IDS_MAINFRM_21);
							break;
						case DVD_ERROR_IncompatibleSystemAndDecoderRegions:
							err = ResStr(IDS_MAINFRM_22);
							err.AppendFormat(L" (%u \x2260 %u)", LOWORD(evParam2), HIWORD(evParam2));
							break;
						case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
							err = ResStr(IDS_MAINFRM_23);
							err.AppendFormat(L" (%u \x2260 %u)", LOWORD(evParam2), HIWORD(evParam2));
							break;
					}

					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

					m_closingmsg = err;
				}
				break;
			case EC_DVD_WARNING:
				if (m_pDVDC) {
					DLog(L"OnGraphNotify: EC_DVD_WARNING -> %d, %d", evParam1, evParam2);
				}
				break;
			case EC_VIDEO_SIZE_CHANGED: {
				CSize size(evParam1);
				DLog(L"OnGraphNotify: EC_VIDEO_SIZE_CHANGED -> %ldx%ld", size.cx, size.cy);

				WINDOWPLACEMENT wp;
				wp.length = sizeof(wp);
				GetWindowPlacement(&wp);

				m_bAudioOnly = (size.cx <= 0 || size.cy <= 0);

				if (s.nPlaybackWindowMode
						&& !(m_bFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)) {
					ZoomVideoWindow();
				} else {
					MoveVideoWindow();
				}
			}
			break;
			case EC_LENGTH_CHANGED: {
				REFERENCE_TIME rtDur = 0;
				m_pMS->GetDuration(&rtDur);
				m_wndPlaylistBar.SetCurTime(rtDur);

				if (!m_bMainIsMPEGSplitter) {
					SetupChapters();
					LoadKeyFrames();
				}
			}
			break;
			case EC_BG_AUDIO_CHANGED:
				if (m_bCustomGraph) {
					int nAudioChannels = evParam1;

					m_wndStatusBar.SetStatusBitmap(nAudioChannels == 1 ? IDB_MONO
												   : nAudioChannels >= 2 ? IDB_STEREO
												   : IDB_NOAUDIO);
				}
				break;
			case EC_BG_ERROR:
				if (m_bCustomGraph) {
					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					if (str.GetLength()) {
						m_closingmsg = str;
					} else {
						m_closingmsg = L"Unspecified graph error";
					}
					m_wndPlaylistBar.SetCurValid(false);
					return hr;
				}
				break;
			case EC_DVD_PLAYBACK_RATE_CHANGE:
				if (m_pDVDC) {
					if (m_bCustomGraph && s.fullScreenModes.bEnabled == 1 &&
							(m_bFullScreen || IsD3DFullScreenMode()) && m_iDVDDomain == DVD_DOMAIN_Title) {
						AutoChangeMonitorMode();
					}
				}
				break;
			case EC_DVD_STILL_ON:
				m_bDVDStillOn = true;
				break;
			case EC_DVD_STILL_OFF:
				m_bDVDStillOn = false;
				break;
		}
	}

	return hr;
}

LRESULT CMainFrame::OnResizeDevice(WPARAM wParam, LPARAM lParam)
{
	DLog(L"CMainFrame::OnResizeDevice() : start");

	if (m_bOpenedThruThread) {
		CAMMsgEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_RESIZE, 0, (LPARAM)&e);
		e.WaitMsg();
	} else {
		ResizeDevice();
	}

	DLog(L"CMainFrame::OnResizeDevice() : end");
	return S_OK;
}

LRESULT CMainFrame::OnResetDevice(WPARAM wParam, LPARAM lParam)
{
	DLog(L"CMainFrame::OnResetDevice() : start");

	OAFilterState fs = State_Stopped;
	m_pMC->GetState(0, &fs);
	if (fs == State_Running) {
		if (GetPlaybackMode() != PM_CAPTURE) {
			m_pMC->Pause();
		} else {
			m_pMC->Stop(); // Capture mode doesn't support pause
		}
	}

	/*
	if (m_OSD.GetOSDType() != OSD_TYPE_GDI) {
		m_OSD.HideMessage(true);
	}
	*/

	if (m_bOpenedThruThread) {
		CAMMsgEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_RESET, 0, (LPARAM)&e);
		e.WaitMsg();
	} else {
		ResetDevice();
	}

	/*
	if (m_OSD.GetOSDType() != OSD_TYPE_GDI) {
		m_OSD.HideMessage(false);
	}
	*/

	if (fs == State_Running) {
		m_pMC->Run();
	}

	DLog(L"CMainFrame::OnResetDevice() : end");
	return S_OK;
}

LRESULT CMainFrame::OnPostOpen(WPARAM wParam, LPARAM lParam)
{
	std::unique_ptr<OpenMediaData> pOMD((OpenMediaData*)wParam);

	const auto& s = AfxGetAppSettings();

	m_nAudioTrackStored = m_nSubtitleTrackStored = -1;
	m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack = m_wndPlaylistBar.curPlayList.m_nSelectedSubtitleTrack = -1;
	m_bRememberSelectedTracks = true;

	if (!m_closingmsg.IsEmpty()) {
		CString aborted(ResStr(IDS_AG_ABORTED));

		CloseMedia();

		m_OSD.DisplayMessage(OSD_TOPLEFT, m_closingmsg, 1000);

		if (m_closingmsg != aborted) {

			if (OpenFileData *pFileData = dynamic_cast<OpenFileData*>(pOMD.get())) {
				m_wndPlaylistBar.SetCurValid(false);

				if (GetAsyncKeyState(VK_ESCAPE)) {
					m_closingmsg = aborted;
				}

				if (m_closingmsg != aborted && s.bPlaylistNextOnError) {
					if (m_wndPlaylistBar.IsAtEnd()) {
						m_nLoops++;
					}

					if (s.fLoopForever || m_nLoops < s.nLoops) {
						bool hasValidFile = false;

						if (m_flastnID == ID_NAVIGATE_SKIPBACK) {
							hasValidFile = m_wndPlaylistBar.SetPrev();
						} else {
							hasValidFile = m_wndPlaylistBar.SetNext();
						}

						if (hasValidFile && OpenCurPlaylistItem()) {
							return true;
						}
					} else if (m_wndPlaylistBar.GetCount() > 1) {
						DoAfterPlaybackEvent();
					}
				}
			} else {
				OnNavigateSkip(ID_NAVIGATE_SKIPFORWARD);
			}
		}
	} else {
		OnFilePostOpenMedia(pOMD);
	}

	Content::Online::Clear();

	m_bNextIsOpened = FALSE;

	m_flastnID = 0;

	RecalcLayout();
	if (IsWindow(m_wndToolBar.m_hWnd) && m_wndToolBar.IsVisible()) {
		m_wndToolBar.Invalidate();
	}

	return 0L;
}

void CMainFrame::SaveAppSettings()
{
	MSG msg;
	if (!PeekMessageW(&msg, m_hWnd, WM_SAVESETTINGS, WM_SAVESETTINGS, PM_NOREMOVE | PM_NOYIELD)) {
		AfxGetAppSettings().SaveSettings();
	}
}

BOOL CMainFrame::MouseMessage(UINT id, UINT nFlags, CPoint point)
{
	if (m_eMediaLoadState == MLS_LOADING) {
		return FALSE;
	}

	SetFocus();

	CRect r;
	if (IsD3DFullScreenMode()) {
		m_pFullscreenWnd->GetClientRect(r);
	} else {
		m_wndView.GetClientRect(r);
		m_wndView.MapWindowPoints(this, &r);
	}

	if (id != MOUSE_WHEEL_DOWN && id != MOUSE_WHEEL_UP && !r.PtInRect(point)) {
		return FALSE;
	}

	if (id == MOUSE_CLICK_LEFT && m_eMediaLoadState != MLS_LOADED && !AfxGetAppSettings().bMouseLeftClickOpenRecent) {
		return FALSE;
	}

	WORD cmd = AssignedMouseToCmd(id, nFlags);
	if (cmd) {
		SendMessageW(WM_COMMAND, cmd);
		return TRUE;
	}

	return FALSE;
}

void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bIsMPCVRExclusiveMode && m_OSD.OnLButtonDown(nFlags, point)) {
		return;
	}

	SetFocus();

	if (GetPlaybackMode() == PM_DVD) {
		CRect vid_rect = m_wndView.GetVideoRect();
		m_wndView.MapWindowPoints(this, &vid_rect);

		CPoint pDVD = point - vid_rect.TopLeft();

		if (SUCCEEDED(m_pDVDC->ActivateAtPosition(pDVD))) {
			return;
		}
	}

	m_bLeftMouseDown = TRUE;

	if (m_bFullScreen || CursorOnD3DFullScreenWindow()) {
		if (AssignedMouseToCmd(MOUSE_CLICK_LEFT, 0)) {
			m_bLeftMouseDownFullScreen = TRUE;
			MouseMessage(MOUSE_CLICK_LEFT, nFlags, point);
		}
		return;
	}

	if (AfxGetAppSettings().bMouseEasyMove) {
		SetCapture();
		m_bBeginCapture = true;
		m_beginCapturePoint = point;
	}

	__super::OnLButtonDown(nFlags, point);
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bWaitingRButtonUp = false;

	if (m_bIsMPCVRExclusiveMode && m_OSD.OnLButtonUp(nFlags, point)) {
		return;
	}

	if (!m_bHideCursor) {
		StartAutoHideCursor();
	}

	if (AfxGetAppSettings().bMouseEasyMove) {
		m_bBeginCapture = false;
		ReleaseCapture();
	}

	if (m_bLeftMouseDownFullScreen) {
		m_bLeftMouseDownFullScreen = FALSE;
		return;
	}

	if (!m_bLeftMouseDown) {
		return;
	}

	if (AssignedMouseToCmd(MOUSE_CLICK_LEFT, 0) && !m_bFullScreen && !CursorOnD3DFullScreenWindow()) {
		MouseMessage(MOUSE_CLICK_LEFT, nFlags, point);
		return;
	}

	__super::OnLButtonUp(nFlags, point);
}

void CMainFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_bLeftMouseDown) {
		MouseMessage(MOUSE_CLICK_LEFT, nFlags, point);
		m_bLeftMouseDown = FALSE;
	}

	if (!MouseMessage(MOUSE_CLICK_LEFT_DBL, nFlags, point)) {
		__super::OnLButtonDblClk(nFlags, point);
	}
}

void CMainFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendMessageW(WM_CANCELMODE);
	__super::OnMButtonDown(nFlags, point);
}

void CMainFrame::OnMButtonUp(UINT nFlags, CPoint point)
{
	m_bWaitingRButtonUp = false;

	if (!MouseMessage(MOUSE_CLICK_MIDLE, nFlags, point)) {
		__super::OnMButtonUp(nFlags, point);
	}
}

void CMainFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_bWaitingRButtonUp = true;

	__super::OnRButtonDown(nFlags, point);
}

void CMainFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_bWaitingRButtonUp) {
		m_bWaitingRButtonUp = false;
		SetFocus();

		UINT cmd = AfxGetAppSettings().nMouseRightClick;
		if (cmd == 0) {
			cmd = (!IsMainMenuVisible() || IsD3DFullScreenMode()) ? ID_MENU_PLAYER_SHORT : ID_MENU_PLAYER_LONG;
		}
		SendMessageW(WM_COMMAND, cmd);
	}
}

LRESULT CMainFrame::OnXButtonDown(WPARAM wParam, LPARAM lParam)
{
	SendMessageW(WM_CANCELMODE);
	return FALSE;
}

LRESULT CMainFrame::OnXButtonUp(WPARAM wParam, LPARAM lParam)
{
	m_bWaitingRButtonUp = false;

	UINT fwButton = GET_XBUTTON_WPARAM(wParam);
	return MouseMessage(fwButton == XBUTTON1 ? MOUSE_CLICK_X1 : fwButton == XBUTTON2 ? MOUSE_CLICK_X2 : 0,
					GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	m_bWaitingRButtonUp = false;

	if (m_wndPreView.IsWindowVisible()) {

		int seek =
			nFlags == MK_SHIFT ? 10 :
			nFlags == MK_CONTROL ? 1 : 5;

		zDelta > 0 ? SetCursorPos(point.x + seek, point.y) :
			zDelta < 0 ? SetCursorPos(point.x - seek, point.y) : SetCursorPos(point.x, point.y);

		return 0;
	}

	ScreenToClient(&point);

	BOOL fRet =
		zDelta > 0 ? MouseMessage(MOUSE_WHEEL_UP, nFlags, point) :
		zDelta < 0 ? MouseMessage(MOUSE_WHEEL_DOWN, nFlags, point) :
		FALSE;

	return fRet;
}

void CMainFrame::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_bWaitingRButtonUp = false;

	if (zDelta) {
		ScreenToClient(&pt);
		MouseMessage((zDelta < 0) ? MOUSE_WHEEL_LEFT : MOUSE_WHEEL_RIGHT, nFlags, pt);
	}
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
	// ignore mousemoves when entering fullscreen
	if (m_lastMouseMove.x == -1 && m_lastMouseMove.y == -1) {
		m_lastMouseMove.x = point.x;
		m_lastMouseMove.y = point.y;
	}

	const CAppSettings& s = AfxGetAppSettings();

	auto DragDetect = [](const CPoint& a, const CPoint& b) {
		auto DiffDelta = [](long a, long b, long delta) {
			return abs(a - b) > delta;
		};

		return DiffDelta(a.x, b.x, ::GetSystemMetrics(SM_CXDRAG)) || DiffDelta(a.y, b.y, ::GetSystemMetrics(SM_CYDRAG));
	};

	if (m_bBeginCapture && DragDetect(m_beginCapturePoint, point)) {
		m_bBeginCapture = false;
		m_bLeftMouseDown = FALSE;
		ReleaseCapture();

		if (s.iCaptionMenuMode == MODE_BORDERLESS) {
			WINDOWPLACEMENT wp;
			GetWindowPlacement(&wp);
			if (wp.showCmd == SW_SHOWMAXIMIZED) {
				SendMessageW(WM_SYSCOMMAND, SC_RESTORE, -1);
				RECT r;
				GetWindowRect(&r);
				ClipRectToMonitor(&r);
				SetWindowPos(nullptr, rc_forceNP.left, rc_forceNP.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		SendMessageW(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(m_beginCapturePoint.x, m_beginCapturePoint.y));
	}

	if (GetPlaybackMode() == PM_DVD) {
		CRect vid_rect = m_wndView.GetVideoRect();
		m_wndView.MapWindowPoints(this, &vid_rect);

		CPoint vp = point - vid_rect.TopLeft();
		ULONG pulButtonIndex;

		if (!m_bHideCursor) {
			SetCursor(LoadCursorW(nullptr, SUCCEEDED(m_pDVDI->GetButtonAtPosition(vp, &pulButtonIndex)) ? IDC_HAND : IDC_ARROW));
		}
		m_pDVDC->SelectAtPosition(vp);
	}

	if (m_lastMouseMove != point) {
		if (IsD3DFullScreenMode()) {
			StopAutoHideCursor();
			m_pFullscreenWnd->ShowCursor(true);
			SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);
		} else if (m_bIsMPCVRExclusiveMode) {
			StopAutoHideCursor();
			SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);

			if (m_OSD.OnMouseMove(nFlags, point)) {
				KillTimer(TIMER_MOUSEHIDER);
			} else {
				SetCursor(LoadCursorW(nullptr, IDC_ARROW));
			}
		} else if (m_bFullScreen) {
			int nTimeOut = s.nShowBarsWhenFullScreenTimeOut;

			if (nTimeOut < 0) {
				m_bHideCursor = false;
				if (s.fShowBarsWhenFullScreen) {
					ShowControls(s.nCS);
					if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && s.iDefaultCaptureDevice == 1) {
						m_wndNavigationBar.m_navdlg.UpdateElementList();
						m_wndNavigationBar.ShowControls(this, TRUE);
					}
				}

				KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
				SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);
			} else if (nTimeOut == 0) {
				CRect r;
				GetClientRect(r);
				r.top = r.bottom;

				int i = 1;
				for (const auto& pBar : m_bars) {
					CSize size = pBar->CalcFixedLayout(FALSE, TRUE);
					if (s.nCS & i) {
						r.top -= size.cy;
					}
					i <<= 1;
				}

				// HACK: the controls would cover the menu too early hiding some buttons
				if (GetPlaybackMode() == PM_DVD
					&& (m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu
						|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu)) {
					r.top = r.bottom - 10;
				}

				m_bHideCursor = false;

				if (r.PtInRect(point)) {
					if (s.fShowBarsWhenFullScreen) {
						ShowControls(s.nCS);
					}
				} else {
					if (s.fShowBarsWhenFullScreen) {
						ShowControls(CS_NONE, false);
					}
				}

				// PM_CAPTURE: Left Navigation panel for switching channels
				if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && s.iDefaultCaptureDevice == 1) {
					CRect rLeft;
					GetClientRect(rLeft);
					rLeft.right = rLeft.left;
					CSize size = m_wndNavigationBar.CalcFixedLayout(FALSE, TRUE);
					rLeft.right += size.cx;

					m_bHideCursor = false;

					if (rLeft.PtInRect(point)) {
						if (s.fShowBarsWhenFullScreen) {
							m_wndNavigationBar.m_navdlg.UpdateElementList();
							m_wndNavigationBar.ShowControls(this, TRUE);
						}
					} else {
						if (s.fShowBarsWhenFullScreen) {
							m_wndNavigationBar.ShowControls(this, FALSE);
						}
					}
				}

				SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);
			} else {
				m_bHideCursor = false;
				if (s.fShowBarsWhenFullScreen) {
					ShowControls(s.nCS);
				}

				SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut * 1000, nullptr);
				SetTimer(TIMER_MOUSEHIDER, std::max(nTimeOut * 1000, 2000), nullptr);
			}
		} else if (s.bHideWindowedMousePointer && m_eMediaLoadState == MLS_LOADED) {
			StopAutoHideCursor();
			SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);
		}
	}

	m_lastMouseMove = point;

	__super::OnMouseMove(nFlags, point);
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (!pScrollBar) {
		return;
	}

	if (pScrollBar->IsKindOf(RUNTIME_CLASS(CVolumeCtrl))) {
		OnPlayVolume(0);
	} else if (pScrollBar->IsKindOf(RUNTIME_CLASS(CPlayerSeekBar)) && m_eMediaLoadState == MLS_LOADED) {
		SeekTo(m_wndSeekBar.GetPos());
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	__super::OnInitMenu(pMenu);

	const UINT uiMenuCount = pMenu->GetMenuItemCount();
	if (uiMenuCount == -1) {
		return;
	}

	MENUITEMINFOW mii = { sizeof(mii) };

	for (UINT i = 0; i < uiMenuCount; ++i) {
		UINT itemID = pMenu->GetMenuItemID(i);
		if (itemID == UINT(-1)) {
			mii.fMask = MIIM_ID;
			pMenu->GetMenuItemInfoW(i, &mii, TRUE);
			itemID = mii.wID;
		}

		CMenu* pSubMenu = nullptr;

		if (itemID == ID_SUBMENU_NAVIGATE_MAIN) {
			pSubMenu = m_NavigateMenu.GetSubMenu(0);
		}
		else if (itemID == ID_SUBMENU_FAVORITES_MAIN) {
			SetupFavoritesSubMenu();
			pSubMenu = &m_favoritesMenu;
		}

		if (pSubMenu) {
			mii.fMask = MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
			mii.fType = MF_POPUP;
			mii.wID = itemID; // save ID after set popup type
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
			pMenu->SetMenuItemInfoW(i, &mii, TRUE);
		}
	}
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	UINT uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == UINT(-1)) {
		return;
	}

	const auto& s = AfxGetAppSettings();

	if (s.bUseDarkTheme && s.bDarkMenu) {
		MENUINFO MenuInfo = { 0 };
		MenuInfo.cbSize = sizeof(MenuInfo);
		MenuInfo.hbrBack = m_hPopupMenuBrush;
		MenuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		SetMenuInfo(pPopupMenu->GetSafeHmenu(), &MenuInfo);
	}

	MENUITEMINFOW mii = { sizeof(mii) };

	for (UINT i = 0; i < uiMenuCount; ++i) {
		CMenu* sm = pPopupMenu->GetSubMenu(i);
		if (sm && ::IsMenu(sm->m_hMenu)) {
			auto firstSubItemID = sm->GetMenuItemID(0);
			if (firstSubItemID == ID_VIEW_ZOOM_50) { // is "Zoom" submenu
				UINT uState = (m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly)
					? MF_ENABLED
					: (MF_DISABLED | MF_GRAYED);
				pPopupMenu->EnableMenuItem(i, MF_BYPOSITION | uState);
				continue;
			}
		}

		UINT itemID = pPopupMenu->GetMenuItemID(i);
		if (itemID == UINT(-1)) {
			mii.fMask = MIIM_ID;
			pPopupMenu->GetMenuItemInfoW(i, &mii, TRUE);
			itemID = mii.wID;
		}
		CMenu* pSubMenu = nullptr;
		bool bEnable = true;

		switch (itemID) {
		case ID_SUBMENU_OPENDISC:
			SetupOpenCDSubMenu();
			pSubMenu = &m_openCDsMenu;
			break;
		case ID_SUBMENU_RECENTFILES:
			SetupRecentFilesSubMenu();
			pSubMenu = &m_recentfilesMenu;
			break;
		case ID_SUBMENU_LANGUAGE:
			SetupLanguageMenu();
			pSubMenu = &m_languageMenu;
			break;
		case ID_SUBMENU_VIDEOFRAME:
			pSubMenu = m_VideoFrameMenu.GetSubMenu(0);
			break;
		case ID_SUBMENU_PANSCAN:
			pSubMenu = m_PanScanMenu.GetSubMenu(0);
			bEnable = (m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
			break;
		case ID_SUBMENU_FILTERS:
			SetupFiltersSubMenu();
			pSubMenu = &m_filtersMenu;
			break;
		case ID_SUBMENU_SHADERS:
			pSubMenu = m_ShadersMenu.GetSubMenu(0);
			break;
		case ID_SUBMENU_AUDIOLANG:
			SetupAudioSubMenu();
			pSubMenu = &m_AudioMenu;
			break;
		case ID_SUBMENU_SUBTITLELANG:
			SetupSubtitlesSubMenu();
			pSubMenu = &m_SubtitlesMenu;
			break;
		case ID_SUBMENU_VIDEOSTREAM:
		{
			const CString menu_str = GetPlaybackMode() == PM_DVD ? ResStr(IDS_MENU_VIDEO_ANGLE) : ResStr(IDS_MENU_VIDEO_STREAM);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = (LPWSTR)menu_str.GetString();
			pPopupMenu->SetMenuItemInfoW(i, &mii, TRUE);
			SetupVideoStreamsSubMenu();
			pSubMenu = &m_VideoStreamsMenu;
		}
			break;
		case ID_SUBMENU_AFTERPLAYBACK:
			pSubMenu = m_AfterPlaybackMenu.GetSubMenu(0);
			break;
		case ID_SUBMENU_NAVIGATE:
			pSubMenu = m_NavigateMenu.GetSubMenu(0);
			bEnable = (m_eMediaLoadState == MLS_LOADED);
			break;
		case ID_SUBMENU_JUMPTO:
			SetupNavChaptersSubMenu();
			pSubMenu = &m_chaptersMenu;
			break;
		case ID_SUBMENU_FAVORITES:
			SetupFavoritesSubMenu();
			pSubMenu = &m_favoritesMenu;
			break;
		}

		if (pSubMenu) {
			mii.fMask = MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
			mii.fType = MF_POPUP;
			mii.wID = itemID; // save ID after set popup type
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (bEnable && pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
			pPopupMenu->SetMenuItemInfoW(i, &mii, TRUE);
			//continue;
		}
	}

	uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == UINT(-1)) {
		return;
	}

	for (UINT i = 0; i < uiMenuCount; ++i) {
		UINT nID = pPopupMenu->GetMenuItemID(i);
		if (nID == ID_SEPARATOR || nID == UINT(-1)
				|| (nID >= ID_FAVORITES_FILE_START && nID <= ID_FAVORITES_FILE_END)
				|| (nID >= ID_FAVORITES_DVD_START && nID <= ID_FAVORITES_DVD_END)
				|| (nID >= ID_RECENT_FILE_START && nID <= ID_RECENT_FILE_END)
				|| (nID >= ID_NAVIGATE_CHAP_SUBITEM_START && nID <= ID_NAVIGATE_CHAP_SUBITEM_END)) {
			continue;
		}

		CString str;
		pPopupMenu->GetMenuStringW(i, str, MF_BYPOSITION);
		int k = str.Find('\t');
		if (k > 0) {
			str = str.Left(k);
		}

		CString key = CPPageAccelTbl::MakeAccelShortcutLabel(nID);
		if (key.IsEmpty() && k < 0) {
			continue;
		}
		str += L"\t" + key;

		// BUG(?): this disables menu item update ui calls for some reason...
		//pPopupMenu->ModifyMenu(i, MF_BYPOSITION|MF_STRING, nID, str);

		// this works fine
		mii.fMask      = MIIM_STRING;
		mii.dwTypeData = (LPWSTR)str.GetString();
		pPopupMenu->SetMenuItemInfoW(i, &mii, TRUE);
	}

	uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == UINT(-1)) {
		return;
	}

	bool fPnSPresets = false;

	for (UINT i = 0; i < uiMenuCount; ++i) {
		UINT nID = pPopupMenu->GetMenuItemID(i);

		if (nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END) {
			do {
				nID = pPopupMenu->GetMenuItemID(i);
				pPopupMenu->DeleteMenu(i, MF_BYPOSITION);
				uiMenuCount--;
			} while (i < uiMenuCount && nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END);

			nID = pPopupMenu->GetMenuItemID(i);
		}

		if (nID == ID_VIEW_RESET) {
			fPnSPresets = true;
		}
	}

	if (fPnSPresets) {
		int i = 0, j = s.m_pnspresets.GetCount();
		for (; i < j; i++) {
			int k = 0;
			CString label = s.m_pnspresets[i].Tokenize(L",", k);
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, label);
		}
		//if (j > 0)
		{
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, ResStr(IDS_PANSCAN_EDIT));
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND | MF_SEPARATOR);
		}
	}

	__super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	if (s.bUseDarkTheme && s.bDarkMenu) {
		CMenuEx::ChangeStyle(pPopupMenu);
	}
}

void CMainFrame::OnUnInitMenuPopup(CMenu* pPopupMenu, UINT nFlags)
{
	__super::OnUnInitMenuPopup(pPopupMenu, nFlags);

	MSG msg;
	PeekMessageW(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);

	m_bLeftMouseDown = FALSE;
}

BOOL CMainFrame::OnMenu(CMenu* pMenu)
{
	CheckPointer(pMenu, FALSE);

	// skip empty menu
	if (!pMenu->GetMenuItemCount()) {
		return FALSE;
	}

	// Do not show popup menu in D3D fullscreen it has several adverse effects.
	if (CursorOnD3DFullScreenWindow()) {
		return FALSE;
	}

	if (m_bClosingState) {
		return FALSE; // prevent crash when player closes with context menu open
	}

	CPoint point;
	GetCursorPos(&point);
	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NOANIMATION, point.x + 1, point.y + 1, this);

	return TRUE;
}

void CMainFrame::OnMenuPlayerShort()
{
	OnMenu(m_popupMainMenu.GetSubMenu(0));
}

void CMainFrame::OnMenuPlayerLong()
{
	OnMenu(m_popupMenu.GetSubMenu(0));
}

void CMainFrame::OnMenuFilters()
{
	SetupFiltersSubMenu();
	OnMenu(&m_filtersMenu);
}

CString CMainFrame::UpdatePlayerStatus()
{
	CString msg;

	if (m_eMediaLoadState == MLS_LOADING) {
		msg = ResStr(IDS_CONTROLS_OPENING);
		if (AfxGetAppSettings().fUseWin7TaskBar && m_pTaskbarList) {
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		}
	} else if (m_eMediaLoadState == MLS_LOADED) {
		msg = FillMessage();

		if (msg.IsEmpty()) {
			OAFilterState fs = GetMediaState();

			if (fs == State_Stopped) {
				msg = ResStr(IDS_CONTROLS_STOPPED);
			}
			else if (fs == State_Paused || m_bFrameSteppingActive) {
				msg = ResStr(IDS_CONTROLS_PAUSED);
			}
			else if (fs == State_Running) {
				msg = ResStr(IDS_CONTROLS_PLAYING);
				if (m_PlaybackRate != 1.0) {
					msg.AppendFormat(L" (%sx)", Rate2String(m_PlaybackRate));
				}
			}

			if (!m_bAudioOnly &&
					(fs == State_Paused || fs == State_Running) &&
					!(AfxGetAppSettings().bUseDarkTheme && m_wndToolBar.IsVisible()) &&
					DXVAState::GetState()) {
				msg.AppendFormat(L" [%s]", DXVAState::GetShortDescription());
			}
		}
	} else if (m_eMediaLoadState == MLS_CLOSING) {
		msg = ResStr(IDS_CONTROLS_CLOSING);
		if (AfxGetAppSettings().fUseWin7TaskBar && m_pTaskbarList) {
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		}
	} else {
		msg = m_closingmsg;
	}

	SetStatusMessage(msg);

	return msg;
}

void CMainFrame::OnUpdatePlayerStatus(CCmdUI* pCmdUI)
{
	pCmdUI->SetText(UpdatePlayerStatus());
}

void CMainFrame::OnFilePostOpenMedia(std::unique_ptr<OpenMediaData>& pOMD)
{
	ASSERT(m_eMediaLoadState == MLS_LOADING);
	SetLoadState(MLS_LOADED);

	// destroy invisible top-level d3dfs window if there is no video renderer
	if (IsD3DFullScreenMode() && !m_pD3DFS) {
		DestroyD3DWindow();
		m_bStartInD3DFullscreen = true;
	}

	// remember OpenMediaData for later use
	m_lastOMD = std::move(pOMD);

	if (m_bIsBDPlay == FALSE) {
		m_BDPlaylists.clear();
		m_LastOpenBDPath.Empty();
	}

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	BOOL bMvcActive = FALSE;
	if (CComQIPtr<IExFilterConfig> pEFC = FindFilter(__uuidof(CMPCVideoDecFilter), m_pGB)) {
		pEFC->Flt_GetInt("decode_mode_mvc", &bMvcActive);
	}

	if (s.iStereo3DMode == STEREO3D_ROWINTERLEAVED || s.iStereo3DMode == STEREO3D_ROWINTERLEAVED_2X
			|| (s.iStereo3DMode == STEREO3D_AUTO && bMvcActive && !m_pBFmadVR)) {
		rs.Stereo3DSets.iTransform = STEREO3D_HalfOverUnder_to_Interlace;
	} else {
		rs.Stereo3DSets.iTransform = STEREO3D_AsIs;
	}
	rs.Stereo3DSets.bSwapLR = s.bStereo3DSwapLR;
	if (m_pCAP) {
		m_pCAP->SetStereo3DSettings(&rs.Stereo3DSets);
	}

	if (OpenDeviceData *pDeviceData = dynamic_cast<OpenDeviceData*>(m_lastOMD.get())) {
		m_wndCaptureBar.m_capdlg.SetVideoInput(pDeviceData->vinput);
		m_wndCaptureBar.m_capdlg.SetVideoChannel(pDeviceData->vchannel);
		m_wndCaptureBar.m_capdlg.SetAudioInput(pDeviceData->ainput);
	}

	REFERENCE_TIME rtDur = 0;
	if (m_pMS && m_pMS->GetDuration(&rtDur) == S_OK) {
		m_wndPlaylistBar.SetCurTime(rtDur);
	}

	m_wndPlaylistBar.SetCurValid(true);

	if (m_youtubeFields.title.IsEmpty()) {
		if (CComQIPtr<IBaseFilter> pBF = FindFilter(CLSID_3DYDYoutubeSource, m_pGB)) {
			if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF.p) {
				CComBSTR bstr;
				if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
					m_youtubeFields.title = bstr;

					CString ext = L".mp4";
					BeginEnumPins(pBF, pEP, pPin) {
						PIN_DIRECTION dir;
						if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT) {
							CMediaType mt;
							if (SUCCEEDED(pPin->ConnectionMediaType(&mt)) && mt.subtype != MEDIASUBTYPE_NULL) {
								if (mt.subtype == MEDIASUBTYPE_Matroska) {
									ext = L".webm";
								} else if (mt.subtype == MEDIASUBTYPE_FLV) {
									ext = L".flv";
								}
							}
							break;
						}
					}
					EndEnumPins;

					m_youtubeFields.fname =  m_youtubeFields.title + ext;
					FixFilename(m_youtubeFields.fname);
				}
			}
		}
	}

	// Waffs : PnS command line
	if (!s.strPnSPreset.IsEmpty()) {
		for (int i = 0; i < s.m_pnspresets.GetCount(); i++) {
			int j = 0;
			CString str = s.m_pnspresets[i];
			CString label = str.Tokenize(L",", j);
			if (s.strPnSPreset == label) {
				OnViewPanNScanPresets(i + ID_PANNSCAN_PRESETS_START);
			}
		}
		s.strPnSPreset.Empty();
	}

	OpenSetupInfoBar();
	OpenSetupStatsBar();
	OpenSetupStatusBar();
	OpenSetupCaptureBar();
	OnTimer(TIMER_STATS);

	if (GetPlaybackMode() == PM_CAPTURE) {
		ShowControlBarInternal(&m_wndSubresyncBar, FALSE);
	}

	m_nCurSubtitle   = -1;
	m_lSubtitleShift = 0;

	if (m_pDVS) {
		int SubtitleDelay, SubtitleSpeedMul, SubtitleSpeedDiv;
		if (SUCCEEDED(m_pDVS->get_SubtitleTiming(&SubtitleDelay, &SubtitleSpeedMul, &SubtitleSpeedDiv))) {
			m_pDVS->put_SubtitleTiming(0, SubtitleSpeedMul, SubtitleSpeedDiv);
		}
	}
	if (m_pCAP) {
		m_pCAP->SetSubtitleDelay(0);
	}

	OnTimer(TIMER_STREAMPOSPOLLER);
	OnTimer(TIMER_STREAMPOSPOLLER2);

	m_bDelaySetOutputRect = false;

	if (s.fLaunchfullscreen && !s.ExclusiveFSAllowed() && !m_bFullScreen && !m_bAudioOnly) {
		ToggleFullscreen(true, true);
	}

	if (m_bAudioOnly) {
		SetAudioPicture(s.nAudioWindowMode != 2);

		if (s.nAudioWindowMode == 2) {
			CRect r;
			GetWindowRect(&r);
			r.bottom = r.top;
			MoveWindow(&r);
		}
	}

	if (!(m_bAudioOnly && IsSomethingLoaded() && s.nAudioWindowMode == 2)) {
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(&wp);

		// Workaround to avoid MadVR freezing when switching channels in PM_CAPTURE mode:
		if (IsWindowVisible() && s.nPlaybackWindowMode
				&& !(m_bFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)
				&& GetPlaybackMode() == PM_CAPTURE && m_clsidCAP == CLSID_madVRAllocatorPresenter) {
			ShowWindow(SW_MAXIMIZE);
			wp.showCmd = SW_SHOWMAXIMIZED;
		}

		// restore magnification
		if (IsWindowVisible() && s.nPlaybackWindowMode
				&& !(m_bFullScreen || IsD3DFullScreenMode()
					|| wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)) {
			ZoomVideoWindow(false);
		}
	}

	m_bfirstPlay   = true;
	m_LastOpenFile = m_lastOMD->title;

	if (GetPlaybackMode() == PM_FILE) {
		REFERENCE_TIME rtDur = {};
		m_pMS->GetDuration(&rtDur);
		if (rtDur == 0) {
			auto fname = GetCurFileName();
			CUrlParser urlParser(fname);
			if (urlParser.IsValid()) {
				m_bIsLiveOnline = true;
			}
		}
	}

	if (!(s.nCLSwitches & CLSW_OPEN) && (s.nLoops > 0)) {
		if (s.fullScreenModes.bEnabled == 1
				&& (m_bFullScreen || IsD3DFullScreenMode())
				&& s.iDMChangeDelay > 0) {
			SetTimer(TIMER_DM_AUTOCHANGING, s.iDMChangeDelay * 1000, nullptr);
		} else {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
		}
	} else {
		SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
	}
	s.nCLSwitches &= ~CLSW_OPEN;

	SendNowPlayingToApi();

	if (m_bFullScreen && s.nPlaybackWindowMode) {
		m_bFirstFSAfterLaunchOnFullScreen = true;
	}

	if (!m_bAudioOnly &&
			((m_bFullScreen && !s.fLaunchfullscreen && !s.ExclusiveFSAllowed() && s.fullScreenModes.bEnabled == 1) || m_bNeedAutoChangeMonitorMode)) {
		AutoChangeMonitorMode();
	}

	// Ensure the dynamically added menu items are updated
	SetupFiltersSubMenu();
	SetupSubtitlesSubMenu();
	SetupAudioTracksSubMenu();
	SetupSubtitleTracksSubMenu();
	SetupVideoStreamsSubMenu();
	SetupNavChaptersSubMenu();
	SetupRecentFilesSubMenu();

	UpdatePlayerStatus();

	SetToolBarAudioButton();
	SetToolBarSubtitleButton();

	UpdateTitle();

	// correct window size if "Limit window proportions on resize" enable.
	if (!s.nPlaybackWindowMode && s.bLimitWindowProportions) {
		const bool bCtrl = !!(GetAsyncKeyState(VK_CONTROL) & 0x80000000);
		if (bCtrl) {
			keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		}

		CRect r;
		GetWindowRect(&r);
		OnSizing(WMSZ_LEFT, r);

		ClampWindowRect(r);
		MoveWindow(r);
	}

	if (!m_wndSeekBar.HasDuration()) {
		ReleasePreviewGraph();
	}

	if (CanPreviewUse() && m_wndSeekBar.IsVisible()) {
		CPoint point;
		GetCursorPos(&point);

		CRect rect;
		m_wndSeekBar.GetWindowRect(&rect);
		if (rect.PtInRect(point)) {
			m_wndSeekBar.PreviewWindowShow();
		}
	}

	// Trying to find LAV Video is in the graph
	if (CComQIPtr<LAVVideo::ILAVVideoStatus> pLAVVideoStatus = FindFilter(GUID_LAVVideoDecoder, m_pGB)) {
		LPCWSTR decoderName = pLAVVideoStatus->GetActiveDecoderName();

		if (wcscmp(decoderName, L"avcodec") == 0) {
			DXVAState::ClearState();
		} else {
			static const struct {
				const WCHAR* name;
				const WCHAR* friendlyname;
			} LAVDecoderNames[] = {
				{L"dxva2cb",         L"DXVA2 Copy-back"},
				{L"dxva2cb direct",  L"DXVA2 Copy-back (direct)"},
				{L"d3d11 cb",        L"D3D11 Copy-back"},
				{L"d3d11 cb direct", L"D3D11 Copy-back (direct)"},
				{L"d3d11 native",    L"D3D11 Native"},
				{L"cuvid",           L"Nvidia CUVID"},
				{L"quicksync",       L"Intel QuickSync"},
				{L"msdk mvc hw",     L"Intel H.264(MVC 3D)"},
			};

			for (const auto &item : LAVDecoderNames) {
				if (wcscmp(item.name, decoderName) == 0) {
					UnHookDirectXVideoDecoderService();
					DXVAState::SetActiveState(GUID_NULL, item.friendlyname);
					break;
				}
			}
		}
		// for "avcodec" nothing needs to be done
	}

	if (bMvcActive == 2) {
		UnHookDirectXVideoDecoderService();
		DXVAState::SetActiveState(GUID_NULL, L"Intel H.264(MVC 3D)");
	}

	m_wndPlaylistBar.SavePlaylist();

	StartAutoHideCursor();

	m_CMediaControls.Update();

	m_bOpening = false;
}

void CMainFrame::OnFilePostCloseMedia()
{
	DLog(L"CMainFrame::OnFilePostCloseMedia() : start");

	SetPlaybackMode(PM_NONE);
	SetLoadState(MLS_CLOSED);

	if (!m_bFullScreen) {
		StopAutoHideCursor();
	}

	KillTimer(TIMER_STATUSERASER);
	m_playingmsg.Empty();

	if (m_closingmsg.IsEmpty()) {
		m_closingmsg = ResStr(IDS_CONTROLS_CLOSED);
	}

	m_wndView.SetVideoRect();

	CAppSettings& s = AfxGetAppSettings();

	s.nCLSwitches &= CLSW_OPEN | CLSW_PLAY | CLSW_AFTERPLAYBACK_MASK | CLSW_NOFOCUS;
	//s.ResetPositions();

	m_OSD.RemoveChapters();
	if (s.ShowOSD.Enable) {
		m_OSD.Start(m_pOSDWnd);
	}

	m_ExtSubFiles.clear();
	m_ExtSubPaths.clear();
	m_EventSubChangeRefreshNotify.Set();

	m_wndSeekBar.Enable(false);
	m_wndSeekBar.SetRange(0);
	m_wndSeekBar.SetPos(0);
	m_wndSeekBar.RemoveChapters();
	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();
	m_wndStatusBar.Clear();
	m_wndStatusBar.ShowTimer(false);
	m_wndStatusBar.Relayout();

	if (IsWindow(m_wndSubresyncBar.m_hWnd)) {
		ShowControlBarInternal(&m_wndSubresyncBar, FALSE);
	}
	SetSubtitle(nullptr);

	if (IsWindow(m_wndCaptureBar.m_hWnd)) {
		ShowControlBarInternal(&m_wndCaptureBar, FALSE);
		m_wndCaptureBar.m_capdlg.SetupVideoControls(L"", nullptr, nullptr, nullptr);
		m_wndCaptureBar.m_capdlg.SetupAudioControls(L"", nullptr, std::vector<CComQIPtr<IAMAudioInputMixer>>());
	}

	RecalcLayout();
	UpdateWindow();

	SetWindowTextW(s_strPlayerTitle);
	m_Lcd.SetMediaTitle(s_strPlayerTitle);

	SetAlwaysOnTop(s.iOnTop);

	m_bIsBDPlay = FALSE;

	// close all menus
	SendMessageW(WM_CANCELMODE);
	// Ensure the dynamically added menu items are cleared
	MakeEmptySubMenu(m_filtersMenu);
	MakeEmptySubMenu(m_VideoStreamsMenu);
	// dynamic menus (which can be called independently) must be destroyed to reset the minimum width value
	m_SubtitlesMenu.DestroyMenu();
	m_AudioMenu.DestroyMenu();
	m_chaptersMenu.DestroyMenu();

	UnloadExternalObjects();

	SetAudioPicture(FALSE);

	if (m_bNeedUnmountImage) {
		m_DiskImage.UnmountDiskImage();
	}
	m_bNeedUnmountImage = TRUE;

	SetCurrentDirectoryW(GetProgramDir()); // It is necessary to unlock the folder after opening files from explorer.

	SetThreadExecutionState(ES_CONTINUOUS);

	if (IsD3DFullScreenMode()) {
		DestroyD3DWindow();
		m_bStartInD3DFullscreen = true;
	}

	m_bAudioOnly = true;

	SetToolBarAudioButton();
	SetToolBarSubtitleButton();

	// close popup menus
	SendMessageW(WM_CANCELMODE);
	if (m_wndPlaylistBar.IsWindowVisible()) {
		m_wndPlaylistBar.SendMessageW(WM_CANCELMODE);
	}
	if (m_wndToolBar.IsWindowVisible()) {
		m_wndToolBar.SendMessageW(WM_CANCELMODE);
	}
	m_wndToolBar.SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	DXVAState::ClearState();
	m_wndToolBar.Invalidate(FALSE);

	DLog(L"CMainFrame::OnFilePostCloseMedia() : end");
}

void CMainFrame::OnBossKey()
{
	if (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) {
		m_wndFlyBar.ShowWindow(SW_HIDE);
	}

	// Disable animation
	ANIMATIONINFO AnimationInfo = { sizeof(AnimationInfo) };
	::SystemParametersInfoW(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
	const int WindowAnimationType = AnimationInfo.iMinAnimate;
	AnimationInfo.iMinAnimate = 0;
	::SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

	if (State_Running == GetMediaState()) {
		SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
	}
	if (m_bFullScreen || IsD3DFullScreenMode()) {
		SendMessageW(WM_COMMAND, ID_VIEW_FULLSCREEN);
	}
	SendMessageW(WM_SYSCOMMAND, SC_MINIMIZE, -1);

	// Enable animation
	AnimationInfo.iMinAnimate = WindowAnimationType;
	::SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
}

void CMainFrame::OnStreamAudio(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams)) && cStreams) {

			for (DWORD i = 0; i < cStreams; i++) {
				DWORD dwFlags = 0;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, nullptr, nullptr, nullptr))) {
					break;
				}
				if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
					long newStream = i;
					if (cStreams > 1) {
						if (nID == ID_STREAM_AUDIO_NEXT) {
							newStream++;
						} else {
							newStream += cStreams - 1;
						}
						newStream %= cStreams;

						pSS->Enable(newStream, AMSTREAMSELECTENABLE_ENABLE);
					}

					CComHeapPtr<WCHAR> pszName;
					if (SUCCEEDED(pSS->Info(newStream, nullptr, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr))) {
						CString strMessage;
						strMessage.Format(ResStr(IDS_AUDIO_STREAM), newStream+1, cStreams, pszName);
						m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
					}
					break;
				}
			}
		}

		return;
	}

	if (GetPlaybackMode() == PM_DVD && m_pDVDI && m_pDVDC) {
		ULONG nStreamsAvailable, nCurrentStream;
		if (SUCCEEDED(m_pDVDI->GetCurrentAudio(&nStreamsAvailable, &nCurrentStream)) && nStreamsAvailable) {
			HRESULT hr_select = S_FALSE;

			ULONG newStream = nCurrentStream;
			if (nStreamsAvailable > 1) {
				if (nID == ID_STREAM_AUDIO_NEXT) {
					newStream++;
				} else {
					newStream += nStreamsAvailable - 1;
				}
				newStream %= nStreamsAvailable;

				hr_select = m_pDVDC->SelectAudioStream(newStream, DVD_CMD_FLAG_Block, nullptr);
			}

			DVD_AudioAttributes AATR;
			if (SUCCEEDED(m_pDVDI->GetAudioAttributes(newStream, &AATR))) {
				CString lang;
				if (AATR.Language) {
					int len = GetLocaleInfoW(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(std::max(len - 1, 0));
				}
				else {
					lang.Format(ResStr(IDS_AG_UNKNOWN), newStream + 1);
				}

				CString format = GetDVDAudioFormatName(AATR);
				if (format.GetLength()) {
					CString str;
					str.Format(ResStr(IDS_MAINFRM_11),
						lang,
						format,
						AATR.dwFrequency,
						AATR.bQuantization,
						AATR.bNumberOfChannels,
						(AATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
					if (FAILED(hr_select)) {
						str += L" [" + ResStr(IDS_AG_ERROR) + L"] ";
					}
					CString strMessage;
					strMessage.Format(ResStr(IDS_AUDIO_STREAM), newStream+1, nStreamsAvailable, str);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}
		}
	}
}

void CMainFrame::OnStreamSub(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	nID -= ID_STREAM_SUB_NEXT;

	if (GetPlaybackMode() == PM_FILE && m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
			bool fHideSubtitles = false;
			m_pDVS->get_HideSubtitles(&fHideSubtitles);

			if (!fHideSubtitles) {
				int subcount = GetStreamCount(2);
				if (subcount) {
					CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
					DWORD cStreams = 0;
					pSS->Count(&cStreams);

					int iSelected = 0;
					m_pDVS->get_SelectedLanguage(&iSelected);
					if (iSelected == (nLangs - 1)) {
						int selectedStream = (nLangs - 1);
						DWORD cStreamsS = 0;
						pSS->Count(&cStreamsS);
						for (long i = 0; i < (long)cStreamsS; i++) {
							DWORD dwFlags = DWORD_MAX;
							DWORD dwGroup = DWORD_MAX;

							if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
								continue;
							}

							if (dwGroup != 2) {
								continue;
							}

							if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
								iSelected = selectedStream;
							}

							selectedStream++;
						}
					}

					subcount += (nLangs - 1);

					iSelected = (iSelected + (nID == 0 ? 1 : subcount - 1)) % subcount;

					WCHAR* pName = nullptr;
					CString	strMessage;

					if (iSelected < (nLangs - 1)) {
						m_pDVS->put_SelectedLanguage(iSelected);
						m_pDVS->get_LanguageName(iSelected, &pName);

						strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), pName);
					} else {
						m_pDVS->put_SelectedLanguage(nLangs - 1);
						iSelected -= (nLangs - 1);

						long substream = 0;
						DWORD cStreamsS = 0;
						pSS->Count(&cStreamsS);
						for (long i = 0; i < (long)cStreamsS; i++) {
							DWORD dwGroup = DWORD_MAX;

							if (FAILED(pSS->Info(i, nullptr, nullptr, nullptr, &dwGroup, &pName, nullptr, nullptr))
									|| !pName) {
								continue;
							}

							CString sub_stream(pName);
							CoTaskMemFree(pName);

							if (dwGroup != 2) {
								continue;
							}

							if (substream == iSelected) {
								pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
								int k = sub_stream.Find(L"Subtitle - ");
								if (k >= 0) {
									sub_stream = sub_stream.Right(sub_stream.GetLength() - k - 8);
								}
								strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), sub_stream);

								break;
							}

							substream++;
						}
					}

					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
					return;
				}

				int iSelected = 0;
				m_pDVS->get_SelectedLanguage(&iSelected);
				iSelected = (iSelected + (nID == 0 ? 1 : nLangs - 1)) % nLangs;
				m_pDVS->put_SelectedLanguage(iSelected);

				WCHAR* pName = nullptr;
				m_pDVS->get_LanguageName(iSelected, &pName);
				CString	strMessage;
				strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), pName);
				CoTaskMemFree(pName);

				m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
			}
		}

		return;
	}

	if (GetPlaybackMode() == PM_FILE && AfxGetAppSettings().fEnableSubtitles) {

		int splcnt	= 0;
		Stream ss;
		std::vector<Stream> MixSS;
		int iSel	= -1;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
		if (pSS && !AfxGetAppSettings().fDisableInternalSubtitles) {
			DWORD cStreamsS = 0;

			if (SUCCEEDED(pSS->Count(&cStreamsS)) && cStreamsS > 0) {
				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS.p;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
					bIsHaali = TRUE;
					cStreamsS--;
				}

				for (DWORD i = 0; i < cStreamsS; i++) {
					DWORD dwFlags = DWORD_MAX;
					DWORD dwGroup = DWORD_MAX;
					if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
						return;
					}

					if (dwGroup == 2) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE)) {
							iSel = MixSS.size();
						}
						ss.Filter	= 1;
						ss.Index	= i;
						ss.Sel		= iSel;
						ss.Num++;
						MixSS.emplace_back(ss);
					}
				}
			}
		}

		splcnt = MixSS.size();

		int subcnt = -1;
		auto it = m_pSubStreams.cbegin();
		if (splcnt > 0) {
			++it;
			subcnt++;
		}
		while (it != m_pSubStreams.cend()) {
			CComPtr<ISubStream> pSubStream = *it++;
			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				subcnt++;
				ss.Filter	= 2;
				ss.Index	= subcnt;
				ss.Num++;
				if (m_iSubtitleSel == subcnt) iSel = MixSS.size();
				ss.Sel		= iSel;
				MixSS.emplace_back(ss);
			}
		}

		int cnt = MixSS.size();
		if (cnt > 1) {
			int nNewStream2 = MixSS[(iSel + (nID == 0 ? 1 : cnt - 1)) % cnt].Num;
			int iF;
			int nNewStream;

			for (size_t i = 0; i < MixSS.size(); i++) {
				if (MixSS[i].Num == nNewStream2) {
					iF			= MixSS[i].Filter;
					nNewStream	= MixSS[i].Index;
					break;
				}
			}

			bool ExtStream = false;
			if (iF == 1) { // Splitter Subtitle Tracks

				for (size_t i = 0; i < MixSS.size(); i++) {
					if (MixSS[i].Sel == iSel && MixSS[i].Filter == 2) {
						ExtStream = true;
						break;
					}
				}

				if (ExtStream) {
					m_iSubtitleSel = 0;
					UpdateSubtitle();
				}
				pSS->Enable(nNewStream, AMSTREAMSELECTENABLE_ENABLE);

				WCHAR* pszName = nullptr;
				if (SUCCEEDED(pSS->Info(nNewStream, nullptr, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr)) && pszName) {
					CString stream_name = pszName;
					CoTaskMemFree(pszName);
					if (StartsWith(stream_name, L"Subtitle - ")) {
						stream_name = stream_name.Mid(10);
					}
					CString strMessage;
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), stream_name);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}

				if (m_pCAP) {
					m_pCAP->Invalidate();
				}

			} else if (iF == 2) {
				m_iSubtitleSel = nNewStream;
				UpdateSubtitle(true);
				SetFocus();
			}
		}

		return;
	}

	if (GetPlaybackMode() == PM_DVD && m_pDVDI && m_pDVDC) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
			&& ulStreamsAvailable > 1) {
			//UINT nNextStream = (ulCurrentStream+(nID==0?1:ulStreamsAvailable-1))%ulStreamsAvailable;
			int nNextStream;

			if (!bIsDisabled) {
				nNextStream = ulCurrentStream + (nID == 0 ? 1 : -1);
			}
			else {
				nNextStream = (nID == 0 ? 0 : ulStreamsAvailable - 1);
			}

			if (!bIsDisabled && ((nNextStream < 0) || ((ULONG)nNextStream >= ulStreamsAvailable))) {
				m_pDVDC->SetSubpictureState(FALSE, DVD_CMD_FLAG_Block, nullptr);
				m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_SUBTITLE_STREAM_OFF));
			}
			else {
				HRESULT hr = m_pDVDC->SelectSubpictureStream(nNextStream, DVD_CMD_FLAG_Block, nullptr);

				DVD_SubpictureAttributes SATR;
				m_pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, nullptr);
				if (SUCCEEDED(m_pDVDI->GetSubpictureAttributes(nNextStream, &SATR))) {
					CString lang;
					CString	strMessage;
					int len = GetLocaleInfoW(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(std::max(len - 1, 0));
					if (FAILED(hr)) {
						lang += L" [" + ResStr(IDS_AG_ERROR) + L"] ";
					}
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), lang);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}
		}
	}
}

void CMainFrame::OnStreamSubOnOff()
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}


	if ((GetPlaybackMode() == PM_DVD && m_pDVDI)
			|| m_pDVS
			|| m_pSubStreams.size()) {
		ToggleSubtitleOnOff(true);
	}
}

void CMainFrame::OnStreamVideo(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
		if (pSS) {
			DWORD count = 0;
			if (SUCCEEDED(pSS->Count(&count)) && count >= 2) {
				std::vector<int> stms;
				int current = -1;

				for (DWORD i = 0; i < count; i++) {
					AM_MEDIA_TYPE* pmt = nullptr;
					DWORD dwFlags = DWORD_MAX;
					DWORD dwGroup = DWORD_MAX;
					if (FAILED(pSS->Info(i, &pmt, &dwFlags, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
						return;
					}

					if (pmt && pmt->majortype == MEDIATYPE_Video) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
							ASSERT(current  < 0);
							current = stms.size();
						}
						stms.emplace_back(i);
					}

					DeleteMediaType(pmt);
				}

				if (current >= 0 && stms.size() >= 2) {
					current += (nID == ID_STREAM_VIDEO_NEXT) ? 1 : -1;
					if (current >= (int)stms.size()) {
						current = 0;
					}
					else if (current < 0) {
						current = stms.size() - 1;
					}

					if (SUCCEEDED(pSS->Enable(stms[current], AMSTREAMSELECTENABLE_ENABLE))) {
						WCHAR* pszName = nullptr;
						if (SUCCEEDED(pSS->Info(stms[current], nullptr, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr)) && pszName) {
							CString stream_name = pszName;
							CoTaskMemFree(pszName);
							if (StartsWith(stream_name, L"Video - ")) {
								stream_name = stream_name.Mid(7);
							}
							CString strMessage;
							strMessage.Format(ResStr(IDS_VIDEO_STREAM), stream_name);
							m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
						}
					}
				}
			}
		}
		return;
	}

	if (GetPlaybackMode() == PM_DVD && m_pDVDI && m_pDVDC) {
		ULONG ulAnglesAvailable, ulCurrentAngle;
		if (SUCCEEDED(m_pDVDI->GetCurrentAngle(&ulAnglesAvailable, &ulCurrentAngle)) && ulAnglesAvailable > 1) {
			ulCurrentAngle += (nID == ID_STREAM_VIDEO_NEXT) ? 1 : -1;
			if (ulCurrentAngle > ulAnglesAvailable) {
				ulCurrentAngle = 1;
			} else if (ulCurrentAngle < 1) {
				ulCurrentAngle = ulAnglesAvailable;
			}
			m_pDVDC->SelectAngle(ulCurrentAngle, DVD_CMD_FLAG_Block, nullptr);

			CString osdMessage;
			osdMessage.Format(ResStr(IDS_AG_ANGLE), ulCurrentAngle);
			m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
		}
	}
}

//
// menu item handlers
//

// file

void CMainFrame::OnFileOpenQuick()
{
	if (m_eMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar) || !CanShowDialog()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	CString filter;
	std::vector<CString> mask;
	s.m_Formats.GetFilter(filter, mask);

	DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR;
	if (!s.bKeepHistory) {
		dwFlags |= OFN_DONTADDTORECENT;
	}

	COpenFileDialog fd(nullptr, nullptr, dwFlags, filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	std::list<CString> fns;
	fd.GetFilePaths(fns);

	bool bMultipleFiles = false;

	if (fns.size() > 1
			|| fns.size() == 1
			&& (fns.front()[fns.front().GetLength()-1] == '\\'
				|| fns.front()[fns.front().GetLength()-1] == '*')) {
		bMultipleFiles = true;
	}

	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	if (AddSimilarFiles(fns)) {
		bMultipleFiles = true;
	}

	m_wndPlaylistBar.Open(fns, bMultipleFiles);

	if (m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating()) {
		//ShowControlBarInternal(&m_wndPlaylistBar, FALSE);
	}

	OpenCurPlaylistItem();
}

void CMainFrame::OnFileOpenMedia()
{
	if (m_eMediaLoadState == MLS_LOADING || !CanShowDialog()) {
		return;
	}

	static COpenDlg dlg;
	if (IsWindow(dlg.GetSafeHwnd()) && dlg.IsWindowVisible()) {
		dlg.SetForegroundWindow();
		return;
	}
	if (dlg.DoModal() != IDOK || dlg.m_fns.empty()) {
		return;
	}

	if (!dlg.m_bAppendPlaylist) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	}

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	auto& fn = dlg.m_fns.front();
	if (OpenYoutubePlaylist(fn, dlg.m_bAppendPlaylist)) {
		return;
	}

	if (AddSimilarFiles(dlg.m_fns)) {
		dlg.m_bMultipleFiles = true;
	}

	if (dlg.m_bAppendPlaylist) {
		m_wndPlaylistBar.Append(dlg.m_fns, dlg.m_bMultipleFiles);
		return;
	}

	m_wndPlaylistBar.Open(dlg.m_fns, dlg.m_bMultipleFiles);
	OpenCurPlaylistItem();
}

void CMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState != MLS_LOADING);

	if (pCmdUI->m_nID == ID_FILE_OPENISO && pCmdUI->m_pMenu != nullptr && !m_DiskImage.DriveAvailable()) {
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID, MF_BYCOMMAND);
	}
}

LRESULT CMainFrame::OnMPCVRSwitchFullscreen(WPARAM wParam, LPARAM lParam)
{
	const auto& s = AfxGetAppSettings();
	m_bIsMPCVRExclusiveMode = static_cast<bool>(wParam);

	m_OSD.Stop();
	if (m_bIsMPCVRExclusiveMode) {
		if (m_wndPlaylistBar.IsVisible()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(true);
			ShowControlBarInternal(&m_wndPlaylistBar, FALSE);
		}
		ShowControls(CS_NONE, false);
		if (s.ShowOSD.Enable || s.bShowDebugInfo) {
			if (m_pMFVMB) {
				m_OSD.Start(m_pVideoWnd, m_pMFVMB);
			}
		}
	} else {
		if (s.ShowOSD.Enable) {
			m_OSD.Start(m_pOSDWnd);
		}
		if (m_wndPlaylistBar.IsHiddenDueToFullscreen()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(false);
			ShowControlBarInternal(&m_wndPlaylistBar, TRUE);
		}
	}

	FlyBarSetPos();
	OSDBarSetPos();

	return 0;
}

LRESULT CMainFrame::OnDXVAStateChange(WPARAM wParam, LPARAM lParam)
{
	if (m_wndToolBar && ::IsWindow(m_wndToolBar.GetSafeHwnd())) {
		m_wndToolBar.Invalidate();
	}

	return 0;
}

LRESULT CMainFrame::HandleCmdLine(WPARAM wParam, LPARAM lParam)
{
	if (m_bClosingState) {
		return 0;
	}

	CAppSettings& s = AfxGetAppSettings();

	cmdLine* cmdln = (cmdLine*)lParam;
	s.ParseCommandLine(*cmdln);

	if (s.nCLSwitches & CLSW_SLAVE) {
		SendAPICommand(CMD_CONNECT, L"%d", PtrToInt(GetSafeHwnd()));
	}

	for (const auto& filter : s.slFilters) {
		CString fullpath = MakeFullPath(filter);

		CString path = GetAddSlash(GetFolderPath(fullpath));

		WIN32_FIND_DATAW fd = {0};
		HANDLE hFind = FindFirstFileW(fullpath, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				CFilterMapper2 fm2(false);
				fm2.Register(path + fd.cFileName);

				for (auto& f : fm2.m_filters) {
					f->fTemporary = true;
					bool bFound = false;

					for (auto& f2 : s.m_ExternalFilters) {
						if (f2->type == FilterOverride::EXTERNAL && !f2->path.CompareNoCase(f->path)) {
							bFound = true;
							break;
						}
					}

					if (!bFound) {
						s.m_ExternalFilters.emplace_front(std::move(f));
					}
				}
			} while (FindNextFileW(hFind, &fd));

			FindClose(hFind);
		}
	}

	bool fSetForegroundWindow = false;

	auto applyRandomizeSwitch = [&]() {
		if (s.nCLSwitches & CLSW_RANDOMIZE) {
			m_wndPlaylistBar.Randomize();
			s.nCLSwitches &= ~CLSW_RANDOMIZE;
		}
	};

	if ((s.nCLSwitches & CLSW_DVD) && !s.slFiles.empty()) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		std::unique_ptr<OpenDVDData> p(DNew OpenDVDData());
		if (p && CheckDVD(s.slFiles.front())) {
			p->path = s.slFiles.front();
			p->subs = s.slSubs;
		}
		OpenMedia(std::move(p));
	} else if (s.nCLSwitches & CLSW_CD) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		std::list<CString> sl;

		if (!s.slFiles.empty()) {
			GetCDROMType(s.slFiles.front()[0], sl);
		} else {
			CString dir = GetCurrentDir();

			GetCDROMType(dir[0], sl);
			for (WCHAR drive = 'C'; sl.empty() && drive <= 'Z'; drive++) {
				GetCDROMType(drive, sl);
			}
		}

		m_wndPlaylistBar.Open(sl, true);
		applyRandomizeSwitch();
		OpenCurPlaylistItem();
	} else if (s.nCLSwitches & CLSW_CLIPBOARD) {
		std::list<CString> sl;
		if (GetFromClipboard(sl)) {
			if (sl.size() == 1 && OpenYoutubePlaylist(sl.front())) {
			} else {
				ParseDirs(sl);

				if (s.nCLSwitches & CLSW_ADD) {
					const auto nCount = m_wndPlaylistBar.GetCount();

					m_wndPlaylistBar.Append(sl, sl.size() > 1);
					applyRandomizeSwitch();

					if ((s.nCLSwitches & (CLSW_OPEN | CLSW_PLAY)) || !nCount) {
						fSetForegroundWindow = true;
						m_wndPlaylistBar.SetSelIdx(nCount, true);
						OpenCurPlaylistItem();
					}
				} else {
					fSetForegroundWindow = true;

					AddSimilarFiles(sl);

					m_wndPlaylistBar.Open(sl, sl.size() > 1, &s.slSubs);
					applyRandomizeSwitch();
					OpenCurPlaylistItem();

					s.nCLSwitches &= ~CLSW_STARTVALID;
					s.rtStart = 0;
				}
			}
		}
	} else if (s.nCLSwitches & CLSW_DEVICE) {
		SendMessageW(WM_COMMAND, ID_FILE_OPENDEVICE);
	} else if (!s.slFiles.empty()) {
		if (s.slFiles.size() == 1 && OpenYoutubePlaylist(s.slFiles.front())) {
		} else if (s.slFiles.size() == 1 && CheckDVD(s.slFiles.front())) {
			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			fSetForegroundWindow = true;

			std::unique_ptr<OpenDVDData> p(DNew OpenDVDData());
			if (p) {
				p->path = s.slFiles.front();
				p->subs = s.slSubs;
			}

			OpenMedia(std::move(p));
		} else {
			std::list<CString> sl;
			sl = s.slFiles;

			ParseDirs(sl);
			bool fMulti = sl.size() > 1;

			if (!fMulti) {
				for (const auto& dub : s.slDubs) {
					sl.emplace_back(dub);
				}
			}

			if (s.nCLSwitches & CLSW_ADD) {
				const auto nCount = m_wndPlaylistBar.GetCount();

				m_wndPlaylistBar.Append(sl, fMulti, &s.slSubs);
				applyRandomizeSwitch();

				if ((s.nCLSwitches & (CLSW_OPEN | CLSW_PLAY)) || !nCount) {
					fSetForegroundWindow = true;
					m_wndPlaylistBar.SetSelIdx(nCount, true);
					OpenCurPlaylistItem();
				}
			} else {
				fSetForegroundWindow = true;

				if (AddSimilarFiles(sl)) {
					fMulti = true;
				}

				m_wndPlaylistBar.Open(sl, fMulti, &s.slSubs);
				applyRandomizeSwitch();
				OpenCurPlaylistItem((s.nCLSwitches & CLSW_STARTVALID) ? s.rtStart : INVALID_TIME);

				s.nCLSwitches &= ~CLSW_STARTVALID;
				s.rtStart = 0;
			}
		}
	} else {
		applyRandomizeSwitch();

		if ((s.nCLSwitches & (CLSW_OPEN | CLSW_PLAY)) && m_wndPlaylistBar.GetCount() > 0) {
			OpenCurPlaylistItem();
		}

		// leave only CLSW_OPEN, if present
		s.nCLSwitches = (s.nCLSwitches & CLSW_OPEN) ? CLSW_OPEN : CLSW_NONE;
	}

	if (s.nCLSwitches & CLSW_VOLUME) {
		m_wndToolBar.SetVolume(s.nCmdVolume);
		s.nCLSwitches &= ~CLSW_VOLUME;
	}

	if (fSetForegroundWindow && !(s.nCLSwitches & CLSW_NOFOCUS)) {
		SetForegroundWindow();
	}
	s.nCLSwitches &= ~CLSW_NOFOCUS;

	UpdateThumbarButton();
	UpdateThumbnailClip();

	return 0;
}

ULONGLONG lastReceiveCmdLineTime = 0ULL;
void CMainFrame::cmdLineThreadFunction()
{
	HANDLE handles[2] = {
		m_ExitCmdLineQueue,
		m_EventCmdLineQueue,
	};

	for (;;) {
		const DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		switch (result) {
			case WAIT_OBJECT_0:     // exit event
				return;
			case WAIT_OBJECT_0 + 1: // new event
				{
					cmdLine cmdLine;

					std::unique_lock<std::mutex> lock(m_mutex_cmdLineQueue);
					if (!m_cmdLineQueue.empty()) {
						DLog(L"CMainFrame::cmdLineThreadFunction() : command line queue size - %zu", m_cmdLineQueue.size());
						const std::vector<BYTE>& pData = m_cmdLineQueue.front();
						const BYTE* p = pData.data();
						DWORD cnt = GETU32(p);
						p += sizeof(DWORD);
						const WCHAR* pBuff    = (WCHAR*)(p);
						const WCHAR* pBuffEnd = (WCHAR*)(p + pData.size() - sizeof(DWORD));

						while (cnt-- > 0) {
							CString str;
							while (pBuff < pBuffEnd && *pBuff) {
								str += *pBuff++;
							}
							pBuff++;
							cmdLine.emplace_back(str);
						}

						const ULONGLONG now = GetTickCount64();
						if ((now - lastReceiveCmdLineTime) <= 500ULL) {
							cmdLine.emplace_back(L"/add");
						}
						lastReceiveCmdLineTime = now;

						m_cmdLineQueue.pop_front();
						if (!m_cmdLineQueue.empty()) {
							m_EventCmdLineQueue.Set();
						}
					}
					lock.unlock();

					if (!cmdLine.empty()) {
						SendMessageW(WM_HANDLE_CMDLINE, 0, LPARAM(&cmdLine));
					}
				}
		}
	}
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS)
{
	if (m_bClosingState) {
		return FALSE;
	}

	if (pCDS->dwData != 0x6ABE51 || pCDS->cbData < sizeof(DWORD)) {
		if (AfxGetAppSettings().hMasterWnd) {
			ProcessAPICommand(pCDS);
			return TRUE;
		} else {
			return FALSE;
		}
	}

	std::vector<BYTE> pData(pCDS->cbData);
	memcpy(pData.data(), pCDS->lpData, pCDS->cbData);

	{
		std::unique_lock<std::mutex> lock(m_mutex_cmdLineQueue);
		m_cmdLineQueue.emplace_back(pData);
		m_EventCmdLineQueue.Set();
	}

	return TRUE;
}

void CMainFrame::OnFileOpenDVD()
{
	if (m_eMediaLoadState == MLS_LOADING || !CanShowDialog()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();
	CString path;

	CFileDialog dlg(TRUE);
	CComPtr<IFileOpenDialog> openDlgPtr = dlg.GetIFileOpenDialog();
	if (openDlgPtr != nullptr) {
		openDlgPtr->SetTitle(ResStr(IDS_MAINFRM_46));

		CComPtr<IShellItem> psiFolder;
		if (SUCCEEDED(afxGlobalData.ShellCreateItemFromParsingName(s.strDVDPath, nullptr, IID_PPV_ARGS(&psiFolder)))) {
			openDlgPtr->SetFolder(psiFolder);
		}

		openDlgPtr->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
		if (FAILED(openDlgPtr->Show(m_hWnd))) {
			return;
		}

		psiFolder.Release();
		if (SUCCEEDED(openDlgPtr->GetResult(&psiFolder))) {
			LPWSTR folderpath = nullptr;
			if(SUCCEEDED(psiFolder->GetDisplayName(SIGDN_FILESYSPATH, &folderpath))) {
				path = folderpath;
				CoTaskMemFree(folderpath);
			}
		}
	}

	if (!path.IsEmpty()) {
		if (CheckDVD(path) || CheckBD(path)) {
			s.strDVDPath = GetFolderPath(path);
			m_wndPlaylistBar.Open(path);
			OpenCurPlaylistItem();
		} else {
			if (m_eMediaLoadState == MLS_LOADED) {
				SendStatusMessage(ResStr(IDS_MAINFRM_93), 3000);
			} else {
				m_closingmsg = ResStr(IDS_MAINFRM_93);
			}
		}
	}
}

void CMainFrame::OnFileOpenIso()
{
	if (m_eMediaLoadState == MLS_LOADING || !CanShowDialog()) {
		return;
	}

	if (m_DiskImage.DriveAvailable()) {
		DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;

		CString szFilter;
		szFilter.Format(L"Image Files (%s)|%s||", m_DiskImage.GetExts(), m_DiskImage.GetExts());
		COpenFileDialog fd(nullptr, nullptr, dwFlags, szFilter);
		if (fd.DoModal() != IDOK) {
			return;
		}

		m_wndPlaylistBar.Open(fd.GetPathName());
		OpenCurPlaylistItem();
	}
}

void CMainFrame::OnFileOpenDevice()
{
	CAppSettings& s = AfxGetAppSettings();

	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	SetForegroundWindow();

	ShowWindow(SW_SHOW);

	ShowControlBarInternal(&m_wndPlaylistBar, FALSE);

	m_wndPlaylistBar.SelectDefaultPlaylist();
	m_wndPlaylistBar.Empty();

	std::unique_ptr<OpenDeviceData> p(DNew OpenDeviceData());
	if (p) {
		if (s.strAnalogVideo.IsEmpty() && s.strAnalogAudio.IsEmpty()) {
			BeginEnumSysDev(CLSID_VideoInputDeviceCategory, pMoniker) {
				LPOLESTR strName = nullptr;
				if (SUCCEEDED(pMoniker->GetDisplayName(nullptr, nullptr, &strName))) {
					p->DisplayName[0] = strName;
					CoTaskMemFree(strName);
					break;
				}
			}
			EndEnumSysDev
		}
		else {
			p->DisplayName[0] = s.strAnalogVideo;
			p->DisplayName[1] = s.strAnalogAudio;
		}
	}
	OpenMedia(std::move(p));
	if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && m_eMediaLoadState == MLS_LOADED && s.iDefaultCaptureDevice == 1) {
		m_wndNavigationBar.m_navdlg.UpdateElementList();
		ShowControlBarInternal(&m_wndNavigationBar, !s.fHideNavigation);
	}
}

void CMainFrame::OnFileOpenCD(UINT nID)
{
	nID -= ID_FILE_OPEN_CD_START;

	nID++;
	for (WCHAR drive = 'C'; drive <= 'Z'; drive++) {
		std::list<CString> sl;

		cdrom_t CDRom_t = GetCDROMType(drive, sl);

		switch (CDRom_t) {
			case CDROM_Audio:
			case CDROM_VideoCD:
			case CDROM_DVDVideo:
			case CDROM_BDVideo:
			case CDROM_DVDAudio:
				nID--;
				break;
			default:
				break;
		}

		if (nID == 0) {
			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			SetForegroundWindow();
			ShowWindow(SW_SHOW);

			m_wndPlaylistBar.Open(sl, true);
			OpenCurPlaylistItem();

			break;
		}
	}
}

void CMainFrame::OnFileReOpen()
{
	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	OpenCurPlaylistItem();
}

void CMainFrame::DropFiles(std::list<CString>& slFiles)
{
	SetForegroundWindow();

	if (slFiles.empty()) {
		return;
	}

	bool bIsValidSubExtAll = true;

	for (const auto& fname : slFiles) {
		const CString ext = GetFileExt(fname).Mid(1).MakeLower(); // extension without a dot

		const bool validate_ext = std::any_of(std::cbegin(Subtitle::s_SubFileExts), std::cend(Subtitle::s_SubFileExts), [&](LPCWSTR subExt) {
			return ext == subExt;
		});

		if (!validate_ext || (m_pDVS && ext == "mks")) {
			bIsValidSubExtAll = false;
			break;
		}
	}

	if (bIsValidSubExtAll && m_eMediaLoadState == MLS_LOADED) {
		if (m_pCAP || m_pDVS) {

			for (const auto& fname : slFiles) {
				bool b_SubLoaded = false;

				if (m_pDVS) {
					if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)fname))) {
						m_pDVS->put_SelectedLanguage(0);
						m_pDVS->put_HideSubtitles(true);
						m_pDVS->put_HideSubtitles(false);

						b_SubLoaded = true;
					}
				} else {
					ISubStream *pSubStream = nullptr;
					if (LoadSubtitle(fname, &pSubStream, false)) {
						SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
						b_SubLoaded = true;

						AddSubtitlePathsAddons(fname.GetString());
					}
				}

				if (b_SubLoaded) {
					SetToolBarSubtitleButton();

					SendStatusMessage(GetFileName(fname) + ResStr(IDS_MAINFRM_47), 3000);
					if (m_pDVS) {
						return;
					}
				}
			}
		} else {
			AfxMessageBox(ResStr(IDS_MAINFRM_60) + ResStr(IDS_MAINFRM_61) + ResStr(IDS_MAINFRM_64), MB_OK);
		}

		return;
	}

	if (m_wndPlaylistBar.IsWindowVisible()) {
		if (slFiles.size() == 1 && OpenYoutubePlaylist(slFiles.front(), TRUE)) {
			return;
		}
		AddSimilarFiles(slFiles);

		m_wndPlaylistBar.DropFiles(slFiles);
		return;
	}

	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	if (slFiles.size() == 1 && OpenYoutubePlaylist(slFiles.front())) {
		return;
	}

	ParseDirs(slFiles);
	AddSimilarFiles(slFiles);

	m_wndPlaylistBar.Open(slFiles, true);
	OpenCurPlaylistItem();
}

void CMainFrame::OnFileSaveAs()
{
	if (!CanShowDialog()) {
		return;
	}

	CStringW ext;
	CStringW ext_list;
	CStringW in = m_strPlaybackRenderedPath;
	CStringW out = in;

	if (!m_youtubeFields.fname.IsEmpty()) {
		out = GetAltFileName();
		ext = GetFileExt(out).MakeLower();
	} else {
		if (!::PathIsURLW(out)) {
			ext = GetFileExt(out).MakeLower();
			out = GetFileName(out);
			if (ext == L".cda") {
				RenameFileExt(out, L".wav");
			} else if (ext == L".ifo") {
				RenameFileExt(out, L".vob");
			}
		} else {
			CString fname = L"streaming_saved";

			CUrlParser urlParser(out.GetString());
			if (urlParser.GetUrlPathLength() > 1) {
				fname = urlParser.GetUrlPath();
			}

			out = fname.Mid(fname.ReverseFind('/') + 1);
			FixFilename(out);

			if (GetFileExt(out).IsEmpty()) {
				ext = L".ts";
				out += ext;
			}
		}
	}

	if (!ext.IsEmpty()) {
		CMediaFormatCategory* mfc = AfxGetAppSettings().m_Formats.FindMediaByExt(ext);
		if (mfc) {
			ext_list.Format(L"%s|*%s|", mfc->GetDescription(), ext);
		} else {
			ext_list.Format(L"Media (*%s)|*%s|", ext, ext);
		}
	}
	ext_list.Append(ResStr(IDS_AG_ALLFILES) + L" (*.*)|*.*||");

	CSaveFileDialog fd(ext.GetLength() ? ext.GetString() : nullptr, out,
				   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
				   ext_list, GetModalParent());

	if (fd.DoModal() != IDOK) {
		return;
	}
	CStringW savedFileName(fd.GetPathName());
	if (in.CompareNoCase(savedFileName) == 0) {
		return;
	}

	if (ext.GetLength() && GetFileExt(savedFileName).IsEmpty()) {
		savedFileName.Append(ext);
	}

	OAFilterState fs = State_Stopped;
	m_pMC->GetState(0, &fs);

	if (fs == State_Running) {
		m_pMC->Pause();
	}

	const CAppSettings& s = AfxGetAppSettings();
	std::list<CSaveTaskDlg::SaveItem_t> saveItems;
	CString ffmpegpath;

	if (m_youtubeFields.fname.GetLength()) {
		if (m_bAudioOnly) {
			saveItems.emplace_back('a', in, GetAltFileName(), "");
			if (ext == L".m4a" || ext == L".mka") {
				auto thumbnail_ext = m_youtubeFields.thumbnailUrl.Mid(m_youtubeFields.thumbnailUrl.ReverseFind('.')).MakeLower();
				if (thumbnail_ext == L".jpg" || thumbnail_ext == L".webp") {
					saveItems.emplace_back('t', m_youtubeFields.thumbnailUrl, thumbnail_ext, "");
					ffmpegpath = GetFullExePath(s.strFFmpegExePath, true);
				}
			}
		}
		else {
			saveItems.emplace_back('v', in, GetAltFileName(), "");

			const auto pFileData = dynamic_cast<OpenFileData*>(m_lastOMD.get());
			if (pFileData) {
				if (pFileData->auds.size()) {
					for (const auto& aud : pFileData->auds) {
						saveItems.emplace_back('a', aud.GetPath(), aud.GetTitle(), "");
					}
					ffmpegpath = GetFullExePath(s.strFFmpegExePath, true);
				}

				for (const auto& sub : pFileData->subs) {
					if (sub.GetPath().Find(L"fmt=vtt") > 0) {
						saveItems.emplace_back('s', sub.GetPath(), sub.GetTitle(), sub.GetLang());
					}
				}
			}
		}
	}
	else {
		saveItems.emplace_back(0, in, in, "");
	}

	HRESULT hr = S_OK;
	CSaveTaskDlg save_dlg(saveItems, savedFileName, hr);

	if (SUCCEEDED(hr)) {
		save_dlg.SetFFmpegPath(ffmpegpath);
		save_dlg.SetLangDefault(CStringA(s.strYoutubeAudioLang));
		save_dlg.DoModal();
	}

	if (fs == State_Running) {
		m_pMC->Run();
	}
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	if (m_eMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_FILE) {
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(TRUE);
}

HRESULT GetBasicVideoFrame(IBasicVideo* pBasicVideo, CSimpleBlock<BYTE>& dib)
{
	// IBasicVideo::GetCurrentImage() gives the original frame

	long size;

	HRESULT hr = pBasicVideo->GetCurrentImage(&size, nullptr);
	if (FAILED(hr)) {
		return hr;
	}
	if (size <= 0) {
		return E_ABORT;
	}

	dib.SetSize(size);

	hr = pBasicVideo->GetCurrentImage(&size, (long*)dib.Data());
	if (FAILED(hr)) {
		dib.SetSize(0);
	}

	return hr;
}

HRESULT GetVideoDisplayControlFrame(IMFVideoDisplayControl* pVideoDisplayControl, CSimpleBlock<BYTE>& dib)
{
	// IMFVideoDisplayControl::GetCurrentImage() gives the displayed frame

	BITMAPINFOHEADER	bih = {sizeof(BITMAPINFOHEADER)};
	BYTE*				pDib;
	DWORD				size;
	REFERENCE_TIME		rtImage = 0;

	HRESULT hr = pVideoDisplayControl->GetCurrentImage(&bih, &pDib, &size, &rtImage);
	if (S_OK != hr) {
		return hr;
	}
	if (size == 0) {
		return E_ABORT;
	}

	dib.SetSize(sizeof(BITMAPINFOHEADER) + size);

	memcpy(dib.Data(), &bih, sizeof(BITMAPINFOHEADER));
	memcpy(dib.Data() + sizeof(BITMAPINFOHEADER), pDib, size);
	CoTaskMemFree(pDib);

	return hr;
}

HRESULT GetMadVRFrameGrabberFrame(IMadVRFrameGrabber* pMadVRFrameGrabber, CSimpleBlock<BYTE>& dib, bool displayed)
{
	LPVOID dibImage = nullptr;
	HRESULT hr;

	if (displayed) {
		hr = pMadVRFrameGrabber->GrabFrame(ZOOM_PLAYBACK_SIZE, 0, 0, 0, 0, 0, &dibImage, 0);
	} else {
		hr = pMadVRFrameGrabber->GrabFrame(ZOOM_ENCODED_SIZE, 0, 0, 0, 0, 0, &dibImage, 0);
	}

	if (S_OK != hr) {
		return hr;
	}
	if (!dibImage) {
		return E_ABORT;
	}

	const BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)dibImage;

	dib.SetSize(sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
	memcpy(dib.Data(), dibImage, sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
	LocalFree(dibImage);

	return hr;
}

HRESULT CMainFrame::GetDisplayedImage(CSimpleBlock<BYTE>& dib, CString& errmsg)
{
	errmsg.Empty();
	HRESULT hr;

	if (m_pCAP) {
		LPVOID dibImage = nullptr;
		hr = m_pCAP->GetDisplayedImage(&dibImage);

		if (S_OK == hr && dibImage) {
			const BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)dibImage;
			dib.SetSize(sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
			memcpy(dib.Data(), dibImage, sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
			LocalFree(dibImage);
		}
	}
	else if (m_pMFVDC) {
		hr = GetVideoDisplayControlFrame(m_pMFVDC, dib);
	} else {
		hr = E_NOINTERFACE;
	}

	if (FAILED(hr)) {
		errmsg.Format(L"CMainFrame::GetCurrentImage() failed, 0x%08x", hr);
	}

	return hr;
}

HRESULT CMainFrame::GetCurrentFrame(CSimpleBlock<BYTE>& dib, CString& errmsg)
{
	HRESULT hr = S_OK;
	errmsg.Empty();

	OAFilterState fs = GetMediaState();
	if (m_eMediaLoadState != MLS_LOADED || m_bAudioOnly || (fs != State_Paused && fs != State_Running)) {
		return E_ABORT;
	}

	if (fs == State_Running && !m_pCAP) {
		m_pMC->Pause();
		GetMediaState(); // wait for completion of the pause command
	}

	if (m_pCAP) {
		DWORD size;
		hr = m_pCAP->GetDIB(nullptr, &size);

		if (S_OK == hr) {
			dib.SetSize(size);
			hr = m_pCAP->GetDIB(dib.Data(), &size);
		}

		if (FAILED(hr)) {
			errmsg.Format(L"IAllocatorPresenter::GetDIB() failed, 0x%08x", hr);
		}
	}
	else if (m_pBV) {
		hr = GetBasicVideoFrame(m_pBV, dib);

		if (hr == E_NOINTERFACE && m_pMFVDC) {
			// hmm, EVR is not able to give the original frame, giving the displayed image
			hr = GetDisplayedImage(dib, errmsg);
		}
		else if (FAILED(hr)) {
			errmsg.Format(L"IBasicVideo::GetCurrentImage() failed, 0x%08x", hr);
		}
	}
	else {
		hr = E_POINTER;
		errmsg.Format(L"Interface not found!");
	}

	if (fs == State_Running && GetMediaState() != State_Running) {
		m_pMC->Run();
	}

	return hr;
}

HRESULT CMainFrame::GetOriginalFrame(CSimpleBlock<BYTE>& dib, CString& errmsg)
{
	HRESULT hr = S_OK;
	errmsg.Empty();

	if (m_pMVRFG) {
		hr = GetMadVRFrameGrabberFrame(m_pMVRFG, dib, false);
		if (FAILED(hr)) {
			errmsg.Format(L"IMadVRFrameGrabber::GrabFrame() failed, 0x%08x", hr);
		}
	} else {
		hr = GetCurrentFrame(dib, errmsg);
	}

	return hr;
}

HRESULT CMainFrame::RenderCurrentSubtitles(BYTE* pData)
{
	CheckPointer(pData, E_FAIL);
	HRESULT hr = S_FALSE;

	const CAppSettings& s = AfxGetAppSettings();
	if (s.fEnableSubtitles && s.bSnapShotSubtitles) {
		if (CComQIPtr<ISubPicProvider> pSubPicProvider = m_pCurrentSubStream.p) {
			const PBITMAPINFOHEADER bih = (PBITMAPINFOHEADER)pData;
			const int width  = bih->biWidth;
			const int height = bih->biHeight;

			SubPicDesc spdRender = {};
			spdRender.w       = width;
			spdRender.h       = abs(height);
			spdRender.bpp     = 32;
			spdRender.pitch   = width * 4;
			spdRender.vidrect = {0, 0, width, height};
			spdRender.bits    = DNew BYTE[spdRender.pitch * spdRender.h];

			REFERENCE_TIME rtNow = 0;
			m_pMS->GetCurrentPosition(&rtNow);

			CMemSubPic memSubPic(spdRender);
			memSubPic.ClearDirtyRect();

			RECT bbox = {};
			hr = pSubPicProvider->Render(spdRender, rtNow, m_pCAP->GetFPS(), bbox);
			if (S_OK == hr) {
				SubPicDesc spdTarget = {};
				spdTarget.w       = width;
				spdTarget.h       = height;
				spdTarget.bpp     = 32;
				spdTarget.pitch   = -width * 4;
				spdTarget.vidrect = {0, 0, width, height};
				spdTarget.bits    = (BYTE*)(bih + 1) + (width * 4) * (height - 1);

				hr = memSubPic.AlphaBlt(&spdRender.vidrect, &spdTarget.vidrect, &spdTarget);
			}
		}
	}

	return hr;
}

bool CMainFrame::SaveDIB(LPCWSTR fn, BYTE* pData, long size)
{
	const CAppSettings& s = AfxGetAppSettings();

	const CString ext = GetFileExt(fn).MakeLower();

	bool ok;

	if (ext == L".png") {
		ok = SaveDIB_libpng(fn, pData, std::clamp(s.iThumbLevelPNG, 1, 9));
	} else {
		size_t dstLen = 0;
		ok = SaveDIB_WIC(fn, pData, s.iThumbQuality, nullptr, dstLen);
	}

	return ok;
}

void CMainFrame::SaveImage(LPCWSTR fn, bool displayed)
{
	CSimpleBlock<BYTE> dib;
	CString errmsg;
	HRESULT hr;
	if (displayed) {
		hr = GetDisplayedImage(dib, errmsg);
	} else {
		hr = GetCurrentFrame(dib, errmsg);
		if (hr == S_OK) {
			RenderCurrentSubtitles(dib.Data());
		}
	}

	if (hr == S_OK && dib.Size()) {
		if (fn) {
			bool ok = SaveDIB(fn, dib.Data(), dib.Size());
			if (ok) {
				SendStatusCompactPath(fn);
				m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_IMAGE_SAVED), 3000);
			}
		} else {
			CopyDataToClipboard(this->m_hWnd, CF_DIB, dib.Data(), dib.Size());
		}
	}
	else {
		m_OSD.DisplayMessage(OSD_TOPLEFT, errmsg, 3000);
	}
}

void CMainFrame::SaveThumbnails(LPCWSTR fn)
{
	if (!m_pMC || !m_pMS || GetPlaybackMode() != PM_FILE /*&& GetPlaybackMode() != PM_DVD*/) {
		return;
	}

	if (GetDur() <= 0) {
		AfxMessageBox(ResStr(IDS_MAINFRM_54));
		return;
	}

	const auto ms = GetMediaState();
	if (ms != State_Paused) {
		SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		GetMediaState(); // wait for completion of the pause command
	}

	const REFERENCE_TIME rtPos = GetPos();

	int volume = m_wndToolBar.Volume;
	m_pBA->put_Volume(-10000);
	m_nVolumeBeforeFrameStepping = -10000;

	m_wndSeekBar.LockWindowUpdate();

	CThumbsTaskDlg dlg(fn);
	dlg.DoModal();

	m_wndSeekBar.UnlockWindowUpdate();

	m_pBA->put_Volume(volume);
	m_nVolumeBeforeFrameStepping = volume;

	SeekTo(rtPos, false);
	if (ms == State_Running) {
		SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
	}

	if (dlg.IsCompleteOk()) {
		SendStatusCompactPath(fn);
		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_THUMBS_SAVED), 3000);
	}
}

static CStringW MakeSnapshotFileName(LPCWSTR prefix)
{
	CTime t = CTime::GetCurrentTime();
	CStringW fn;
	fn.Format(L"%s_[%s]%s", prefix, t.Format(L"%Y-%m-%d_%H.%M.%S"), AfxGetAppSettings().strSnapShotExt);
	return fn;
}

bool CMainFrame::IsRendererCompatibleWithSaveImage()
{
	bool result = true;

	if (m_bShockwaveGraph) {
		AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_SHOCKWAVE), MB_ICONEXCLAMATION | MB_OK);
		result = false;
	}
	// the latest madVR build v0.84.0 now supports screenshots.
	else if (m_clsidCAP == CLSID_madVRAllocatorPresenter) {
		CRegKey key;
		CString clsid = L"{E1A8B82A-32CE-4B0D-BE0D-AA68C772E423}";

		WCHAR buff[256];
		ULONG len = std::size(buff);

		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + clsid + L"\\InprocServer32", KEY_READ)
				&& ERROR_SUCCESS == key.QueryStringValue(nullptr, buff, &len)) {

			const FileVersion::Ver madVR_ver = FileVersion::GetVer(buff);
			if (madVR_ver.value < FileVersion::Ver(0, 84, 0, 0).value) {
				result = false;
			}

			key.Close();
		}

		if (!result) {
			AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_MADVR), MB_ICONEXCLAMATION | MB_OK);
		}
	}

	return result;
}

CStringW CMainFrame::GetVidPos()
{
	CStringW str;

	if ((GetPlaybackMode() == PM_FILE) || (GetPlaybackMode() == PM_DVD)) {
		REFERENCE_TIME stop = m_wndSeekBar.GetRange();
		REFERENCE_TIME pos = m_wndSeekBar.GetPosReal();

		TimeCode_t tcNow = ReftimeToTimecode(pos);
		auto DurHours = stop / 36000000000ll;

		if (DurHours > 0 || (pos >= stop && tcNow.Hours > 0)) {
			str.Format(L"%02d.%02d.%02d", tcNow.Hours, tcNow.Minutes, tcNow.Seconds);
		} else {
			str.Format(L"%02d.%02d", tcNow.Minutes, tcNow.Seconds);
		}

		if (m_bShowMilliSecs) {
			str.AppendFormat(L".%03d", tcNow.Milliseconds);
		}
	}

	return str;
}

CStringW CMainFrame::CreateSnapShotFileName()
{
	CStringW path(AfxGetAppSettings().strSnapShotPath);
	if (!::PathFileExistsW(path)) {
		PWSTR pathPictures = nullptr;
		SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pathPictures);

		path = CString(pathPictures) + L"\\";
		CoTaskMemFree(pathPictures);
	}

	CStringW filename = L"snapshot";

	if (GetPlaybackMode() == PM_FILE) {
		if (m_youtubeFields.fname.GetLength()) {
			filename = GetAltFileName();
		}else {
			filename = GetFileName(GetCurFileName());
			FixFilename(filename); // need for URLs
		}
		filename.AppendFormat(L"_snapshot_%s", GetVidPos());
	}
	else if (GetPlaybackMode() == PM_DVD) {
		filename.Format(L"snapshot_dvd_%s", GetVidPos());
	}
	else {
		filename = L"snapshot";
	}

	CombineFilePath(path, MakeSnapshotFileName(filename));

	return path;
}

void CMainFrame::OnFileSaveImage()
{
	if (!CanShowDialog() || !IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	CSaveImageDialog fd(
		s.iThumbQuality, s.iThumbLevelPNG, s.bSnapShotSubtitles, m_pCurrentSubStream != nullptr,
		nullptr, CreateSnapShotFileName(),
		L"BMP - Windows Bitmap (*.bmp)|*.bmp|JPG - JPEG Image (*.jpg)|*.jpg|PNG - Portable Network Graphics (*.png)|*.png||", GetModalParent());

	if (s.strSnapShotExt == L".bmp") {
		fd.m_pOFN->nFilterIndex = 1;
	} else if (s.strSnapShotExt == L".jpg") {
		fd.m_pOFN->nFilterIndex = 2;
	} else if (s.strSnapShotExt == L".png") {
		fd.m_pOFN->nFilterIndex = 3;
	}

	if (fd.DoModal() != IDOK) {
		return;
	}

	if (fd.m_pOFN->nFilterIndex == 1) {
		s.strSnapShotExt = L".bmp";
	} else if (fd.m_pOFN->nFilterIndex == 2) {
		s.strSnapShotExt = L".jpg";
	} else if (fd.m_pOFN->nFilterIndex == 3) {
		s.strSnapShotExt = L".png";
	}

	s.iThumbQuality      = std::clamp(fd.m_JpegQuality, 70, 100);
	s.iThumbLevelPNG     = std::clamp(fd.m_PngCompression, 1, 9);
	s.bSnapShotSubtitles = fd.m_bDrawSubtitles;

	CStringW pdst = fd.GetPathName();
	if (GetFileExt(pdst).MakeLower() != s.strSnapShotExt) {
		RenameFileExt(pdst, s.strSnapShotExt);
	}

	s.strSnapShotPath = GetFolderPath(pdst);
	SaveImage(pdst, false);
}

void CMainFrame::OnUpdateFileSaveImage(CCmdUI* pCmdUI)
{
	OAFilterState fs = GetMediaState();
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly && (fs == State_Paused || fs == State_Running));
}

void CMainFrame::OnAutoSaveImage()
{
	// Check if a compatible renderer is being used
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	SaveImage(CreateSnapShotFileName(), false);
}

void CMainFrame::OnAutoSaveDisplay()
{
	// Check if a compatible renderer is being used
	if (!m_pCAP && !m_pMFVDC) {
		return;
	}

	SaveImage(CreateSnapShotFileName(), true);
}

void CMainFrame::OnCopyImage()
{
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	SaveImage(nullptr, false);
}

void CMainFrame::OnFileSaveThumbnails()
{
	if (!CanShowDialog() || !IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	CStringW prefix = L"thumbs";
	if (GetPlaybackMode() == PM_FILE) {
		CString path = GetFileName(GetCurFileName());
		if (!m_youtubeFields.fname.IsEmpty()) {
			path = GetAltFileName();
		}

		prefix.Format(L"%s_thumbs", path);
	}
	CStringW psrc = GetCombineFilePath(s.strSnapShotPath, MakeSnapshotFileName(prefix));

	CSaveThumbnailsDialog fd(
		s.iThumbRows, s.iThumbCols, s.iThumbWidth, s.iThumbQuality, s.iThumbLevelPNG, s.bSnapShotSubtitles, m_pCurrentSubStream != nullptr,
		nullptr, psrc,
		L"BMP - Windows Bitmap (*.bmp)|*.bmp|JPG - JPEG Image (*.jpg)|*.jpg|PNG - Portable Network Graphics (*.png)|*.png||", GetModalParent());

	if (s.strSnapShotExt == L".bmp") {
		fd.m_pOFN->nFilterIndex = 1;
	} else if (s.strSnapShotExt == L".jpg") {
		fd.m_pOFN->nFilterIndex = 2;
	} else if (s.strSnapShotExt == L".png") {
		fd.m_pOFN->nFilterIndex = 3;
	}

	if (fd.DoModal() != IDOK) {
		return;
	}

	if (fd.m_pOFN->nFilterIndex == 1) {
		s.strSnapShotExt = L".bmp";
	} else if (fd.m_pOFN->nFilterIndex == 2) {
		s.strSnapShotExt = L".jpg";
	} else if (fd.m_pOFN->nFilterIndex == 3) {
		s.strSnapShotExt = L".png";
	}

	s.iThumbRows         = std::clamp(fd.m_rows, 1, 20);
	s.iThumbCols         = std::clamp(fd.m_cols, 1, 10);
	s.iThumbWidth        = std::clamp(fd.m_width, APP_THUMBWIDTH_MIN, APP_THUMBWIDTH_MAX);
	s.iThumbQuality      = std::clamp(fd.m_JpegQuality, 70, 100);
	s.iThumbLevelPNG     = std::clamp(fd.m_PngCompression, 1, 9);
	s.bSnapShotSubtitles = fd.m_bDrawSubtitles;

	CStringW pdst = fd.GetPathName();
	if (GetFileExt(pdst).MakeLower() != s.strSnapShotExt) {
		RenameFileExt(pdst, s.strSnapShotExt);
	}

	s.strSnapShotPath = GetFolderPath(pdst);
	SaveThumbnails(pdst);
}

void CMainFrame::OnUpdateFileSaveThumbnails(CCmdUI* pCmdUI)
{
	OAFilterState fs = GetMediaState();
	UNREFERENCED_PARAMETER(fs);

	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED
					&& !m_bAudioOnly
					&& (GetPlaybackMode() == PM_FILE /*|| GetPlaybackMode() == PM_DVD*/)
					&& !GetBufferingProgress());

	if (GetPlaybackMode() == PM_FILE) {
		REFERENCE_TIME total = 0;
		if (m_pMS) {
			m_pMS->GetDuration(&total);
		}
		if (!total) {
			pCmdUI->Enable(FALSE);
		}
	}
}

void CMainFrame::OnFileLoadSubtitle()
{
	if (!CanShowDialog()) {
		return;
	}
	if (!m_pCAP && !m_pDVS) {
		AfxMessageBox(ResStr(IDS_MAINFRM_60)+
					  ResStr(IDS_MAINFRM_61)+
					  ResStr(IDS_MAINFRM_64)
					  , MB_OK);
		return;
	}

	CStringW exts;
	for (const auto& subext : Subtitle::s_SubFileExts) {
		exts.AppendFormat(L"*.%s;", subext);
	}
	exts.TrimRight(';');

	CStringW filter;
	filter.Format(L"%s (%s)|%s||", ResStr(IDS_AG_SUBTITLEFILES), exts, exts);

	std::vector<CStringW> mask;
	for (const auto& subExt : Subtitle::s_SubFileExts) {
		mask.emplace_back(L"*." + CString(subExt));
	}

	COpenFileDialog fd(nullptr, GetCurFileName(),
					OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT,
					filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	std::list<CStringW> fns;
	fd.GetFilePaths(fns);

	if (m_pDVS) {
		if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)(*fns.cbegin())))) {
			m_pDVS->put_SelectedLanguage(0);
			m_pDVS->put_HideSubtitles(true);
			m_pDVS->put_HideSubtitles(false);
		}
	} else {
		for (const auto& fn : fns) {
			ISubStream *pSubStream = nullptr;
			if (LoadSubtitle(fn, &pSubStream, false)) {
				SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
				AddSubtitlePathsAddons(fn.GetString());
			}
		}
	}

	SetToolBarSubtitleButton();
}

void CMainFrame::OnUpdateFileLoadSubtitle(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && /*m_pCAP &&*/ !m_bAudioOnly);
	pCmdUI->Enable((m_pCAP || m_pDVS) && !m_bAudioOnly && GetPlaybackMode() != PM_DVD);
}

void CMainFrame::OnFileLoadAudio()
{
	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	CString filter;
	std::vector<CString> mask;
	s.m_Formats.GetAudioFilter(filter, mask);

	COpenFileDialog fd(nullptr, GetCurFileName(),
					OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT,
					filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	CPlaylistItem* pli = m_wndPlaylistBar.GetCur();
	if (pli && pli->m_fi.Valid()) {
		std::list<CString> fns;
		fd.GetFilePaths(fns);

		for (const auto& fname : fns) {
			if (!m_wndPlaylistBar.CheckAudioInCurrent(fname)) {
				if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB.p) {
					HRESULT hr = pGBA->RenderAudioFile(fname);
					if (SUCCEEDED(hr)) {
						m_wndPlaylistBar.AddAudioToCurrent(fname);
						AddAudioPathsAddons(fname.GetString());

						CComQIPtr<IAMStreamSelect> pSS = FindSwitcherFilter();
						if (pSS) {
							DWORD cStreams = 0;
							if (SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0) {
								pSS->Enable(cStreams - 1, AMSTREAMSELECTENABLE_ENABLE);
							}
						}
					}
				}
			}
		}
	}

	m_pSwitcherFilter.Release();
	m_pSwitcherFilter = FindSwitcherFilter();

	SetToolBarAudioButton();
}

void CMainFrame::OnUpdateFileLoadAudio(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetPlaybackMode() == PM_FILE && !m_bAudioOnly);
}

void CMainFrame::OnFileSaveSubtitle()
{
	if (m_pCurrentSubStream && CanShowDialog()) {
		OpenMediaData *pOMD = m_wndPlaylistBar.GetCurOMD();
		CString suggestedFileName("");
		if (OpenFileData* pFileData = dynamic_cast<OpenFileData*>(pOMD)) {
			// HACK: get the file name from the current playlist item
			suggestedFileName = GetCurFileName();
			if (IsLikelyFilePath(suggestedFileName)) {
				suggestedFileName = suggestedFileName.Left(suggestedFileName.ReverseFind('.')); // exclude the extension, it will be auto completed
			} else {
				suggestedFileName = L"subtitle";
			}
		}

		if (auto pVSF = dynamic_cast<CVobSubFile*>(m_pCurrentSubStream.p)) {
			// remember to set lpszDefExt to the first extension in the filter so that the save dialog autocompletes the extension
			// and tracks attempts to overwrite in a graceful manner
			CSaveFileDialog fd(L"idx", suggestedFileName,
						   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
						   L"VobSub (*.idx, *.sub)|*.idx;*.sub||", GetModalParent());

			if (fd.DoModal() == IDOK) {
				CAutoLock cAutoLock(&m_csSubLock);
				pVSF->Save(fd.GetPathName());
			}

			return;
		}

		if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
			const Subtitle::SubType types[] = {
				Subtitle::SRT,
				Subtitle::SSA,
				Subtitle::ASS,
				Subtitle::SUB,
				Subtitle::SMI,
				Subtitle::PSB
			};

			CString filter;
			filter += L"SubRip (*.srt)|*.srt|";
			filter += L"SubStation Alpha (*.ssa)|*.ssa|";
			filter += L"Advanced SubStation Alpha (*.ass)|*.ass|";
			filter += L"MicroDVD (*.sub)|*.sub|";
			filter += L"SAMI (*.smi)|*.smi|";
			filter += L"PowerDivX (*.psb)|*.psb|";
			filter += L"|";

			CAppSettings& s = AfxGetAppSettings();

			// same thing as in the case of CVobSubFile above for lpszDefExt
			CSaveTextFileDialog fd(pRTS->m_encoding, L"srt", suggestedFileName, filter, GetModalParent(), FALSE, s.bSubSaveExternalStyleFile);

			if (fd.DoModal() == IDOK) {
				s.bSubSaveExternalStyleFile = !!fd.GetSaveExternalStyleFile();

				CAutoLock cAutoLock(&m_csSubLock);
				pRTS->SaveAs(fd.GetPathName(), types[fd.m_ofn.nFilterIndex - 1], m_pCAP->GetFPS(), m_pCAP->GetSubtitleDelay(), fd.GetEncoding(), s.bSubSaveExternalStyleFile);
			}

			return;
		}
	}
}

void CMainFrame::OnUpdateFileSaveSubtitle(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;

	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
		bEnable = !pRTS->IsEmpty();
	} else if (dynamic_cast<CVobSubFile*>(m_pCurrentSubStream.p)) {
		bEnable = TRUE;
	}

	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnFileProperties()
{
	if (!CanShowDialog()) {
		return;
	}

	std::list<CString> files;
	if (GetPlaybackMode() == PM_FILE) {
		if (m_youtubeFields.fname.GetLength()) {
			files.emplace_back(m_wndPlaylistBar.GetCurFileName());
		}
		else {
			BeginEnumFilters(m_pGB, pEF, pBF)
			{
				if (CComQIPtr<IFileSourceFilter>pFSF = pBF.p) {
					LPOLESTR pFN = nullptr;
					if (SUCCEEDED(pFSF->GetCurFile(&pFN, nullptr)) && pFN && *pFN) {
						files.emplace_front(pFN);
					}
				}
			}
			EndEnumFilters;

			if (files.empty()) {
				ASSERT(0);
				files.emplace_back(m_strPlaybackRenderedPath);
			}
		}
	}
	else {
		files.emplace_back(GetCurDVDPath(TRUE));
	}

	CPPageFileInfoSheet fileInfo(files, this, GetModalParent());
	fileInfo.DoModal();
}

void CMainFrame::OnUpdateFileProperties(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED &&
				   (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD));
}

void CMainFrame::OnFileCloseMedia()
{
	CloseMedia();
}

void CMainFrame::OnRepeatForever()
{
	CAppSettings& s = AfxGetAppSettings();
	s.fLoopForever = !s.fLoopForever;

	m_OSD.DisplayMessage(OSD_TOPLEFT,
						 s.fLoopForever ? ResStr(IDS_REPEAT_FOREVER_ON) : ResStr(IDS_REPEAT_FOREVER_OFF));
}

void CMainFrame::OnUpdateViewTearingTest(CCmdUI* pCmdUI)
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter || m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetRenderersSettings().ExtraSets.bTearingTest);

		return;
	}

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(0);
}

void CMainFrame::OnViewTearingTest()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.ExtraSets.bTearingTest = !rs.ExtraSets.bTearingTest;

	if (m_pCAP) {
		m_pCAP->SetExtraSettings(&rs.ExtraSets);
	}
}

void CMainFrame::OnUpdateViewDisplayStats(CCmdUI* pCmdUI)
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter || m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetRenderersSettings().ExtraSets.iDisplayStats > 0);

		return;
	}

	if (m_clsidCAP == CLSID_MPCVRAllocatorPresenter) {
		CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pCAP.p;
		if (pIExFilterConfig) {
			bool statsEnable = 0;
			if (S_OK == pIExFilterConfig->Flt_GetBool("statsEnable", &statsEnable)) {
				pCmdUI->Enable(TRUE);
				pCmdUI->SetCheck(statsEnable);

				return;
			}
		}
	}

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(0);
}

void CMainFrame::OnViewResetStats()
{
	if (m_pCAP) {
		m_pCAP->ResetStats();
	}
}

void CMainFrame::OnViewDisplayStatsSC()
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter || m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		CRenderersSettings& rs = GetRenderersSettings();
		if (!rs.ExtraSets.iDisplayStats && m_pCAP) {
			m_pCAP->ResetStats(); // to Reset statistics on first call ...
		}

		++rs.ExtraSets.iDisplayStats;
		if (rs.ExtraSets.iDisplayStats > 3) {
			rs.ExtraSets.iDisplayStats = 0;
		}

		if (m_pCAP) {
			m_pCAP->SetExtraSettings(&rs.ExtraSets);
		}

		RepaintVideo();
	}
	else if (m_clsidCAP == CLSID_MPCVRAllocatorPresenter) {
		CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pCAP.p;
		if (pIExFilterConfig) {
			bool statsEnable = 0;
			if (S_OK == pIExFilterConfig->Flt_GetBool("statsEnable", &statsEnable)) {
				statsEnable = !statsEnable;
				pIExFilterConfig->Flt_SetBool("statsEnable", statsEnable);
			}
		}
	}
}

void CMainFrame::OnUpdateViewD3DFullscreen(CCmdUI* pCmdUI)
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetRenderersSettings().bExclusiveFullscreen);

		return;
	}

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(0);
}

void CMainFrame::OnUpdateViewEnableFrameTimeCorrection(CCmdUI* pCmdUI)
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetRenderersSettings().ExtraSets.bEVRFrameTimeCorrection);

		return;
	}

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(0);
}

void CMainFrame::OnViewVSync()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.ExtraSets.bVSync = !rs.ExtraSets.bVSync;
	rs.Save();
	if (m_pCAP) {
		m_pCAP->SetExtraSettings(&rs.ExtraSets);
	}
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.ExtraSets.bVSync ? ResStr(IDS_OSD_RS_VSYNC_ON) : ResStr(IDS_OSD_RS_VSYNC_OFF));
}

void CMainFrame::OnViewVSyncInternal()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.ExtraSets.bVSyncInternal = !rs.ExtraSets.bVSyncInternal;
	rs.Save();
	if (m_pCAP) {
		m_pCAP->SetExtraSettings(&rs.ExtraSets);
	}
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.ExtraSets.bVSyncInternal ? ResStr(IDS_OSD_RS_INTERNAL_VSYNC_ON) : ResStr(IDS_OSD_RS_INTERNAL_VSYNC_OFF));
}

void CMainFrame::OnViewD3DFullScreen()
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	rs.bExclusiveFullscreen = !rs.bExclusiveFullscreen;
	s.SaveSettings();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bExclusiveFullscreen ? ResStr(IDS_OSD_RS_EXCLUSIVE_FSCREEN_ON) : ResStr(IDS_OSD_RS_EXCLUSIVE_FSCREEN_OFF));
}

void CMainFrame::OnViewResetDefault()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.SetDefault();
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_RS_RESET_DEFAULT));
}

void CMainFrame::OnUpdateViewResetDefault(CCmdUI* pCmdUI)
{
	if (m_clsidCAP == CLSID_EVRAllocatorPresenter || m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		pCmdUI->Enable(TRUE);

		return;
	}

	pCmdUI->Enable(FALSE);
	pCmdUI->SetCheck(0);
}

void CMainFrame::OnViewEnableFrameTimeCorrection()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.ExtraSets.bEVRFrameTimeCorrection = !rs.ExtraSets.bEVRFrameTimeCorrection;
	rs.Save();
	if (m_pCAP) {
		m_pCAP->SetExtraSettings(&rs.ExtraSets);
	}
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.ExtraSets.bEVRFrameTimeCorrection ? ResStr(IDS_OSD_RS_FT_CORRECTION_ON) : ResStr(IDS_OSD_RS_FT_CORRECTION_OFF));
}

void CMainFrame::OnViewVSyncOffsetIncrease()
{

	if (m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		CRenderersSettings& rs = GetRenderersSettings();
		rs.ExtraSets.dTargetSyncOffset -= 0.5; // Yeah, it should be a "-"
		rs.Save();
		if (m_pCAP) {
			m_pCAP->SetExtraSettings(&rs.ExtraSets);
		}
		CString strOSD;
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), rs.ExtraSets.dTargetSyncOffset);
		m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
	}
}

void CMainFrame::OnViewVSyncOffsetDecrease()
{
	if (m_clsidCAP == CLSID_SyncAllocatorPresenter) {
		CRenderersSettings& rs = GetRenderersSettings();
		rs.ExtraSets.dTargetSyncOffset += 0.5;
		rs.Save();
		if (m_pCAP) {
			m_pCAP->SetExtraSettings(&rs.ExtraSets);
		}
		CString strOSD;
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), rs.ExtraSets.dTargetSyncOffset);
		m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
	}
}

void CMainFrame::OnViewRemainingTime()
{
	CAppSettings& s = AfxGetAppSettings();
	s.bOSDRemainingTime = !s.bOSDRemainingTime;

	if (!s.bOSDRemainingTime) {
		m_OSD.ClearMessage();
	}

	OnTimer(TIMER_STREAMPOSPOLLER2);
}

CString CMainFrame::GetSystemLocalTime()
{
	CString strResult;

	SYSTEMTIME st;
	GetLocalTime(&st);
	strResult.Format(L"%02d:%02d",st.wHour,st.wMinute);

	return strResult;
}

void CMainFrame::OnUpdateViewOSDLocalTime(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(AfxGetAppSettings().ShowOSD.Enable && m_eMediaLoadState != MLS_CLOSED);
	pCmdUI->SetCheck(AfxGetAppSettings().bOSDLocalTime);
}

void CMainFrame::OnViewOSDLocalTime()
{
	CAppSettings& s = AfxGetAppSettings();
	s.bOSDLocalTime = !s.bOSDLocalTime;

	if (!s.bOSDLocalTime) {
		m_OSD.ClearMessage();
	}

	OnTimer(TIMER_STREAMPOSPOLLER2);
}

void CMainFrame::OnUpdateViewOSDFileName(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(AfxGetAppSettings().ShowOSD.Enable && m_eMediaLoadState != MLS_CLOSED);
	pCmdUI->SetCheck(AfxGetAppSettings().bOSDFileName);
}

void CMainFrame::OnViewOSDFileName()
{
	CAppSettings& s = AfxGetAppSettings();
	s.bOSDFileName = !s.bOSDFileName;

	if (!s.bOSDFileName) {
		m_OSD.ClearMessage();
	}

	OnTimer(TIMER_STREAMPOSPOLLER2);
}

void CMainFrame::OnUpdateShaderToggle1(CCmdUI* pCmdUI)
{
	if (AfxGetAppSettings().ShaderList.empty()) {
		pCmdUI->Enable(FALSE);
		m_bToggleShader = false;
		pCmdUI->SetCheck (0);
	} else {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck (m_bToggleShader);
	}
}

void CMainFrame::OnUpdateShaderToggle2(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();

	if (s.ShaderListScreenSpace.empty() && s.Shaders11PostScale.empty()) {
		pCmdUI->Enable(FALSE);
		m_bToggleShaderScreenSpace = false;
		pCmdUI->SetCheck(0);
	} else {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bToggleShaderScreenSpace);
	}
}

void CMainFrame::OnShaderToggle1()
{
	m_bToggleShader = !m_bToggleShader;
	if (m_bToggleShader) {
		SetShaders();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_SHADERS_1_ON));
	} else {
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_FRAME);
		}
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_SHADERS_1_OFF));
	}

	if (m_pCAP) {
		RepaintVideo();
	}
}

void CMainFrame::OnShaderToggle2()
{
	m_bToggleShaderScreenSpace = !m_bToggleShaderScreenSpace;
	if (m_bToggleShaderScreenSpace) {
		SetShaders();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_SHADERS_2_ON));
	} else {
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_SCREEN);
		}
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_SHADERS_2_OFF));
	}

	if (m_pCAP) {
		RepaintVideo();
	}
}

void CMainFrame::OnD3DFullscreenToggle()
{
	OnViewD3DFullScreen();
}

void CMainFrame::OnFileClosePlaylist()
{
	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	CAppSettings& s = AfxGetAppSettings();

	if (m_bFullScreen && s.fExitFullScreenAtTheEnd) {
		OnViewFullscreen();
	}

	if (s.bResetWindowAfterClosingFile) {
		RestoreDefaultWindowRect();
	}
}

void CMainFrame::OnUpdateFileClose(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED || m_eMediaLoadState == MLS_LOADING);
}

// view

void CMainFrame::OnViewCaptionmenu()
{
	CAppSettings& s = AfxGetAppSettings();
	s.iCaptionMenuMode++;
	s.iCaptionMenuMode %= MODE_COUNT; // three states: normal->borderless->frame only->

	if (m_bFullScreen) {
		return;
	}

	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwMenuFlags = GetMenuBarVisibility();

	const BOOL bZoomed = IsZoomed();

	CRect windowRect;
	GetWindowRect(&windowRect);
	const CRect oldwindowRect(windowRect);
	if (!bZoomed) {
		CRect decorationsRect;
		VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), dwMenuFlags == AFX_MBV_KEEPVISIBLE, GetWindowExStyle(m_hWnd)));
		windowRect.bottom -= decorationsRect.bottom;
		windowRect.right  -= decorationsRect.right;
		windowRect.top    -= decorationsRect.top;
		windowRect.left   -= decorationsRect.left;
	}

	switch (s.iCaptionMenuMode) {
		case MODE_SHOWCAPTIONMENU:	// borderless -> normal
			dwMenuFlags = AFX_MBV_KEEPVISIBLE;
			dwAdd |= (WS_CAPTION | WS_THICKFRAME);
			dwRemove &= ~(WS_CAPTION | WS_THICKFRAME);
			break;
		case MODE_HIDEMENU:			// normal -> hidemenu
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			break;
		case MODE_FRAMEONLY:		// hidemenu -> frameonly
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			dwAdd &= ~WS_CAPTION;
			dwRemove |= WS_CAPTION;
			break;
		case MODE_BORDERLESS:		// frameonly -> borderless
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			dwAdd &= ~WS_THICKFRAME;
			dwRemove |= WS_THICKFRAME;
		break;
	}

	UINT uFlags = SWP_NOZORDER;
	if (dwRemove != dwAdd) {
		uFlags |= SWP_FRAMECHANGED;
		VERIFY(SetWindowLong(m_hWnd, GWL_STYLE, (GetWindowLong(m_hWnd, GWL_STYLE) | dwAdd) & ~dwRemove));
	}

	SetMenuBarVisibility(dwMenuFlags);

	if (bZoomed) {
		CMonitors::GetNearestMonitor(this).GetWorkAreaRect(windowRect);
	} else {
		VERIFY(AdjustWindowRectEx(windowRect, GetWindowStyle(m_hWnd), dwMenuFlags == AFX_MBV_KEEPVISIBLE, GetWindowExStyle(m_hWnd)));
		if (oldwindowRect.top != windowRect.top) {
			// restore original top position
			windowRect.OffsetRect(0, oldwindowRect.top - windowRect.top);
		}
	}
	VERIFY(SetWindowPos(nullptr, windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(), uFlags));
	RepaintVideo();

	FlyBarSetPos();
	OSDBarSetPos();
}

void CMainFrame::OnUpdateViewCaptionmenu(CCmdUI* pCmdUI)
{
	static int NEXT_MODE[] = {IDS_VIEW_HIDEMENU, IDS_VIEW_FRAMEONLY, IDS_VIEW_BORDERLESS, IDS_VIEW_CAPTIONMENU};
	int idx = (AfxGetAppSettings().iCaptionMenuMode %= MODE_COUNT);
	pCmdUI->SetText(ResStr(NEXT_MODE[idx]));
}

void CMainFrame::OnViewControlBar(UINT nID)
{
	const auto& s = AfxGetAppSettings();

	nID -= ID_VIEW_SEEKER;
	ShowControls(s.nCS ^ (1<<nID));

	if (m_bFullScreen && s.nShowBarsWhenFullScreenTimeOut == 0) {
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, 2000, nullptr);
	}
}

void CMainFrame::OnUpdateViewControlBar(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_VIEW_SEEKER;
	pCmdUI->SetCheck(!!(AfxGetAppSettings().nCS & (1<<nID)));
}

void CMainFrame::OnViewSubresync()
{
	ShowControlBarInternal(&m_wndSubresyncBar, !m_wndSubresyncBar.IsWindowVisible());
}

void CMainFrame::OnUpdateViewSubresync(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndSubresyncBar.IsWindowVisible());
	pCmdUI->Enable(m_pCAP && m_iSubtitleSel >= 0);
}

void CMainFrame::OnViewPlaylist()
{
	ShowControlBarInternal(&m_wndPlaylistBar, !m_wndPlaylistBar.IsWindowVisible());
	if (m_wndPlaylistBar.IsWindowVisible()) {
		m_wndPlaylistBar.SetFocus();
	}
}

void CMainFrame::OnUpdateViewPlaylist(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPlaylistBar.IsWindowVisible());
}

// Navigation menu
void CMainFrame::OnViewNavigation()
{
	CAppSettings& s = AfxGetAppSettings();
	s.fHideNavigation = !s.fHideNavigation;
	m_wndNavigationBar.m_navdlg.UpdateElementList();
	if (GetPlaybackMode() == PM_CAPTURE) {
		ShowControlBarInternal(&m_wndNavigationBar, !s.fHideNavigation);
	}
}

void CMainFrame::OnUpdateViewNavigation(CCmdUI* pCmdUI)
{
	const auto& s = AfxGetAppSettings();
	pCmdUI->SetCheck(!s.fHideNavigation);
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1);
}

void CMainFrame::OnViewCapture()
{
	ShowControlBarInternal(&m_wndCaptureBar, !m_wndCaptureBar.IsWindowVisible());
}

void CMainFrame::OnUpdateViewCapture(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptureBar.IsWindowVisible());
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_CAPTURE);
}

void CMainFrame::OnViewShaderEditor()
{
	if (m_wndShaderEditorBar.IsWindowVisible()) {
		SetShaders(); // reset shaders
	} else {
		m_wndShaderEditorBar.m_dlg.UpdateShaderList();
	}
	ShowControlBarInternal(&m_wndShaderEditorBar, !m_wndShaderEditorBar.IsWindowVisible());
}

void CMainFrame::OnUpdateViewShaderEditor(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndShaderEditorBar.IsWindowVisible());
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnViewMinimal()
{
	while (AfxGetAppSettings().iCaptionMenuMode != MODE_BORDERLESS) {
		SendMessageW(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_NONE);
}

void CMainFrame::OnViewCompact()
{
	while (AfxGetAppSettings().iCaptionMenuMode != MODE_FRAMEONLY) {
		SendMessageW(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_SEEKBAR | CS_TOOLBAR);
}

void CMainFrame::OnViewNormal()
{
	while (AfxGetAppSettings().iCaptionMenuMode != MODE_SHOWCAPTIONMENU) {
		SendMessageW(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_SEEKBAR | CS_TOOLBAR | CS_STATUSBAR);
}

bool CMainFrame::CanSwitchD3DFS()
{
	if (IsD3DFullScreenMode()) {
		return true;
	}

	const CAppSettings& s = AfxGetAppSettings();
	if (m_eMediaLoadState == MLS_LOADED) {
		bool optOn = s.ExclusiveFSAllowed();
		return optOn && m_pD3DFS && !m_bFullScreen;
	} else {
		return s.ExclusiveFSAllowed();
	}
}

void CMainFrame::OnViewFullscreen()
{
	if (CanSwitchD3DFS()) {
		ToggleD3DFullscreen(true);
	} else {
		ToggleFullscreen(true, true);
	}
}

void CMainFrame::OnViewFullscreenSecondary()
{
	if (CanSwitchD3DFS()) {
		ToggleD3DFullscreen(false);
	} else {
		ToggleFullscreen(true, false);
	}
}

void CMainFrame::OnMoveWindowToPrimaryScreen()
{
	if (m_bFullScreen) {
		ToggleFullscreen(false,false);
	}

	if (!IsD3DFullScreenMode()) {
		CRect windowRect;
		GetWindowRect(&windowRect);

		CMonitor monitor = CMonitors::GetPrimaryMonitor();
		monitor.CenterRectToMonitor(windowRect, TRUE, GetInvisibleBorderSize());
		SetWindowPos(nullptr, windowRect.left, windowRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void CMainFrame::OnViewZoom(UINT nID)
{
	double scale = (nID == ID_VIEW_ZOOM_50) ? 0.5 : (nID == ID_VIEW_ZOOM_200) ? 2.0 : 1.0;

	ZoomVideoWindow(true, scale);

	CString strODSMessage;
	strODSMessage.Format(IDS_OSD_ZOOM, scale * 100);
	m_OSD.DisplayMessage(OSD_TOPLEFT, strODSMessage, 3000);
}

void CMainFrame::OnUpdateViewZoom(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
}

void CMainFrame::OnViewZoomAutoFit()
{
	ZoomVideoWindow(true, GetZoomAutoFitScale());
	m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_ZOOM_AUTO), 3000);
}

void CMainFrame::OnViewDefaultVideoFrame(UINT nID)
{
	m_iVideoSize = nID - ID_VIEW_VF_HALF;
	m_ZoomX = m_ZoomY = 1;
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI)
{
	bool bCheck = m_iVideoSize == (pCmdUI->m_nID - ID_VIEW_VF_HALF);
	SetMenuRadioCheck(pCmdUI, bCheck);
}

void CMainFrame::OnViewSwitchVideoFrame()
{
	int vs = m_iVideoSize;
	if (vs <= DVS_DOUBLE || vs == DVS_FROMOUTSIDE) {
		vs = DVS_STRETCH;
	} else if (vs == DVS_FROMINSIDE) {
		vs = DVS_ZOOM1;
	} else if (vs == DVS_ZOOM2) {
		vs = DVS_FROMOUTSIDE;
	} else {
		vs++;
	}
	switch (vs) { // TODO: Read messages from resource file
		case DVS_STRETCH:
			m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_STRETCH_TO_WINDOW));
			break;
		case DVS_FROMINSIDE:
			m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_TOUCH_WINDOW_FROM_INSIDE));
			break;
		case DVS_ZOOM1:
			m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_ZOOM1));
			break;
		case DVS_ZOOM2:
			m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_ZOOM2));
			break;
		case DVS_FROMOUTSIDE:
			m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_TOUCH_WINDOW_FROM_OUTSIDE));
			break;
	}
	m_iVideoSize = vs;
	m_ZoomX = m_ZoomY = 1;
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnViewKeepaspectratio()
{
	CAppSettings& s = AfxGetAppSettings();
	s.fKeepAspectRatio = !s.fKeepAspectRatio;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AfxGetAppSettings().fKeepAspectRatio);
}

void CMainFrame::OnViewCompMonDeskARDiff()
{
	CAppSettings& s = AfxGetAppSettings();
	s.fCompMonDeskARDiff = !s.fCompMonDeskARDiff;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AfxGetAppSettings().fCompMonDeskARDiff);
}

void CMainFrame::OnViewPanNScan(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	int x = 0, y = 0;
	int dx = 0, dy = 0;

	switch (nID) {
		case ID_VIEW_RESET:
			m_ZoomX = m_ZoomY = 1.0;
			m_PosX = m_PosY = 0.5;
			if (m_pCAP) {
				m_pCAP->SetRotation(m_iDefRotation);
				m_pCAP->SetFlip(false);
			}
			break;
		case ID_VIEW_INCSIZE:
			x = y = 1;
			break;
		case ID_VIEW_DECSIZE:
			x = y = -1;
			break;
		case ID_VIEW_INCWIDTH:
			x = 1;
			break;
		case ID_VIEW_DECWIDTH:
			x = -1;
			break;
		case ID_VIEW_INCHEIGHT:
			y = 1;
			break;
		case ID_VIEW_DECHEIGHT:
			y = -1;
			break;
		case ID_PANSCAN_CENTER:
			m_PosX = m_PosY = 0.5;
			break;
		case ID_PANSCAN_MOVELEFT:
			dx = -1;
			break;
		case ID_PANSCAN_MOVERIGHT:
			dx = 1;
			break;
		case ID_PANSCAN_MOVEUP:
			dy = -1;
			break;
		case ID_PANSCAN_MOVEDOWN:
			dy = 1;
			break;
		case ID_PANSCAN_MOVEUPLEFT:
			dx = dy = -1;
			break;
		case ID_PANSCAN_MOVEUPRIGHT:
			dx = 1;
			dy = -1;
			break;
		case ID_PANSCAN_MOVEDOWNLEFT:
			dx = -1;
			dy = 1;
			break;
		case ID_PANSCAN_MOVEDOWNRIGHT:
			dx = dy = 1;
			break;
		default:
			break;
	}

	if (x) {
		if (x > 0) {
			m_ZoomX = std::min(m_ZoomX * 1.02, 5.0);
		} else { // x < 0
			m_ZoomX = std::max(m_ZoomX / 1.02, 0.2);
		}
		if (abs(m_ZoomX - 1) < 0.01) {
			m_ZoomX = 1.0;
		}
	}

	if (y) {
		if (y > 0) {
			m_ZoomY = std::min(m_ZoomY * 1.02, 5.0);
		} else { // y < 0
			m_ZoomY = std::max(m_ZoomY / 1.02, 0.2);
		}
		if (abs(m_ZoomY - 1) < 0.01) {
			m_ZoomY = 1.0;
		}
	}

	const double shift = 0.005;

	if (dx) {
		m_PosX = std::clamp(m_PosX + shift * dx, 0.0, 1.0);

		if (abs(m_PosX - 0.5) * 2 < shift) {
			m_PosX = 0.5;
		}
	}

	if (dy) {
		m_PosY = std::clamp(m_PosY + shift * dy, 0.0, 1.0);

		if (abs(m_PosY - 0.5) * 2 < shift) {
			m_PosY = 0.5;
		}
	}

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
}

void CMainFrame::OnViewPanNScanPresets(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	nID -= ID_PANNSCAN_PRESETS_START;

	if ((INT_PTR)nID == s.m_pnspresets.GetCount()) {
		CPnSPresetsDlg dlg;
		dlg.m_pnspresets.Copy(s.m_pnspresets);
		if (dlg.DoModal() == IDOK) {
			s.m_pnspresets.Copy(dlg.m_pnspresets);
			s.SaveSettings();
		}
		return;
	}

	m_PosX = 0.5;
	m_PosY = 0.5;
	m_ZoomX = 1.0;
	m_ZoomY = 1.0;

	CString str = s.m_pnspresets[nID];

	int i = 0, j = 0;
	for (CString token = str.Tokenize(L",", i); !token.IsEmpty(); token = str.Tokenize(L",", i), j++) {
		float f = 0;
		if (swscanf_s(token, L"%f", &f) != 1) {
			continue;
		}

		switch (j) {
			case 0:
				break;
			case 1:
				m_PosX = f;
				break;
			case 2:
				m_PosY = f;
				break;
			case 3:
				m_ZoomX = f;
				break;
			case 4:
				m_ZoomY = f;
				break;
			default:
				break;
		}
	}

	if (j != 5) {
		return;
	}

	m_PosX  = std::clamp(m_PosX, 0.0, 1.0);
	m_PosY  = std::clamp(m_PosY, 0.0, 1.0);
	m_ZoomX = std::clamp(m_ZoomX, 0.2, 5.0);
	m_ZoomY = std::clamp(m_ZoomY, 0.2, 5.0);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI)
{
	int nID = pCmdUI->m_nID - ID_PANNSCAN_PRESETS_START;
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly && nID >= 0 && nID <= AfxGetAppSettings().m_pnspresets.GetCount());
}

void CMainFrame::OnViewRotate(UINT nID)
{
	if (m_pCAP) {
		HRESULT hr = S_OK;
		int rotation = m_pCAP->GetRotation();

		switch (nID) {
		case ID_PANSCAN_ROTATE_CCW:
			rotation += 270;
			break;
		case ID_PANSCAN_ROTATE_CW:
			rotation += 90;
			break;
		default:
			return;
		}
		rotation %= 360;
		ASSERT(rotation >= 0);

		hr = m_pCAP->SetRotation(rotation);
		if (S_OK == hr) {
			if (!m_pMVRC) {
				MoveVideoWindow(); // need for EVRcp and Sync renderer
			}

			CString info;
			info.Format(L"Rotation: %d�", rotation);
			SendStatusMessage(info, 3000);
		}
	}
}

void CMainFrame::OnViewFlip()
{
	if (m_pCAP) {
		HRESULT hr = S_OK;
		bool flip = !m_pCAP->GetFlip();
		hr = m_pCAP->SetFlip(flip);
		if (S_OK == hr) {
			m_pCAP->Paint(false);
		}
	}
}

const static SIZE s_ar[] = { {0, 0}, {4, 3}, {5, 4}, {16, 9}, {235, 100}, {185, 100} };

void CMainFrame::OnViewAspectRatio(UINT nID)
{
	CSize& ar = AfxGetAppSettings().sizeAspectRatio;

	ar = s_ar[nID - ID_ASPECTRATIO_START];

	CString info;
	if (ar.cx && ar.cy) {
		info.Format(ResStr(IDS_MAINFRM_68), ar.cx, ar.cy);
	} else {
		info.LoadString(IDS_MAINFRM_69);
	}
	SendStatusMessage(info, 3000);

	m_OSD.DisplayMessage(OSD_TOPLEFT, info, 3000);

	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewAspectRatio(CCmdUI* pCmdUI)
{
	bool bCheck = AfxGetAppSettings().sizeAspectRatio == s_ar[pCmdUI->m_nID - ID_ASPECTRATIO_START];
	SetMenuRadioCheck(pCmdUI, bCheck);

	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
}

void CMainFrame::OnViewAspectRatioNext()
{
	CSize& ar = AfxGetAppSettings().sizeAspectRatio;

	UINT nID = ID_ASPECTRATIO_START;

	for (unsigned i = 0; i < std::size(s_ar); i++) {
		if (ar == s_ar[i]) {
			nID += (i + 1) % std::size(s_ar);
			break;
		}
	}

	OnViewAspectRatio(nID);
}

void CMainFrame::OnUpdateViewStereo3DMode(CCmdUI* pCmdUI)
{
	bool bCheck = AfxGetAppSettings().iStereo3DMode == (pCmdUI->m_nID - ID_STEREO3D_AUTO);
	SetMenuRadioCheck(pCmdUI, bCheck);
}

void CMainFrame::OnViewStereo3DMode(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	s.iStereo3DMode = nID - ID_STEREO3D_AUTO;
	ASSERT(s.iStereo3DMode >= STEREO3D_AUTO && s.iStereo3DMode <= STEREO3D_OVERUNDER);

	BOOL bMvcActive = FALSE;
	if (CComQIPtr<IExFilterConfig> pEFC = FindFilter(__uuidof(CMPCVideoDecFilter), m_pGB)) {
		int iMvcOutputMode = MVC_OUTPUT_Auto;
		switch (s.iStereo3DMode) {
		case STEREO3D_MONO:              iMvcOutputMode = MVC_OUTPUT_Mono;          break;
		case STEREO3D_ROWINTERLEAVED:    iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
		case STEREO3D_ROWINTERLEAVED_2X: iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
		case STEREO3D_HALFOVERUNDER:     iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
		case STEREO3D_OVERUNDER:         iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
		}
		const int mvc_mode_value = (iMvcOutputMode << 16) | (s.bStereo3DSwapLR ? 1 : 0);

		pEFC->Flt_SetInt("mvc_mode", mvc_mode_value);
		pEFC->Flt_GetInt("decode_mode_mvc", &bMvcActive);
	}

	if (s.iStereo3DMode == STEREO3D_ROWINTERLEAVED || s.iStereo3DMode == STEREO3D_ROWINTERLEAVED_2X
			|| (s.iStereo3DMode == STEREO3D_AUTO && bMvcActive && !m_pBFmadVR)) {
		rs.Stereo3DSets.iTransform = STEREO3D_HalfOverUnder_to_Interlace;
	} else {
		rs.Stereo3DSets.iTransform = STEREO3D_AsIs;
	}
	rs.Stereo3DSets.bSwapLR = s.bStereo3DSwapLR;
	if (m_pCAP) {
		m_pCAP->SetStereo3DSettings(&rs.Stereo3DSets);
	}

	RepaintVideo();
}

void CMainFrame::OnUpdateViewSwapLeftRight(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AfxGetAppSettings().bStereo3DSwapLR);
}

void CMainFrame::OnViewSwapLeftRight()
{
	CAppSettings& s = AfxGetAppSettings();

	s.bStereo3DSwapLR = !s.bStereo3DSwapLR;
	s.m_VRSettings.Stereo3DSets.bSwapLR = s.bStereo3DSwapLR;

	IFilterGraph* pFG = m_pGB;
	if (pFG) {
		if (CComQIPtr<IExFilterConfig> pEFC = FindFilter(__uuidof(CMPCVideoDecFilter), pFG)) {
			int iMvcOutputMode = MVC_OUTPUT_Auto;
			switch (s.iStereo3DMode) {
			case STEREO3D_MONO:              iMvcOutputMode = MVC_OUTPUT_Mono;          break;
			case STEREO3D_ROWINTERLEAVED:    iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
			case STEREO3D_ROWINTERLEAVED_2X: iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
			case STEREO3D_HALFOVERUNDER:     iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
			case STEREO3D_OVERUNDER:         iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
			}
			const int mvc_mode_value = (iMvcOutputMode << 16) | (s.bStereo3DSwapLR ? 1 : 0);

			pEFC->Flt_SetInt("mvc_mode", mvc_mode_value);
		}
	}

	if (m_pCAP) {
		m_pCAP->SetStereo3DSettings(&s.m_VRSettings.Stereo3DSets);
	}
}

void CMainFrame::OnViewOntop(UINT nID)
{
	nID -= ID_ONTOP_NEVER;
	if (AfxGetAppSettings().iOnTop == (int)nID) {
		nID = !nID;
	}
	SetAlwaysOnTop(nID);
}

void CMainFrame::OnUpdateViewOntop(CCmdUI* pCmdUI)
{
	bool bCheck = AfxGetAppSettings().iOnTop == (pCmdUI->m_nID - ID_ONTOP_NEVER);
	SetMenuRadioCheck(pCmdUI, bCheck);
}

void CMainFrame::OnViewOptions()
{
	if (!CanShowDialog()) {
		return;
	}

	ShowOptions();
	StartAutoHideCursor();
}

// play

void CMainFrame::OnPlayPlay()
{
	if (m_eMediaLoadState == MLS_CLOSED) {
		m_bfirstPlay = false;
		OpenCurPlaylistItem();
		return;
	}

	const auto& s = AfxGetAppSettings();
	bool bShowOSD = s.ShowOSD.Enable && s.ShowOSD.FileName;
	CString strOSD;

	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE) {
			if (m_bEndOfStream) {
				m_bEndOfStream = false;
				SendMessageW(WM_COMMAND, ID_PLAY_STOP);
				SendMessageW(WM_COMMAND, ID_PLAY_PLAYPAUSE);
			}
			if (m_bIsLiveOnline || FAILED(m_pMS->SetRate(m_PlaybackRate))) {
				m_PlaybackRate = 1.0;
			};
			m_pMC->Run();
		} else if (GetPlaybackMode() == PM_DVD) {
			if (m_PlaybackRate >= 0.0) {
				m_pDVDC->PlayForwards(m_PlaybackRate, DVD_CMD_FLAG_Block, nullptr);
			} else {
				m_pDVDC->PlayBackwards(abs(m_PlaybackRate), DVD_CMD_FLAG_Block, nullptr);
			}
			m_pDVDC->Pause(FALSE);
			m_pMC->Run();
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			m_pMC->Stop(); // audio preview won't be in sync if we run it from paused state
			m_pMC->Run();
			if (s.iDefaultCaptureDevice == 1) {
				CComQIPtr<IBDATuner> pTun = m_pGB.p;
				if (pTun) {
					pTun->SetChannel(s.nDVBLastChannel);
					DisplayCurrentChannelOSD();
				}
			}
			else if (m_pAMTuner) {
				long lChannel = 0, lVivSub = 0, lAudSub = 0;
				m_pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);
				long lFreq = 0;
				m_pAMTuner->get_VideoFrequency(&lFreq);

				strOSD.Format(ResStr(IDS_CAPTURE_CHANNEL_FREQ), lChannel, lFreq / 1000000.0);
				bShowOSD = s.ShowOSD.Enable;
			}
		}

		SetTimersPlay();
		if (m_bFrameSteppingActive) { // FIXME
			m_bFrameSteppingActive = false;
			m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
		} else {
			m_pBA->put_Volume(m_wndToolBar.Volume);
		}

		SetAlwaysOnTop(s.iOnTop);
	}

	MoveVideoWindow(false, true);
	m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_PLAYING), 3000);
	SetPlayState(PS_PLAY);

	OnTimer(TIMER_STREAMPOSPOLLER);

	SetupEVRColorControl(); // can be configured when streaming begins

	if (m_bfirstPlay) {
		m_bfirstPlay = false;

		if (!m_youtubeFields.title.IsEmpty()) {
			strOSD = m_youtubeFields.title;
		} else if (::PathIsURLW(GetCurFileName())) {
			CPlaylistItem pli;
			if (m_wndPlaylistBar.GetCur(pli) && !pli.m_label.IsEmpty()) {
				strOSD = pli.m_label;
			}

			if (strOSD.IsEmpty() && m_pAMMC[0]) {
				for (const auto& pAMMC : m_pAMMC) {
					if (pAMMC) {
						CComBSTR bstr;
						if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
							strOSD = bstr.m_str;
							break;
						}
					}
				}
			}
		}

		if (strOSD.IsEmpty()) {
			if (GetPlaybackMode() == PM_FILE) {
				strOSD = GetCurFileName();
				if (m_LastOpenBDPath.GetLength() > 0) {
					strOSD = ResStr(ID_PLAY_PLAY);
					int i = strOSD.Find('\n');
					if (i > 0) {
						strOSD.Delete(i, strOSD.GetLength() - i);
					}
					strOSD.Append(L" Blu-ray");
					if (m_BDLabel.GetLength() > 0) {
						strOSD.AppendFormat(L" \"%s\"", m_BDLabel);
					} else {
						MakeBDLabel(GetCurFileName(), strOSD);
					}

					strOSD.AppendFormat(L" (%s)", GetFileName(m_strPlaybackRenderedPath));
				} else if (strOSD.GetLength() > 0) {
					strOSD.TrimRight('/');
					strOSD.Replace('\\', '/');
					strOSD = strOSD.Mid(strOSD.ReverseFind('/') + 1);
				}
			} else if (GetPlaybackMode() == PM_DVD) {
				strOSD = ResStr(ID_PLAY_PLAY);
				int i = strOSD.Find('\n');
				if (i > 0) {
					strOSD.Delete(i, strOSD.GetLength() - i);
				}
				strOSD += L" DVD";
				MakeDVDLabel(L"", strOSD);
			}
		}
	}

	if (strOSD.IsEmpty()) {
		strOSD = ResStr(ID_PLAY_PLAY);
		int i = strOSD.Find('\n');
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
		bShowOSD = s.ShowOSD.Enable;
	}

	if (bShowOSD) {
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
	}
}

void CMainFrame::OnPlayPause()
{
	OAFilterState fs = GetMediaState();

	if (m_eMediaLoadState == MLS_LOADED && fs == State_Stopped) {
		MoveVideoWindow(false, true);
	}

	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE
				|| GetPlaybackMode() == PM_DVD
				|| GetPlaybackMode() == PM_CAPTURE) {
			m_pMC->Pause();
		} else {
			ASSERT(FALSE);
		}

		KillTimer(TIMER_STATS);
		SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
	}

	if (!m_bEndOfStream) {
		CString strOSD = ResStr(ID_PLAY_PAUSE);
		int i = strOSD.Find(L"\n");
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_PAUSED), 3000);
	}

	SetPlayState(PS_PAUSE);
}

void CMainFrame::OnPlayPlayPause()
{
	OAFilterState fs = GetMediaState();
	if (fs == State_Running) {
		SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
	} else {
		SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
	}
}

void CMainFrame::OnPlayStop()
{
	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE) {
			LONGLONG pos = 0;
			m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			m_pMC->Stop();

			// BUG: after pause or stop the netshow url source filter won't continue
			// on the next play command, unless we cheat it by setting the file name again.
			//
			// Note: WMPx may be using some undocumented interface to restart streaming.

			BeginEnumFilters(m_pGB, pEF, pBF) {
				CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF.p;
				CComQIPtr<IFileSourceFilter> pFSF = pBF.p;
				if (pAMNS && pFSF) {
					WCHAR* pFN = nullptr;
					AM_MEDIA_TYPE mt;
					if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
						pFSF->Load(pFN, nullptr);
						CoTaskMemFree(pFN);
					}
					break;
				}
			}
			EndEnumFilters;
		} else if (GetPlaybackMode() == PM_DVD) {
			m_pDVDC->SetOption(DVD_ResetOnStop, TRUE);
			m_pMC->Stop();
			m_pDVDC->SetOption(DVD_ResetOnStop, FALSE);

			if (m_pDVDC_preview) {
				m_pDVDC_preview->SetOption(DVD_ResetOnStop, TRUE);
				m_pMC_preview->Stop();
				m_pDVDC_preview->SetOption(DVD_ResetOnStop, FALSE);
			}

		} else if (GetPlaybackMode() == PM_CAPTURE) {
			m_pMC->Stop();
		}

		if (m_bFrameSteppingActive) { // FIXME
			m_bFrameSteppingActive = false;
			m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
		}

		if (m_pPlaybackNotify) {
			m_pPlaybackNotify->Stop();
		}

		m_OSD.SetPos(0);

		if (m_pCAP) {
			m_pCAP->SetTime(0);
		}
	}

	m_nLoops = 0;

	if (m_hWnd) {
		KillTimersStop();
		MoveVideoWindow();

		if (m_eMediaLoadState == MLS_LOADED) {
			REFERENCE_TIME stop = m_wndSeekBar.GetRange();
			if (GetPlaybackMode() != PM_CAPTURE) {
				const bool bShowMilliSecs = m_bShowMilliSecs || m_wndSubresyncBar.IsWindowVisible();
				m_wndStatusBar.SetStatusTimer(m_wndSeekBar.GetPosReal(), stop, bShowMilliSecs, GetTimeFormat());
			}

			SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
		}
	}

	if (!m_bEndOfStream && m_eMediaLoadState != MLS_CLOSING) {
		CString strOSD = ResStr(ID_PLAY_STOP);
		int i = strOSD.Find(L"\n");
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_STOPPED), 3000);
	}

	DisableABRepeat();
	SetPlayState(PS_STOP);
}

void CMainFrame::OnUpdatePlayPauseStop(CCmdUI* pCmdUI)
{
	OAFilterState fs = m_bFrameSteppingActive ? State_Paused : GetMediaState();

	pCmdUI->SetCheck(fs == State_Running && pCmdUI->m_nID == ID_PLAY_PLAY
					 || fs == State_Paused && pCmdUI->m_nID == ID_PLAY_PAUSE
					 || fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_STOP
					 || (fs == State_Paused || fs == State_Running) && pCmdUI->m_nID == ID_PLAY_PLAYPAUSE);

	bool fEnable = false;

	if (fs >= 0) {
		if (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_CAPTURE) {
			fEnable = true;

			if (m_bCapturing) {
				fEnable = false;
			} else if (m_bLiveWM && pCmdUI->m_nID == ID_PLAY_PAUSE) {
				fEnable = false;
			} else if (GetPlaybackMode() == PM_CAPTURE && pCmdUI->m_nID == ID_PLAY_PAUSE && AfxGetAppSettings().iDefaultCaptureDevice == 1) {
				fEnable = false;
			}
		} else if (GetPlaybackMode() == PM_DVD) {
			fEnable = m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu
					  && m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu;

			if (fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_PAUSE) {
				fEnable = false;
			}
		}
	} else if ((pCmdUI->m_nID == ID_PLAY_PLAY || pCmdUI->m_nID == ID_PLAY_PLAYPAUSE) && m_wndPlaylistBar.GetCount() > 0) {
		fEnable = true;
	}

	pCmdUI->Enable(fEnable);

	m_CMediaControls.UpdateButtons();
}

void CMainFrame::OnPlayFrameStep(UINT nID)
{
	m_OSD.EnableShowMessage(false);

	if (m_pFS && nID == ID_PLAY_FRAMESTEP) {
		if (GetMediaState() != State_Paused) {
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}

		// To support framestep back, store the initial position when
		// stepping forward
		if (m_nStepForwardCount == 0) {
			if (GetPlaybackMode() == PM_DVD) {
				OnTimer(TIMER_STREAMPOSPOLLER);
				m_rtStepForwardStart = m_wndSeekBar.GetPos();
			} else {
				m_pMS->GetCurrentPosition(&m_rtStepForwardStart);
			}
		}

		m_bFrameSteppingActive = true;

		m_nVolumeBeforeFrameStepping = m_wndToolBar.Volume;
		m_pBA->put_Volume(-10000);

		m_pFS->Step(1, nullptr);
	} else if (S_OK == m_pMS->IsFormatSupported(&TIME_FORMAT_FRAME)) {
		if (GetMediaState() != State_Paused) {
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}

		if (SUCCEEDED(m_pMS->SetTimeFormat(&TIME_FORMAT_FRAME))) {
			REFERENCE_TIME rtCurPos;

			if (SUCCEEDED(m_pMS->GetCurrentPosition(&rtCurPos))) {
				rtCurPos += (nID == ID_PLAY_FRAMESTEP) ? 1 : -1;

				m_pMS->SetPositions(&rtCurPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			}
			m_pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
		}
	} else {
		if (GetMediaState() != State_Paused) {
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}

		const REFERENCE_TIME rtAvgTimePerFrame = std::llround(GetAvgTimePerFrame() * 10000000i64);
		REFERENCE_TIME rtCurPos = 0;

		// Exit of framestep forward : calculate the initial position
		if (m_nStepForwardCount) {
			m_pFS->CancelStep();
			rtCurPos = m_rtStepForwardStart + m_nStepForwardCount * rtAvgTimePerFrame;
			m_nStepForwardCount = 0;
		} else if (GetPlaybackMode() == PM_DVD) {
			// IMediaSeeking doesn't work well with DVD Navigator
			OnTimer(TIMER_STREAMPOSPOLLER);
			rtCurPos = m_wndSeekBar.GetPos();
		} else {
			m_pMS->GetCurrentPosition(&rtCurPos);
		}

		rtCurPos += (nID == ID_PLAY_FRAMESTEP) ? rtAvgTimePerFrame : -rtAvgTimePerFrame;
		SeekTo(rtCurPos);
	}
	m_OSD.EnableShowMessage();
}

void CMainFrame::OnUpdatePlayFrameStep(CCmdUI* pCmdUI)
{
	BOOL fEnable = FALSE;

	if (m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly &&
			(GetPlaybackMode() != PM_DVD || m_iDVDDomain == DVD_DOMAIN_Title) &&
			GetPlaybackMode() != PM_CAPTURE &&
			!m_bLiveWM) {
		if (S_OK == m_pMS->IsFormatSupported(&TIME_FORMAT_FRAME)) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEP) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEP_BACK && m_pFS && m_pFS->CanStep(0, nullptr) == S_OK) {
			fEnable = TRUE;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlaySeek(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();

	REFERENCE_TIME rtSeekTo =
		nID == ID_PLAY_SEEKBACKWARDSMALL ? -10000i64*s.nJumpDistS :
		nID == ID_PLAY_SEEKFORWARDSMALL ? +10000i64*s.nJumpDistS :
		nID == ID_PLAY_SEEKBACKWARDMED ? -10000i64*s.nJumpDistM :
		nID == ID_PLAY_SEEKFORWARDMED ? +10000i64*s.nJumpDistM :
		nID == ID_PLAY_SEEKBACKWARDLARGE ? -10000i64*s.nJumpDistL :
		nID == ID_PLAY_SEEKFORWARDLARGE ? +10000i64*s.nJumpDistL :
		0;

	bool bSeekingForward = (nID == ID_PLAY_SEEKFORWARDSMALL ||
							nID == ID_PLAY_SEEKFORWARDMED ||
							nID == ID_PLAY_SEEKFORWARDLARGE);

	if (rtSeekTo == 0) {
		ASSERT(FALSE);
		return;
	}

	if (m_bShockwaveGraph) {
		// HACK: the custom graph should support frame based seeking instead
		rtSeekTo /= 10000i64 * 100;
	}

	const REFERENCE_TIME rtPos = m_wndSeekBar.GetPos();
	rtSeekTo += rtPos;

	if (s.fFastSeek) {
		MatroskaLoadKeyFrames();
	}

	if (s.fFastSeek && !m_kfs.empty()) {
		// seek to the closest keyframe, but never in the opposite direction
		rtSeekTo = GetClosestKeyFrame(rtSeekTo);
		if ((bSeekingForward && rtSeekTo <= rtPos) ||
				(!bSeekingForward &&
				 rtSeekTo >= rtPos - (GetMediaState() == State_Running ? 10000000 : 0))) {
			OnPlaySeekKey(bSeekingForward ? ID_PLAY_SEEKKEYFORWARD : ID_PLAY_SEEKKEYBACKWARD);
			return;
		}
	}

	SeekTo(rtSeekTo);
}

void CMainFrame::OnPlaySeekBegin()
{
	const REFERENCE_TIME rtPos = m_wndSeekBar.GetPos();
	if (rtPos != 0) {
		SeekTo(0);
	}
}

void CMainFrame::SetTimersPlay()
{
	SetTimer(TIMER_STREAMPOSPOLLER, 40, nullptr);
	SetTimer(TIMER_STREAMPOSPOLLER2, 500, nullptr);
	SetTimer(TIMER_STATS, 1000, nullptr);
}

void CMainFrame::KillTimersStop()
{
	KillTimer(TIMER_STREAMPOSPOLLER2);
	KillTimer(TIMER_STREAMPOSPOLLER);
	KillTimer(TIMER_STATS);
	KillTimer(TIMER_DM_AUTOCHANGING);
}

void CMainFrame::OnPlaySeekKey(UINT nID)
{
	MatroskaLoadKeyFrames();

	if (!m_kfs.empty()) {
		bool bSeekingForward = (nID == ID_PLAY_SEEKKEYFORWARD);
		const REFERENCE_TIME rtPos = m_wndSeekBar.GetPos();
		REFERENCE_TIME rtSeekTo = rtPos - (bSeekingForward ? 0 : (GetMediaState() == State_Running) ? 10000000 : 10000);

		REFERENCE_TIME rtDuration = m_wndSeekBar.GetRange();
		if (rtDuration && rtSeekTo >= rtDuration) {
			SeekTo(rtDuration);
			return;
		}

		std::pair<REFERENCE_TIME, REFERENCE_TIME> keyframes;

		if (GetNeighbouringKeyFrames(rtSeekTo, keyframes)) {
			rtSeekTo = bSeekingForward ? keyframes.second : keyframes.first;
			if (bSeekingForward && rtSeekTo <= rtPos) {
				// the end of stream is near, no keyframes before it
				return;
			}
			SeekTo(rtSeekTo);
		}
	}
}

void CMainFrame::OnUpdatePlaySeek(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	OAFilterState fs = GetMediaState();

	if (m_eMediaLoadState == MLS_LOADED && (fs == State_Paused || fs == State_Running)) {
		fEnable = true;
		if (GetPlaybackMode() == PM_DVD && (m_iDVDDomain != DVD_DOMAIN_Title || fs != State_Running)) {
			fEnable = false;
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			fEnable = false;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayGoto()
{
	if (m_eMediaLoadState != MLS_LOADED || !CanShowDialog()) {
		return;
	}

	REFTIME atpf = 0;
	if (FAILED(m_pBV->get_AvgTimePerFrame(&atpf)) || atpf < 0) {
		atpf = 0;

		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (atpf > 0) {
				break;
			}

			BeginEnumPins(pBF, pEP, pPin) {
				if (atpf > 0) {
					break;
				}

				CMediaType mt;
				pPin->ConnectionMediaType(&mt);

				if (mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo) {
					atpf = (REFTIME)((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				} else if (mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo2) {
					atpf = (REFTIME)((VIDEOINFOHEADER2*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				}
			}
			EndEnumPins;
		}
		EndEnumFilters;
	}

	REFERENCE_TIME dur = m_wndSeekBar.GetRange();
	CGoToDlg dlg(m_wndSeekBar.GetPos(), dur, atpf > 0 ? (1.0/atpf) : 0);
	if (IDOK != dlg.DoModal() || dlg.m_time < 0) {
		return;
	}

	SeekTo(dlg.m_time);
}

void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (m_eMediaLoadState == MLS_LOADED) {
		fEnable = true;
		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) {
			fEnable = false;
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			fEnable = false;
		}
	}

	pCmdUI->Enable(fEnable);
}


void CMainFrame::SetPlayingRate(double rate)
{
	if (m_eMediaLoadState != MLS_LOADED || m_bIsLiveOnline) {
		return;
	}

	HRESULT hr = E_FAIL;

	if (GetPlaybackMode() == PM_FILE) {
		if (rate < 0.125) {
			if (GetMediaState() != State_Paused) {
				SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
			}
			return;
		} else {
			if (GetMediaState() != State_Running) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			}
			hr = m_pMS->SetRate(rate);
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		if (GetMediaState() != State_Running) {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
		}
		if (rate > 0) {
			hr = m_pDVDC->PlayForwards(rate, DVD_CMD_FLAG_Block, nullptr);
		} else {
			hr = m_pDVDC->PlayBackwards(-rate, DVD_CMD_FLAG_Block, nullptr);
		}
	}

	if (SUCCEEDED(hr)) {
		m_PlaybackRate = rate;

		CString strODSMessage;
		strODSMessage.Format(ResStr(IDS_OSD_SPEED), Rate2String(m_PlaybackRate));
		m_OSD.DisplayMessage(OSD_TOPRIGHT, strODSMessage);
	}
}

void CMainFrame::OnPlayChangeRate(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED || m_bIsLiveOnline) {
		return;
	}

	if (GetPlaybackMode() != PM_FILE && GetPlaybackMode() != PM_DVD || nID != ID_PLAY_INCRATE && nID != ID_PLAY_DECRATE) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	HRESULT hr = E_FAIL;
	double PlaybackRate = 0;
	if (GetPlaybackMode() == PM_FILE) {
		if (nID == ID_PLAY_INCRATE) {
			PlaybackRate = GetNextRate(m_PlaybackRate, s.nSpeedStep);
		} else if (nID == ID_PLAY_DECRATE) {
			PlaybackRate = GetPreviousRate(m_PlaybackRate, s.nSpeedStep);
		}
		hr = m_pMS->SetRate(PlaybackRate);
	}
	else if (GetPlaybackMode() == PM_DVD) {
		if (nID == ID_PLAY_INCRATE) {
			PlaybackRate = GetNextDVDRate(m_PlaybackRate);
		} else if (nID == ID_PLAY_DECRATE) {
			PlaybackRate = GetPreviousDVDRate(m_PlaybackRate);
		}
		if (PlaybackRate >= 0.0) {
			hr = m_pDVDC->PlayForwards(PlaybackRate, DVD_CMD_FLAG_Block, nullptr);
		} else {
			hr = m_pDVDC->PlayBackwards(abs(PlaybackRate), DVD_CMD_FLAG_Block, nullptr);
		}
	}

	if (SUCCEEDED(hr)) {
		m_PlaybackRate = PlaybackRate;

		CString strODSMessage;
		strODSMessage.Format(ResStr(IDS_OSD_SPEED), Rate2String(m_PlaybackRate));
		m_OSD.DisplayMessage(OSD_TOPRIGHT, strODSMessage);
	}
}

void CMainFrame::OnUpdatePlayChangeRate(CCmdUI* pCmdUI)
{
	BOOL enable = FALSE;

	if (m_eMediaLoadState == MLS_LOADED && !m_bIsLiveOnline) {
		bool bIncRate = (pCmdUI->m_nID == ID_PLAY_INCRATE);
		bool bDecRate = (pCmdUI->m_nID == ID_PLAY_DECRATE);

		if (GetPlaybackMode() == PM_CAPTURE && m_wndCaptureBar.m_capdlg.IsTunerActive() && !m_bCapturing) {
			enable = TRUE;
		}
		else if (GetPlaybackMode() == PM_FILE) {
			if (bIncRate && m_PlaybackRate < MAXRATE || bDecRate && m_PlaybackRate > MINRATE) {
				enable = TRUE;
			}
		}
		else if (GetPlaybackMode() == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title) {
			if (bIncRate && m_PlaybackRate < MAXDVDRATE || bDecRate && m_PlaybackRate > MINDVDRATE) {
				enable = TRUE;
			}
		}
	}

	pCmdUI->Enable(enable);
}

void CMainFrame::OnPlayResetRate()
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_PlaybackRate != 1.0) {
		HRESULT hr = E_FAIL;

		if (GetPlaybackMode() == PM_FILE) {
			hr = m_pMS->SetRate(1.0);
		} else if (GetPlaybackMode() == PM_DVD) {
			hr = m_pDVDC->PlayForwards(1.0, DVD_CMD_FLAG_Block, nullptr);
		}

		if (SUCCEEDED(hr)) {
			m_PlaybackRate = 1.0;

			CString strODSMessage;
			strODSMessage.Format(ResStr(IDS_OSD_SPEED), Rate2String(m_PlaybackRate));
			m_OSD.DisplayMessage(OSD_TOPRIGHT, strODSMessage);
		}
	}
}

void CMainFrame::OnUpdatePlayResetRate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && m_PlaybackRate != 1.0);
}

void CMainFrame::SetAudioDelay(REFERENCE_TIME rtShift)
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter.p) {
		pASF->SetAudioTimeShift(rtShift);

		CString str;
		str.Format(ResStr(IDS_MAINFRM_70), rtShift/10000);
		SendStatusMessage(str, 3000);
		m_OSD.DisplayMessage(OSD_TOPLEFT, str);
	}
}

void CMainFrame::SetSubtitleDelay(int delay_ms)
{
	if (!m_pCAP && !m_pDVS) {
		return;
	}

	if (m_pCAP) {
		m_pCAP->SetSubtitleDelay(delay_ms);
	}

	RepaintVideo();

	CString strSubDelay;
	strSubDelay.Format(ResStr(IDS_MAINFRM_139), delay_ms);
	SendStatusMessage(strSubDelay, 3000);
	m_OSD.DisplayMessage(OSD_TOPLEFT, strSubDelay);
}

void CMainFrame::OnPlayChangeAudDelay(UINT nID)
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter.p) {
		REFERENCE_TIME rtShift = pASF->GetAudioTimeShift();
		rtShift +=
			nID == ID_PLAY_AUDIODELAY_PLUS ? 100000 :
			nID == ID_PLAY_AUDIODELAY_MINUS ? -100000 :
			0;

		SetAudioDelay (rtShift);
	}
}

void CMainFrame::OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_pGB /*&& !!FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB)*/);
}

void CMainFrame::OnPlayFiltersCopyToClipboard()
{
	// Don't translate that output since it's mostly for debugging purpose
	CStringW filtersList = s_strPlayerTitle;
	filtersList.Append(L"\r\nFilters currently loaded:\r\n");
	// Skip the first two entries since they are the "Copy to clipboard" menu entry and a separator
	for (int i = 2, count = m_filtersMenu.GetMenuItemCount(); i < count; i++) {
		CStringW filterName;
		m_filtersMenu.GetMenuString(i, filterName, MF_BYPOSITION);
		filtersList.AppendFormat(L"  - %s\r\n", filterName.GetString());
	}

	// Allocate a global memory object for the text
	CopyStringToClipboard(this->m_hWnd, filtersList);
}

void CMainFrame::OnPlayFilters(UINT nID)
{
	//ShowPPage(m_spparray[nID - ID_FILTERS_SUBITEM_START], m_hWnd);

	CComPtr<IUnknown> pUnk = m_pparray[nID - ID_FILTERS_SUBITEM_START];

	CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), GetModalParent());

	if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk.p) {
		ps.AddPages(pSPP);
	}

	if (CComQIPtr<IBaseFilter> pBF = pUnk.p) {
		HRESULT hr;
		CComPtr<IPropertyPage> pPP = DNew CInternalPropertyPageTempl<CPinInfoWnd>(nullptr, &hr);
		ps.AddPage(pPP, pBF);
	}

	if (ps.GetPageCount() > 0) {
		m_pFilterPropSheet = &ps;
		ps.DoModal();
		OpenSetupStatusBar();
		m_pFilterPropSheet = nullptr;
	}
}

void CMainFrame::OnUpdatePlayFilters(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_bCapturing);
}

void CMainFrame::OnSelectShaders()
{
	const bool bEnableD3D11 = IsWindows7SP1OrGreater() && AfxGetAppSettings().m_VRSettings.iVideoRenderer == VIDRNDT_MPCVR;

	if (IDOK != CShaderCombineDlg(GetModalParent(), bEnableD3D11).DoModal()) {
		return;
	}
}

void CMainFrame::OnMenuAudioOption()
{
	ShowOptions(CPPageAudio::IDD);
}

void CMainFrame::OnMenuSubtitlesOption()
{
	ShowOptions(CPPageSubtitles::IDD);
}

void CMainFrame::OnMenuSubtitlesEnable()
{
	ToggleSubtitleOnOff();
}

void CMainFrame::OnUpdateSubtitlesEnable(CCmdUI* pCmdUI)
{
	if (GetPlaybackMode() == PM_DVD && m_pDVDI) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			pCmdUI->SetCheck(!bIsDisabled);
		}
	}
	else if (m_pDVS) {
		bool fHideSubtitles = false;
		m_pDVS->get_HideSubtitles(&fHideSubtitles);
		pCmdUI->SetCheck(!fHideSubtitles);
	}
	else {
		pCmdUI->SetCheck(AfxGetAppSettings().fEnableSubtitles);
	}
}

void CMainFrame::OnMenuSubtitlesStyle()
{
	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
		std::vector<std::unique_ptr<CPPageSubStyle>> pages;
		std::vector<STSStyle*> styles;

		POSITION pos = pRTS->m_styles.GetStartPosition();
		while (pos) {
			CString key;
			STSStyle* val;
			pRTS->m_styles.GetNextAssoc(pos, key, val);

			std::unique_ptr<CPPageSubStyle> page(DNew CPPageSubStyle());
			page->InitSubStyle(key, val);
			pages.emplace_back(std::move(page));
			styles.emplace_back(val);
		}

		CString caption = ResStr(IDS_SUBTITLES_STYLES);
		caption.Replace(L"&", nullptr);

		CPropertySheet dlg(caption, GetModalParent());
		for (size_t i = 0; i < pages.size(); i++) {
			dlg.AddPage(pages[i].get());
		}

		if (dlg.DoModal() == IDOK) {
			for (size_t i = 0; i < pages.size(); i++) {
				pages[i]->GetSubStyle(styles[i]);
			}
			UpdateSubtitle(false, false);
		}
	}
}

void CMainFrame::OnUpdateSubtitlesStyle(CCmdUI* pCmdUI)
{
	if (dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
		pCmdUI->Enable(TRUE);
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CMainFrame::OnMenuSubtitlesReload()
{
	ReloadSubtitle();
}

void CMainFrame::OnUpdateSubtitlesReload(CCmdUI* pCmdUI)
{
	if (m_pDVS) {
		pCmdUI->Enable(FALSE);
	} else {
		pCmdUI->Enable(GetPlaybackMode() == PM_FILE && m_pCAP && m_pCurrentSubStream);
	}
}

void CMainFrame::OnMenuSubtitlesDefStyle()
{
	CAppSettings& s = AfxGetAppSettings();

	s.fUseDefaultSubtitlesStyle = !s.fUseDefaultSubtitlesStyle;
	UpdateSubtitle();
}

void CMainFrame::OnUpdateSubtitlesDefStyle(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AfxGetAppSettings().fUseDefaultSubtitlesStyle);
}

void CMainFrame::OnMenuSubtitlesForcedOnly()
{
	CAppSettings& s = AfxGetAppSettings();

	if (m_pDVS) {
		bool fBuffer, fOnlyShowForcedSubs, fPolygonize;
		m_pDVS->get_VobSubSettings(&fBuffer, &fOnlyShowForcedSubs, &fPolygonize);
		fOnlyShowForcedSubs = !fOnlyShowForcedSubs;
		m_pDVS->put_VobSubSettings(fBuffer, fOnlyShowForcedSubs, fPolygonize);
	} else {
		s.fForcedSubtitles = !s.fForcedSubtitles;
		g_bForcedSubtitle = s.fForcedSubtitles;
		if (m_pCAP) {
			m_pCAP->Invalidate();
		}
	}
}

void CMainFrame::OnUpdateSubtitlesForcedOnly(CCmdUI* pCmdUI)
{
	if (m_pDVS) {
		bool fOnlyShowForcedSubs = false;
		bool fBuffer, fPolygonize;
		m_pDVS->get_VobSubSettings(&fBuffer, &fOnlyShowForcedSubs, &fPolygonize);
		pCmdUI->SetCheck(fOnlyShowForcedSubs);
		return;
	} else {
		pCmdUI->SetCheck(AfxGetAppSettings().fForcedSubtitles);
	}
}

void CMainFrame::OnStereoSubtitles(UINT nID)
{
	auto& rs = GetRenderersSettings();

	CString osd = ResStr(IDS_SUBTITLES_STEREO);

	switch (nID) {
	case ID_SUBTITLES_STEREO_DONTUSE:
		rs.Stereo3DSets.iMode = SUBPIC_STEREO_NONE;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
		break;
	case ID_SUBTITLES_STEREO_SIDEBYSIDE:
		rs.Stereo3DSets.iMode = SUBPIC_STEREO_SIDEBYSIDE;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
		break;
	case ID_SUBTITLES_STEREO_TOPBOTTOM:
		rs.Stereo3DSets.iMode = SUBPIC_STEREO_TOPANDBOTTOM;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
		break;
	}

	if (m_pCAP) {
		m_pCAP->SetStereo3DSettings(&rs.Stereo3DSets);
		m_pCAP->Invalidate();
		RepaintVideo();
	}

	m_OSD.DisplayMessage(OSD_TOPLEFT, osd, 3000);
}

void CMainFrame::OnUpdateNavMixSubtitles(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_SUBTITLES_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		if (m_pDVS) {
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
				bool fHideSubtitles = false;
				m_pDVS->get_HideSubtitles(&fHideSubtitles);
				pCmdUI->Enable(!fHideSubtitles);
			}
		}
		else {
			pCmdUI->Enable(AfxGetAppSettings().fEnableSubtitles);
		}
	}
	else if (GetPlaybackMode() == PM_DVD && m_pDVDI) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			pCmdUI->Enable(!bIsDisabled);
		}
	}
}

void CMainFrame::OnSelectStream(UINT nID)
{
	nID -= ID_FILTERSTREAMS_SUBITEM_START;
	CComPtr<IAMStreamSelect> pAMSS = m_ssarray[nID];
	UINT i = nID;
	while (i > 0 && pAMSS == m_ssarray[i-1]) {
		i--;
	}

	DWORD iGr = 0;
	DWORD cStreams;
	if (!FAILED(pAMSS->Count(&cStreams))) {
		DWORD dwGroup = DWORD_MAX;
		if (!FAILED(pAMSS->Info(nID - i, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
			iGr = dwGroup;
		}
	}

	if (iGr == 1) { // Audio Stream
		bool bExternalTrack = false;
		CComQIPtr<IAMStreamSelect> pSSA = m_pSwitcherFilter.p;
		if (pSSA) {
			DWORD cStreamsA = 0;
			if (SUCCEEDED(pSSA->Count(&cStreamsA)) && cStreamsA > 1) {
				for (DWORD n = 1; n < cStreamsA; n++) {
					DWORD flags = DWORD_MAX;
					if (SUCCEEDED(pSSA->Info(n, nullptr, &flags, nullptr, nullptr, nullptr, nullptr, nullptr))) {
						if (flags & AMSTREAMSELECTINFO_EXCLUSIVE/* |flags&AMSTREAMSELECTINFO_ENABLED*/) {
							bExternalTrack = true;
							break;
						}
					}
				}
			}
		}

		if (bExternalTrack) {
			pSSA->Enable(0, AMSTREAMSELECTENABLE_ENABLE);
		}
	} else if (iGr == 2) { // Subtitle Stream
		if (m_iSubtitleSel > 0) {
			m_iSubtitleSel = 0;
			UpdateSubtitle();
		}
	}

	if (FAILED(pAMSS->Enable(nID - i, AMSTREAMSELECTENABLE_ENABLE))) {
		MessageBeep(UINT_MAX);
	}

	OpenSetupStatusBar();
}

void CMainFrame::OnMenuNavAudio()
{
	SetupAudioTracksSubMenu();
	OnMenu(&m_AudioMenu);
}

void CMainFrame::OnMenuNavSubtitle()
{
	SetupSubtitleTracksSubMenu();
	OnMenu(&m_SubtitlesMenu);
}

void CMainFrame::OnMenuNavAudioOptions()
{
	SetupAudioRButtonMenu();
	OnMenu(&m_RButtonMenu);
}

void CMainFrame::OnMenuNavSubtitleOptions()
{
	SetupSubtitlesRButtonMenu();
	OnMenu(&m_RButtonMenu);
}

void CMainFrame::OnMenuNavJumpTo()
{
	SetupNavChaptersSubMenu();
	OnMenu(&m_chaptersMenu);
}

void CMainFrame::OnMenuRecentFiles()
{
	SetupRecentFilesSubMenu();
	OnMenu(&m_recentfilesMenu);
}

void CMainFrame::OnMenuFavorites()
{
	SetupFavoritesSubMenu();
	OnMenu(&m_favoritesMenu);
}

void CMainFrame::OnUpdateMenuNavSubtitle(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (/*IsSomethingLoaded() && */m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly) {
		fEnable = true;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnUpdateMenuNavAudio(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (/*IsSomethingLoaded() && */m_eMediaLoadState == MLS_LOADED/* && !m_bAudioOnly*/) {
		fEnable = true;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnMenuAfterPlayback()
{
	OnMenu(m_AfterPlaybackMenu.GetSubMenu(0));
}

void CMainFrame::OnPlayVolume(UINT nID)
{
	CString strVolume;
	if (AfxGetAppSettings().fMute) {
		strVolume.Format(ResStr(IDS_VOLUME_OSD_MUTE), m_wndToolBar.m_volctrl.GetPos());
	} else {
		strVolume.Format(ResStr(IDS_VOLUME_OSD), m_wndToolBar.m_volctrl.GetPos());
	}

	if (m_eMediaLoadState == MLS_LOADED) {
		m_pBA->put_Volume(m_wndToolBar.Volume);
		//strVolume.Format(L"Vol : %d dB", m_wndToolBar.Volume / 100);
		SendStatusMessage(strVolume, 3000);
	}

	m_OSD.DisplayMessage(OSD_TOPLEFT, strVolume, 3000);
	m_Lcd.SetVolume((m_wndToolBar.Volume > -10000 ? m_wndToolBar.m_volctrl.GetPos() : 1));
}

void CMainFrame::OnPlayVolumeGain(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();
	if (s.bAudioAutoVolumeControl) {
		return;
	}

	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter.p) {
		switch (nID) {
		case ID_VOLUME_GAIN_INC:
			s.dAudioGain_dB = IncreaseFloatByGrid(s.dAudioGain_dB, -2);
			if (s.dAudioGain_dB > APP_AUDIOGAIN_MAX) {
				s.dAudioGain_dB = APP_AUDIOGAIN_MAX;
			}
			break;
		case ID_VOLUME_GAIN_DEC:
			s.dAudioGain_dB = DecreaseFloatByGrid(s.dAudioGain_dB, -2);
			if (s.dAudioGain_dB < APP_AUDIOGAIN_MIN) {
				s.dAudioGain_dB = APP_AUDIOGAIN_MIN;
			}
			break;
		case ID_VOLUME_GAIN_OFF:
			s.dAudioGain_dB = 0.0;
			break;
		case ID_VOLUME_GAIN_MAX:
			s.dAudioGain_dB = 10.0;
			break;
		}
		pASF->SetAudioGain(s.dAudioGain_dB);

		CString osdMessage;
		osdMessage.Format(IDS_GAIN_OSD, s.dAudioGain_dB);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnAutoVolumeControl()
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter.p) {
		CAppSettings& s = AfxGetAppSettings();

		s.bAudioAutoVolumeControl = !s.bAudioAutoVolumeControl;
		pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);

		CString osdMessage = ResStr(s.bAudioAutoVolumeControl ? IDS_OSD_AUTOVOLUMECONTROL_ON : IDS_OSD_AUTOVOLUMECONTROL_OFF);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnPlayCenterLevel(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();

	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter.p) {
		switch (nID) {
		case ID_AUDIO_CENTER_INC:
			s.dAudioCenter_dB = IncreaseFloatByGrid(s.dAudioCenter_dB, -2);
			if (s.dAudioCenter_dB > APP_AUDIOLEVEL_MAX) {
				s.dAudioCenter_dB = APP_AUDIOLEVEL_MAX;
			}
			break;
		case ID_AUDIO_CENTER_DEC:
			s.dAudioCenter_dB = DecreaseFloatByGrid(s.dAudioCenter_dB, -2);
			if (s.dAudioCenter_dB < APP_AUDIOLEVEL_MIN) {
				s.dAudioCenter_dB = APP_AUDIOLEVEL_MIN;
			}
			break;
		}
		pASF->SetLevels(s.dAudioCenter_dB, s.dAudioSurround_dB);

		CString osdMessage;
		osdMessage.Format(IDS_CENTER_LEVEL_OSD, s.dAudioCenter_dB);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnPlayColor(UINT nID)
{
	if (m_pVMRMC9 || m_pMFVP) {
		auto& rs = GetRenderersSettings();
		int& brightness = rs.iBrightness;
		int& contrast   = rs.iContrast;
		int& hue        = rs.iHue;
		int& saturation = rs.iSaturation;
		CString tmp, str;

		switch (nID) {

		case ID_COLOR_BRIGHTNESS_INC:
			brightness += 2;
		case ID_COLOR_BRIGHTNESS_DEC:
			brightness -= 1;
			SetColorControl(ProcAmp_Brightness, brightness, contrast, hue, saturation);
			if (brightness) {
				tmp.Format(L"%+d", brightness);
			} else {
				tmp = L"0";
			}
			str.Format(IDS_OSD_BRIGHTNESS, tmp);
			break;

		case ID_COLOR_CONTRAST_INC:
			contrast += 2;
		case ID_COLOR_CONTRAST_DEC:
			contrast -= 1;
			SetColorControl(ProcAmp_Contrast, brightness, contrast, hue, saturation);
			if (contrast) {
				tmp.Format(L"%+d", contrast);
			} else {
				tmp = L"0";
			}
			str.Format(IDS_OSD_CONTRAST, tmp);
			break;

		case ID_COLOR_HUE_INC:
			hue += 2;
		case ID_COLOR_HUE_DEC:
			hue -= 1;
			SetColorControl(ProcAmp_Hue, brightness, contrast, hue, saturation);
			if (hue) {
				tmp.Format(L"%+d", hue);
			} else {
				tmp = L"0";
			}
			str.Format(IDS_OSD_HUE, tmp);
			break;

		case ID_COLOR_SATURATION_INC:
			saturation += 2;
		case ID_COLOR_SATURATION_DEC:
			saturation -= 1;
			SetColorControl(ProcAmp_Saturation, brightness, contrast, hue, saturation);
			if (saturation) {
				tmp.Format(L"%+d", saturation);
			} else {
				tmp = L"0";
			}
			str.Format(IDS_OSD_SATURATION, tmp);
			break;

		case ID_COLOR_RESET:
			m_ColorControl.GetDefaultValues(brightness, contrast, hue, saturation);
			SetColorControl(ProcAmp_All, brightness, contrast, hue, saturation);
			str = ResStr(IDS_OSD_RESET_COLOR);
			break;
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, str);
	} else {
		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_NO_COLORCONTROL));
	}
}

void CMainFrame::OnAfterplayback(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();
	s.nCLSwitches &= ~CLSW_AFTERPLAYBACK_MASK;
	CString osdMsg;

	switch (nID) {
		case ID_AFTERPLAYBACK_CLOSE:
			s.nCLSwitches |= CLSW_CLOSE;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_CLOSE);
			break;
		case ID_AFTERPLAYBACK_STANDBY:
			s.nCLSwitches |= CLSW_STANDBY;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_STANDBY);
			break;
		case ID_AFTERPLAYBACK_HIBERNATE:
			s.nCLSwitches |= CLSW_HIBERNATE;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_HIBERNATE);
			break;
		case ID_AFTERPLAYBACK_SHUTDOWN:
			s.nCLSwitches |= CLSW_SHUTDOWN;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_SHUTDOWN);
			break;
		case ID_AFTERPLAYBACK_LOGOFF:
			s.nCLSwitches |= CLSW_LOGOFF;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_LOGOFF);
			break;
		case ID_AFTERPLAYBACK_LOCK:
			s.nCLSwitches |= CLSW_LOCK;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_LOCK);
			break;
		case ID_AFTERPLAYBACK_DONOTHING:
			s.nCLSwitches &= ~CLSW_AFTERPLAYBACK_MASK;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_DONOTHING);
			break;
		case ID_AFTERPLAYBACK_NEXT:
			s.fExitAfterPlayback = false;
			s.bCloseFileAfterPlayback = false;
			s.fNextInDirAfterPlayback = true;
			s.fNextInDirAfterPlaybackLooped = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_NEXT);
			break;
		case ID_AFTERPLAYBACK_NEXT_LOOPED:
			s.fExitAfterPlayback = false;
			s.bCloseFileAfterPlayback = false;
			s.fNextInDirAfterPlayback = true;
			s.fNextInDirAfterPlaybackLooped = true;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_NEXT);
			break;
		case ID_AFTERPLAYBACK_EXIT:
			s.fExitAfterPlayback = true;
			s.bCloseFileAfterPlayback = false;
			s.fNextInDirAfterPlayback = false;
			s.fNextInDirAfterPlaybackLooped = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_EXIT);
			break;
		case ID_AFTERPLAYBACK_CLOSE_FILE:
			s.fExitAfterPlayback = false;
			s.bCloseFileAfterPlayback = true;
			s.fNextInDirAfterPlayback = false;
			s.fNextInDirAfterPlaybackLooped = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_CLOSE_FILE);
			break;
		case ID_AFTERPLAYBACK_EVERYTIMEDONOTHING:
			s.fExitAfterPlayback = false;
			s.bCloseFileAfterPlayback = false;
			s.fNextInDirAfterPlayback = false;
			s.fNextInDirAfterPlaybackLooped = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_DONOTHING);
			break;
	}

	m_OSD.DisplayMessage(OSD_TOPLEFT, osdMsg);
}

void CMainFrame::OnUpdateAfterplayback(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();
	bool bChecked = false;

	switch (pCmdUI->m_nID) {
		case ID_AFTERPLAYBACK_CLOSE:
			bChecked = !!(s.nCLSwitches & CLSW_CLOSE);
			break;
		case ID_AFTERPLAYBACK_STANDBY:
			bChecked = !!(s.nCLSwitches & CLSW_STANDBY);
			break;
		case ID_AFTERPLAYBACK_HIBERNATE:
			bChecked = !!(s.nCLSwitches & CLSW_HIBERNATE);
			break;
		case ID_AFTERPLAYBACK_SHUTDOWN:
			bChecked = !!(s.nCLSwitches & CLSW_SHUTDOWN);
			break;
		case ID_AFTERPLAYBACK_LOGOFF:
			bChecked = !!(s.nCLSwitches & CLSW_LOGOFF);
			break;
		case ID_AFTERPLAYBACK_LOCK:
			bChecked = !!(s.nCLSwitches & CLSW_LOCK);
			break;
		case ID_AFTERPLAYBACK_DONOTHING:
			bChecked = !(s.nCLSwitches & CLSW_AFTERPLAYBACK_MASK);
			break;
		case ID_AFTERPLAYBACK_EXIT:
			bChecked = !!s.fExitAfterPlayback;
			break;
		case ID_AFTERPLAYBACK_CLOSE_FILE:
			bChecked = !!s.bCloseFileAfterPlayback;
			break;
		case ID_AFTERPLAYBACK_NEXT:
			bChecked = !!s.fNextInDirAfterPlayback && !s.fNextInDirAfterPlaybackLooped;
			break;
		case ID_AFTERPLAYBACK_NEXT_LOOPED:
			bChecked = !!s.fNextInDirAfterPlayback && !!s.fNextInDirAfterPlaybackLooped;
			break;
		case ID_AFTERPLAYBACK_EVERYTIMEDONOTHING:
			bChecked = !s.fExitAfterPlayback && !s.fNextInDirAfterPlayback && !s.bCloseFileAfterPlayback;
			break;
	}

	SetMenuRadioCheck(pCmdUI, bChecked);
}

// navigate
void CMainFrame::OnNavigateSkip(UINT nID)
{
	if (GetPlaybackMode() == PM_FILE) {
		m_flastnID = nID;

		if (DWORD nChapters = m_pCB->ChapGetCount()) {
			REFERENCE_TIME rtCur;
			m_pMS->GetCurrentPosition(&rtCur);

			REFERENCE_TIME rt = rtCur;
			CComBSTR name;
			long i = 0;

			if (nID == ID_NAVIGATE_SKIPBACK) {
				rt -= 30000000;
				i = m_pCB->ChapLookup(&rt, &name);
			} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
				i = m_pCB->ChapLookup(&rt, &name) + 1;
				name.Empty();
				if (i < (long)nChapters) {
					m_pCB->ChapGet(i, &rt, &name);
				}
			}

			if (i >= 0 && (DWORD)i < nChapters) {
				SeekTo(rt, false);

				if (name.Length()) {
					SendStatusMessage(ResStr(IDS_AG_CHAPTER2) + name, 3000);
				}

				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				CString strOSD;
				CString chapName;
				if (name.Length()) {
					chapName.Format(L" - \"%s\"", name);
				}
				strOSD.Format(L"%s/%s %s%d/%u%s", ReftimeToString2(rt), ReftimeToString2(rtDur), ResStr(IDS_AG_CHAPTER2), i + 1, nChapters, chapName);
				m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
				return;
			}
		}

		if (nID == ID_NAVIGATE_SKIPBACK) {
			SendMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACKFILE);
		} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
			SendMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARDFILE);
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_PlaybackRate = 1.0;

		if (GetMediaState() != State_Running) {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
		}

		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		m_pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		m_pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		if (nID == ID_NAVIGATE_SKIPBACK) {
			if (Location.ChapterNum == 1 && Location.TitleNum > 1) {
				m_pDVDI->GetNumberOfChapters(Location.TitleNum - 1, &ulNumOfChapters);
				m_pDVDC->PlayChapterInTitle(Location.TitleNum - 1, ulNumOfChapters, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
			} else {
				ULONG tsec = (Location.TimeCode.bHours * 3600)
							 + (Location.TimeCode.bMinutes * 60)
							 + (Location.TimeCode.bSeconds);
				ULONG diff = 0;
				if ( m_lChapterStartTime != 0xFFFFFFFF && tsec > m_lChapterStartTime ) {
					diff = tsec - m_lChapterStartTime;
				}
				// Restart the chapter if more than 7 seconds have passed
				if ( diff <= 7 ) {
					m_pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
				} else {
					m_pDVDC->ReplayChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
				}
			}
		} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
			if (Location.ChapterNum == ulNumOfChapters && Location.TitleNum < ulNumOfTitles) {
				m_pDVDC->PlayChapterInTitle(Location.TitleNum + 1, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
			} else {
				m_pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
			}
		}

		if ((m_pDVDI->GetCurrentLocation(&Location) == S_OK))
		{
			m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
			CString strTitle;
			strTitle.Format(IDS_AG_TITLE2, Location.TitleNum, ulNumOfTitles);

			REFERENCE_TIME stop = m_wndSeekBar.GetRange();

			CString strOSD;
			if (stop > 0) {
				DVD_HMSF_TIMECODE stopHMSF = RT2HMS_r(stop);
				strOSD.Format(L"%s/%s %s, %s%02u/%02u", DVDtimeToString(Location.TimeCode, stopHMSF.bHours > 0), DVDtimeToString(stopHMSF),
							  strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			} else {
				strOSD.Format(L"%s, %s%02u/%02u", strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			}

			m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);

			SetupChapters();
		}

		/*
		if (nID == ID_NAVIGATE_SKIPBACK)
			pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block, nullptr);
		else if (nID == ID_NAVIGATE_SKIPFORWARD)
			pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block, nullptr);
		*/
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		if (AfxGetAppSettings().iDefaultCaptureDevice == 1) {
			CComQIPtr<IBDATuner> pTun = m_pGB.p;
			if (pTun) {
				int nCurrentChannel;
				CAppSettings& s = AfxGetAppSettings();

				nCurrentChannel = s.nDVBLastChannel;

				if (nID == ID_NAVIGATE_SKIPBACK) {
					pTun->SetChannel (nCurrentChannel - 1);
					DisplayCurrentChannelOSD();
					if (m_wndNavigationBar.IsVisible()) {
						m_wndNavigationBar.m_navdlg.UpdatePos(nCurrentChannel - 1);
					}
				} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
					pTun->SetChannel (nCurrentChannel + 1);
					DisplayCurrentChannelOSD();
					if (m_wndNavigationBar.IsVisible()) {
						m_wndNavigationBar.m_navdlg.UpdatePos(nCurrentChannel + 1);
					}
				}
			}
		}
		else if (m_pAMTuner) {
			if (GetMediaState() != State_Running) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			}

			long lChannelMin = 0, lChannelMax = 0;
			m_pAMTuner->ChannelMinMax(&lChannelMin, &lChannelMax);
			long lChannel = 0, lVivSub = 0, lAudSub = 0;
			m_pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);

			if (nID == ID_NAVIGATE_SKIPBACK) {
				if (lChannel > lChannelMin) {
					lChannel--;
				} else {
					lChannel = lChannelMax;
				}
			}
			else if (nID == ID_NAVIGATE_SKIPFORWARD) {
				if (lChannel < lChannelMax) {
					lChannel++;
				} else {
					lChannel = lChannelMin;
				}
			}

			if (FAILED(m_pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT))) {
				return;
			}

			long flFoundSignal = 0;
			m_pAMTuner->AutoTune(lChannel, &flFoundSignal);
			long lFreq = 0;
			m_pAMTuner->get_VideoFrequency(&lFreq);
			long lSignalStrength;
			m_pAMTuner->SignalPresent(&lSignalStrength); // good if AMTUNER_SIGNALPRESENT

			CString strOSD;
			strOSD.Format(ResStr(IDS_CAPTURE_CHANNEL_FREQ), lChannel, lFreq/1000000.0);
			m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		}
	}
}

void CMainFrame::OnUpdateNavigateSkip(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsNavigateSkipEnabled());
}

bool CMainFrame::IsNavigateSkipEnabled()
{
	bool bEnabled = false;
	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu && m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu) {
			bEnabled = true;
		} else if (GetPlaybackMode() == PM_CAPTURE && !m_bCapturing) {
			bEnabled = true;
		} else if (GetPlaybackMode() == PM_FILE) {
			if (m_pCB->ChapGetCount() > 1) {
				bEnabled = true;
			} else if (m_wndPlaylistBar.GetCount() == 1) {
				if (m_bIsBDPlay) {
					bEnabled = !m_BDPlaylists.empty();
				} else if (!AfxGetAppSettings().fDontUseSearchInFolder) {
					bEnabled = true;
				}
			} else {
				bEnabled = true;
			}
		}
	}

	return bEnabled;
}

void CMainFrame::OnNavigateSkipFile(UINT nID)
{
	if (m_bIsBDPlay && m_wndPlaylistBar.GetCount() == 1) {
		if (!m_BDPlaylists.empty()) {
			auto it = std::find_if(m_BDPlaylists.cbegin(), m_BDPlaylists.cend(), [&](const CHdmvClipInfo::PlaylistItem& item) {
				return item.m_strFileName == m_strPlaybackRenderedPath;
			});

			if (nID == ID_NAVIGATE_SKIPBACKFILE) {
				if (it == m_BDPlaylists.cbegin()) {
					return;
				}

				--it;
			} else if (nID == ID_NAVIGATE_SKIPFORWARDFILE) {
				if (it == std::prev(m_BDPlaylists.cend())) {
					return;
				}
				++it;
			}

			const CString fileName(it->m_strFileName);
			m_bNeedUnmountImage = FALSE;
			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_bIsBDPlay = TRUE;

			OpenFile(fileName, INVALID_TIME, FALSE);
		}
	} else if (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_CAPTURE) {
		if (m_wndPlaylistBar.GetCount() == 1) {
			if (GetPlaybackMode() == PM_CAPTURE || AfxGetAppSettings().fDontUseSearchInFolder) {
				SendMessageW(WM_COMMAND, ID_PLAY_STOP); // do not remove this, unless you want a circular call with OnPlayPlay()
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			} else {
				if (nID == ID_NAVIGATE_SKIPBACKFILE) {
					if (!SearchInDir(false)) {
						m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_FIRST_IN_FOLDER));
					}
				} else if (nID == ID_NAVIGATE_SKIPFORWARDFILE) {
					if (!SearchInDir(true)) {
						m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_LAST_IN_FOLDER));
					}
				}
			}
		} else {
			if (nID == ID_NAVIGATE_SKIPBACKFILE) {
				m_wndPlaylistBar.SetPrev();
			} else if (nID == ID_NAVIGATE_SKIPFORWARDFILE) {
				m_wndPlaylistBar.SetNext();
			}

			OpenCurPlaylistItem();
		}
	}
}

void CMainFrame::OnUpdateNavigateSkipFile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED
				   && ((GetPlaybackMode() == PM_FILE && (m_wndPlaylistBar.GetCount() > 1 || !AfxGetAppSettings().fDontUseSearchInFolder))
					   || (GetPlaybackMode() == PM_CAPTURE && !m_bCapturing && m_wndPlaylistBar.GetCount() > 1)));
}

void CMainFrame::OnNavigateMenu(UINT nID)
{
	nID -= ID_NAVIGATE_TITLEMENU;

	if (m_eMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_DVD) {
		return;
	}

	m_PlaybackRate = 1.0;

	if (GetMediaState() != State_Running) {
		SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
	}

	m_pDVDC->ShowMenu((DVD_MENU_ID)(nID+2), DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
}

void CMainFrame::OnUpdateNavigateMenu(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_TITLEMENU;
	ULONG ulUOPs;

	if (m_eMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_DVD
			|| FAILED(m_pDVDI->GetCurrentUOPS(&ulUOPs))) {
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(!(ulUOPs & (UOP_FLAG_ShowMenu_Title << nID)));
}

void CMainFrame::OnNavigateAudio(UINT nID)
{
	nID -= ID_AUDIO_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			pSS->Enable(nID, AMSTREAMSELECTENABLE_ENABLE);
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectAudioStream(nID, DVD_CMD_FLAG_Block, nullptr);
	}
}

void CMainFrame::OnNavigateSubpic(UINT nID)
{
	nID -= ID_SUBTITLES_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		SelectSubtilesAMStream(nID);
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectSubpictureStream(nID, DVD_CMD_FLAG_Block, nullptr);
		m_pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, nullptr);
	}
}

void CMainFrame::SelectSubtilesAMStream(UINT id)
{
	const auto& s = AfxGetAppSettings();

	int streamCount = GetStreamCount(SUBTITLE_GROUP);
	if (streamCount) {
		streamCount--;
	}

	if (GetPlaybackMode() == PM_FILE && !m_pDVS) {
		if (id != NO_SUBTITLES_INDEX) {
			size_t subStreamCount = 0;
			for (const auto& pSubStream : m_pSubStreams) {
				subStreamCount += pSubStream->GetStreamCount();
			}

			if (id >= (subStreamCount + streamCount)) {
				id = 0;
			}
		}
	}

	int splsubcnt = 0;
	const int i = (int)id;

	if (GetPlaybackMode() == PM_FILE && m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

			int subcount = GetStreamCount(SUBTITLE_GROUP);
			if (subcount) {
				CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
				DWORD cStreams = 0;
				pSS->Count(&cStreams);

				if (nLangs > 1) {
					if (i < (nLangs-1)) {
						m_pDVS->put_SelectedLanguage(i);
						return;
					} else {
						m_pDVS->put_SelectedLanguage(nLangs - 1);
						id -= (nLangs - 1);
					}
				}

				for (int m = 0, j = cStreams; m < j; m++) {
					DWORD dwGroup = DWORD_MAX;

					if (FAILED(pSS->Info(m, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
						continue;
					}

					if (dwGroup != SUBTITLE_GROUP) {
						continue;
					}

					if (id == 0) {
						pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
						return;
					}

					id--;
				}
			}

			if (i <= (nLangs - 1)) {
				m_pDVS->put_SelectedLanguage(i);
			}
		}

		return;
	}

	if ((GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1)) && i >= 0) {

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
		if (pSS) {
			DWORD cStreams = 0;
			if (!FAILED(pSS->Count(&cStreams))) {

				CComQIPtr<IBaseFilter> pBF = pSS.p;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
					cStreams--;
				}

				for (DWORD m = 0, j = cStreams; m < j; m++) {
					DWORD dwGroup = DWORD_MAX;

					if (FAILED(pSS->Info(m, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
						continue;
					}

					if (dwGroup != SUBTITLE_GROUP) {
						continue;
					}
					splsubcnt++;

					if (id == 0) {
						pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
						break;
					}

					id--;
				}
			}
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectSubpictureStream(id, DVD_CMD_FLAG_Block, nullptr);
		return;
	}

	int m = splsubcnt - (splsubcnt > 0 ? 1 : 0);
	m = i - m;

	m_iSubtitleSel = m;
	m_nSelSub2 = m_iSubtitleSel;
	if (!s.fEnableSubtitles) {
		m_iSubtitleSel = -1;
	}

	UpdateSubtitle();
}

void CMainFrame::OnNavigateAngle(UINT nID)
{
	nID -= ID_VIDEO_STREAMS_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		SelectAMStream(nID, 0);
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectAngle(nID+1, DVD_CMD_FLAG_Block, nullptr);

		CString osdMessage;
		osdMessage.Format(ResStr(IDS_AG_ANGLE), nID+1);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnNavigateChapters(UINT nID)
{
	if (nID < ID_NAVIGATE_CHAP_SUBITEM_START) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		UINT id = nID - ID_NAVIGATE_CHAP_SUBITEM_START;

		if (id < m_BDPlaylists.size()) {
			UINT idx = 0;
			for (const auto& item : m_BDPlaylists) {
				if (idx == id && item.m_strFileName != m_strPlaybackRenderedPath) {
					m_bNeedUnmountImage = FALSE;
					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					m_bIsBDPlay = TRUE;

					OpenFile(item.m_strFileName, INVALID_TIME, FALSE);
					return;
				}
				idx++;
			}
		}

		if (!m_BDPlaylists.empty()) {
			id -= m_BDPlaylists.size();
		}

		if (m_youtubeUrllist.size() > 1 && id < m_youtubeUrllist.size()) {
			UINT idx = 0;
			for (const auto& item : m_youtubeUrllist) {
				if (idx == id && item.url != m_strPlaybackRenderedPath) {
					const int tagSelected = item.profile->iTag;
					m_bYoutubeOpened = true;

					REFERENCE_TIME rtNow = INVALID_TIME;
					m_pMS->GetCurrentPosition(&rtNow);

					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

					AfxGetAppSettings().iYoutubeTagSelected = tagSelected;
					OpenCurPlaylistItem(rtNow, FALSE);
					return;
				}
				idx++;
			}
		}

		if (m_youtubeUrllist.size() > 1) {
			id -= m_youtubeUrllist.size();
		}

		if (id >= 0 && id < (UINT)m_pCB->ChapGetCount() && m_pCB->ChapGetCount() > 1) {
			REFERENCE_TIME rt;
			CComBSTR name;
			if (SUCCEEDED(m_pCB->ChapGet(id, &rt, &name))) {
				SeekTo(rt, false);

				if (name.Length()) {
					SendStatusMessage(ResStr(IDS_AG_CHAPTER2) + name, 3000);
				}

				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				CString strOSD;
				CString chapName;
				if (name.Length()) {
					chapName.Format(L" - \"%s\"", name);
				}
				strOSD.Format(L"%s/%s %s%u/%u%s", ReftimeToString2(rt), ReftimeToString2(rtDur), ResStr(IDS_AG_CHAPTER2), id + 1, m_pCB->ChapGetCount(), chapName);
				m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
			}
			return;
		}

		if (m_pCB->ChapGetCount() > 1) {
			id -= m_pCB->ChapGetCount();
		}

		if (id >= 0 && id < (UINT)m_wndPlaylistBar.GetCount() && m_wndPlaylistBar.GetSelIdx() != id) {
			m_wndPlaylistBar.SetSelIdx(id);
			OpenCurPlaylistItem();
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		m_pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		m_pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		nID -= (ID_NAVIGATE_CHAP_SUBITEM_START - 1);

		if (nID <= ulNumOfTitles) {
			m_pDVDC->PlayTitle(nID, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr); // sometimes this does not do anything ...
			m_pDVDC->PlayChapterInTitle(nID, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr); // ... but this does!
		} else {
			nID -= ulNumOfTitles;

			if (nID <= ulNumOfChapters) {
				m_pDVDC->PlayChapter(nID, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
			}
		}

		if ((m_pDVDI->GetCurrentLocation(&Location) == S_OK)) {
			m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
			CString strTitle;
			strTitle.Format(IDS_AG_TITLE2, Location.TitleNum, ulNumOfTitles);

			REFERENCE_TIME stop = m_wndSeekBar.GetRange();

			CString strOSD;
			if (stop > 0) {
				DVD_HMSF_TIMECODE stopHMSF = RT2HMS_r(stop);
				strOSD.Format(L"%s/%s %s, %s%02u/%02u", DVDtimeToString(Location.TimeCode, stopHMSF.bHours > 0), DVDtimeToString(stopHMSF),
							  strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			} else {
				strOSD.Format(L"%s, %s%02u/%02u", strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			}

			m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		CAppSettings& s = AfxGetAppSettings();

		nID -= ID_NAVIGATE_CHAP_SUBITEM_START;

		if (s.iDefaultCaptureDevice == 1) {
			CComQIPtr<IBDATuner> pTun = m_pGB.p;
			if (pTun) {
				if (s.nDVBLastChannel != nID) {
					pTun->SetChannel (nID);
					DisplayCurrentChannelOSD();
					if (m_wndNavigationBar.IsVisible()) {
						m_wndNavigationBar.m_navdlg.UpdatePos(nID);
					}
				}
			}
		}
	}
}

void CMainFrame::OnNavigateMenuItem(UINT nID)
{
	if (GetPlaybackMode() == PM_DVD) {
		switch (nID) {
			case ID_NAVIGATE_MENU_LEFT:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Left);
				break;
			case ID_NAVIGATE_MENU_RIGHT:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Right);
				break;
			case IDS_AG_DVD_MENU_UP:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Upper);
				break;
			case ID_NAVIGATE_MENU_DOWN:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Lower);
				break;
			case ID_NAVIGATE_MENU_ACTIVATE:
				//if (m_iDVDDomain != DVD_DOMAIN_Title) { // for the remote control
					m_pDVDC->ActivateButton();
				//} else {
				//	OnPlayPlay();
				//}
				break;
			case ID_NAVIGATE_MENU_BACK:
				m_pDVDC->ReturnFromSubmenu(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
				break;
			case ID_NAVIGATE_MENU_LEAVE:
				m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
				break;
			default:
				break;
		}
	}
}

void CMainFrame::OnUpdateNavigateMenuItem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_DVD);
}

void CMainFrame::OnTunerScan()
{
	CTunerScanDlg Dlg;
	Dlg.DoModal();
}

void CMainFrame::OnUpdateTunerScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_eMediaLoadState == MLS_LOADED) &&
				   (AfxGetAppSettings().iDefaultCaptureDevice == 1) &&
				   ((GetPlaybackMode() == PM_CAPTURE)));
}

// favorites

class CDVDStateStream : public CUnknown, public IStream
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
		return
			QI(IStream)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	__int64 m_pos;

public:
	CDVDStateStream() : CUnknown(L"CDVDStateStream", nullptr) {
		m_pos = 0;
	}

	DECLARE_IUNKNOWN;

	std::vector<BYTE> m_data;

	// ISequentialStream
	STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) {
		__int64 cbRead = std::min((__int64)(m_data.size() - m_pos), (__int64)cb);
		cbRead = std::max(cbRead, 0LL);
		memcpy(pv, &m_data[(INT_PTR)m_pos], (int)cbRead);
		if (pcbRead) {
			*pcbRead = (ULONG)cbRead;
		}
		m_pos += cbRead;
		return S_OK;
	}
	STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten) {
		BYTE* p = (BYTE*)pv;
		ULONG cbWritten = (ULONG)-1;
		while (++cbWritten < cb) {
			m_data.emplace_back(*p++);
		}
		if (pcbWritten) {
			*pcbWritten = cbWritten;
		}
		return S_OK;
	}

	// IStream
	STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) {
		return E_NOTIMPL;
	}
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) {
		return E_NOTIMPL;
	}
	STDMETHODIMP CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {
		return E_NOTIMPL;
	}
	STDMETHODIMP Commit(DWORD grfCommitFlags) {
		return E_NOTIMPL;
	}
	STDMETHODIMP Revert() {
		return E_NOTIMPL;
	}
	STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) {
		return E_NOTIMPL;
	}
	STDMETHODIMP Clone(IStream** ppstm) {
		return E_NOTIMPL;
	}
};

void CMainFrame::OnFavoritesAdd()
{
	AddFavorite();
}

void CMainFrame::OnUpdateFavoritesAdd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD);
}

void CMainFrame::OnFavoritesQuickAdd()
{
	AddFavorite(true, false);
}

void CMainFrame::AddFavorite(bool bDisplayMessage/* = false*/, bool bShowDialog/* = true*/)
{
	if (GetPlaybackMode() != PM_FILE && GetPlaybackMode() != PM_DVD || m_SessionInfo.Path.IsEmpty()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();
	CString osdMsg;

	SessionInfo sesInfo = m_SessionInfo;
	sesInfo.CleanPosition();

	std::list<CString> descList;
	if (m_FileName.GetLength()) {
		descList.emplace_back(m_FileName);
	}
	if (m_SessionInfo.Title.GetLength()) {
		descList.emplace_back(m_SessionInfo.Title);
	}

	if (bShowDialog) {
		CFavoriteAddDlg dlg(descList, sesInfo.Path);
		if (dlg.DoModal() != IDOK) {
			return;
		}
		sesInfo.Title = dlg.m_name;
	} else {
		sesInfo.Title = descList.front();
	}

	// RelativeDrive
	if (s.bFavRelativeDrive && StartsWith(sesInfo.Path, L":\\", 1)) {
		sesInfo.Path.SetAt(0, '?');
	}

	if (GetPlaybackMode() == PM_FILE) {
		// RememberPos
		if (s.bFavRememberPos) {
			sesInfo.Position    = GetPos();
			sesInfo.AudioNum    = GetAudioTrackIdx();
			sesInfo.SubtitleNum = GetSubtitleTrackIdx();
		}
		AfxGetMyApp()->m_FavoritesFile.AppendFavorite(sesInfo);
		osdMsg = ResStr(IDS_FILE_FAV_ADDED);
	}
	else if (GetPlaybackMode() == PM_DVD) {
		// RememberPos
		if (s.bFavRememberPos && m_iDVDTitleForHistory > 0) {
			CDVDStateStream stream;
			stream.AddRef();
			CComPtr<IDvdState> pStateData;
			CComQIPtr<IPersistStream> pPersistStream;
			if (SUCCEEDED(m_pDVDI->GetState(&pStateData))
				&& (pPersistStream = pStateData)
				&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream))) {
				sesInfo.DVDState = stream.m_data;
			}
			sesInfo.DVDTitle = m_iDVDTitleForHistory;
			sesInfo.DVDTimecode = m_SessionInfo.DVDTimecode;
		}
		AfxGetMyApp()->m_FavoritesFile.AppendFavorite(sesInfo);
		osdMsg = ResStr(IDS_DVD_FAV_ADDED);
	}

	if (bDisplayMessage && !osdMsg.IsEmpty()) {
		SendStatusMessage(osdMsg, 3000);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMsg, 3000);
	}
}

void CMainFrame::OnFavoritesOrganize()
{
	CFavoriteOrganizeDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnShowHistory()
{
	if (!m_pHistoryDlg) {
		m_pHistoryDlg.reset(new CHistoryDlg(this));
		m_pHistoryDlg->Create(IDD_HISTORY);
	}
	else if (m_pHistoryDlg->IsWindowVisible()) {
		m_pHistoryDlg->SetActiveWindow();
	}
	else {
		m_pHistoryDlg->ShowWindow(SW_SHOW);
	}
}

void CMainFrame::OnRecentFileClear()
{
	if (IDYES != AfxMessageBox(ResStr(IDS_RECENT_FILES_QUESTION), MB_YESNO)) {
		return;
	}

	auto& historyFile = AfxGetMyApp()->m_HistoryFile;
	if (historyFile.Clear()) {
		// Empty the "Recent" jump list
		CComPtr<IApplicationDestinations> pDests;
		HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, nullptr, CLSCTX_INPROC_SERVER);
		if (SUCCEEDED(hr)) {
			hr = pDests->RemoveAllDestinations();
		}

		AfxGetAppSettings().strLastOpenFile.Empty();
	}
}

void CMainFrame::OnUpdateRecentFileClear(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CMainFrame::OnFavoritesFile(UINT nID)
{
	nID -= ID_FAVORITES_FILE_START;

	if (nID < m_FavFiles.size()) {
		auto it = std::next(m_FavFiles.begin(), nID);
		PlayFavoriteFile(*it);
	}
}

void CMainFrame::PlayFavoriteFile(SessionInfo fav) // use a copy of SessionInfo
{
	m_nAudioTrackStored = fav.AudioNum;
	m_nSubtitleTrackStored = fav.SubtitleNum;

	// NOTE: This is just for the favorites but we could add a global settings that does this always when on.
	//       Could be useful when using removable devices. All you have to do then is plug in your 500 gb drive,
	//       full with movies and/or music, start MPC-BE (from the 500 gb drive) with a preloaded playlist and press play.
	if (StartsWith(fav.Path, L"?:\\")) {
		CString exepath(GetProgramPath());

		if (StartsWith(exepath, L":\\", 1)) {
			fav.Path.SetAt(0, exepath[0]);
		}
	}

	m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack = m_nAudioTrackStored;
	m_wndPlaylistBar.curPlayList.m_nSelectedSubtitleTrack = m_nSubtitleTrackStored;

	if (!m_wndPlaylistBar.SelectFileInPlaylist(fav.Path)) {
		m_wndPlaylistBar.Open(fav.Path);
	}

	if (GetPlaybackMode() == PM_FILE && fav.Path == m_lastOMD->title && !m_bEndOfStream) {
		if (m_nAudioTrackStored != -1) {
			SetAudioTrackIdx(m_nAudioTrackStored);
		}
		if (m_nSubtitleTrackStored != -1) {
			SetSubtitleTrackIdx(m_nSubtitleTrackStored);
		}

		m_wndPlaylistBar.SetFirstSelected();
		m_pMS->SetPositions(&fav.Position, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
		OnPlayPlay();
	} else {
		OpenCurPlaylistItem(fav.Position);
	}
}

void CMainFrame::OnRecentFile(UINT nID)
{
	nID -= ID_RECENT_FILE_START;

	if (nID < m_RecentPaths.size() && m_RecentPaths[nID].GetLength()) {
		if (!m_wndPlaylistBar.SelectFileInPlaylist(m_RecentPaths[nID])) {
			m_wndPlaylistBar.Open(m_RecentPaths[nID]);
		}
		OpenCurPlaylistItem();
	}
}

void CMainFrame::OnFavoritesDVD(UINT nID)
{
	nID -= ID_FAVORITES_DVD_START;

	if (nID < m_FavDVDs.size()) {
		auto it = std::next(m_FavDVDs.begin(), nID);
		PlayFavoriteDVD(*it);
	}
}

void CMainFrame::PlayFavoriteDVD(SessionInfo fav) // use a copy of SessionInfo
{
	if (StartsWith(fav.Path, L"?:\\")) {
		CString exepath(GetProgramPath());
		if (StartsWith(exepath, L":\\", 1)) {
			fav.Path.SetAt(0, exepath[0]);
		}
	}

	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	CComPtr<IDvdState> pDvdState;
	if (fav.DVDState.size()) {
		CDVDStateStream stream;
		stream.AddRef();
		stream.m_data = fav.DVDState;
		HRESULT hr = OleLoadFromStream((IStream*)&stream, IID_PPV_ARGS(&pDvdState));
		UNREFERENCED_PARAMETER(hr);
	}

	AfxGetAppSettings().bNormalStartDVD = false;

	std::unique_ptr<OpenDVDData> p(DNew OpenDVDData());
	if (p) {
		p->path = fav.Path;
		p->pDvdState = pDvdState;
	}
	OpenMedia(std::move(p));
}

// help

void CMainFrame::OnHelpHomepage()
{
	ShellExecuteW(m_hWnd, L"open", _CRT_WIDE(MPC_VERSION_COMMENTS), nullptr, nullptr, SW_SHOWDEFAULT);
}

void CMainFrame::OnHelpCheckForUpdate()
{
	UpdateChecker updatechecker;
	updatechecker.CheckForUpdate();
}

/*
void CMainFrame::OnHelpDocumentation()
{
	ShellExecuteW(m_hWnd, L"open", L"", nullptr, nullptr, SW_SHOWDEFAULT);
}
*/

void CMainFrame::OnHelpToolbarImages()
{
	ShellExecuteW(m_hWnd, L"open", L"https://sourceforge.net/projects/mpcbe/files/Toolbars/", nullptr, nullptr, SW_SHOWDEFAULT);
}

/*
void CMainFrame::OnHelpDonate()
{
	ShellExecuteW(m_hWnd, L"open", L"", nullptr, nullptr, SW_SHOWDEFAULT);
}
*/

void CMainFrame::OnSubtitlePos(UINT nID)
{
	if (m_pCAP) {
		auto& rs = GetRenderersSettings();
		switch (nID) {
			case ID_SUB_POS_UP:
				rs.SubpicSets.ShiftPos.y--;
				break;
			case ID_SUB_POS_DOWN:
				rs.SubpicSets.ShiftPos.y++;
				break;
			case ID_SUB_POS_LEFT:
				rs.SubpicSets.ShiftPos.x--;
				break;
			case ID_SUB_POS_RIGHT:
				rs.SubpicSets.ShiftPos.x++;
				break;
			case ID_SUB_POS_RESTORE:
				rs.SubpicSets.ShiftPos = { 0, 0 };
				break;
		}

		m_pCAP->SetSubpicSettings(&rs.SubpicSets);

		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	}
}

void CMainFrame::OnSubtitleSize(UINT nID)
{
	if (m_pCAP) {
		CAppSettings& s = AfxGetAppSettings();

		if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
			if (nID == ID_SUB_SIZE_DEC && s.subdefstyle.fontSize > 8) {
				s.subdefstyle.fontSize--;
			}
			else if (nID == ID_SUB_SIZE_INC && s.subdefstyle.fontSize < 48) {
				s.subdefstyle.fontSize++;
			}
			else {
				return;
			}

			s.subdefstyle.fontSize = std::round(s.subdefstyle.fontSize);

			{
				CAutoLock cAutoLock(&m_csSubLock);

				pRTS->SetOverride(s.fUseDefaultSubtitlesStyle, s.subdefstyle);
				if (!s.fUseDefaultSubtitlesStyle) {
					pRTS->SetDefaultStyle(s.subdefstyle);
				}
				pRTS->Deinit();
			}
			InvalidateSubtitle();
			if (GetMediaState() != State_Running) {
				m_pCAP->Paint(false);
			}

			CStringW str;
			str.Format(ResStr(IDS_SUB_SIZE), (int)s.subdefstyle.fontSize);
			m_OSD.DisplayMessage(OSD_TOPRIGHT, str);
		}
	}
}

void CMainFrame::OnSubCopyClipboard()
{
	if (m_pCAP && GetPlaybackMode() == PM_FILE && m_pMS) {
		if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
			REFERENCE_TIME rtNow = 0;
			m_pMS->GetCurrentPosition(&rtNow);
			rtNow -= 10000i64 * m_pCAP->GetSubtitleDelay();

			CString text;
			if (pRTS->GetText(rtNow, m_pCAP->GetFPS(), text)) {
				CopyStringToClipboard(this->m_hWnd, text);
			}
		}
	}
}

void CMainFrame::OnAddToPlaylistFromClipboard()
{
	m_wndPlaylistBar.PasteFromClipboard();
}

void CMainFrame::OnChangeMouseEasyMove()
{
	CAppSettings& s = AfxGetAppSettings();
	s.bMouseEasyMove = !s.bMouseEasyMove;

	m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(s.bMouseEasyMove ? IDS_MOVEWINDOWBYVIDEO_ON : IDS_MOVEWINDOWBYVIDEO_OFF));
}

void CMainFrame::OnPlaylistOpenFolder()
{
	if (m_wndPlaylistBar.GetCount() == 0) {
		return;
	}

	CPlaylistItem pli;
	if (m_wndPlaylistBar.GetCur(pli) && pli.m_fi.Valid()) {
		ShellExecuteW(nullptr, nullptr, L"explorer.exe", L"/select,\"" + pli.m_fi.GetPath() + L"\"", nullptr, SW_SHOWNORMAL);
	}
}

void CMainFrame::SetDefaultWindowRect(int iMonitor, const bool bLastCall)
{
	const CAppSettings& s = AfxGetAppSettings();
	const auto nLastWindowType  = s.nLastWindowType;
	const auto ptLastWindowPos  = s.ptLastWindowPos;
	const auto szLastWindowSize = s.szLastWindowSize;

	if (s.iCaptionMenuMode != MODE_SHOWCAPTIONMENU) {
		if (s.iCaptionMenuMode == MODE_FRAMEONLY) {
			ModifyStyle(WS_CAPTION, 0, SWP_NOZORDER);
		} else if (s.iCaptionMenuMode == MODE_BORDERLESS) {
			ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0, SWP_NOZORDER);
		}
		SetMenuBarVisibility(AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
		SetWindowPos(nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	CSize windowSize;
	if (s.HasFixedWindowSize()) {
		windowSize = s.sizeFixedWindow;
	}
	else if (s.nStartupWindowMode == STARTUPWND_REMLAST) {
		windowSize = szLastWindowSize;
	}
	else if (s.nStartupWindowMode == STARTUPWND_SPECIFIED) {
		windowSize = s.szSpecifiedWndSize;
	}
	else {
		CRect windowRect;
		GetWindowRect(&windowRect);
		CRect clientRect;
		GetClientRect(&clientRect);

		CSize logoSize = m_wndView.GetLogoSize();
		logoSize.cx = std::max(logoSize.cx, (LONG)DEFCLIENTW);
		logoSize.cy = std::max(logoSize.cy, (LONG)DEFCLIENTH);

		windowSize.cx = windowRect.Width() - clientRect.Width() + logoSize.cx;
		windowSize.cy = windowRect.Height() - clientRect.Height() + logoSize.cy;

		CSize cSize;
		CalcControlsSize(cSize);

		windowSize.cx += cSize.cx;
		windowSize.cy += cSize.cy;
	}

	bool bRestoredWindowPosition = false;
	if (s.bRememberWindowPos) {
		CRect windowRect(ptLastWindowPos, windowSize);
		if (CMonitors::IsOnScreen(windowRect)) {
			ClampWindowRect(windowRect);
			MoveWindow(windowRect);
			bRestoredWindowPosition = true;
		}
	}

	if (!bRestoredWindowPosition) {
		CMonitors monitors;
		CMonitor monitor;
		if (iMonitor > 0 && iMonitor <= monitors.GetCount()) {
			monitor = monitors.GetMonitor(--iMonitor);
		} else {
			monitor = CMonitors::GetNearestMonitor(this);
		}

		MINMAXINFO mmi;
		OnGetMinMaxInfo(&mmi);
		CRect windowRect(0, 0, std::max(windowSize.cx, mmi.ptMinTrackSize.x), std::max(windowSize.cy, mmi.ptMinTrackSize.y));
		ClampWindowRect(windowRect);

		monitor.CenterRectToMonitor(windowRect, TRUE, GetInvisibleBorderSize());
		SetWindowPos(nullptr, windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}

	if (bLastCall && s.nStartupWindowMode == STARTUPWND_REMLAST && s.bRememberWindowPos && !(s.nCLSwitches & CLSW_MINIMIZED)) {
		if (nLastWindowType == SIZE_MAXIMIZED) {
			ShowWindow(SW_MAXIMIZE);
		} else if (nLastWindowType == SIZE_MINIMIZED) {
			ShowWindow(SW_MINIMIZE);
		}
	}

	if (s.bSavePnSZoom) {
		m_ZoomX = s.dZoomX;
		m_ZoomY = s.dZoomY;
	}
}

void CMainFrame::SetDefaultFullscreenState()
{
	CAppSettings& s = AfxGetAppSettings();

	// Waffs : fullscreen command line
	if (!(s.nCLSwitches & CLSW_ADD) && (s.nCLSwitches & CLSW_FULLSCREEN) && !s.slFiles.empty()) {
		if (s.ExclusiveFSAllowed()) {
			m_bStartInD3DFullscreen = true;
		} else {
			ToggleFullscreen(true, true);
			m_bFirstFSAfterLaunchOnFullScreen = true;
		}

		SetCursor(nullptr);
		s.nCLSwitches &= ~CLSW_FULLSCREEN;
	}
}

void CMainFrame::RestoreDefaultWindowRect()
{
	const CAppSettings& s = AfxGetAppSettings();

	if (!m_bFullScreen && !IsZoomed() && !IsIconic() && s.nStartupWindowMode != STARTUPWND_REMLAST) {
		CSize windowSize;

		if (s.HasFixedWindowSize()) {
			windowSize = s.sizeFixedWindow;
		}
		else if (s.nStartupWindowMode == STARTUPWND_REMLAST) { // hmmm
			windowSize = s.szLastWindowSize;
		}
		else if (s.nStartupWindowMode == STARTUPWND_SPECIFIED) {
			windowSize = s.szSpecifiedWndSize;
		}
		else {
			CRect windowRect;
			GetWindowRect(&windowRect);
			CRect clientRect;
			GetClientRect(&clientRect);

			CSize logoSize = m_wndView.GetLogoSize();
			logoSize.cx = std::max(logoSize.cx, (LONG)DEFCLIENTW);
			logoSize.cy = std::max(logoSize.cy, (LONG)DEFCLIENTH);

			windowSize.cx = windowRect.Width() - clientRect.Width() + logoSize.cx;
			windowSize.cy = windowRect.Height() - clientRect.Height() + logoSize.cy;

			CSize cSize;
			CalcControlsSize(cSize);

			windowSize.cx += cSize.cx;
			windowSize.cy += cSize.cy;
		}

		if (s.bRememberWindowPos) {
			MoveWindow(CRect(s.ptLastWindowPos, windowSize));
		} else {
			SetWindowPos(nullptr, 0, 0, windowSize.cx, windowSize.cy, SWP_NOMOVE | SWP_NOZORDER);
			CenterWindow();
		}
	}
}

OAFilterState CMainFrame::GetMediaState()
{
	OAFilterState ret = -1;
	if (m_eMediaLoadState == MLS_LOADED) {
		m_pMC->GetState(0, &ret);
	}
	return ret;
}

void CMainFrame::SetPlaybackMode(PMODE eNewStatus)
{
	m_ePlaybackMode = eNewStatus;
	if (m_wndNavigationBar.IsWindowVisible() && GetPlaybackMode() != PM_CAPTURE) {
		ShowControlBarInternal(&m_wndNavigationBar, !m_wndNavigationBar.IsWindowVisible());
	}
}

CSize CMainFrame::GetVideoSize()
{
	auto& s = AfxGetAppSettings();
	const auto& fKeepAspectRatio = s.fKeepAspectRatio;
	const auto& fCompMonDeskARDiff = s.fCompMonDeskARDiff;

	CSize ret(0,0);
	if (m_eMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return ret;
	}

	CSize wh(0, 0), arxy(0, 0);

	if (m_pCAP) {
		wh = m_pCAP->GetVideoSize();
		arxy = fKeepAspectRatio ? m_pCAP->GetVideoSizeAR() : wh;
	} else if (m_pMFVDC) {
		m_pMFVDC->GetNativeVideoSize(&wh, &arxy); // TODO : check AR !!
	} else {
		m_pBV->GetVideoSize(&wh.cx, &wh.cy);

		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = m_pBV.p;
		// FIXME: It can hang here, for few seconds (CPU goes to 100%), after the window have been moving over to another screen,
		// due to GetPreferredAspectRatio, if it happens before CAudioSwitcherFilter::DeliverEndFlush, it seems.
		if (pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0) {
			arxy.SetSize(arx, ary);
		}
	}

	if (wh.cx <= 0 || wh.cy <= 0) {
		return ret;
	}

	if (GetPlaybackMode() == PM_DVD) {
		// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
		DVD_VideoAttributes VATR;
		if (SUCCEEDED(m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
			arxy.SetSize(VATR.ulAspectX, VATR.ulAspectY);
		}
	}

	CSize& ar = s.sizeAspectRatio;
	if (ar.cx && ar.cy) {
		arxy = ar;
	}

	ret = (!fKeepAspectRatio || arxy.cx <= 0 || arxy.cy <= 0)
		  ? wh
		  : CSize(MulDiv(wh.cy, arxy.cx, arxy.cy), wh.cy);

	if (fCompMonDeskARDiff)
		if (HDC hDC = ::GetDC(0)) {
			int _HORZSIZE = GetDeviceCaps(hDC, HORZSIZE);
			int _VERTSIZE = GetDeviceCaps(hDC, VERTSIZE);
			int _HORZRES = GetDeviceCaps(hDC, HORZRES);
			int _VERTRES = GetDeviceCaps(hDC, VERTRES);

			if (_HORZSIZE > 0 && _VERTSIZE > 0 && _HORZRES > 0 && _VERTRES > 0) {
				double a = 1.0*_HORZSIZE/_VERTSIZE;
				double b = 1.0*_HORZRES/_VERTRES;

				if (b < a) {
					ret.cy = (DWORD)(1.0*ret.cy * a / b);
				} else if (a < b) {
					ret.cx = (DWORD)(1.0*ret.cx * b / a);
				}
			}

			::ReleaseDC(0, hDC);
		}

	return ret;
}

void CMainFrame::ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo)
{
	if (IsD3DFullScreenMode()) {
		return;
	}

	m_bFullScreenChangingMode = true;

	CAppSettings& s = AfxGetAppSettings();
	CRect r;
	DWORD dwRemove = 0, dwAdd = 0;
	MONITORINFO mi = { sizeof(mi) };

	HMONITOR hm		= MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	HMONITOR hm_cur = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

	CMonitors monitors;

	const CWnd* pInsertAfter = nullptr;

	if (!m_bFullScreen) {
		if (s.bHidePlaylistFullScreen && m_wndPlaylistBar.IsVisible()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(true);
			ShowControlBarInternal(&m_wndPlaylistBar, FALSE);
		}

		if (!m_bFirstFSAfterLaunchOnFullScreen) {
			GetWindowRect(&m_lastWindowRect);
		}
		if (s.fullScreenModes.bEnabled == 1 && fSwitchScreenResWhenHasTo && (GetPlaybackMode() != PM_NONE)) {
			AutoChangeMonitorMode();
		}
		m_LastWindow_HM = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

		CString str;
		CMonitor monitor;
		if (s.strFullScreenMonitor == L"Current") {
			hm = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		} else {
			for ( int i = 0; i < monitors.GetCount(); i++ ) {
				monitor = monitors.GetMonitor( i );
				monitor.GetName(str);
				if ((monitor.IsMonitor()) && (s.strFullScreenMonitor == str)) {
					hm = monitor;
					break;
				}
			}
			if (!hm) {
				hm = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			}
		}

		dwRemove = WS_CAPTION | WS_THICKFRAME;
		GetMonitorInfoW(hm, &mi);
		if (fToNearest) {
			r = mi.rcMonitor;
		} else {
			GetDesktopWindow()->GetWindowRect(&r);
		}

		pInsertAfter = &wndTopMost;

		SetMenuBarVisibility(AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
	} else {
		if (s.fullScreenModes.bEnabled == 1 && s.fullScreenModes.bApplyDefault) {
			CString strFullScreenMonitor = s.strFullScreenMonitor;
			CString strFullScreenMonitorID = s.strFullScreenMonitorID;
			if (strFullScreenMonitor == L"Current") {
				auto curmonitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
				curmonitor.GetName(strFullScreenMonitor);
				curmonitor.GetDeviceId(strFullScreenMonitorID);
			}

			auto it = std::find_if(s.fullScreenModes.res.cbegin(), s.fullScreenModes.res.cend(), [&strFullScreenMonitorID](const fullScreenRes& item) {
				return item.monitorId == strFullScreenMonitorID;
			});

			if (it != s.fullScreenModes.res.cend() && it->dmFullscreenRes[0].bChecked) {
				SetDispMode(it->dmFullscreenRes[0].dmFSRes, strFullScreenMonitor);
			}
		}

		dwAdd = (s.iCaptionMenuMode == MODE_BORDERLESS ? 0 : s.iCaptionMenuMode == MODE_FRAMEONLY? WS_THICKFRAME: WS_CAPTION | WS_THICKFRAME);
		if (!m_bFirstFSAfterLaunchOnFullScreen) {
			r = m_lastWindowRect;
		}

		if (s.bHidePlaylistFullScreen && m_wndPlaylistBar.IsHiddenDueToFullscreen() && !m_bIsMPCVRExclusiveMode) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(false);
			ShowControlBarInternal(&m_wndPlaylistBar, TRUE);
		}

		pInsertAfter = &wndNoTopMost;
	}

	m_lastMouseMove.x = m_lastMouseMove.y = -1;

	bool fAudioOnly = m_bAudioOnly;
	m_bAudioOnly = true;

	m_bFullScreen = !m_bFullScreen;

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);

	static bool bChangeMonitor = false;
	// try disable shader when move from one monitor to other ...
	if (m_bFullScreen) {
		bChangeMonitor = (hm != hm_cur);
		if (bChangeMonitor && m_pCAP) {
			if (!m_bToggleShader) {
				m_pCAP->ClearPixelShaders(TARGET_FRAME);
			}

			if (m_bToggleShaderScreenSpace) {
				m_pCAP->ClearPixelShaders(TARGET_SCREEN);
			}
		}
	} else {
		if (bChangeMonitor && m_pCAP && m_bToggleShader) { // ???
			m_pCAP->ClearPixelShaders(TARGET_FRAME);
		}
	}

	if (m_bFullScreen) {
		m_bHideCursor = true;
		if (s.fShowBarsWhenFullScreen) {
			int nTimeOut = s.nShowBarsWhenFullScreenTimeOut;
			if (nTimeOut == 0) {
				ShowControls(CS_NONE, false);
				ShowControlBarInternal(&m_wndNavigationBar, FALSE);
			} else if (nTimeOut > 0) {
				SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut * 1000, nullptr);
				SetTimer(TIMER_MOUSEHIDER, std::max(nTimeOut * 1000, 2000), nullptr);
			}
		} else {
			ShowControls(CS_NONE, false);
			ShowControlBarInternal(&m_wndNavigationBar, FALSE);
		}

		if (s.fPreventMinimize) {
			if (hm != hm_cur) {
				ModifyStyle(WS_MINIMIZEBOX, 0, SWP_NOZORDER);
			}
		}
	} else {
		ModifyStyle(0, WS_MINIMIZEBOX, SWP_NOZORDER);
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		if (!(s.bHideWindowedMousePointer && m_eMediaLoadState == MLS_LOADED)) {
			StopAutoHideCursor();
		}
		ShowControls(s.nCS);
		if (GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1) {
			ShowControlBarInternal(&m_wndNavigationBar, !s.fHideNavigation);
		}
	}

	m_bAudioOnly = fAudioOnly;

	SetMenuBarState(AFX_MBS_HIDDEN);
	if (!m_bFullScreen) {
		SetMenuBarVisibility(s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU ?
							 AFX_MBV_KEEPVISIBLE : AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
	}

	const UINT nFlags = m_bFullScreen ? SWP_NOSENDCHANGING : SWP_NOSENDCHANGING | SWP_NOACTIVATE;

	if (m_bFirstFSAfterLaunchOnFullScreen) { // Play started in Fullscreen
		if (s.nStartupWindowMode == STARTUPWND_REMLAST || s.bRememberWindowPos) {
			r.SetRect(s.ptLastWindowPos, s.ptLastWindowPos + s.szLastWindowSize);
			if (!s.bRememberWindowPos) {
				hm = MonitorFromPoint( CPoint( 0,0 ), MONITOR_DEFAULTTOPRIMARY );
				GetMonitorInfoW(hm, &mi);
				CRect rMI = mi.rcMonitor;
				int left = rMI.left + (rMI.Width() - r.Width())/2;
				int top = rMI.top + (rMI.Height() - r.Height())/2;
				r = CRect(left, top, left + r.Width(), top + r.Height());
			}
			if (s.nStartupWindowMode != STARTUPWND_REMLAST) {
				CSize vsize = GetVideoSize();
				r = CRect(r.left, r.top, r.left + vsize.cx, r.top + vsize.cy);
				ShowWindow(SW_HIDE);
			}
			SetWindowPos(pInsertAfter, r.left, r.top, r.Width(), r.Height(), nFlags);
			if (s.nStartupWindowMode != STARTUPWND_REMLAST) {
				ZoomVideoWindow();
				ShowWindow(SW_SHOW);
			}
		} else {
			if (m_LastWindow_HM != hm_cur) {
				GetMonitorInfoW(m_LastWindow_HM, &mi);
				r = mi.rcMonitor;
				ShowWindow(SW_HIDE);
				SetWindowPos(pInsertAfter, r.left, r.top, r.Width(), r.Height(), nFlags);
			}

			if (m_eMediaLoadState == MLS_LOADED) {
				ZoomVideoWindow();
			} else {
				RestoreDefaultWindowRect();
			}

			if (m_LastWindow_HM != hm_cur) {
				ShowWindow(SW_SHOW);
			}
		}
		m_bFirstFSAfterLaunchOnFullScreen = false;
	} else {
		SetWindowPos(pInsertAfter, r.left, r.top, r.Width(), r.Height(), nFlags);
	}

	SetAlwaysOnTop(s.iOnTop);

	m_bFullScreenChangingMode = false;
	MoveVideoWindow();

	if (bChangeMonitor && (!m_bToggleShader || !m_bToggleShaderScreenSpace)) { // Enabled shader ...
		SetShaders();
	}

	UpdateThumbarButton();
	UpdateThumbnailClip();
}

void CMainFrame::ToggleD3DFullscreen(bool fSwitchScreenResWhenHasTo)
{
	if (m_pD3DFS) {
		CAppSettings& s = AfxGetAppSettings();

		bool bIsFullscreen = false;
		m_pD3DFS->GetD3DFullscreen(&bIsFullscreen);

		m_OSD.Stop();

		if (bIsFullscreen) {
			// Turn off D3D Fullscreen
			m_pD3DFS->SetD3DFullscreen(false);

			// Assign the windowed video frame and pass it to the relevant classes.
			m_pVideoWnd = &m_wndView;
			if (m_pMFVDC) {
				m_pMFVDC->SetVideoWindow(m_pVideoWnd->m_hWnd);
			} else {
				m_pVW->put_Owner((OAHWND)m_pVideoWnd->m_hWnd);
			}

			if (s.fullScreenModes.bEnabled == 1 && s.fullScreenModes.bApplyDefault) {
				CString strFullScreenMonitor = s.strFullScreenMonitor;
				CString strFullScreenMonitorID = s.strFullScreenMonitorID;
				if (strFullScreenMonitor == L"Current") {
					auto curmonitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
					curmonitor.GetName(strFullScreenMonitor);
					curmonitor.GetDeviceId(strFullScreenMonitorID);
				}

				auto it = std::find_if(s.fullScreenModes.res.cbegin(), s.fullScreenModes.res.cend(), [&strFullScreenMonitorID](const fullScreenRes& item) {
					return item.monitorId == strFullScreenMonitorID;
				});

				if (it != s.fullScreenModes.res.cend() && it->dmFullscreenRes[0].bChecked) {
					SetDispMode(it->dmFullscreenRes[0].dmFSRes, strFullScreenMonitor, TRUE);
				}
			}

			if (s.ShowOSD.Enable) {
				m_OSD.Start(m_pOSDWnd);
			}

			// Destroy the D3D Fullscreen window
			DestroyD3DWindow();

			SetAlwaysOnTop(s.iOnTop);

			MoveVideoWindow();
			RecalcLayout();

			StartAutoHideCursor();
		} else {
			// Set the fullscreen display mode
			if (s.fullScreenModes.bEnabled == 1 && fSwitchScreenResWhenHasTo) {
				AutoChangeMonitorMode();
			}

			// Create a new D3D Fullscreen window
			CreateFullScreenWindow();

			SetAlwaysOnTop(s.iOnTop);

			// Turn on D3D Fullscreen
			m_pD3DFS->SetD3DFullscreen(true);

			// Assign the windowed video frame and pass it to the relevant classes.
			m_pVideoWnd = m_pFullscreenWnd;
			if (m_pMFVDC) {
				m_pMFVDC->SetVideoWindow(m_pVideoWnd->m_hWnd);
			} else {
				m_pVW->put_Owner((OAHWND)m_pVideoWnd->m_hWnd);
			}

			if (s.ShowOSD.Enable || s.bShowDebugInfo) {
				if (m_pMFVMB) {
					m_OSD.Start(m_pVideoWnd, m_pMFVMB);
				}
			}

			MoveVideoWindow();
		}

		SetFocus();
	}
}

void CMainFrame::AutoChangeMonitorMode()
{
	const CAppSettings& s = AfxGetAppSettings();
	CString strFullScreenMonitor = s.strFullScreenMonitor;
	CString strFullScreenMonitorID = s.strFullScreenMonitorID;
	BOOL bMonValid = FALSE;
	if (strFullScreenMonitor == L"Current") {
		auto curmonitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		curmonitor.GetName(strFullScreenMonitor);
		curmonitor.GetDeviceId(strFullScreenMonitorID);
		bMonValid = !strFullScreenMonitorID.IsEmpty();
	} else {
		DISPLAY_DEVICEW dd = { sizeof(dd) };
		DWORD dev = 0; // device index
		while (EnumDisplayDevicesW(0, dev, &dd, 0) && !bMonValid) {
			DISPLAY_DEVICE ddMon = { sizeof(ddMon) };
			DWORD devMon = 0;
			while (EnumDisplayDevicesW(dd.DeviceName, devMon, &ddMon, 0)) {
				if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
					const CString DeviceID = RegExpParse(ddMon.DeviceID, LR"(MONITOR\\(\S*?)\\)");
					if (strFullScreenMonitorID == DeviceID) {
						strFullScreenMonitor = RegExpParse(ddMon.DeviceName, LR"((\\\\.\\DISPLAY\d+)\\)");
						bMonValid = TRUE;
						break;
					}
				}
				devMon++;
			}
			dev++;
		}
	}

	// Set Display Mode
	if (s.fullScreenModes.bEnabled && bMonValid == TRUE) {
		double dFPS = 0.0;
		if (m_dMediaInfoFPS > 0.9) {
			dFPS = m_dMediaInfoFPS;
		} else {
			const REFERENCE_TIME rtAvgTimePerFrame = std::llround(GetAvgTimePerFrame(FALSE) * 10000000i64);
			if (rtAvgTimePerFrame > 0) {
				dFPS = 10000000.0 / rtAvgTimePerFrame;

				if (m_clsidCAP == CLSID_MPCVRAllocatorPresenter) {
					if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pCAP.p) {
						bool bDoubleRate = false;
						if (S_OK == pIExFilterConfig->Flt_GetBool("doubleRate", &bDoubleRate) && bDoubleRate) {
							dFPS *= 2.0;
						}
					}
				} else if (g_nFrameType == PICT_BOTTOM_FIELD || g_nFrameType == PICT_TOP_FIELD) {
					dFPS *= 2.0;
				}
			}
		}

		if (dFPS == 0.0) {
			return;
		}

		auto it = std::find_if(s.fullScreenModes.res.cbegin(), s.fullScreenModes.res.cend(), [&strFullScreenMonitorID](const fullScreenRes& item) {
			return item.monitorId == strFullScreenMonitorID;
		});

		if (it != s.fullScreenModes.res.cend()) {
			for (const auto& fsmode : it->dmFullscreenRes) {
				if (fsmode.bValid
						&& fsmode.bChecked
						&& dFPS >= fsmode.vfr_from
						&& dFPS <= fsmode.vfr_to) {

					SetDispMode(fsmode.dmFSRes, strFullScreenMonitor, CanSwitchD3DFS());
					return;
				}
			}
		}
	}
}

bool CMainFrame::GetCurDispMode(dispmode& dm, const CString& DisplayName)
{
	return GetDispMode(ENUM_CURRENT_SETTINGS, dm, DisplayName);
}

bool CMainFrame::GetDispMode(const DWORD iModeNum, dispmode& dm, const CString& DisplayName)
{
	DEVMODEW devmode;
	devmode.dmSize = sizeof(DEVMODEW);
	CString GetDisplayName = DisplayName;
	if (GetDisplayName == L"Current" || GetDisplayName.IsEmpty()) {
		CMonitor monitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		monitor.GetName(GetDisplayName);
	}

	dm.bValid = !!EnumDisplaySettingsW(GetDisplayName, iModeNum, &devmode);

	if (dm.bValid) {
		dm.size = CSize(devmode.dmPelsWidth, devmode.dmPelsHeight);
		dm.bpp = devmode.dmBitsPerPel;
		dm.freq = devmode.dmDisplayFrequency;
		dm.dmDisplayFlags = devmode.dmDisplayFlags;
	}

	return dm.bValid;
}

void CMainFrame::SetDispMode(const dispmode& dm, const CString& DisplayName, const BOOL bForceRegistryMode/* = FALSE*/)
{
	const CAppSettings& s = AfxGetAppSettings();

	dispmode currentdm;
	if (!s.fullScreenModes.bEnabled
			|| !GetCurDispMode(currentdm, DisplayName)
			|| ((dm.size == currentdm.size) && (dm.bpp == currentdm.bpp) && (dm.freq == currentdm.freq))) {
		return;
	}

	CString ChangeDisplayName = DisplayName;
	if (DisplayName == L"Current" || DisplayName.IsEmpty()) {
		CMonitor monitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		monitor.GetName(ChangeDisplayName);
	}

	BOOL bPausedForAutochangeMonitorMode = FALSE;
	if (m_eMediaLoadState == MLS_LOADED && GetMediaState() == State_Running && s.iDMChangeDelay) {
		// pause if the mode is being changed during playback
		OnPlayPause();
		bPausedForAutochangeMonitorMode = TRUE;
	}

	DEVMODEW dmScreenSettings = { 0 };
	dmScreenSettings.dmSize             = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth        = dm.size.cx;
	dmScreenSettings.dmPelsHeight       = dm.size.cy;
	dmScreenSettings.dmBitsPerPel       = dm.bpp;
	dmScreenSettings.dmDisplayFrequency = dm.freq;
	dmScreenSettings.dmDisplayFlags     = dm.dmDisplayFlags;
	dmScreenSettings.dmFields           = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

	if (s.fRestoreResAfterExit && !bForceRegistryMode) {
		ChangeDisplaySettingsExW(ChangeDisplayName, &dmScreenSettings, nullptr, CDS_FULLSCREEN, nullptr);
	} else {
		LONG ret = ChangeDisplaySettingsExW(ChangeDisplayName, &dmScreenSettings, nullptr, CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);
		if (ret == DISP_CHANGE_SUCCESSFUL) {
			ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
		}
	}

	if (bPausedForAutochangeMonitorMode) {
		SetTimer(TIMER_DM_AUTOCHANGING, s.iDMChangeDelay * 1000, nullptr);
	}
}

void CMainFrame::MoveVideoWindow(bool bShowStats/* = false*/, bool bForcedSetVideoRect/* = false*/)
{
	if (!m_bDelaySetOutputRect && !m_bFullScreenChangingMode && m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly && IsWindowVisible()) {
		CRect rWnd;
		CRect rVid;

		if (IsD3DFullScreenMode()) {
			m_pFullscreenWnd->GetClientRect(&rWnd);
		} else if (m_bFullScreen) {
			GetWindowRect(&rWnd);
			CRect r;
			m_wndView.GetWindowRect(&r);
			rWnd -= r.TopLeft();
		} else {
			m_wndView.GetClientRect(&rWnd);
		}

		const int wnd_w = rWnd.Width();
		const int wnd_h = rWnd.Height();

		OAFilterState fs = GetMediaState();
		if (fs != State_Stopped || bForcedSetVideoRect || (fs == State_Stopped && m_bShockwaveGraph)) {
			const CSize szVideo = GetVideoSize();

			double w = wnd_w;
			double h = wnd_h;
			const long wy = wnd_w * szVideo.cy;
			const long hx = wnd_h * szVideo.cx;

			switch (m_iVideoSize) {
			case DVS_HALF:
				w = szVideo.cx / 2;
				h = szVideo.cy / 2;
				break;
			case DVS_NORMAL:
				w = szVideo.cx;
				h = szVideo.cy;
				break;
			case DVS_DOUBLE:
				w = szVideo.cx * 2;
				h = szVideo.cy * 2;
				break;
			case DVS_FROMINSIDE:
				if (!m_bShockwaveGraph) {
					if (wy > hx) {
						w = (double)hx / szVideo.cy;
					} else {
						h = (double)wy / szVideo.cx;
					}

					if (m_bFullScreen || IsD3DFullScreenMode()) {
						const CAppSettings& s = AfxGetAppSettings();
						const double factor = (wy > hx) ? w / szVideo.cx : h / szVideo.cy;

						if (s.bNoSmallUpscale && factor > 1.0 && factor < 1.02
								|| s.bNoSmallDownscale && factor > 0.937 && factor < 1.0) {
							w = szVideo.cx;
							h = szVideo.cy;
						}
					}
				}
				break;
			case DVS_FROMOUTSIDE:
				if (!m_bShockwaveGraph) {
					if (wy < hx) {
						w = (double)hx / szVideo.cy;
					} else {
						h = (double)wy / szVideo.cx;
					}
				}
				break;
			case DVS_ZOOM1:
				if (!m_bShockwaveGraph) {
					if (wy > hx) {
						w = ((double)hx + (wy - hx) * 0.333) / szVideo.cy;
						h = w * szVideo.cy / szVideo.cx;
					} else {
						h = ((double)wy + (hx - wy) * 0.333) / szVideo.cx;
						w = h * szVideo.cx / szVideo.cy;
					}
				}
				break;
			case DVS_ZOOM2:
				if (!m_bShockwaveGraph) {
					if (wy > hx) {
						w = ((double)hx + (wy - hx) * 0.667) / szVideo.cy;
						h = w * szVideo.cy / szVideo.cx;
					} else {
						h = ((double)wy + (hx - wy) * 0.667) / szVideo.cx;
						w = h * szVideo.cx / szVideo.cy;
					}
				}
				break;
			}

			const double shift2X = wnd_w * (m_PosX * 2 - 1);
			const double shift2Y = wnd_h * (m_PosY * 2 - 1);

			const double dLeft   = ((shift2X - w) * m_ZoomX + wnd_w) / 2;
			const double dTop    = ((shift2Y - h) * m_ZoomY + wnd_h) / 2;
			const double dRight  = ((shift2X + w) * m_ZoomX + wnd_w) / 2;
			const double dBottom = ((shift2Y + h) * m_ZoomY + wnd_h) / 2;

			rVid.left   = std::round(dLeft);
			rVid.top    = std::round(dTop);
			rVid.right  = std::round(dRight);
			rVid.bottom = std::round(dBottom);
		}

		if (m_pCAP) {
			m_pCAP->SetPosition(rWnd, rVid);
		} else {
			HRESULT hr;
			hr = m_pBV->SetDefaultSourcePosition();
			hr = m_pBV->SetDestinationPosition(rVid.left, rVid.top, rVid.Width(), rVid.Height());
			hr = m_pVW->SetWindowPosition(rWnd.left, rWnd.top, wnd_w, wnd_h);

			if (m_pMFVDC) {
				m_pMFVDC->SetVideoPosition(nullptr, rWnd);
			}
		}

		if (m_bOpening) {
			if (auto hdc = ::GetDC(m_wndView.GetSafeHwnd())) {
				m_wndView.SendMessageW(WM_ERASEBKGND, (WPARAM)hdc, 0);
			}
		}

		m_wndView.SetVideoRect(rWnd);

		if (bShowStats && rVid.Height() > 0) {
			CString info;
			info.Format(L"Pos %.3f %.3f, Zoom %.2f %.2f, AR %.2f",
				m_PosX, m_PosY, m_ZoomX, m_ZoomY, (double)rVid.Width() / rVid.Height());
			SendStatusMessage(info, 3000);
		}
	} else {
		m_wndView.SetVideoRect();
	}
}

void CMainFrame::SetPreviewVideoPosition()
{
	if (m_pGB_preview) {
		CPoint point;
		GetCursorPos(&point);
		m_wndSeekBar.ScreenToClient(&point);
		m_wndSeekBar.UpdateToolTipPosition(point);

		CRect wr;
		m_wndPreView.GetVideoRect(&wr);

		const CSize ws(wr.Size());
		int w = ws.cx;
		int h = ws.cy;

		const CSize arxy(GetVideoSize());
		{
			const int dh = ws.cy;
			const int dw = MulDiv(dh, arxy.cx, arxy.cy);

			if (arxy.cx >= arxy.cy) {
				int minw = dw;
				int maxw = dw;
				if (ws.cx < dw) {
					minw = ws.cx;
				} else if (ws.cx > dw) {
					maxw = ws.cx;
				}
				const float scale = 1 / 3.0f;
				w = minw + (maxw - minw) * scale;
				h = MulDiv(w, arxy.cy, arxy.cx);
			}
			else {
				int minh = dh;
				int maxh = dh;
				if (ws.cy < dh) {
					minh = ws.cy;
				} else if (ws.cy > dh) {
					maxh = ws.cy;
				}
				const float scale = 1 / 3.0f;
				h = minh + (maxh - minh) * scale;
				w = MulDiv(h, arxy.cx, arxy.cy);
			}
		}

		const CPoint pos(m_PosX * (wr.Width() * 3 - w) - wr.Width(), m_PosY * (wr.Height() * 3 - h) - wr.Height());
		const CRect vr(pos, CSize(w, h));

		if (m_pCAP_preview) {
			m_pCAP_preview->SetPosition(wr, vr);
		}
		else {
			if (m_pMFVDC_preview) {
				m_pMFVDC_preview->SetVideoPosition(nullptr, wr);
			}
			m_pBV_preview->SetDefaultSourcePosition();
			m_pBV_preview->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
			m_pVW_preview->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());
		}
	}
}

void CMainFrame::HideVideoWindow(bool fHide)
{
	CRect wr;
	if (IsD3DFullScreenMode()) {
		m_pFullscreenWnd->GetClientRect(&wr);
	} else if (m_bFullScreen) {
		GetWindowRect(&wr);
		CRect r;
		m_wndView.GetWindowRect(&r);
		wr -= r.TopLeft();
	} else {
		m_wndView.GetClientRect(&wr);
	}

	if (m_pCAP) {
		if (fHide) {
			m_pCAP->SetPosition(wr, RECT{0, 0, 0, 0});
		} else {
			m_pCAP->SetPosition(wr, wr);
		}
	}
}

void CMainFrame::ZoomVideoWindow(bool snap, double scale)
{
	if (m_eMediaLoadState != MLS_LOADED || IsD3DFullScreenMode()) {
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();

	if (m_bFullScreen) {
		OnViewFullscreen();
	}

	if (!s.HasFixedWindowSize()) {
		MINMAXINFO mmi = {};
		OnGetMinMaxInfo(&mmi);

		CRect r;
		GetWindowRect(r);

		CSize videoSize = GetVideoSize();
		if (m_bAudioOnly) {
			scale = 1.0;
			videoSize = m_wndView.GetLogoSize();
			videoSize.cx = std::max(videoSize.cx, (LONG)DEFCLIENTW);
			videoSize.cy = std::max(videoSize.cy, (LONG)DEFCLIENTH);
		} else {
			if (scale <= 0) {
				if (s.nPlaybackWindowMode == PLAYBACKWND_FITSCREEN || s.nPlaybackWindowMode == PLAYBACKWND_FITSCREENLARGER) {
					scale = GetZoomAutoFitScale();
				} else {
					scale = (double)s.nAutoScaleFactor / 100;
				}
			}
		}

		CSize videoTargetSize(int(videoSize.cx * scale + 0.5), int(videoSize.cy * scale + 0.5));

		CRect decorationsRect;
		VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), IsMainMenuVisible(), GetWindowExStyle(m_hWnd)));

		/*
		if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && !m_bFullScreen && !m_wndNavigationBar.IsVisible()) {
			CSize r = m_wndNavigationBar.CalcFixedLayout(FALSE, TRUE);
			videoTargetSize.cx += r.cx;
		}
		*/

		CSize controlsSize;
		CalcControlsSize(controlsSize);

		CRect workRect;
		CMonitors::GetNearestMonitor(this).GetWorkAreaRect(workRect);
		if (workRect.Width() && workRect.Height()) {
			if (SysVersion::IsWin10orLater()) {
				workRect.InflateRect(GetInvisibleBorderSize());
			}

			// don't go larger than the current monitor working area and prevent black bars in this case
			const CSize videoSpaceSize = workRect.Size() - controlsSize - decorationsRect.Size();

			// Do not adjust window size for video frame aspect ratio when video size is independent from window size
			const bool bAdjustWindowAR = !(m_iVideoSize == DVS_HALF || m_iVideoSize == DVS_NORMAL || m_iVideoSize == DVS_DOUBLE);
			const double videoAR = videoSize.cx / (double)videoSize.cy;

			if (videoTargetSize.cx > videoSpaceSize.cx) {
				videoTargetSize.cx = videoSpaceSize.cx;
				if (bAdjustWindowAR) {
					videoTargetSize.cy = std::lround(videoSpaceSize.cx / videoAR);
				}
			}

			if (videoTargetSize.cy > videoSpaceSize.cy) {
				videoTargetSize.cy = videoSpaceSize.cy;
				if (bAdjustWindowAR) {
					videoTargetSize.cx = std::lround(videoSpaceSize.cy * videoAR);
				}
			}
		} else {
			ASSERT(FALSE);
		}

		CSize finalSize = videoTargetSize + controlsSize + decorationsRect.Size();
		finalSize.cx = std::max(finalSize.cx, mmi.ptMinTrackSize.x);
		finalSize.cy = std::max(finalSize.cy, mmi.ptMinTrackSize.y);

		if (!s.bRememberWindowPos) {
			bool isSnapped = false;

			if (snap && s.bSnapToDesktopEdges && m_bWasSnapped) { // check if snapped to edges
				isSnapped = (r.left == workRect.left) || (r.top == workRect.top)
							|| (r.right == workRect.right) || (r.bottom == workRect.bottom);
			}

			if (isSnapped) { // prefer left, top snap to right, bottom snap
				if (r.left == workRect.left) {}
				else if (r.right == workRect.right) {
					r.left = r.right - finalSize.cx;
				}

				if (r.top == workRect.top) {}
				else if (r.bottom == workRect.bottom) {
					r.top = r.bottom - finalSize.cy;
				}
			} else { // center window
				r.left += r.Width()  / 2 - finalSize.cx / 2;
				r.top  += r.Height() / 2 - finalSize.cy / 2;
				m_bWasSnapped = false;
			}
		}

		r.right = r.left + finalSize.cx;
		r.bottom = r.top + finalSize.cy;

		if (r.right > workRect.right) {
			r.OffsetRect(workRect.right - r.right, 0);
		}
		if (r.left < workRect.left) {
			r.OffsetRect(workRect.left - r.left, 0);
		}
		if (r.bottom > workRect.bottom) {
			r.OffsetRect(0, workRect.bottom - r.bottom);
		}
		if (r.top < workRect.top) {
			r.OffsetRect(0, workRect.top - r.top);
		}

		WINDOWPLACEMENT wp = {};
		if (GetWindowPlacement(&wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
			wp.showCmd = SW_SHOWNOACTIVATE;
			wp.rcNormalPosition = r;
			SetWindowPlacement(&wp);
		} else {
			MoveWindow(r);
		}

		MoveVideoWindow();
	}
}

double CMainFrame::GetZoomAutoFitScale()
{
	if (m_eMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return 1.0;
	}

	const CAppSettings& s = AfxGetAppSettings();
	CSize arxy = GetVideoSize();

	double fitscale =
		s.nAutoFitFactor == 33 ? 1.0 / 3 :
		s.nAutoFitFactor == 67 ? 2.0 / 3 :
		(double)s.nAutoFitFactor / 100;

	fitscale *= std::min((double)m_rcDesktop.Width() / arxy.cx, (double)m_rcDesktop.Height() / arxy.cy);

	if (s.nPlaybackWindowMode == PLAYBACKWND_FITSCREENLARGER && fitscale > 1.0) {
		fitscale = 1.0;
	}

	return fitscale;
}

CRect CMainFrame::GetInvisibleBorderSize() const
{
	CRect invisibleBorders;
	if (SysVersion::IsWin10orLater()
			&& SUCCEEDED(DwmGetWindowAttribute(GetSafeHwnd(), DWMWA_EXTENDED_FRAME_BOUNDS, &invisibleBorders, sizeof(RECT)))) {
		CRect windowRect;
		GetWindowRect(&windowRect);

		invisibleBorders.TopLeft() = invisibleBorders.TopLeft() - windowRect.TopLeft();
		invisibleBorders.BottomRight() = windowRect.BottomRight() - invisibleBorders.BottomRight();
	}

	return invisibleBorders;
}

void CMainFrame::ClampWindowRect(RECT& windowRect)
{
	const auto monitor = CMonitors::GetNearestMonitor(&windowRect);
	CRect rcWork;
	monitor.GetWorkAreaRect(&rcWork);
	if (SysVersion::IsWin10orLater()) {
		rcWork.InflateRect(GetInvisibleBorderSize());
	}

	if (windowRect.right > rcWork.right) {
		windowRect.right = rcWork.right;
	}
	if (windowRect.bottom > rcWork.bottom) {
		windowRect.bottom = rcWork.bottom;
	}
};

void CMainFrame::RepaintVideo(const bool bForceRepaint/* = false*/)
{
	if (!m_bDelaySetOutputRect && (bForceRepaint || GetMediaState() != State_Running) || m_bDVDStillOn) {
		if (m_pCAP) {
			m_pCAP->Paint(false);
		} else if (m_pMFVDC) {
			m_pMFVDC->RepaintVideo();
		}
	}
}

ShaderC* CMainFrame::GetShader(LPCWSTR label, bool bD3D11)
{
	ShaderC* pShader = nullptr;

	for (auto& shader : m_ShaderCashe) {
		if (shader.Match(label, bD3D11)) {
			pShader = &shader;
			break;
		}
	}

	if (!pShader) {
		CString path;
		if (AfxGetMyApp()->GetAppSavePath(path)) {
			if (bD3D11) {
				path.AppendFormat(L"Shaders11\\%s.hlsl", label);
			} else {
				path.AppendFormat(L"Shaders\\%s.hlsl", label);
			}

			if (::PathFileExistsW(path)) {
				CStdioFile file;
				if (file.Open(path, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText)) {
					ShaderC shader;
					shader.label = label;

					CString str;
					file.ReadString(str); // read first string
					if (StartsWith(str, L"// $MinimumShaderProfile:")) {
						shader.profile = str.Mid(25).Trim(); // shader version property
					}
					else {
						file.SeekToBegin();
					}

					if (bD3D11) {
						shader.profile = L"ps_4_0";
					}
					else {
						shader.profile = L"ps_3_0";
					}

					while (file.ReadString(str)) {
						shader.srcdata += str + L"\n";
					}

					shader.length = file.GetLength();

					FILETIME ftCreate, ftAccess, ftWrite;
					if (GetFileTime(file.m_hFile, &ftCreate, &ftAccess, &ftWrite)) {
						shader.ftwrite = ftWrite;
					}

					file.Close();

					m_ShaderCashe.emplace_back(shader);
					pShader = &m_ShaderCashe.back();
				}
			}
		}
	}

	return pShader;
}

bool CMainFrame::SaveShaderFile(ShaderC* shader, bool bD3D11)
{
	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		if (bD3D11) {
			path.AppendFormat(L"Shaders11\\%s.hlsl", shader->label);
		} else {
			path.AppendFormat(L"Shaders\\%s.hlsl", shader->label);
		}

		CStdioFile file;
		if (file.Open(path, CFile::modeWrite  | CFile::shareExclusive | CFile::typeText)) {
			file.SetLength(0);

			CString str;
			str.Format(L"// $MinimumShaderProfile: %s\n", shader->profile);
			file.WriteString(str);

			file.WriteString(shader->srcdata);
			file.Close();

			// delete out-of-date data from the cache
			for (auto it = m_ShaderCashe.begin(), end = m_ShaderCashe.end(); it != end; ++it) {
				if (it->Match(shader->label, bD3D11)) {
					m_ShaderCashe.erase(it);
					break;
				}
			}

			return true;
		}
	}
	return false;
}

bool CMainFrame::DeleteShaderFile(LPCWSTR label, bool bD3D11)
{
	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		if (bD3D11) {
			path.AppendFormat(L"Shaders11\\%s.hlsl", label);
		} else {
			path.AppendFormat(L"Shaders\\%s.hlsl", label);
		}

		if (!::PathFileExistsW(path) || ::DeleteFileW(path)) {
			// if the file is missing or deleted successfully, then remove it from the cache
			for (auto it = m_ShaderCashe.begin(), end = m_ShaderCashe.end(); it != end; ++it) {
				if (it->Match(label, bD3D11)) {
					m_ShaderCashe.erase(it);
					return true;
				}
			}
		}
	}

	return false;
}

void CMainFrame::TidyShaderCashe()
{
	CString appsavepath;
	if (!AfxGetMyApp()->GetAppSavePath(appsavepath)) {
		return;
	}

	for (auto it = m_ShaderCashe.cbegin(); it != m_ShaderCashe.cend(); ) {
		CString path(appsavepath);
		if ((*it).profile == "ps_4_0") {
			path += L"Shaders11\\";
		} else {
			path += L"Shaders\\";
		}
		path += (*it).label + L".hlsl";

		CFile file;
		if (file.Open(path, CFile::modeRead | CFile::modeCreate | CFile::shareDenyNone)) {
			ULONGLONG length = file.GetLength();
			FILETIME ftCreate = {}, ftAccess = {}, ftWrite = {};
			GetFileTime(file.m_hFile, &ftCreate, &ftAccess, &ftWrite);

			file.Close();

			if ((*it).length == length && CompareFileTime(&(*it).ftwrite, &ftWrite) == 0) {
				++it;
				continue; // actual shader
			}
		}

		m_ShaderCashe.erase(it++); // outdated shader
	}
}

void CMainFrame::SetShaders()
{
	if (!m_pCAP) {
		return;
	}

	const int PShaderMode = m_pCAP->GetPixelShaderMode();
	if (PShaderMode != 9 && PShaderMode != 11) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();
	const bool bD3D11 = PShaderMode == 11;

	m_pCAP->ClearPixelShaders(TARGET_FRAME);
	m_pCAP->ClearPixelShaders(TARGET_SCREEN);

	auto AddPixelShader = [this, bD3D11](std::list<CString>& list, int target) {
		for (const auto& shader : list) {
			ShaderC* pShader = GetShader(shader, bD3D11);
			if (pShader) {
				CStringW label = pShader->label;
				CStringA profile(pShader->profile);
				CStringA srcdata(pShader->srcdata);

				HRESULT hr = m_pCAP->AddPixelShader(target, label, profile, srcdata);
				if (FAILED(hr)) {
					SendStatusMessage(ResStr(IDS_MAINFRM_73) + label, 3000);
				}
			}
		}
	};

	if (m_bToggleShader && !bD3D11) {
		AddPixelShader(s.ShaderList, TARGET_FRAME);
	}

	if (m_bToggleShaderScreenSpace) {
		if (bD3D11) {
			AddPixelShader(s.Shaders11PostScale, TARGET_SCREEN);
		} else {
			AddPixelShader(s.ShaderListScreenSpace, TARGET_SCREEN);
		}
	}
}

void CMainFrame::SetBalance(int balance)
{
	int sign = balance>0?-1:1; // -1: invert sign for more right channel
	int balance_dB;
	if (balance > -100 && balance < 100) {
		balance_dB = sign*(int)(100*20*log10(1-abs(balance)/100.0f));
	} else {
		balance_dB = sign*(-10000);    // -10000: only left, 10000: only right
	}

	if (m_eMediaLoadState == MLS_LOADED) {
		CString strBalance, strBalanceOSD;

		m_pBA->put_Balance(balance_dB);

		if (balance == 0) {
			strBalance = L"L = R";
		} else if (balance < 0) {
			strBalance.Format(L"L +%d%%", -balance);
		} else { //if (m_nBalance > 0)
			strBalance.Format(L"R +%d%%", balance);
		}

		strBalanceOSD.Format(ResStr(IDS_BALANCE_OSD), strBalance);
		m_OSD.DisplayMessage(OSD_TOPLEFT, strBalanceOSD);
	}
}

//
// Open/Close
//

CString CMainFrame::OpenCreateGraphObject(OpenMediaData* pOMD)
{
	ASSERT(m_pGB == nullptr);
	ASSERT(m_pGB_preview == nullptr);

	m_bCustomGraph = false;
	m_bShockwaveGraph = false;

	CAppSettings& s = AfxGetAppSettings();

	ReleasePreviewGraph(); // Hmm, most likely it is no longer needed

	bool bUseSmartSeek = s.fSmartSeek;
	if (!s.bSmartSeekOnline) {
		if (OpenFileData* pFileData = dynamic_cast<OpenFileData*>(pOMD)) {
			auto& fn = pFileData->fi.GetPath();
			if (!fn.IsEmpty()) {
				CUrlParser urlParser(fn.GetString());
				if (urlParser.IsValid()) {
					bUseSmartSeek = false;
				}
			}
		}
	}

	if (OpenFileData* pFileData = dynamic_cast<OpenFileData*>(pOMD)) {
		engine_t engine = s.GetFileEngine(pFileData->fi);

		if (engine == ShockWave) {
			HRESULT hr = E_FAIL;
			CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew DSObjects::CShockwaveGraph(m_pVideoWnd->m_hWnd, hr);
			if (!pUnk) {
				return ResStr(IDS_AG_OUT_OF_MEMORY);
			}

			if (SUCCEEDED(hr)) {
				m_pGB = CComQIPtr<IGraphBuilder>(pUnk);
			}
			if (FAILED(hr) || !m_pGB) {
				return ResStr(IDS_MAINFRM_77);
			}
			m_bShockwaveGraph = true;
		}

		m_bCustomGraph = m_bShockwaveGraph;

		if (!m_bCustomGraph) {
			auto pFGManager = DNew CFGManagerPlayer(L"CFGManagerPlayer", nullptr, m_pVideoWnd->m_hWnd);
			m_pGB = pFGManager;

			if (m_pGB) {
				pFGManager->SetUserAgent(s.strUserAgent);

				if (bUseSmartSeek) {
					// build graph for preview
					auto pFGManager_preview = DNew CFGManagerPlayer(L"CFGManagerPlayer", nullptr, m_wndPreView.GetVideoHWND(), s.iSmartSeekVR + 1);
					m_pGB_preview = pFGManager_preview;

					if (m_pGB_preview) {
						pFGManager_preview->SetUserAgent(s.strUserAgent);
					}
				}
			}
		}
	} else if (OpenDVDData* pDVDData = dynamic_cast<OpenDVDData*>(pOMD)) {
		m_pGB = DNew CFGManagerDVD(L"CFGManagerDVD", nullptr, m_pVideoWnd->m_hWnd);

		if (m_pGB && bUseSmartSeek) {
			m_pGB_preview = DNew CFGManagerDVD(L"CFGManagerDVD", nullptr, m_wndPreView.GetVideoHWND(), true);
		}
	} else if (OpenDeviceData* pDeviceData = dynamic_cast<OpenDeviceData*>(pOMD)) {
		if (s.iDefaultCaptureDevice == 1) {
			m_pGB = DNew CFGManagerBDA(L"CFGManagerBDA", nullptr, m_pVideoWnd->m_hWnd);
		} else {
			m_pGB = DNew CFGManagerCapture(L"CFGManagerCapture", nullptr, m_pVideoWnd->m_hWnd);
		}
	}

	if (!m_pGB) {
		return ResStr(IDS_MAINFRM_80);
	}

	m_pGB->AddToROT();

	m_pMC = m_pGB;
	m_pME = m_pGB;
	m_pMS = m_pGB; // general
	m_pVW = m_pGB;
	m_pBV = m_pGB; // video
	m_pBA = m_pGB; // audio
	m_pFS = m_pGB;

	if (m_pGB_preview) {
		m_pGB_preview->AddToROT();

		m_pMC_preview = m_pGB_preview;
		m_pMS_preview = m_pGB_preview; // general

		m_pVW_preview = m_pGB_preview;
		m_pBV_preview = m_pGB_preview;

		m_bWndPreViewOn = true;
	} else {
		m_bWndPreViewOn = false;
	}

	if (!(m_pMC && m_pME && m_pMS)
			|| !(m_pVW && m_pBV)
			|| !(m_pBA)) {
		return ResStr(IDS_GRAPH_INTERFACES_ERROR);
	}

	if (FAILED(m_pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0))) {
		return ResStr(IDS_GRAPH_TARGET_WND_ERROR);
	}

	m_pProv = (IUnknown*)DNew CKeyProvider();

	if (CComQIPtr<IObjectWithSite> pObjectWithSite = m_pGB.p) {
		pObjectWithSite->SetSite(m_pProv);
	}

	m_pCB = DNew CDSMChapterBag(nullptr, nullptr);

	return {};
}

void CMainFrame::ReleasePreviewGraph()
{
	if (m_pGB_preview) {
		m_pMFVDC_preview.Release();
		m_pCAP_preview.Release();

		m_pMC_preview.Release();
		m_pMS_preview.Release();
		m_pVW_preview.Release();
		m_pBV_preview.Release();

		m_pDVDC_preview.Release();
		m_pDVDI_preview.Release();

		m_pGB_preview->RemoveFromROT();
		m_pGB_preview.Release();
	}
}

HRESULT CMainFrame::PreviewWindowHide()
{
	HRESULT hr = S_OK;

	if (!m_pGB_preview) {
		return E_FAIL;
	}

	if (m_wndPreView.IsWindowVisible()) {
		// Disable animation
		ANIMATIONINFO AnimationInfo;
		AnimationInfo.cbSize = sizeof(ANIMATIONINFO);
		::SystemParametersInfoW(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
		int WindowAnimationType = AnimationInfo.iMinAnimate;
		AnimationInfo.iMinAnimate = 0;
		::SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

		m_wndPreView.ShowWindow(SW_HIDE);

		// Enable animation
		AnimationInfo.iMinAnimate = WindowAnimationType;
		::SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

		if (m_pGB_preview) {
			m_pMC_preview->Pause();
		}
	}

	return hr;
}

HRESULT CMainFrame::PreviewWindowShow(REFERENCE_TIME rtCur2)
{
	if (!CanPreviewUse()) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	rtCur2 = GetClosestKeyFrame(rtCur2);

	if (GetPlaybackMode() == PM_DVD && m_pDVDC_preview) {
		DVD_DOMAIN domain;
		DVD_PLAYBACK_LOCATION2 location;

		hr = m_pDVDI->GetCurrentDomain(&domain);
		if (SUCCEEDED(hr)) {
			hr = m_pDVDI->GetCurrentLocation(&location);
		}
		if (FAILED(hr)) {
			return hr;
		}

		DVD_DOMAIN domain2;
		DVD_PLAYBACK_LOCATION2 location2;
		hr = m_pDVDI_preview->GetCurrentDomain(&domain2);
		if (SUCCEEDED(hr)) {
			hr = m_pDVDI_preview->GetCurrentLocation(&location2);
		}

		if (FAILED(hr) || domain != domain2 || location.TitleNum != location2.TitleNum) {
			CComPtr<IDvdState> pStateData;
			hr = m_pDVDI->GetState(&pStateData);
			if (SUCCEEDED(hr)) {
				hr = m_pDVDC_preview->SetState(pStateData, DVD_CMD_FLAG_Flush, nullptr);
			}
		}

		if (SUCCEEDED(hr)) {
			const double fps = location.TimeCodeFlags == DVD_TC_FLAG_25fps ? 25.0
				: location.TimeCodeFlags == DVD_TC_FLAG_30fps ? 30.0
				: location.TimeCodeFlags == DVD_TC_FLAG_DropFrame ? 29.97
				: 25.0;
			DVD_HMSF_TIMECODE dvdTo = RT2HMSF(rtCur2, fps);

			hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, nullptr);
		}

		if (SUCCEEDED(hr)) {
			m_pDVDC_preview->Pause(FALSE);
			m_pMC_preview->Run();
		}
	}
	else if (GetPlaybackMode() == PM_FILE && m_pMS_preview) {
		hr = m_pMS_preview->SetPositions(&rtCur2, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
	}

	if (FAILED(hr)) {
		return hr;
	}

	/*
	if (GetPlaybackMode() == PM_FILE) {
		hr = pFS2 ? pFS2->Step(2, nullptr) : E_FAIL;
		if (SUCCEEDED(hr)) {
			Sleep(10);
		}
	}
	*/

	if (!m_wndPreView.IsWindowVisible()) {
		m_wndPreView.ShowWindow(SW_SHOWNOACTIVATE);
	}

	return hr;
}

CString CMainFrame::OpenFile(OpenFileData* pOFD)
{
	if (!pOFD->fi.Valid()) {
		return ResStr(IDS_MAINFRM_81);
	}

	CAppSettings& s = AfxGetAppSettings();

	m_strPlaybackRenderedPath.Empty();

	CString youtubeUrl;
	CString youtubeErrorMessage;

	if (!m_youtubeUrllist.empty()) {
		youtubeUrl = pOFD->fi.GetPath();
		Content::Online::Disconnect(youtubeUrl);

		pOFD->fi.Clear();
		pOFD->auds.clear();
		pOFD->subs.clear();

		auto it = std::find_if(m_youtubeUrllist.cbegin(), m_youtubeUrllist.cend(), [&s](const Youtube::YoutubeUrllistItem& item) {
			return item.profile->iTag == s.iYoutubeTagSelected;
		});
		if (it != m_youtubeUrllist.cend()) {
			pOFD->fi = it->url;

			if (it->profile->type == Youtube::y_video && !m_youtubeAudioUrllist.empty()) {
				const auto audio_item = Youtube::GetAudioUrl(it->profile, m_youtubeAudioUrllist);
				pOFD->auds.emplace_back(audio_item->url);
			}

			pOFD->subs = m_lastOMD->subs;

			if (it->profile->type == Youtube::y_audio) {
				m_youtubeFields.fname.Format(L"%s.%s", m_youtubeFields.title, it->profile->ext);
			} else {
				m_youtubeFields.fname.Format(L"%s.%dp.%s", m_youtubeFields.title, it->profile->quality, it->profile->ext);
			}
			FixFilename(m_youtubeFields.fname);
		}

		m_strPlaybackRenderedPath = pOFD->fi.GetPath();
		m_wndPlaylistBar.SetCurLabel(m_youtubeFields.title);
	}
	else if (s.bYoutubePageParser && pOFD->auds.empty()) {
		auto url = pOFD->fi.GetPath();
		bool ok = Youtube::CheckURL(url);
		if (ok) {
			ok = Youtube::Parse_URL(
				url, pOFD->rtStart,
				m_youtubeFields,
				m_youtubeUrllist,
				m_youtubeAudioUrllist,
				pOFD,
				youtubeErrorMessage
			);
			if (ok && m_pGB->ShouldOperationContinue() == S_OK) {
				youtubeUrl = url;
				Content::Online::Disconnect(url);

				m_strPlaybackRenderedPath = pOFD->fi.GetPath();
				m_wndPlaylistBar.SetCurLabel(m_youtubeFields.title);
			}
		}
	}

	if (s.bYDLEnable
			&& m_pGB->ShouldOperationContinue() == S_OK
			&& youtubeUrl.IsEmpty()
			&& pOFD->auds.empty()
			&& ::PathIsURLW(pOFD->fi)) {

		auto& url = pOFD->fi.GetPath();
		const auto ext = GetFileExt(url).MakeLower();

		bool ok = (ext != L".m3u" && ext != L".m3u8");
		if (ok) {
			ok = Content::Online::CheckConnect(url);
		}

		if (ok) {
			CString online_hdr;
			Content::Online::GetHeader(url, online_hdr);
			if (!online_hdr.IsEmpty()) {
				online_hdr.Trim(L"\r\n "); online_hdr.Replace(L"\r", L"");
				std::list<CString> params;
				Explode(online_hdr, params, L'\n');
				bool bIsHtml = false;

				for (const auto& param : params) {
					int k = param.Find(L':');
					if (k > 0) {
						const CString key = param.Left(k).Trim().MakeLower();
						const CString value = param.Mid(k).MakeLower();
						if (key == L"content-type") {
							bIsHtml = (value.Find(L"text/html") != -1);
							break;
						}
					}
				}

				if (bIsHtml) {
					OpenFileData OFD;
					ok = YoutubeDL::Parse_URL(
						url,
						s.strYDLExePath,
						s.iYDLMaxHeight,
						s.bYDLMaximumQuality,
						CStringA(s.strYoutubeAudioLang),
						m_youtubeFields,
						m_youtubeUrllist,
						m_youtubeAudioUrllist,
						&OFD
					);
					if (ok && m_pGB->ShouldOperationContinue() == S_OK) {
						youtubeUrl = url;
						Content::Online::Disconnect(url);

						*pOFD = OFD;
						m_strPlaybackRenderedPath = pOFD->fi.GetPath();
						m_wndPlaylistBar.SetCurLabel(m_youtubeFields.title);
					}
				}
			}
		}
	}

	if (m_pGB->ShouldOperationContinue() != S_OK) {
		return ResStr(IDS_MAINFRM_82);
	}

	if (youtubeUrl.IsEmpty() && !youtubeErrorMessage.IsEmpty()) {
		return youtubeErrorMessage;
	}

	if (!::PathIsURLW(pOFD->fi)) {
		m_FileName = GetFileName(pOFD->fi);
	}
	else if (youtubeUrl.IsEmpty()) {
		m_bUpdateTitle = true;
	}

	auto AddCustomChapters = [&](const auto& chaplist) {
		CComQIPtr<IDSMChapterBag> pCB;

		BeginEnumFilters(m_pGB, pE, pBF) {
			if (CComQIPtr<IDSMChapterBag> pCB2 = pBF.p) {
				pCB = pBF;
				break;
			}
		}
		EndEnumFilters;

		if (!pCB) {
			ChaptersSouce* pCS = DNew ChaptersSouce;
			m_pGB->AddFilter(pCS, L"Chapters");
			pCB = pCS;
		}

		if (pCB) {
			pCB->ChapRemoveAll();

			for (const auto& chap : chaplist) {
				pCB->ChapAppend(chap.rt, chap.name);
			}
		}
	};

	{
		auto& fi = pOFD->fi;
		CString fn = fi.GetPath();
		fn.Trim();

		HRESULT hr = S_OK;

		if (!Content::Online::CheckConnect(fn)) {
			DLog(L"CMainFrame::OpenFile: Connection failed to %s", fn);
			hr = VFW_E_NOT_FOUND;
		}
		CString online_hdr;
		Content::Online::GetHeader(fn, online_hdr);
		if (online_hdr.Find(L"StreamBuffRe") == -1) {
			Content::Online::Disconnect(fn);
		}

		CorrectAceStream(fn);

		if (SUCCEEDED(hr)) {
			CStringW oldcurdir;
			BOOL bIsDirSet = FALSE;
			if (!::PathIsURLW(fn)) {
				oldcurdir = GetCurrentDir();
				if (oldcurdir.GetLength()) {
					bIsDirSet = ::SetCurrentDirectoryW(GetFolderPath(fn));
				}
			}

			hr = m_pGB->RenderFile(fn, nullptr);

			if (bIsDirSet) {
				::SetCurrentDirectoryW(oldcurdir);
			}
		}

		if (FAILED(hr)) {
			if (s.fReportFailedPins && !IsD3DFullScreenMode()) {
				CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB.p;
				if (pGBDE && pGBDE->GetCount()) {
					CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
				}
			}

			CString err;

			switch (hr) {
				case E_FAIL:
				case E_POINTER:
				default:                              err = ResStr(IDS_MAINFRM_83); break;
				case E_ABORT:                         err = ResStr(IDS_MAINFRM_82); break;
				case E_INVALIDARG:                    err = ResStr(IDS_MAINFRM_84); break;
				case E_OUTOFMEMORY:                   err = ResStr(IDS_AG_OUT_OF_MEMORY); break;
				case VFW_E_CANNOT_CONNECT:            err = ResStr(IDS_MAINFRM_86); break;
				case VFW_E_CANNOT_LOAD_SOURCE_FILTER: err = ResStr(IDS_MAINFRM_87); break;
				case VFW_E_CANNOT_RENDER:             err = ResStr(IDS_MAINFRM_88); break;
				case VFW_E_INVALID_FILE_FORMAT:       err = ResStr(IDS_MAINFRM_89); break;
				case VFW_E_NOT_FOUND:                 err = ResStr(IDS_MAINFRM_90); break;
				case VFW_E_UNKNOWN_FILE_TYPE:         err = ResStr(IDS_MAINFRM_91); break;
				case VFW_E_UNSUPPORTED_STREAM:        err = ResStr(IDS_MAINFRM_92); break;
			}

			return err;
		}

		bool bIsVideo = false;

		BeginEnumFilters(m_pGB, pEF, pBF)
		{
			if (!m_pMainFSF) {
				m_pMainFSF = pBF;
			}
			if (!m_pKFI) {
				m_pKFI = pBF;
			}

			if (!m_pAMMC[0]) {
				m_pAMMC[0] = pBF;
			}
			else if (!m_pAMMC[1]) {
				m_pAMMC[1] = pBF;
			}

			if (!bIsVideo && IsVideoRenderer(pBF)) {
				bIsVideo = true;
			}
		}
		EndEnumFilters;

		m_pMainSourceFilter = FindFilter(__uuidof(CMpegSplitterFilter), m_pGB);
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(__uuidof(CMpegSourceFilter), m_pGB);
		}
		m_bMainIsMPEGSplitter = (m_pMainSourceFilter != nullptr);

		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(CLSID_LAVSplitter, m_pGB);
		}
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(CLSID_LAVSource, m_pGB);
		}
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(CLSID_HaaliSplitterAR, m_pGB);
		}
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(CLSID_HaaliSplitter, m_pGB);
		}
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(L"{529A00DB-0C43-4f5b-8EF2-05004CBE0C6F}", m_pGB); // AV Splitter
		}
		if (!m_pMainSourceFilter) {
			m_pMainSourceFilter = FindFilter(L"{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}", m_pGB); // AV Source
		}

		if (fi.GetChapterCount()) {
			ChaptersList chaplist;
			fi.GetChapters(chaplist);

			AddCustomChapters(chaplist);
		}

		if (m_pGB_preview) {
			if (!bIsVideo || FAILED(m_pGB_preview->RenderFile(fn, nullptr))) {
				ReleasePreviewGraph();
			}
		}

		if (youtubeUrl.GetLength()) {
			fn = youtubeUrl;
		}
		if (m_strPlaybackRenderedPath.IsEmpty()) {
			m_strPlaybackRenderedPath = fn;
		}
		if (s.bKeepHistory && pOFD->bAddRecent && IsLikelyFilePath(fn)) {
			// there should not be a URL, otherwise explorer dirtied HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
			SHAddToRecentDocs(SHARD_PATHW, fn); // remember the last open files (system) through the drag-n-drop
		}
		pOFD->title = m_strPlaybackRenderedPath;
	}

	for (auto& aud : pOFD->auds) {
		if (m_bCustomGraph) {
			break;
		}

		CString fn = aud.GetPath();
		fn.Trim();
		if (fn.IsEmpty()) {
			break;
		}

		HRESULT hr = S_OK;

		if (!Content::Online::CheckConnect(fn)) {
			DLog(L"CMainFrame::OpenFile: Connection failed to %s", fn);
			hr = VFW_E_NOT_FOUND;
		}
		CString online_hdr;
		Content::Online::GetHeader(fn, online_hdr);
		if (online_hdr.Find(L"StreamBuffRe") == -1) {
			Content::Online::Disconnect(fn);
		}

		CorrectAceStream(fn);

		if (SUCCEEDED(hr)) {
			CStringW oldcurdir;
			BOOL bIsDirSet = FALSE;
			if (!::PathIsURLW(fn)) {
				oldcurdir = GetCurrentDir();
				if (oldcurdir.GetLength()) {
					bIsDirSet = ::SetCurrentDirectoryW(GetFolderPath(fn));
				}
			}

			if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB.p) {
				hr = pGBA->RenderAudioFile(fn);
			}

			if (bIsDirSet) {
				::SetCurrentDirectoryW(oldcurdir);
			}
		}
	}

	if (!m_youtubeFields.chaptersList.empty()) {
		AddCustomChapters(m_youtubeFields.chaptersList);
	}

	if (pOFD->fi.Valid()) {
		auto& fn = youtubeUrl.GetLength() ? youtubeUrl : pOFD->fi.GetPath();

		if (!StartsWith(fn, L"pipe:")) {
			const bool diskImage = m_DiskImage.GetDriveLetter() && m_SessionInfo.Path.GetLength();
			if (!diskImage) {
				m_SessionInfo.NewPath(fn);
			}

			if (m_youtubeFields.title.GetLength()) {
				m_SessionInfo.Title = m_youtubeFields.title;
			}
			else if (m_LastOpenBDPath.GetLength()) {
				CString fn2 = L"Blu-ray";
				if (m_BDLabel.GetLength()) {
					fn2.AppendFormat(L" \"%s\"", m_BDLabel);
				} else {
					MakeBDLabel(pOFD->fi, fn2);
				}
				m_SessionInfo.Title = fn2;
			}
			else if (m_pAMMC[0]) {
				for (const auto& pAMMC : m_pAMMC) {
					if (pAMMC) {
						CComBSTR bstr;
						if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
							m_SessionInfo.Title = bstr.m_str;
							break;
						}
					}
				}

				if (m_SessionInfo.Title.IsEmpty()) {
					CPlaylistItem pli;
					if (m_wndPlaylistBar.GetCur(pli) && pli.m_fi.Valid() && !pli.m_label.IsEmpty()) {
						m_SessionInfo.Title = pli.m_label;
					}
				}
			}

			if (s.bKeepHistory) {
				// read file position from history
				auto& historyFile = AfxGetMyApp()->m_HistoryFile;
				bool found = historyFile.OpenSessionInfo(m_SessionInfo, s.bRememberFilePos);

				if (found && s.bRememberFilePos) {
					// restore file position and track numbers
					if (m_pMS && m_SessionInfo.Position > 0) {
						REFERENCE_TIME rtDur;
						m_pMS->GetDuration(&rtDur);
						if (rtDur > 0) {
							REFERENCE_TIME rtPos = m_SessionInfo.Position;
							m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
						}
					}

					if (m_nAudioTrackStored == -1) {
						m_nAudioTrackStored = m_SessionInfo.AudioNum;
					}

					if (m_nSubtitleTrackStored == -1) {
						m_nSubtitleTrackStored = m_SessionInfo.SubtitleNum;
					}
				}
			}
		}
	}

	if (s.fReportFailedPins) {
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB.p;
		if (pGBDE && pGBDE->GetCount()) {
			CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
		}
	}

	if (!(m_pAMOP = m_pGB)) {
		BeginEnumFilters(m_pGB, pEF, pBF)
		if (m_pAMOP = pBF) {
			break;
		}
		EndEnumFilters;
	}

	SetPlaybackMode(PM_FILE);

	SetupChapters();
	LoadKeyFrames();

	m_wndSeekBar.Invalidate();

	return L"";
}

void CMainFrame::SetupChapters()
{
	if (!m_pCB) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		m_pCB->ChapRemoveAll();

		std::list<CComPtr<IBaseFilter>> pBFs;
		BeginEnumFilters(m_pGB, pEF, pBF)
			pBFs.emplace_back(pBF);
		EndEnumFilters;

		for (auto& pBF : pBFs) {
			if (m_pCB->ChapGetCount()) {
				break;
			}

			CComQIPtr<IDSMChapterBag> pCB = pBF.p;
			if (pCB) {
				for (DWORD i = 0, cnt = pCB->ChapGetCount(); i < cnt; i++) {
					REFERENCE_TIME rt;
					CComBSTR name;
					if (SUCCEEDED(pCB->ChapGet(i, &rt, &name))) {
						m_pCB->ChapAppend(rt, name);
					}
				}
			}
		}

		for (auto& pBF : pBFs) {
			if (m_pCB->ChapGetCount()) {
				break;
			}

			CComQIPtr<IChapterInfo> pCI = pBF.p;
			if (pCI) {
				CHAR iso6391[3];
				::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, iso6391, 3);
				CStringA iso6392(ISO6391To6392(iso6391));
				if (iso6392.GetLength() < 3) {
					iso6392 = "eng";
				}

				UINT cnt = pCI->GetChapterCount(CHAPTER_ROOT_ID);
				for (UINT i = 1; i <= cnt; i++) {
					UINT cid = pCI->GetChapterId(CHAPTER_ROOT_ID, i);

					ChapterElement ce;
					if (pCI->GetChapterInfo(cid, &ce)) {
						char pl[3] = { iso6392[0], iso6392[1], iso6392[2] };
						char cc[] = "  ";
						CComBSTR name;
						name.Attach(pCI->GetChapterStringInfo(cid, pl, cc));
						m_pCB->ChapAppend(ce.rtStart, name);
					}
				}
			}
		}

		for (auto& pBF : pBFs) {
			if (m_pCB->ChapGetCount()) {
				break;
			}

			CComQIPtr<IAMExtendedSeeking, &IID_IAMExtendedSeeking> pES = pBF.p;
			if (pES) {
				long MarkerCount = 0;
				if (SUCCEEDED(pES->get_MarkerCount(&MarkerCount))) {
					for (long i = 1; i <= MarkerCount; i++) {
						double MarkerTime = 0;
						if (SUCCEEDED(pES->GetMarkerTime(i, &MarkerTime))) {
							CStringW name;
							name.Format(L"Chapter %d", i);

							CComBSTR bstr;
							if (S_OK == pES->GetMarkerName(i, &bstr)) {
								name = bstr;
							}

							m_pCB->ChapAppend(REFERENCE_TIME(MarkerTime * 10000000), name);
						}
					}
				}
			}
		}
	}
	else if (GetPlaybackMode() == PM_DVD) {
		WCHAR buff[MAX_PATH] = {};
		ULONG len = 0;
		DVD_PLAYBACK_LOCATION2 loc = {};
		ULONG ulNumOfChapters = 0;

		HRESULT hr = m_pDVDI->GetDVDDirectory(buff, std::size(buff), &len);
		if (SUCCEEDED(hr)) {
			hr = m_pDVDI->GetCurrentLocation(&loc);
		}
		if (loc.TitleNum != m_chapterTitleNum) {
			m_pCB->ChapRemoveAll(); // remove chapters even if GetDVDDirectory and GetCurrentLocation fails

			if (SUCCEEDED(hr)) {
				hr = m_pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters);
			}
			if (SUCCEEDED(hr) && ulNumOfChapters > 1) {
				CStringW path;
				path.Format(L"%s\\VIDEO_TS.IFO", buff);

				ULONG VTSN, TTN;
				if (CIfoFile::GetTitleInfo(path, loc.TitleNum, VTSN, TTN)) {
					path.Format(L"%s\\VTS_%02lu_0.IFO", buff, VTSN);

					CIfoFile ifo;
					if (ifo.OpenIFO(path, TTN) && ulNumOfChapters >= ifo.GetChaptersCount()) {
						for (UINT i = 0; i < ifo.GetChaptersCount(); i++) {
							REFERENCE_TIME rt = ifo.GetChapterTime(i);
							CString str;
							str.Format(IDS_AG_CHAPTER, i + 1);
							m_pCB->ChapAppend(rt, str);
						}
					}
				}
			}
		}
	}

	m_pCB->ChapSort();
	m_wndSeekBar.SetChapterBag(m_pCB);
	m_OSD.SetChapterBag(m_pCB);
}

CString CMainFrame::OpenDVD(OpenDVDData* pODD)
{
	HRESULT hr = m_pGB->RenderFile(pODD->path, nullptr);

	CAppSettings& s = AfxGetAppSettings();

	if (s.fReportFailedPins) {
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB.p;
		if (pGBDE && pGBDE->GetCount()) {
			CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
		}
	}

	if (SUCCEEDED(hr) && m_pGB_preview) {
		if (FAILED(hr = m_pGB_preview->RenderFile(pODD->path, nullptr))) {
			ReleasePreviewGraph();
		}
	}

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if ((m_pDVDC = pBF) && (m_pDVDI = pBF)) {
			break;
		}
	}
	EndEnumFilters;

	if (m_pGB_preview) {
		BeginEnumFilters(m_pGB_preview, pEF, pBF) {
			if ((m_pDVDC_preview = pBF) && (m_pDVDI_preview = pBF)) {
				break;
			}
		}
		EndEnumFilters;
	}

	if (hr == E_INVALIDARG) {
		return ResStr(IDS_MAINFRM_93);
	} else if (hr == VFW_E_CANNOT_RENDER) {
		return ResStr(IDS_DVD_NAV_ALL_PINS_ERROR);
	} else if (hr == VFW_S_PARTIAL_RENDER) {
		return ResStr(IDS_DVD_NAV_SOME_PINS_ERROR);
	} else if (hr == E_NOINTERFACE || !m_pDVDC || !m_pDVDI) {
		return ResStr(IDS_DVD_INTERFACES_ERROR);
	} else if (hr == VFW_E_CANNOT_LOAD_SOURCE_FILTER) {
		return ResStr(IDS_MAINFRM_94);
	} else if (FAILED(hr)) {
		return ResStr(IDS_AG_FAILED);
	}

	WCHAR buff[MAX_PATH] = { 0 };
	ULONG len = 0;
	if (SUCCEEDED(hr = m_pDVDI->GetDVDDirectory(buff, std::size(buff), &len))) {
		pODD->title = CString(buff) + L"\\VIDEO_TS.IFO";
		if (pODD->bAddRecent) {
			AddRecent(pODD->title);
		}
	}

	// TODO: resetdvd
	m_pDVDC->SetOption(DVD_ResetOnStop, FALSE);
	m_pDVDC->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);

	if (m_pDVDC_preview) {
		m_pDVDC_preview->SetOption(DVD_ResetOnStop, FALSE);
		m_pDVDC_preview->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);
	}

	ULONGLONG ullDiscID = 0;
	hr = m_pDVDI->GetDiscID(nullptr, &ullDiscID);
	if (SUCCEEDED(hr)) {
		m_SessionInfo.DVDId = ullDiscID;

		const CStringW& path = pODD->title;

		if (EndsWithNoCase(path, L"\\VIDEO_TS\\VIDEO_TS.IFO")) {
			if (path.GetLength() == 24 && path[1] == L':') {
				const CString DVDLabel = GetDriveLabel(path[0]);
				if (DVDLabel.GetLength() > 0) {
					m_SessionInfo.Title = DVDLabel;
				}
			}
			else if (path.GetLength() > 24) {
				CStringW str = path.Left(path.GetLength() - 22);
				int k= str.ReverseFind(L'\\');
				if (k > 1) {
					m_SessionInfo.Title = str.Mid(k + 1);
				}
			}
		}

		if (s.bKeepHistory) {
			// read DVD-Video position from history
			auto& historyFile = AfxGetMyApp()->m_HistoryFile;
			bool found = historyFile.OpenSessionInfo(m_SessionInfo, s.bRememberDVDPos);

			if (found && !s.bRememberDVDPos) {
				m_SessionInfo.CleanPosition();
			}
		}
	}

	if (s.idMenuLang) {
		m_pDVDC->SelectDefaultMenuLanguage(s.idMenuLang);
	}
	if (s.idAudioLang) {
		m_pDVDC->SelectDefaultAudioLanguage(s.idAudioLang, DVD_AUD_EXT_NotSpecified);
	}
	if (s.idSubtitlesLang) {
		m_pDVDC->SelectDefaultSubpictureLanguage(s.idSubtitlesLang, DVD_SP_EXT_NotSpecified);
	}

	m_iDVDDomain = DVD_DOMAIN_Stop;

	SetPlaybackMode(PM_DVD);

	return L"";
}

HRESULT CMainFrame::OpenBDAGraph()
{
	HRESULT hr = m_pGB->RenderFile(L"", nullptr);
	if (!FAILED(hr)) {
		AddTextPassThruFilter();
		SetPlaybackMode(PM_CAPTURE);
	}
	return hr;
}

CString CMainFrame::OpenCapture(OpenDeviceData* pODD)
{
	m_wndCaptureBar.InitControls();

	CStringW vidfrname, audfrname;
	CComPtr<IBaseFilter> pVidCapTmp, pAudCapTmp;

	m_VidDispName = pODD->DisplayName[0];

	if (!m_VidDispName.IsEmpty()) {
		if (!CreateFilter(m_VidDispName, &pVidCapTmp, vidfrname)) {
			return ResStr(IDS_MAINFRM_96);
		}
	}

	m_AudDispName = pODD->DisplayName[1];

	if (!m_AudDispName.IsEmpty()) {
		if (!CreateFilter(m_AudDispName, &pAudCapTmp, audfrname)) {
			return ResStr(IDS_MAINFRM_96);
		}
	}

	if (!pVidCapTmp && !pAudCapTmp) {
		return ResStr(IDS_MAINFRM_98);
	}

	m_pCGB.Release();
	m_pVidCap.Release();
	m_pAudCap.Release();

	if (FAILED(m_pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2))) {
		return ResStr(IDS_MAINFRM_99);
	}

	HRESULT hr;

	m_pCGB->SetFiltergraph(m_pGB);

	if (pVidCapTmp) {
		if (FAILED(hr = m_pGB->AddFilter(pVidCapTmp, vidfrname))) {
			return ResStr(IDS_CAPTURE_ERROR_VID_FILTER);
		}

		m_pVidCap = pVidCapTmp;

		if (!pAudCapTmp) {
			if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCCap))
					&& FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCCap))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Interleaved, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCPrev))
					&& FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCPrev))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMASC))
					&& FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMASC))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap");
			} else {
				m_pAudCap = m_pVidCap;
			}
		} else {
			if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCCap))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVidCap, IID_IAMStreamConfig, (void **)&m_pAMVSCPrev))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}
		}

		if (FAILED(m_pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, nullptr, m_pVidCap, IID_IAMCrossbar, (void**)&m_pAMXBar))) {
			DLog(L"Warning: No IAMCrossbar interface was found");
		}

		if (FAILED(m_pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, nullptr, m_pVidCap, IID_IAMTVTuner, (void**)&m_pAMTuner))) {
			DLog(L"Warning: No IAMTVTuner interface was found");
		}

		if (m_pAMTuner) { // load saved channel
			CProfile& profile = AfxGetProfile();
			int country = 1;
			profile.ReadInt(IDS_R_CAPTURE, L"Country", country);
			m_pAMTuner->put_CountryCode(country);

			int vchannel = pODD->vchannel;
			if (vchannel < 0) {
				profile.ReadInt(IDS_R_CAPTURE, L"Channel", vchannel);
			}
			if (vchannel >= 0) {
				OAFilterState fs = State_Stopped;
				m_pMC->GetState(0, &fs);
				if (fs == State_Running) {
					m_pMC->Pause();
				}
				m_pAMTuner->put_Channel(vchannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
				if (fs == State_Running) {
					m_pMC->Run();
				}
			}
		}
	}

	if (pAudCapTmp) {
		if (FAILED(hr = m_pGB->AddFilter(pAudCapTmp, CStringW(audfrname)))) {
			return ResStr(IDS_CAPTURE_ERROR_AUD_FILTER);
		}

		m_pAudCap = pAudCapTmp;

		if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, m_pAudCap, IID_IAMStreamConfig, (void **)&m_pAMASC))
				&& FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, m_pAudCap, IID_IAMStreamConfig, (void **)&m_pAMASC))) {
			DLog(L"Warning: No IAMStreamConfig interface for vidcap");
		}

		/*
		std::vector<CComQIPtr<IAMAudioInputMixer>> pAMAIM;

		BeginEnumPins(m_pAudCap, pEP, pPin) {
			PIN_DIRECTION dir;
			if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_INPUT) {
				continue;
			}
			if (CComQIPtr<IAMAudioInputMixer> pAIM = pPin) {
				pAMAIM.emplace_back(pAIM);
			}
		}
		EndEnumPins;

		if (m_pAMASC) {
			m_wndCaptureBar.m_capdlg.SetupAudioControls(m_AudDispName, m_pAMASC, pAMAIM);
		}
		*/
	}

	if (!(m_pVidCap || m_pAudCap)) {
		return ResStr(IDS_MAINFRM_108);
	}

	pODD->title = ResStr(IDS_CAPTURE_LIVE);

	ClearPlaybackInfo();
	m_SessionInfo.Title = ResStr(IDS_CAPTURE_LIVE);

	SetPlaybackMode(PM_CAPTURE);

	return L"";
}

void CMainFrame::OpenCustomizeGraph()
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		return;
	}

	CleanGraph();

	CAppSettings& s = AfxGetAppSettings();
	if (GetPlaybackMode() == PM_FILE) {
		if (m_pCAP && s.IsISRAutoLoadEnabled()) {
			AddTextPassThruFilter();
		}
	}

	const CRenderersSettings& rs = s.m_VRSettings;
	if (rs.iVideoRenderer == VIDRNDT_SYNC && rs.ExtraSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
		HRESULT hr;
		m_pRefClock = DNew CSyncClockFilter(nullptr, &hr);
		CStringW name;
		name = L"SyncClock Filter";
		m_pGB->AddFilter(m_pRefClock, name);

		CComPtr<IReferenceClock> refClock;
		m_pRefClock->QueryInterface(IID_PPV_ARGS(&refClock));
		CComPtr<IMediaFilter> mediaFilter;
		m_pGB->QueryInterface(IID_PPV_ARGS(&mediaFilter));
		mediaFilter->SetSyncSource(refClock);
		mediaFilter.Release();
		refClock.Release();

		m_pRefClock->QueryInterface(IID_PPV_ARGS(&m_pSyncClock));

		CComQIPtr<ISyncClockAdviser> pAdviser = m_pCAP.p;
		if (pAdviser) {
			pAdviser->AdviseSyncClock(m_pSyncClock);
		}
	}

	if (GetPlaybackMode() == PM_DVD) {
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IDirectVobSub2> pDVS2 = pBF.p) {
				//pDVS2->AdviseSubClock(m_pSubClock = DNew CSubClock);
				//break;

				// TODO: test multiple dvobsub instances with one clock
				if (!m_pSubClock) {
					m_pSubClock = DNew CSubClock;
				}
				pDVS2->AdviseSubClock(m_pSubClock);
			}
		}
		EndEnumFilters;
	}

	CleanGraph();
}

void CMainFrame::OpenSetupVideo()
{
	m_bAudioOnly = true;

	if (m_pMFVDC) {// EVR
		m_bAudioOnly = false;
	} else if (m_pCAP) {
		CSize vs = m_pCAP->GetVideoSizeAR();
		m_bAudioOnly = (vs.cx <= 0 || vs.cy <= 0);
	} else {
		{
			long w = 0, h = 0;

			if (CComQIPtr<IBasicVideo> pBV = m_pGB.p) {
				pBV->GetVideoSize(&w, &h);
			}

			if (w > 0 && h > 0) {
				m_bAudioOnly = false;
			}
		}

		if (m_bAudioOnly) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				long w = 0, h = 0;

				if (CComQIPtr<IVideoWindow> pVW = pBF.p) {
					long lVisible;
					if (FAILED(pVW->get_Visible(&lVisible))) {
						continue;
					}

					pVW->get_Width(&w);
					pVW->get_Height(&h);
				}

				if (w > 0 && h > 0) {
					m_bAudioOnly = false;
					break;
				}
			}
			EndEnumFilters;
		}
	}

	if (m_bShockwaveGraph) {
		m_bAudioOnly = false;
	}

	if (m_pCAP) {
		m_pCAP->SetRotation(m_iDefRotation);
		SetShaders();
	}
	if (m_pCAP_preview) {
		m_pCAP_preview->SetRotation(m_iDefRotation);
	}

	{
		OAHWND Owner;
		m_pVW->get_Owner(&Owner);
		if ((OAHWND)m_pVideoWnd->m_hWnd != Owner) {
			m_pVW->put_Owner((OAHWND)m_pVideoWnd->m_hWnd);
		}

		m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		m_pVW->put_MessageDrain((OAHWND)m_hWnd);

		for (CWnd* pWnd = m_wndView.GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
			pWnd->EnableWindow(FALSE); // little trick to let WM_SETCURSOR thru
		}

		if (m_pVW_preview) {
			m_pVW_preview->put_Owner((OAHWND)m_wndPreView.GetVideoHWND());
			m_pVW_preview->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		}
	}

	// destroy invisible top-level d3dfs window if there is no video renderer
	if (IsD3DFullScreenMode() && m_bAudioOnly) {
		DestroyD3DWindow();
		m_bStartInD3DFullscreen = true;
	}
}

void CMainFrame::OpenSetupAudio()
{
	m_pBA->put_Volume(m_wndToolBar.Volume);

	// FIXME
	int balance = AfxGetAppSettings().nBalance;

	int sign = balance>0?-1:1; // -1: invert sign for more right channel
	if (balance > -100 && balance < 100) {
		balance = sign*(int)(100*20*log10(1-abs(balance)/100.0f));
	} else {
		balance = sign*(-10000);    // -10000: only left, 10000: only right
	}

	m_pBA->put_Balance(balance);
}
/*
void CMainFrame::OpenSetupToolBar()
{
//	m_wndToolBar.Volume = AfxGetAppSettings().nVolume;
//	SetBalance(AfxGetAppSettings().nBalance);
}
*/
void CMainFrame::OpenSetupCaptureBar()
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		if (m_pVidCap && m_pAMVSCCap) {
			CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = m_pVidCap.p;

			if (!m_pAMXBar && pVfwCD) {
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, m_pAMVSCCap, pVfwCD);
			} else {
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, m_pAMVSCCap, m_pAMXBar, m_pAMTuner);
			}
		}

		if (m_pAudCap && m_pAMASC) {
			std::vector<CComQIPtr<IAMAudioInputMixer>> pAMAIM;

			BeginEnumPins(m_pAudCap, pEP, pPin) {
				if (CComQIPtr<IAMAudioInputMixer> pAIM = pPin.p) {
					pAMAIM.emplace_back(pAIM);
				}
			}
			EndEnumPins;

			m_wndCaptureBar.m_capdlg.SetupAudioControls(m_AudDispName, m_pAMASC, pAMAIM);
		}
	}

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, false,
		m_wndCaptureBar.m_capdlg.m_fAudPreview, false);
}

void CMainFrame::OpenSetupInfoBar()
{
	if (GetPlaybackMode() == PM_FILE) {
		bool fEmpty = true;
		for (const auto& pAMMC : m_pAMMC) {
			if (pAMMC) {
				CComBSTR bstr;
				if (SUCCEEDED(pAMMC->get_Title(&bstr))) {
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_TITLE), bstr.m_str);
					if (bstr.Length()) {
						fEmpty = false;
					}
					bstr.Empty();
				}
				if (SUCCEEDED(pAMMC->get_AuthorName(&bstr))) {
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUTHOR), bstr.m_str);
					if (bstr.Length()) {
						fEmpty = false;
					}
					bstr.Empty();
				}
				if (SUCCEEDED(pAMMC->get_Copyright(&bstr))) {
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_COPYRIGHT), bstr.m_str);
					if (bstr.Length()) {
						fEmpty = false;
					}
					bstr.Empty();
				}
				if (SUCCEEDED(pAMMC->get_Rating(&bstr))) {
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_RATING), bstr.m_str);
					if (bstr.Length()) {
						fEmpty = false;
					}
					bstr.Empty();
				}
				if (SUCCEEDED(pAMMC->get_Description(&bstr))) {
					// show only the first description line
					CString str(bstr.m_str);
					int k = str.Find(L"\r\n");
					if (k > 0) {
						str.Truncate(k);
						str.Append(L" \x2026");
					}
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DESCRIPTION), str);
					if (bstr.Length()) {
						fEmpty = false;
					}
					bstr.Empty();
				}
				if (!fEmpty) {
					RecalcLayout();
					break;
				}
			}
		}

		if (!m_youtubeFields.title.IsEmpty()) {
			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_TITLE), m_youtubeFields.title);
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		CString info('-');
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_LOCATION), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_VIDEO), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUDIO), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), info);
		RecalcLayout();
	}
}

void CMainFrame::OpenSetupStatsBar()
{
	CString info('-');

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if (!m_pQP && (m_pQP = pBF)) {
			m_wndStatsBar.SetLine(ResStr(IDS_AG_FRAMERATE), info);
			m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_SYNC_OFFSET), info);
			m_wndStatsBar.SetLine(ResStr(IDS_AG_FRAMES), info);
			m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_JITTER), info);
			m_wndStatsBar.SetLine(ResStr(IDS_AG_BUFFERS), info);
			m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_BITRATE), info);
			RecalcLayout();
		}

		if (CComQIPtr<IBufferInfo> pBI = pBF.p) {
			m_wndStatsBar.SetLine(ResStr(IDS_AG_BUFFERS), info);
			m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_BITRATE), info); // FIXME: shouldn't be here
			RecalcLayout();
		}
	}
	EndEnumFilters;
}

void CMainFrame::OpenSetupStatusBar()
{
	m_wndStatusBar.ShowTimer(true);

	//

	if (!m_bCustomGraph) {
		UINT id = IDB_NOAUDIO;

		BeginEnumFilters(m_pGB, pEF, pBF) {
			CComQIPtr<IBasicAudio> pBA = pBF.p;
			if (!pBA) {
				continue;
			}

			BeginEnumPins(pBF, pEP, pPin) {
				if (S_OK == m_pGB->IsPinDirection(pPin, PINDIR_INPUT)
						&& S_OK == m_pGB->IsPinConnected(pPin)) {
					CMediaType mt;
					if (FAILED(pPin->ConnectionMediaType(&mt))) {
						continue;
					}

					if (mt.majortype == MEDIATYPE_Audio && mt.formattype == FORMAT_WaveFormatEx) {
						switch (((WAVEFORMATEX*)mt.pbFormat)->nChannels) {
							case 1:
								id = IDB_MONO;
								break;
							case 2:
							default:
								id = IDB_STEREO;
								break;
						}
						break;
					} else if (mt.majortype == MEDIATYPE_Midi) {
						id = 0;
						break;
					}
				}
			}
			EndEnumPins;

			if (id != IDB_NOAUDIO) {
				break;
			}
		}
		EndEnumFilters;

		m_wndStatusBar.SetStatusBitmap(id);
	}
}

void CMainFrame::UpdateWindowTitle()
{
	const int style = AfxGetAppSettings().iTitleBarTextStyle;
	CString title = GetTextForBar(style);

	m_Lcd.SetMediaTitle(title);

	if (style == TEXTBAR_EMPTY) {
		title = s_strPlayerTitle;
	} else {
		title.Append(L" - ");
		title.Append(s_strPlayerTitle);
	}

	CString curTitle;
	GetWindowTextW(curTitle);
	if (curTitle != title) {
		SetWindowTextW(title);
	}
}

BOOL CMainFrame::SelectMatchTrack(const std::vector<Stream>& Tracks, CString pattern, const BOOL bExtPrior, size_t& nIdx, bool bAddForcedAndDefault/* = true*/)
{
	CharLowerW(pattern.GetBuffer());
	pattern.Replace(L"[fc]", L"forced");
	pattern.Replace(L"[def]", L"default");
	if (bAddForcedAndDefault) {
		if (pattern.Find(L"forced") == -1) {
			pattern.Append(L" forced");
		}
		if (pattern.Find(L"default") == -1) {
			pattern.Append(L" default");
		}
	}
	pattern.Trim();

	auto random_string = [](const int length) {
		constexpr wchar_t characters[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		static std::mt19937 generator(std::random_device{}());
		static std::uniform_int_distribution<> distribution(0, std::size(characters) - 2);

		CString random_string; random_string.Preallocate(length);
		for (int i = 0; i < length; i++) {
			random_string.AppendChar(characters[distribution(generator)]);
		}

		return random_string;
	};

	std::map<CString, CString> replace_data;
	auto bFound = false;
	do {
		bFound = false;
		auto posStart = pattern.Find(L'"');
		if (posStart != -1) {
			auto posEnd = pattern.Find(L'"', posStart + 1);
			if (posEnd != -1) {
				const auto length = posEnd - posStart + 1;
				const auto randomStr = random_string(length);
				CString matchStr(pattern.GetString() + posStart, length);
				pattern.Replace(matchStr.GetString(), randomStr.GetString());

				matchStr.Replace(L"\"", L"");
				replace_data.emplace(randomStr, matchStr);
				bFound = true;
			}
		}
	} while (bFound);

	constexpr LPCWSTR pszTokens = L" ";

	nIdx = 0;
	int tPos = 0;
	CString lang = pattern.Tokenize(pszTokens, tPos);
	while (tPos != -1) {
		size_t iIndex = 0;
		for (const auto& track : Tracks) {
			if (bExtPrior && !track.Ext) {
				iIndex++;
				continue;
			}

			CString name(track.Name);
			CharLowerW(name.GetBuffer());

			std::list<CString> sl;
			Explode(lang, sl, L'+');

			size_t nLangMatch = 0;
			for (CString subPattern : sl) {
				if (!replace_data.empty()) {
					const auto it = replace_data.find(subPattern);
					if (it != replace_data.cend()) {
						subPattern = it->second;
					}
				}

				bool bSkip = false;
				if (subPattern[0] == '!') {
					subPattern.Delete(0, 1);
					bSkip = true;
				}

				if ((track.forced && subPattern == L"forced") || (track.def && subPattern == L"default")
						|| name.Find(subPattern) >= 0) {
					if (!bSkip) {
						nLangMatch++;
					}
				} else if (bSkip) {
					nLangMatch++;
				}
			}

			if (nLangMatch == sl.size()) {
				nIdx = iIndex;
				return TRUE;
			}

			iIndex++;
		}

		lang = pattern.Tokenize(pszTokens, tPos);
	}

	return FALSE;
}

void CMainFrame::OpenSetupAudioStream()
{
	if (m_eMediaLoadState != MLS_LOADING || GetPlaybackMode() != PM_FILE) {
		return;
	}

	if (m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack != -1) {
		if (m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack != 0) {
			SetAudioTrackIdx(m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack);
		}
		return;
	}

	if (m_nAudioTrackStored != -1) {
		if (m_nAudioTrackStored != 0) {
			SetAudioTrackIdx(m_nAudioTrackStored);
		}
		return;
	}

	std::list<CString> extAudioList;
	CPlaylistItem pli;
	if (m_wndPlaylistBar.GetCur(pli)) {
		for (const auto& fi : pli.m_auds) {
			extAudioList.emplace_back(fi.GetPath());
		}
	}

	CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
	DWORD cStreams = 0;
	if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
		Stream stream;
		std::vector<Stream> streams;
		for (DWORD i = 0; i < cStreams; i++) {
			DWORD dwFlags = 0;
			CComHeapPtr<WCHAR> pszName;
			if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, &pszName, nullptr, nullptr))) {
				return;
			}

			CString name(pszName);
			const bool bExternal = Contains(extAudioList, name);

			stream.Name  = name;
			stream.Index = i;
			stream.Ext   = bExternal;
			streams.emplace_back(stream);
		}

#ifdef DEBUG
		if (!streams.empty()) {
			DLog(L"Audio Track list :");
			for (size_t i = 0; i < streams.size(); i++) {
				DLog(L"    %s", streams[i].Name);
			}
		}
#endif

		const CAppSettings& s = AfxGetAppSettings();
		if (!s.fUseInternalSelectTrackLogic) {
			if (s.fPrioritizeExternalAudio && extAudioList.size() > 0) {
				for (size_t i = 0; i < streams.size(); i++) {
					if (streams[i].Ext) {
						Stream& stream = streams[i];
						pSS->Enable(stream.Index, AMSTREAMSELECTENABLE_ENABLE);
						return;
					}
				}
			}
		} else {
			CString pattern = s.strAudiosLanguageOrder;
			if (s.fPrioritizeExternalAudio && extAudioList.size() > 0) {
				size_t nIdx = 0;
				const BOOL bMatch = SelectMatchTrack(streams, pattern, TRUE, nIdx);

				Stream stream;
				if (bMatch) {
					stream = streams[nIdx];
				} else {
					for (size_t i = 0; i < streams.size(); i++) {
						if (streams[i].Ext) {
							stream = streams[i];
							break;
						}
					}
				}

				if (stream.Ext) {
					pSS->Enable(stream.Index, AMSTREAMSELECTENABLE_ENABLE);
				}

				return;
			}

			size_t nIdx = 0;
			const BOOL bMatch = SelectMatchTrack(streams, pattern, FALSE, nIdx);

			if (bMatch) {
				Stream& stream = streams[nIdx];
				pSS->Enable(stream.Index, AMSTREAMSELECTENABLE_ENABLE);
			}
		}
	}
}

void CMainFrame::SubFlags(CString strname, bool& forced, bool& def)
{
	strname.Remove(' ');
	if (strname.Right(16).MakeLower() == L"[default,forced]") {
		def		= true;
		forced	= true;
	} else if (strname.Right(9).MakeLower() == L"[default]") {
		def		= true;
		forced	= false;
	} else if (strname.Right(8).MakeLower() == L"[forced]") {
		def		= false;
		forced	= true;
	} else {
		def		= false;
		forced	= false;
	}
}

size_t CMainFrame::GetSubSelIdx()
{
	if (m_SubtitlesStreams.size()) {
		CAppSettings& s = AfxGetAppSettings();
		CString pattern = s.strSubtitlesLanguageOrder;

		const bool bSubtitlesOffIfPatternNotMatch = EndsWith(pattern, L"[off]");
		if (bSubtitlesOffIfPatternNotMatch) {
			pattern.Replace(L"[off]", L"");
		}

		if (s.fPrioritizeExternalSubtitles) { // try external sub ...
			size_t nIdx	= 0;
			BOOL bMatch = SelectMatchTrack(m_SubtitlesStreams, pattern, TRUE, nIdx, !bSubtitlesOffIfPatternNotMatch);

			if (bMatch) {
				return nIdx;
			} else {
				for (size_t iIndex = 0; iIndex < m_SubtitlesStreams.size(); iIndex++) {
					if (m_SubtitlesStreams[iIndex].Ext) {
						return iIndex;
					}
				}
			}
		}

		size_t nIdx = 0;
		BOOL bMatch = SelectMatchTrack(m_SubtitlesStreams, pattern, FALSE, nIdx, !bSubtitlesOffIfPatternNotMatch);

		if (bMatch) {
			return nIdx;
		}

		if (bSubtitlesOffIfPatternNotMatch) {
			return NO_SUBTITLES_INDEX;
		}
	}

	return 0;
}

void CMainFrame::OpenSetupSubStream(OpenMediaData* pOMD)
{
	CAppSettings& s = AfxGetAppSettings();

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if (m_pDVS = pBF) {
			break;
		}
	}
	EndEnumFilters;

	if (m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
			m_SubtitlesStreams.clear();

			int subcount = GetStreamCount(2);
			CComQIPtr<IAMStreamSelect> pSS	= m_pMainSourceFilter.p;
			if (subcount) {
				for (int i = 0; i < nLangs - 1; i++) {
					WCHAR *pName;
					if (SUCCEEDED(m_pDVS->get_LanguageName(i, &pName)) && pName) {
						Stream substream;
						substream.Filter	= 1;
						substream.Num		= i;
						substream.Index		= i;
						substream.Ext		= true;
						substream.Name		= pName;
						m_SubtitlesStreams.emplace_back(substream);

						CoTaskMemFree(pName);
					}
				}

				DWORD cStreams;
				pSS->Count(&cStreams);
				for (int i = 0, j = cStreams; i < j; i++) {
					DWORD dwFlags  = DWORD_MAX;
					DWORD dwGroup  = DWORD_MAX;
					WCHAR* pszName = nullptr;

					if (SUCCEEDED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, &pszName, nullptr, nullptr)) && pszName) {
						if (dwGroup != 2) {
							CoTaskMemFree(pszName);
							continue;
						}

						Stream substream;
						substream.Filter = 2;
						substream.Num++;
						substream.Index++;
						substream.Name = pszName;
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
							substream.Sel = m_SubtitlesStreams.size();
						}
						m_SubtitlesStreams.emplace_back(substream);

						CoTaskMemFree(pszName);
					}
				}
			} else {
				for (int i = 0; i < nLangs; i++) {
					WCHAR *pName;
					if (SUCCEEDED(m_pDVS->get_LanguageName(i, &pName)) && pName) {
						Stream substream;
						substream.Filter = 1;
						substream.Num    = i;
						substream.Index  = i;
						substream.Name   = pName;

						if (CComQIPtr<IDirectVobSub3> pDVS3 = m_pDVS.p) {
							int nType = 0;
							pDVS3->get_LanguageType(i, &nType);
							substream.Ext = !!nType;
						}

						m_SubtitlesStreams.emplace_back(substream);

						CoTaskMemFree(pName);
					}
				}
			}

			if (s.fUseInternalSelectTrackLogic) {
				SelectSubtilesAMStream(GetSubSelIdx());
			} else if (subcount && !s.fPrioritizeExternalSubtitles) {
				m_pDVS->put_SelectedLanguage(nLangs - 1);
			}
		}

		return;
	}

	if (m_pCAP && !m_bAudioOnly) {
		Stream substream;
		m_SubtitlesStreams.clear();
		int checkedsplsub    = 0;
		int subIndex         = -1;
		int iNum             = 0;
		m_nInternalSubsCount = 0;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
		if (!pSS) {
			pSS = m_pGB;
		}
		if (pSS && !s.fDisableInternalSubtitles) {

			DWORD cStreams;
			if (SUCCEEDED(pSS->Count(&cStreams))) {

				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS.p;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
					bIsHaali = TRUE;
					cStreams--;
				}

				for (DWORD i = 0, j = cStreams; i < j; i++) {
					DWORD dwFlags	= DWORD_MAX;
					DWORD dwGroup	= DWORD_MAX;
					LCID lcid		= DWORD_MAX;
					WCHAR* pName	= nullptr;

					if (FAILED(pSS->Info(i, nullptr, &dwFlags, &lcid, &dwGroup, &pName, nullptr, nullptr)) || !pName) {
						continue;
					}

					if (dwGroup == 2) {
						subIndex++;

						UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
						if (dwFlags) {
							checkedsplsub = subIndex;
						}
						m_nInternalSubsCount++;
						substream.Ext		= false;
						substream.Filter	= 1;
						substream.Num		= iNum++;
						substream.Index		= i;

						bool Forced, Def;
						SubFlags(pName, Forced, Def);

						substream.Name		= pName;
						substream.forced	= Forced;
						substream.def		= Def;

						m_SubtitlesStreams.emplace_back(substream);
					}

					CoTaskMemFree(pName);
				}
			}
		}

		if (s.fDisableInternalSubtitles) {
			m_pSubStreams.clear();
		}

		if (m_SubtitlesStreams.empty()) {
			auto it = m_pSubStreams.cbegin();
			int tPos = -1;
			while (it != m_pSubStreams.cend()) {
				tPos++;
				CComPtr<ISubStream> pSubStream = *it++;
				int i = pSubStream->GetStream();
				WCHAR* pName = nullptr;
				LCID lcid;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, &lcid))) {
					m_nInternalSubsCount++;

					substream.Ext		= false;
					substream.Filter	= 2;
					substream.Num		= iNum++;
					substream.Index		= tPos;

					bool Forced, Def;
					SubFlags(pName, Forced, Def);

					substream.Name		= pName;
					substream.forced	= Forced;
					substream.def		= Def;

					m_SubtitlesStreams.emplace_back(substream);

					if (pName) {
						CoTaskMemFree(pName);
					}
				}
			}
		}

		std::list<CComPtr<ISubStream>> subs;
		while (m_pSubStreams.size()) {
			subs.emplace_back(m_pSubStreams.front());
			m_pSubStreams.pop_front();
		}

		int iInternalSub = subs.size();

		for (const auto& si : pOMD->subs) {
			LoadSubtitle(si);
		}

		CComPtr<ISubStream> pSubStream;
		int tPos = -1;
		int extcnt = -1;
		for (size_t i = 0; i < m_SubtitlesStreams.size(); i++) {
			if (m_SubtitlesStreams[i].Filter == 2) extcnt++;
		}

		for (const auto& pSubStream : m_pSubStreams) {
			tPos++;
			if (!pSubStream) continue;

			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				WCHAR* pName = nullptr;
				LCID lcid;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, &lcid))) {
					substream.Ext		= true;
					substream.Filter	= 2;
					substream.Num		= iNum++;
					substream.Index		= tPos + (extcnt < 0 ? 0 : extcnt + 1);

					bool Forced, Def;
					SubFlags(pName, Forced, Def);

					substream.Name		= pName;
					substream.forced	= Forced;
					substream.def		= Def;

					m_SubtitlesStreams.emplace_back(substream);

					if (pName) {
						CoTaskMemFree(pName);
					}
				}
			}
		}

		auto it = m_pSubStreams.cbegin();
		for (const auto& sub : subs) {
			m_pSubStreams.insert(it, sub);
		}
		subs.clear();

		if (m_wndPlaylistBar.curPlayList.m_nSelectedSubtitleTrack != -1) {
			SetSubtitleTrackIdx(m_wndPlaylistBar.curPlayList.m_nSelectedSubtitleTrack);
			return;
		}

		if (m_nSubtitleTrackStored != -1) {
			SetSubtitleTrackIdx(m_nSubtitleTrackStored);
			return;
		}

		if (!s.fUseInternalSelectTrackLogic) {
			if (s.fPrioritizeExternalSubtitles) {
				size_t cnt = m_SubtitlesStreams.size();
				for (size_t i = 0; i < cnt; i++) {
					if (m_SubtitlesStreams[i].Ext)	{
						checkedsplsub = i;
						break;
					}
				}
			}
			SelectSubtilesAMStream(checkedsplsub);
		} else {
			size_t defsub = GetSubSelIdx();

			if (m_pMainSourceFilter && s.fDisableInternalSubtitles) {
				defsub++;
			}

			if (!m_SubtitlesStreams.empty()) {
				SelectSubtilesAMStream(defsub);
			}
		}
	}
}

void __stdcall MadVRExclusiveModeCallback(LPVOID context, int event)
{
#ifdef DEBUG_OR_LOG
	CString _event;
	switch (event) {
		case ExclusiveModeIsAboutToBeEntered : _event = L"ExclusiveModeIsAboutToBeEntered"; break;
		case ExclusiveModeWasJustEntered     : _event = L"ExclusiveModeWasJustEntered"; break;
		case ExclusiveModeIsAboutToBeLeft    : _event = L"ExclusiveModeIsAboutToBeLeft"; break;
		case ExclusiveModeWasJustLeft        : _event = L"ExclusiveModeWasJustLeft"; break;
		default                              : _event = L"Unknown event"; break;
	}
	DLog(L"MadVRExclusiveModeCallback() : event = %s", _event);
#endif

	CMainFrame* pFrame = (CMainFrame*)context;
	if (event == ExclusiveModeIsAboutToBeEntered) {
		pFrame->m_bIsMadVRExclusiveMode = true;
	} else if (event == ExclusiveModeIsAboutToBeLeft) {
		pFrame->m_bIsMadVRExclusiveMode = false;
	}
	pFrame->FlyBarSetPos();
}

#define BREAK(msg) {err = msg; break;}
bool CMainFrame::OpenMediaPrivate(std::unique_ptr<OpenMediaData>& pOMD)
{
	ASSERT(m_eMediaLoadState == MLS_LOADING);
	CAppSettings& s = AfxGetAppSettings();

	SetAudioPicture(FALSE);

	m_bValidDVDOpen = false;

	OpenFileData*   pFileData   = dynamic_cast<OpenFileData*>(pOMD.get());
	OpenDVDData*    pDVDData    = dynamic_cast<OpenDVDData*>(pOMD.get());
	OpenDeviceData* pDeviceData = dynamic_cast<OpenDeviceData*>(pOMD.get());
	ASSERT(pFileData || pDVDData || pDeviceData);

	m_ExtSubFiles.clear();
	m_ExtSubPaths.clear();
	m_EventSubChangeRefreshNotify.Set();

	if (!s.bSpeedNotReset) {
		m_PlaybackRate = 1.0;
	}
	m_iDefRotation = 0;

	DXVAState::ClearState();

	m_bWasPausedOnMinimizedVideo = false;

	if (pFileData) {
		auto& path = pFileData->fi.GetPath();
		if (::PathIsURLW(path) && path.Find(L"://") <= 0) {
			pFileData->fi = L"http://" + path;
		}
		DLog(L"--> CMainFrame::OpenMediaPrivate() - pFileData->fns:");
		DLog(L"    %s", pFileData->fi.GetPath());

		UINT index = 0;
		for (const auto& aud : pFileData->auds) {
			DLog(L"--> CMainFrame::OpenMediaPrivate() - pFileData->auds[%d]:", index++);
			DLog(L"    %s", aud.GetPath());
		}
	}

	CString mi_fn;
	for (;;) {
		if (pFileData) {
			if (!pFileData->fi.Valid()) {
				ASSERT(FALSE);
				break;
			}

			CString fn = pFileData->fi;

			int i = fn.Find(L":\\");
			if (i > 0) {
				CString drive = fn.Left(i+2);
				UINT type = GetDriveTypeW(drive);
				std::list<CString> sl;
				if (type == DRIVE_REMOVABLE || (type == DRIVE_CDROM && GetCDROMType(drive[0], sl) != CDROM_Audio)) {
					int ret = IDRETRY;
					while (ret == IDRETRY) {
						WIN32_FIND_DATAW findFileData;
						HANDLE h = FindFirstFileW(fn, &findFileData);
						if (h != INVALID_HANDLE_VALUE) {
							FindClose(h);
							ret = IDOK;
						} else {
							CString msg;
							msg.Format(ResStr(IDS_MAINFRM_114), fn);
							ret = AfxMessageBox(msg, MB_RETRYCANCEL);
						}
					}

					if (ret != IDOK) {
						ASSERT(FALSE);
						break;
					}
				}
			}
			mi_fn = fn;
		}

		m_dMediaInfoFPS	= 0.0;
		m_bNeedAutoChangeMonitorMode = false;

		if ((s.fullScreenModes.bEnabled == 1 && (IsD3DFullScreenMode() || m_bFullScreen || s.fLaunchfullscreen))
				|| s.fullScreenModes.bEnabled == 2) {
			// DVD
			if (pDVDData) {
				mi_fn = pDVDData->path;
				CString ext = GetFileExt(mi_fn);
				if (ext.IsEmpty()) {
					if (mi_fn.Right(10) == L"\\VIDEO_TS\\") {
						mi_fn = mi_fn + L"VTS_01_1.VOB";
					} else {
						mi_fn = mi_fn + L"\\VIDEO_TS\\VTS_01_1.VOB";
					}
				} else if (ext == L".IFO") {
					mi_fn = GetFolderPath(mi_fn) + L"\\VTS_01_1.VOB";
				}
			} else {
				CString ext = GetFileExt(mi_fn);
				// BD
				if (ext == L".mpls") {
					CHdmvClipInfo ClipInfo;
					CHdmvClipInfo::CPlaylist CurPlaylist;
					REFERENCE_TIME rtDuration;
					if (SUCCEEDED(ClipInfo.ReadPlaylist(mi_fn, rtDuration, CurPlaylist)) && !CurPlaylist.empty()) {
						mi_fn = CurPlaylist.begin()->m_strFileName;
					}
				} else if (ext == L".IFO") {
					// DVD structure
					CString sVOB = mi_fn;

					for (int i = 1; i < 10; i++) {
						sVOB = mi_fn;
						CString vob;
						vob.Format(L"%d.VOB", i);
						sVOB.Replace(L"0.IFO", vob);

						if (::PathFileExistsW(sVOB)) {
							mi_fn = sVOB;
							break;
						}
					}
				}
			}

			// Get FPS
			MediaInfo MI;
			MI.Option(L"ParseSpeed", L"0");
			if (MI.Open(mi_fn.GetString())) {
				for (int i = 0; i < 2; i++) {
					CString strFPS = MI.Get(Stream_Video, 0, L"FrameRate", Info_Text, Info_Name).c_str();
					if (strFPS.IsEmpty() || _wtof(strFPS) > 200.0) {
						strFPS = MI.Get(Stream_Video, 0, L"FrameRate_Original", Info_Text, Info_Name).c_str();
					}
					CString strST = MI.Get(Stream_Video, 0, L"ScanType", Info_Text, Info_Name).c_str();
					CString strSO = MI.Get(Stream_Video, 0, L"ScanOrder", Info_Text, Info_Name).c_str();

					double nFactor = 1.0;

					// 2:3 pulldown
					if (strFPS == L"29.970" && (strSO == L"2:3 Pulldown" || (strST == L"Progressive" && (strSO == L"TFF" || strSO == L"BFF" || strSO == L"2:3 Pulldown")))) {
						strFPS = L"23.976";
					} else if (strST == L"Interlaced" || strST == L"MBAFF") {
						// double fps for Interlaced video.
						nFactor = 2.0;
					}
					m_dMediaInfoFPS = _wtof(strFPS);
					if (m_dMediaInfoFPS < 30.0 && nFactor > 1.0) {
						m_dMediaInfoFPS *= nFactor;
					}

					if (m_dMediaInfoFPS > 0.9) {
						break;
					}

					MI.Close();
					MI.Option(L"ParseSpeed", L"0.5");
					if (!MI.Open(mi_fn.GetString())) {
						break;
					}
				}

				AutoChangeMonitorMode();

				if (s.fLaunchfullscreen && !IsD3DFullScreenMode() && !m_bFullScreen && !m_bAudioOnly ) {
					ToggleFullscreen(true, true);
				}
			} else {
				m_bNeedAutoChangeMonitorMode = true;
			}
		}

		break;
	}

	// load fonts from 'fonts' folder
	if (pFileData) {
		const CString path =  GetFolderPath(pFileData->fi) + L"\\fonts\\";

		if (::PathIsDirectoryW(path)) {
			WIN32_FIND_DATAW fd = { 0 };
			HANDLE hFind = FindFirstFileW(path + L"*.ttf", &fd);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					m_FontInstaller.InstallFontFile(path + fd.cFileName);
				} while (FindNextFileW(hFind, &fd));
				FindClose(hFind);
			}

			hFind = FindFirstFileW(path + L"*.otf", &fd);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					m_FontInstaller.InstallFontFile(path + fd.cFileName);
				} while (FindNextFileW(hFind, &fd));
				FindClose(hFind);
			}
		}
	}

	CString err, aborted(ResStr(IDS_AG_ABORTED));

	BeginWaitCursor();

	for (;;) {
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		err = OpenCreateGraphObject(pOMD.get());
		if (!err.IsEmpty()) {
			break;
		}

		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		if (pFileData) {
			err = OpenFile(pFileData);
			if (!err.IsEmpty()) {
				break;
			}
		} else if (pDVDData) {
			err = OpenDVD(pDVDData);
			if (!err.IsEmpty()) {
				break;
			}
		} else if (pDeviceData) {
			if (s.iDefaultCaptureDevice == 1) {
				HRESULT hr = OpenBDAGraph();
				if (FAILED(hr)) {
					err = ResStr(IDS_CAPTURE_ERROR_DEVICE);
				}
			} else {
				err = OpenCapture(pDeviceData);
				if (!err.IsEmpty()) {
					break;
				}
			}
		} else {
			BREAK(ResStr(IDS_INVALID_PARAMS_ERROR))
		}

		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		m_pCAP.Release();
		m_clsidCAP = GUID_NULL;

		m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
		if (m_pCAP) {
			m_clsidCAP = m_pCAP->GetAPCLSID();
		}
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRWC), FALSE); // might have IVMRMixerBitmap9, but not IVMRWindowlessControl9
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRMC9), TRUE);
		m_pMVRSR = m_pCAP;
		m_pMVRS  = m_pCAP;
		m_pMVRC  = m_pCAP;
		m_pMVRI  = m_pCAP;
		m_pMVRFG = m_pCAP;
		m_pMVTO  = m_pCAP;
		m_pD3DFS = m_pCAP;

		SetupVMR9ColorControl();

		// === EVR !
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVDC), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVP), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVMB), TRUE);

		if (m_pMFVDC) {
			RECT Rect;
			m_pVideoWnd->GetClientRect(&Rect);
			m_pMFVDC->SetVideoWindow(m_pVideoWnd->m_hWnd);
			m_pMFVDC->SetVideoPosition(nullptr, &Rect);

			m_pPlaybackNotify = m_pMFVDC;
		}

		if (m_pGB_preview) {
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pMFVDC_preview), TRUE);
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pCAP_preview), TRUE);

			RECT wr;
			m_wndPreView.GetClientRect(&wr);
			if (m_pMFVDC_preview) {
				m_pMFVDC_preview->SetVideoWindow(m_wndPreView.GetVideoHWND());
				m_pMFVDC_preview->SetVideoPosition(nullptr, &wr);
			}
			if (m_pCAP_preview) {
				m_pCAP_preview->SetPosition(wr, wr);
			}
		}

		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (m_pLN21 = pBF) {
				m_pLN21->SetServiceState(s.bClosedCaptions ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off);
				break;
			}
		}
		EndEnumFilters;

		{
			CComQIPtr<IExFilterConfig> pIExFilterConfig;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (pIExFilterConfig = pBF) {
					pIExFilterConfig->Flt_SetInt("queueDuration", s.iBufferDuration);
					//pIExFilterConfig->SetInt("networkTimeout", s.iNetworkTimeout);
				}
			}
			EndEnumFilters;
			pIExFilterConfig.Release();
		}

		if (m_pCAP) {
			CComQIPtr<IAllocatorPresenter> pCap;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (pCap = pBF) { // video renderer found
					while (pBF = GetUpStreamFilter(pBF)) {
						if (CComQIPtr<IPropertyBag> pPB = pBF.p) {
							CComVariant var;
							if (SUCCEEDED(pPB->Read(L"ROTATION", &var, nullptr)) && var.vt == VT_BSTR) {
								m_iDefRotation = _wtoi(var.bstrVal);
							}
							var.Clear();
						}
					}
					break;
				}
			}
			EndEnumFilters;
		}

		OpenCustomizeGraph();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		OpenSetupVideo();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		if (s.ShowOSD.Enable || s.bShowDebugInfo) { // Force OSD on when the debug switch is used
			m_OSD.Stop();

			if (IsD3DFullScreenMode() && !m_bAudioOnly) {
				if (m_pMFVMB) {
					m_OSD.Start(m_pVideoWnd, m_pMFVMB);
				}
			} else {
				if (m_pMVTO) {
					m_OSD.Start(m_pVideoWnd, m_pMVTO);
				} else {
					m_OSD.Start(m_pOSDWnd);
				}
			}
		}

		OpenSetupAudio();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}
		// Apply command line audio shift
		if (s.rtShift != 0) {
			SetAudioDelay(s.rtShift);
			s.rtShift = 0;
		}

		UpdateWindowTitle();

		OpenSetupSubStream(pOMD.get());

		m_pSwitcherFilter = FindSwitcherFilter();

		OpenSetupAudioStream();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		m_bIsMadVRExclusiveMode = false;
		// madVR - register Callback function for detect Entered to ExclusiveMode
		m_pBFmadVR = FindFilter(CLSID_madVR, m_pGB);
		if (m_pBFmadVR) {
			CComQIPtr<IMadVRExclusiveModeCallback> pMVEMC = m_pBFmadVR.p;
			if (pMVEMC) {
				pMVEMC->Register(MadVRExclusiveModeCallback, this);
			}
		}

		m_bUseReclock = (FindFilter(CLSID_ReClock, m_pGB) != nullptr);

		if (pFileData && pFileData->rtStart > 0 && m_pMS) {
			REFERENCE_TIME rtDur = 0;
			m_pMS->GetDuration(&rtDur);
			if (rtDur > 0 && pFileData->rtStart <= rtDur) {
				REFERENCE_TIME rtPos = pFileData->rtStart;
				VERIFY(SUCCEEDED(m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning)));
			}
		}

		if (m_pMC_preview) {
			m_pMC_preview->Pause();
		}

		break;
	}

	EndWaitCursor();

	m_closingmsg = err;
	WPARAM wp    = (WPARAM)pOMD.release();
	if (::GetCurrentThreadId() == AfxGetApp()->m_nThreadID) {
		OnPostOpen(wp, 0);
	} else {
		PostMessageW(WM_POSTOPEN, wp, 0);
	}

	return (err.IsEmpty());
}

void CMainFrame::CloseMediaPrivate()
{
	DLog(L"CMainFrame::CloseMediaPrivate() : start");

	ASSERT(m_eMediaLoadState == MLS_CLOSING);

	KillTimer(TIMER_DM_AUTOCHANGING);

	auto& s = AfxGetAppSettings();

	if (s.bKeepHistory) {
		SaveHistory();
	}

	if (s.bRememberSelectedTracks && m_bRememberSelectedTracks) {
		m_wndPlaylistBar.curPlayList.m_nSelectedAudioTrack = GetAudioTrackIdx();
		m_wndPlaylistBar.curPlayList.m_nSelectedSubtitleTrack = GetSubtitleTrackIdx();
	}

	m_ExternalSubstreams.clear();

	ClearPlaybackInfo();

	m_strPlaybackRenderedPath.Empty();

	if (!m_bYoutubeOpened) {
		m_youtubeFields.Empty();
		m_youtubeUrllist.clear();
		m_youtubeAudioUrllist.clear();
		s.iYoutubeTagSelected = 0;

		YoutubeDL::Clear();
	}
	m_youtubeThumbnailData.clear();
	m_bYoutubeOpened = false;

	OnPlayStop();
	m_OSD.Stop();

	m_pparray.clear();
	m_ssarray.clear();

	Content::Online::Clear();

	if (m_pMC) {
		if (GetPlaybackMode() == PM_FILE && m_clsidCAP == CLSID_EVRAllocatorPresenter) {
			LONGLONG dur = 0;
			m_pMS->GetDuration(&dur);
			// LAV Splitter may hang when calling IMediaSeeking::SetPositions if duration is unknown
			if (dur > 0) {
				// Requires a position reset so that the EVR-CP stops waiting for the next frame
				// when playback ends immediately after the first frame.
				LONGLONG pos = 0;
				m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			}
		}
		OAFilterState fs;
		m_pMC->GetState(0, &fs);
		if (fs != State_Stopped) {
			// Some filters, such as Microsoft StreamBufferSource, need to call IMediaControl::Stop() before close the graph;
			m_pMC->Stop();
		}
	}
	m_bCapturing = false;

	UnHookDirectXVideoDecoderService();

	// madVR - unregister Callback function
	if (m_pBFmadVR) {
		CComQIPtr<IMadVRExclusiveModeCallback> pMVEMC = m_pBFmadVR.p;
		if (pMVEMC) {
			pMVEMC->Unregister(MadVRExclusiveModeCallback, this);
		}
		m_pBFmadVR.Release();
	}
	m_bIsMadVRExclusiveMode = false;
	m_bIsMPCVRExclusiveMode = false;

	m_bLiveWM = false;
	m_bEndOfStream = false;
	m_bGraphEventComplete = false;
	m_rtDurationOverride = -1;
	m_kfs.clear();
	m_pCB.Release();
	m_chapterTitleNum = 0;
	m_bFrameSteppingActive = false;

	{
		CAutoLock cAutoLock(&m_csSubLock);
		m_pSubStreams.clear();
	}
	m_pSubClock.Release();

	//if (pVW) pVW->put_Visible(OAFALSE);
	//if (pVW) pVW->put_MessageDrain((OAHWND)nullptr), pVW->put_Owner((OAHWND)nullptr);

	// IMPORTANT: IVMRSurfaceAllocatorNotify/IVMRSurfaceAllocatorNotify9 has to be released before the VMR/VMR9, otherwise it will crash in Release()
	m_pMVRSR.Release();
	m_pMVRS.Release();
	m_pMVRC.Release();
	m_pMVRI.Release();
	m_pMVRFG.Release();
	m_pCAP.Release();
	m_clsidCAP = GUID_NULL;
	m_pVMRWC.Release();
	m_pVMRMC9.Release();
	m_pMVTO.Release();

	m_pPlaybackNotify.Release();
	m_pD3DFS.Release();

	m_pMFVMB.Release();
	m_pMFVP.Release();
	m_pMFVDC.Release();
	m_pLN21.Release();
	m_pSyncClock.Release();

	m_pAMXBar.Release();
	m_pAMDF.Release();
	m_pAMVSCCap.Release();
	m_pAMVSCPrev.Release();
	m_pAMASC.Release();
	m_pVidCap.Release();
	m_pAudCap.Release();
	m_pAMTuner.Release();
	m_pCGB.Release();

	m_pDVDC.Release();
	m_pDVDI.Release();
	m_pAMOP.Release();
	m_pQP.Release();
	m_pFS.Release();
	m_pMS.Release();
	m_pBA.Release();
	m_pBV.Release();
	m_pVW.Release();
	m_pME.Release();
	m_pMC.Release();
	m_pMainFSF.Release();
	m_pMainSourceFilter.Release();
	m_pSwitcherFilter.Release();
	m_pDVS.Release();
	for (auto& pAMMC : m_pAMMC) {
		pAMMC.Release();
	}
	m_pKFI.Release();

	if (m_pGB) {
		m_pGB->RemoveFromROT();
		m_pGB.Release();
	}

	PreviewWindowHide();
	ReleasePreviewGraph();

	m_pProv.Release();

	m_bShockwaveGraph = false;

	m_VidDispName.Empty();
	m_AudDispName.Empty();

	m_bMainIsMPEGSplitter = false;

	m_FontInstaller.UninstallFonts();

	m_CMediaControls.Update();

	m_bIsLiveOnline = false;

	m_iDVDTitle = 0;
	m_iDVDTitleForHistory = 0;
	m_bDVDStillOn = false;

	DLog(L"CMainFrame::CloseMediaPrivate() : end");
}

inline bool Check_DVD_BD_Folder(const CString& path)
{
	return (::PathFileExistsW(path + L"\\VIDEO_TS.IFO") || ::PathFileExistsW(path + L"\\index.bdmv"));
}


static void RecurseAddDir(const CString& path, std::list<CString>& sl)
{
	WIN32_FIND_DATAW fd = {0};

	HANDLE hFind = FindFirstFileW(path + L"*.*", &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			CString f_name = fd.cFileName;

			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (f_name != L".") && (f_name != L"..")) {
				const CString fullpath = GetAddSlash(path + f_name);

				sl.emplace_back(fullpath);
				if (Check_DVD_BD_Folder(fullpath)) {
					break;
				}
				RecurseAddDir(fullpath, sl);
			} else {
				continue;
			}
		} while (FindNextFileW(hFind, &fd));

		FindClose(hFind);
	}
}

void CMainFrame::ParseDirs(std::list<CString>& sl)
{
	std::vector<CString> tmp{ sl.cbegin(), sl.cend() };
	for (auto fn : tmp) {
		if (::PathIsDirectoryW(fn) && ::PathFileExistsW(fn)) {
			AddSlash(fn);
			if (Check_DVD_BD_Folder(fn)) {
				continue;
			}

			RecurseAddDir(GetAddSlash(fn), sl);
		}
	}
}

int CMainFrame::SearchInDir(const bool bForward)
{
	std::list<CString> sl;

	CAppSettings& s = AfxGetAppSettings();
	CMediaFormats& mf = s.m_Formats;

	const CString dir  = GetAddSlash(GetFolderPath(m_LastOpenFile));
	const CString mask = dir + L"*.*";
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(mask, &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			}

			const CString ext = GetFileExt(fd.cFileName).MakeLower();
			if (mf.FindExt(ext)) {
				sl.emplace_back(dir + fd.cFileName);
			}
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}

	if (sl.size() == 1) {
		return 1;
	}

	sl.sort([](const CString& a, const CString& b) {
		return (StrCmpLogicalW(a, b) < 0);
	});

	auto it = std::find(sl.cbegin(), sl.cend(), m_LastOpenFile);
	if (it == sl.cend()) {
		return 0;
	}

	if (bForward) {
		if (it == std::prev(sl.cend())) {
			if (s.fNextInDirAfterPlaybackLooped) {
				it = sl.cbegin();
			} else {
				return 0;
			}
		} else {
			++it;
		}
	} else {
		if (it == sl.cbegin()) {
			if (s.fNextInDirAfterPlaybackLooped) {
				it = std::prev(sl.cend());
			} else {
				return 0;
			}
		} else {
			--it;
		}
	}

	m_wndPlaylistBar.Open(*it);
	OpenCurPlaylistItem();

	return sl.size();
}

void CMainFrame::DoTunerScan(TunerScanData* pTSD)
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		CComQIPtr<IBDATuner>	pTun = m_pGB.p;
		if (pTun) {
			BOOLEAN bPresent;
			BOOLEAN bLocked;
			LONG    lStrength;
			LONG    lQuality;
			int     nProgress;
			int     nOffset = pTSD->Offset ? 3 : 1;
			LONG    lOffsets[3] = { 0, pTSD->Offset, -pTSD->Offset };
			bool    bSucceeded;
			m_bStopTunerScan = false;

			for (ULONG ulFrequency = pTSD->FrequencyStart; ulFrequency <= pTSD->FrequencyStop; ulFrequency += pTSD->Bandwidth) {
				bSucceeded = false;
				for (int nOffsetPos = 0; nOffsetPos < nOffset && !bSucceeded; nOffsetPos++) {
					pTun->SetFrequency (ulFrequency + lOffsets[nOffsetPos]);
					Sleep (200);
					if (SUCCEEDED (pTun->GetStats (bPresent, bLocked, lStrength, lQuality)) && bPresent) {
						::SendMessageW (pTSD->Hwnd, WM_TUNER_STATS, lStrength, lQuality);
						pTun->Scan (ulFrequency + lOffsets[nOffsetPos], pTSD->Hwnd);
						bSucceeded = true;
					}
				}

				nProgress = MulDiv(ulFrequency-pTSD->FrequencyStart, 100, pTSD->FrequencyStop-pTSD->FrequencyStart);
				::SendMessageW (pTSD->Hwnd, WM_TUNER_SCAN_PROGRESS, nProgress,0);
				::SendMessageW (pTSD->Hwnd, WM_TUNER_STATS, lStrength, lQuality);

				if (m_bStopTunerScan) {
					break;
				}
			}

			::SendMessageW (pTSD->Hwnd, WM_TUNER_SCAN_END, 0, 0);
		}
	}
}

// dynamic menus

void CMainFrame::MakeEmptySubMenu(CMenu& menu)
{
	if (!IsMenu(menu.m_hMenu)) {
		menu.m_hMenu = nullptr;
		menu.CreatePopupMenu();
	}
	else while (menu.RemoveMenu(0, MF_BYPOSITION));
}

void CMainFrame::SetupOpenCDSubMenu()
{
	CMenu& submenu = m_openCDsMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState == MLS_LOADING || AfxGetAppSettings().bHideCDROMsSubMenu) {
		return;
	}

	UINT id = ID_FILE_OPEN_CD_START;

	for (WCHAR drive = 'C'; drive <= 'Z'; drive++) {
		std::list<CString> files;
		cdrom_t CDrom_t = GetCDROMType(drive, files);
		if (CDrom_t != CDROM_NotFound && CDrom_t != CDROM_Unknown) {
			CString label = GetDriveLabel(drive);

			CString DiskType;
			switch (CDrom_t) {
				case CDROM_Audio:
					DiskType = L"Audio CD";
					break;
				case CDROM_VideoCD:
					DiskType = L"(S)VCD";
					break;
				case CDROM_DVDVideo:
					DiskType = L"DVD-Video";
					break;
				case CDROM_BDVideo:
					DiskType = L"Blu-ray Disc";
					break;
				case CDROM_DVDAudio:
					DiskType = L"DVD-Audio";
					break;
				default:
					ASSERT(FALSE);
					break;
			}

			if (label.IsEmpty()) {
				label = DiskType;
			} else {
				label.AppendFormat(L" - %s", DiskType);
			}

			CString str;
			str.Format(L"%s (%c:)", label, drive);
			if (!str.IsEmpty()) {
				//UINT state = (m_DiskImage.GetDriveLetter() == drive) ? (MF_GRAYED) : MF_ENABLED; // hmm, don't work
				submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, str);
			}
		}
	}
}

void CMainFrame::SetupFiltersSubMenu()
{
	CMenu& submenu = m_filtersMenu;
	MakeEmptySubMenu(submenu);

	m_pparray.clear();
	m_ssarray.clear();

	if (m_eMediaLoadState == MLS_LOADED) {
		UINT idf = 0;
		UINT ids = ID_FILTERS_SUBITEM_START;
		UINT idl = ID_FILTERSTREAMS_SUBITEM_START;

		BeginEnumFilters(m_pGB, pEF, pBF) {
			CString name(GetFilterName(pBF));
			if (name.GetLength() >= 43) {
				name = name.Left(40) + L"...";
			}

			CLSID clsid = GetCLSID(pBF);
			if (clsid == CLSID_AVIDec) {
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				CMediaType mt;
				if (pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
					DWORD c = ((VIDEOINFOHEADER*)mt.pbFormat)->bmiHeader.biCompression;
					switch (c) {
						case BI_RGB:
							name += L" (RGB)";
							break;
						case BI_RLE4:
							name += L" (RLE4)";
							break;
						case BI_RLE8:
							name += L" (RLE8)";
							break;
						case BI_BITFIELDS:
							name += L" (BITF)";
							break;
						default:
							name.Format(L"%s (%c%c%c%c)",
										CString(name),
										(WCHAR)(c & 0xff),
										(WCHAR)((c >> 8) & 0xff),
										(WCHAR)((c >> 16) & 0xff),
										(WCHAR)((c >> 24) & 0xff));
							break;
					}
				}
			} else if (clsid == CLSID_ACMWrapper) {
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				CMediaType mt;
				if (pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
					WORD c = ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag;
					name.Format(L"%s (0x%04x)", CString(name), (int)c);
				}
			} else if (clsid == __uuidof(CTextPassThruFilter)
					|| clsid == __uuidof(CNullTextRenderer)
					|| clsid == __uuidof(ChaptersSouce)
					|| clsid == GUIDFromCString(L"{48025243-2D39-11CE-875D-00608CB78066}")) { // ISCR
				// hide these
				continue;
			}

			CMenu subMenu;
			subMenu.CreatePopupMenu();

			int nPPages = 0;

			CComQIPtr<ISpecifyPropertyPages> pSPP = pBF.p;
			m_pparray.emplace_back(pBF);
			subMenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids, ResStr(IDS_MAINFRM_116));
			nPPages++;

			BeginEnumPins(pBF, pEP, pPin) {
				CString name = GetPinName(pPin);
				name.Replace(L"&", L"&&");

				if (pSPP = pPin) {
					CAUUID caGUID;
					caGUID.pElems = nullptr;
					if (SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0) {
						m_pparray.emplace_back(pPin);
						subMenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids + nPPages, name + ResStr(IDS_MAINFRM_117));

						if (caGUID.pElems) {
							CoTaskMemFree(caGUID.pElems);
						}

						nPPages++;
					}
				}
			}
			EndEnumPins;

			CComQIPtr<IAMStreamSelect> pSS = pBF.p;
			if (pSS) {
				DWORD nStreams = 0;
				DWORD flags		= (DWORD)-1;
				DWORD group		= (DWORD)-1;
				DWORD prevgroup	= (DWORD)-1;
				WCHAR* wname	= nullptr;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);

				if (nStreams > 0 && nPPages > 0) {
					subMenu.AppendMenuW(MF_SEPARATOR | MF_ENABLED);
				}

				UINT idlstart = idl;

				for (DWORD i = 0; i < nStreams; i++, pObj = nullptr, pUnk = nullptr) {
					m_ssarray.emplace_back(pSS);

					flags = group = DWORD_MAX;
					wname = nullptr;
					pSS->Info(i, nullptr, &flags, nullptr, &group, &wname, &pObj, &pUnk);

					if (group != prevgroup && idl > idlstart) {
						subMenu.AppendMenuW(MF_SEPARATOR | MF_ENABLED);
					}
					prevgroup = group;

					UINT nflags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
					if (flags & AMSTREAMSELECTINFO_EXCLUSIVE) {
						nflags |= MF_CHECKED | MFT_RADIOCHECK;
					} else if (flags & AMSTREAMSELECTINFO_ENABLED) {
						nflags |= MF_CHECKED;
					}

					if (!wname) {
						CStringW stream(ResStr(IDS_AG_UNKNOWN_STREAM));
						size_t count = stream.GetLength() + 3 + 1;
						wname = (WCHAR*)CoTaskMemAlloc(count * sizeof(WCHAR));
						swprintf_s(wname, count, L"%s %d", stream.GetString(), std::min(i + 1, 999uL));
					}

					CString name(wname);
					name.Replace(L"&", L"&&");

					subMenu.AppendMenuW(nflags, idl++, name);

					CoTaskMemFree(wname);
				}

				if (!nStreams) {
					pSS.Release();
				}
			}

			if (nPPages == 1 && !pSS) {
				submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids, name);
			} else {
				//submenu.AppendMenuW(MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, idf, name);

				if (nPPages > 0 || pSS) {
					//MENUITEMINFO mii;
					//mii.cbSize = sizeof(mii);
					//mii.fMask = MIIM_STATE | MIIM_SUBMENU;
					//mii.fType = MF_POPUP;
					//mii.hSubMenu = subMenu.Detach();
					//mii.fState = (pSPP || pSS) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
					//submenu.SetMenuItemInfo(idf, &mii, TRUE);
					submenu.AppendMenuW(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)subMenu.Detach(), name);
				}
			}

			ids += nPPages;
			idf++;
		}
		EndEnumFilters;

		if (submenu.GetMenuItemCount() > 0) {
			VERIFY(submenu.InsertMenu(0, MF_STRING | MF_ENABLED | MF_BYPOSITION, ID_FILTERS_COPY_TO_CLIPBOARD, ResStr(IDS_FILTERS_COPY_TO_CLIPBOARD)));
			VERIFY(submenu.InsertMenu(1, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION));
		}
	}
}

void CMainFrame::SetupLanguageMenu()
{
	CMenu& submenu = m_languageMenu;
	MakeEmptySubMenu(submenu);

	int iCount = 0;
	const CAppSettings& s = AfxGetAppSettings();

	for (size_t i = 0; i < CMPlayerCApp::languageResourcesCount; i++) {

		const LanguageResource& lr	= CMPlayerCApp::languageResources[i];
		CString strSatellite		= CMPlayerCApp::GetSatelliteDll(i);

		if (lr.resourceID == ID_LANGUAGE_ENGLISH || ::PathFileExistsW(strSatellite)) {
			UINT uFlags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (i == s.iLanguage) {
				uFlags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			submenu.AppendMenuW(uFlags, i + ID_LANGUAGE_ENGLISH, lr.name);
			iCount++;
		}
	}

	if (iCount <= 1) {
		submenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

void CMainFrame::SetupAudioSubMenu()
{
	SetupAudioTracksSubMenu();

	CMenu& submenu = m_AudioMenu;
	if (submenu.GetMenuItemCount() >= 1) {
		submenu.AppendMenuW(MF_SEPARATOR);
	}

	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_AUDIO, ResStr(IDS_AG_LOAD_AUDIO));
}

void CMainFrame::SetupAudioRButtonMenu()
{
	CMenu& submenu = m_RButtonMenu;
	MakeEmptySubMenu(submenu);

	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_AUDIO_OPTIONS, ResStr(IDS_SUBTITLES_OPTIONS));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_AUDIO, ResStr(IDS_AG_LOAD_AUDIO));
}

void CMainFrame::SetupSubtitlesSubMenu()
{
	SetupSubtitleTracksSubMenu();

	CMenu& submenu = m_SubtitlesMenu;
	if (submenu.GetMenuItemCount() == 0) {
		submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
	}

	submenu.AppendMenuW(MF_SEPARATOR);
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_SUBTITLE, ResStr(IDS_AG_LOAD_SUBTITLE));


	CMenu submenu2;
	submenu2.CreatePopupMenu();
	submenu2.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_STYLES, ResStr(IDS_SUBTITLES_STYLES));
	submenu2.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_RELOAD, ResStr(IDS_SUBTITLES_RELOAD));
	submenu2.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_DEFSTYLE, ResStr(IDS_SUBTITLES_DEFAULT_STYLE));
	submenu2.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_FORCEDONLY, ResStr(IDS_SUBTITLES_FORCED));
	submenu.AppendMenuW(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu2.Detach(), ResStr(IDS_AG_ADVANCED));

	auto SetFlags = [](int smode) {
		if (AfxGetAppSettings().m_VRSettings.Stereo3DSets.iMode == smode) {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED | MF_CHECKED | MFT_RADIOCHECK;
		} else {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		}
	};

	CMenu submenu3;
	submenu3.CreatePopupMenu();
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_NONE), ID_SUBTITLES_STEREO_DONTUSE, ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_SIDEBYSIDE), ID_SUBTITLES_STEREO_SIDEBYSIDE, ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_TOPANDBOTTOM), ID_SUBTITLES_STEREO_TOPBOTTOM, ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
	submenu.AppendMenuW(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu3.Detach(), ResStr(IDS_SUBTITLES_STEREO));
}

void CMainFrame::SetupSubtitlesRButtonMenu()
{
	CMenu& submenu = m_RButtonMenu;
	MakeEmptySubMenu(submenu);

	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_OPTIONS, ResStr(IDS_SUBTITLES_OPTIONS));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_SUBTITLE, ResStr(IDS_AG_LOAD_SUBTITLE));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_STYLES, ResStr(IDS_SUBTITLES_STYLES));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_RELOAD, ResStr(IDS_SUBTITLES_RELOAD));
	submenu.AppendMenuW(MF_SEPARATOR);

	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_DEFSTYLE, ResStr(IDS_SUBTITLES_DEFAULT_STYLE));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_FORCEDONLY, ResStr(IDS_SUBTITLES_FORCED));
	submenu.AppendMenuW(MF_SEPARATOR);

	auto SetFlags = [](int smode) {
		if (AfxGetAppSettings().m_VRSettings.Stereo3DSets.iMode == smode) {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED | MF_CHECKED | MFT_RADIOCHECK;
		}
		else {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		}
	};

	CMenu submenu3;
	submenu3.CreatePopupMenu();
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_NONE), ID_SUBTITLES_STEREO_DONTUSE, ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_SIDEBYSIDE), ID_SUBTITLES_STEREO_SIDEBYSIDE, ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
	submenu3.AppendMenuW(SetFlags(SUBPIC_STEREO_TOPANDBOTTOM), ID_SUBTITLES_STEREO_TOPBOTTOM, ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
	submenu.AppendMenuW(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu3.Detach(), ResStr(IDS_SUBTITLES_STEREO));
}

void CMainFrame::SetupSubtitleTracksSubMenu()
{
	CMenu& submenu = m_SubtitlesMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_SUBTITLES_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		SetupSubtilesAMStreamSubMenu(submenu, id);
	} else if (GetPlaybackMode() == PM_DVD) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (FAILED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
				|| ulStreamsAvailable == 0) {
			return;
		}

		LCID DefLanguage;
		DVD_SUBPICTURE_LANG_EXT ext;
		if (FAILED(m_pDVDI->GetDefaultSubpictureLanguage(&DefLanguage, &ext))) {
			return;
		}

		submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | (bIsDisabled ? MF_ENABLED : MF_CHECKED), ID_SUBTITLES_ENABLE, ResStr(IDS_AG_ENABLED));
		submenu.AppendMenuW(MF_BYCOMMAND | MF_SEPARATOR | MF_ENABLED);

		for (ULONG i = 0; i < ulStreamsAvailable; i++) {
			LCID Language;
			if (FAILED(m_pDVDI->GetSubpictureLanguage(i, &Language))) {
				continue;
			}

			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (Language == DefLanguage) {
				flags |= MF_DEFAULT;
			}
			if (i == ulCurrentStream && !bIsDisabled) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}

			CString str;
			if (Language) {
				int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(std::max(len - 1, 0));
			} else {
				str.Format(ResStr(IDS_AG_UNKNOWN), i + 1);
			}

			DVD_SubpictureAttributes ATR;
			if (SUCCEEDED(m_pDVDI->GetSubpictureAttributes(i, &ATR))) {
				switch (ATR.LanguageExtension) {
					case DVD_SP_EXT_NotSpecified:
					default:
						break;
					case DVD_SP_EXT_Caption_Normal:
						str += L"";
						break;
					case DVD_SP_EXT_Caption_Big:
						str += L" (Big)";
						break;
					case DVD_SP_EXT_Caption_Children:
						str += L" (Children)";
						break;
					case DVD_SP_EXT_CC_Normal:
						str += L" (CC)";
						break;
					case DVD_SP_EXT_CC_Big:
						str += L" (CC Big)";
						break;
					case DVD_SP_EXT_CC_Children:
						str += L" (CC Children)";
						break;
					case DVD_SP_EXT_Forced:
						str += L" (Forced)";
						break;
					case DVD_SP_EXT_DirectorComments_Normal:
						str += L" (Director Comments)";
						break;
					case DVD_SP_EXT_DirectorComments_Big:
						str += L" (Director Comments, Big)";
						break;
					case DVD_SP_EXT_DirectorComments_Children:
						str += L" (Director Comments, Children)";
						break;
				}
			}

			str.Replace(L"&", L"&&");
			submenu.AppendMenuW(flags, id++, str);
		}
	}
}

void CMainFrame::SetupSubtilesAMStreamSubMenu(CMenu& submenu, UINT id)
{
	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		if (m_pDVS) {
			CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

				bool fHideSubtitles = false;
				m_pDVS->get_HideSubtitles(&fHideSubtitles);
				submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED), ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
				submenu.AppendMenuW(MF_SEPARATOR);

				int subcount = GetStreamCount(SUBTITLE_GROUP);
				if (subcount) {
					int iSelectedLanguage = 0;
					m_pDVS->get_SelectedLanguage(&iSelectedLanguage);

					if (nLangs > 1) {
						for (int i = 0; i < nLangs - 1; i++) {
							WCHAR *pName = nullptr;
							m_pDVS->get_LanguageName(i, &pName);
							CString streamName(pName);
							CoTaskMemFree(pName);

							UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
							if (i == iSelectedLanguage) {
								flags |= MF_CHECKED | MFT_RADIOCHECK;
							}

							streamName.Replace(L"&", L"&&");
							submenu.AppendMenuW(flags, id++, streamName);
						}

						submenu.AppendMenuW(MF_SEPARATOR);
					}

					DWORD cStreams;
					pSS->Count(&cStreams);
					UINT baseid = id;

					for (int i = 0, j = cStreams; i < j; i++) {
						DWORD dwFlags  = DWORD_MAX;
						DWORD dwGroup  = DWORD_MAX;
						WCHAR* pszName = nullptr;

						if (SUCCEEDED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, &pszName, nullptr, nullptr)) && pszName) {
							if (dwGroup != SUBTITLE_GROUP) {
								CoTaskMemFree(pszName);
								continue;
							}

							CString name(pszName);
							CoTaskMemFree(pszName);

							UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
							if (dwFlags && iSelectedLanguage == (nLangs - 1)) {
								flags |= MF_CHECKED | MFT_RADIOCHECK;
							}

							name.Replace(L"&", L"&&");
							submenu.AppendMenuW(flags, id++, name);
						}
					}

					return;
				}

				int iSelectedLanguage = 0;
				m_pDVS->get_SelectedLanguage(&iSelectedLanguage);

				DWORD dwGroupPrev = DWORD_MAX;
				CComQIPtr<IAMStreamSelect> pDVS_SS = m_pDVS.p;

				for (int i = 0; i < nLangs; i++) {
					WCHAR *pName = nullptr;
					m_pDVS->get_LanguageName(i, &pName);
					CString streamName(pName);
					CoTaskMemFree(pName);

					UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
					if (i == iSelectedLanguage) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}

					if (pDVS_SS) {
						DWORD dwGroup = DWORD_MAX;
						if (SUCCEEDED(pDVS_SS->Info(i+1, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
							if (dwGroupPrev != DWORD_MAX && dwGroupPrev != dwGroup) {
								submenu.AppendMenuW(MF_SEPARATOR);
							}
							dwGroupPrev = dwGroup;
						}
					}

					streamName.Replace(L"&", L"&&");
					submenu.AppendMenuW(flags, id++, streamName);
				}
			}

			return;
		}

		submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | (m_pSubStreams.size() ? MF_ENABLED : MF_DISABLED), ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
		submenu.AppendMenuW(MF_SEPARATOR);

		bool sep = false;
		int splcnt = 0;
		UINT baseid = id;
		int intsub = 0;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
		if (pSS && !AfxGetAppSettings().fDisableInternalSubtitles) {
			DWORD cStreams;
			if (!FAILED(pSS->Count(&cStreams))) {

				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS.p;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
					bIsHaali = TRUE;
					cStreams--;
				}

				for (DWORD i = 0, j = cStreams; i < j; i++) {
					DWORD dwFlags  = DWORD_MAX;
					DWORD dwGroup  = DWORD_MAX;
					WCHAR* pszName = nullptr;

					if (SUCCEEDED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, &pszName, nullptr, nullptr)) && pszName) {
						if (dwGroup != SUBTITLE_GROUP) {
							CoTaskMemFree(pszName);
							continue;
						}

						CString name(pszName);
						CoTaskMemFree(pszName);

						UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
						if (dwFlags) {
							if (m_iSubtitleSel == 0) {
								flags |= MF_CHECKED | MFT_RADIOCHECK;
							}

						}

						name.Replace(L"&", L"&&");
						intsub++;
						submenu.AppendMenuW(flags, id++, name);
						splcnt++;
					}
				}
			}
		}

		auto it = m_pSubStreams.cbegin();
		int tPos = -1;
		if (splcnt > 0 && it != m_pSubStreams.cend()) {
			++it;
			tPos++;
		}

		while (it != m_pSubStreams.cend()) {
			CComPtr<ISubStream> pSubStream = *it++;
			if (!pSubStream) {
				continue;
			}
			//bool sep = false;

			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				tPos++;

				WCHAR* pName = nullptr;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, nullptr))) {
					CString name(pName);
					name.Replace(L"&", L"&&");

					if ((splcnt > 0 || (m_nInternalSubsCount == intsub && m_nInternalSubsCount != 0)) && !sep) {
						submenu.AppendMenuW(MF_SEPARATOR);
						sep = true;
					}

					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					if (m_nInternalSubsCount <= intsub) {
						name = L"* " + name;
					}
					submenu.AppendMenuW(flags, id++, name);
					intsub++;
					CoTaskMemFree(pName);
				} else {
					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					CString sname;
					if (m_nInternalSubsCount <= intsub) {
						sname = L"* " + ResStr(IDS_AG_UNKNOWN);
					}
					submenu.AppendMenuW(flags, id++, sname);
					intsub++;
				}
			}
		}

		if ((submenu.GetMenuItemCount() < 2) || (m_pSubStreams.empty() && submenu.GetMenuItemCount() <= 3)) {
			while (submenu.RemoveMenu(0, MF_BYPOSITION));
		}
	}
}

void CMainFrame::SetupVideoStreamsSubMenu()
{
	CMenu& submenu = m_VideoStreamsMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_VIDEO_STREAMS_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		SetupAMStreamSubMenu(submenu, id, 0);
	} else if (GetPlaybackMode() == PM_DVD) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		if (FAILED(m_pDVDI->GetCurrentAngle(&ulStreamsAvailable, &ulCurrentStream))) {
			return;
		}

		if (ulStreamsAvailable < 2) {
			return;    // one choice is not a choice...
		}

		for (ULONG i = 1; i <= ulStreamsAvailable; i++) {
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (i == ulCurrentStream) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}

			CString str;
			str.Format(ResStr(IDS_AG_ANGLE), i);

			submenu.AppendMenuW(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavChaptersSubMenu()
{
	CMenu& submenu = m_chaptersMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_NAVIGATE_CHAP_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		if (!m_BDPlaylists.empty()) {
			int mline = 1;
			for (const auto& Item : m_BDPlaylists) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (mline > MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					mline = 1;
				}
				mline++;

				if (Item.m_strFileName == m_strPlaybackRenderedPath) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				const CString time = L"[" + ReftimeToString2(Item.Duration()) + L"]";
				submenu.AppendMenuW(flags, id++, GetFileName(Item.m_strFileName) + '\t' + time);
			}
		} else if (m_youtubeUrllist.size() > 1) {
			for (size_t i = 0; i < m_youtubeUrllist.size(); i++) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

				if (m_youtubeUrllist[i].url == m_strPlaybackRenderedPath) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				if (i > 0) {
					const auto& prev_profile = m_youtubeUrllist[i - 1].profile;
					const auto& profile = m_youtubeUrllist[i].profile;

					if (prev_profile->type != Youtube::y_audio
							&& (prev_profile->format != profile->format || profile->type == Youtube::y_audio)) {
						if (m_youtubeUrllist.size() > 20 && profile->format == Youtube::y_mp4_av1) {
							flags |= MF_MENUBARBREAK;
						} else {
							submenu.AppendMenuW(MF_SEPARATOR);
						}
					}
				}

				submenu.AppendMenuW(flags, id++, m_youtubeUrllist[i].title);
			}
		}

		REFERENCE_TIME rt = GetPos();
		const DWORD curChap = m_pCB->ChapLookup(&rt, nullptr);

		if (m_pCB->ChapGetCount() > 1) {
			int mline = 1;
			for (DWORD i = 0; i < m_pCB->ChapGetCount(); i++, id++) { // change id here, because ChapGet can potentially fail
				rt = 0;
				CComBSTR bstr;
				if (FAILED(m_pCB->ChapGet(i, &rt, &bstr))) {
					continue;
				}

				CString name(bstr);
				name.Replace(L"&", L"&&");
				name.Replace('\t', ' ');

				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (i == curChap) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				if (mline > MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					mline = 1;
				}
				mline++;

				if (id != ID_NAVIGATE_CHAP_SUBITEM_START && i == 0) {
					//pSub->AppendMenuW(MF_SEPARATOR | MF_ENABLED);
					if (!m_BDPlaylists.empty() || m_youtubeUrllist.size() > 1) {
						flags |= MF_MENUBARBREAK;
					}
				}

				CString time = L"[" + ReftimeToString2(rt) + L"]";
				if (name.IsEmpty()) {
					name = time;
				} else {
					name += '\t' + time;
				}
				submenu.AppendMenuW(flags, id, name);
			}
		}

	} else if (GetPlaybackMode() == PM_DVD) {
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		m_pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		m_pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		ULONG ulUOPs = 0;
		m_pDVDI->GetCurrentUOPS(&ulUOPs);

		for (ULONG i = 1; i <= ulNumOfTitles; i++) {
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (i == Location.TitleNum) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			if (ulUOPs & UOP_FLAG_Play_Title) {
				flags |= MF_DISABLED | MF_GRAYED;
			}

			CString str;
			str.Format(ResStr(IDS_AG_TITLE), i);

			submenu.AppendMenuW(flags, id++, str);
		}

		for (ULONG i = 1; i <= ulNumOfChapters; i++) {
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (i == Location.ChapterNum) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			if (ulUOPs & UOP_FLAG_Play_Chapter) {
				flags |= MF_DISABLED | MF_GRAYED;
			}
			if (i == 1) {
				flags |= MF_MENUBARBREAK;
			}

			CString str;
			str.Format(ResStr(IDS_AG_CHAPTER), i);

			submenu.AppendMenuW(flags, id++, str);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1) {
		CAppSettings& s = AfxGetAppSettings();

		for (auto& Channel : s.m_DVBChannels) {
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

			if (Channel.GetPrefNumber() == s.nDVBLastChannel) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			submenu.AppendMenuW(flags, ID_NAVIGATE_CHAP_SUBITEM_START + Channel.GetPrefNumber(), Channel.GetName());
		}
	}
}

IBaseFilter* CMainFrame::FindSwitcherFilter()
{
	return FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB);
}

void CMainFrame::SetupAMStreamSubMenu(CMenu& submenu, UINT id, DWORD dwSelGroup)
{
	UINT baseid = id;

	CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
	if (!pSS) {
		pSS = m_pGB;
	}
	if (!pSS) {
		return;
	}

	DWORD cStreams;
	if (FAILED(pSS->Count(&cStreams))) {
		return;
	}

	DWORD dwPrevGroup = (DWORD)-1;

	for (int i = 0, j = cStreams; i < j; i++) {
		DWORD dwFlags  = DWORD_MAX;
		DWORD dwGroup  = DWORD_MAX;
		WCHAR* pszName = nullptr;

		if (SUCCEEDED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, &pszName, nullptr, nullptr)) && pszName) {
			if (dwGroup != dwSelGroup) {
				CoTaskMemFree(pszName);
				continue;
			}

			CString name(pszName);
			CoTaskMemFree(pszName);

			if (dwPrevGroup != -1 && dwPrevGroup != dwGroup) {
				submenu.AppendMenuW(MF_SEPARATOR);
			}
			dwPrevGroup = dwGroup;

			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (dwFlags) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}

			name.Replace(L"&", L"&&");
			submenu.AppendMenuW(flags, id++, name);
		}
	}
}

void CMainFrame::SelectAMStream(UINT id, DWORD dwSelGroup)
{
	CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
	if (!pSS) {
		pSS = m_pGB;
	}
	if (!pSS) {
		return;
	}

	DWORD cStreams;
	if (FAILED(pSS->Count(&cStreams))) {
		return;
	}

	for (int i = 0, j = cStreams; i < j; i++) {
		DWORD dwGroup = DWORD_MAX;

		if (FAILED(pSS->Info(i, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
			continue;
		}

		if (dwGroup != dwSelGroup) {
			continue;
		}

		if (id == 0) {
			pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);

			if (dwSelGroup == 2) {
				if (m_pCAP) {
					m_pCAP->Invalidate();
				}
			}
			break;
		}

		id--;
	}
}

void CMainFrame::SetupAudioTracksSubMenu()
{
	CMenu& submenu = m_AudioMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_AUDIO_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			bool sep = false;
			for (DWORD i = 0; i < cStreams; i++) {
				DWORD dwFlags = 0;
				CComHeapPtr<WCHAR> pszName;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, &pszName, nullptr, nullptr))) {
					return;
				}

				CString name(pszName);

				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (dwFlags) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				bool fExternal = false;
				CPlaylistItem pli;
				if (m_wndPlaylistBar.GetCur(pli)) {
					if (!pli.m_auds.empty()) {
						auto mainFileDir = GetFolderPath(pli.m_fi);
						for (const auto& fi : pli.m_auds) {
							auto& audioFile = fi.GetPath();
							if (!audioFile.IsEmpty() && name == audioFile) {
								auto audioFileDir = GetFolderPath(audioFile);
								if (!mainFileDir.IsEmpty() && audioFileDir != mainFileDir && StartsWith(name, mainFileDir.GetString())) {
									name.Delete(0, mainFileDir.GetLength());
								} else {
									name = GetFileName(name.GetString());
								}

								fExternal = true;
								break;
							}
						}
					} else if (name == pli.m_fi.GetPath()) {
						name = GetFileName(name.GetString());
					}
				}

				if (!sep && cStreams > 1 && fExternal) {
					if (submenu.GetMenuItemCount()) {
						submenu.AppendMenuW(MF_SEPARATOR | MF_ENABLED);
					}
					sep = true;
				}

				name.Replace(L"&", L"&&");
				submenu.AppendMenuW(flags, id++, name);
			}
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		if (FAILED(m_pDVDI->GetCurrentAudio(&ulStreamsAvailable, &ulCurrentStream))) {
			return;
		}

		LCID DefLanguage;
		DVD_AUDIO_LANG_EXT ext;
		if (FAILED(m_pDVDI->GetDefaultAudioLanguage(&DefLanguage, &ext))) {
			return;
		}

		for (ULONG i = 0; i < ulStreamsAvailable; i++) {
			LCID Language;
			if (FAILED(m_pDVDI->GetAudioLanguage(i, &Language))) {
				continue;
			}

			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (Language == DefLanguage) {
				flags |= MF_DEFAULT;
			}
			if (i == ulCurrentStream) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}

			CString str;
			if (Language) {
				int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(std::max(len-1, 0));
			} else {
				str.Format(ResStr(IDS_AG_UNKNOWN), i+1);
			}

			DVD_AudioAttributes ATR;
			if (SUCCEEDED(m_pDVDI->GetAudioAttributes(i, &ATR))) {
				switch (ATR.LanguageExtension) {
					case DVD_AUD_EXT_NotSpecified:
					default:
						break;
					case DVD_AUD_EXT_Captions:
						str += L" (Captions)";
						break;
					case DVD_AUD_EXT_VisuallyImpaired:
						str += L" (Visually Impaired)";
						break;
					case DVD_AUD_EXT_DirectorComments1:
						str += ResStr(IDS_MAINFRM_121);
						break;
					case DVD_AUD_EXT_DirectorComments2:
						str += ResStr(IDS_MAINFRM_122);
						break;
				}

				CString format = GetDVDAudioFormatName(ATR);

				if (!format.IsEmpty()) {
					str.Format(ResStr(IDS_MAINFRM_11),
							   CString(str),
							   format,
							   ATR.dwFrequency,
							   ATR.bQuantization,
							   ATR.bNumberOfChannels,
							   (ATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
				}
			}

			str.Replace(L"&", L"&&");

			submenu.AppendMenuW(flags, id++, str);
		}
	}
}

void CMainFrame::SetupRecentFilesSubMenu()
{
	CMenu& submenu = m_recentfilesMenu;
	MakeEmptySubMenu(submenu);

	CAppSettings& s = AfxGetAppSettings();

	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	UINT id = ID_RECENT_FILE_START;

	m_RecentPaths.clear();
	std::vector<SessionInfo> recentSessions;
	AfxGetMyApp()->m_HistoryFile.GetRecentSessions(recentSessions, s.iRecentFilesNumber);

	if (recentSessions.size()) {
		submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHOW_HISTORY, ResStr(IDS_SHOW_HISTORY));
		submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_RECENT_FILES_CLEAR, ResStr(IDS_RECENT_FILES_CLEAR));
		submenu.AppendMenuW(MF_SEPARATOR | MF_ENABLED);

		for (const auto& session : recentSessions) {
			m_RecentPaths.emplace_back(session.Path);

			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			CString str(session.Path);

			if (PathIsURLW(str)) {
				if (session.Title.GetLength() && Youtube::CheckURL(str)) {
					str.SetString(L"YouTube - " + session.Title);
					EllipsisText(str, 100);
				}
				else if (s.bRecentFilesShowUrlTitle && session.Title.GetLength()) {
					str.SetString(L"URL - " + session.Title);
					EllipsisText(str, 100);
				} else {
					EllipsisURL(str, 100);
				}
			}
			else {
				bool bPath = true;
				CStringW prefix;

				if (IsDVDStartFile(str)) {
					if (str.GetLength() == 24 && session.Title.GetLength() && str.Mid(1).CompareNoCase(L":\\VIDEO_TS\\VIDEO_TS.IFO") == 0) {
						WCHAR drive = str[0];
						str.Format(L"DVD - %c:\"%s\"", drive, session.Title);
						bPath = false;
					} else {
						str.Truncate(str.ReverseFind('\\'));
						prefix = L"DVD - ";
					}
				}
				else if (IsBDStartFile(str)) {
					str.Truncate(str.ReverseFind('\\'));
					if (str.Right(5).MakeUpper() == L"\\BDMV") {
						str.Truncate(str.GetLength() - 5);
					}
					prefix = L"Blu-ray - ";
				}
				else if (IsBDPlsFile(str)) {
					prefix = L"Blu-ray - ";
				}

				if (bPath && s.bRecentFilesMenuEllipsis) {
					EllipsisPath(str, 100);
				}
				str.Insert(0, prefix);
			}

			str.Replace(L"&", L"&&");
			submenu.AppendMenuW(flags, id, str);

			id++;
		}
	}
}

void CMainFrame::SetupFavoritesSubMenu()
{
	CMenu& submenu = m_favoritesMenu;
	MakeEmptySubMenu(submenu);

	CAppSettings& s = AfxGetAppSettings();

	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ADD, ResStr(IDS_FAVORITES_ADD));
	submenu.AppendMenuW(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ORGANIZE, ResStr(IDS_FAVORITES_ORGANIZE));

	UINT nLastGroupStart = submenu.GetMenuItemCount();

	const UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

	UINT id = ID_FAVORITES_FILE_START;
	AfxGetMyApp()->m_FavoritesFile.GetFavorites(m_FavFiles, m_FavDVDs);

	for (const auto& favFile : m_FavFiles) {
		CString favname = favFile.Title.GetLength() ? favFile.Title : favFile.Path;
		favname.Replace(L"&", L"&&");
		favname.Replace(L"\t", L" ");

		// pos
		CString posStr;
		if (favFile.Position > UNITS) {
			LONGLONG seconds = favFile.Position / UNITS;
			int h = (int)(seconds / 3600);
			int m = (int)(seconds / 60 % 60);
			int s = (int)(seconds % 60);
			posStr.Format(L"[%02d:%02d:%02d]", h, m, s);
		}

		// relative drive from path
		if (favFile.Path == L"?:\\") {
			posStr.Insert(0, L"[RD]");
		}

		if (!posStr.IsEmpty()) {
			favname.AppendFormat(L"\t%.14s", posStr);
		}
		submenu.AppendMenuW(flags, id, favname);

		id++;
	}

	if (id > ID_FAVORITES_FILE_START) {
		submenu.InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = submenu.GetMenuItemCount();

	id = ID_FAVORITES_DVD_START;

	for (const auto& favDvd : m_FavDVDs) {
		CString favname = favDvd.Title.GetLength() ? favDvd.Title : favDvd.Path;
		favname.Replace(L"&", L"&&");
		favname.Replace(L"\t", L" ");

		CString posStr;
		if (favDvd.DVDTitle) {
			posStr.Format(L"[%02u,%02u:%02u:%02u]",
				(unsigned)favDvd.DVDTitle,
				(unsigned)favDvd.DVDTimecode.bHours,
				(unsigned)favDvd.DVDTimecode.bMinutes,
				(unsigned)favDvd.DVDTimecode.bSeconds);
		}

		// relative drive from path
		if (favDvd.Path == L"?:\\") {
			posStr.Insert(0, L"[RD]");
		}

		if (!posStr.IsEmpty()) {
			favname.AppendFormat(L"\t%.14s", posStr);
		}
		submenu.AppendMenuW(flags, id, favname);

		id++;
	}

	if (id > ID_FAVORITES_DVD_START) {
		submenu.InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = submenu.GetMenuItemCount();
}

/////////////

void CMainFrame::ShowControls(int nCS, bool fSave)
{
	if (nCS == CS_NONE && !m_pLastBar && !fSave) {
		return;
	}

	if (nCS != CS_NONE && m_bFullScreen && m_bIsMPCVRExclusiveMode) {
		return;
	}

	auto& s = AfxGetAppSettings();
	int nCSprev = s.nCS;
	int hbefore = 0, hafter = 0;

	m_pLastBar = nullptr;

	int i = 1;
	for (const auto& pBar : m_bars) {
		const BOOL bShow = nCS & i;
		ShowControlBar(pBar, !!bShow, TRUE);
		if (bShow) {
			m_pLastBar = pBar;
		}

		CSize s = pBar->CalcFixedLayout(FALSE, TRUE);
		if (nCSprev & i) {
			hbefore += s.cy;
		}
		if (bShow) {
			hafter += s.cy;
		}

		i <<= 1;
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	if (wp.showCmd != SW_SHOWMAXIMIZED && !m_bFullScreen) {
		CRect r;
		GetWindowRect(r);
		MoveWindow(r.left, r.top, r.Width(), r.Height()+(hafter-hbefore));
	}

	if (fSave) {
		s.nCS = nCS;

		if (s.bUseDarkTheme) {
			OnTimer(TIMER_STREAMPOSPOLLER2);
			m_wndSeekBar.Invalidate();
		}
	}

	RecalcLayout();

	RepaintVideo();

	OnTimer(TIMER_STATS);
}

void CMainFrame::CalcControlsSize(CSize& cSize)
{
	cSize.SetSize(0, 0);

	for (const auto& pBar : m_bars) {
		if (IsWindow(pBar->m_hWnd) && pBar->IsVisible()) {
			cSize.cy += pBar->CalcFixedLayout(TRUE, TRUE).cy;
		}
	}

	for (const auto& pDockingBar : m_dockingbars) {
		if (IsWindow(pDockingBar->m_hWnd)) {
			BOOL bIsWindowVisible = pDockingBar->IsWindowVisible();
			if (auto* playlistBar = dynamic_cast<CPlayerPlaylistBar*>(pDockingBar)) {
				bIsWindowVisible = playlistBar->IsPlaylistVisible();
			}

			if (bIsWindowVisible && !pDockingBar->IsFloating()) {
				const auto bHorz = pDockingBar->IsHorzDocked();
				CSize size = pDockingBar->CalcFixedLayout(TRUE, bHorz);
				if (bHorz) {
					cSize.cy += (size.cy - 2);
				} else {
					cSize.cx += (cSize.cx ? size.cx - 2 : size.cx - 4);
				}
			}
		}
	}
}

void CMainFrame::SetAlwaysOnTop(int i)
{
	AfxGetAppSettings().iOnTop = i;

	BOOL bD3DOnMain = FALSE;
	if (IsD3DFullScreenMode()) {
		const auto mainMonitor = CMonitors::GetNearestMonitor(this);
		const auto fullScreenMonitor = CMonitors::GetNearestMonitor(m_pFullscreenWnd);

		bD3DOnMain = mainMonitor == fullScreenMonitor;
	}

	if (!m_bFullScreen && !bD3DOnMain) {
		const CWnd* pInsertAfter = nullptr;

		if (i == 0) {
			pInsertAfter = &wndNoTopMost;
		} else if (i == 1) {
			pInsertAfter = &wndTopMost;
		} else if (i == 2) {
			pInsertAfter = (GetMediaState() == State_Running) ? &wndTopMost : &wndNoTopMost;
		} else { // if (i == 3)
			pInsertAfter = (GetMediaState() == State_Running && !m_bAudioOnly) ? &wndTopMost : &wndNoTopMost;
		}

		if (pInsertAfter) {
			if (i >= 2 && pInsertAfter == &wndTopMost && !IsIconic()) {
				ShowWindow(SW_SHOWNA);
			}
			SetWindowPos(pInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	} else if (bD3DOnMain) {
		SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

ISubStream *InsertSubStream(std::list<CComPtr<ISubStream>>* subStreams, const CComPtr<ISubStream>& theSubStream)
{
	return subStreams->emplace_back(theSubStream).p;
}

void CMainFrame::AddTextPassThruFilter()
{
	BeginEnumFilters(m_pGB, pEF, pBF) {
		/*
		if (!IsSplitter(pBF)) {
			continue;
		}
		*/

		BeginEnumPins(pBF, pEP, pPin) {
			CComPtr<IPin> pPinTo;
			CMediaTypeEx mt;
			if (FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo
					|| FAILED(pPin->ConnectionMediaType(&mt))
					|| (!mt.ValidateSubtitle())) {
				continue;
			}

			// TextPassThruFilter can connect only to NullTextRenderer & ffdshow (video decoder/subtitles filter) - verify it.
			CPinInfo pi;
			if (SUCCEEDED(pPinTo->QueryPinInfo(&pi))) {
				CLSID clsid = GetCLSID(pi.pFilter);
				if (clsid != __uuidof(CNullTextRenderer)
						&& clsid != GUIDFromCString(L"{04FE9017-F873-410E-871E-AB91661A4EF7}") // ffdshow video decoder
						&& clsid != GUIDFromCString(L"{DBF9000E-F08C-4858-B769-C914A0FBB1D7}") // ffdshow subtitles filter
#if ENABLE_ASSFILTERMOD
						&& clsid != CLSID_AssFilterMod
#endif
				) {
					continue;
				}
			}

			CComQIPtr<IBaseFilter> pTPTF = DNew CTextPassThruFilter(this);
			CStringW name;
			name.Format(L"TextPassThru%08zx", pTPTF.p);
			if (FAILED(m_pGB->AddFilter(pTPTF, name))) {
				continue;
			}

			HRESULT hr;

			hr = pPinTo->Disconnect();
			hr = pPin->Disconnect();

			if (FAILED(hr = m_pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), nullptr))
					|| FAILED(hr = m_pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), pPinTo, nullptr))) {
				hr = m_pGB->ConnectDirect(pPin, pPinTo, nullptr);
			} else {
				InsertSubStream(&m_pSubStreams, CComQIPtr<ISubStream>(pTPTF));
			}
		}
		EndEnumPins;
	}
	EndEnumFilters;
}

void CMainFrame::ApplySubpicSettings()
{
	if (m_pCAP) {
		m_pCAP->SetSubpicSettings(&GetRenderersSettings().SubpicSets);
	}
}

void CMainFrame::ApplyExraRendererSettings()
{
	if (m_pCAP) {
		m_pCAP->SetExtraSettings(&GetRenderersSettings().ExtraSets);
	}
}

bool CMainFrame::LoadSubtitle(const CExtraFileItem& subItem, ISubStream **actualStream, bool bAutoLoad)
{
	CAppSettings& s = AfxGetAppSettings();

	if (!s.IsISRAutoLoadEnabled()) {
		return false;
	}

	CComPtr<ISubStream> pSubStream;
	auto& fname = subItem.GetPath();
	const CStringW ext = GetFileExt(fname).MakeLower();

	if (ext == L".mks" && s.IsISRAutoLoadEnabled()) {
		if (CComQIPtr<IGraphBuilderSub> pGBS = m_pGB.p) {
			HRESULT hr = pGBS->RenderSubFile(fname);
			if (SUCCEEDED(hr)) {
				size_t c = m_pSubStreams.size();
				AddTextPassThruFilter();

				if (m_pSubStreams.size() > c) {
					if (actualStream != nullptr) {
						*actualStream = m_pSubStreams.back();
						s.fEnableSubtitles = true;
					}
					if (!bAutoLoad) {
						m_wndPlaylistBar.AddSubtitleToCurrent(fname);
					}

					return true;
				}

				return false;
			}
		}
	}

	// TMP: maybe this will catch something for those who get a runtime error dialog when opening subtitles from cds
	try {
		CString videoName;
		if (GetPlaybackMode() == PM_FILE) {
			videoName = GetCurFileName();
		}

		if (!pSubStream && ext == L".sup") {
			std::unique_ptr<CRenderedHdmvSubtitle> pRHS(DNew CRenderedHdmvSubtitle(&m_csSubLock));
			if (pRHS->Open(fname, subItem.GetTitle(), videoName)) {
				pSubStream = pRHS.release();
			}
		}

		if (!pSubStream && ext == L".idx") {
			std::unique_ptr<CVobSubFile> pVSF(DNew CVobSubFile(&m_csSubLock));
			if (pVSF->Open(fname) && pVSF->GetStreamCount() > 0) {
				pSubStream = pVSF.release();
			}
		}

		if (!pSubStream) {
			std::unique_ptr<CRenderedTextSubtitle> pRTS(DNew CRenderedTextSubtitle(&m_csSubLock));
			if (pRTS->Open(fname, DEFAULT_CHARSET, subItem.GetTitle(), videoName) && pRTS->GetStreamCount() > 0) {
				pSubStream = pRTS.release();
			}
		}
	} catch (CException* e) {
		e->Delete();
	}

	if (pSubStream) {
		ISubStream *r = InsertSubStream(&m_pSubStreams, pSubStream);
		m_ExternalSubstreams.emplace_back(r);
		if (actualStream != nullptr) {
			*actualStream = r;

			s.fEnableSubtitles = true;
		}

		if (!bAutoLoad) {
			m_wndPlaylistBar.AddSubtitleToCurrent(fname);
		}

		if (subChangeNotifyThread.joinable() && !::PathIsURLW(fname)) {
			auto it = std::find_if(m_ExtSubFiles.cbegin(), m_ExtSubFiles.cend(), [&fname](filepathtime_t fpt) { return fpt.path == fname; });
			if (it == m_ExtSubFiles.cend()) {
				m_ExtSubFiles.emplace_back(fname, std::filesystem::last_write_time(fname.GetString()));
			}

			const CString path = GetFolderPath(fname);
			if (!Contains(m_ExtSubPaths, path)) {
				m_ExtSubPaths.emplace_back(path);
				m_EventSubChangeRefreshNotify.Set();
			}
		}
	}

	return !!pSubStream;
}

void CMainFrame::UpdateSubtitle(bool fDisplayMessage, bool fApplyDefStyle)
{
	if (!m_pCAP) {
		return;
	}

	int i = m_iSubtitleSel;

	auto it = m_pSubStreams.cbegin();
	while (it != m_pSubStreams.cend() && i >= 0) {
		CComPtr<ISubStream> pSubStream = *it++;

		if (i < pSubStream->GetStreamCount()) {
			SetSubtitle(pSubStream, i, fApplyDefStyle);

			if (fDisplayMessage) {
				WCHAR* pName = nullptr;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, nullptr))) {
					CString strMessage;
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), pName);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}
			return;
		}

		i -= pSubStream->GetStreamCount();
	}

	if (fDisplayMessage && m_iSubtitleSel < 0) {
		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_SUBTITLE_STREAM_OFF));
	}

	m_pCAP->SetSubPicProvider(nullptr);
}

void CMainFrame::SetSubtitle(ISubStream* pSubStream, int iSubtitleSel/* = -1*/, bool fApplyDefStyle/* = false*/)
{
	CAppSettings& s = AfxGetAppSettings();

	s.m_VRSettings.SubpicSets.iPosRelative = 0;
	s.m_VRSettings.SubpicSets.ShiftPos = {0, 0};

	{
		CAutoLock cAutoLock(&m_csSubLock);

		if (pSubStream) {
			CLSID clsid;
			pSubStream->GetClassID(&clsid);

			if (clsid == __uuidof(CVobSubFile) || clsid == __uuidof(CVobSubStream)) {
				if (auto pVSS = dynamic_cast<CVobSubSettings*>(pSubStream)) {
					pVSS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos);
				}
			} else if (clsid == __uuidof(CRenderedTextSubtitle)) {
				CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)pSubStream;

				STSStyle style = s.subdefstyle;

				if (fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle) {
					pRTS->SetDefaultStyle(style);
				}

				if (pRTS->GetDefaultStyle(style) && style.relativeTo == STSStyle::AUTO) {
					style.relativeTo = s.subdefstyle.relativeTo;
					pRTS->SetDefaultStyle(style);
				}

				pRTS->SetOverride(s.fUseDefaultSubtitlesStyle, s.subdefstyle);
				pRTS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos);

				if (m_pCAP && s.fKeepAspectRatio && pRTS->m_path.IsEmpty() && pRTS->m_dstScreenSizeActual) {
					CSize szAspectRatio = m_pCAP->GetVideoSizeAR();

					if (szAspectRatio.cx && szAspectRatio.cy && pRTS->m_dstScreenSize.cx && pRTS->m_dstScreenSize.cy) {
						pRTS->m_ePARCompensationType = CSimpleTextSubtitle::EPARCompensationType::EPCTAccurateSize_ISR;
						pRTS->m_dPARCompensation = ((double)szAspectRatio.cx / szAspectRatio.cy) /
													((double)pRTS->m_dstScreenSize.cx / pRTS->m_dstScreenSize.cy);
					} else {
						pRTS->m_ePARCompensationType = CSimpleTextSubtitle::EPARCompensationType::EPCTDisabled;
						pRTS->m_dPARCompensation = 1.0;
					}
				}

				pRTS->Deinit();
			} else if (clsid == __uuidof(CRenderedHdmvSubtitle)) {
				s.m_VRSettings.SubpicSets.iPosRelative = s.subdefstyle.relativeTo;
			}

			CComQIPtr<ISubRenderOptions> pSRO = m_pCAP.p;

			LPWSTR yuvMatrixString = nullptr;
			int nLen;
			if (m_pMVRI) {
				m_pMVRI->GetString("yuvMatrix", &yuvMatrixString, &nLen);
			} else if (pSRO) {
				pSRO->GetString("yuvMatrix", &yuvMatrixString, &nLen);
			}

			CString yuvMatrix(yuvMatrixString);
			LocalFree(yuvMatrixString);

			CString inputRange(L"TV"), outpuRange(L"PC");
			if (!yuvMatrix.IsEmpty()) {
				int nPos = 0;
				inputRange = yuvMatrix.Tokenize(L".", nPos);
				yuvMatrix = yuvMatrix.Mid(nPos);
				if (yuvMatrix != L"601") {
					yuvMatrix = L"709";
				}
			} else {
				yuvMatrix = L"709";
			}

			if (m_pMVRS) {
				int targetWhiteLevel = 255;
				m_pMVRS->SettingsGetInteger(L"White", &targetWhiteLevel);
				if (targetWhiteLevel < 245) {
					outpuRange = L"TV";
				}
			} else if (pSRO) {
				int range = 0;
				pSRO->GetInt("supportedLevels", &range);
				if (range == 3) {
					outpuRange = L"TV";
				}
			}

			pSubStream->SetSourceTargetInfo(yuvMatrix, inputRange, outpuRange);
		}

		if (!fApplyDefStyle && iSubtitleSel == -1) {
			m_iSubtitleSel = iSubtitleSel;
			if (pSubStream) {
				int i = 0;

				for (const auto& pSubStream2 : m_pSubStreams) {
					if (pSubStream == pSubStream2) {
						m_iSubtitleSel = i + pSubStream2->GetStream();
						break;
					}
					i += pSubStream2->GetStreamCount();
				}
			}
		}

		{
			int i = m_iSubtitleSel;
			auto it = m_pSubStreams.cbegin();
			while (it != m_pSubStreams.cend() && i >= 0) {
				CComPtr<ISubStream> pSubStream = *it++;

				if (i < pSubStream->GetStreamCount()) {
					CAutoLock cAutoLock(&m_csSubLock);
					pSubStream->SetStream(i);
					break;
				}

				i -= pSubStream->GetStreamCount();
			}
		}

		m_pCurrentSubStream = pSubStream;
	}

	if (m_pCAP) {
		m_pCAP->SetSubpicSettings(&s.m_VRSettings.SubpicSets);

		g_bExternalSubtitle = (std::find(m_ExternalSubstreams.cbegin(), m_ExternalSubstreams.cend(), pSubStream) != m_ExternalSubstreams.cend());
		m_pCAP->SetSubPicProvider(CComQIPtr<ISubPicProvider>(pSubStream));
		if (s.fUseSybresync) {
			CAutoLock cAutoLock(&m_csSubLock);
			m_wndSubresyncBar.SetSubtitle(pSubStream, m_pCAP->GetFPS());
		}
	}
}

void CMainFrame::ReplaceSubtitle(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew)
{
	for (auto& pSubStream : m_pSubStreams) {
		if (pSubStreamOld == pSubStream.p) {
			pSubStream = pSubStreamNew;
			UpdateSubtitle();
			break;
		}
	}
}

void CMainFrame::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
	if (m_pCAP) {
		if (nSubtitleId == -1 || nSubtitleId == (DWORD_PTR)m_pCurrentSubStream.p) {
			m_pCAP->Invalidate(rtInvalidate);
		}
	}
}

void CMainFrame::ReloadSubtitle()
{
	{
		CAutoLock cAutoLock(&m_csSubLock);

		for (const auto& pSubStream : m_pSubStreams) {
			pSubStream->Reload();
		}
	}

	UpdateSubtitle();

	if (AfxGetAppSettings().fUseSybresync) {
		m_wndSubresyncBar.ReloadSubtitle();
	}
}

void CMainFrame::ToggleSubtitleOnOff(bool bDisplayMessage/* = false*/)
{
	if (GetPlaybackMode() == PM_DVD && m_pDVDI) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			m_pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, nullptr);
		}
	} else if (m_pDVS) {
		bool bHideSubtitles = false;
		m_pDVS->get_HideSubtitles(&bHideSubtitles);
		bHideSubtitles = !bHideSubtitles;
		m_pDVS->put_HideSubtitles(bHideSubtitles);

		if (bDisplayMessage) {
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
				if (bHideSubtitles) {
					m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_SUBTITLE_STREAM_OFF));
				} else {
					int iSelected = 0;
					m_pDVS->get_SelectedLanguage(&iSelected);
					WCHAR* pName = nullptr;
					m_pDVS->get_LanguageName(iSelected, &pName);

					CString	strMessage;
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), pName);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
					CoTaskMemFree(pName);
				}
			}
		}
	} else {
		auto& s = AfxGetAppSettings();
		s.fEnableSubtitles = !s.fEnableSubtitles;

		if (s.fEnableSubtitles) {
			m_iSubtitleSel = m_nSelSub2;
		} else {
			m_nSelSub2 = m_iSubtitleSel;
			m_iSubtitleSel = -1;
		}

		UpdateSubtitle(bDisplayMessage);
	}
}

void CMainFrame::UpdateSubDefaultStyle()
{
	CAppSettings& s = AfxGetAppSettings();
	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>(m_pCurrentSubStream.p)) {
		{
			CAutoLock cAutoLock(&m_csSubLock);

			pRTS->SetOverride(s.fUseDefaultSubtitlesStyle, s.subdefstyle);
			pRTS->Deinit();
		}
		InvalidateSubtitle();
		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	} else if (dynamic_cast<CRenderedHdmvSubtitle*>(m_pCurrentSubStream.p)) {
		s.m_VRSettings.SubpicSets.iPosRelative = s.subdefstyle.relativeTo;
		if (m_pCAP) {
			m_pCAP->SetSubpicSettings(&s.m_VRSettings.SubpicSets);
		}
		InvalidateSubtitle();
		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	}
}

void CMainFrame::SetAudioTrackIdx(int index)
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			pSS->Enable(index, AMSTREAMSELECTENABLE_ENABLE);
		}
	}
}

void CMainFrame::SetSubtitleTrackIdx(int index)
{
	if (m_eMediaLoadState == MLS_LOADING || m_eMediaLoadState == MLS_LOADED) {
		if (index != -1) {
			AfxGetAppSettings().fEnableSubtitles = true;
		}
		SelectSubtilesAMStream(index);
	}
}

int CMainFrame::GetAudioTrackIdx()
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			for (DWORD i = 0; i < cStreams; i++) {
				DWORD dwFlags = 0;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, nullptr, nullptr, nullptr))) {
					return -1;
				}
				if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
					return (int)i;
				}
			}
		}
	}

	return -1;
}

int CMainFrame::GetSubtitleTrackIdx()
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE
			&& !(m_iSubtitleSel & 0x80000000)) {
		int subStreams = 0;

		if (!AfxGetAppSettings().fDisableInternalSubtitles) {
			CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
			DWORD cStreams = 0;
			if (pSS && SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0) {
				CLSID clsid = GetCLSID(m_pMainSourceFilter);
				if (clsid == CLSID_HaaliSplitterAR || clsid == CLSID_HaaliSplitter) {
					cStreams--;
				}

				for (DWORD i = 0; i < cStreams; i++) {
					DWORD dwFlags = DWORD_MAX;
					DWORD dwGroup = DWORD_MAX;
					if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, nullptr, nullptr, nullptr))) {
						return -1;
					}

					if (dwGroup == 2) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
							if (!m_iSubtitleSel) {
								return subStreams;
							}
						}
						subStreams++;
					}
				}

				if (subStreams) {
					subStreams--;
				}
			}
		}

		return m_iSubtitleSel + subStreams;
	}

	return -1;
}

REFERENCE_TIME CMainFrame::GetPos()
{
	return (m_eMediaLoadState == MLS_LOADED ? m_wndSeekBar.GetPos() : 0);
}

REFERENCE_TIME CMainFrame::GetDur()
{
	return (m_eMediaLoadState == MLS_LOADED ? m_wndSeekBar.GetRange() : 0);
}

bool CMainFrame::GetNeighbouringKeyFrames(REFERENCE_TIME rtTarget, std::pair<REFERENCE_TIME, REFERENCE_TIME>& keyframes) const
{
	bool ret = false;
	REFERENCE_TIME rtLower, rtUpper;
	if (!m_kfs.empty()) {
		const auto cbegin = m_kfs.cbegin();
		const auto cend = m_kfs.cend();
		ASSERT(std::is_sorted(cbegin, cend));
		auto upper = std::upper_bound(cbegin, cend, rtTarget);
		if (upper == cbegin) {
			// we assume that streams always start with keyframe
			rtLower = *cbegin;
			rtUpper = (++upper != cend) ? *upper : rtLower;
		} else if (upper == cend) {
			rtLower = rtUpper = *(--upper);
		} else {
			rtUpper = *upper;
			rtLower = *(--upper);
		}
		ret = true;
	} else {
		rtLower = rtUpper = rtTarget;
	}
	keyframes = std::make_pair(rtLower, rtUpper);

	return ret;
}

void CMainFrame::LoadKeyFrames()
{
	UINT nKFs = 0;
	m_kfs.clear();
	if (m_pKFI && S_OK == m_pKFI->GetKeyFrameCount(nKFs) && nKFs > 1) {
		UINT k = nKFs;
		m_kfs.resize(k);
		if (FAILED(m_pKFI->GetKeyFrames(&TIME_FORMAT_MEDIA_TIME, m_kfs.data(), k)) || k != nKFs) {
			m_kfs.clear();
		}
	}
}

REFERENCE_TIME const CMainFrame::GetClosestKeyFrame(REFERENCE_TIME rtTarget)
{
	REFERENCE_TIME rtDuration = m_wndSeekBar.GetRange();
	if (rtDuration && rtTarget >= rtDuration) {
		return rtDuration;
	}

	MatroskaLoadKeyFrames();

	REFERENCE_TIME ret = rtTarget;
	std::pair<REFERENCE_TIME, REFERENCE_TIME> keyframes;
	if (GetNeighbouringKeyFrames(rtTarget, keyframes)) {
		ret = (rtTarget - keyframes.first < keyframes.second - rtTarget) ? keyframes.first : keyframes.second;
	}

	return ret;
}

void CMainFrame::SeekTo(REFERENCE_TIME rtPos, bool bShowOSD/* = true*/)
{
	const OAFilterState fs = GetMediaState();

	if (rtPos < 0) {
		rtPos = 0;
	}
	REFERENCE_TIME stop = 0;

	if (GetPlaybackMode() != PM_CAPTURE) {
		stop = m_wndSeekBar.GetRange();
		if (rtPos > stop) {
			rtPos = stop;
		}
		const bool bShowMilliSecs = m_bShowMilliSecs || m_wndSubresyncBar.IsWindowVisible();
		m_wndStatusBar.SetStatusTimer(rtPos, stop, bShowMilliSecs, GetTimeFormat());
		if (bShowOSD && stop > 0 && AfxGetAppSettings().ShowOSD.SeekTime) {
			m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 1500);
		}
	}

	if (GetPlaybackMode() == PM_FILE) {
		REFERENCE_TIME total = 0;
		if (m_pMS) {
			m_pMS->GetDuration(&total);
		}
		if (!total) {
			return;
		}

		if (fs == State_Stopped) {
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}

		if (!ValidateSeek(rtPos, stop)) {
			return;
		}

		m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);

		if (m_bUseReclock) {
			// Crazy ReClock require delay between seek operation or causes crash in EVR :)
			Sleep(50);
		}
	} else if (GetPlaybackMode() == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title) {
		DVD_HMSF_TIMECODE tc = RT2HMSF(rtPos);
		m_pDVDC->PlayAtTime(&tc, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);

		if (fs == State_Paused) {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			Sleep(100);
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		//DLog(L"Warning (CMainFrame::SeekTo): Trying to seek in capture mode");
	}

	m_bEndOfStream = false;
	m_bGraphEventComplete = false;
	m_nStepForwardCount = 0;

	if (m_abRepeatPositionAEnabled && rtPos < m_abRepeatPositionA ||
		m_abRepeatPositionBEnabled && rtPos > m_abRepeatPositionB)
	{
		DisableABRepeat();
	}

	OnTimer(TIMER_STREAMPOSPOLLER);
	OnTimer(TIMER_STREAMPOSPOLLER2);
	if (fs == State_Paused) {
		OnTimer(TIMER_STATS);
	}

	SendCurrentPositionToApi(true);
}

bool CMainFrame::ValidateSeek(REFERENCE_TIME rtPos, REFERENCE_TIME rtStop)
{
	// Disable seeking while buffering data if the seeking position is more than a loading progress
	if (m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_FILE && (rtPos > 0) && (rtStop > 0) && (rtPos <= rtStop)) {
		if (m_pAMOP) {
			__int64 t = 0, c = 0;
			if (SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t) {
				int Query_percent = llMulDiv(c, 100, t, 0);
				int Seek_percent  = llMulDiv(rtPos, 100, rtStop, 0);
				if (Seek_percent > Query_percent) {
					return false;
				}
			}
		}

		if (m_bBuffering) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF.p) {
					long BufferingProgress = 0;
					if (SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && (BufferingProgress > 0 && BufferingProgress < 100)) {
						int Seek_percent = llMulDiv(rtPos, 100, rtStop, 0);
						if (Seek_percent > BufferingProgress) {
							return false;
						}

					}
					break;
				}
			}
			EndEnumFilters;
		}
	}

	return true;
}

void CMainFrame::MatroskaLoadKeyFrames()
{
	if (m_pKFI && m_kfs.empty()) {
		if (CComQIPtr<IMatroskaSplitterFilter> pMatroskaSplitterFilter = m_pKFI.p) {
			LoadKeyFrames();
		}
	}
}

bool CMainFrame::GetBufferingProgress(int* iProgress/* = nullptr*/)
{
	if (iProgress) {
		*iProgress = 0;
	}

	int Progress = 0;
	if (m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_FILE) {
		if (m_pAMOP) {
			__int64 t = 0, c = 0;
			if (SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t) {
				Progress = llMulDiv(c, 100, t, 0);
			}
		}

		if (m_bBuffering) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF.p) {
					long BufferingProgress = 0;
					if (SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && (BufferingProgress > 0 && BufferingProgress < 100)) {
						Progress = BufferingProgress;
					}
					break;
				}
			}
			EndEnumFilters;
		}

		if (Progress > 0 && Progress < 100) {
			if (iProgress) {
				*iProgress = Progress;
			}
			return true;
		}
	}

	return false;
}

void CMainFrame::CleanGraph()
{
	if (!m_pGB) {
		return;
	}

	BeginEnumFilters(m_pGB, pEF, pBF) {
		CComQIPtr<IAMFilterMiscFlags> pAMMF(pBF);
		if (pAMMF && (pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE)) {
			continue;
		}

		// some capture filters forget to set AM_FILTER_MISC_FLAGS_IS_SOURCE
		// or to implement the IAMFilterMiscFlags interface
		if (pBF == m_pVidCap || pBF == m_pAudCap) {
			continue;
		}

		if (CComQIPtr<IFileSourceFilter>(pBF)) {
			continue;
		}

		if (GetCLSID(pBF) == CLSID_XySubFilter) {
			continue;
		}

		int nIn, nOut, nInC, nOutC;
		if (CountPins(pBF, nIn, nOut, nInC, nOutC) > 0 && (nInC + nOutC) == 0) {
			DLog(L"Removing from graph : %s", GetFilterName(pBF));

			m_pGB->RemoveFilter(pBF);
			pEF->Reset();
		}
	}
	EndEnumFilters;
}

#define AUDIOBUFFERLEN 500

static void SetLatency(IBaseFilter* pBF, int cbBuffer)
{
	BeginEnumPins(pBF, pEP, pPin) {
		if (CComQIPtr<IAMBufferNegotiation> pAMBN = pPin.p) {
			ALLOCATOR_PROPERTIES ap;
			ap.cbAlign = -1;  // -1 means no preference.
			ap.cbBuffer = cbBuffer;
			ap.cbPrefix = -1;
			ap.cBuffers = -1;
			pAMBN->SuggestAllocatorProperties(&ap);
		}
	}
	EndEnumPins;
}

HRESULT CMainFrame::BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt)
{
	IBaseFilter* pBuff = pBF[0];
	IBaseFilter* pEnc = pBF[1];
	IBaseFilter* pMux = pBF[2];

	if (!pPin || !pMux) {
		return E_FAIL;
	}

	CString err;

	HRESULT hr = S_OK;

	CFilterInfo fi;
	if (FAILED(pMux->QueryFilterInfo(&fi)) || !fi.pGraph) {
		m_pGB->AddFilter(pMux, L"Multiplexer");
	}

	CStringW prefix;
	CString type;
	if (majortype == MEDIATYPE_Video) {
		prefix = L"Video ";
		type = ResStr(IDS_CAPTURE_ERROR_VIDEO);
	} else if (majortype == MEDIATYPE_Audio) {
		prefix = L"Audio ";
		type = ResStr(IDS_CAPTURE_ERROR_AUDIO);
	}

	if (pBuff) {
		hr = m_pGB->AddFilter(pBuff, prefix + L"Buffer");
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_ADD_BUFFER), type);
			MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		hr = m_pGB->ConnectFilter(pPin, pBuff);
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_CONNECT_BUFF), type);
			MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		pPin = GetFirstPin(pBuff, PINDIR_OUTPUT);
	}

	if (pEnc) {
		hr = m_pGB->AddFilter(pEnc, prefix + L"Encoder");
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_ADD_ENCODER), type);
			MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		hr = m_pGB->ConnectFilter(pPin, pEnc);
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_CONNECT_ENC), type);
			MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		pPin = GetFirstPin(pEnc, PINDIR_OUTPUT);

		if (CComQIPtr<IAMStreamConfig> pAMSC = pPin) {
			if (pmt->majortype == majortype) {
				hr = pAMSC->SetFormat(pmt);
				if (FAILED(hr)) {
					err.Format(ResStr(IDS_CAPTURE_ERROR_COMPRESSION), type);
					MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
					return hr;
				}
			}
		}

	}

	//if (pMux)
	{
		hr = m_pGB->ConnectFilter(pPin, pMux);
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_MULTIPLEXER), type);
			MessageBoxW(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}
	}

	CleanGraph();

	return S_OK;
}

bool CMainFrame::BuildToCapturePreviewPin(
	IBaseFilter* pVidCap, IPin** ppVidCapPin, IPin** ppVidPrevPin,
	IBaseFilter* pAudCap, IPin** ppAudCapPin, IPin** ppAudPrevPin)
{
	HRESULT hr;

	*ppVidCapPin = *ppVidPrevPin = nullptr;
	*ppAudCapPin = *ppAudPrevPin = nullptr;

	CComPtr<IPin> pDVAudPin;

	if (pVidCap) {
		CComPtr<IPin> pPin;
		if (!pAudCap // only look for interleaved stream when we don't use any other audio capture source
				&& SUCCEEDED(m_pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, TRUE, 0, &pPin))) {
			CComPtr<IBaseFilter> pDVSplitter;
			hr = pDVSplitter.CoCreateInstance(CLSID_DVSplitter);
			hr = m_pGB->AddFilter(pDVSplitter, L"DV Splitter");

			hr = m_pCGB->RenderStream(nullptr, &MEDIATYPE_Interleaved, pPin, nullptr, pDVSplitter);

			pPin.Release();
			hr = m_pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Video, TRUE, 0, &pPin);
			hr = m_pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Audio, TRUE, 0, &pDVAudPin);

			CComPtr<IBaseFilter> pDVDec;
			hr = pDVDec.CoCreateInstance(CLSID_DVVideoCodec);
			hr = m_pGB->AddFilter(pDVDec, L"DV Video Decoder");

			hr = m_pGB->ConnectFilter(pPin, pDVDec);

			pPin.Release();
			hr = m_pCGB->FindPin(pDVDec, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Video, TRUE, 0, &pPin);
		} else if (FAILED(m_pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, TRUE, 0, &pPin))) {
			MessageBoxW(ResStr(IDS_CAPTURE_ERROR_VID_CAPT_PIN), ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return false;
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = m_pGB->AddFilter(pSmartTee, L"Smart Tee (video)");

		hr = m_pGB->ConnectFilter(pPin, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppVidPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppVidCapPin);
	}

	if (pAudCap || pDVAudPin) {
		CComPtr<IPin> pPin;
		if (pDVAudPin) {
			pPin = pDVAudPin;
		} else if (FAILED(m_pCGB->FindPin(pAudCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, TRUE, 0, &pPin))) {
			MessageBoxW(ResStr(IDS_CAPTURE_ERROR_AUD_CAPT_PIN), ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return false;
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = m_pGB->AddFilter(pSmartTee, L"Smart Tee (audio)");

		hr = m_pGB->ConnectFilter(pPin, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppAudPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppAudCapPin);
	}

	return true;
}

bool CMainFrame::BuildGraphVideoAudio(int fVPreview, bool fVCapture, int fAPreview, bool fACapture)
{
	if (!m_pCGB) {
		return false;
	}

	OAFilterState fs = GetMediaState();

	if (fs != State_Stopped) {
		SendMessageW(WM_COMMAND, ID_PLAY_STOP);
	}

	HRESULT hr;

	m_pGB->NukeDownstream(m_pVidCap);
	m_pGB->NukeDownstream(m_pAudCap);

	CleanGraph();

	if (m_pAMVSCCap) {
		hr = m_pAMVSCCap->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	}
	if (m_pAMVSCPrev) {
		hr = m_pAMVSCPrev->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	}
	if (m_pAMASC) {
		hr = m_pAMASC->SetFormat(&m_wndCaptureBar.m_capdlg.m_mta);
	}

	CComPtr<IBaseFilter> pVidBuffer = m_wndCaptureBar.m_capdlg.m_pVidBuffer;
	CComPtr<IBaseFilter> pAudBuffer = m_wndCaptureBar.m_capdlg.m_pAudBuffer;
	CComPtr<IBaseFilter> pVidEnc    = m_wndCaptureBar.m_capdlg.m_pVidEnc;
	CComPtr<IBaseFilter> pAudEnc    = m_wndCaptureBar.m_capdlg.m_pAudEnc;
	CComPtr<IBaseFilter> pMux       = m_wndCaptureBar.m_capdlg.m_pMux;
	CComPtr<IBaseFilter> pDst       = m_wndCaptureBar.m_capdlg.m_pDst;
	CComPtr<IBaseFilter> pAudMux    = m_wndCaptureBar.m_capdlg.m_pAudMux;
	CComPtr<IBaseFilter> pAudDst    = m_wndCaptureBar.m_capdlg.m_pAudDst;

	bool fFileOutput = (pMux && pDst) || (pAudMux && pAudDst);
	bool fCapture = (fVCapture || fACapture);

	if (m_pAudCap) {
		AM_MEDIA_TYPE* pmt = &m_wndCaptureBar.m_capdlg.m_mta;
		int ms = (fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput) ? AUDIOBUFFERLEN : 60;
		if (pMux != pAudMux && fACapture) {
			SetLatency(m_pAudCap, -1);
		} else if (pmt->pbFormat) {
			SetLatency(m_pAudCap, ((WAVEFORMATEX*)pmt->pbFormat)->nAvgBytesPerSec * ms / 1000);
		}
	}

	CComPtr<IPin> pVidCapPin, pVidPrevPin, pAudCapPin, pAudPrevPin;
	BuildToCapturePreviewPin(m_pVidCap, &pVidCapPin, &pVidPrevPin, m_pAudCap, &pAudCapPin, &pAudPrevPin);


	bool fVidPrev = pVidPrevPin && fVPreview;
	bool fVidCap = pVidCapPin && fVCapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fVidOutput;

	if (fVPreview == 2 && !fVidCap && pVidCapPin) {
		pVidPrevPin = pVidCapPin;
		pVidCapPin.Release();
	}

	bool fAudPrev = pAudPrevPin && fAPreview;
	bool fAudCap = pAudCapPin && fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput;

	if (fAPreview == 2 && !fAudCap && pAudCapPin) {
		pAudPrevPin = pAudCapPin;
		pAudCapPin.Release();
	}

	// Preview Video
	if (fVidPrev) {
		CComPtr<IVMRMixerBitmap9>    pVMB;
		CComPtr<IMFVideoMixerBitmap> pMFVMB;
		CComPtr<IMadVRTextOsd>       pMVTO;

		m_pMVRS.Release();
		m_pMVRSR.Release();

		m_OSD.Stop();
		m_pCAP.Release();
		m_clsidCAP = GUID_NULL;
		m_pVMRWC.Release();
		m_pVMRMC9.Release();
		m_pMFVP.Release();
		m_pMFVDC.Release();
		m_pQP.Release();

		m_pGB->Render(pVidPrevPin);

		m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
		if (m_pCAP) {
			m_clsidCAP = m_pCAP->GetAPCLSID();
		}
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRWC), FALSE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRMC9), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&pVMB), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&pMFVMB), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVDC), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVP), TRUE);
		pMVTO = m_pCAP;
		m_pMVRSR = m_pCAP;
		m_pMVRS = m_pCAP;

		const CAppSettings& s = AfxGetAppSettings();
		m_pVideoWnd = &m_wndView;

		if (m_pMFVDC) {
			m_pMFVDC->SetVideoWindow(m_pVideoWnd->m_hWnd);
		}
		else if (m_pVMRWC) {
			m_pVMRWC->SetVideoClippingWindow(m_pVideoWnd->m_hWnd);
		}

		if (s.ShowOSD.Enable || s.bShowDebugInfo) {
			if (pMFVMB) {
				m_OSD.Start(m_pVideoWnd, pMFVMB);
			}
			else if (pMVTO) {
				m_OSD.Start(m_pVideoWnd, pMVTO);
			}
		}
	}

	// Preview Audio (render audio before adding muxer)
	if (fAudPrev) {
		m_pGB->Render(pAudPrevPin);
	}

	// Capture Video
	if (fVidCap) {
		IBaseFilter* pBF[3] = {pVidBuffer, pVidEnc, pMux};
		HRESULT hr = BuildCapture(pVidCapPin, pBF, MEDIATYPE_Video, &m_wndCaptureBar.m_capdlg.m_mtcv);
		UNREFERENCED_PARAMETER(hr);
	}

	m_pAMDF.Release();
	if (FAILED(m_pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVidCap, IID_PPV_ARGS(&m_pAMDF)))) {
		DLog("Warning: No IAMDroppedFrames interface for vidcap capture");
	}

	// Capture Audio
	if (fAudCap) {
		IBaseFilter* pBF[3] = { pAudBuffer, pAudEnc, pAudMux ? pAudMux : pMux };
		HRESULT hr = BuildCapture(pAudCapPin, pBF, MEDIATYPE_Audio, &m_wndCaptureBar.m_capdlg.m_mtca);
		UNREFERENCED_PARAMETER(hr);
	}

	if ((m_pVidCap || m_pAudCap) && fCapture && fFileOutput) {
		if (pMux != pDst) {
			hr = m_pGB->AddFilter(pDst, L"File Writer V/A");
			hr = m_pGB->ConnectFilter(GetFirstPin(pMux, PINDIR_OUTPUT), pDst);
		}

		if (CComQIPtr<IConfigAviMux> pCAM = pMux.p) {
			int nIn, nOut, nInC, nOutC;
			CountPins(pMux, nIn, nOut, nInC, nOutC);
			pCAM->SetMasterStream(nInC-1);
			//pCAM->SetMasterStream(-1);
			pCAM->SetOutputCompatibilityIndex(FALSE);
		}

		if (CComQIPtr<IConfigInterleaving> pCI = pMux.p) {
			//if (FAILED(pCI->put_Mode(INTERLEAVE_CAPTURE)))
			if (FAILED(pCI->put_Mode(INTERLEAVE_NONE_BUFFERED))) {
				pCI->put_Mode(INTERLEAVE_NONE);
			}

			REFERENCE_TIME rtInterleave = 10000i64*AUDIOBUFFERLEN, rtPreroll = 0;	//10000i64*500
			pCI->put_Interleaving(&rtInterleave, &rtPreroll);
		}

		if (pMux != pAudMux && pAudMux != pAudDst) {
			hr = m_pGB->AddFilter(pAudDst, L"File Writer A");
			hr = m_pGB->ConnectFilter(GetFirstPin(pAudMux, PINDIR_OUTPUT), pAudDst);
		}
	}

	REFERENCE_TIME stop = MAX_TIME;
	hr = m_pCGB->ControlStream(&PIN_CATEGORY_CAPTURE, nullptr, nullptr, nullptr, &stop, 0, 0); // stop in the infinite

	OpenSetupVideo();
	OpenSetupAudio();
	OpenSetupStatsBar();
	OpenSetupStatusBar();
	RecalcLayout();

	SetupVMR9ColorControl();

	if (m_eMediaLoadState == MLS_LOADED) {
		if (fs == State_Running) {
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
		}
		else if (fs == State_Paused) {
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
		}
	}

	return true;
}

bool CMainFrame::StartCapture()
{
	if (!m_pCGB || m_bCapturing) {
		return false;
	}

	if (!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) {
		return false;
	}

	HRESULT hr;

	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// rare to see two capture filters to support IAMPushSource at the same time...
	//hr = CComQIPtr<IAMGraphStreams>(pGB)->SyncUsingStreamOffset(TRUE); // TODO:

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, true,
		m_wndCaptureBar.m_capdlg.m_fAudPreview, true);

	hr = m_pME->CancelDefaultHandling(EC_REPAINT);

	SendMessageW(WM_COMMAND, ID_PLAY_PLAY);

	m_bCapturing = true;

	return true;
}

bool CMainFrame::StopCapture()
{
	if (!m_pCGB || !m_bCapturing) {
		return false;
	}

	if (!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) {
		return false;
	}

	HRESULT hr;

	m_wndStatusBar.SetStatusMessage(ResStr(IDS_CONTROLS_COMPLETING));

	m_bCapturing = false;

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, false,
		m_wndCaptureBar.m_capdlg.m_fAudPreview, false);

	hr = m_pME->RestoreDefaultHandling(EC_REPAINT);

	::SetPriorityClass(::GetCurrentProcess(), AfxGetAppSettings().dwPriority);

	m_rtDurationOverride = -1;

	return true;
}

//

void CMainFrame::ShowOptions(int idPage)
{
	const auto bOnTop = (::GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) == WS_EX_TOPMOST;
	if (bOnTop) {
		SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	CAppSettings& s = AfxGetAppSettings();
	s.iSelectedVideoRenderer = -1;

	CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), GetModalParent(), idPage);

	m_bInOptions = true;
	INT_PTR dResult = options.DoModal();

	if (bOnTop) {
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	if (s.bResetSettings) {
		PostMessageW(WM_CLOSE);
		return;
	}
	if (m_bClosingState) {
		return; //prevent crash when player closes with options dialog opened
	}

	if (dResult == IDOK) {
		m_bInOptions = false;
		if (!m_bFullScreen) {
			SetAlwaysOnTop(s.iOnTop);
		}

		m_wndView.LoadLogo();
	}

	if (!IsD3DFullScreenMode()) {
		if (s.bHideWindowedMousePointer && m_eMediaLoadState == MLS_LOADED) {
			KillTimer(TIMER_MOUSEHIDER);
			SetTimer(TIMER_MOUSEHIDER, 2000, nullptr);
		} else {
			KillTimer(TIMER_MOUSEHIDER);
			m_bHideCursor = false;
		}
	}

	Invalidate();
	m_wndStatusBar.Relayout();
	m_wndPlaylistBar.Invalidate();

	m_bInOptions = false;
}

void CMainFrame::StartWebServer(int nPort)
{
	if (!m_pWebServer) {
		m_pWebServer = std::make_unique<CWebServer>(this, nPort);
	}
}

void CMainFrame::StopWebServer()
{
	if (m_pWebServer) {
		m_pWebServer.reset();
	}
}

void CMainFrame::SendStatusMessage(const CString& msg, const int nTimeOut)
{
	KillTimer(TIMER_STATUSERASER);

	m_playingmsg.Empty();
	if (nTimeOut <= 0) {
		return;
	}

	m_playingmsg = msg;
	SetTimer(TIMER_STATUSERASER, nTimeOut, nullptr);
	m_Lcd.SetStatusMessage(msg, nTimeOut);
}

void CMainFrame::SendStatusCompactPath(const CString& path)
{
	CString msg(path);
	if (CDC* pDC = m_wndStatusBar.GetDC()) {
		CRect r;
		m_wndStatusBar.GetClientRect(&r);
		if (::PathCompactPathW(pDC->m_hDC, msg.GetBuffer(), r.Width())) {
			msg.ReleaseBuffer();
		}
		m_wndStatusBar.ReleaseDC(pDC);
	}

	SendStatusMessage(msg, 3000);
}

BOOL CMainFrame::OpenCurPlaylistItem(REFERENCE_TIME rtStart/* = INVALID_TIME*/, BOOL bAddRecent/* = TRUE*/)
{
	if (m_wndPlaylistBar.GetCount() == 0) {
		return FALSE;
	}

	CPlaylistItem pli;
	if (!m_wndPlaylistBar.GetCur(pli)) {
		m_wndPlaylistBar.SetFirstSelected();
	}
	if (!m_wndPlaylistBar.GetCur(pli)) {
		return FALSE;
	}

	if (pli.m_fi.Valid()) {
		if (!::PathIsURLW(pli.m_fi)) {
			AfxGetAppSettings().strLastOpenFile = pli.m_fi.GetPath();
		}
		if (OpenIso(pli.m_fi, rtStart) || OpenBD(pli.m_fi, rtStart, bAddRecent)) {
			return TRUE;
		}
	}

	std::unique_ptr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	if (p) {
		p->bAddRecent = bAddRecent;
		OpenMedia(std::move(p));
	}

	return TRUE;
}

BOOL CMainFrame::OpenFile(const CString fname, REFERENCE_TIME rtStart/* = INVALID_TIME*/, BOOL bAddRecent/* = TRUE*/)
{
	std::unique_ptr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	if (p) {
		auto pFileData = dynamic_cast<OpenFileData*>(p.get());
		if (pFileData) {
			pFileData->fi = fname;
			p->bAddRecent = bAddRecent;
			OpenMedia(std::move(p));
			return TRUE;
		}
	}

	return FALSE;
}


void CMainFrame::AddCurDevToPlaylist()
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		m_wndPlaylistBar.Append(
			m_VidDispName,
			m_AudDispName,
			m_wndCaptureBar.m_capdlg.GetVideoInput(),
			m_wndCaptureBar.m_capdlg.GetVideoChannel(),
			m_wndCaptureBar.m_capdlg.GetAudioInput()
		);
	}
}

void CMainFrame::OpenMedia(std::unique_ptr<OpenMediaData> pOMD)
{
	auto pFileData   = dynamic_cast<const OpenFileData*>(pOMD.get());
	auto pDeviceData = dynamic_cast<const OpenDeviceData*>(pOMD.get());

	// shortcut
	if (pDeviceData
			&& m_eMediaLoadState == MLS_LOADED && m_pAMTuner
			&& m_VidDispName == pDeviceData->DisplayName[0] && m_AudDispName == pDeviceData->DisplayName[1]) {
		m_wndCaptureBar.m_capdlg.SetVideoInput(pDeviceData->vinput);
		m_wndCaptureBar.m_capdlg.SetVideoChannel(pDeviceData->vchannel);
		m_wndCaptureBar.m_capdlg.SetAudioInput(pDeviceData->ainput);
		return;
	}

	if (m_eMediaLoadState != MLS_CLOSED) {
		CloseMedia(TRUE);
	}

	CAppSettings& s = AfxGetAppSettings();

	bool bDirectShow = pFileData && pFileData->fi.Valid() && s.GetFileEngine(pFileData->fi) == DirectShow;
	bool bUseThread  = m_pGraphThread && s.fEnableWorkerThreadForOpening && (bDirectShow || !pFileData) && !pDeviceData;

	// create d3dfs window if launching in fullscreen and d3dfs is enabled
	if (s.ExclusiveFSAllowed() && (m_bStartInD3DFullscreen || s.fLaunchfullscreen)) {
		CreateFullScreenWindow();
		m_pVideoWnd = m_pFullscreenWnd;
		m_bStartInD3DFullscreen = false;
	} else {
		m_pVideoWnd = &m_wndView;
	}

	// don't set video renderer output rect until the window is repositioned
	m_bDelaySetOutputRect = true;

	SetLoadState(MLS_LOADING);

	if (bUseThread) {
		m_hGraphThreadEventOpen.ResetEvent();
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_OPEN, 0, (LPARAM)pOMD.release());
		m_bOpenedThruThread = true;
	} else {
		OpenMediaPrivate(pOMD);
		m_bOpenedThruThread = false;
	}
}

bool CMainFrame::ResizeDevice()
{
	if (m_pCAP) {
		return m_pCAP->ResizeDevice();
	}
	return true;
}

bool CMainFrame::ResetDevice()
{
	if (m_pCAP_preview) {
		m_pCAP_preview->ResetDevice();
	}
	if (m_pCAP) {
		return m_pCAP->ResetDevice();
	}
	return true;
}

bool CMainFrame::DisplayChange()
{
	if (m_pCAP_preview) {
		m_pCAP_preview->DisplayChange();
	}
	if (m_pCAP) {
		return m_pCAP->DisplayChange();
	}
	return true;
}

void CMainFrame::CloseMedia(BOOL bNextIsOpened/* = FALSE*/)
{
	if (m_eMediaLoadState == MLS_CLOSING || m_eMediaLoadState == MLS_CLOSED) {
		return;
	}

	DLog(L"CMainFrame::CloseMedia() : start");

	if (m_pFilterPropSheet) {
		m_pFilterPropSheet->EndDialog(0);
		//m_pFilterPropSheet->DestroyWindow();
		m_pFilterPropSheet = nullptr;
	}

	m_bNextIsOpened = bNextIsOpened;

	if (m_eMediaLoadState == MLS_LOADING) {
		m_fOpeningAborted = true;

		if (m_pGB) {
			if (!m_pAMOP) {
				m_pAMOP = m_pGB;
				if (!m_pAMOP) {
					BeginEnumFilters(m_pGB, pEF, pBF)
						if (m_pAMOP = pBF) {
							break;
						}
					EndEnumFilters;
				}
			}
			if (m_pAMOP) {
				m_pAMOP->AbortOperation();
			}
			m_pGB->Abort(); // TODO: lock on graph objects somehow, this is not thread safe
		}

		if (m_pGraphThread) {
			BeginWaitCursor();
			/*
			if (WaitForSingleObject(m_hGraphThreadEventOpen, 10000) == WAIT_TIMEOUT) {
				MessageBeep(MB_ICONEXCLAMATION);
				DLog(L"CRITICAL ERROR: Failed to abort graph creation.");
				TerminateProcess(GetCurrentProcess(), 0xDEAD);
			}
			*/
			WaitForSingleObject(m_hGraphThreadEventOpen, INFINITE);
			EndWaitCursor();

			MSG msg;
			if (PeekMessageW(&msg, m_hWnd, WM_POSTOPEN, WM_POSTOPEN, PM_REMOVE | PM_NOYIELD)) {
				delete ((OpenMediaData*)msg.wParam);
			}
		}
	}

	m_fOpeningAborted = false;

	SetLoadState(MLS_CLOSING);

	//PostMessageW(WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(0, 0));

	if (m_pGraphThread && m_bOpenedThruThread) {
		m_hGraphThreadEventClose.ResetEvent();
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_CLOSE, 0, 0);
		//WaitForSingleObject(m_hGraphThreadEventClose, INFINITE);

		HANDLE handle = m_hGraphThreadEventClose;
		DWORD dwWait;
		// We don't want to block messages processing completely so we stop waiting
		// on new message received from another thread or application.
		while ((dwWait = MsgWaitForMultipleObjects(1, &handle, FALSE, INFINITE, QS_SENDMESSAGE)) != WAIT_OBJECT_0) {
			// If we haven't got the event we were waiting for, we ensure that we have got a new message instead
			if (dwWait == WAIT_OBJECT_0 + 1) {
				// and we make sure it is handled before resuming waiting
				MSG msg;
				PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE);
			} else {
				ASSERT(FALSE);
			}
		}
	} else {
		CloseMediaPrivate();
	}

	OnFilePostCloseMedia();

	DLog(L"CMainFrame::CloseMedia() : end");
}

void CMainFrame::StartTunerScan(std::unique_ptr<TunerScanData>& pTSD)
{
	if (m_pGraphThread) {
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_TUNER_SCAN, 0, (LPARAM)pTSD.release());
	} else {
		DoTunerScan(pTSD.get());
	}
}

void CMainFrame::StopTunerScan()
{
	m_bStopTunerScan = true;
}

void CMainFrame::DisplayCurrentChannelOSD()
{
	CAppSettings& s = AfxGetAppSettings();
	CDVBChannel* pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	if (pChannel != nullptr) {
		// Get EIT information:
		PresentFollowing NowNext;
		CComQIPtr<IBDATuner> pTun = m_pGB.p;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}
		NowNext.cPresent.Insert(0,L" ");
		CString osd = pChannel->GetName() + NowNext.cPresent;
		const int i = osd.Find(L"\n");
		if (i > 0) {
			osd.Delete(i, osd.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd ,3500);
	}
}

void CMainFrame::DisplayCurrentChannelInfo()
{
	CAppSettings& s = AfxGetAppSettings();
	CDVBChannel* pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	if (pChannel != nullptr) {
		// Get EIT information:
		PresentFollowing NowNext;
		CComQIPtr<IBDATuner> pTun = m_pGB.p;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}

		CString osd = NowNext.cPresent + L". " + NowNext.StartTime + L" - " + NowNext.Duration + L". " + NowNext.SummaryDesc +L" ";
		int	 i = osd.Find(L"\n");
		if (i > 0) {
			osd.Delete(i, osd.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd, 8000, false, 12);
	}
}

void CMainFrame::SetLoadState(MPC_LOADSTATE iState)
{
	m_eMediaLoadState = iState;
	SendAPICommand(CMD_STATE, L"%d", m_eMediaLoadState);

	m_bOpening = true;

	switch (m_eMediaLoadState) {
		case MLS_CLOSED:
			m_bOpening = false;
			DLog(L"CMainFrame::SetLoadState() : CLOSED");
			break;
		case MLS_LOADING:
			DLog(L"CMainFrame::SetLoadState() : LOADING");
			break;
		case MLS_LOADED:
			DLog(L"CMainFrame::SetLoadState() : LOADED");
			break;
		case MLS_CLOSING:
			m_bOpening = false;
			DLog(L"CMainFrame::SetLoadState() : CLOSING");
			break;
		default:
			break;
	}
}

void CMainFrame::SetPlayState(MPC_PLAYSTATE iState)
{
	m_Lcd.SetPlayState((CMPC_Lcd::PlayState)iState);
	SendAPICommand(CMD_PLAYMODE, L"%d", iState);

	if (m_bEndOfStream) {
		SendAPICommand(CMD_NOTIFYENDOFSTREAM, L"\0"); // do not pass nullptr here!
	}

	// Prevent sleep when playing audio and/or video, but allow screensaver when only audio
	if (!m_bAudioOnly) {
		mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
		SetThreadExecutionState(iState == PS_PLAY ? ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
	} else {
		SetThreadExecutionState(iState == PS_PLAY ? ES_CONTINUOUS | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
	}

	UpdateThumbarButton();
	UpdateThumbnailClip();

	if (iState == PS_PAUSE) {
		auto& s = AfxGetAppSettings();
		if (s.bKeepHistory) {
			if ((GetPlaybackMode() == PM_FILE && s.bRememberFilePos)
					|| (GetPlaybackMode() == PM_DVD && s.bRememberDVDPos)) {
				SetTimer(TIMER_PAUSE, 5000, nullptr);
			}
		}
	} else {
		KillTimer(TIMER_PAUSE);
	}
}

BOOL CMainFrame::CreateFullScreenWindow()
{
	const CAppSettings& s = AfxGetAppSettings();
	CMonitor monitor;

	if (m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->DestroyWindow();
	}

	if (s.iMonitor == 0 && s.strFullScreenMonitor != L"Current") {
		CMonitors monitors;
		for (int i = 0; i < monitors.GetCount(); i++) {
			monitor = monitors.GetMonitor(i);
			if (monitor.IsMonitor()) {
				CString str;
				monitor.GetName(str);
				if (s.strFullScreenMonitor == str) {
					break;
				}
				monitor.Detach();
			}
		}
	}

	if (!monitor.IsMonitor()) {
		monitor = CMonitors::GetNearestMonitor(this);
	}

	CRect monitorRect;
	monitor.GetMonitorRect(monitorRect);

	return m_pFullscreenWnd->CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"", ResStr(IDS_MAINFRM_136),
										WS_POPUP | WS_VISIBLE, monitorRect, nullptr, 0);
}

bool CMainFrame::IsD3DFullScreenMode() const
{
	return m_pFullscreenWnd && m_pFullscreenWnd->IsWindow();
};

bool CMainFrame::CursorOnD3DFullScreenWindow() const
{
	if (IsD3DFullScreenMode()) {
		CPoint p;
		GetCursorPos(&p);
		CWnd* pWnd = WindowFromPoint(p);
		if (pWnd && *pWnd == *m_pFullscreenWnd) {
			return true;
		}
	}

	return false;
}

bool CMainFrame::CanShowDialog() const
{
	if (IsD3DFullScreenMode()) {
		const auto monitorD3D = CMonitors::GetNearestMonitor(m_pFullscreenWnd);
		const auto monitorMain = CMonitors::GetNearestMonitor(this);

		return monitorD3D != monitorMain;
	}

	return !m_bIsMadVRExclusiveMode && !m_bIsMPCVRExclusiveMode;
}

void CMainFrame::DestroyD3DWindow()
{
	if (IsD3DFullScreenMode()) {
		m_pFullscreenWnd->ShowWindow(SW_HIDE);
		m_pFullscreenWnd->DestroyWindow();

		StopAutoHideCursor();
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);

		SetFocus();
	}
}

void CMainFrame::SetupEVRColorControl()
{
	if (m_pMFVP) {
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Brightness, m_ColorControl.GetEVRColorControl(ProcAmp_Brightness)))) return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Contrast,   m_ColorControl.GetEVRColorControl(ProcAmp_Contrast))))   return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Hue,        m_ColorControl.GetEVRColorControl(ProcAmp_Hue))))        return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Saturation, m_ColorControl.GetEVRColorControl(ProcAmp_Saturation)))) return;
		m_ColorControl.EnableEVRColorControl();

		auto& rs = GetRenderersSettings();
		SetColorControl(ProcAmp_All, rs.iBrightness, rs.iContrast, rs.iHue, rs.iSaturation);
	}
}

void CMainFrame::SetupVMR9ColorControl()
{
	if (m_pVMRMC9) {
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorControl.GetVMR9ColorControl(ProcAmp_Brightness)))) return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorControl.GetVMR9ColorControl(ProcAmp_Contrast))))   return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorControl.GetVMR9ColorControl(ProcAmp_Hue))))        return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorControl.GetVMR9ColorControl(ProcAmp_Saturation)))) return;
		m_ColorControl.EnableVMR9ColorControl();

		auto& rs = GetRenderersSettings();
		SetColorControl(ProcAmp_All, rs.iBrightness, rs.iContrast, rs.iHue, rs.iSaturation);
	}
}

void CMainFrame::SetColorControl(DWORD flags, int& brightness, int& contrast, int& hue, int& saturation)
{
	if (flags & ProcAmp_Brightness) {
		brightness = std::clamp(brightness, -100, 100);
	}
	if (flags & ProcAmp_Contrast) {
		contrast = std::clamp(contrast, -100, 100);
	}
	if (flags & ProcAmp_Hue) {
		hue = std::clamp(hue, -180, 180);
	}
	if (flags & ProcAmp_Saturation) {
		saturation = std::clamp(saturation, -100, 100);
	}

	if (m_pVMRMC9) {
		VMR9ProcAmpControl ClrControl = m_ColorControl.GetVMR9ProcAmpControl(flags, brightness, contrast, hue, saturation);
		m_pVMRMC9->SetProcAmpControl(0, &ClrControl);
	}
	else if (m_pMFVP) {
		DXVA2_ProcAmpValues ClrValues = m_ColorControl.GetEVRProcAmpValues(brightness, contrast, hue, saturation);
		m_pMFVP->SetProcAmpValues(flags, &ClrValues);
	}
}

void CMainFrame::SetClosedCaptions(bool enable)
{
	if (m_pLN21) {
		m_pLN21->SetServiceState(enable ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off);
	}
}

LPCWSTR CMainFrame::GetDVDAudioFormatName(DVD_AudioAttributes& ATR) const
{
	switch (ATR.AudioFormat) {
		case DVD_AudioFormat_AC3:
			return L"AC3";
		case DVD_AudioFormat_MPEG1:
		case DVD_AudioFormat_MPEG1_DRC:
			return L"MPEG1";
		case DVD_AudioFormat_MPEG2:
		case DVD_AudioFormat_MPEG2_DRC:
			return L"MPEG2";
		case DVD_AudioFormat_LPCM:
			return L"LPCM";
		case DVD_AudioFormat_DTS:
			return L"DTS";
		case DVD_AudioFormat_SDDS:
			return L"SDDS";
		case DVD_AudioFormat_Other:
		default:
			return ResStr(IDS_MAINFRM_137);
	}
}

afx_msg void CMainFrame::OnGotoSubtitle(UINT nID)
{
	if (m_pMS) {
		m_rtCurSubPos		= m_wndSeekBar.GetPosReal();
		m_lSubtitleShift	= 0;
		m_nCurSubtitle		= m_wndSubresyncBar.FindNearestSub(m_rtCurSubPos, (nID == ID_GOTO_NEXT_SUB));
		if (m_nCurSubtitle >= 0) {
			m_pMS->SetPositions(&m_rtCurSubPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
		}
	}
}

afx_msg void CMainFrame::OnShiftSubtitle(UINT nID)
{
	if (m_nCurSubtitle >= 0) {
		long lShift = (nID == ID_SHIFT_SUB_DOWN) ? -100 : 100;
		CString strSubShift;

		if (m_wndSubresyncBar.ShiftSubtitle(m_nCurSubtitle, lShift, m_rtCurSubPos)) {
			m_lSubtitleShift += lShift;
			if (m_pMS) {
				m_pMS->SetPositions(&m_rtCurSubPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			}
		}

		strSubShift.Format(ResStr(IDS_MAINFRM_138), m_lSubtitleShift);
		m_OSD.DisplayMessage(OSD_TOPLEFT, strSubShift);
	}
}

afx_msg void CMainFrame::OnSubtitleDelay(UINT nID)
{
	if (m_pDVS) {
		int SubtitleDelay, SubtitleSpeedMul, SubtitleSpeedDiv;
		if (SUCCEEDED(m_pDVS->get_SubtitleTiming(&SubtitleDelay, &SubtitleSpeedMul, &SubtitleSpeedDiv))) {
			if (nID == ID_SUB_DELAY_DEC) {
				SubtitleDelay -= AfxGetAppSettings().nSubDelayInterval;
			} else {
				SubtitleDelay += AfxGetAppSettings().nSubDelayInterval;
			}
			m_pDVS->put_SubtitleTiming(SubtitleDelay, SubtitleSpeedMul, SubtitleSpeedDiv);
			SetSubtitleDelay(SubtitleDelay);
		}

		return;
	}

	if (m_pCAP) {
		if (m_pSubStreams.empty()) {
			SendStatusMessage(ResStr(IDS_SUBTITLES_ERROR), 3000);
			return;
		}

		int delay = m_pCAP->GetSubtitleDelay();

		if (nID == ID_SUB_DELAY_DEC) {
			delay -= AfxGetAppSettings().nSubDelayInterval;
		} else {
			delay += AfxGetAppSettings().nSubDelayInterval;
		}

		SetSubtitleDelay(delay);
	}
}

afx_msg void CMainFrame::OnLanguage(UINT nID)
{
	nID -= ID_LANGUAGE_ENGLISH; // resource ID to index

	if (nID == CMPlayerCApp::GetLanguageIndex(ID_LANGUAGE_HEBREW)) { // Show a warning when switching to Hebrew (must not be translated)
		MessageBoxW(L"The Hebrew translation will be correctly displayed (with a right-to-left layout) after restarting the application.\n",
					L"MPC-BE", MB_ICONINFORMATION | MB_OK);
	}

	CMPlayerCApp::SetLanguage(nID);

	m_openCDsMenu.DestroyMenu();
	m_filtersMenu.DestroyMenu();
	m_SubtitlesMenu.DestroyMenu();
	m_AudioMenu.DestroyMenu();
	m_AudioMenu.DestroyMenu();
	m_SubtitlesMenu.DestroyMenu();
	m_VideoStreamsMenu.DestroyMenu();
	m_chaptersMenu.DestroyMenu();
	m_favoritesMenu.DestroyMenu();
	m_shadersMenu.DestroyMenu();
	m_recentfilesMenu.DestroyMenu();
	m_languageMenu.DestroyMenu();
	m_RButtonMenu.DestroyMenu();

	m_popupMenu.DestroyMenu();
	m_popupMainMenu.DestroyMenu();
	m_VideoFrameMenu.DestroyMenu();
	m_PanScanMenu.DestroyMenu();
	m_ShadersMenu.DestroyMenu();
	m_AfterPlaybackMenu.DestroyMenu();
	m_NavigateMenu.DestroyMenu();

	m_popupMenu.LoadMenuW(IDR_POPUP);
	m_popupMainMenu.LoadMenuW(IDR_POPUPMAIN);
	m_VideoFrameMenu.LoadMenuW(IDR_POPUP_VIDEOFRAME);
	m_PanScanMenu.LoadMenuW(IDR_POPUP_PANSCAN);
	m_ShadersMenu.LoadMenuW(IDR_POPUP_SHADERS);
	m_AfterPlaybackMenu.LoadMenuW(IDR_POPUP_AFTERPLAYBACK);
	m_NavigateMenu.LoadMenuW(IDR_POPUP_NAVIGATE);

	CMenu defaultMenu;
	defaultMenu.LoadMenuW(IDR_MAINFRAME);
	CMenu* oldMenu = GetMenu();
	if (oldMenu) {
		// Attach the new menu to the window only if there was a menu before
		SetMenu(&defaultMenu);
		// and then destroy the old one
		oldMenu->DestroyMenu();
	}
	m_hMenuDefault = defaultMenu.Detach();

	auto& s = AfxGetAppSettings();

	if (s.bUseDarkTheme && s.bDarkMenu) {
		SetColorMenu();
	}

	// Re-create Win 7 TaskBar preview button for change button hint
	CreateThumbnailToolbar();

	m_wndSubresyncBar.ReloadTranslatableResources();
	m_wndCaptureBar.ReloadTranslatableResources();
	m_wndNavigationBar.ReloadTranslatableResources();
	m_wndShaderEditorBar.ReloadTranslatableResources();
	m_wndPlaylistBar.ReloadTranslatableResources();

	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();
	OnTimer(TIMER_STATS);

	s.SaveSettings();
}

void CMainFrame::ProcessAPICommand(COPYDATASTRUCT* pCDS)
{
	if (pCDS->lpData && (pCDS->cbData < 2 || ((LPWSTR)pCDS->lpData)[pCDS->cbData/sizeof(WCHAR) - 1] != 0)) {
		DLog(L"ProcessAPICommand: possibly incorrect data! prevent possible crash");
		return;
	}

	CString fn;
	REFERENCE_TIME rtPos = 0;

	switch (pCDS->dwData) {
		case CMD_OPENFILE:
			fn = (LPCWSTR)pCDS->lpData;
			fn.Trim();
			if (fn.GetLength()) {
				m_wndPlaylistBar.Open(fn);
				OpenCurPlaylistItem();
			}
			break;
		case CMD_OPENFILE_DUB:
			fn = (LPCWSTR)pCDS->lpData;
			fn.Trim();
			if (fn.GetLength()) {
				std::list<CString> fns;
				Explode(fn, fns, L'|');
				m_wndPlaylistBar.Open(fns, false);
				OpenCurPlaylistItem();
			}
			break;
		case CMD_STOP :
			OnPlayStop();
			break;
		case CMD_CLOSEFILE :
			CloseMedia();
			break;
		case CMD_PLAYPAUSE :
			OnPlayPlayPause();
			break;
		case CMD_PLAY :
			OnPlayPlay();
			break;
		case CMD_PAUSE :
			OnPlayPause();
			break;
		case CMD_ADDTOPLAYLIST :
			fn = (LPCWSTR)pCDS->lpData;
			fn.Trim();
			if (fn.GetLength()) {
				m_wndPlaylistBar.Append(fn);
			}
			break;
		case CMD_CLEARPLAYLIST :
			m_wndPlaylistBar.Empty();
			break;
		case CMD_STARTPLAYLIST :
			OpenCurPlaylistItem();
			break;
		case CMD_REMOVEFROMPLAYLIST: // TODO
			break;
		case CMD_SETPOSITION :
			rtPos = 10000i64 * REFERENCE_TIME(_wtof((LPCWSTR)pCDS->lpData) * 1000); //with accuracy of 1 ms
			// imianz: quick and dirty trick
			// Pause->SeekTo->Play (in place of SeekTo only) seems to prevents in most cases
			// some strange video effects on avi files (ex. locks a while and than running fast).
			if (!m_bAudioOnly && GetMediaState()==State_Running) {
				SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
				SeekTo(rtPos);
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			} else {
				SeekTo(rtPos);
			}
			// show current position overridden by play command
			m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 2000);
			break;
		case CMD_SETAUDIODELAY :
			rtPos = _wtol((LPCWSTR)pCDS->lpData) * 10000i64;
			SetAudioDelay(rtPos);
			break;
		case CMD_SETSUBTITLEDELAY :
			SetSubtitleDelay(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_SETINDEXPLAYLIST : // DOESN'T WORK
			//m_wndPlaylistBar.SetSelIdx(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_SETAUDIOTRACK :
			SetAudioTrackIdx(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_SETSUBTITLETRACK :
			SetSubtitleTrackIdx(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_GETSUBTITLETRACKS :
			SendSubtitleTracksToApi();
			break;
		case CMD_GETAUDIOTRACKS :
			SendAudioTracksToApi();
			break;
		case CMD_GETNOWPLAYING:
			SendNowPlayingToApi();
			break;
		case CMD_GETPLAYLIST :
			SendPlaylistToApi();
			break;
		case CMD_GETCURRENTPOSITION :
			SendCurrentPositionToApi();
			break;
		case CMD_JUMPOFNSECONDS :
			JumpOfNSeconds(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_GETVERSION:
			SendAPICommand(CMD_VERSION, MPC_VERSION_FULL_WSTR);
			break;
		case CMD_TOGGLEFULLSCREEN :
			OnViewFullscreen();
			break;
		case CMD_JUMPFORWARDMED :
			OnPlaySeek(ID_PLAY_SEEKFORWARDMED);
			break;
		case CMD_JUMPBACKWARDMED :
			OnPlaySeek(ID_PLAY_SEEKBACKWARDMED);
			break;
		case CMD_INCREASEVOLUME :
			m_wndToolBar.m_volctrl.IncreaseVolume();
			break;
		case CMD_DECREASEVOLUME :
			m_wndToolBar.m_volctrl.DecreaseVolume();
			break;
		case CMD_SHADER_TOGGLE :
			OnShaderToggle1();
			break;
		case CMD_CLOSEAPP :
			PostMessageW(WM_CLOSE);
			break;
		case CMD_SETSPEED:
			SetPlayingRate(_wtof((LPCWSTR)pCDS->lpData));
			break;
		case CMD_OSDSHOWMESSAGE:
			ShowOSDCustomMessageApi((MPC_OSDDATA *)pCDS->lpData);
			break;
	}
}

void CMainFrame::SendAPICommand(MPCAPI_COMMAND nCommand, LPCWSTR fmt, ...)
{
	const CAppSettings& s = AfxGetAppSettings();

	if (s.hMasterWnd) {
		WCHAR buff[800] = {};

		va_list args;
		va_start(args, fmt);
		vswprintf_s(buff, std::size(buff), fmt, args);

		COPYDATASTRUCT CDS;
		CDS.cbData = (wcslen (buff) + 1) * sizeof(WCHAR);
		CDS.dwData = nCommand;
		CDS.lpData = (LPVOID)buff;

		::SendMessageW(s.hMasterWnd, WM_COPYDATA, (WPARAM)GetSafeHwnd(), (LPARAM)&CDS);

		va_end(args);
	}
}

void CMainFrame::SendNowPlayingToApi()
{
	if (!AfxGetAppSettings().hMasterWnd) {
		return;
	}

	if (m_eMediaLoadState == MLS_LOADED) {
		CPlaylistItem  pli;
		CString        title, author, description;
		CString        label;
		long           lDuration = 0;
		REFERENCE_TIME rtDur;

		if (GetPlaybackMode() == PM_FILE) {
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_TITLE), title);
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_AUTHOR), author);
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_DESCRIPTION), description);

			m_wndPlaylistBar.GetCur(pli);
			if (pli.m_fi.Valid()) {
				label = !pli.m_label.IsEmpty() ? pli.m_label : pli.m_fi.GetPath();

				m_pMS->GetDuration(&rtDur);
				DVD_HMSF_TIMECODE tcDur = RT2HMSF(rtDur);
				lDuration = tcDur.bHours * 60 * 60 + tcDur.bMinutes * 60 + tcDur.bSeconds;
			}
		}
		else if (GetPlaybackMode() == PM_DVD) {
			DVD_DOMAIN DVDDomain;
			ULONG ulNumOfChapters = 0;
			DVD_PLAYBACK_LOCATION2 Location;

			// Get current DVD Domain
			if (SUCCEEDED(m_pDVDI->GetCurrentDomain(&DVDDomain))) {
				switch (DVDDomain) {
					case DVD_DOMAIN_Stop:
						title = L"DVD - Stopped";
						break;
					case DVD_DOMAIN_FirstPlay:
						title = L"DVD - FirstPlay";
						break;
					case DVD_DOMAIN_VideoManagerMenu:
						title = L"DVD - RootMenu";
						break;
					case DVD_DOMAIN_VideoTitleSetMenu:
						title = L"DVD - TitleMenu";
						break;
					case DVD_DOMAIN_Title:
						title = L"DVD - Title";
						break;
				}

				// get title information
				if (DVDDomain == DVD_DOMAIN_Title) {
					// get current location (title number & chapter)
					if (SUCCEEDED(m_pDVDI->GetCurrentLocation(&Location))) {
						// get number of chapters in current title
						m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
					}

					// get total time of title
					DVD_HMSF_TIMECODE tcDur;
					ULONG ulFlags;
					if (SUCCEEDED(m_pDVDI->GetTotalTitleTime(&tcDur, &ulFlags))) {
						// calculate duration in seconds
						lDuration = tcDur.bHours * 60 * 60 + tcDur.bMinutes * 60 + tcDur.bSeconds;
					}

					// build string
					// DVD - xxxxx|currenttitle|numberofchapters|currentchapter|titleduration
					author.Format(L"%d", Location.TitleNum);
					description.Format(L"%u", ulNumOfChapters);
					label.Format(L"%u", Location.ChapterNum);
				}
			}
		}

		title.Replace(L"|", L"\\|");
		author.Replace(L"|", L"\\|");
		description.Replace(L"|", L"\\|");
		label.Replace(L"|", L"\\|");

		CStringW buff;
		buff.Format(L"%s|%s|%s|%s|%d", title, author, description, label, lDuration);

		SendAPICommand(CMD_NOWPLAYING, L"%s", buff);
		SendSubtitleTracksToApi();
		SendAudioTracksToApi();
	}
}

void CMainFrame::SendSubtitleTracksToApi()
{
	CString strSubs;
	if (m_eMediaLoadState == MLS_LOADED) {
		const auto& s = AfxGetAppSettings();
		if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1)) {
			int currentStream = -1;
			int intSubCount = 0;
			CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p;
			if (pSS) {
				DWORD cStreams;
				if (!FAILED(pSS->Count(&cStreams))) {

					BOOL bIsHaali = FALSE;
					CComQIPtr<IBaseFilter> pBF = pSS.p;
					if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
						bIsHaali = TRUE;
						cStreams--;
					}

					for (DWORD i = 0, j = cStreams; i < j; i++) {
						DWORD dwFlags  = DWORD_MAX;
						DWORD dwGroup  = DWORD_MAX;
						WCHAR* pszName = nullptr;

						if (SUCCEEDED(pSS->Info(i, nullptr, &dwFlags, nullptr, &dwGroup, &pszName, nullptr, nullptr)) && pszName) {
							if (dwGroup != SUBTITLE_GROUP) {
								CoTaskMemFree(pszName);
								continue;
							}

							CString name(pszName);
							name.Replace(L"|", L"\\|");
							strSubs.AppendFormat(L"%s", name);
							CoTaskMemFree(pszName);

							if ((dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) && m_iSubtitleSel == 0) {
								currentStream = intSubCount;
							}
							intSubCount++;
						}
					}
				}
			}

			int iSubtitleSel = m_iSubtitleSel;

			auto it = m_pSubStreams.cbegin();
			CComPtr<ISubStream> pSubStream;
			if (intSubCount > 0 && it != m_pSubStreams.cend()) {
				++it;
				if (iSubtitleSel != -1) {
					iSubtitleSel += (intSubCount - 1);
				}
			}

			while (it != m_pSubStreams.cend()) {
				pSubStream = *it++;
				if (!pSubStream) {
					continue;
				}

				for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
					WCHAR* pName = nullptr;
					if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, nullptr))) {
						if (!strSubs.IsEmpty()) {
							strSubs.Append(L"|");
						}
						CString name(pName);
						name.Replace(L"|", L"\\|");
						strSubs.AppendFormat(L"%s", name);
						CoTaskMemFree(pName);
					}
				}
			}

			if (!strSubs.IsEmpty()) {
				if (s.fEnableSubtitles) {
					if (currentStream != -1) {
						strSubs.AppendFormat(L"|%i", currentStream);
					} else if (iSubtitleSel != -1) {
						strSubs.AppendFormat(L"|%i", iSubtitleSel);
					}
				} else {
					strSubs.Append(L"|-1");
				}
			} else {
				strSubs.Append(L"-1");
			}
		} else if (GetPlaybackMode() == PM_DVD && m_pDVDC) {
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			if (FAILED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
					|| ulStreamsAvailable == 0) {
				return;
			}

			LCID DefLanguage;
			DVD_SUBPICTURE_LANG_EXT ext;
			if (FAILED(m_pDVDI->GetDefaultSubpictureLanguage(&DefLanguage, &ext))) {
				return;
			}

			int currentStream = -1;
			for (ULONG i = 0; i < ulStreamsAvailable; i++) {
				LCID Language;
				if (FAILED(m_pDVDI->GetSubpictureLanguage(i, &Language))) {
					continue;
				}

				CString name;
				if (Language) {
					int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, name.GetBuffer(256), 256);
					name.ReleaseBufferSetLength(std::max(len - 1, 0));
				} else {
					name.Format(ResStr(IDS_AG_UNKNOWN), i + 1);
				}

				DVD_SubpictureAttributes ATR;
				if (SUCCEEDED(m_pDVDI->GetSubpictureAttributes(i, &ATR))) {
					switch (ATR.LanguageExtension) {
						case DVD_SP_EXT_NotSpecified:
						default:
							break;
						case DVD_SP_EXT_Caption_Normal:
							name += L"";
							break;
						case DVD_SP_EXT_Caption_Big:
							name += L" (Big)";
							break;
						case DVD_SP_EXT_Caption_Children:
							name += L" (Children)";
							break;
						case DVD_SP_EXT_CC_Normal:
							name += L" (CC)";
							break;
						case DVD_SP_EXT_CC_Big:
							name += L" (CC Big)";
							break;
						case DVD_SP_EXT_CC_Children:
							name += L" (CC Children)";
							break;
						case DVD_SP_EXT_Forced:
							name += L" (Forced)";
							break;
						case DVD_SP_EXT_DirectorComments_Normal:
							name += L" (Director Comments)";
							break;
						case DVD_SP_EXT_DirectorComments_Big:
							name += L" (Director Comments, Big)";
							break;
						case DVD_SP_EXT_DirectorComments_Children:
							name += L" (Director Comments, Children)";
							break;
					}
				}

				if (i == ulCurrentStream) {
					currentStream = i;
				}
				if (!strSubs.IsEmpty()) {
					strSubs.Append(L"|");
				}
				name.Replace(L"|", L"\\|");
				strSubs.AppendFormat(L"%s", name);
			}
			if (s.fEnableSubtitles) {
				strSubs.AppendFormat(L"|%i", currentStream);
			} else {
				strSubs.Append(L"|-1");
			}
		}
	} else {
		strSubs.Append(L"-2");
	}

	SendAPICommand(CMD_LISTSUBTITLETRACKS, L"%s", strSubs);
}

void CMainFrame::SendAudioTracksToApi()
{
	CString strAudios;
	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
			CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter.p;

			DWORD cStreams = 0;
			if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
				int currentStream = -1;
				for (DWORD i = 0; i < cStreams; i++) {
					DWORD dwFlags  = 0;
					WCHAR* pszName = nullptr;
					if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, &pszName, nullptr, nullptr)) || !pszName) {
						return;
					}
					if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
						currentStream = i;
					}
					if (!strAudios.IsEmpty()) {
						strAudios.Append(L"|");
					}
					CString name(pszName);
					name.Replace(L"|", L"\\|");
					strAudios.AppendFormat(L"%s", name);

					CoTaskMemFree(pszName);
				}
				strAudios.AppendFormat(L"|%i", currentStream);
			} else {
				strAudios.Append(L"-1");
			}
		} else if (GetPlaybackMode() == PM_DVD && m_pDVDC) {
			ULONG ulStreamsAvailable, ulCurrentStream;
			if (FAILED(m_pDVDI->GetCurrentAudio(&ulStreamsAvailable, &ulCurrentStream))
					|| ulStreamsAvailable == 0) {
				return;
			}

			LCID DefLanguage;
			DVD_AUDIO_LANG_EXT ext;
			if (FAILED(m_pDVDI->GetDefaultAudioLanguage(&DefLanguage, &ext))) {
				return;
			}

			int currentStream = -1;
			for (ULONG i = 0; i < ulStreamsAvailable; i++) {
				LCID Language;
				if (FAILED(m_pDVDI->GetAudioLanguage(i, &Language))) {
					continue;
				}

				CString name;
				if (Language) {
					int len = GetLocaleInfoW(Language, LOCALE_SENGLANGUAGE, name.GetBuffer(256), 256);
					name.ReleaseBufferSetLength(std::max(len-1, 0));
				} else {
					name.Format(ResStr(IDS_AG_UNKNOWN), i+1);
				}

				DVD_AudioAttributes ATR;
				if (SUCCEEDED(m_pDVDI->GetAudioAttributes(i, &ATR))) {
					switch (ATR.LanguageExtension) {
						case DVD_AUD_EXT_NotSpecified:
						default:
							break;
						case DVD_AUD_EXT_Captions:
							name += L" (Captions)";
							break;
						case DVD_AUD_EXT_VisuallyImpaired:
							name += L" (Visually Impaired)";
							break;
						case DVD_AUD_EXT_DirectorComments1:
							name += ResStr(IDS_MAINFRM_121);
							break;
						case DVD_AUD_EXT_DirectorComments2:
							name += ResStr(IDS_MAINFRM_122);
							break;
					}

					const CString format = GetDVDAudioFormatName(ATR);
					if (!format.IsEmpty()) {
						name.Format(ResStr(IDS_MAINFRM_11),
									name.GetString(),
									format,
									ATR.dwFrequency,
									ATR.bQuantization,
									ATR.bNumberOfChannels,
									(ATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
					}
				}

				if (i == ulCurrentStream) {
					currentStream = i;
				}
				if (!strAudios.IsEmpty()) {
					strAudios.Append(L"|");
				}
				name.Replace(L"|", L"\\|");
				strAudios.AppendFormat(L"%s", name);
			}
			strAudios.AppendFormat(L"|%i", currentStream);
		}
	} else {
		strAudios.Append(L"-2");
	}

	SendAPICommand(CMD_LISTAUDIOTRACKS, L"%s", strAudios);
}

void CMainFrame::SendPlaylistToApi()
{
	CString strPlaylist;

	POSITION pos = m_wndPlaylistBar.curPlayList.GetHeadPosition();
	while (pos) {
		CPlaylistItem& pli = m_wndPlaylistBar.curPlayList.GetNext(pos);

		if (pli.m_type == CPlaylistItem::file) {
			CString fn = pli.m_fi.GetPath();
			fn.Replace(L"|", L"\\|");
			if (strPlaylist.GetLength()) {
				strPlaylist.Append(L"|");
			}
			strPlaylist.Append(fn);

			for (const auto& fi : pli.m_auds) {
				fn = fi.GetPath();
				fn.Replace(L"|", L"\\|");
				if (!strPlaylist.IsEmpty()) {
					strPlaylist.Append (L"|");
				}
				strPlaylist.Append(fn);
			}
		}
	}

	if (strPlaylist.IsEmpty()) {
		strPlaylist.Append(L"-1");
	} else {
		strPlaylist.AppendFormat(L"|%i", m_wndPlaylistBar.GetSelIdx());
	}

	SendAPICommand(CMD_PLAYLIST, L"%s", strPlaylist);
}

void CMainFrame::SendCurrentPositionToApi(bool fNotifySeek)
{
	if (!AfxGetAppSettings().hMasterWnd) {
		return;
	}

	if (m_eMediaLoadState == MLS_LOADED) {
		CStringW strPos;

		if (GetPlaybackMode() == PM_FILE) {
			REFERENCE_TIME rtCur;
			m_pMS->GetCurrentPosition(&rtCur);
			strPos.Format(L"%.3f", rtCur/10000000.0);
		}
		else if (GetPlaybackMode() == PM_DVD) {
			DVD_PLAYBACK_LOCATION2 Location;
			// get current location while playing disc, will return 0, if at a menu
			if (m_pDVDI->GetCurrentLocation(&Location) == S_OK) {
				strPos.Format(L"%u", Location.TimeCode.bHours*60*60 + Location.TimeCode.bMinutes*60 + Location.TimeCode.bSeconds);
			}
		}

		SendAPICommand(fNotifySeek ? CMD_NOTIFYSEEK : CMD_CURRENTPOSITION, strPos);
	}
}

void CMainFrame::ShowOSDCustomMessageApi(MPC_OSDDATA *osdData)
{
	m_OSD.DisplayMessage((OSD_MESSAGEPOS)osdData->nMsgPos, osdData->strMsg, osdData->nDurationMS);
}

void CMainFrame::JumpOfNSeconds(int nSeconds)
{
	if (m_eMediaLoadState == MLS_LOADED) {
		long			lPosition = 0;
		REFERENCE_TIME	rtCur;

		if (GetPlaybackMode() == PM_FILE) {
			m_pMS->GetCurrentPosition(&rtCur);
			DVD_HMSF_TIMECODE tcCur = RT2HMSF(rtCur);
			lPosition = tcCur.bHours*60*60 + tcCur.bMinutes*60 + tcCur.bSeconds + nSeconds;

			// revert the update position to REFERENCE_TIME format
			tcCur.bHours	= lPosition/3600;
			tcCur.bMinutes	= (lPosition/60) % 60;
			tcCur.bSeconds	= lPosition%60;
			rtCur = HMSF2RT(tcCur);

			// quick and dirty trick:
			// pause->seekto->play seems to prevents some strange
			// video effect (ex. locks for a while and than running fast)
			if (!m_bAudioOnly) {
				SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
			}
			SeekTo(rtCur);
			if (!m_bAudioOnly) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
				// show current position overridden by play command
				m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 2000);
			}
		}
	}
}

void CMainFrame::OnFileOpenDirectory()
{
	if (m_eMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();
	CString path;
	BOOL recur = TRUE;

	CFileDialog dlg(TRUE);
	dlg.AddCheckButton(IDS_MAINFRM_DIR_CHECK, ResStr(IDS_MAINFRM_DIR_CHECK), TRUE);
	CComPtr<IFileOpenDialog> openDlgPtr = dlg.GetIFileOpenDialog();
	if (openDlgPtr != nullptr) {
		CComPtr<IShellItem> psiFolder;
		if (SUCCEEDED(afxGlobalData.ShellCreateItemFromParsingName(s.strLastOpenDir, nullptr, IID_PPV_ARGS(&psiFolder)))) {
			openDlgPtr->SetFolder(psiFolder);
		}

		openDlgPtr->SetTitle(ResStr(IDS_MAINFRM_DIR_TITLE));
		openDlgPtr->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
		if (FAILED(openDlgPtr->Show(m_hWnd))) {
			return;
		}

		psiFolder.Release();
		if (SUCCEEDED(openDlgPtr->GetResult(&psiFolder))) {
			LPWSTR folderpath = nullptr;
			if(SUCCEEDED(psiFolder->GetDisplayName(SIGDN_FILESYSPATH, &folderpath))) {
				path = folderpath;
				CoTaskMemFree(folderpath);
			}
		}

		dlg.GetCheckButtonState(IDS_MAINFRM_DIR_CHECK, recur);
	}

	if (!path.IsEmpty()) {
		AddSlash(path);
		s.strLastOpenDir = path;

		std::list<CString> sl;
		sl.emplace_back(path);
		if (recur) {
			RecurseAddDir(path, sl);
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
}

HRESULT CMainFrame::CreateThumbnailToolbar()
{
	if (!AfxGetAppSettings().fUseWin7TaskBar || !m_svgTaskbarButtons.IsLoad()) {
		return E_FAIL;
	}

	if (m_pTaskbarList) {
		m_pTaskbarList->Release();
	}

	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbarList));
	if (SUCCEEDED(hr)) {
		HBITMAP hBitmap = nullptr;
		int width = 0, height;

		if (SysVersion::IsWin10orLater()) {
			height = GetSystemMetrics(SM_CYICON);
		} else {
			// for Windows 7, 8.1
			height = ScaleY(18); // Don't use GetSystemMetrics(SM_CYICON) here!
		}
		hBitmap = m_svgTaskbarButtons.Rasterize(width, height);

		if (!hBitmap) {
			m_pTaskbarList->Release();
			return E_FAIL;
		}

		const auto nI = width / height;
		HIMAGELIST himl = ImageList_Create(height, height, ILC_COLOR32, nI, 0);

		// Add the bitmap
		ImageList_Add(himl, hBitmap, 0);
		DeleteObject(hBitmap);
		HRESULT hr = m_pTaskbarList->ThumbBarSetImageList(m_hWnd, himl);

		if (SUCCEEDED(hr)) {
			THUMBBUTTON buttons[5] = {};

			// PREVIOUS
			buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[0].dwFlags = THBF_DISABLED;
			buttons[0].iId = IDTB_BUTTON3;
			buttons[0].iBitmap = 0;
			StringCchCopyW(buttons[0].szTip, std::size(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

			// STOP
			buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[1].iId = IDTB_BUTTON1;
			buttons[1].iBitmap = 1;
			StringCchCopyW(buttons[1].szTip, std::size(buttons[1].szTip), ResStr(IDS_AG_STOP));

			// PLAY/PAUSE
			buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[2].iId = IDTB_BUTTON2;
			buttons[2].iBitmap = 3;
			StringCchCopyW(buttons[2].szTip, std::size(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

			// NEXT
			buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[3].dwFlags = THBF_DISABLED;
			buttons[3].iId = IDTB_BUTTON4;
			buttons[3].iBitmap = 4;
			StringCchCopyW(buttons[3].szTip, std::size(buttons[3].szTip), ResStr(IDS_AG_NEXT));

			// FULLSCREEN
			buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[4].dwFlags = THBF_DISABLED;
			buttons[4].iId = IDTB_BUTTON5;
			buttons[4].iBitmap = 5;
			StringCchCopyW(buttons[4].szTip, std::size(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

			hr = m_pTaskbarList->ThumbBarAddButtons(m_hWnd, std::size(buttons), buttons);
		}
		ImageList_Destroy(himl);

		if (!m_pTaskbarStateIconsImages.GetSafeHandle()) {
			CSvgImage svgTaskbarStateIcons;
			if (svgTaskbarStateIcons.Load(IDF_SVG_TASKBAR_STATE_ICONS)) {
				int width = 0;
				int height = ScaleY(16);
				if (HBITMAP hBitmap = svgTaskbarStateIcons.Rasterize(width, height)) {
					if (width == height * 3) {
						m_pTaskbarStateIconsImages.Create(height, height, ILC_COLOR32, 3, 0);
						ImageList_Add(m_pTaskbarStateIconsImages.GetSafeHandle(), hBitmap, 0);
					}

					DeleteObject(hBitmap);
				}
			}
		}
	}

	UpdateThumbarButton();
	UpdateThumbnailClip();

	return hr;
}

HRESULT CMainFrame::UpdateThumbarButton()
{
	if (!m_pTaskbarList) {
		return E_FAIL;
	}

	const auto& s = AfxGetAppSettings();

	if (!s.fUseWin7TaskBar) {
		m_pTaskbarList->SetOverlayIcon(m_hWnd, nullptr, L"");
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);

		THUMBBUTTON buttons[5] = {};

		buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
		buttons[0].dwFlags = THBF_HIDDEN;
		buttons[0].iId = IDTB_BUTTON3;

		buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
		buttons[1].dwFlags = THBF_HIDDEN;
		buttons[1].iId = IDTB_BUTTON1;

		buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
		buttons[2].dwFlags = THBF_HIDDEN;
		buttons[2].iId = IDTB_BUTTON2;

		buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
		buttons[3].dwFlags = THBF_HIDDEN;
		buttons[3].iId = IDTB_BUTTON4;

		buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
		buttons[4].dwFlags = THBF_HIDDEN;
		buttons[4].iId = IDTB_BUTTON5;

		HRESULT hr = m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, std::size(buttons), buttons);
		return hr;
	}

	THUMBBUTTON buttons[5] = {};

	buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[0].dwFlags = (s.fDontUseSearchInFolder && m_wndPlaylistBar.GetCount() <= 1 && (m_pCB && m_pCB->ChapGetCount() <= 1)) ? THBF_DISABLED : THBF_ENABLED;
	buttons[0].iId = IDTB_BUTTON3;
	buttons[0].iBitmap = 0;
	StringCchCopyW(buttons[0].szTip, std::size(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

	buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[1].iId = IDTB_BUTTON1;
	buttons[1].iBitmap = 1;
	StringCchCopyW(buttons[1].szTip, std::size(buttons[1].szTip), ResStr(IDS_AG_STOP));

	buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[2].iId = IDTB_BUTTON2;
	buttons[2].iBitmap = 3;
	StringCchCopyW(buttons[2].szTip, std::size(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

	buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[3].dwFlags = (s.fDontUseSearchInFolder && m_wndPlaylistBar.GetCount() <= 1 && (m_pCB && m_pCB->ChapGetCount() <= 1)) ? THBF_DISABLED : THBF_ENABLED;
	buttons[3].iId = IDTB_BUTTON4;
	buttons[3].iBitmap = 4;
	StringCchCopyW(buttons[3].szTip, std::size(buttons[3].szTip), ResStr(IDS_AG_NEXT));

	buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[4].dwFlags = THBF_ENABLED;
	buttons[4].iId = IDTB_BUTTON5;
	buttons[4].iBitmap = 5;
	StringCchCopyW(buttons[4].szTip, std::size(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

	HICON hIcon = nullptr;

	if (m_eMediaLoadState == MLS_LOADED) {
		OAFilterState fs = GetMediaState();
		if (fs == State_Running) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 2;

			hIcon = m_pTaskbarStateIconsImages.ExtractIconW(0);
			m_pTaskbarList->SetProgressState(m_hWnd, m_wndSeekBar.HasDuration() ? TBPF_NORMAL : TBPF_NOPROGRESS);
		} else if (fs == State_Stopped) {
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = m_pTaskbarStateIconsImages.ExtractIconW(2);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
		} else if (fs == State_Paused) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = m_pTaskbarStateIconsImages.ExtractIconW(1);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		}

		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) {
			buttons[0].dwFlags = THBF_DISABLED;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[3].dwFlags = THBF_DISABLED;
		}

		m_pTaskbarList->SetOverlayIcon(m_hWnd, hIcon, L"");

		if (hIcon) {
			DestroyIcon(hIcon);
		}
	} else {
		buttons[0].dwFlags = THBF_DISABLED;
		buttons[1].dwFlags = THBF_DISABLED;
		buttons[2].dwFlags = m_wndPlaylistBar.GetCount() > 0 ? THBF_ENABLED : THBF_DISABLED;
		buttons[3].dwFlags = THBF_DISABLED;
		buttons[4].dwFlags = THBF_DISABLED;

		m_pTaskbarList->SetOverlayIcon(m_hWnd, nullptr, L"");
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}

	HRESULT hr = m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, std::size(buttons), buttons);

	return hr;
}

HRESULT CMainFrame::UpdateThumbnailClip()
{
	CheckPointer(m_pTaskbarList, E_FAIL);

	CRect clientRect;
	m_wndView.GetClientRect(&clientRect);
	if (IsMainMenuVisible()) {
		clientRect.OffsetRect(0, GetSystemMetrics(SM_CYMENU));
	}

	if (!AfxGetAppSettings().fUseWin7TaskBar
			|| m_eMediaLoadState != MLS_LOADED
			|| (m_bAudioOnly && !SysVersion::IsWin10orLater())
			|| m_bFullScreen
			|| IsD3DFullScreenMode()
			|| clientRect.IsRectEmpty()) {
		return m_pTaskbarList->SetThumbnailClip(m_hWnd, nullptr);
	}

	return m_pTaskbarList->SetThumbnailClip(m_hWnd, &clientRect);
}

LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_COMMAND) && (THBN_CLICKED == HIWORD(wParam))) {
		int const wmId = LOWORD(wParam);
		switch (wmId) {
			case IDTB_BUTTON1:
				SendMessageW(WM_COMMAND, ID_PLAY_STOP);
				break;

			case IDTB_BUTTON2: {
					OAFilterState fs = GetMediaState();
					if (fs != -1) {
						SendMessageW(WM_COMMAND, ID_PLAY_PLAYPAUSE);
					} else {
						SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
					}
				}
				break;

			case IDTB_BUTTON3:
				SendMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
				break;

			case IDTB_BUTTON4:
				SendMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
				break;

			case IDTB_BUTTON5:
				WINDOWPLACEMENT wp;
				GetWindowPlacement(&wp);
				if (wp.showCmd == SW_SHOWMINIMIZED) {
					SendMessageW(WM_SYSCOMMAND, SC_RESTORE, -1);
				}
				SetForegroundWindow();
				SendMessageW(WM_COMMAND, ID_VIEW_FULLSCREEN);
				break;

			default:
				break;
		}
		return 0;
	}

	LRESULT ret = 0;
	bool bCallOurProc = true;
	if (m_pMVRSR) {
		// call madVR window proc directly when the interface is available
		bCallOurProc = !m_pMVRSR->ParentWindowProc(m_hWnd, message, &wParam, &lParam, &ret);
	}
	if (bCallOurProc) {
		ret = __super::WindowProc(message, wParam, lParam);
	}

	return ret;
}

UINT CMainFrame::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
{
	static BOOL bWasPausedBeforeSuspention;
	OAFilterState mediaState;

	switch (nPowerEvent) {
		case PBT_APMSUSPEND: // System is suspending operation.
			DLog(L"OnPowerBroadcast() - suspending"); // For user tracking

			bWasPausedBeforeSuspention = FALSE; // Reset value

			mediaState = GetMediaState();
			if (mediaState == State_Running) {
				bWasPausedBeforeSuspention = TRUE;
				SendMessageW(WM_COMMAND, ID_PLAY_PAUSE); // Pause
			}
			break;
		case PBT_APMRESUMEAUTOMATIC: // Operation is resuming automatically from a low-power state. This message is sent every time the system resumes.
			DLog(L"OnPowerBroadcast() - resuming"); // For user tracking

			// Resume if we paused before suspension.
			if (bWasPausedBeforeSuspention) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);    // Resume
			}
			break;
	}

	return __super::OnPowerBroadcast(nPowerEvent, nEventData);
}

void CMainFrame::OnSessionChange(UINT nSessionState, UINT nId)
{
	static BOOL bWasPausedBeforeSessionChange;

	switch (nSessionState) {
		case WTS_SESSION_LOCK:
			DLog(L"OnSessionChange() - Lock session");

			bWasPausedBeforeSessionChange = FALSE;
			if (GetMediaState() == State_Running && !m_bAudioOnly) {
				bWasPausedBeforeSessionChange = TRUE;
				SendMessageW( WM_COMMAND, ID_PLAY_PAUSE );
			}
			break;
		case WTS_SESSION_UNLOCK:
			DLog(L"OnSessionChange() - UnLock session");

			if (bWasPausedBeforeSessionChange) {
				SendMessageW( WM_COMMAND, ID_PLAY_PLAY );
			}
			break;
	}
}

void CMainFrame::OnSettingChange(UINT, LPCTSTR)
{
	SetColorTitle(true);
}

#define NOTIFY_FOR_THIS_SESSION 0
void CMainFrame::WTSRegisterSessionNotification()
{
	typedef BOOL (WINAPI *WTSREGISTERSESSIONNOTIFICATION)(HWND, DWORD);

	m_hWtsLib = LoadLibraryW(L"wtsapi32.dll");

	if (m_hWtsLib) {
		WTSREGISTERSESSIONNOTIFICATION fnWtsRegisterSessionNotification;

		fnWtsRegisterSessionNotification = (WTSREGISTERSESSIONNOTIFICATION)GetProcAddress(m_hWtsLib, "WTSRegisterSessionNotification");
		if (fnWtsRegisterSessionNotification) {
			fnWtsRegisterSessionNotification(m_hWnd, NOTIFY_FOR_THIS_SESSION);
		}
	}
}

void CMainFrame::WTSUnRegisterSessionNotification()
{
	typedef BOOL (WINAPI *WTSUNREGISTERSESSIONNOTIFICATION)(HWND);

	if (m_hWtsLib) {
		WTSUNREGISTERSESSIONNOTIFICATION fnWtsUnRegisterSessionNotification;

		fnWtsUnRegisterSessionNotification = (WTSUNREGISTERSESSIONNOTIFICATION)GetProcAddress(m_hWtsLib, "WTSUnRegisterSessionNotification");

		if (fnWtsUnRegisterSessionNotification) {
			fnWtsUnRegisterSessionNotification(m_hWnd);
		}

		FreeLibrary(m_hWtsLib);
	}
}

void CMainFrame::EnableShaders1(bool enable)
{
	if (enable && !AfxGetAppSettings().ShaderList.empty()) {
		m_bToggleShader = true;
		SetShaders();
	} else {
		m_bToggleShader = false;
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_FRAME);
		}
	}
}

void CMainFrame::EnableShaders2(bool enable)
{
	CAppSettings& s = AfxGetAppSettings();

	if (enable && (!s.ShaderListScreenSpace.empty() || !s.Shaders11PostScale.empty())) {
		m_bToggleShaderScreenSpace = true;
		SetShaders();
	} else {
		m_bToggleShaderScreenSpace = false;
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_SCREEN);
		}
	}
}

BOOL CMainFrame::OpenBD(const CString& path, REFERENCE_TIME rtStart, BOOL bAddRecent)
{
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();

	CString bdmv_folder = path.Left(path.ReverseFind('\\'));
	CString mpls_file;
	if (IsBDPlsFile(path)) {
		bdmv_folder.Truncate(bdmv_folder.ReverseFind('\\'));
		mpls_file = path;
	} else if (!IsBDStartFile(path)) {
		return FALSE;
	}

	if (::PathIsDirectoryW(bdmv_folder + L"\\PLAYLIST") && ::PathIsDirectoryW(bdmv_folder + L"\\STREAM")) {
		CHdmvClipInfo ClipInfo;
		CString main_mpls_file;
		CHdmvClipInfo::CPlaylist Playlist;

		if (SUCCEEDED(ClipInfo.FindMainMovie(bdmv_folder, main_mpls_file, Playlist))) {
			if (bdmv_folder.Right(5).MakeUpper() == L"\\BDMV") { // real BDMV folder
				CString infFile = bdmv_folder.Left(bdmv_folder.GetLength() - 5) + L"\\disc.inf";

				if (::PathFileExistsW(infFile)) {
					CTextFile cf(CTextFile::UTF8, CTextFile::ANSI);
					if (cf.Open(infFile)) {
						CString line;
						while (cf.ReadString(line)) {
							std::list<CString> sl;
							Explode(line, sl, L'=');
							if (sl.size() == 2 && CString(sl.front().Trim()).MakeLower() == L"label") {
								m_BDLabel = sl.back();
								break;
							}
						}
					}
				}

				if (m_BDLabel.IsEmpty()) {
					const auto& lr = CMPlayerCApp::languageResources[AfxGetAppSettings().iCurrentLanguage];
					CString bdmt_xml_file;
					bdmt_xml_file.Format(L"%s\\META\\DL\\bdmt_%s.xml", bdmv_folder, lr.iso6392);
					if (!::PathFileExistsW(bdmt_xml_file)) {
						bdmt_xml_file = bdmv_folder + L"\\META\\DL\\bdmt_eng.xml";
					}
					if (::PathFileExistsW(bdmt_xml_file)) {
						CTextFile cf(CTextFile::UTF8, CTextFile::ANSI);
						if (cf.Open(bdmt_xml_file)) {
							CString line;
							while (cf.ReadString(line)) {
								const auto title = RegExpParse(line.GetString(), L"<di:name>([^<>\\n]+)</di:name>");
								if (!title.IsEmpty()) {
									m_BDLabel = title;
									break;
								}
							}
						}
					}
				}
			}

			if (bAddRecent) {
				AddRecent((IsBDStartFile(path) || mpls_file == main_mpls_file) ? bdmv_folder + L"\\index.bdmv" : path);
			}

			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_bIsBDPlay = TRUE;
			m_LastOpenBDPath = bdmv_folder;

			if (OpenFile(mpls_file.GetLength() ? mpls_file : main_mpls_file, rtStart, FALSE) == TRUE) {
				const auto rtMinMPlsDuration = (REFERENCE_TIME)AfxGetAppSettings().nMinMPlsDuration * 60 * UNITS;
				m_BDPlaylists.clear();
				for (const auto& item : Playlist) {
					if (item.Duration() >= rtMinMPlsDuration) {
						m_BDPlaylists.emplace_back(item);
					}
				}

				return TRUE;
			}
		}
	}

	m_bIsBDPlay = FALSE;
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();
	return FALSE;
}

BOOL CMainFrame::IsBDStartFile(const CString& path)
{
	return (path.Right(11).MakeLower() == L"\\index.bdmv" || path.Right(17).MakeLower() == L"\\movieobject.bdmv");
}

BOOL CMainFrame::IsBDPlsFile(const CString& path)
{
	return (path.Right(5).MakeLower() == ".mpls" && path.Mid(path.ReverseFind('\\') - 9, 9).MakeUpper() == "\\PLAYLIST");
}

BOOL CMainFrame::CheckBD(CString& path)
{
	if (::PathIsDirectoryW(path)) {
		if (::PathFileExistsW(path + L"\\index.bdmv")) {
			path.TrimRight('\\');
			path.Append(L"\\index.bdmv");
			return TRUE;
		}
		if (::PathFileExistsW(path + L"\\BDMV\\index.bdmv")) {
			path.TrimRight('\\');
			path.Append(L"\\BDMV\\index.bdmv");
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CMainFrame::IsDVDStartFile(const CString& path)
{
	return (path.Right(13).MakeUpper() == L"\\VIDEO_TS.IFO");
}

BOOL CMainFrame::CheckDVD(CString& path)
{
	if (::PathIsDirectoryW(path)) {
		if (::PathFileExistsW(path + L"\\VIDEO_TS.IFO")) {
			path.TrimRight('\\');
			path.Append(L"\\VIDEO_TS.IFO");
			return TRUE;
		}
		if (::PathFileExistsW(path + L"\\VIDEO_TS\\VIDEO_TS.IFO")) {
			path.TrimRight('\\');
			path.Append(L"\\VIDEO_TS\\VIDEO_TS.IFO");
			return TRUE;
		}
	}

	return FALSE;
}

void CMainFrame::SetStatusMessage(const CString& msg)
{
	if (m_OldMessage != msg) {
		m_wndStatusBar.SetStatusMessage(msg);
		m_OldMessage = msg;
	}
}

CString CMainFrame::FillMessage()
{
	CString msg;

	if (!m_playingmsg.IsEmpty()) {
		msg = m_playingmsg;
	} else if (m_bCapturing) {
		msg = ResStr(IDS_CONTROLS_CAPTURING);

		if (m_pAMDF) {
			long lDropped = 0;
			m_pAMDF->GetNumDropped(&lDropped);
			long lNotDropped = 0;
			m_pAMDF->GetNumNotDropped(&lNotDropped);

			if ((lDropped + lNotDropped) > 0) {
				CString str;
				str.Format(ResStr(IDS_MAINFRM_37), lDropped + lNotDropped, lDropped);
				msg += str;
			}
		}

		CComPtr<IPin> pPin;
		if (m_pCGB && SUCCEEDED(m_pCGB->FindPin(m_wndCaptureBar.m_capdlg.m_pDst, PINDIR_INPUT, nullptr, nullptr, FALSE, 0, &pPin))) {
			LONGLONG size = 0;
			if (CComQIPtr<IStream> pStream = pPin.p) {
				pStream->Commit(STGC_DEFAULT);

				WIN32_FIND_DATAW findFileData;
				HANDLE h = FindFirstFileW(m_wndCaptureBar.m_capdlg.m_file, &findFileData);
				if (h != INVALID_HANDLE_VALUE) {
					size = ((LONGLONG)findFileData.nFileSizeHigh << 32) | findFileData.nFileSizeLow;

					CString str;
					if (size < 1024i64*1024) {
						str.Format(ResStr(IDS_MAINFRM_38), size/1024);
					} else { //if (size < 1024i64*1024*1024)
						str.Format(ResStr(IDS_MAINFRM_39), size/1024/1024);
					}
					msg += str;

					FindClose(h);
				}
			}

			ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
			if (GetDiskFreeSpaceEx(
						m_wndCaptureBar.m_capdlg.m_file.Left(m_wndCaptureBar.m_capdlg.m_file.ReverseFind('\\')+1),
						&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
				CString str;
				if (FreeBytesAvailable.QuadPart < 1024i64*1024) {
					str.Format(ResStr(IDS_MAINFRM_40), FreeBytesAvailable.QuadPart/1024);
				} else { //if (FreeBytesAvailable.QuadPart < 1024i64*1024*1024)
					str.Format(ResStr(IDS_MAINFRM_41), FreeBytesAvailable.QuadPart/1024/1024);
				}
				msg += str;
			}

			if (m_wndCaptureBar.m_capdlg.m_pMux) {
				__int64 pos = 0;
				CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux.p;
				if (pMuxMS && SUCCEEDED(pMuxMS->GetCurrentPosition(&pos)) && pos > 0) {
					double bytepersec = 10000000.0 * size / pos;
					if (bytepersec > 0) {
						m_rtDurationOverride = __int64(10000000.0 * (FreeBytesAvailable.QuadPart+size) / bytepersec);
					}
				}
			}

			if (m_wndCaptureBar.m_capdlg.m_pVidBuffer
					|| m_wndCaptureBar.m_capdlg.m_pAudBuffer) {
				int nFreeVidBuffers = 0, nFreeAudBuffers = 0;
				if (CComQIPtr<IBufferFilter> pVB = m_wndCaptureBar.m_capdlg.m_pVidBuffer.p) {
					nFreeVidBuffers = pVB->GetFreeBuffers();
				}
				if (CComQIPtr<IBufferFilter> pAB = m_wndCaptureBar.m_capdlg.m_pAudBuffer.p) {
					nFreeAudBuffers = pAB->GetFreeBuffers();
				}

				CString str;
				str.Format(ResStr(IDS_MAINFRM_42), nFreeVidBuffers, nFreeAudBuffers);
				msg += str;
			}
		}
	} else if (m_bBuffering) {
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF.p) {
				long BufferingProgress = 0;
				if (SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && BufferingProgress > 0) {
					msg.Format(ResStr(IDS_CONTROLS_BUFFERING), BufferingProgress);

					REFERENCE_TIME stop = m_wndSeekBar.GetRange();
					m_bLiveWM = (stop == 0);
				}
				break;
			}
		}
		EndEnumFilters;
	} else if (m_pAMOP) {
		__int64 t = 0, c = 0;
		if (SUCCEEDED(m_pMS->GetDuration(&t)) && t > 0 && SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t) {
			msg.Format(ResStr(IDS_CONTROLS_BUFFERING), llMulDiv(c, 100, t, 0));
		}

		if (!msg.IsEmpty() && State_Stopped == GetMediaState()) {
			OnTimer(TIMER_STREAMPOSPOLLER);
		}

	}

	return msg;
}

bool CMainFrame::CanPreviewUse()
{
	return (m_pGB_preview && m_bWndPreViewOn
			&& m_eMediaLoadState == MLS_LOADED
			&& !m_bAudioOnly);
}

bool CMainFrame::TogglePreview()
{
	if (m_pGB_preview && m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly) {
		m_bWndPreViewOn = !m_bWndPreViewOn;

		return m_bWndPreViewOn;
	}
	return false;
}

CStringW GetCoverImgFromPath(CStringW fullfilename)
{
	CStringW path = GetRemoveFileExt(fullfilename);

	const CStringW filename = GetFileName(path);

	RemoveFileSpec(path);

	const CStringW foldername = GetFileName(path);

	AddSlash(path);

	const CStringW coverNames[] = {
		L"cover",
		L"folder",
		foldername,
		L"front",
		filename,
		L"cover\\front",
		L"covers\\front",
		L"box"
	};

	std::vector<LPCWSTR> coverExts = { L".jpg", L".jpeg", L".png", L".bmp" };
	if (WicMatchDecoderFileExtension(L".webp")) {
		coverExts.emplace_back(L".webp");
	}
	if (WicMatchDecoderFileExtension(L".jxl")) {
		coverExts.emplace_back(L".jxl");
	}
	if (WicMatchDecoderFileExtension(L".heic")) {
		coverExts.emplace_back(L".heic");
	}

	for (const auto& coverName : coverNames) {
		for (const auto& coverExt : coverExts) {
			CStringW coverPath = path + coverName + coverExt;
			if (::PathFileExistsW(coverPath)) {
				return coverPath;
			}
		}
	}

	return L"";
}

HRESULT CMainFrame::SetAudioPicture(BOOL show)
{
	const CAppSettings& s = AfxGetAppSettings();

	if (m_ThumbCashedBitmap) {
		::DeleteObject(m_ThumbCashedBitmap);
		m_ThumbCashedBitmap = nullptr;
	}
	m_ThumbCashedSize.SetSize(0, 0);

	if (m_pfnDwmSetIconicThumbnail) {
		// for customize an iconic thumbnail and a live preview in Windows 7/8/8.1
		const BOOL set = s.fUseWin7TaskBar && m_bAudioOnly && IsSomethingLoaded() && show;
		DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_HAS_ICONIC_BITMAP, &set, sizeof(set));
		DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_FORCE_ICONIC_REPRESENTATION, &set, sizeof(set));
	}

	m_pMainBitmap.Release();
	m_pMainBitmapSmall.Release();

	if (m_bAudioOnly && IsSomethingLoaded() && show) {
		HRESULT hr = S_FALSE;
		bool bLoadRes = false;

		if (s.nAudioWindowMode == 1) {
			// load image from DSMResource to show in preview & logo;
			std::vector<LPCWSTR> mimeStrins = {
				// available formats
				L"image/jpeg",
				L"image/jpg", // non-standard
				L"image/png",
				L"image/bmp",
				// additional WIC components are needed
				L"image/webp",
				L"image/jxl",
				L"image/heic",
				L"image/avif",
			};

			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IDSMResourceBag> pRB = pBF.p)
					if (pRB && CheckMainFilter(pBF) && pRB->ResGetCount() > 0) {
						for (DWORD i = 0; i < pRB->ResGetCount() && bLoadRes == false; i++) {
							CComBSTR name, desc, mime;
							BYTE* pData = nullptr;
							DWORD len = 0;
							if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, nullptr))) {
								CString mimeStr(mime);
								mimeStr.Trim();

								if (std::find(mimeStrins.cbegin(), mimeStrins.cend(), mimeStr) != mimeStrins.cend()) {
									hr = WicLoadImage(&m_pMainBitmap, true, pData, len);
									if (SUCCEEDED(hr)) {
										bLoadRes = true;
									}
									DLogIf(FAILED(hr), L"Loading image '%s' (%s) failed with error %s",
										name, mime, HR2Str(hr));
								}

								CoTaskMemFree(pData);
							}
						}
					}
			}
			EndEnumFilters;

			if (!bLoadRes && !m_youtubeFields.thumbnailUrl.IsEmpty()) {
				CHTTPAsync HTTPAsync;
				if (SUCCEEDED(HTTPAsync.Connect(m_youtubeFields.thumbnailUrl.GetString(), http::connectTimeout))) {
					const auto contentLength = HTTPAsync.GetLenght();
					if (contentLength) {
						m_youtubeThumbnailData.resize(contentLength);
						DWORD dwSizeRead = 0;
						if (S_OK != HTTPAsync.Read((PBYTE)m_youtubeThumbnailData.data(), contentLength, dwSizeRead, http::readTimeout) || dwSizeRead != contentLength) {
							m_youtubeThumbnailData.clear();
						}
					} else {
						std::vector<char> tmp(16 * KILOBYTE);
						for (;;) {
							DWORD dwSizeRead = 0;
							if (S_OK != HTTPAsync.Read((PBYTE)tmp.data(), tmp.size(), dwSizeRead, http::readTimeout)) {
								break;
							}

							m_youtubeThumbnailData.insert(m_youtubeThumbnailData.end(), tmp.begin(), tmp.begin() + dwSizeRead);
						}
					}

					if (!m_youtubeThumbnailData.empty()) {
						hr = WicLoadImage(&m_pMainBitmap, true, m_youtubeThumbnailData.data(), m_youtubeThumbnailData.size());
						if (SUCCEEDED(hr)) {
							bLoadRes = true;
						}
					}
				}
			}

			if (!bLoadRes) {
				// try to load image from file in the same dir that media file to show in preview & logo;
				CString img_fname = GetCoverImgFromPath(m_strPlaybackRenderedPath);

				if (!img_fname.IsEmpty()) {
					hr = WicLoadImage(&m_pMainBitmap, true, img_fname.GetString());
					if (SUCCEEDED(hr)) {
						bLoadRes = true;
					}
				}
			}
		}

		if (!bLoadRes) {
			BYTE* data;
			UINT size;
			hr = LoadResourceFile(IDB_W7_AUDIO, &data, size);
			if (SUCCEEDED(hr)) {
				hr = WicLoadImage(&m_pMainBitmap, true, data, size);
			}
			if (SUCCEEDED(hr)) {
				hr = WicCreateBitmap(&m_pMainBitmapSmall, m_pMainBitmap);
			}
		} else {
			if (m_pMainBitmap) {
				UINT width, height;
				m_pMainBitmap->GetSize(&width, &height);

				const UINT maxSize = 256;
				if (width <= maxSize && height <= maxSize) {
					hr = WicCreateBitmap(&m_pMainBitmapSmall, m_pMainBitmap);
				} else {
					// Resize image to improve speed of show TaskBar preview

					int h = std::min(height, maxSize);
					int w = MulDiv(h, width, height);
					h     = MulDiv(w, height, width);

					hr = WicCreateBitmapScaled(&m_pMainBitmapSmall, w, h, m_pMainBitmap);
				}
				ASSERT(SUCCEEDED(hr));
			}
		}
	}

	m_wndView.ClearResizedImage();
	m_wndView.Invalidate();

	return S_OK;
}

LRESULT CMainFrame::OnDwmSendIconicThumbnail(WPARAM wParam, LPARAM lParam)
{
	if (!IsSomethingLoaded() || !m_bAudioOnly || !m_pfnDwmSetIconicThumbnail || !m_pMainBitmapSmall) {
		return 0;
	}

	UINT nWidth  = HIWORD(lParam);
	UINT nHeight = LOWORD(lParam);
	HRESULT hr = S_FALSE;

	if (m_ThumbCashedBitmap && m_ThumbCashedSize != CSize(nWidth, nHeight)) {
		::DeleteObject(m_ThumbCashedBitmap);
		m_ThumbCashedBitmap = nullptr;
	}

	if (!m_ThumbCashedBitmap && m_pMainBitmapSmall) {
		UINT width, height;
		hr = m_pMainBitmapSmall->GetSize(&width, &height);

		if (SUCCEEDED(hr)) {
			int h = std::min(height, nHeight);
			int w = MulDiv(h, width, height);
			h = MulDiv(w, height, width);

			CComPtr<IWICBitmap> pBitmap;
			hr = WicCreateBitmapScaled(&pBitmap, w, h, m_pMainBitmap);
			if (SUCCEEDED(hr)) {
				hr = WicCreateDibSecton(m_ThumbCashedBitmap, pBitmap);
			}
		}
	}

	if (m_ThumbCashedBitmap) {
		hr = m_pfnDwmSetIconicThumbnail(m_hWnd, m_ThumbCashedBitmap, 0);
		if (SUCCEEDED(hr)) {
			m_ThumbCashedSize.SetSize(nWidth, nHeight);
		}
	}

	if (FAILED(hr)) {
		// TODO ...
	}

	return 0;
}

LRESULT CMainFrame::OnDwmSendIconicLivePreviewBitmap(WPARAM, LPARAM)
{
	if (!IsSomethingLoaded() || !m_bAudioOnly || !m_pfnDwmSetIconicLivePreviewBitmap) {
		return 0;
	}

	const DWORD style = GetStyle();
	HRESULT hr = S_FALSE;

	if (m_isWindowMinimized && m_CaptureWndBitmap) {
		hr = m_pfnDwmSetIconicLivePreviewBitmap(m_hWnd, m_CaptureWndBitmap, nullptr, (style & WS_THICKFRAME) ? DWM_SIT_DISPLAYFRAME : 0);
	}
	else {
		const int borderX = (style & WS_THICKFRAME) ? GetSystemMetrics(SM_CXSIZEFRAME) : 0;
		const int borderY = (style & WS_THICKFRAME) ? GetSystemMetrics(SM_CYSIZEFRAME) : 0;
		const int captionY = (style & WS_CAPTION) ? GetSystemMetrics(SM_CYCAPTION) : 0;

		CRect rect;
		GetWindowRect(&rect);
		int x = borderX;
		int y = borderY + captionY;
		int w = rect.Width() - borderX * 2;
		int h = rect.Height() - (borderY * 2 + captionY);
		{
			// HACK
			x += borderX;
			y += borderY;
			w -= borderX*2;
			h -= borderY*2;
		}

		if (HBITMAP hBitmap = CreateCaptureDIB(x, y, w, h)) {
			hr = m_pfnDwmSetIconicLivePreviewBitmap(m_hWnd, hBitmap, nullptr, (style & WS_THICKFRAME) ? DWM_SIT_DISPLAYFRAME : 0);
			::DeleteObject(hBitmap);
		}
	}

	DLogIf(FAILED(hr), L"DwmSetIconicLivePreviewBitmap() failed with error %s", HR2Str(hr));

	return 0;
}

HBITMAP CMainFrame::CreateCaptureDIB(const int x, const int y, const int w, const int h)
{
	HBITMAP hbm = nullptr;
	CWindowDC wDC(this);

	if (HDC hdcMem = CreateCompatibleDC(wDC)) {
		BITMAPINFO bmi = {};
		bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth    = w;
		bmi.bmiHeader.biHeight   = -h;
		bmi.bmiHeader.biPlanes   = 1;
		bmi.bmiHeader.biBitCount = 32;

		PBYTE pbDS = nullptr;
		hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, nullptr, 0);
		if (hbm) {
			SelectObject(hdcMem, hbm);
			BitBlt(hdcMem, 0, 0, w, h, wDC, x, y, MERGECOPY);
		}

		DeleteDC(hdcMem);
	}

	return hbm;
}

void CMainFrame::CreateCaptureWindow()
{
	if (m_CaptureWndBitmap) {
		DeleteObject(m_CaptureWndBitmap);
		m_CaptureWndBitmap = nullptr;
	}

	const DWORD style = GetStyle();

	const int borderX = (style & WS_THICKFRAME) ? GetSystemMetrics(SM_CXSIZEFRAME) : 0;
	const int borderY = (style & WS_THICKFRAME) ? GetSystemMetrics(SM_CYSIZEFRAME) : 0;
	const int captionY = (style & WS_CAPTION) ? GetSystemMetrics(SM_CYCAPTION) : 0;

	CRect rect;
	GetWindowRect(&rect);
	int x = borderX;
	int y = borderY + captionY;
	int w = rect.Width() - borderX * 2;
	int h = rect.Height() - (borderY * 2 + captionY);
	{
		// HACK
		x += borderX;
		y += borderY;
		w -= borderX * 2;
		h -= borderY * 2;
	}

	m_CaptureWndBitmap = CreateCaptureDIB(x, y, w, h);
}

const CString CMainFrame::GetAltFileName()
{
	CString ret;
	if (!m_youtubeFields.fname.IsEmpty()) {
		ret = m_youtubeFields.fname;
		FixFilename(ret);
	}

	return ret;
}

void CMainFrame::subChangeNotifyThreadStart()
{
	subChangeNotifyThread = std::thread([this] { subChangeNotifyThreadFunction(); });
	::SetThreadPriority(subChangeNotifyThread.native_handle(), THREAD_PRIORITY_LOWEST);
}

void CMainFrame::subChangeNotifySetupThread(std::vector<HANDLE>& handles)
{
	for (size_t i = 2; i < handles.size(); i++) {
		FindCloseChangeNotification(handles[i]);
	}
	handles.clear();
	handles.emplace_back(m_EventSubChangeStopNotify);
	handles.emplace_back(m_EventSubChangeRefreshNotify);

	for (const auto& path : m_ExtSubPaths) {
		const HANDLE h = FindFirstChangeNotificationW(path, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
		if (h != INVALID_HANDLE_VALUE) {
			handles.emplace_back(h);
		}
	}
}

void CMainFrame::subChangeNotifyThreadFunction()
{
	std::vector<HANDLE> handles;
	subChangeNotifySetupThread(handles);

	while (true) {
		DWORD idx = WaitForMultipleObjects(handles.size(), handles.data(), FALSE, INFINITE);
		if (idx == WAIT_OBJECT_0) { // m_EventSubChangeStopNotify
			break;
		} else if (idx == (WAIT_OBJECT_0 + 1)) { // m_EventSubChangeRefreshNotify
			subChangeNotifySetupThread(handles);
		} else if (idx > (WAIT_OBJECT_0 + 1) && idx < (WAIT_OBJECT_0 + handles.size())) {
			if (FindNextChangeNotification(handles[idx - WAIT_OBJECT_0]) == FALSE) {
				break;
			}

			bool bChanged = false;
			for (auto& extSubFile : m_ExtSubFiles) {
				auto ftime = std::filesystem::last_write_time(extSubFile.path.GetString());
				if (ftime != extSubFile.time) {
					extSubFile.time = ftime;
					bChanged = true;
				}
			}

			if (bChanged) {
				ReloadSubtitle();
			}
		} else {
			DLog(L"CMainFrame::NotifyRenderThread() : %s", GetLastErrorMsg((LPWSTR)L"WaitForMultipleObjects"));
			ASSERT(FALSE);
			break;
		}
	}

	for (size_t i = 2; i < handles.size(); i++) {
		FindCloseChangeNotification(handles[i]);
	}
}

int CMainFrame::GetStreamCount(DWORD dwSelGroup)
{
	int streamcount = 0;
	if (CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter.p) {
		DWORD cStreams;
		if (S_OK == pSS->Count(&cStreams)) {
			for (long i = 0, n = cStreams; i < n; i++) {
				DWORD dwGroup = DWORD_MAX;
				if (S_OK == pSS->Info(i, nullptr, nullptr, nullptr, &dwGroup, nullptr, nullptr, nullptr)) {
					if (dwGroup == dwSelGroup) {
						streamcount++;
					}
				}
			}
		}
	}

	return streamcount;
}

void CMainFrame::AddSubtitlePathsAddons(LPCWSTR FileName)
{
	const CString tmp(GetAddSlash(GetFolderPath(FileName)).MakeUpper());
	auto& s = AfxGetAppSettings();

	if (!Contains(s.slSubtitlePathsAddons, tmp)) {
		s.slSubtitlePathsAddons.emplace_back(tmp);
	}
}

void CMainFrame::AddAudioPathsAddons(LPCWSTR FileName)
{
	const CString tmp(GetAddSlash(GetFolderPath(FileName)).MakeUpper());
	auto& s = AfxGetAppSettings();

	if (!Contains(s.slAudioPathsAddons, tmp)) {
		s.slAudioPathsAddons.emplace_back(tmp);
	}
}

BOOL CMainFrame::CheckMainFilter(IBaseFilter* pBF)
{
	while (pBF) {
		if (CComQIPtr<IFileSourceFilter> pFSF = pBF) {
			if (pFSF == m_pMainFSF) {
				return TRUE;
			}

			break;
		}

		pBF = GetUpStreamFilter(pBF);
	}

	return FALSE;
}

void CMainFrame::MakeBDLabel(CString path, CString& label, CString* pBDlabel)
{
	CString fn(path);

	fn.Replace('\\', '/');
	int pos = fn.Find(L"/BDMV");
	if (pos > 1) {
		fn.Delete(pos, fn.GetLength() - pos);
		CString fn2 = fn.Mid(fn.ReverseFind('/')+1);

		if (pBDlabel) {
			*pBDlabel = fn2;
		}

		if (fn2.GetLength() == 2 && fn2[fn2.GetLength() - 1] == ':') {
			WCHAR drive = fn2[0];
			std::list<CString> sl;
			cdrom_t CDRom_t = GetCDROMType(drive, sl);
			if (CDRom_t == CDROM_BDVideo) {
				CString BDLabel = GetDriveLabel(drive);
				if (BDLabel.GetLength() > 0) {
					fn2.AppendFormat(L" (%s)", BDLabel);
					if (pBDlabel) {
						*pBDlabel = BDLabel;
					}
				}
			}
		}

		if (!fn2.IsEmpty()) {
			label.AppendFormat(L" \"%s\"", fn2);
		}
	}
}

void CMainFrame::MakeDVDLabel(CString path, CString& label, CString* pDVDlabel)
{
	WCHAR buff[MAX_PATH] = { 0 };
	ULONG len = 0;

	CString DVDPath(path);
	if (DVDPath.IsEmpty()
			&& m_pDVDI && SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, std::size(buff), &len))) {
		DVDPath = buff;
	}

	if (!DVDPath.IsEmpty()) {
		const int pos = DVDPath.Find(L"\\VIDEO_TS");
		if (pos > 1) {
			DVDPath.Delete(pos, DVDPath.GetLength() - pos);
			DVDPath = DVDPath.Mid(DVDPath.ReverseFind('\\') + 1);

			if (pDVDlabel) {
				*pDVDlabel = DVDPath;
			}

			if (DVDPath.GetLength() == 2 && DVDPath[DVDPath.GetLength() - 1] == ':') {
				WCHAR drive = DVDPath[0];
				std::list<CString> sl;
				cdrom_t CDRom_t = GetCDROMType(drive, sl);
				if (CDRom_t == CDROM_DVDVideo) {
					const CString DVDLabel = GetDriveLabel(drive);
					if (DVDLabel.GetLength() > 0) {
						DVDPath.AppendFormat(L" (%s)", DVDLabel);
						if (pDVDlabel) {
							*pDVDlabel = DVDLabel;
						}
					}
				}
			}

			if (!DVDPath.IsEmpty()) {
				label.AppendFormat(L" \"%s\"", DVDPath);
			}
		}
	}
}

CString CMainFrame::GetCurFileName()
{
	CString fn = !m_youtubeFields.fname.IsEmpty() ? m_wndPlaylistBar.GetCurFileName() : m_strPlaybackRenderedPath;

	if (fn.IsEmpty() && m_pMainFSF) {
		LPOLESTR pFN = nullptr;
		AM_MEDIA_TYPE mt;
		if (SUCCEEDED(m_pMainFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
			fn = pFN;
			CoTaskMemFree(pFN);
		}
	}

	return fn;
}

CString CMainFrame::GetCurDVDPath(BOOL bSelectCurTitle/* = FALSE*/)
{
	CString path;
	WCHAR buff[MAX_PATH] = { 0 };
	ULONG len = 0;
	if (m_pDVDI && SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, MAX_PATH, &len)) && len) {
		path = buff;
		if (bSelectCurTitle) {
			path.Format(L"%s\\VIDEO_TS.IFO", buff);

			DVD_PLAYBACK_LOCATION2 loc;
			if (SUCCEEDED(m_pDVDI->GetCurrentLocation(&loc))) {
				ULONG VTSN, TTN;
				if (::PathFileExistsW(path) && CIfoFile::GetTitleInfo(path, loc.TitleNum, VTSN, TTN)) {
					path.Format(L"%s\\VTS_%02lu_0.IFO", buff, VTSN);
				}
			}
		}
	}

	return path;
}

GUID CMainFrame::GetTimeFormat()
{
	GUID ret;
	if (!m_pMS || !SUCCEEDED(m_pMS->GetTimeFormat(&ret))) {
		ASSERT(FALSE);
		ret = TIME_FORMAT_NONE;
	}

	return ret;
}

BOOL CMainFrame::OpenIso(const CString& pathName, REFERENCE_TIME rtStart/* = INVALID_TIME*/)
{
	if (m_DiskImage.CheckExtension(pathName)) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		WCHAR diskletter = m_DiskImage.MountDiskImage(pathName);
		if (diskletter) {
			if (::PathFileExistsW(CString(diskletter) + L":\\BDMV\\index.bdmv")) {
				OpenBD(CString(diskletter) + L":\\BDMV\\index.bdmv", rtStart, FALSE);
				AddRecent(pathName);
				return TRUE;
			}

			if (::PathFileExistsW(CString(diskletter) + L":\\VIDEO_TS\\VIDEO_TS.IFO")) {
				std::unique_ptr<OpenDVDData> p(DNew OpenDVDData());
				p->path = CString(diskletter) + L":\\VIDEO_TS\\VIDEO_TS.IFO";
				p->bAddRecent = FALSE;
				OpenMedia(std::move(p));

				AddRecent(pathName);
				return TRUE;
			}

			if (::PathFileExistsW(CString(diskletter) + L":\\AUDIO_TS\\ATS_01_0.IFO")) {
				OpenFile(CString(diskletter) + L":\\AUDIO_TS\\ATS_01_0.IFO", rtStart, FALSE);
				AddRecent(pathName);
				return TRUE;
			}

			if (pathName.Right(7).MakeLower() == L".iso.wv") {
				WIN32_FIND_DATAW fd;
				HANDLE hFind = FindFirstFileW(CString(diskletter) + L":\\*.wv", &fd);
				if (hFind != INVALID_HANDLE_VALUE) {
					OpenFile(CString(diskletter) + L":\\" + fd.cFileName, rtStart, FALSE);
					FindClose(hFind);

					AddRecent(pathName);
					return TRUE;
				}
			}

			m_DiskImage.UnmountDiskImage();
		}
	}

	return FALSE;
}

void CMainFrame::AddRecent(const CString& pathName)
{
	auto& s = AfxGetAppSettings();
	if (s.bKeepHistory) {
		m_SessionInfo.NewPath(pathName);

		if (IsLikelyFilePath(pathName)) {
			// there should not be a URL, otherwise explorer dirtied HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
			SHAddToRecentDocs(SHARD_PATHW, pathName); // remember the last open files (system) through the drag-n-drop
		}
	}
}

template<typename T>
bool NEARLY_EQ(T a, T b, T tol)
{
	return (abs(a - b) < tol);
}

REFTIME CMainFrame::GetAvgTimePerFrame(BOOL bUsePCAP/* = TRUE*/) const
{
	REFTIME refAvgTimePerFrame = 0.0;

	if (!m_pBV || FAILED(m_pBV->get_AvgTimePerFrame(&refAvgTimePerFrame))) {
		if (bUsePCAP && m_pCAP) {
			refAvgTimePerFrame = 1.0 / m_pCAP->GetFPS();
		}

		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (refAvgTimePerFrame > 0.0) {
				break;
			}

			BeginEnumPins(pBF, pEP, pPin) {
				CMediaType mt;
				if (SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
					if (mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo) {
						refAvgTimePerFrame = (REFTIME)((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
						break;
					} else if (mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo2) {
						refAvgTimePerFrame = (REFTIME)((VIDEOINFOHEADER2*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
						break;
					}
				}
			}
			EndEnumPins;
		}
		EndEnumFilters;
	}

	// Double-check that the detection is correct for DVDs
	DVD_VideoAttributes VATR;
	if (m_pDVDI && SUCCEEDED(m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
		double ratio;
		if (VATR.ulFrameRate == 50) {
			ratio = 25.0 * refAvgTimePerFrame;
			// Accept 25 or 50 fps
			if (!NEARLY_EQ(ratio, 1.0, 1e-2) && !NEARLY_EQ(ratio, 2.0, 1e-2)) {
				refAvgTimePerFrame = 1.0 / 25.0;
			}
		} else {
			ratio = 29.97 * refAvgTimePerFrame;
			// Accept 29,97, 59.94, 23.976 or 47.952 fps
			if (!NEARLY_EQ(ratio, 1.0, 1e-2) && !NEARLY_EQ(ratio, 2.0, 1e-2)
					&& !NEARLY_EQ(ratio, 1.25, 1e-2) && !NEARLY_EQ(ratio, 2.5, 1e-2)) {
				refAvgTimePerFrame = 1.0 / 29.97;
			}
		}
	}

	return refAvgTimePerFrame;
}

BOOL CMainFrame::OpenYoutubePlaylist(const CString& url, BOOL bOnlyParse/* = FALSE*/)
{
	if (AfxGetAppSettings().bYoutubeLoadPlaylist && Youtube::CheckPlaylist(url)) {
		Youtube::YoutubePlaylist youtubePlaylist;
		int idx_CurrentPlay = 0;
		if (Youtube::Parse_Playlist(url, youtubePlaylist, idx_CurrentPlay)) {
			if (!bOnlyParse) {
				m_wndPlaylistBar.Empty();
			}

			CFileItemList fis;
			for (const auto& item : youtubePlaylist) {
				CFileItem fi(item.url, item.title, item.duration);
				fis.emplace_back(fi);
			}
			m_wndPlaylistBar.Append(fis);

			if (!bOnlyParse) {
				m_wndPlaylistBar.SetSelIdx(idx_CurrentPlay, true);
				OpenCurPlaylistItem();
			}
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CMainFrame::AddSimilarFiles(std::list<CString>& fns)
{
	auto& s = AfxGetAppSettings();
	if (!s.nAddSimilarFiles
			|| fns.size() != 1
			|| !::PathFileExistsW(fns.front())
			|| ::PathIsDirectoryW(fns.front())) {
		return FALSE;
	}

	CString fname = fns.front();
	const CString path = GetAddSlash(GetFolderPath(fname));
	fname = GetFileName(fname);

	std::vector<CString> files;
	if (s.nAddSimilarFiles == 1) {
		CString name(fname);
		CString ext;
		const int n = name.ReverseFind('.');
		if (n > 0) {
			ext = name.Mid(n).MakeLower();
			name.Truncate(n);
		}

		const LPCWSTR excludeMask =
			L"ATS_\\d{2}_\\d{1}.*|" // DVD Audio
			L"VTS_\\d{2}_\\d{1}.*|" // DVD Video
			L"\\d{5}\\.clpi|"       // Blu-ray clip info
			L"\\d{5}\\.m2ts|"       // Blu-ray streams
			L"\\d{5}\\.mpls|"       // Blu-ray playlist
			L".+\\.evo";            // HD-DVD streams
		const std::wregex excludeMaskRe(excludeMask, std::wregex::icase);
		if (std::regex_match(fname.GetString(), excludeMaskRe)) {
			return FALSE;
		};

		std::wstring regExp = std::regex_replace(name.GetString(), std::wregex(LR"([\.\(\)\[\]\{\}\+])"), L"\\$&");

		static const LPCWSTR replaceText[] = {
			LR"(\b(?:s\d+\\?.?e\d+\\?.?)(.+)(?:(?:\b576|720|1080|1440|2160)[ip]\b|DVD))",
			LR"((?:(?:\b576|720|1080|1440|2160)[ip]\b|DVD)([^0-9]+))",
			LR"(\b(?:s\d+\\?.?e\d+\\?.?)([^0-9]+)$)"
		};
		for (const auto text : replaceText) {
			std::wcmatch match;
			if (std::regex_search(regExp.c_str(), match, std::wregex(text, std::regex_constants::icase))
					&& match.size() == 2
					&& match[1].length() > 1) {
				regExp.replace(match.position(1), match[1].length(), LR"((.+))");
			}
		}

		regExp = std::regex_replace(regExp, std::wregex(LR"((a|p)\\.m\\.)"), LR"(((a|p)\.m\.))");

		auto replaceDigital = [](const std::wstring& input) {
			std::wregex re(LR"((?:\b576|720|1080|1440|2160)[ip]\b|\d+ *- *\d+|\d+)");
			std::wsregex_token_iterator
				begin(input.cbegin(), input.end(), re, { -1, 0 }),
				end;

			std::wstring output;
			std::for_each(begin, end, [&](const std::wstring& m) {
				if (std::regex_match(m, std::wregex(L"(576|720|1080|1440|2160)[ip]"))) {
					output += m;
				} else if (std::regex_match(m, std::wregex(LR"(\d+ *- *\d+|\d+)"))) {
					output += LR"((\d+ *- *\d+|\d+))";
				} else {
					output += m;
				}
			});
			return output;
		};

		regExp = replaceDigital(regExp);

		regExp += L".*";
		if (!ext.IsEmpty()) {
			regExp += L"\\" + ext;
		}

		const std::wregex mask(regExp, std::wregex::icase);

		WIN32_FIND_DATAW wfd = {};
		HANDLE hFile = FindFirstFileW(path + '*' + ext, &wfd);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				if (std::regex_match(wfd.cFileName, mask)) {
					files.emplace_back(wfd.cFileName);
				}
			} while (FindNextFileW(hFile, &wfd));

			FindClose(hFile);
		}
	} else {
		auto& mf = s.m_Formats;
		WIN32_FIND_DATAW wfd = {};
		HANDLE hFile = FindFirstFileW(path + L"*.*", &wfd);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}
				const CString ext = GetFileExt(wfd.cFileName).MakeLower();
				if (auto mfc = mf.FindMediaByExt(ext); mfc && mfc->GetFileType() != TPlaylist) {
					files.emplace_back(wfd.cFileName);
				}
			} while (FindNextFileW(hFile, &wfd));

			FindClose(hFile);
		}
	}

	if (!files.empty()) {
		std::sort(files.begin(), files.end(), [](const CString& a, const CString& b) {
			return (StrCmpLogicalW(a, b) < 0);
		});

		auto it = std::find(files.cbegin(), files.cend(), fname);
		if (it != files.cend()) {
			++it;
			for (auto it_cur = it; it_cur < files.cend(); ++it_cur) {
				fns.emplace_back(path + (*it_cur));
			}
		}
	}

	return (fns.size() > 1);
}

void CMainFrame::SetToolBarAudioButton()
{
	m_wndToolBar.m_bAudioEnable = (m_eMediaLoadState == MLS_LOADED && m_pSwitcherFilter != nullptr);
}

void CMainFrame::SetToolBarSubtitleButton()
{
	BOOL bEnabled = FALSE;

	if (m_eMediaLoadState == MLS_LOADED) {
		if (m_pDVS) {
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
				bEnabled = TRUE;
			}
		} else if (m_pSubStreams.size()) {
			bEnabled = TRUE;
		} else if (m_pDVDI) {
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			bEnabled = SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled)) && (ulStreamsAvailable > 0);
		}
	}

	m_wndToolBar.m_bSubtitleEnable = bEnabled;
}

COLORREF CMainFrame::ColorBrightness(int lSkale, COLORREF color)
{
	int red   = lSkale > 0 ? std::min(GetRValue(color) + lSkale, 255) : std::max(0, GetRValue(color) + lSkale);
	int green = lSkale > 0 ? std::min(GetGValue(color) + lSkale, 255) : std::max(0, GetGValue(color) + lSkale);
	int blue  = lSkale > 0 ? std::min(GetBValue(color) + lSkale, 255) : std::max(0, GetBValue(color) + lSkale);

	return RGB(red, green, blue);
}

void CMainFrame::SetColorMenu()
{
	m_colMenuBk = ThemeRGB(45, 50, 55);

	const COLORREF crBkBar = m_colMenuBk;						// background system menu bar
	const COLORREF crBN    = ColorBrightness(-25, m_colMenuBk);	// backgroung normal
	const COLORREF crBNL   = ColorBrightness(+30, crBN);		// backgroung normal lighten (for light edge)
	const COLORREF crBND   = ColorBrightness(-30, crBN);		// backgroung normal darken (for dark edge)

	const COLORREF crBR    = ColorBrightness(+30, crBN);		// backgroung raisen (selected)
	const COLORREF crBRL   = ColorBrightness(+30, crBR);		// backgroung raisen lighten (for light edge)
	const COLORREF crBRD   = ColorBrightness(-60, crBR);		// backgroung raisen darken (for dark edge)

	const COLORREF crBS    = ColorBrightness(+20, crBN);		// backgroung sunken (selected grayed)
	const COLORREF crBSL   = ColorBrightness(+0, crBS);			// backgroung sunken lighten (for light edge)
	const COLORREF crBSD   = ColorBrightness(-0, crBS);			// backgroung sunken darken (for dark edge)

	const COLORREF crTN    = ColorBrightness(+140, crBN);		// text normal
	const COLORREF crTNL   = ColorBrightness(+80, crTN);		// text normal lighten
	const COLORREF crTG    = ColorBrightness(-80, crTN);		// text grayed
	const COLORREF crTGL   = ColorBrightness(-60, crTN);		// text grayed lighten
	const COLORREF crTS    = ThemeRGB(0xFF, 0xFF, 0xFF);		// text selected

	CMenuEx::SetColorMenu(
		crBkBar,
		crBN, crBNL, crBND,
		crBR, crBRL, crBRD,
		crBS, crBSL, crBSD,
		crTN, crTNL, crTG, crTGL, crTS);

	if (m_hMainMenuBrush) {
		::DeleteObject(m_hMainMenuBrush);
		m_hMainMenuBrush = nullptr;
	}
	m_hMainMenuBrush = ::CreateSolidBrush(m_colMenuBk);

	if (m_hPopupMenuBrush) {
		::DeleteObject(m_hPopupMenuBrush);
		m_hPopupMenuBrush = nullptr;
	}
	m_hPopupMenuBrush = ::CreateSolidBrush(crBN);

	auto pMenu = GetMenu();
	if (pMenu) {
		MENUINFO MenuInfo = { 0 };
		MenuInfo.cbSize = sizeof(MenuInfo);
		MenuInfo.hbrBack = m_hMainMenuBrush;
		MenuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		SetMenuInfo(pMenu->GetSafeHmenu(), &MenuInfo);

		CMenuEx::ChangeStyle(pMenu, true);
		DrawMenuBar();
	}
}

void CMainFrame::SetColorMenu(CMenu& menu)
{
	const auto& s = AfxGetAppSettings();
	if (s.bUseDarkTheme && s.bDarkMenu) {
		MENUINFO MenuInfo = { 0 };
		MenuInfo.cbSize = sizeof(MenuInfo);
		MenuInfo.hbrBack = m_hPopupMenuBrush;
		MenuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		SetMenuInfo(menu.GetSafeHmenu(), &MenuInfo);

		CMenuEx::ChangeStyle(&menu);
	}
}

void CMainFrame::SetColorTitle(const bool bSystemOnly/* = false*/)
{
	if (SysVersion::IsWin11orLater()) {
		const auto& s = AfxGetAppSettings();
		if (s.bUseDarkTheme && s.bDarkTitle) {
			if (!bSystemOnly) {
				m_colTitleBk = ThemeRGB(45, 50, 55);
				DwmSetWindowAttribute(m_hWnd, 35 /*DWMWA_CAPTION_COLOR*/, &m_colTitleBk, sizeof(m_colTitleBk));
			}
		} else {
			DwmSetWindowAttribute(m_hWnd, 35 /*DWMWA_CAPTION_COLOR*/, &m_colTitleBkSystem, sizeof(m_colTitleBkSystem));
		}
	} else if (SysVersion::IsWin10v1809orLater()) {
		static HMODULE hUser = GetModuleHandleW(L"user32.dll");
		if (hUser) {
			static auto pSetWindowCompositionAttribute = reinterpret_cast<pfnSetWindowCompositionAttribute>(GetProcAddress(hUser, "SetWindowCompositionAttribute"));
			if (pSetWindowCompositionAttribute) {
				const auto& s = AfxGetAppSettings();
				ACCENT_POLICY accent = { (s.bUseDarkTheme && s.bDarkTitle ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED), 0, 0, 0 };
				WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &accent, sizeof(accent) };
				pSetWindowCompositionAttribute(GetSafeHwnd(), &data);
			}
		}
	}
}

void CMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!nIDCtl && lpDrawItemStruct->CtlType == ODT_MENU && AfxGetAppSettings().bUseDarkTheme && AfxGetAppSettings().bDarkMenu) {
		CMenuEx::DrawItem(lpDrawItemStruct);
		return;
	}

	__super::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (!nIDCtl && lpMeasureItemStruct->CtlType == ODT_MENU && AfxGetAppSettings().bUseDarkTheme && AfxGetAppSettings().bDarkMenu) {
		CMenuEx::MeasureItem(lpMeasureItemStruct);
		return;
	}

	__super::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CMainFrame::ResetMenu()
{
	// DarkTheme <-> WhiteTheme

	m_openCDsMenu.DestroyMenu();
	m_filtersMenu.DestroyMenu();
	m_SubtitlesMenu.DestroyMenu();
	m_AudioMenu.DestroyMenu();
	m_AudioMenu.DestroyMenu();
	m_SubtitlesMenu.DestroyMenu();
	m_VideoStreamsMenu.DestroyMenu();
	m_chaptersMenu.DestroyMenu();
	m_favoritesMenu.DestroyMenu();
	m_shadersMenu.DestroyMenu();
	m_recentfilesMenu.DestroyMenu();
	m_languageMenu.DestroyMenu();
	m_RButtonMenu.DestroyMenu();

	m_popupMenu.DestroyMenu();
	m_popupMainMenu.DestroyMenu();
	m_VideoFrameMenu.DestroyMenu();
	m_PanScanMenu.DestroyMenu();
	m_ShadersMenu.DestroyMenu();
	m_AfterPlaybackMenu.DestroyMenu();
	m_NavigateMenu.DestroyMenu();

	m_popupMenu.LoadMenuW(IDR_POPUP);
	m_popupMainMenu.LoadMenuW(IDR_POPUPMAIN);
	m_VideoFrameMenu.LoadMenuW(IDR_POPUP_VIDEOFRAME);
	m_PanScanMenu.LoadMenuW(IDR_POPUP_PANSCAN);
	m_ShadersMenu.LoadMenuW(IDR_POPUP_SHADERS);
	m_AfterPlaybackMenu.LoadMenuW(IDR_POPUP_AFTERPLAYBACK);
	m_NavigateMenu.LoadMenuW(IDR_POPUP_NAVIGATE);

	CMenu defaultMenu;
	defaultMenu.LoadMenuW(IDR_MAINFRAME);

	CMenu* oldMenu = GetMenu();
	if (oldMenu) {
		// Attach the new menu to the window only if there was a menu before
		SetMenu(&defaultMenu);
		// and then destroy the old one
		oldMenu->DestroyMenu();
	}
	m_hMenuDefault = defaultMenu.Detach();

	const auto& s = AfxGetAppSettings();
	const auto enableDarkMenu = s.bUseDarkTheme && s.bDarkMenu;
	CMenuEx::EnableHook(enableDarkMenu);

	if (enableDarkMenu) {
		SetColorMenu();
	}
}

void CMainFrame::StopAutoHideCursor()
{
	m_bHideCursor = false;
	KillTimer(TIMER_MOUSEHIDER);
}

void CMainFrame::StartAutoHideCursor()
{
	const auto& s = AfxGetAppSettings();
	UINT nElapse = 0;
	if (m_bFullScreen || IsD3DFullScreenMode()) {
		if (s.nShowBarsWhenFullScreenTimeOut > 0) {
			nElapse = std::max(s.nShowBarsWhenFullScreenTimeOut * 1000, 2000);
		} else {
			nElapse = 2000;
		}
	} else if (s.bHideWindowedMousePointer && m_eMediaLoadState == MLS_LOADED) {
		nElapse = 2000;
	}

	if (nElapse) {
		SetTimer(TIMER_MOUSEHIDER, nElapse, nullptr);
	}
}

const bool CMainFrame::GetFromClipboard(std::list<CString>& sl) const
{
	sl.clear();

	if (::IsClipboardFormatAvailable(CF_UNICODETEXT) && ::OpenClipboard(m_hWnd)) {
		if (HGLOBAL hglb = ::GetClipboardData(CF_UNICODETEXT)) {
			if (LPCWSTR pText = (LPCWSTR)::GlobalLock(hglb)) {
				CString text(pText);
				if (!text.IsEmpty()) {
					text.Remove(L'\r');

					std::list<CString> lines;
					Explode(text, lines, L'\n');
					for (const auto& line : lines) {
						auto path = ParseFileName(line);
						if (::PathIsURLW(path) || ::PathFileExistsW(path)) {
							sl.emplace_back(path);
						}
					}
				}
				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();
	} else if (::IsClipboardFormatAvailable(CF_HDROP) && ::OpenClipboard(m_hWnd)) {
		if (HGLOBAL hglb = ::GetClipboardData(CF_HDROP)) {
			if (HDROP hDrop = (HDROP)::GlobalLock(hglb)) {
				UINT nFiles = ::DragQueryFileW(hDrop, UINT_MAX, nullptr, 0);
				for (UINT iFile = 0; iFile < nFiles; iFile++) {
					CString fn = GetDragQueryFileName(hDrop, iFile);
					sl.emplace_back(fn);
				}
				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();
	}

	return !sl.empty();
}

void CMainFrame::ShowControlBarInternal(CControlBar* pBar, BOOL bShow)
{
	if (bShow && m_bFullScreen && m_bIsMPCVRExclusiveMode) {
		return;
	}

	ShowControlBar(pBar, bShow, FALSE);
	if (!bShow) {
		RepaintVideo();
	}
}

void CMainFrame::SetColor()
{
	m_wndToolBar.SetColor();
	m_wndPlaylistBar.SetColor();
	m_wndSeekBar.SetColor();
	m_wndPreView.SetColor();
}

LRESULT CALLBACK CMainFrame::MenuHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		auto pS = (PCWPSTRUCT)lParam;
		if (pS->message == WM_ENTERMENULOOP) {
			m_pThis->StopAutoHideCursor();
			m_pThis->m_bInMenu = true;
		} else if (pS->message == WM_MENUSELECT && HIWORD(pS->wParam) == 0xFFFF && pS->lParam == NULL) {
			m_pThis->StartAutoHideCursor();
			m_pThis->m_bInMenu = false;
		}
	}

	return ::CallNextHookEx(m_MenuHook, nCode, wParam, lParam);
}

void CMainFrame::OnUpdateABRepeat(CCmdUI* pCmdUI)
{
	bool canABRepeat = GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD;
	bool abRepeatActive = m_abRepeatPositionAEnabled || m_abRepeatPositionBEnabled;

	switch (pCmdUI->m_nID) {
	case ID_PLAY_REPEAT_AB:
		pCmdUI->Enable(canABRepeat && abRepeatActive);
		break;
	case ID_PLAY_REPEAT_AB_MARK_A:
		if (pCmdUI->m_pMenu) {
			pCmdUI->m_pMenu->CheckMenuItem(ID_PLAY_REPEAT_AB_MARK_A, MF_BYCOMMAND | (m_abRepeatPositionAEnabled ? MF_CHECKED : MF_UNCHECKED));
		}
		pCmdUI->Enable(canABRepeat);
		break;
	case ID_PLAY_REPEAT_AB_MARK_B:
		if (pCmdUI->m_pMenu) {
			pCmdUI->m_pMenu->CheckMenuItem(ID_PLAY_REPEAT_AB_MARK_B, MF_BYCOMMAND | (m_abRepeatPositionBEnabled ? MF_CHECKED : MF_UNCHECKED));
		}
		pCmdUI->Enable(canABRepeat);
		break;
	default:
		ASSERT(FALSE);
		return;
	}
}

void CMainFrame::OnABRepeat(UINT nID)
{
	switch (nID) {
	case ID_PLAY_REPEAT_AB:
		if (m_abRepeatPositionAEnabled || m_abRepeatPositionBEnabled) { //only support disabling from the menu
			m_abRepeatPositionAEnabled = false;
			m_abRepeatPositionBEnabled = false;
			m_abRepeatPositionA = 0;
			m_abRepeatPositionB = 0;
			m_wndSeekBar.Invalidate();
		}
		break;
	case ID_PLAY_REPEAT_AB_MARK_A:
	case ID_PLAY_REPEAT_AB_MARK_B:
		REFERENCE_TIME rtDur = 0;
		int playmode = GetPlaybackMode();

		if (playmode == PM_FILE && m_pMS) {
			m_pMS->GetDuration(&rtDur);
		}
		else if (playmode == PM_DVD && m_pDVDI) {
			DVD_PLAYBACK_LOCATION2 Location;
			if (m_pDVDI->GetCurrentLocation(&Location) == S_OK) {
				double fps = Location.TimeCodeFlags == DVD_TC_FLAG_25fps ? 25.0
					: Location.TimeCodeFlags == DVD_TC_FLAG_30fps ? 30.0
					: Location.TimeCodeFlags == DVD_TC_FLAG_DropFrame ? 30 / 1.001
					: 25.0;
				DVD_HMSF_TIMECODE tcDur;
				ULONG ulFlags;
				if (SUCCEEDED(m_pDVDI->GetTotalTitleTime(&tcDur, &ulFlags))) {
					rtDur = HMSF2RT(tcDur, fps);
				}
			}
		}
		else {
			return;
		}

		if (nID == ID_PLAY_REPEAT_AB_MARK_A) {
			if (m_abRepeatPositionAEnabled) {
				m_abRepeatPositionAEnabled = false;
				m_abRepeatPositionA = 0;
			}
			else if (SUCCEEDED(m_pMS->GetCurrentPosition(&m_abRepeatPositionA))) {
				if (m_abRepeatPositionA < rtDur) {
					m_abRepeatPositionAEnabled = true;
					if (m_abRepeatPositionBEnabled && m_abRepeatPositionA >= m_abRepeatPositionB) {
						m_abRepeatPositionBEnabled = false;
						m_abRepeatPositionB = 0;
					}
				}
				else {
					m_abRepeatPositionA = 0;
				}
			}
		}
		else if (nID == ID_PLAY_REPEAT_AB_MARK_B) {
			if (m_abRepeatPositionBEnabled) {
				m_abRepeatPositionBEnabled = false;
				m_abRepeatPositionB = 0;
			}
			else if (SUCCEEDED(m_pMS->GetCurrentPosition(&m_abRepeatPositionB))) {
				if (m_abRepeatPositionB > 0 && m_abRepeatPositionB > m_abRepeatPositionA && rtDur >= m_abRepeatPositionB) {
					m_abRepeatPositionBEnabled = true;
					if (GetMediaState() == State_Running) {
						PerformABRepeat(); //we just set loop point B, so we need to repeat right now
					}
				}
				else {
					m_abRepeatPositionB = 0;
				}
			}
		}

		m_wndSeekBar.Invalidate();
		break;
	}
}

void CMainFrame::PerformABRepeat()
{
	SeekTo(m_abRepeatPositionA);

	if (GetMediaState() == State_Stopped) {
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);
	}
}

void CMainFrame::DisableABRepeat()
{
	m_abRepeatPositionAEnabled = false;
	m_abRepeatPositionBEnabled = false;
	m_abRepeatPositionA = 0;
	m_abRepeatPositionB = 0;

	m_wndSeekBar.Invalidate();
}

bool CMainFrame::CheckABRepeat(REFERENCE_TIME& aPos, REFERENCE_TIME& bPos, bool& aEnabled, bool& bEnabled)
{
	if (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD) {
		if (m_abRepeatPositionAEnabled || m_abRepeatPositionBEnabled) {
			aPos = m_abRepeatPositionA;
			bPos = m_abRepeatPositionB;
			aEnabled = m_abRepeatPositionAEnabled;
			bEnabled = m_abRepeatPositionBEnabled;
			return true;
		}
	}
	return false;
}

void CMainFrame::OnUpdateRepeatForever(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();

	if (pCmdUI->m_pMenu) {
		pCmdUI->m_pMenu->CheckMenuItem(ID_REPEAT_FOREVER, MF_BYCOMMAND | (s.fLoopForever ? MF_CHECKED : MF_UNCHECKED));
	}
}

void CMainFrame::SaveHistory()
{
	auto& historyFile = AfxGetMyApp()->m_HistoryFile;
	auto& s = AfxGetAppSettings();

	if (GetPlaybackMode() == PM_FILE) {
		if (s.bRememberFilePos && !m_bGraphEventComplete) {
			REFERENCE_TIME rtDur;
			m_pMS->GetDuration(&rtDur);
			REFERENCE_TIME rtNow;
			m_pMS->GetCurrentPosition(&rtNow);

			m_SessionInfo.Position    = rtDur > 30 * UNITS ? rtNow : 0;
			m_SessionInfo.AudioNum    = GetAudioTrackIdx();
			m_SessionInfo.SubtitleNum = GetSubtitleTrackIdx();
		} else {
			m_SessionInfo.CleanPosition();
		}
		historyFile.SaveSessionInfo(m_SessionInfo);
	} else if (GetPlaybackMode() == PM_DVD && m_SessionInfo.DVDId) {
		if (s.bRememberDVDPos && m_iDVDTitleForHistory > 0) {
			m_SessionInfo.DVDTitle = m_iDVDTitleForHistory;

			CDVDStateStream stream;
			stream.AddRef();
			CComPtr<IDvdState> pStateData;
			if (SUCCEEDED(m_pDVDI->GetState(&pStateData))) {
				CComQIPtr<IPersistStream> pPersistStream = pStateData.p;
				if (pPersistStream && SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream))) {
					m_SessionInfo.DVDState = stream.m_data;
				}
			}
		} else {
			m_SessionInfo.CleanPosition();
		}
		historyFile.SaveSessionInfo(m_SessionInfo);
	}
}
