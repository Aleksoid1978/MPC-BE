/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <atlconv.h>
#include <atlsync.h>
#include <afxglobals.h>

#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include "../../DSUtil/FileVersion.h"
#include "../../DSUtil/DXVAState.h"
#include "OpenDlg.h"
#include "SaveDlg.h"
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
#include "SubtitleDlDlg.h"
#include "ISDb.h"
#include "UpdateChecker.h"

#include <BaseClasses/mtype.h>
#include <Mpconfig.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdevcod.h>
#include <dsound.h>
#include <InitGuid.h>
#include <uuids.h>
#include <moreuuids.h>
#include <qnetwork.h>
#include <psapi.h>
#include <IBufferControl.h>

#include "FGManager.h"
#include "FGManagerBDA.h"
#include "filters/TextPassThruFilter.h"
#include "filters/ChaptersSouce.h"
#include "../../filters/filters.h"
#include "../../filters/filters/InternalPropertyPage.h"
#include <AllocatorCommon.h>
#include <SyncAllocatorPresenter.h>
#include "ComPropertyPage.h"
#include "LcdSupport.h"
#include <IPinHook.h>
#include <comdef.h>
#include <dwmapi.h>

#include "OpenImage.h"

#include "../../Subtitles/RenderedHdmvSubtitle.h"
#include "../../Subtitles/SupSubFile.h"
#include "../../Subtitles/XSUBSubtitle.h"
#include "../../SubPic/MemSubPic.h"

#include "MultiMonitor.h"
#include <mvrInterfaces.h>

#include <string>
#include <regex>

#include "ThumbsTaskDlg.h"
#include "Content.h"

#include <SubRenderIntf.h>
#include <LAVVideoSettings.h>

#include "Variables.h"

#define DEFCLIENTW		292
#define DEFCLIENTH		200
#define MENUBARBREAK	30

#define BLU_RAY			L"Blu-ray"

static UINT s_uTaskbarRestart		= RegisterWindowMessage(L"TaskbarCreated");
static UINT s_uTBBC					= RegisterWindowMessage(L"TaskbarButtonCreated");
static UINT WM_NOTIFYICON			= RegisterWindowMessage(L"MYWM_NOTIFYICON");
static UINT s_uQueryCancelAutoPlay	= RegisterWindowMessage(L"QueryCancelAutoPlay");

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

//

#define SaveMediaState \
	OAFilterState __fs = GetMediaState(); \
 \
	REFERENCE_TIME __rt = 0; \
	if (m_eMediaLoadState == MLS_LOADED) __rt = GetPos(); \
 \
	if (__fs != State_Stopped) \
		SendMessageW(WM_COMMAND, ID_PLAY_STOP); \


#define RestoreMediaState \
	if (m_eMediaLoadState == MLS_LOADED) \
	{ \
		SeekTo(__rt); \
 \
		if (__fs == State_Stopped) \
			SendMessageW(WM_COMMAND, ID_PLAY_STOP); \
		else if (__fs == State_Paused) \
			SendMessageW(WM_COMMAND, ID_PLAY_PAUSE); \
		else if (__fs == State_Running) \
			SendMessageW(WM_COMMAND, ID_PLAY_PLAY); \
	} \

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

#define WM_HANDLE_CMDLINE (WM_USER + 300)

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
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

	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)

	ON_WM_SYSCOMMAND()
	ON_WM_ACTIVATEAPP()
	ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)
	ON_WM_INPUT()
	ON_MESSAGE(WM_HOTKEY, OnHotKey)

	ON_WM_TIMER()

	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_MESSAGE(WM_RESET_DEVICE, OnResetDevice)
	ON_MESSAGE(WM_REARRANGERENDERLESS, OnRepaintRenderLess)

	ON_MESSAGE(WM_POSTOPEN, OnPostOpen)

	ON_MESSAGE_VOID(WM_SAVESETTINGS, SaveAppSettings)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_MESSAGE(WM_XBUTTONDOWN, OnXButtonDown)
	ON_MESSAGE(WM_XBUTTONUP, OnXButtonUp)
	ON_MESSAGE(WM_XBUTTONDBLCLK, OnXButtonDblClk)
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

	ON_UPDATE_COMMAND_UI(IDC_PLAYERSTATUS, OnUpdatePlayerStatus)

	ON_COMMAND(ID_BOSS, OnBossKey)

	ON_COMMAND_RANGE(ID_STREAM_AUDIO_NEXT, ID_STREAM_AUDIO_PREV, OnStreamAudio)
	ON_COMMAND_RANGE(ID_STREAM_SUB_NEXT, ID_STREAM_SUB_PREV, OnStreamSub)
	ON_COMMAND(ID_STREAM_SUB_ONOFF, OnStreamSubOnOff)
	ON_COMMAND_RANGE(ID_STREAM_VIDEO_NEXT, ID_STREAM_VIDEO_PREV, OnStreamVideo)

	ON_COMMAND(ID_FILE_OPENQUICK, OnFileOpenQuick)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENQUICK, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENMEDIA, OnFileOpenMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENMEDIA, OnUpdateFileOpen)
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
	ON_COMMAND(ID_FILE_SAVE_THUMBNAILS, OnFileSaveThumbnails)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_THUMBNAILS, OnUpdateFileSaveThumbnails)
	ON_COMMAND(ID_FILE_LOAD_SUBTITLE, OnFileLoadSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_SUBTITLE, OnUpdateFileLoadSubtitle)
	ON_COMMAND(ID_FILE_SAVE_SUBTITLE, OnFileSaveSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_SUBTITLE, OnUpdateFileSaveSubtitle)
	ON_COMMAND(ID_FILE_LOAD_AUDIO, OnFileLoadAudio)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_AUDIO, OnUpdateFileLoadAudio)
	ON_COMMAND(ID_FILE_ISDB_SEARCH, OnFileISDBSearch)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_SEARCH, OnUpdateFileISDBSearch)
	ON_COMMAND(ID_FILE_ISDB_DOWNLOAD, OnFileISDBDownload)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_DOWNLOAD, OnUpdateFileISDBDownload)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPLAYLIST, OnUpdateFileClose)
	ON_COMMAND(ID_FILE_CLOSEMEDIA, OnFileCloseMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEMEDIA, OnUpdateFileClose)
	ON_COMMAND(ID_REPEAT_FOREVER, OnRepeatForever)

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
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_MINIMAL, OnUpdateViewMinimal)
	ON_COMMAND(ID_VIEW_PRESETS_COMPACT, OnViewCompact)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_COMPACT, OnUpdateViewCompact)
	ON_COMMAND(ID_VIEW_PRESETS_NORMAL, OnViewNormal)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_COMMAND(ID_VIEW_FULLSCREEN_SECONDARY, OnViewFullscreenSecondary)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)

	ON_COMMAND(ID_WINDOW_TO_PRIMARYSCREEN, OnMoveWindowToPrimaryScreen)

	ON_UPDATE_COMMAND_UI(ID_VIEW_FULLSCREEN, OnUpdateViewFullscreen)
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
	ON_COMMAND_RANGE(ID_PANSCAN_ROTATEZP, ID_PANSCAN_ROTATEZM, OnViewRotate)
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
	ON_COMMAND(ID_VIEW_VSYNCACCURATE, OnViewVSyncAccurate)

	ON_COMMAND(ID_VIEW_EXCLUSIVE_FULLSCREEN, OnViewD3DFullScreen)
	ON_COMMAND(ID_VIEW_DISABLEDESKTOPCOMPOSITION, OnViewDisableDesktopComposition)
	ON_COMMAND(ID_VIEW_ALTERNATIVEVSYNC, OnViewAlternativeVSync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EXCLUSIVE_FULLSCREEN, OnUpdateViewD3DFullscreen)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DISABLEDESKTOPCOMPOSITION, OnUpdateViewDisableDesktopComposition)

	ON_COMMAND(ID_VIEW_RESET_DEFAULT, OnViewResetDefault)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RESET_DEFAULT, OnUpdateViewReset)

	ON_COMMAND(ID_VIEW_VSYNCOFFSET_INCREASE, OnViewVSyncOffsetIncrease)
	ON_COMMAND(ID_VIEW_VSYNCOFFSET_DECREASE, OnViewVSyncOffsetDecrease)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_TOGGLE, OnUpdateShaderToggle1)
	ON_COMMAND(ID_SHADERS_TOGGLE, OnShaderToggle1)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_TOGGLE_SCREENSPACE, OnUpdateShaderToggle2)
	ON_COMMAND(ID_SHADERS_TOGGLE_SCREENSPACE, OnShaderToggle2)

	ON_UPDATE_COMMAND_UI(ID_OSD_LOCAL_TIME, OnUpdateViewOSDLocalTime)
	ON_UPDATE_COMMAND_UI(ID_OSD_FILE_NAME, OnUpdateViewOSDFileName)
	ON_COMMAND(ID_OSD_LOCAL_TIME, OnViewOSDLocalTime)
	ON_COMMAND(ID_OSD_FILE_NAME, OnViewOSDFileName)

	ON_COMMAND(ID_VIEW_REMAINING_TIME, OnViewRemainingTime)
	ON_COMMAND(ID_D3DFULLSCREEN_TOGGLE, OnD3DFullscreenToggle)
	ON_COMMAND_RANGE(ID_GOTO_PREV_SUB, ID_GOTO_NEXT_SUB, OnGotoSubtitle)
	ON_COMMAND_RANGE(ID_SHIFT_SUB_DOWN, ID_SHIFT_SUB_UP, OnShiftSubtitle)
	ON_COMMAND_RANGE(ID_SUB_DELAY_DOWN, ID_SUB_DELAY_UP, OnSubtitleDelay)
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

	ON_COMMAND_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnPlayFrameStep)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnUpdatePlayFrameStep)
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
	ON_COMMAND_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnPlayFilters)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnUpdatePlayFilters)
	ON_COMMAND_RANGE(ID_SHADERS_START, ID_SHADERS_END, OnPlayShaders)
	ON_COMMAND(ID_MENU_NAVIGATE_AUDIO, OnMenuNavAudio)
	ON_COMMAND(ID_MENU_NAVIGATE_SUBTITLES, OnMenuNavSubtitle)
	ON_COMMAND(ID_MENU_NAVIGATE_JUMPTO, OnMenuNavJumpTo)
	ON_COMMAND(ID_MENU_RECENT_FILES, OnMenuRecentFiles)
	ON_COMMAND(ID_FAVORITES, OnMenuFavorites)

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
	ON_UPDATE_COMMAND_UI(ID_NORMALIZE, OnUpdateNormalizeVolume)
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
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnUpdateFavoritesFile)
	ON_COMMAND_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnFavoritesDVD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnUpdateFavoritesDVD)
	ON_COMMAND_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnFavoritesDevice)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnUpdateFavoritesDevice)

	ON_COMMAND(ID_RECENT_FILES_CLEAR, OnRecentFileClear)
	ON_UPDATE_COMMAND_UI(ID_RECENT_FILES_CLEAR, OnUpdateRecentFileClear)
	ON_COMMAND_RANGE(ID_RECENT_FILE_START, ID_RECENT_FILE_END, OnRecentFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_RECENT_FILE_START, ID_RECENT_FILE_END, OnUpdateRecentFile)

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

	ON_WM_WTSSESSION_CHANGE()
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
	m_fCustomGraph(false),
	m_fShockwaveGraph(false),
	m_fFrameSteppingActive(false),
	m_bEndOfStream(false),
	m_bGraphEventComplete(false),
	m_fCapturing(false),
	m_fLiveWM(false),
	m_fOpeningAborted(false),
	m_fBuffering(false),
	m_fileDropTarget(this),
	m_bTrayIcon(false),
	m_pFullscreenWnd(nullptr),
	m_pVideoWnd(nullptr),
	m_pOSDWnd(nullptr),
	m_bOSDLocalTime(false),
	m_bOSDFileName(false),
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
	m_bUseSmartSeek(false),
	m_flastnID(0),
	m_bfirstPlay(false),
	IsMadVRExclusiveMode(false),
	m_pBFmadVR(nullptr),
	m_hDWMAPI(0),
	m_hWtsLib(0),
	m_CaptureWndBitmap(nullptr),
	m_ThumbCashedBitmap(nullptr),
	m_nSelSub2(-1),
	m_hGraphThreadEventOpen(FALSE, TRUE),
	m_hGraphThreadEventClose(FALSE, TRUE),
	m_DwmGetWindowAttributeFnc(nullptr),
	m_DwmSetWindowAttributeFnc(nullptr),
	m_DwmSetIconicThumbnailFnc(nullptr),
	m_DwmSetIconicLivePreviewBitmapFnc(nullptr),
	m_DwmInvalidateIconicBitmapsFnc(nullptr),
	m_OSD(this),
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
	m_bMainIsMPEGSplitter(false)
{
	m_Lcd.SetVolumeRange(0, 100);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	UseCurentMonitorDPI(m_hWnd);

	m_popupMenu.LoadMenuW(IDR_POPUP);
	m_popupMainMenu.LoadMenuW(IDR_POPUPMAIN);

	CAppSettings& s = AfxGetAppSettings();

	// create a Main View Window
	if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						  CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST)) {
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

	m_bars.push_back(&m_wndSeekBar);
	m_bars.push_back(&m_wndToolBar);
	m_bars.push_back(&m_wndInfoBar);
	m_bars.push_back(&m_wndStatsBar);
	m_bars.push_back(&m_wndStatusBar);

	m_wndSeekBar.Enable(false);

	// dockable bars

	EnableDocking(CBRS_ALIGN_ANY);

	m_dockingbars.clear();

	m_wndSubresyncBar.Create(this, AFX_IDW_DOCKBAR_TOP, &m_csSubLock);
	m_wndSubresyncBar.SetBarStyle(m_wndSubresyncBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSubresyncBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndSubresyncBar.SetHeight(200);
	m_dockingbars.push_back(&m_wndSubresyncBar);

	m_wndPlaylistBar.Create(this, AFX_IDW_DOCKBAR_BOTTOM);
	m_wndPlaylistBar.SetBarStyle(m_wndPlaylistBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPlaylistBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndPlaylistBar.SetHeight(100);
	m_dockingbars.push_back(&m_wndPlaylistBar);
	m_wndPlaylistBar.LoadPlaylist(GetRecentFile());

	m_wndCaptureBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndCaptureBar.SetBarStyle(m_wndCaptureBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCaptureBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.push_back(&m_wndCaptureBar);

	m_wndNavigationBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndNavigationBar.SetBarStyle(m_wndNavigationBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndNavigationBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.push_back(&m_wndNavigationBar);

	m_wndShaderEditorBar.Create(this, AFX_IDW_DOCKBAR_TOP);
	m_wndShaderEditorBar.SetBarStyle(m_wndShaderEditorBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndShaderEditorBar.EnableDocking(CBRS_ALIGN_ANY);
	m_dockingbars.push_back(&m_wndShaderEditorBar);

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

#ifdef _WIN64
	m_strTitle.Format(L"%s x64 - v%s", ResStr(IDR_MAINFRAME), MPC_VERSION_WSTR);
#else
	m_strTitle.Format(L"%s - v%s", ResStr(IDR_MAINFRAME), MPC_VERSION_WSTR);
#endif
#if (MPC_VERSION_STATUS == 0)
	m_strTitle.AppendFormat(L" (build %d) beta",  MPC_VERSION_REV);
#endif

	SetWindowTextW(m_strTitle);
	m_Lcd.SetMediaTitle(m_strTitle);

	m_hWnd_toolbar = m_wndToolBar.GetSafeHwnd();

	WTSRegisterSessionNotification();

	s.strFSMonOnLaunch = s.strFullScreenMonitor;
	GetCurDispMode(s.dmFSMonOnLaunch, s.strFSMonOnLaunch);

	if (SysVersion::IsWin7orLater()) {
		m_hDWMAPI = LoadLibraryW(L"dwmapi.dll");
		if (m_hDWMAPI) {
			if (SysVersion::IsWin10orLater()) {
				(FARPROC &)m_DwmGetWindowAttributeFnc         = GetProcAddress(m_hDWMAPI, "DwmGetWindowAttribute");
			} else {
				(FARPROC &)m_DwmSetWindowAttributeFnc         = GetProcAddress(m_hDWMAPI, "DwmSetWindowAttribute");
				(FARPROC &)m_DwmSetIconicThumbnailFnc         = GetProcAddress(m_hDWMAPI, "DwmSetIconicThumbnail");
				(FARPROC &)m_DwmSetIconicLivePreviewBitmapFnc = GetProcAddress(m_hDWMAPI, "DwmSetIconicLivePreviewBitmap");
				(FARPROC &)m_DwmInvalidateIconicBitmapsFnc    = GetProcAddress(m_hDWMAPI, "DwmInvalidateIconicBitmaps");
			}
		}
	}

	// Windows 8 - if app is not pinned on the taskbar, it's not receive "TaskbarButtonCreated" message. Bug ???
	if (SysVersion::IsWin8orLater()) {
		CreateThumbnailToolbar();
	}

	m_DiskImage.Init();

	if (s.fAutoReloadExtSubtitles) {
		subChangeNotifyThread = std::thread([this] { subChangeNotifyThreadFunction(); });
		::SetThreadPriority(subChangeNotifyThread.native_handle(), THREAD_PRIORITY_LOWEST);
	}

	cmdLineThread = std::thread([this] { cmdLineThreadFunction(); });

	return 0;
}

void CMainFrame::OnPaint()
{
	CPaintDC dc(this);
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CMainFrame::OnDestroy()
{
	WTSUnRegisterSessionNotification();

	ShowTrayIcon(false);

	m_fileDropTarget.Revoke();

	if (m_pGraphThread) {
		CAMMsgEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_EXIT, 0, (LPARAM)&e);
		if (!e.Wait(5000)) {
			DLog(L"ERROR: Must call TerminateThread() on CMainFrame::m_pGraphThread->m_hThread");
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

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();

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
		SaveControlBars();
	}

	CloseMedia();

	ShowWindow(SW_HIDE);

	s.WinLircClient.DisConnect();
	s.UIceClient.DisConnect();

	SendAPICommand(CMD_DISCONNECT, L"\0"); // according to CMD_NOTIFYENDOFSTREAM (ctrl+f it here), you're not supposed to send NULL here

	if (s.AutoChangeFullscrRes.bEnabled && s.fRestoreResAfterExit) {
		SetDispMode(s.dmFSMonOnLaunch, s.strFSMonOnLaunch, TRUE);
	}

	if (m_hDWMAPI) {
		FreeLibrary(m_hDWMAPI);
	}

	m_InternalImage.Destroy();
	m_InternalImageSmall.Destroy();
	if (m_ThumbCashedBitmap) {
		::DeleteObject(m_ThumbCashedBitmap);
	}

	for (const auto& tmpFile : s.slTMPFilesList) {
		if (::PathFileExistsW(tmpFile)) {
			DeleteFileW(tmpFile);
		}
	}
	s.slTMPFilesList.clear();

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
	bool bUnicode = false;
	bool bUrl = false;

	if (pDataObject->IsDataAvailable(CF_HDROP)) {
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_HDROP)) {
			if (HDROP hDrop = (HDROP)GlobalLock(hGlobal)) {
				std::list<CString> slFiles;
				UINT nFiles = ::DragQueryFile(hDrop, UINT_MAX, nullptr, 0);
				for (UINT iFile = 0; iFile < nFiles; iFile++) {
					CString fn;
					fn.ReleaseBuffer(::DragQueryFile(hDrop, iFile, fn.GetBuffer(MAX_PATH), MAX_PATH));
					slFiles.push_back(fn);
				}
				::DragFinish(hDrop);
				DropFiles(slFiles);
				bResult = TRUE;
			}
			GlobalUnlock(hGlobal);
		}
	} else if (pDataObject->IsDataAvailable(CF_URLW)) {
		cfFormat = CF_URLW;
		bUnicode = true;
		bUrl = true;
	} else if (pDataObject->IsDataAvailable(CF_URLA)) {
		cfFormat = CF_URLA;
		bUrl = true;
	} else if (pDataObject->IsDataAvailable(CF_UNICODETEXT)) {
		cfFormat = CF_UNICODETEXT;
		bUnicode = true;
	} else if (pDataObject->IsDataAvailable(CF_TEXT)) {
		cfFormat = CF_TEXT;
	}

	if (cfFormat) {
		FORMATETC fmt = { cfFormat, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(cfFormat, &fmt)) {
			LPVOID LockData = GlobalLock(hGlobal);
			auto pUnicodeText = (LPCTSTR)LockData;
			auto pAnsiText = (LPCSTR)LockData;
			if (bUnicode ? AfxIsValidString(pUnicodeText) : AfxIsValidString(pAnsiText)) {
				CString text(bUnicode ? pUnicodeText : (LPCTSTR)pAnsiText);
				if (!text.IsEmpty()) {
					std::list<CString> slFiles;
					if (bUrl) {
						slFiles.push_back(text);
					} else {
						std::list<CString> lines;
						Explode(text, lines, L'\n');
						for (const auto& line : lines) {
							if (::PathIsURLW(line) || ::PathFileExistsW(line)) {
								slFiles.push_back(line);
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

LPCTSTR CMainFrame::GetRecentFile()
{
	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	MRU.ReadList();
	for (int i = 0; i < MRU.GetSize(); i++) {
		if (!MRU[i].IsEmpty()) {
			return MRU[i].GetString();
		}
	}
	return nullptr;
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
			break;

		case WM_LBUTTONDBLCLK:
			PostMessageW(WM_COMMAND, ID_FILE_OPENMEDIA);
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
	BOOL bWasVisible = ShowWindow(SW_HIDE);
	NOTIFYICONDATAW tnid;

	ZeroMemory(&tnid, sizeof(NOTIFYICONDATAW));
	tnid.cbSize = sizeof(NOTIFYICONDATAW);
	tnid.hWnd = m_hWnd;
	tnid.uID = IDR_MAINFRAME;

	if (fShow) {
		if (!m_bTrayIcon) {
			tnid.hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
			tnid.uCallbackMessage = WM_NOTIFYICON;
			StringCchCopyW(tnid.szTip, _countof(tnid.szTip), L"MPC-BE");
			Shell_NotifyIconW(NIM_ADD, &tnid);

			m_bTrayIcon = true;
		}
	} else {
		if (m_bTrayIcon) {
			Shell_NotifyIconW(NIM_DELETE, &tnid);

			m_bTrayIcon = false;
		}
	}

	if (bWasVisible) {
		ShowWindow(SW_SHOW);
	}
}

void CMainFrame::SetTrayTip(CString str)
{
	NOTIFYICONDATAW tnid;
	tnid.cbSize = sizeof(NOTIFYICONDATAW);
	tnid.hWnd = m_hWnd;
	tnid.uID = IDR_MAINFRAME;
	tnid.uFlags = NIF_TIP;
	StringCchCopyW(tnid.szTip, _countof(tnid.szTip), str);
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
			const bool bEscapeNotAssigned = !AssignedToCmd(VK_ESCAPE, m_bFullScreen, false);
			if (bEscapeNotAssigned
					&& (m_bFullScreen || IsD3DFullScreenMode())) {
				OnViewFullscreen();
				if (m_eMediaLoadState == MLS_LOADED) {
					PostMessageW(WM_COMMAND, ID_PLAY_PAUSE);
				}
				return TRUE;
			}
		} else if (pMsg->wParam == VK_LEFT && pAMTuner) {
			PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			return TRUE;
		} else if (pMsg->wParam == VK_RIGHT && pAMTuner) {
			PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			return TRUE;
		}
	} else if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_MENU
			&& CursorOnD3DFullScreenWindow()) {
		// disable pressing Alt on D3D FullScreen window.
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
							&& AfxGetAppSettings().iShowOSD & OSD_SEEKTIME) {

						REFERENCE_TIME stop;
						m_wndSeekBar.GetRange(stop);
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
									const bool bHighPrecision = !!m_wndSubresyncBar.IsWindowVisible();

									m_wndStatusBar.SetStatusTimer(rtNewPos, stop, bHighPrecision, GetTimeFormat());
									m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 1000);
									m_wndStatusBar.SetStatusTimer(rtPos, stop, bHighPrecision, GetTimeFormat());
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
								REFERENCE_TIME stop;
								m_wndSeekBar.GetRange(stop);
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
								while (PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, PM_REMOVE));
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
							int index = 0;
							for (int i = 0; i < _countof(m_touchScreen.point); i++) {
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
	SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

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
	const BOOL bMenuVisible = !IsMenuHidden();

	CSize cSize;
	CalcControlsSize(cSize);
	lpMMI->ptMinTrackSize = CPoint(cSize);

	if (bMenuVisible) {
		MENUBARINFO mbi = { sizeof(mbi) };
		GetMenuBarInfo(OBJID_MENU, 0, &mbi);

		// Calculate menu's horizontal length in pixels
		long x = GetSystemMetrics(SM_CYMENU) / 2; // free space after menu
		CRect r;
		for (UINT uItem = 0; ::GetMenuItemRect(m_hWnd, mbi.hMenu, uItem, &r); uItem++) {
			x += r.Width();
		}
		lpMMI->ptMinTrackSize.x = max(lpMMI->ptMinTrackSize.x, x);
	}

	if (IsWindow(m_wndToolBar.m_hWnd) && m_wndToolBar.IsVisible()) {
		lpMMI->ptMinTrackSize.x = std::max((LONG)m_wndToolBar.GetMinWidth(), lpMMI->ptMinTrackSize.x);
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

	if (IsMadVRExclusiveMode || !m_wndView.IsWindowVisible()) {
		if (m_wndFlyBar.IsWindowVisible()) {
			m_wndFlyBar.ShowWindow(SW_HIDE);
		}
		return false;
	}

	if (m_pBFmadVR) {
		CComQIPtr<IMadVRSettings> pMVS = m_pBFmadVR;
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

		CComQIPtr<IMadVRInfo> pMVRInfo = m_pBFmadVR;
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
		if (m_bAudioOnly && IsSomethingLoaded() && s.nAudioWindowMode == 2) {
			CRect rc;
			GetWindowRect(&rc);
			s.rcLastWindowPos.left  = rc.left;
			s.rcLastWindowPos.top   = rc.top;
			s.rcLastWindowPos.right = rc.right;
		} else {
			GetWindowRect(s.rcLastWindowPos);
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

	prc->left   = max(rcWork.left, min(rcWork.right-w, cur_pos.x - (w/2)));
	prc->top    = max(rcWork.top,  min(rcWork.bottom-h, cur_pos.y - (h/2)));
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
			CAppSettings& s = AfxGetAppSettings();
			if (m_bAudioOnly && IsSomethingLoaded() && s.nAudioWindowMode == 2) {
				CRect rc;
				GetWindowRect(&rc);
				s.rcLastWindowPos.left = rc.left;
				s.rcLastWindowPos.top = rc.top;
				s.rcLastWindowPos.right = rc.right;
			}
			else {
				GetWindowRect(s.rcLastWindowPos);
			}
		}
		s.nLastWindowType = nType;

		// maximized window in MODE_FRAMEONLY | MODE_BORDERLESS is not correct
		if (nType == SIZE_MAXIMIZED && (s.iCaptionMenuMode == MODE_FRAMEONLY || s.iCaptionMenuMode == MODE_BORDERLESS)) {
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
			CRect wr(mi.rcWork);
			if (s.iCaptionMenuMode == MODE_FRAMEONLY && SysVersion::IsWin10orLater()) {
				CRect invisibleBorders = GetInvisibleBorderSize();
				wr.InflateRect(invisibleBorders);
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
	VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), !IsMenuHidden(), GetWindowExStyle(m_hWnd)));
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
		HMONITOR hMonitor = MonitorFromWindow(cwnd->m_hWnd, 0);
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

LRESULT CMainFrame::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	OverrideDPI(LOWORD(wParam), HIWORD(wParam));

	m_wndToolBar.ScaleToolbar();
	m_wndInfoBar.ScaleFont();
	m_wndStatsBar.ScaleFont();
	m_wndPlaylistBar.ScaleFont();
	m_wndStatusBar.ScaleFont();

	MoveWindow(reinterpret_cast<RECT*>(lParam));
	RecalcLayout();

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
		ShowWindow(SW_HIDE);
		return;
	} else if ((nID & 0xFFF0) == SC_MINIMIZE) {
		if (m_bAudioOnly && m_DwmSetIconicLivePreviewBitmapFnc) {
			isWindowMinimized = true;
			CreateCaptureWindow();
		}
	} else if ((nID & 0xFFF0) == SC_RESTORE) {
		if (m_bAudioOnly && m_DwmSetIconicLivePreviewBitmapFnc) {
			isWindowMinimized = false;
		}
	} else if ((nID & 0xFFF0) == SC_MAXIMIZE && m_bFullScreen) {
		ToggleFullscreen(true, true);
	}

	__super::OnSysCommand(nID, lParam);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	__super::OnActivateApp(bActive, dwThreadID);

	if (m_bFullScreen) {
		if (bActive) {
			SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
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

			if (CWnd* pActiveWnd = GetForegroundWindow()) {
				bool bExitFullscreen = s.fExitFullScreenAtFocusLost;
				if (bExitFullscreen) {
					HMONITOR hMonitor1 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
					HMONITOR hMonitor2 = MonitorFromWindow(pActiveWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
					if (hMonitor1 && hMonitor2 && hMonitor1 != hMonitor2) {
						bExitFullscreen = false;
					}
				}

				CString title;
				pActiveWnd->GetWindowTextW(title);

				CString module;

				DWORD pid;
				GetWindowThreadProcessId(pActiveWnd->m_hWnd, &pid);

				if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
					HMODULE hModule;
					DWORD cbNeeded;

					if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
						module.ReleaseBufferSetLength(GetModuleFileNameEx(hProcess, hModule, module.GetBuffer(MAX_PATH), MAX_PATH));
					}

					CloseHandle(hProcess);
				}

				if (!title.IsEmpty() && !module.IsEmpty()) {
					CString str;
					str.Format(ResStr(IDS_MAINFRM_2), GetFileOnly(module).MakeLower(), title);
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
		CAppSettings& s = AfxGetAppSettings();

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
	CAppSettings&	s			= AfxGetAppSettings();
	UINT			nMceCmd		= 0;

	nMceCmd = AfxGetMyApp()->GetRemoteControlCode (nInputcode, hRawInput);
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
	CAppSettings& s = AfxGetAppSettings();
	BOOL fRet = FALSE;

	if (GetActiveWindow() == this || s.bGlobalMedia == TRUE) {
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
			break;
		case TIMER_STREAMPOSPOLLER:
			if (m_eMediaLoadState == MLS_LOADED) {

				if (m_bFullScreen) {
					// Get the position of the cursor outside the window in fullscreen mode
					POINT m_pos;
					GetCursorPos(&m_pos);
					ScreenToClient(&m_pos);
					CRect r;
					GetClientRect(r);

					if (!r.PtInRect(m_pos) && (m_lastMouseMoveFullScreen.x != m_pos.x || m_lastMouseMoveFullScreen.y != m_pos.y)) {
						m_lastMouseMoveFullScreen.x = m_pos.x;
						m_lastMouseMoveFullScreen.y = m_pos.y;
						OnMouseMove(0, m_pos);
					}
				}

				g_bForcedSubtitle = AfxGetAppSettings().fForcedSubtitles;
				REFERENCE_TIME rtNow = 0, rtDur = 0;

				switch (GetPlaybackMode()) {
					case PM_FILE:
						g_bExternalSubtitleTime = false;

						m_pMS->GetDuration(&rtDur);
						m_pMS->GetCurrentPosition(&rtNow);

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

						m_wndStatusBar.SetStatusTimer(rtNow, rtDur, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
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
						m_wndStatusBar.SetStatusTimer(rtNow, rtDur, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
						break;
					case PM_CAPTURE:
						g_bExternalSubtitleTime = true;
						if (m_fCapturing) {
							if (m_wndCaptureBar.m_capdlg.m_pMux) {
								CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
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
				m_OSD.SetRange(0, rtDur);
				m_OSD.SetPos(rtNow);
				m_Lcd.SetMediaRange(0, rtDur);
				m_Lcd.SetMediaPos(rtNow);

				if (m_pCAP) {
					if (g_bExternalSubtitleTime) {
						m_pCAP->SetTime(rtNow);
					}
					m_wndSubresyncBar.SetTime(rtNow);
					m_wndSubresyncBar.SetFPS(m_pCAP->GetFPS());
				}

				if (m_DwmInvalidateIconicBitmapsFnc) {
					m_DwmInvalidateIconicBitmapsFnc(m_hWnd);
				}
			}
			break;
		case TIMER_STREAMPOSPOLLER2:
			if (m_eMediaLoadState == MLS_LOADED) {
				if (AfxGetAppSettings().nCS < CS_STATUSBAR) {
					AfxGetAppSettings().bStatusBarIsVisible = false;
				} else {
					AfxGetAppSettings().bStatusBarIsVisible = true;
				}

				if (GetPlaybackMode() == PM_FILE && !m_bGraphEventComplete) {
					CAppSettings& s = AfxGetAppSettings();
					if (s.bKeepHistory && s.bRememberFilePos) {
						if (FILE_POSITION* FilePosition = s.CurrentFilePosition()) {
							REFERENCE_TIME rtDur;
							m_pMS->GetDuration(&rtDur);
							if (rtDur > 0) {
								REFERENCE_TIME rtNow;
								m_pMS->GetCurrentPosition(&rtNow);

								const bool bSave = llabs(FilePosition->llPosition - rtNow) > 300000000LL;
								if (bSave) {
									FilePosition->llPosition = rtNow;
									FilePosition->nAudioTrack = GetAudioTrackIdx();
									FilePosition->nSubtitleTrack = GetSubtitleTrackIdx();

									s.SaveCurrentFilePosition();
								}
							}
						}
					}
				}

				if (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing) {
					CString str = ResStr(IDS_CAPTURE_LIVE);

					long lChannel = 0, lVivSub = 0, lAudSub = 0;
					if (pAMTuner
							&& m_wndCaptureBar.m_capdlg.IsTunerActive()
							&& SUCCEEDED(pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub))) {
						CString ch;
						ch.Format(L" (ch%d)", lChannel);
						str += ch;
					}

					m_wndStatusBar.SetStatusTimer(str);
				} else {
					CString str_temp;
					bool bmadvr = (GetRenderersSettings().iVideoRenderer == VIDRNDT_MADVR);

					if (m_bOSDLocalTime) {
						str_temp = GetSystemLocalTime();
					}

					if (AfxGetAppSettings().bOSDRemainingTime) {
						str_temp.GetLength() > 0 ? str_temp += L"\n" + m_wndStatusBar.GetStatusTimer() : str_temp = m_wndStatusBar.GetStatusTimer();
					}

					if (m_bOSDFileName) {
						CString strOSD;
						if (!m_youtubeFields.title.IsEmpty()) {
							strOSD = m_youtubeFields.title;
						} else if (::PathIsURLW(GetCurFileName())) {
							CPlaylistItem pli;
							if (m_wndPlaylistBar.GetCur(pli) && !pli.m_label.IsEmpty()) {
								strOSD = pli.m_label;
							}

							if (strOSD.IsEmpty()) {
								BeginEnumFilters(m_pGB, pEF, pBF) {
									if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
										if (!CheckMainFilter(pBF)) {
											continue;
										}

										CComBSTR bstr;
										if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
											strOSD = bstr.m_str;
											break;
										}
									}
								}
								EndEnumFilters;
							}
						}

						if (strOSD.IsEmpty()) {
							strOSD = m_strPlaybackLabel;
						}

						str_temp.GetLength() > 0 ? str_temp += L"\n" + strOSD : str_temp = strOSD;
					}

					if (bmadvr) {
						str_temp.Replace(L"\n", L" / "); // MadVR support only singleline OSD message
					}

					if (str_temp.GetLength() > 0) {
						m_OSD.DisplayMessage(OSD_TOPLEFT, str_temp);
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
		case TIMER_FULLSCREENMOUSEHIDER: {
			CPoint p;
			GetCursorPos(&p);

			CRect r;
			GetWindowRect(r);
			bool fCursorOutside = !r.PtInRect(p);
			CWnd* pWnd = WindowFromPoint(p);
			if (IsD3DFullScreenMode()) {
				if (!m_bInOptions && *pWnd == *m_pFullscreenWnd) {
					m_pFullscreenWnd->ShowCursor(false);
				}
				KillTimer(TIMER_FULLSCREENMOUSEHIDER);
			} else {
				if (pWnd && !m_bInOptions && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside)) {
					m_bHideCursor = true;
					SetCursor(nullptr);
				}
			}
		}
		break;
		case TIMER_STATS: {
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

			if (m_pBI) {
				std::list<CString> sl;

				for (int i = 0, j = m_pBI->GetCount(); i < j; i++) {
					int samples, size;
					if (S_OK == m_pBI->GetStatus(i, samples, size)) {
						CString str;
						str.Format(L"[%d]: %03d/%d KB", i, samples, size / 1024);
						sl.push_back(str);
					}
				}

				if (!sl.empty()) {
					CString str;
					str.Format(L"%s (p%d)", Implode(sl, ' '), m_pBI->GetPriority());

					m_wndStatsBar.SetLine(ResStr(IDS_AG_BUFFERS), str);
				}
			}

			CInterfaceList<IBitRateInfo> pBRIs;

			BeginEnumFilters(m_pGB, pEF, pBF) {
				BeginEnumPins(pBF, pEP, pPin) {
					if (CComQIPtr<IBitRateInfo> pBRI = pPin) {
						pBRIs.AddTail(pBRI);
					}
				}
				EndEnumPins;

				if (!pBRIs.IsEmpty()) {
					std::list<CString> sl;

					POSITION pos = pBRIs.GetHeadPosition();
					for (int i = 0; pos; i++) {
						IBitRateInfo* pBRI = pBRIs.GetNext(pos);

						DWORD cur = pBRI->GetCurrentBitRate() / 1000;
						DWORD avg = pBRI->GetAverageBitRate() / 1000;

						if (avg == 0) {
							continue;
						}

						CString str;
						if (cur != avg) {
							str.Format(L"[%d]: %u/%u Kb/s", i, avg, cur);
						} else {
							str.Format(L"[%d]: %u Kb/s", i, avg);
						}
						sl.push_back(str);
					}

					if (!sl.empty()) {
						m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_BITRATE), Implode(sl, ' ') + ResStr(IDS_STATSBAR_BITRATE_AVG_CUR));
					}

					break;
				}
			}
			EndEnumFilters;

			if (GetPlaybackMode() == PM_DVD) { // we also use this timer to update the info panel for DVD playback
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
						lang.ReleaseBufferSetLength(max(len-1, 0));
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
					lang.ReleaseBufferSetLength(max(len-1, 0));

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

					Subtitles.Format(L"%s",
									 lang);
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), Subtitles);
			}
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
	if (s.bKeepHistory && s.bRememberFilePos) {
		if (FILE_POSITION* FilePosition = s.CurrentFilePosition()) {
			FilePosition->llPosition = 0;
			FilePosition->nAudioTrack = -1;
			FilePosition->nSubtitleTrack = -1;

			s.SaveCurrentFilePosition();
		}
	}

	if (m_wndPlaylistBar.GetCount() <= 1) {
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
	} else if (m_wndPlaylistBar.GetCount() > 1) {
		if (m_wndPlaylistBar.IsAtEnd()) {
			if (DoAfterPlaybackEvent()) {
				return false;
			}

			m_nLoops++;
		}

		if (s.fLoopForever || m_nLoops < s.nLoops) {
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

		if (m_fCustomGraph) {
			if (EC_BG_ERROR == evCode) {
				str = CString((char*)evParam1);
			}
		}

		if (!m_fFrameSteppingActive) {
			m_nStepForwardCount = 0;
		}

		hr = m_pME->FreeEventParams(evCode, evParam1, evParam2);

		switch (evCode) {
			case EC_COMPLETE:
				if (!GraphEventComplete()) {
					return hr;
				}
				break;
			case EC_ERRORABORT:
				DLog(L"OnGraphNotify: EC_ERRORABORT -> hr = %08x", (HRESULT)evParam1);
				break;
			case EC_BUFFERING_DATA:
				DLog(L"OnGraphNotify: EC_BUFFERING_DATA -> %d, %d", evParam1, evParam2);

				m_fBuffering = ((HRESULT)evParam1 != S_OK);
				break;
			case EC_STEP_COMPLETE:
				if (m_fFrameSteppingActive) {
					m_nStepForwardCount++;
					m_fFrameSteppingActive = false;
					m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
				}
				break;
			case EC_DEVICE_LOST:
				if (GetPlaybackMode() == PM_CAPTURE && evParam2 == 0) {
					CComQIPtr<IBaseFilter> pBF = (IUnknown*)evParam1;
					if (!pVidCap && pVidCap == pBF || !pAudCap && pAudCap == pBF) {
						SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					}
				}
				break;
			case EC_DVD_TITLE_CHANGE:
				if (m_pDVDC) {
					if (m_PlaybackRate != 1.0) {
						OnPlayResetRate();
					}

					// Save current chapter
					DVD_POSITION* DvdPos = s.CurrentDVDPosition();
					if (DvdPos) {
						DvdPos->lTitle = (DWORD)evParam1;
					}

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
					OpenDVDData* pDVDData = dynamic_cast<OpenDVDData*>(m_lastOMD.m_p);
					ASSERT(pDVDData);

					CString Domain('-');

					switch (m_iDVDDomain) {
						case DVD_DOMAIN_FirstPlay:
							ULONGLONG	llDVDGuid;

							Domain = L"First Play";

							if (s.bShowDebugInfo) {
								m_OSD.DebugMessage(L"%s", Domain);
							}

							if (m_pDVDI && SUCCEEDED (m_pDVDI->GetDiscID (nullptr, &llDVDGuid))) {
								m_fValidDVDOpen = true;

								if (s.bShowDebugInfo) {
									m_OSD.DebugMessage(L"DVD Title: %d", s.lDVDTitle);
								}

								if (s.lDVDTitle != 0) {
									s.NewDvd (llDVDGuid);
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
								} else if (s.bKeepHistory && s.bRememberDVDPos && !s.NewDvd(llDVDGuid)) {
									// Set last remembered position (if founded...)
									DVD_POSITION* DvdPos = s.CurrentDVDPosition();

									if (SUCCEEDED(hr = m_pDVDC->PlayTitle(DvdPos->lTitle, DVD_CMD_FLAG_Flush, nullptr))) {
										if (SUCCEEDED(hr = m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr))) {
											hr = m_pDVDC->PlayAtTime(&DvdPos->Timecode, DVD_CMD_FLAG_Flush, nullptr);
										} else {
											if (SUCCEEDED(hr = m_pDVDC->PlayChapterInTitle(DvdPos->lTitle, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr))) {
												if (FAILED(hr = m_pDVDC->PlayAtTime(&DvdPos->Timecode, DVD_CMD_FLAG_Flush, nullptr))) {
													hr = m_pDVDC->PlayAtTimeInTitle(DvdPos->lTitle, &DvdPos->Timecode, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
												}
											}
										}
									}

									if (SUCCEEDED(hr)) {
										m_iDVDTitle = DvdPos->lTitle;
									}
								} else if (s.bStartMainTitle && s.bNormalStartDVD) {
									m_pDVDC->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
								}
								s.bNormalStartDVD = true;
								if (s.bRememberZoomLevel && !m_bFullScreen && !IsD3DFullScreenMode()) { // Hack to the normal initial zoom for DVD + DXVA ...
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
							DVD_POSITION* DvdPos;
							DvdPos = s.CurrentDVDPosition();
							if (DvdPos) {
								DvdPos->lTitle = m_iDVDTitle;
							}

							if (!m_fValidDVDOpen) {
								m_fValidDVDOpen = true;
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
					DVD_POSITION* DvdPos = s.CurrentDVDPosition();
					if (DvdPos) {
						memcpy(&DvdPos->Timecode, (void*)&evParam1, sizeof(DVD_HMSF_TIMECODE));
					}
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

				if (s.bRememberZoomLevel
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
				if (m_fCustomGraph) {
					int nAudioChannels = evParam1;

					m_wndStatusBar.SetStatusBitmap(nAudioChannels == 1 ? IDB_MONO
												   : nAudioChannels >= 2 ? IDB_STEREO
												   : IDB_NOAUDIO);
				}
				break;
			case EC_BG_ERROR:
				if (m_fCustomGraph) {
					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					m_closingmsg = !str.IsEmpty() ? str : L"Unspecified graph error";
					m_wndPlaylistBar.SetCurValid(false);
					return hr;
				}
				break;
			case EC_DVD_PLAYBACK_RATE_CHANGE:
				if (m_pDVDC) {
					if (m_fCustomGraph && s.AutoChangeFullscrRes.bEnabled == 1 &&
							(m_bFullScreen || IsD3DFullScreenMode()) && m_iDVDDomain == DVD_DOMAIN_Title) {
						AutoChangeMonitorMode();
					}
				}
				break;
		}
	}

	return hr;
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

LRESULT CMainFrame::OnRepaintRenderLess(WPARAM wParam, LPARAM lParam)
{
	MoveVideoWindow();
	return TRUE;
}

LRESULT CMainFrame::OnPostOpen(WPARAM wParam, LPARAM lParam)
{
	CAutoPtr<OpenMediaData> pOMD;
	pOMD.Attach((OpenMediaData*)wParam);

	CAppSettings& s = AfxGetAppSettings();

	m_nAudioTrackStored = m_nSubtitleTrackStored = -1;

	if (!m_closingmsg.IsEmpty()) {
		CString aborted(ResStr(IDS_AG_ABORTED));

		CloseMedia();

		if (m_closingmsg != aborted) {

			if (OpenFileData *pFileData = dynamic_cast<OpenFileData*>(pOMD.m_p)) {
				m_wndPlaylistBar.SetCurValid(false);

				if (GetAsyncKeyState(VK_ESCAPE)) {
					m_closingmsg = aborted;
				}

				if (m_closingmsg != aborted) {
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
	if (!PeekMessage(&msg, m_hWnd, WM_SAVESETTINGS, WM_SAVESETTINGS, PM_NOREMOVE | PM_NOYIELD)) {
		AfxGetAppSettings().SaveSettings();
	}
}

BOOL CMainFrame::OnButton(UINT id, UINT nFlags, CPoint point)
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

	if (id != wmcmd::WDOWN && id != wmcmd::WUP && !r.PtInRect(point)) {
		return FALSE;
	}

	BOOL ret = FALSE;

	WORD cmd = AssignedToCmd(id, m_bFullScreen);
	if (cmd) {
		SendMessageW(WM_COMMAND, cmd);
		ret = TRUE;
	}

	return ret;
}

void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
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
		if (AssignedToCmd(wmcmd::LDOWN, m_bFullScreen)) {
			m_bLeftMouseDownFullScreen = TRUE;
			OnButton(wmcmd::LDOWN, nFlags, point);
		}
		return;
	}

	templclick = false;
	SetCapture();

	__super::OnLButtonDown(nFlags, point);
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	if (m_bLeftMouseDownFullScreen) {
		m_bLeftMouseDownFullScreen = FALSE;
		return;
	}

	if (!m_bLeftMouseDown) {
		return;
	}

	bool fLeftDownMouseBtnUnassigned = !AssignedToCmd(wmcmd::LDOWN, m_bFullScreen);
	if (!fLeftDownMouseBtnUnassigned && !m_bFullScreen && !CursorOnD3DFullScreenWindow()) {
		OnButton(wmcmd::LDOWN, nFlags, point);
		return;
 	}

	bool fLeftUpMouseBtnUnassigned = !AssignedToCmd(wmcmd::LUP, m_bFullScreen);
	if (!fLeftUpMouseBtnUnassigned && !m_bFullScreen) {
		OnButton(wmcmd::LUP, nFlags, point);
		return;
 	}

	__super::OnLButtonUp(nFlags, point);
}

void CMainFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{

	if (m_bLeftMouseDown) {
		OnButton(wmcmd::LDOWN, nFlags, point);
		m_bLeftMouseDown = FALSE;
	}

	if (!OnButton(wmcmd::LDBLCLK, nFlags, point)) {
		__super::OnLButtonDblClk(nFlags, point);
	}
}

void CMainFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendMessageW(WM_CANCELMODE);
	if (!OnButton(wmcmd::MDOWN, nFlags, point)) {
		__super::OnMButtonDown(nFlags, point);
	}
}

void CMainFrame::OnMButtonUp(UINT nFlags, CPoint point)
{
	if (!OnButton(wmcmd::MUP, nFlags, point)) {
		__super::OnMButtonUp(nFlags, point);
	}
}

void CMainFrame::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessageW(WM_MBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	if (!OnButton(wmcmd::MDBLCLK, nFlags, point)) {
		__super::OnMButtonDblClk(nFlags, point);
	}
}

void CMainFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (!OnButton(wmcmd::RDOWN, nFlags, point)) {
		__super::OnRButtonDown(nFlags, point);
	}
}

void CMainFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint p;
	GetCursorPos(&p);
	SetFocus();
	if (*WindowFromPoint(p) != *m_pFullscreenWnd && !OnButton(wmcmd::RUP, nFlags, point)) {
		__super::OnRButtonUp(nFlags, point);
	}
}

void CMainFrame::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessageW(WM_RBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	if (!OnButton(wmcmd::RDBLCLK, nFlags, point)) {
		__super::OnRButtonDblClk(nFlags, point);
	}
}

LRESULT CMainFrame::OnXButtonDown(WPARAM wParam, LPARAM lParam)
{
	SendMessageW(WM_CANCELMODE);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam);
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DOWN : fwButton == XBUTTON2 ? wmcmd::X2DOWN : wmcmd::NONE,
					GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonUp(WPARAM wParam, LPARAM lParam)
{
	UINT fwButton = GET_XBUTTON_WPARAM(wParam);
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1UP : fwButton == XBUTTON2 ? wmcmd::X2UP : wmcmd::NONE,
					GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonDblClk(WPARAM wParam, LPARAM lParam)
{
	SendMessageW(WM_XBUTTONDOWN, wParam, lParam);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam);
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DBLCLK : fwButton == XBUTTON2 ? wmcmd::X2DBLCLK : wmcmd::NONE,
					GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
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
		zDelta > 0 ? OnButton(wmcmd::WUP, nFlags, point) :
		zDelta < 0 ? OnButton(wmcmd::WDOWN, nFlags, point) :
		FALSE;

	return fRet;
}

void CMainFrame::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta) {
		ScreenToClient(&pt);
		OnButton((zDelta < 0) ? wmcmd::WLEFT : wmcmd::WRIGHT, nFlags, pt);
	}
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
	// Waffs : ignore mousemoves when entering fullscreen
	if (m_lastMouseMove.x == -1 && m_lastMouseMove.y == -1) {
		m_lastMouseMove.x = point.x;
		m_lastMouseMove.y = point.y;
	}

	CWnd* w = GetCapture();
	if (w && w->m_hWnd == m_hWnd && (nFlags & MK_LBUTTON) && templclick) {
		ReleaseCapture();

		if (AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS) {
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

		SendMessageW(WM_NCLBUTTONDOWN, HTCAPTION, 0);
 	}
	templclick = true;

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

	CSize diff = m_lastMouseMove - point;
	CAppSettings& s = AfxGetAppSettings();

	if (IsD3DFullScreenMode() && (abs(diff.cx) + abs(diff.cy)) >= 1) {
		m_pFullscreenWnd->ShowCursor(true);

		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, nullptr);
	} else if (m_bFullScreen && (abs(diff.cx) + abs(diff.cy)) >= 1) {
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
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, nullptr);
		} else if (nTimeOut == 0) {
			CRect r;
			GetClientRect(r);
			r.top = r.bottom;

			int i = 1;
			for (const auto& pBar : m_bars) {
				CSize size = pBar->CalcFixedLayout(FALSE, TRUE);
				if (s.nCS&i) {
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

			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, nullptr);
		} else {
			m_bHideCursor = false;
			if (s.fShowBarsWhenFullScreen) {
				ShowControls(s.nCS);
			}

			SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut * 1000, nullptr);
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut * 1000, 2000), nullptr);
		}
	}

	m_lastMouseMove = point;

	__super::OnMouseMove(nFlags, point);
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar->IsKindOf(RUNTIME_CLASS(CVolumeCtrl))) {
		OnPlayVolume(0);
	} else if (pScrollBar->IsKindOf(RUNTIME_CLASS(CPlayerSeekBar)) && m_eMediaLoadState == MLS_LOADED) {
		SeekTo(m_wndSeekBar.GetPos());
	} else {
		SeekTo(m_OSD.GetPos());
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

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for (UINT i = 0; i < uiMenuCount; ++i) {
#ifdef _DEBUG
		CString str;
		pMenu->GetMenuString(i, str, MF_BYPOSITION);
		str.Remove('&');
#endif
		UINT itemID = pMenu->GetMenuItemID(i);
		if (itemID == 0xFFFFFFFF) {
			mii.fMask = MIIM_ID;
			pMenu->GetMenuItemInfo(i, &mii, TRUE);
			itemID = mii.wID;
		}

		CMenu* pSubMenu = nullptr;

		if (itemID == ID_FAVORITES) {
			SetupFavoritesSubMenu();
			pSubMenu = &m_favoritesMenu;
		}/*else if(itemID == ID_RECENT_FILES) {
			SetupRecentFilesSubMenu();
			pSubMenu = &m_recentfilesMenu;
		}*/

		if (pSubMenu) {
			mii.fMask = MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
			mii.fType = MF_POPUP;
			mii.wID = itemID; // save ID after set popup type
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
			pMenu->SetMenuItemInfo(i, &mii, TRUE);
		}
	}
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	__super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	UINT uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == -1) {
		return;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for (UINT i = 0; i < uiMenuCount; ++i) {
#ifdef _DEBUG
		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);
		str.Remove('&');
#endif
		UINT firstSubItemID = 0;
		CMenu* sm = pPopupMenu->GetSubMenu(i);
		if (sm) {
			firstSubItemID= sm->GetMenuItemID(0);
		}

		if (firstSubItemID == ID_NAVIGATE_SKIPBACK) { // is "Navigate" submenu {
			UINT fState = (m_eMediaLoadState == MLS_LOADED
						   && (1/*GetPlaybackMode() == PM_DVD *//*|| (GetPlaybackMode() == PM_FILE && m_PlayList.GetCount() > 0)*/))
						  ? MF_ENABLED
						  : (MF_DISABLED | MF_GRAYED);
			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
			continue;
		}
		if (firstSubItemID == ID_VIEW_INCSIZE           // is "Pan&Scan" submenu
				|| firstSubItemID == ID_VIEW_ZOOM_50) { // is "Zoom" submenu
			UINT fState = (m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly)
						  ? MF_ENABLED
						  : (MF_DISABLED | MF_GRAYED);
			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
			continue;
		}

		UINT itemID = pPopupMenu->GetMenuItemID(i);
		if (itemID == 0xFFFFFFFF) {
			mii.fMask = MIIM_ID;
			pPopupMenu->GetMenuItemInfo(i, &mii, TRUE);
			itemID = mii.wID;
		}
		CMenu* pSubMenu = nullptr;

		if (itemID == ID_FILE_OPENDISC) {
			SetupOpenCDSubMenu();
			pSubMenu = &m_openCDsMenu;
		} else if (itemID == ID_FILTERS) {
			SetupFiltersSubMenu();
			pSubMenu = &m_filtersMenu;
		} else if (itemID == ID_MENU_LANGUAGE) {
			SetupLanguageMenu();
			pSubMenu = &m_languageMenu;
		} else if (itemID == ID_AUDIOS) {
			SetupAudioSubMenu();
			pSubMenu = &m_AudioMenu;
		} else if (itemID == ID_SUBTITLES) {
			SetupSubtitlesSubMenu();
			pSubMenu = &m_SubtitlesMenu;
		} else if (itemID == ID_VIDEOSTREAMS) {
			CString menu_str = GetPlaybackMode() == PM_DVD ? ResStr(IDS_MENU_VIDEO_ANGLE) : ResStr(IDS_MENU_VIDEO_STREAM);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = (LPTSTR)(LPCTSTR)menu_str;
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
			SetupVideoStreamsSubMenu();
			pSubMenu = &m_VideoStreamsMenu;
		} else if (itemID == ID_JUMPTO) {
			SetupNavChaptersSubMenu();
			pSubMenu = &m_chaptersMenu;
		} else if (itemID == ID_FAVORITES) {
			SetupFavoritesSubMenu();
			pSubMenu = &m_favoritesMenu;
		} else if (itemID == ID_RECENT_FILES) {
			SetupRecentFilesSubMenu();
			pSubMenu = &m_recentfilesMenu;
		} else if (itemID == ID_SHADERS) {
			SetupShadersSubMenu();
			pSubMenu = &m_shadersMenu;
		}

		if (pSubMenu) {
			mii.fMask = MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
			mii.fType = MF_POPUP;
			mii.wID = itemID; // save ID after set popup type
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
			//continue;
		}
	}

	//

	uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == -1) {
		return;
	}

	for (__int64 i = 0; i < uiMenuCount; ++i) {  //temp inst UINT -> __int64
		__int64 nID = pPopupMenu->GetMenuItemID(i); //temp ren UINT -> __int64
		if (nID == ID_SEPARATOR || nID == -1
				|| (nID >= ID_FAVORITES_FILE_START && nID <= ID_FAVORITES_FILE_END)
				|| (nID >= ID_FAVORITES_DVD_START && nID <= ID_FAVORITES_DVD_END)
				|| (nID >= ID_RECENT_FILE_START && nID <= ID_RECENT_FILE_END)
				|| (nID >= ID_NAVIGATE_CHAP_SUBITEM_START && nID <= ID_NAVIGATE_CHAP_SUBITEM_END)) {
			continue;
		}

		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);
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
		MENUITEMINFO mii;
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING;
		mii.dwTypeData	= (LPTSTR)(LPCTSTR)str;
		pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
	}

	//

	uiMenuCount = pPopupMenu->GetMenuItemCount();
	if (uiMenuCount == -1) {
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
		CAppSettings& s = AfxGetAppSettings();
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
}

void CMainFrame::OnUnInitMenuPopup(CMenu* pPopupMenu, UINT nFlags)
{
	__super::OnUnInitMenuPopup(pPopupMenu, nFlags);

	MSG msg;
	PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);

	m_bLeftMouseDown = FALSE;
}

BOOL CMainFrame::OnMenu(CMenu* pMenu)
{
	CheckPointer(pMenu, FALSE);

	CPoint point;
	GetCursorPos(&point);

	// Do not show popup menu in D3D fullscreen it has several adverse effects.
	if (IsD3DFullScreenMode()) {
		CWnd* pWnd = WindowFromPoint(point);
		if (pWnd && *pWnd == *m_pFullscreenWnd) {
			return FALSE;
		}
	}

	if (m_bClosingState) {
		return FALSE; //prevent crash when player closes with context menu open
	}

	KillTimer(TIMER_FULLSCREENMOUSEHIDER);
	m_bHideCursor = false;

	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NOANIMATION, point.x + 1, point.y + 1, this);

	if (m_bFullScreen) {
		SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, nullptr);    //need when working with menus and use the keyboard only
	}

	return TRUE;
}

void CMainFrame::OnMenuPlayerShort()
{
	OnMenu((IsMenuHidden() || IsD3DFullScreenMode()) ? m_popupMainMenu.GetSubMenu(0) : m_popupMenu.GetSubMenu(0));
}

void CMainFrame::OnMenuPlayerLong()
{
	OnMenu(m_popupMainMenu.GetSubMenu(0));
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
			else if (fs == State_Paused || m_fFrameSteppingActive) {
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

void CMainFrame::OnFilePostOpenMedia(CAutoPtr<OpenMediaData> pOMD)
{
	ASSERT(m_eMediaLoadState == MLS_LOADING);
	SetLoadState(MLS_LOADED);

	// destroy invisible top-level d3dfs window if there is no video renderer
	if (IsD3DFullScreenMode() && !m_pMFVDC && !m_pVMRWC) {
		DestroyD3DWindow();
		m_bStartInD3DFullscreen = true;
	}

	// remember OpenMediaData for later use
	m_lastOMD.Free();
	m_lastOMD.Attach(pOMD.Detach());

	if (m_bIsBDPlay == FALSE) {
		m_BDPlaylists.clear();
		m_LastOpenBDPath.Empty();
	}

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	BOOL bMvcActive = FALSE;
	if (CComQIPtr<IMPCVideoDecFilter> pVDF = FindFilter(__uuidof(CMPCVideoDecFilter), m_pGB)) {
		bMvcActive = pVDF->GetMvcActive();
	}

	if (s.iStereo3DMode == STEREO3D_ROWINTERLEAVED || s.iStereo3DMode == STEREO3D_ROWINTERLEAVED_2X
			|| (s.iStereo3DMode == STEREO3D_AUTO && bMvcActive && !m_pBFmadVR)) {
		rs.iStereo3DTransform = STEREO3D_HalfOverUnder_to_Interlace;
	} else {
		rs.iStereo3DTransform = STEREO3D_AsIs;
	}

	rs.bStereo3DSwapLR = s.bStereo3DSwapLR;

	if (OpenDeviceData *pDeviceData = dynamic_cast<OpenDeviceData*>(m_lastOMD.m_p)) {
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
			if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
				CComBSTR bstr;
				if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
					m_youtubeFields.title = bstr;

					CString ext = L".mp4";
					BeginEnumPins(pBF, pEP, pPin) {
						PIN_DIRECTION dir;
						if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT) {
							AM_MEDIA_TYPE mt;
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

	if (GetPlaybackMode() == PM_CAPTURE) {
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE);
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

	if (s.fLaunchfullscreen && !s.IsD3DFullscreen() && !m_bFullScreen && !m_bAudioOnly) {
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
		if (IsWindowVisible() && s.bRememberZoomLevel
				&& !(m_bFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)
				&& GetPlaybackMode() == PM_CAPTURE && rs.iVideoRenderer == VIDRNDT_MADVR) {
			ShowWindow(SW_MAXIMIZE);
			wp.showCmd = SW_SHOWMAXIMIZED;
		}

		// restore magnification
		if (IsWindowVisible() && s.bRememberZoomLevel
				&& !(m_bFullScreen || IsD3DFullScreenMode()
					|| wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)) {
			ZoomVideoWindow(false);
		}
	}

	m_bfirstPlay   = true;
	m_LastOpenFile = m_lastOMD->title;

	if (!(s.nCLSwitches & CLSW_OPEN) && (s.nLoops > 0)) {
		if (s.AutoChangeFullscrRes.bEnabled == 1
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

	if (m_bFullScreen && s.bRememberZoomLevel) {
		m_bFirstFSAfterLaunchOnFullScreen = true;
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

	// correct window size if "Limit window proportions on resize" enable.
	if (!s.bRememberZoomLevel && s.bLimitWindowProportions) {
		const bool bCtrl = !!(GetAsyncKeyState(VK_CONTROL) & 0x80000000);
		if (bCtrl) {
			keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		}

		CRect r;
		GetWindowRect(&r);
		OnSizing(WMSZ_LEFT, r);
		MoveWindow(r);
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
	if (CComQIPtr<ILAVVideoStatus> pLAVVideoStatus = FindFilter(GUID_LAVVideoDecoder, m_pGB)) {
		LPCWSTR decoderName = pLAVVideoStatus->GetActiveDecoderName();

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
		};

		for (const auto &item : LAVDecoderNames) {
			if (wcscmp(item.name, decoderName) == 0) {
				UnHookDirectXVideoDecoderService();
				DXVAState::SetActiveState(GUID_NULL, item.friendlyname);
				break;
			}
		}
	}

	if (bMvcActive == 2) {
		UnHookDirectXVideoDecoderService();
		DXVAState::SetActiveState(GUID_NULL, L"Intel H.264(MVC 3D)");
	}
}

void CMainFrame::OnFilePostCloseMedia()
{
	DLog(L"CMainFrame::OnFilePostCloseMedia() : start");

	SetPlaybackMode(PM_NONE);
	SetLoadState(MLS_CLOSED);

	KillTimer(TIMER_STATUSERASER);
	m_playingmsg.Empty();

	if (m_closingmsg.IsEmpty()) {
		m_closingmsg = ResStr(IDS_CONTROLS_CLOSED);
	}

	m_wndView.SetVideoRect();

	CAppSettings& s = AfxGetAppSettings();

	s.nCLSwitches &= CLSW_OPEN | CLSW_PLAY | CLSW_AFTERPLAYBACK_MASK | CLSW_NOFOCUS;
	s.ResetPositions();

	if (s.iShowOSD & OSD_ENABLE) {
		m_OSD.Start(m_pOSDWnd);
	}

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();
	m_ExtSubPathsHandles.RemoveAll();
	m_EventSubChangeRefreshNotify.Set();

	m_wndSeekBar.Enable(false);
	m_wndSeekBar.SetRange(0);
	m_wndSeekBar.SetPos(0);
	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();
	m_wndStatusBar.Clear();
	m_wndStatusBar.ShowTimer(false);
	m_wndStatusBar.Relayout();

	if (IsWindow(m_wndSubresyncBar.m_hWnd)) {
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE);
	}
	SetSubtitle(nullptr);

	if (IsWindow(m_wndCaptureBar.m_hWnd)) {
		ShowControlBar(&m_wndCaptureBar, FALSE, TRUE);
		m_wndCaptureBar.m_capdlg.SetupVideoControls(L"", nullptr, nullptr, nullptr);
		m_wndCaptureBar.m_capdlg.SetupAudioControls(L"", nullptr, CInterfaceArray<IAMAudioInputMixer>());
	}

	RecalcLayout();
	UpdateWindow();

	SetWindowTextW(m_strTitle);
	m_Lcd.SetMediaTitle(m_strTitle);

	SetAlwaysOnTop(s.iOnTop);

	m_bIsBDPlay = FALSE;

	// Ensure the dynamically added menu items are cleared
	MakeEmptySubMenu(m_filtersMenu);
	MakeEmptySubMenu(m_SubtitlesMenu);
	MakeEmptySubMenu(m_AudioMenu);
	MakeEmptySubMenu(m_SubtitlesMenu);
	MakeEmptySubMenu(m_VideoStreamsMenu);
	MakeEmptySubMenu(m_chaptersMenu);

	UnloadExternalObjects();

	SetAudioPicture(FALSE);
	m_wndToolBar.SwitchTheme();

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

	DLog(L"CMainFrame::OnFilePostCloseMedia() : end");
}

void CMainFrame::OnBossKey()
{
	if (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) {
		m_wndFlyBar.ShowWindow(SW_HIDE);
	}

	// Disable animation
	ANIMATIONINFO AnimationInfo = { sizeof(AnimationInfo) };
	::SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
	const int WindowAnimationType = AnimationInfo.iMinAnimate;
	AnimationInfo.iMinAnimate = 0;
	::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

	if (State_Running == GetMediaState()) {
		SendMessageW(WM_COMMAND, ID_PLAY_PAUSE);
	}
	if (m_bFullScreen || IsD3DFullScreenMode()) {
		SendMessageW(WM_COMMAND, ID_VIEW_FULLSCREEN);
	}
	SendMessageW(WM_SYSCOMMAND, SC_MINIMIZE, -1);

	// Enable animation
	AnimationInfo.iMinAnimate = WindowAnimationType;
	::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
}

void CMainFrame::OnStreamAudio(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	nID -= ID_STREAM_AUDIO_NEXT;

	if (GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1) {
			for (DWORD i = 0; i < cStreams; i++) {
				DWORD dwFlags = 0;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, nullptr, nullptr, nullptr))) {
					return;
				}
				if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
					long lNextStream = (i + (nID == 0 ? 1 : cStreams - 1)) % cStreams;
					pSS->Enable(lNextStream, AMSTREAMSELECTENABLE_ENABLE);
					CComHeapPtr<WCHAR> pszName;
					if (SUCCEEDED(pSS->Info(lNextStream, nullptr, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr))) {
						CString	strMessage;
						strMessage.Format(ResStr(IDS_AUDIO_STREAM), pszName);
						m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
					}

					return;
				}
			}
		}

		return;
	}

	if (GetPlaybackMode() == PM_DVD && m_pDVDI && m_pDVDC) {
		ULONG nStreamsAvailable, nCurrentStream;
		if (SUCCEEDED(m_pDVDI->GetCurrentAudio(&nStreamsAvailable, &nCurrentStream)) && nStreamsAvailable > 1) {
			DVD_AudioAttributes AATR;
			UINT nNextStream = (nCurrentStream + (nID == 0 ? 1 : nStreamsAvailable - 1)) % nStreamsAvailable;

			HRESULT hr = m_pDVDC->SelectAudioStream(nNextStream, DVD_CMD_FLAG_Block, nullptr);
			if (SUCCEEDED(m_pDVDI->GetAudioAttributes(nNextStream, &AATR))) {
				CString lang;
				CString	strMessage;
				if (AATR.Language) {
					int len = GetLocaleInfoW(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(max(len - 1, 0));
				}
				else {
					lang.Format(ResStr(IDS_AG_UNKNOWN), nNextStream + 1);
				}

				CString format = GetDVDAudioFormatName(AATR);
				CString str;

				if (!format.IsEmpty()) {
					str.Format(ResStr(IDS_MAINFRM_11),
						lang,
						format,
						AATR.dwFrequency,
						AATR.bQuantization,
						AATR.bNumberOfChannels,
						(AATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
					str += FAILED(hr) ? L" [" + ResStr(IDS_AG_ERROR) + L"] " : L"";
					strMessage.Format(ResStr(IDS_AUDIO_STREAM), str);
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
					CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
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
							if (pName) {
								CoTaskMemFree(pName);
							}

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

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS && !AfxGetAppSettings().fDisableInternalSubtitles) {
			DWORD cStreamsS = 0;

			if (SUCCEEDED(pSS->Count(&cStreamsS)) && cStreamsS > 0) {
				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS;
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
						MixSS.push_back(ss);
					}
				}
			}
		}

		splcnt = MixSS.size();

		int subcnt = -1;
		POSITION pos = m_pSubStreams.GetHeadPosition();
		if (splcnt > 0) {
			m_pSubStreams.GetNext(pos);
			subcnt++;
		}
		while (pos) {
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);
			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				subcnt++;
				ss.Filter	= 2;
				ss.Index	= subcnt;
				ss.Num++;
				if (m_iSubtitleSel == subcnt) iSel = MixSS.size();
				ss.Sel		= iSel;
				MixSS.push_back(ss);
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
					if (stream_name.Left(11) == L"Subtitle - ") {
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
					lang.ReleaseBufferSetLength(max(len - 1, 0));
					lang += FAILED(hr) ? L" [" + ResStr(IDS_AG_ERROR) + L"] " : L"";
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

	if (GetPlaybackMode() == PM_FILE && m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
			bool fHideSubtitles = false;
			m_pDVS->get_HideSubtitles(&fHideSubtitles);
			fHideSubtitles = !fHideSubtitles;
			m_pDVS->put_HideSubtitles(fHideSubtitles);

			if (fHideSubtitles) {
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

		return;
	}

	int cnt = 0;
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while (pos) {
		cnt += m_pSubStreams.GetNext(pos)->GetStreamCount();
	}

	if (cnt > 0) {
		if (m_iSubtitleSel == -1) {
			m_iSubtitleSel = 0;
		} else {
			m_iSubtitleSel ^= 0x80000000;
		}
		UpdateSubtitle(true);
		SetFocus();
		AfxGetAppSettings().fEnableSubtitles = !(m_iSubtitleSel & 0x80000000);

		return;
	}

	if (GetPlaybackMode() == PM_DVD && m_pDVDI && m_pDVDC) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			m_pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, nullptr);
		}
	}
}

void CMainFrame::OnStreamVideo(UINT nID)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
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
						stms.push_back(i);
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
							if (stream_name.Left(8) == L"Video - ") {
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
	if (m_eMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) {
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

	COpenFileDlg fd(mask, true, nullptr, nullptr, dwFlags, filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	std::list<CString> fns;

	POSITION pos = fd.GetStartPosition();
	while (pos) {
		fns.push_back(fd.GetNextPathName(pos));
	}

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
		//ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem();
}

void CMainFrame::OnFileOpenMedia()
{
	if (m_eMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar) || IsD3DFullScreenMode()) {
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

	CString fn = dlg.m_fns.front();
	if (OpenYoutubePlaylist(fn)) {
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

		CString path = AddSlash(GetFolderOnly(fullpath));

		WIN32_FIND_DATAW fd = {0};
		HANDLE hFind = FindFirstFileW(fullpath, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				CFilterMapper2 fm2(false);
				fm2.Register(path + fd.cFileName);

				while (!fm2.m_filters.empty()) {
					FilterOverride* f = fm2.m_filters.back();
					fm2.m_filters.pop_back();

					if (f) {
						f->fTemporary = true;
						bool bFound = false;

						POSITION pos2 = s.m_filters.GetHeadPosition();
						while (pos2) {
							FilterOverride* f2 = s.m_filters.GetNext(pos2);
							if (f2->type == FilterOverride::EXTERNAL && !f2->path.CompareNoCase(f->path)) {
								bFound = true;
								delete f;
								break;
							}
						}

						if (!bFound) {
							CAutoPtr<FilterOverride> p(f);
							s.m_filters.AddHead(p);
						}
					}
				}
			} while (FindNextFileW(hFind, &fd));

			FindClose(hFind);
		}
	}

	bool fSetForegroundWindow = false;

	if ((s.nCLSwitches & CLSW_DVD) && !s.slFiles.empty()) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
		if (p) {
			p->path = s.slFiles.front();
			p->subs = s.slSubs;
		}
		OpenMedia(p);
	} else if (s.nCLSwitches & CLSW_CD) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		std::list<CString> sl;

		if (!s.slFiles.empty()) {
			GetCDROMType(s.slFiles.front()[0], sl);
		} else {
			CString dir;
			dir.ReleaseBufferSetLength(GetCurrentDirectoryW(MAX_PATH, dir.GetBuffer(MAX_PATH)));

			GetCDROMType(dir[0], sl);
			for (WCHAR drive = 'C'; sl.empty() && drive <= 'Z'; drive++) {
				GetCDROMType(drive, sl);
			}
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	} else if (!s.slFiles.empty()) {
		if (s.slFiles.size() == 1 && OpenYoutubePlaylist(s.slFiles.front())) {
			;
		} else if (s.slFiles.size() == 1 && ::PathIsDirectoryW(s.slFiles.front() + L"\\VIDEO_TS")) {
			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			fSetForegroundWindow = true;

			CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
			if (p) {
				p->path = s.slFiles.front();
				p->subs = s.slSubs;
			}
			OpenMedia(p);
		} else {
			std::list<CString> sl;
			sl = s.slFiles;

			ParseDirs(sl);
			bool fMulti = sl.size() > 1;

			if (!fMulti) {
				for (const auto& dub : s.slDubs) {
					sl.push_back(dub);
				}
			}

			if ((s.nCLSwitches & CLSW_ADD) && m_wndPlaylistBar.GetCount() > 0) {
				m_wndPlaylistBar.Append(sl, fMulti, &s.slSubs);

				if (s.nCLSwitches & (CLSW_OPEN | CLSW_PLAY)) {
					m_wndPlaylistBar.SetLast();
					OpenCurPlaylistItem();
				}
			} else {
				//UINT nCLSwitches = s.nCLSwitches;	// backup cmdline params

				//SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
				fSetForegroundWindow = true;

				//s.nCLSwitches = nCLSwitches;		// restore cmdline params
				if (AddSimilarFiles(sl)) {
					fMulti = true;
				}

				m_wndPlaylistBar.Open(sl, fMulti, &s.slSubs);
				OpenCurPlaylistItem((s.nCLSwitches & CLSW_STARTVALID) ? s.rtStart : INVALID_TIME);

				s.nCLSwitches &= ~CLSW_STARTVALID;
				s.rtStart = 0;
			}
		}
	} else {
		if ((s.nCLSwitches & (CLSW_OPEN | CLSW_PLAY)) && m_wndPlaylistBar.GetCount() > 0) {
			OpenCurPlaylistItem();
		}

		// leave only CLSW_OPEN, if present
		s.nCLSwitches = (s.nCLSwitches & CLSW_OPEN) ? CLSW_OPEN : CLSW_NONE;
	}

	if (fSetForegroundWindow && !(s.nCLSwitches & CLSW_NOFOCUS)) {
		SetForegroundWindow();
	}
	s.nCLSwitches &= ~CLSW_NOFOCUS;

	UpdateThumbarButton();

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
						DLog(L"CMainFrame::cmdLineThreadFunction() : command line queue size - %u", m_cmdLineQueue.size());
						const std::vector<BYTE>& pData = m_cmdLineQueue.front();
						const BYTE* p = pData.data();
						DWORD cnt = GETDWORD(p);
						p += sizeof(DWORD);
						const WCHAR* pBuff    = (WCHAR*)(p);
						const WCHAR* pBuffEnd = (WCHAR*)(p + pData.size() - sizeof(DWORD));

						while (cnt-- > 0) {
							CString str;
							while (pBuff < pBuffEnd && *pBuff) {
								str += *pBuff++;
							}
							pBuff++;
							cmdLine.push_back(str);
						}

						const ULONGLONG now = GetTickCount64();
						if ((now - lastReceiveCmdLineTime) <= 500ULL) {
							cmdLine.push_back(L"/add");
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
		m_cmdLineQueue.push_back(pData);
		m_EventCmdLineQueue.Set();
	}

	return TRUE;
}

void CMainFrame::OnFileOpenDVD()
{
	if (m_eMediaLoadState == MLS_LOADING) {
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
		if (CheckBD(path) || CheckDVD(path)) {
			s.strDVDPath = AddSlash(path);
			m_wndPlaylistBar.Open(path);
			OpenCurPlaylistItem();
		} else {
			m_closingmsg = ResStr(IDS_MAINFRM_93);
		}
	}
}

void CMainFrame::OnFileOpenIso()
{
	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	if (m_DiskImage.DriveAvailable()) {
		DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;

		CString szFilter;
		szFilter.Format(L"Image Files (%s)|%s||", m_DiskImage.GetExts(), m_DiskImage.GetExts());
		CFileDialog fd(TRUE, nullptr, nullptr, dwFlags, szFilter);
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

	ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	m_wndPlaylistBar.Empty();

	CAutoPtr<OpenDeviceData> p(DNew OpenDeviceData());
	if (p) {
		p->DisplayName[0] = s.strAnalogVideo;
		p->DisplayName[1] = s.strAnalogAudio;
	}
	OpenMedia(p);
	if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && m_eMediaLoadState == MLS_LOADED && s.iDefaultCaptureDevice == 1) {
		m_wndNavigationBar.m_navdlg.UpdateElementList();
		ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
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

	if (m_wndPlaylistBar.IsWindowVisible()) {
		if (slFiles.size() == 1) {
			const CString& path = slFiles.front();
			if (OpenYoutubePlaylist(path, TRUE)) {
				return;
			}
		}
		AddSimilarFiles(slFiles);

		m_wndPlaylistBar.DropFiles(slFiles);
		return;
	}

	BOOL bIsValidSubExtAll = TRUE;

	for (const auto& fname : slFiles) {
		const CString ext = GetFileExt(fname).Mid(1).MakeLower(); // extension without a dot

		const bool validate_ext = std::any_of(std::cbegin(Subtitle::s_SubFileExts), std::cend(Subtitle::s_SubFileExts), [&](LPCWSTR subExt) {
			return ext == subExt;
		});

		if (!validate_ext
				|| (m_pDVS && ext == "mks")) {
			bIsValidSubExtAll = FALSE;
			break;
		}
	}

	if (bIsValidSubExtAll && m_eMediaLoadState == MLS_LOADED && (m_pCAP || m_pDVS)) {

		for (const auto& fname : slFiles) {
			BOOL b_SubLoaded = FALSE;

			if (m_pDVS) {
				if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)fname))) {
					m_pDVS->put_SelectedLanguage(0);
					m_pDVS->put_HideSubtitles(true);
					m_pDVS->put_HideSubtitles(false);

					b_SubLoaded = TRUE;
				}
			} else {
				ISubStream *pSubStream = nullptr;
				if (LoadSubtitle(fname, &pSubStream)) {
					SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
					b_SubLoaded = TRUE;

					AddSubtitlePathsAddons(fname);
				}
			}

			if (b_SubLoaded) {
				SetToolBarSubtitleButton();

				SendStatusMessage(GetFileOnly(fname) + ResStr(IDS_MAINFRM_47), 3000);
				if (m_pDVS) {
					return;
				}
			}
		}

		return;
	}

	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	if (slFiles.size() == 1
			&& OpenYoutubePlaylist(slFiles.front())) {
		return;
	}

	ParseDirs(slFiles);
	AddSimilarFiles(slFiles);

	m_wndPlaylistBar.Open(slFiles, true);
	OpenCurPlaylistItem();
}

void CMainFrame::OnFileSaveAs()
{
	CString ext, ext_list, in = m_strPlaybackRenderedPath, out = in;

	if (!m_youtubeFields.fname.IsEmpty()) {
		out = GetAltFileName();
		ext = GetFileExt(out).MakeLower();
	} else {
		if (!::PathIsURLW(out)) {
			ext = GetFileExt(out).MakeLower();
			out = GetFileOnly(out);
			if (ext == L".cda") {
				out = RenameFileExt(out, L".wav");
			} else if (ext == L".ifo") {
				out = RenameFileExt(out, L".vob");
			}
		} else {
			CString fname = L"streaming_saved";

			CUrl url;
			url.CrackUrl(out);
			if (url.GetUrlPathLength() > 1) {
				fname = url.GetUrlPath();
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
	ext_list.Append(ResStr(IDS_MAINFRM_48));

	CFileDialog fd(FALSE, 0, out,
				   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
				   ext_list, GetModalParent(), 0);

	if (fd.DoModal() != IDOK || !in.CompareNoCase(fd.GetPathName())) {
		return;
	}

	CPath p(fd.GetPathName());

	if (!ext.IsEmpty()) {
		p.AddExtension(ext);
	}

	OAFilterState fs = State_Stopped;
	m_pMC->GetState(0, &fs);

	if (fs == State_Running) {
		m_pMC->Pause();
	}

	CString name = in;
	if (!m_youtubeFields.fname.IsEmpty()) {
		name = GetAltFileName();
	}

	HRESULT hr;
	CSaveDlg dlg(in, name, p, hr);
	if (SUCCEEDED(hr)) {
		dlg.DoModal();
		if (!m_youtubeFields.fname.IsEmpty()) {
			const auto pFileData = dynamic_cast<OpenFileData*>(m_lastOMD.m_p);
			if (pFileData && pFileData->fns.size() == 2) {
				ext = p.GetExtension().MakeLower();
				if (ext == L".mp4") {
					p.RenameExtension(L".audio.m4a");
				} else {
					p.RenameExtension(L".audio.mka");
				}

				if (pFileData->fns.size() >= 2) {
					auto it = pFileData->fns.begin();
					++it;
					in = (*it).GetName();

					CSaveDlg dlg_second(in, name, p, hr);
					if (SUCCEEDED(hr)) {
						dlg_second.DoModal();
					}
				}
			}
		}
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

HRESULT GetBasicVideoFrame(IBasicVideo* pBasicVideo, std::vector<BYTE>& dib)
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

	dib.resize(size);

	hr = pBasicVideo->GetCurrentImage(&size, (long*)dib.data());
	if (FAILED(hr)) {
		dib.clear();
	}

	return hr;
}

HRESULT GetVideoDisplayControlFrame(IMFVideoDisplayControl* pVideoDisplayControl, std::vector<BYTE>& dib)
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

	dib.resize(sizeof(BITMAPINFOHEADER) + size);

	memcpy(dib.data(), &bih, sizeof(BITMAPINFOHEADER));
	memcpy(dib.data() + sizeof(BITMAPINFOHEADER), pDib, size);
	CoTaskMemFree(pDib);

	return hr;
}

HRESULT GetMadVRFrameGrabberFrame(IMadVRFrameGrabber* pMadVRFrameGrabber, std::vector<BYTE>& dib, bool displayed)
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

	dib.resize(sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
	memcpy(dib.data(), dibImage, sizeof(BITMAPINFOHEADER) + bih->biSizeImage);
	LocalFree(dibImage);

	return hr;
}

HRESULT CMainFrame::GetDisplayedImage(std::vector<BYTE>& dib, CString& errmsg)
{
	errmsg.Empty();
	HRESULT hr;

	if (m_pMFVDC) {
		hr = GetVideoDisplayControlFrame(m_pMFVDC, dib);
	} else if (m_pMVRFG) {
		hr = GetMadVRFrameGrabberFrame(m_pMVRFG, dib, true);
	} else {
		hr = E_NOINTERFACE;
	}

	if (FAILED(hr)) {
		errmsg.Format(L"IMFVideoDisplayControl::GetCurrentImage() failed, 0x%08x", hr);
	}

	return hr;
}

HRESULT CMainFrame::GetCurrentFrame(std::vector<BYTE>& dib, CString& errmsg)
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
			dib.resize(size);
			hr = m_pCAP->GetDIB(dib.data(), &size);
		}

		if (FAILED(hr)) {
			errmsg.Format(L"ISubPicAllocatorPresenter3::GetDIB() failed, 0x%08x", hr);
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

HRESULT CMainFrame::GetOriginalFrame(std::vector<BYTE>& dib, CString& errmsg)
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
		if (CComQIPtr<ISubPicProvider> pSubPicProvider = m_pCurrentSubStream) {
			const PBITMAPINFOHEADER bih = (PBITMAPINFOHEADER)pData;
			const int width  = bih->biWidth;
			const int height = bih->biHeight;

			SubPicDesc spdRender;
			spdRender.type    = MSP_RGBA;
			spdRender.w       = width;
			spdRender.h       = abs(height);
			spdRender.bpp     = 32;
			spdRender.pitch   = width * 4;
			spdRender.vidrect = {0, 0, width, height};
			spdRender.bits    = DNew BYTE[spdRender.pitch * spdRender.h];

			REFERENCE_TIME rtNow = 0;
			m_pMS->GetCurrentPosition(&rtNow);

			CMemSubPic memSubPic(spdRender);
			memSubPic.ClearDirtyRect(0xFF000000);

			RECT bbox = {};
			hr = pSubPicProvider->Render(spdRender, rtNow, m_pCAP->GetFPS(), bbox);
			if (S_OK == hr) {
				SubPicDesc spdTarget;
				spdTarget.type    = MSP_RGBA;
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

void CMainFrame::SaveDIB(LPCWSTR fn, BYTE* pData, long size)
{
	const CAppSettings& s = AfxGetAppSettings();

	const CString ext = GetFileExt(fn).MakeLower();
	if (ext == L".bmp") {
		BMPDIB(fn, pData, L"", 0, 0, 0, 0);
	} else if (ext == L".png") {
		PNGDIB(fn, pData, std::clamp(s.iThumbLevelPNG, 1, 9));
	} else if (ext == L".jpg") {
		BMPDIB(fn, pData, L"image/jpeg", s.iThumbQuality, 0, 0, 0);
	}

	CString fName(fn);
	fName.Replace(L"\\\\", L"\\");

	CPath p(fName);
	if (CDC* pDC = m_wndStatusBar.GetDC()) {
		CRect r;
		m_wndStatusBar.GetClientRect(&r);
		p.CompactPath(pDC->m_hDC, r.Width());
		m_wndStatusBar.ReleaseDC(pDC);
	}

	SendStatusMessage(p, 3000);
}

void CMainFrame::SaveImage(LPCWSTR fn, bool displayed)
{
	std::vector<BYTE> dib;
	CString errmsg;
	HRESULT hr;
	if (displayed) {
		hr = GetDisplayedImage(dib, errmsg);
	} else {
		hr = GetCurrentFrame(dib, errmsg);
		if (hr == S_OK) {
			RenderCurrentSubtitles(dib.data());
		}
	}

	if (hr == S_OK) {
		SaveDIB(fn, dib.data(), dib.size());
		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_IMAGE_SAVED), 3000);
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

	if (dlg.m_bSuccessfully) {
		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_THUMBS_SAVED), 3000);
	}
}

static CString MakeSnapshotFileName(LPCTSTR prefix)
{
	CTime t = CTime::GetCurrentTime();
	CString fn;
	fn.Format(L"%s_[%s]%s", prefix, t.Format(L"%Y.%m.%d_%H.%M.%S"), AfxGetAppSettings().strSnapShotExt);
	return fn;
}

BOOL CMainFrame::IsRendererCompatibleWithSaveImage()
{
	BOOL result = TRUE;
	const CRenderersSettings& rs = GetRenderersSettings();

	if (m_fShockwaveGraph) {
		AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_SHOCKWAVE), MB_ICONEXCLAMATION | MB_OK);
		result = FALSE;
	}
	// the latest madVR build v0.84.0 now supports screenshots.
	else if (rs.iVideoRenderer == VIDRNDT_MADVR) {
		CRegKey key;
		CString clsid = L"{E1A8B82A-32CE-4B0D-BE0D-AA68C772E423}";

		WCHAR buff[256];
		ULONG len = _countof(buff);
		memset(buff, 0, len);

		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + clsid + L"\\InprocServer32", KEY_READ)
				&& ERROR_SUCCESS == key.QueryStringValue(nullptr, buff, &len)) {

			const FileVersion::Ver madVR_ver = FileVersion::GetVer(buff);
			if (madVR_ver.value < FileVersion::Ver(0, 84, 0, 0).value) {
				result = FALSE;
			}

			key.Close();
		}

		if (!result) {
			AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_MADVR), MB_ICONEXCLAMATION | MB_OK);
		}
	}

	return result;
}

CString CMainFrame::GetVidPos()
{
	CString posstr = L"";
	if ((GetPlaybackMode() == PM_FILE) || (GetPlaybackMode() == PM_DVD)) {
		__int64 stop, pos;
		m_wndSeekBar.GetRange(stop);
		pos = m_wndSeekBar.GetPosReal();

		DVD_HMSF_TIMECODE tcNow = RT2HMSF(pos);
		DVD_HMSF_TIMECODE tcDur = RT2HMSF(stop);

		if (tcDur.bHours > 0 || (pos >= stop && tcNow.bHours > 0)) {
			posstr.Format(L"%02d.%02d.%02d", tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
		} else {
			posstr.Format(L"%02d.%02d", tcNow.bMinutes, tcNow.bSeconds);
		}
	}

	return posstr;
}

CString CMainFrame::CreateSnapShotFileName()
{
	CString path(AfxGetAppSettings().strSnapShotPath);
	if (!::PathFileExistsW(path)) {
		WCHAR szPath[MAX_PATH] = { 0 };
		if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0, szPath))) {
			path = CString(szPath) + L"\\";
		}
	}

	CString prefix = L"snapshot";
	if (GetPlaybackMode() == PM_FILE) {
		CString filename = GetFileOnly(GetCurFileName());
		FixFilename(filename); // need for URLs
		if (!m_youtubeFields.fname.IsEmpty()) {
			filename = GetAltFileName();
		}

		prefix.Format(L"%s_snapshot_%s", filename, GetVidPos());
	} else if (GetPlaybackMode() == PM_DVD) {
		prefix.Format(L"snapshot_dvd_%s", GetVidPos());
	}

	CPath psrc;
	psrc.Combine(path, MakeSnapshotFileName(prefix));

	return CString(psrc);
}

void CMainFrame::OnFileSaveImage()
{
	CAppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CSaveImageDialog fd(
		s.iThumbQuality, s.iThumbLevelPNG, s.bSnapShotSubtitles,
		0, CreateSnapShotFileName(),
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

	s.iThumbQuality      = fd.m_quality;
	s.iThumbLevelPNG     = fd.m_levelPNG;
	s.bSnapShotSubtitles = fd.m_bSnapShotSubtitles;

	CString pdst = fd.GetPathName();
	if (GetFileExt(pdst).MakeLower() != s.strSnapShotExt) {
		pdst = RenameFileExt(pdst, s.strSnapShotExt);
	}

	s.strSnapShotPath = GetFolderOnly(pdst);
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
	if (!m_pMFVDC && !m_pMVRFG) {
		return;
	}

	SaveImage(CreateSnapShotFileName(), true);
}

void CMainFrame::OnFileSaveThumbnails()
{
	CAppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CPath psrc(s.strSnapShotPath);
	CStringW prefix = L"thumbs";
	if (GetPlaybackMode() == PM_FILE) {
		CString path = GetFileOnly(GetCurFileName());
		if (!m_youtubeFields.fname.IsEmpty()) {
			path = GetAltFileName();
		}

		prefix.Format(L"%s_thumbs", path);
	}
	psrc.Combine(s.strSnapShotPath, MakeSnapshotFileName(prefix));

	CSaveThumbnailsDialog fd(
		s.iThumbRows, s.iThumbCols, s.iThumbWidth, s.iThumbQuality, s.iThumbLevelPNG, s.bSnapShotSubtitles,
		0, (LPCTSTR)psrc,
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

	s.iThumbRows         = fd.m_rows;
	s.iThumbCols         = fd.m_cols;
	s.iThumbWidth        = fd.m_width;
	s.iThumbQuality      = fd.m_quality;
	s.iThumbLevelPNG     = fd.m_levelPNG;
	s.bSnapShotSubtitles = fd.m_bSnapShotSubtitles;

	CString pdst = fd.GetPathName();
	if (GetFileExt(pdst).MakeLower() != s.strSnapShotExt) {
		pdst = RenameFileExt(pdst, s.strSnapShotExt);
	}

	s.strSnapShotPath = GetFolderOnly(pdst);
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
	if (!m_pCAP && !m_pDVS) {
		AfxMessageBox(ResStr(IDS_MAINFRM_60)+
					  ResStr(IDS_MAINFRM_61)+
					  ResStr(IDS_MAINFRM_64)
					  , MB_OK);
		return;
	}

	CString filter;
	for (size_t idx = 0; idx < std::size(Subtitle::s_SubFileExts); idx++) {
		filter += (idx == 0 ? L"." : L" .") + CString(Subtitle::s_SubFileExts[idx]);
	}
	filter += L"|";

	for (size_t idx = 0; idx < std::size(Subtitle::s_SubFileExts); idx++) {
		filter += (idx == 0 ? L"*." : L";*.") + CString(Subtitle::s_SubFileExts[idx]);
	}
	filter += L"||";

	std::vector<CString> mask;
	for (const auto& subExt : Subtitle::s_SubFileExts) {
		mask.emplace_back(L"*." + CString(subExt));
	}

	COpenFileDlg fd(mask, false, nullptr, GetCurFileName(),
					OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT,
					filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	std::list<CString> fns;
	POSITION pos = fd.GetStartPosition();
	while (pos) {
		fns.emplace_back(fd.GetNextPathName(pos));
	}

	if (m_pDVS) {
		if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)(*fns.cbegin())))) {
			m_pDVS->put_SelectedLanguage(0);
			m_pDVS->put_HideSubtitles(true);
			m_pDVS->put_HideSubtitles(false);
		}
	} else {
		for (const auto& fn : fns) {
			ISubStream *pSubStream = nullptr;
			if (LoadSubtitle(fn, &pSubStream)) {
				SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
				AddSubtitlePathsAddons(fn);
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
	if (m_eMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar) || IsD3DFullScreenMode()) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	CString filter;
	std::vector<CString> mask;
	s.m_Formats.GetAudioFilter(filter, mask);

	COpenFileDlg fd(mask, false, nullptr, GetCurFileName(),
					OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT,
					filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	CPlaylistItem* pli = m_wndPlaylistBar.GetCur();
	if (pli && pli->m_fns.size()) {
		POSITION pos = fd.GetStartPosition();
		while (pos) {
			CString fname = fd.GetNextPathName(pos);

			if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB) {
				HRESULT hr = pGBA->RenderAudioFile(fname);
				if (SUCCEEDED(hr)) {
					pli->m_fns.push_back(fname);
					AddAudioPathsAddons(fname);

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
	if (m_pCurrentSubStream) {
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

		if (auto pVSF = dynamic_cast<CVobSubFile*>((ISubStream*)m_pCurrentSubStream)) {
			// remember to set lpszDefExt to the first extension in the filter so that the save dialog autocompletes the extension
			// and tracks attempts to overwrite in a graceful manner
			CFileDialog fd(FALSE, L"idx", suggestedFileName,
						   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
						   L"VobSub (*.idx, *.sub)|*.idx;*.sub||", GetModalParent());

			if (fd.DoModal() == IDOK) {
				CAutoLock cAutoLock(&m_csSubLock);
				pVSF->Save(fd.GetPathName());
			}

			return;
		} else if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
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
				pRTS->SaveAs(fd.GetPathName(), types[fd.m_ofn.nFilterIndex - 1], m_pCAP->GetFPS(), 0, fd.GetEncoding(), s.bSubSaveExternalStyleFile);
			}

			return;
		}
	}
}

void CMainFrame::OnUpdateFileSaveSubtitle(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;

	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
		bEnable = !pRTS->IsEmpty();
	} else if (dynamic_cast<CVobSubFile*>((ISubStream*)m_pCurrentSubStream)) {
		bEnable = TRUE;
	}

	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnFileISDBSearch()
{
	CStringA url = "http://" + AfxGetAppSettings().strISDb + "/index.php?";
	CStringA args = makeargs(m_wndPlaylistBar.m_pl);
	ShellExecuteW(m_hWnd, L"open", CString(url+args), nullptr, nullptr, SW_SHOWDEFAULT);
}

void CMainFrame::OnUpdateFileISDBSearch(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnFileISDBDownload()
{
	CAppSettings& s = AfxGetAppSettings();
	filehash fh;
	if (!::mpc_filehash(GetCurFileName(), fh)) {
		MessageBeep(UINT_MAX);
		return;
	}

	try {
		CStringA url = "http://" + s.strISDb + "/index.php?";
		CStringA args;
		args.Format("player=mpc&name[0]=%s&size[0]=%016I64x&hash[0]=%016I64x",
					UrlEncode(CStringA(fh.name), true), fh.size, fh.mpc_filehash);
		url.Append(args);

		CSubtitleDlDlg dlg(GetModalParent(), url, fh.name);
		dlg.DoModal();

		SetToolBarSubtitleButton();
	} catch (CInternetException* ie) {
		ie->Delete();
		return;
	}
}

void CMainFrame::OnUpdateFileISDBDownload(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() != PM_CAPTURE && (m_pCAP || m_pDVS) && !m_bAudioOnly);
}

void CMainFrame::OnFileProperties()
{
	CPPageFileInfoSheet m_fileinfo(GetPlaybackMode() == PM_FILE ? GetCurFileName() : GetCurDVDPath(TRUE), this, GetModalParent());
	m_fileinfo.DoModal();
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
	const CRenderersSettings& rs = GetRenderersSettings();
	bool supported = (rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM || rs.iVideoRenderer == VIDRNDT_SYNC);

	pCmdUI->Enable(supported && m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(rs.bTearingTest);
}

void CMainFrame::OnViewTearingTest()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bTearingTest = !rs.bTearingTest;
}

void CMainFrame::OnUpdateViewDisplayStats(CCmdUI* pCmdUI)
{
	const CRenderersSettings& rs = GetRenderersSettings();
	bool supported = (rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM || rs.iVideoRenderer == VIDRNDT_SYNC);

	pCmdUI->Enable(supported && m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(supported && rs.iDisplayStats);
}

void CMainFrame::OnViewResetStats()
{
	GetRenderersSettings().bResetStats = true; // Reset by "consumer"
}

void CMainFrame::OnViewDisplayStatsSC()
{
	CRenderersSettings& rs = GetRenderersSettings();

	if (!rs.iDisplayStats) {
		rs.bResetStats = true; // to Reset statistics on first call ...
	}

	++rs.iDisplayStats;
	if (rs.iDisplayStats > 3) {
		rs.iDisplayStats = 0;
	}

	RepaintVideo();
}

void CMainFrame::OnUpdateViewD3DFullscreen(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();
	const CRenderersSettings& rs = s.m_VRSettings;
	bool supported = (rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM || rs.iVideoRenderer == VIDRNDT_SYNC);

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(s.fD3DFullscreen);
}

void CMainFrame::OnUpdateViewDisableDesktopComposition(CCmdUI* pCmdUI)
{
	const CRenderersSettings& rs = GetRenderersSettings();
	bool supported = ((rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM ||
					   rs.iVideoRenderer == VIDRNDT_SYNC) &&
					  (!SysVersion::IsWin8orLater()));

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(rs.bDisableDesktopComposition);
}

void CMainFrame::OnUpdateViewEnableFrameTimeCorrection(CCmdUI* pCmdUI)
{
	const CRenderersSettings& rs = GetRenderersSettings();
	bool supported = (rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM);

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(rs.bEVRFrameTimeCorrection);
}

void CMainFrame::OnViewVSync()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bVSync = !rs.bVSync;
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bVSync ? ResStr(IDS_OSD_RS_VSYNC_ON) : ResStr(IDS_OSD_RS_VSYNC_OFF));
}

void CMainFrame::OnViewVSyncAccurate()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bVSyncAccurate = !rs.bVSyncAccurate;
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bVSyncAccurate ? ResStr(IDS_OSD_RS_ACCURATE_VSYNC_ON) : ResStr(IDS_OSD_RS_ACCURATE_VSYNC_OFF));
}

void CMainFrame::OnViewD3DFullScreen()
{
	CAppSettings& s = AfxGetAppSettings();
	s.fD3DFullscreen = !s.fD3DFullscreen;
	s.SaveSettings();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.fD3DFullscreen ? ResStr(IDS_OSD_RS_EXCLUSIVE_FSCREEN_ON) : ResStr(IDS_OSD_RS_EXCLUSIVE_FSCREEN_OFF));
}

void CMainFrame::OnViewDisableDesktopComposition()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bDisableDesktopComposition = !rs.bDisableDesktopComposition;
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bDisableDesktopComposition ? ResStr(IDS_OSD_RS_NO_DESKTOP_COMP_ON) : ResStr(IDS_OSD_RS_NO_DESKTOP_COMP_OFF));
}

void CMainFrame::OnViewAlternativeVSync()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bAlterativeVSync = !rs.bAlterativeVSync;
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bAlterativeVSync ? ResStr(IDS_OSD_RS_ALT_VSYNC_ON) : ResStr(IDS_OSD_RS_ALT_VSYNC_OFF));
}

void CMainFrame::OnViewResetDefault()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.SetDefault();
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_RS_RESET_DEFAULT));
}

void CMainFrame::OnUpdateViewReset(CCmdUI* pCmdUI)
{
	const CRenderersSettings& rs = GetRenderersSettings();;
	bool supported = (rs.iVideoRenderer == VIDRNDT_EVR_CUSTOM || rs.iVideoRenderer == VIDRNDT_SYNC);
	pCmdUI->Enable(supported);
}

void CMainFrame::OnViewEnableFrameTimeCorrection()
{
	CRenderersSettings& rs = GetRenderersSettings();
	rs.bEVRFrameTimeCorrection = !rs.bEVRFrameTimeCorrection;
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 rs.bEVRFrameTimeCorrection ? ResStr(IDS_OSD_RS_FT_CORRECTION_ON) : ResStr(IDS_OSD_RS_FT_CORRECTION_OFF));
}

void CMainFrame::OnViewVSyncOffsetIncrease()
{
	CRenderersSettings& rs = GetRenderersSettings();
	CString strOSD;
	if (rs.iVideoRenderer == VIDRNDT_SYNC) {
		rs.dTargetSyncOffset = rs.dTargetSyncOffset - 0.5; // Yeah, it should be a "-"
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), rs.dTargetSyncOffset);
	} else {
		++rs.iVSyncOffset;
		strOSD.Format(ResStr(IDS_OSD_RS_VSYNC_OFFSET), rs.iVSyncOffset);
	}
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
}

void CMainFrame::OnViewVSyncOffsetDecrease()
{
	CRenderersSettings& rs = GetRenderersSettings();
	CString strOSD;
	if (rs.iVideoRenderer == VIDRNDT_SYNC) {
		rs.dTargetSyncOffset = rs.dTargetSyncOffset + 0.5;
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), rs.dTargetSyncOffset);
	} else {
		--rs.iVSyncOffset;
		strOSD.Format(ResStr(IDS_OSD_RS_VSYNC_OFFSET), rs.iVSyncOffset);
	}
	rs.Save();
	m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
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
	CAppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable((s.iShowOSD & OSD_ENABLE) && m_eMediaLoadState != MLS_CLOSED);
	pCmdUI->SetCheck (m_bOSDLocalTime);
}

void CMainFrame::OnViewOSDLocalTime()
{
	m_bOSDLocalTime = !m_bOSDLocalTime;

	if (!m_bOSDLocalTime) {
		m_OSD.ClearMessage();
	}

	OnTimer(TIMER_STREAMPOSPOLLER2);
}

void CMainFrame::OnUpdateViewOSDFileName(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable((s.iShowOSD & OSD_ENABLE) && m_eMediaLoadState != MLS_CLOSED);
	pCmdUI->SetCheck (m_bOSDFileName);
}

void CMainFrame::OnViewOSDFileName()
{
	m_bOSDFileName = !m_bOSDFileName;

	if (!m_bOSDFileName) {
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
	if (AfxGetAppSettings().ShaderListScreenSpace.empty()) {
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
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_65));
	} else {
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_FRAME);
		}
		RepaintVideo();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_66));
	}
}

void CMainFrame::OnShaderToggle2()
{
	m_bToggleShaderScreenSpace = !m_bToggleShaderScreenSpace;
	if (m_bToggleShaderScreenSpace) {
		SetShaders();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_PPONSCR));
	} else {
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_SCREEN);
		}
		RepaintVideo();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_PPOFFSCR));
	}
}

void CMainFrame::OnD3DFullscreenToggle()
{
	OnViewD3DFullScreen();
}

void CMainFrame::OnFileClosePlaylist()
{
	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	RestoreDefaultWindowRect();
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
	nID -= ID_VIEW_SEEKER;
	ShowControls(AfxGetAppSettings().nCS ^ (1<<nID));
}

void CMainFrame::OnUpdateViewControlBar(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_VIEW_SEEKER;
	pCmdUI->SetCheck(!!(AfxGetAppSettings().nCS & (1<<nID)));
}

void CMainFrame::OnViewSubresync()
{
	ShowControlBar(&m_wndSubresyncBar, !m_wndSubresyncBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewSubresync(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndSubresyncBar.IsWindowVisible());
	pCmdUI->Enable(m_pCAP && m_iSubtitleSel >= 0);
}

void CMainFrame::OnViewPlaylist()
{
	ShowControlBar(&m_wndPlaylistBar, !m_wndPlaylistBar.IsWindowVisible(), TRUE);
	m_wndPlaylistBar.SetFocus();
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
		ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
	}
}

void CMainFrame::OnUpdateViewNavigation(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!AfxGetAppSettings().fHideNavigation);
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1);
}

void CMainFrame::OnViewCapture()
{
	ShowControlBar(&m_wndCaptureBar, !m_wndCaptureBar.IsWindowVisible(), TRUE);
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
	ShowControlBar(&m_wndShaderEditorBar, !m_wndShaderEditorBar.IsWindowVisible(), TRUE);
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

void CMainFrame::OnUpdateViewMinimal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewCompact()
{
	while (AfxGetAppSettings().iCaptionMenuMode != MODE_FRAMEONLY) {
		SendMessageW(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_SEEKBAR | CS_TOOLBAR);
}

void CMainFrame::OnUpdateViewCompact(CCmdUI* pCmdUI)
{
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
	if ((s.fD3DFullscreen || (s.nCLSwitches & CLSW_D3DFULLSCREEN))) {
		if (m_eMediaLoadState == MLS_LOADED) {
			return m_pD3DFS && !m_bFullScreen;
		}

		return s.IsD3DFullscreen();
	}

	return false;
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

void CMainFrame::OnUpdateViewFullscreen(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly || m_bFullScreen);
	//pCmdUI->SetCheck(m_bFullScreen);
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
		monitor.CenterRectToMonitor(windowRect, TRUE);
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
	if (m_iVideoSize == (pCmdUI->m_nID - ID_VIEW_VF_HALF)) {
		CheckMenuRadioItem(ID_VIEW_VF_HALF, ID_VIEW_VF_ZOOM2, pCmdUI->m_nID);
	}
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

	if (x > 0 && m_ZoomX < 3) {
		m_ZoomX *= 1.02;
	} else if (x < 0 && m_ZoomX > 0.2) {
		m_ZoomX /= 1.02;
	}

	if (y > 0 && m_ZoomY < 3) {
		m_ZoomY *= 1.02;
	} else if (y < 0 && m_ZoomY > 0.2) {
		m_ZoomY /= 1.02;
	}

	if (dx < 0 && m_PosX > 0) {
		m_PosX = std::max(m_PosX - 0.005*m_ZoomX, 0.0);
	} else if (dx > 0 && m_PosX < 1) {
		m_PosX = std::min(m_PosX + 0.005*m_ZoomX, 1.0);
	}

	if (dy < 0 && m_PosY > 0) {
		m_PosY = std::max(m_PosY - 0.005*m_ZoomY, 0.0);
	} else if (dy > 0 && m_PosY < 1) {
		m_PosY = std::min(m_PosY + 0.005*m_ZoomY, 1.0);
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
	m_ZoomX = std::clamp(m_ZoomX, 0.2, 3.0);
	m_ZoomY = std::clamp(m_ZoomY, 0.2, 3.0);

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
		case ID_PANSCAN_ROTATEZP:
			rotation += 270;
			break;
		case ID_PANSCAN_ROTATEZM:
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
			info.Format(L"Rotation: %d°", rotation);
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
	pCmdUI->Enable(m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly);

	if (AfxGetAppSettings().sizeAspectRatio == s_ar[pCmdUI->m_nID - ID_ASPECTRATIO_START]) {
		CheckMenuRadioItem(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, pCmdUI->m_nID);
	}
}

void CMainFrame::OnViewAspectRatioNext()
{
	CSize& ar = AfxGetAppSettings().sizeAspectRatio;

	UINT nID = ID_ASPECTRATIO_START;

	for (int i = 0; i < _countof(s_ar); i++) {
		if (ar == s_ar[i]) {
			nID += (i + 1) % _countof(s_ar);
			break;
		}
	}

	OnViewAspectRatio(nID);
}

void CMainFrame::OnUpdateViewStereo3DMode(CCmdUI* pCmdUI)
{
	if (AfxGetAppSettings().iStereo3DMode == (pCmdUI->m_nID - ID_STEREO3D_AUTO)) {
		CheckMenuRadioItem(ID_STEREO3D_AUTO, ID_STEREO3D_OVERUNDER, pCmdUI->m_nID);
	}
}

void CMainFrame::OnViewStereo3DMode(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	s.iStereo3DMode = nID - ID_STEREO3D_AUTO;

	int iMvcOutputMode;
	switch (s.iStereo3DMode) {
	case STEREO3D_AUTO:              iMvcOutputMode = MVC_OUTPUT_Auto;          break;
	case STEREO3D_MONO:              iMvcOutputMode = MVC_OUTPUT_Mono;          break;
	case STEREO3D_ROWINTERLEAVED:    iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
	case STEREO3D_ROWINTERLEAVED_2X: iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
	case STEREO3D_HALFOVERUNDER:     iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
	case STEREO3D_OVERUNDER:         iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
	default:
		ASSERT(0);
		return;
	}

	BOOL bMvcActive = FALSE;
	if (CComQIPtr<IMPCVideoDecFilter> pVDF = FindFilter(__uuidof(CMPCVideoDecFilter), m_pGB)) {
		pVDF->SetMvcOutputMode(iMvcOutputMode, s.bStereo3DSwapLR);
		bMvcActive = pVDF->GetMvcActive();
	}

	if (s.iStereo3DMode == STEREO3D_ROWINTERLEAVED || s.iStereo3DMode == STEREO3D_ROWINTERLEAVED_2X
			|| (s.iStereo3DMode == STEREO3D_AUTO && bMvcActive && !m_pBFmadVR)) {
		rs.iStereo3DTransform = STEREO3D_HalfOverUnder_to_Interlace;
	} else {
		rs.iStereo3DTransform = STEREO3D_AsIs;
	}

	rs.bStereo3DSwapLR = s.bStereo3DSwapLR;

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
	GetRenderersSettings().bStereo3DSwapLR = s.bStereo3DSwapLR;

	IFilterGraph* pFG = m_pGB;
	if (pFG) {
		CComQIPtr<IMPCVideoDecFilter> pVDF = FindFilter(__uuidof(CMPCVideoDecFilter), pFG);
		if (pVDF) {
			pVDF->SetMvcOutputMode(s.iStereo3DMode == STEREO3D_HALFOVERUNDER ? MVC_OUTPUT_HalfTopBottom : s.iStereo3DMode, s.bStereo3DSwapLR);
		}
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
	if (AfxGetAppSettings().iOnTop == (pCmdUI->m_nID - ID_ONTOP_NEVER)) {
		CheckMenuRadioItem(ID_ONTOP_NEVER, ID_ONTOP_WHILEPLAYINGVIDEO, pCmdUI->m_nID);
	}
}

void CMainFrame::OnViewOptions()
{
	if (IsD3DFullScreenMode()) {
		const CMonitor monitorD3D  = CMonitors::GetNearestMonitor(m_pFullscreenWnd);
		const CMonitor monitorMain = CMonitors::GetNearestMonitor(this);

		if (monitorD3D == monitorMain) {
			return;
		}
	}

	ShowOptions();
}

// play

void CMainFrame::OnPlayPlay()
{
	if (m_eMediaLoadState == MLS_CLOSED) {
		m_bfirstPlay = false;
		OpenCurPlaylistItem();
		return;
	}

	CString strOSD;
	int OSD_Flag = OSD_FILENAME;

	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE) {
			if (m_bEndOfStream) {
				m_bEndOfStream = false;
				SendMessageW(WM_COMMAND, ID_PLAY_STOP);
				SendMessageW(WM_COMMAND, ID_PLAY_PLAYPAUSE);
			}
			m_pMS->SetRate(m_PlaybackRate);
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
			if (AfxGetAppSettings().iDefaultCaptureDevice == 1) {
				CComQIPtr<IBDATuner> pTun = m_pGB;
				if (pTun) {
					pTun->SetChannel (AfxGetAppSettings().nDVBLastChannel);
					DisplayCurrentChannelOSD();
				}
			}
			else if (pAMTuner) {
				long lChannel = 0, lVivSub = 0, lAudSub = 0;
				pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);
				long lFreq = 0;
				pAMTuner->get_VideoFrequency(&lFreq);

				strOSD.Format(ResStr(IDS_CAPTURE_CHANNEL_FREQ), lChannel, lFreq / 1000000.0);
				OSD_Flag = OSD_ENABLE;
			}
		}

		SetTimersPlay();
		if (m_fFrameSteppingActive) { // FIXME
			m_fFrameSteppingActive = false;
			m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
		} else {
			m_pBA->put_Volume(m_wndToolBar.Volume);
		}

		SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
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

			if (strOSD.IsEmpty()) {
				BeginEnumFilters(m_pGB, pEF, pBF) {
					if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
						if (!CheckMainFilter(pBF)) {
							continue;
						}

						CComBSTR bstr;
						if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
							strOSD = bstr.m_str;
							break;
						}
					}
				}
				EndEnumFilters;
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
					strOSD.AppendFormat(L" %s", BLU_RAY);
					if (m_BDLabel.GetLength() > 0) {
						strOSD.AppendFormat(L" \"%s\"", m_BDLabel);
					} else {
						MakeBDLabel(GetCurFileName(), strOSD);
					}

					strOSD.AppendFormat(L" (%s)", GetFileOnly(m_strPlaybackRenderedPath));
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
		OSD_Flag = OSD_ENABLE;
		strOSD = ResStr(ID_PLAY_PLAY);
		int i = strOSD.Find('\n');
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
	}

	if (AfxGetAppSettings().iShowOSD & OSD_Flag) {
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
	}
}

void CMainFrame::OnPlayPause()
{
	OAFilterState fs = GetMediaState();

	if (m_eMediaLoadState == MLS_LOADED && fs == State_Stopped) {
		MoveVideoWindow();
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
				CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF;
				CComQIPtr<IFileSourceFilter> pFSF = pBF;
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

			if (m_bUseSmartSeek && m_pDVDC_preview) {
				m_pDVDC_preview->SetOption(DVD_ResetOnStop, TRUE);
				m_pMC_preview->Stop();
				m_pDVDC_preview->SetOption(DVD_ResetOnStop, FALSE);
			}

		} else if (GetPlaybackMode() == PM_CAPTURE) {
			m_pMC->Stop();
		}

		if (m_fFrameSteppingActive) { // FIXME
			m_fFrameSteppingActive = false;
			m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
		}

		if (m_pPlaybackNotify) {
			m_pPlaybackNotify->Stop();
		}
	}

	m_nLoops = 0;

	if (m_hWnd) {
		KillTimersStop();
		MoveVideoWindow();

		if (m_eMediaLoadState == MLS_LOADED) {
			__int64 stop;
			m_wndSeekBar.GetRange(stop);
			if (GetPlaybackMode() != PM_CAPTURE) {
				m_wndStatusBar.SetStatusTimer(m_wndSeekBar.GetPosReal(), stop, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
			}

			SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
		}
	}

	if (!m_bEndOfStream) {
		CString strOSD = ResStr(ID_PLAY_STOP);
		int i = strOSD.Find(L"\n");
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_STOPPED), 3000);
	}

	SetPlayState(PS_STOP);
}

void CMainFrame::OnUpdatePlayPauseStop(CCmdUI* pCmdUI)
{
	OAFilterState fs = m_fFrameSteppingActive ? State_Paused : GetMediaState();

	pCmdUI->SetCheck(fs == State_Running && pCmdUI->m_nID == ID_PLAY_PLAY
					 || fs == State_Paused && pCmdUI->m_nID == ID_PLAY_PAUSE
					 || fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_STOP
					 || (fs == State_Paused || fs == State_Running) && pCmdUI->m_nID == ID_PLAY_PLAYPAUSE);

	bool fEnable = false;

	if (fs >= 0) {
		if (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_CAPTURE) {
			fEnable = true;

			if (m_fCapturing) {
				fEnable = false;
			} else if (m_fLiveWM && pCmdUI->m_nID == ID_PLAY_PAUSE) {
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

		m_fFrameSteppingActive = true;

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
			!m_fLiveWM) {
		if (S_OK == m_pMS->IsFormatSupported(&TIME_FORMAT_FRAME)) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEP) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEPCANCEL && m_pFS && m_pFS->CanStep(0, nullptr) == S_OK) {
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

	if (m_fShockwaveGraph) {
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

		REFERENCE_TIME rtDuration;
		m_wndSeekBar.GetRange(rtDuration);
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
	if (m_eMediaLoadState != MLS_LOADED || IsD3DFullScreenMode()) {
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

				AM_MEDIA_TYPE mt;
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

	REFERENCE_TIME dur;
	m_wndSeekBar.GetRange(dur);
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
	if (m_eMediaLoadState != MLS_LOADED) {
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
	if (m_eMediaLoadState != MLS_LOADED) {
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
	bool enable = false;

	if (m_eMediaLoadState == MLS_LOADED) {
		bool bIncRate = (pCmdUI->m_nID == ID_PLAY_INCRATE);
		bool bDecRate = (pCmdUI->m_nID == ID_PLAY_DECRATE);

		if (GetPlaybackMode() == PM_CAPTURE && m_wndCaptureBar.m_capdlg.IsTunerActive() && !m_fCapturing) {
			enable = true;
		}
		else if (GetPlaybackMode() == PM_FILE) {
			if (bIncRate && m_PlaybackRate < MAXRATE || bDecRate && m_PlaybackRate > MINRATE) {
				enable = true;
			}
		}
		else if (GetPlaybackMode() == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title) {
			if (bIncRate && m_PlaybackRate < MAXDVDRATE || bDecRate && m_PlaybackRate > MINDVDRATE) {
				enable = true;
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
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter) {
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

	CString strSubDelay;
	strSubDelay.Format(ResStr(IDS_MAINFRM_139), delay_ms);
	SendStatusMessage(strSubDelay, 3000);
	m_OSD.DisplayMessage(OSD_TOPLEFT, strSubDelay);
}

void CMainFrame::OnPlayChangeAudDelay(UINT nID)
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter) {
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

void CMainFrame::OnPlayFilters(UINT nID)
{
	//ShowPPage(m_spparray[nID - ID_FILTERS_SUBITEM_START], m_hWnd);

	CComPtr<IUnknown> pUnk = m_pparray[nID - ID_FILTERS_SUBITEM_START];

	CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), GetModalParent());

	if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk) {
		ps.AddPages(pSPP);
	}

	if (CComQIPtr<IBaseFilter> pBF = pUnk) {
		HRESULT hr;
		CComPtr<IPropertyPage> pPP = DNew CInternalPropertyPageTempl<CPinInfoWnd>(nullptr, &hr);
		ps.AddPage(pPP, pBF);
	}

	if (ps.GetPageCount() > 0) {
		ps.DoModal();
		OpenSetupStatusBar();
	}
}

void CMainFrame::OnUpdatePlayFilters(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_fCapturing);
}

enum {
	ID_SHADERS_SELECT = ID_SHADERS_START,
	ID_SHADERS_SELECT_SCREENSPACE
};

void CMainFrame::OnPlayShaders(UINT nID)
{
	if (nID == ID_SHADERS_SELECT) {
		if (IDOK != CShaderCombineDlg(GetModalParent()).DoModal()) {
			return;
		}
	}

	SetShaders();
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
	if (GetPlaybackMode() == PM_DVD && m_pDVDI) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			m_pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, nullptr);
		}
	}
	else if (m_pDVS) {
		bool fHideSubtitles = false;
		m_pDVS->get_HideSubtitles(&fHideSubtitles);
		fHideSubtitles = !fHideSubtitles;
		m_pDVS->put_HideSubtitles(fHideSubtitles);
		return;
	}
	else {
		AfxGetAppSettings().fEnableSubtitles = !AfxGetAppSettings().fEnableSubtitles;

		if (AfxGetAppSettings().fEnableSubtitles) {
			m_iSubtitleSel = m_nSelSub2;
		} else {
			m_nSelSub2 = m_iSubtitleSel;
			m_iSubtitleSel = -1;
		}

		UpdateSubtitle();
	}
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
	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
		CAutoPtrArray<CPPageSubStyle> pages;
		CAtlArray<STSStyle*> styles;

		POSITION pos = pRTS->m_styles.GetStartPosition();
		for (int i = 0; pos; i++) {
			CString key;
			STSStyle* val;
			pRTS->m_styles.GetNextAssoc(pos, key, val);

			CAutoPtr<CPPageSubStyle> page(DNew CPPageSubStyle());
			page->InitSubStyle(key, val);
			pages.Add(page);
			styles.Add(val);
		}

		CString m_style = ResStr(IDS_SUBTITLES_STYLES);
		int i = m_style.Find(L"&");
		if (i!=-1 ) {
			m_style.Delete(i, 1);
		}
		CPropertySheet dlg(m_style, GetModalParent());
		for (int i = 0; i < (int)pages.GetCount(); i++) {
			dlg.AddPage(pages[i]);
		}

		if (dlg.DoModal() == IDOK) {
			/*
			for (int j = 0; j < (int)pages.GetCount(); j++) {
				pages[j]->GetSubStyle(styles[j]);
			}
			UpdateSubtitle(false, false);
			*/
		}
	}
}

void CMainFrame::OnUpdateSubtitlesStyle(CCmdUI* pCmdUI)
{
	if (dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
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
	CAppSettings& s = AfxGetAppSettings();

	CString osd = ResStr(IDS_SUBTITLES_STEREO);

	switch (nID) {
	case ID_SUBTITLES_STEREO_DONTUSE:
		s.m_VRSettings.iSubpicStereoMode = SUBPIC_STEREO_NONE;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
		break;
	case ID_SUBTITLES_STEREO_SIDEBYSIDE:
		s.m_VRSettings.iSubpicStereoMode = SUBPIC_STEREO_SIDEBYSIDE;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
		break;
	case ID_SUBTITLES_STEREO_TOPBOTTOM:
		s.m_VRSettings.iSubpicStereoMode = SUBPIC_STEREO_TOPANDBOTTOM;
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
		break;
	}

	if (m_pCAP) {
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
		CComQIPtr<IAMStreamSelect> pSSA = m_pSwitcherFilter;
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

	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter) {
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
	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter) {
		CAppSettings& s = AfxGetAppSettings();

		s.bAudioAutoVolumeControl = !s.bAudioAutoVolumeControl;
		pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);

		CString osdMessage = ResStr(s.bAudioAutoVolumeControl ? IDS_OSD_AUTOVOLUMECONTROL_ON : IDS_OSD_AUTOVOLUMECONTROL_OFF);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnUpdateNormalizeVolume(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CMainFrame::OnPlayCenterLevel(UINT nID)
{
	CAppSettings& s = AfxGetAppSettings();

	if (CComQIPtr<IAudioSwitcherFilter> pASF = m_pSwitcherFilter) {
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
		CAppSettings& s = AfxGetAppSettings();
		int& brightness = s.iBrightness;
		int& contrast   = s.iContrast;
		int& hue        = s.iHue;
		int& saturation = s.iSaturation;
		CString tmp, str;

		switch (nID) {

		case ID_COLOR_BRIGHTNESS_INC:
			brightness += 2;
		case ID_COLOR_BRIGHTNESS_DEC:
			brightness -= 1;
			SetColorControl(ProcAmp_Brightness, brightness, contrast, hue, saturation);
			brightness ? tmp.Format(L"%+d", brightness) : tmp = L"0";
			str.Format(IDS_OSD_BRIGHTNESS, tmp);
			break;

		case ID_COLOR_CONTRAST_INC:
			contrast += 2;
		case ID_COLOR_CONTRAST_DEC:
			contrast -= 1;
			SetColorControl(ProcAmp_Contrast, brightness, contrast, hue, saturation);
			contrast ? tmp.Format(L"%+d", contrast) : tmp = L"0";
			str.Format(IDS_OSD_CONTRAST, tmp);
			break;

		case ID_COLOR_HUE_INC:
			hue += 2;
		case ID_COLOR_HUE_DEC:
			hue -= 1;
			SetColorControl(ProcAmp_Hue, brightness, contrast, hue, saturation);
			hue ? tmp.Format(L"%+d", hue) : tmp = L"0";
			str.Format(IDS_OSD_HUE, tmp);
			break;

		case ID_COLOR_SATURATION_INC:
			saturation += 2;
		case ID_COLOR_SATURATION_DEC:
			saturation -= 1;
			SetColorControl(ProcAmp_Saturation, brightness, contrast, hue, saturation);
			saturation ? tmp.Format(L"%+d", saturation) : tmp = L"0";
			str.Format(IDS_OSD_SATURATION, tmp);
			break;

		case ID_COLOR_RESET:
			m_ColorCintrol.GetDefaultValues(brightness, contrast, hue, saturation);
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

	pCmdUI->SetCheck(FALSE);

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

	if (bChecked) {
		if (pCmdUI->m_nID >= ID_AFTERPLAYBACK_EXIT && pCmdUI->m_nID <= ID_AFTERPLAYBACK_EVERYTIMEDONOTHING) {
			CheckMenuRadioItem(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, pCmdUI->m_nID);
		} else if (pCmdUI->m_nID >= ID_AFTERPLAYBACK_CLOSE && pCmdUI->m_nID <= ID_AFTERPLAYBACK_LOCK) {
			CheckMenuRadioItem(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_LOCK, pCmdUI->m_nID);
		} else if (pCmdUI->m_nID == ID_AFTERPLAYBACK_NEXT || pCmdUI->m_nID == ID_AFTERPLAYBACK_DONOTHING) {
			CheckMenuRadioItem(pCmdUI->m_nID, pCmdUI->m_nID, pCmdUI->m_nID);
		}
	}
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
			__int64 stop;
			m_wndSeekBar.GetRange(stop);

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
			CComQIPtr<IBDATuner> pTun = m_pGB;
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
		else if (pAMTuner) {
			if (GetMediaState() != State_Running) {
				SendMessageW(WM_COMMAND, ID_PLAY_PLAY);
			}

			long lChannelMin = 0, lChannelMax = 0;
			pAMTuner->ChannelMinMax(&lChannelMin, &lChannelMax);
			long lChannel = 0, lVivSub = 0, lAudSub = 0;
			pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);

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

			if (FAILED(pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT))) {
				return;
			}

			long flFoundSignal = 0;
			pAMTuner->AutoTune(lChannel, &flFoundSignal);
			long lFreq = 0;
			pAMTuner->get_VideoFrequency(&lFreq);
			long lSignalStrength;
			pAMTuner->SignalPresent(&lSignalStrength); // good if AMTUNER_SIGNALPRESENT

			CString strOSD;
			strOSD.Format(ResStr(IDS_CAPTURE_CHANNEL_FREQ), lChannel, lFreq/1000000.0);
			m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		}
	}
}

void CMainFrame::OnUpdateNavigateSkip(CCmdUI* pCmdUI)
{
	BOOL bOn = FALSE;
	if (m_eMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu && m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu) {
			bOn = TRUE;
		} else if (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing) {
			bOn = TRUE;
		} else if (GetPlaybackMode() == PM_FILE) {
			if (m_pCB->ChapGetCount() > 1) {
				bOn = TRUE;
			} else if (m_wndPlaylistBar.GetCount() == 1) {
				if (m_bIsBDPlay) {
					bOn = !m_BDPlaylists.empty();
				} else if (!AfxGetAppSettings().fDontUseSearchInFolder) {
					bOn = TRUE;
				}
			} else {
				bOn = TRUE;
			}
		}
	}

	pCmdUI->Enable(bOn);
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
				it--;
			} else if (nID == ID_NAVIGATE_SKIPFORWARDFILE) {
				if (it == std::prev(m_BDPlaylists.cend())) {
					return;
				}
				it++;
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
					   || (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing && m_wndPlaylistBar.GetCount() > 1)));
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
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
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
	int streamCount = GetStreamCount(SUBTITLE_GROUP);
	if (streamCount) {
		streamCount--;
	}

	if (GetPlaybackMode() == PM_FILE && !m_pDVS) {
		size_t subStreamCount = 0;
		POSITION pos = m_pSubStreams.GetHeadPosition();
		while (pos) {
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);
			subStreamCount += pSubStream->GetStreamCount();
		}

		if (id >= (subStreamCount + streamCount)) {
			id = 0;
		}
	}

	int splsubcnt = 0;
	const int i = (int)id;

	if (GetPlaybackMode() == PM_FILE && m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

			int subcount = GetStreamCount(SUBTITLE_GROUP);
			if (subcount) {
				CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
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

	if ((GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) && i >= 0) {

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS) {
			DWORD cStreams = 0;
			if (!FAILED(pSS->Count(&cStreams))) {

				CComQIPtr<IBaseFilter> pBF = pSS;
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
	if (!AfxGetAppSettings().fEnableSubtitles) {
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

		if (!m_BDPlaylists.empty() && id < m_BDPlaylists.size()) {
			UINT idx = 0;
			for (const auto& Item : m_BDPlaylists) {
				if (idx == id) {
					m_bNeedUnmountImage = FALSE;
					SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					m_bIsBDPlay = TRUE;

					OpenFile(Item.m_strFileName, INVALID_TIME, FALSE);

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
			for(auto item = m_youtubeUrllist.begin(); item != m_youtubeUrllist.end(); ++item) {
				if (idx == id && item->url != m_strPlaybackRenderedPath) {
					const int tagSelected = item->profile->iTag;
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
			__int64 stop;
			m_wndSeekBar.GetRange(stop);

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
			CComQIPtr<IBDATuner> pTun = m_pGB;
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
	CTunerScanDlg		Dlg;
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
			m_data.push_back(*p++);
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
	CAppSettings& s = AfxGetAppSettings();

	CString osdMsg;

	if (GetPlaybackMode() == PM_FILE) {
		CString fn = GetCurFileName();
		CString desc = GetFileOnly(fn);

		std::list<CString> descList;
		descList.push_back(desc);

		if (m_LastOpenBDPath.GetLength() > 0) {
			CString fn2 = BLU_RAY;
			if (m_BDLabel.GetLength() > 0) {
				fn2.AppendFormat(L" \"%s\"", m_BDLabel);
			} else {
				MakeBDLabel(fn, fn2);
			}
			descList.push_front(fn2);
		} else if (!m_youtubeFields.title.IsEmpty()) {
			descList.push_front(m_youtubeFields.title);
		}

		// Name
		CString str;
		if (bShowDialog) {
			CFavoriteAddDlg dlg(descList, fn);
			if (dlg.DoModal() != IDOK) {
				return;
			}
			str = dlg.m_name;
		} else {
			str = descList.front();
		}

		str.Remove(';');

		// RememberPos
		CString pos(L"0");
		if (s.bFavRememberPos) {
			pos.Format(L"%I64d", GetPos());
		}

		str += ';';
		str += pos;

		// RelativeDrive
		CString relativeDrive;
		relativeDrive.Format(L"%d", s.bFavRelativeDrive);

		str += ';';
		str += relativeDrive;

		// Paths
		if (m_LastOpenBDPath.GetLength() > 0 && !m_DiskImage.DriveAvailable()) {
			str += L";" + m_LastOpenBDPath;
		} else {
			CPlaylistItem pli;
			if (m_wndPlaylistBar.GetCur(pli)) {
				for (const auto& fi : pli.m_fns) {
					str += L";" + fi.GetName();
				}
			}
		}

		str.AppendFormat(L";%d;%d", GetAudioTrackIdx(), GetSubtitleTrackIdx());

		s.AddFav(FAV_FILE, str);

		osdMsg = ResStr(IDS_FILE_FAV_ADDED);
	} else if (GetPlaybackMode() == PM_DVD) {
		WCHAR path[MAX_PATH] = { 0 };
		ULONG len = 0;
		if (SUCCEEDED(m_pDVDI->GetDVDDirectory(path, _countof(path), &len))) {
			CString fn = path;
			fn.TrimRight(L"/\\");

			std::list<CString> fnList;

			// Name
			CString str;
			if (bShowDialog) {
				CFavoriteAddDlg dlg(fnList, fn, FALSE);
				if (dlg.DoModal() != IDOK) {
					return;
				}
				str = dlg.m_name;
			} else {
				str = fn;
			}

			str.Remove(';');

			// RememberPos - seek bar position
			CString pos(L"0");
			if (s.bFavRememberPos) {
				pos.Format(L"%I64d", GetPos());
			}

			str += ';';
			str += pos;

			// RememberPos - DVD state
			pos = L"0";
			if (s.bFavRememberPos) {
				CDVDStateStream stream;
				stream.AddRef();

				CComPtr<IDvdState> pStateData;
				CComQIPtr<IPersistStream> pPersistStream;
				if (SUCCEEDED(m_pDVDI->GetState(&pStateData))
						&& (pPersistStream = pStateData)
						&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream))) {
					pos = BinToCString(stream.m_data.data(), stream.m_data.size());
				}
			}

			str += ';';
			str += pos;

			// Paths
			str += ';';
			str += fn;

			s.AddFav(FAV_DVD, str);

			osdMsg = ResStr(IDS_DVD_FAV_ADDED);
		}
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

void CMainFrame::OnRecentFileClear()
{
	if (IDYES != AfxMessageBox(ResStr(IDS_RECENT_FILES_QUESTION), MB_YESNO)) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	for (int i = s.MRU.GetSize() - 1; i >= 0; i--) {
		s.MRU.Remove(i);
	}
	for (int i = s.MRUDub.GetSize() - 1; i >= 0; i--) {
		s.MRUDub.Remove(i);
	}
	s.MRU.WriteList();
	s.MRUDub.WriteList();

	// Empty the "Recent" jump list
	CComPtr<IApplicationDestinations> pDests;
	HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, nullptr, CLSCTX_INPROC_SERVER);
	if (SUCCEEDED(hr)) {
		hr = pDests->RemoveAllDestinations();
	}

	s.ClearFilePositions();
	s.ClearDVDPositions();
}

void CMainFrame::OnUpdateRecentFileClear(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CMainFrame::OnFavoritesFile(UINT nID)
{
	nID -= ID_FAVORITES_FILE_START;

	std::list<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	if (nID < sl.size()) {
		auto it = std::next(sl.begin(), nID);
		PlayFavoriteFile(*it);
	}
}

void CMainFrame::PlayFavoriteFile(CString fav)
{
	std::list<CString> args;
	REFERENCE_TIME rtStart = 0;
	BOOL bRelativeDrive = FALSE;

	ExplodeEsc(fav, args, L';');
	args.pop_front();									// desc / name
	swscanf_s(args.front(), L"%I64d", &rtStart);		// pos
	args.pop_front();
	swscanf_s(args.front(), L"%d", &bRelativeDrive);	// relative drive
	args.pop_front();
	rtStart = max(rtStart, 0LL);

	m_nAudioTrackStored    = -1;
	m_nSubtitleTrackStored = -1;
	if (args.size() > 2) {
		if (swscanf_s(args.back(), L"%d", &m_nSubtitleTrackStored) == 1) {
			args.pop_back();
		}
		if (swscanf_s(args.back(), L"%d", &m_nAudioTrackStored) == 1) {
			args.pop_back();
		}
	}

	// NOTE: This is just for the favorites but we could add a global settings that does this always when on. Could be useful when using removable devices.
	//       All you have to do then is plug in your 500 gb drive, full with movies and/or music, start MPC-BE (from the 500 gb drive) with a preloaded playlist and press play.
	if (bRelativeDrive) {
		// Get the drive MPC-BE is on and apply it to the path list
		CPath exeDrive(GetProgramPath());

		if (exeDrive.StripToRoot()) {
			for (auto& stringPath : args) {
				CPath path(stringPath);
				const int rootLength = path.SkipRoot();

				if (path.StripToRoot()) {
					if (_wcsicmp(exeDrive, path) != 0) { // Do we need to replace the drive letter ?
						// Replace drive letter
						CString newPath(exeDrive);

						newPath += stringPath.Mid(rootLength);//newPath += stringPath.Mid( 3 );
						stringPath = newPath;
					}
				}
			}
		}
	}

	if (!m_wndPlaylistBar.SelectFileInPlaylist(args.front())) {
		m_wndPlaylistBar.Open(args, false);
	}
	if (GetPlaybackMode() == PM_FILE && args.front() == m_lastOMD->title && !m_bEndOfStream) {
		m_wndPlaylistBar.SetFirstSelected();
		m_pMS->SetPositions(&rtStart, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
		OnPlayPlay();
	} else {
		OpenCurPlaylistItem(rtStart);
	}
}

void CMainFrame::OnUpdateFavoritesFile(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_FILE_START;
	UNREFERENCED_PARAMETER(nID);
}

void CMainFrame::OnRecentFile(UINT nID)
{
	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	MRU.ReadList();

	nID -= ID_RECENT_FILE_START;
	CString str;
	UINT ID = 0;
	for (int i = 0; i < MRU.GetSize(); i++) {
		if (!MRU[i].IsEmpty()) {
			if (ID == nID) {
				str = MRU[i];
				break;
			}
			ID++;
		}
	}

	if (str.IsEmpty()) {
		return;
	}

	if (!m_wndPlaylistBar.SelectFileInPlaylist(str)) {
		m_wndPlaylistBar.Open(str);
	}
	OpenCurPlaylistItem();
}

void CMainFrame::OnUpdateRecentFile(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_RECENT_FILE_START;
	UNREFERENCED_PARAMETER(nID);
}

void CMainFrame::OnFavoritesDVD(UINT nID)
{
	nID -= ID_FAVORITES_DVD_START;

	std::list<CString> sl;
	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	if (nID < sl.size()) {
		auto it = std::next(sl.begin(), nID);
		PlayFavoriteDVD(*it);
	}
}

void CMainFrame::PlayFavoriteDVD(CString fav)
{
	CString fn;
	CDVDStateStream stream;
	stream.AddRef();

	std::list<CString> args;
	ExplodeEsc(fav, args, L';', 4);

	auto it = args.cbegin();
	++it; // desc / name
	if (args.size() == 4) {
		++it; // pos
	}
	CString state = *it++; // state
	if (state != L"0") {
		CStringToBin(state, stream.m_data);
	}
	fn = *it++; // path

	SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	CComPtr<IDvdState> pDvdState;
	if (stream.m_data.size()) {
		HRESULT hr = OleLoadFromStream((IStream*)&stream, IID_PPV_ARGS(&pDvdState));
		UNREFERENCED_PARAMETER(hr);
	}

	AfxGetAppSettings().bNormalStartDVD = false;

	CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
	if (p) {
		p->path = fn;
		p->pDvdState = pDvdState;
	}
	OpenMedia(p);
}

void CMainFrame::OnUpdateFavoritesDVD(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DVD_START;
	UNREFERENCED_PARAMETER(nID);
}

void CMainFrame::OnFavoritesDevice(UINT nID)
{
	nID -= ID_FAVORITES_DEVICE_START;
}

void CMainFrame::OnUpdateFavoritesDevice(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DEVICE_START;
	UNREFERENCED_PARAMETER(nID);
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
	ShellExecuteW(m_hWnd, L"open", L"http://sourceforge.net/projects/mpcbe/files/Toolbars/", nullptr, nullptr, SW_SHOWDEFAULT);
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
		CAppSettings& s = AfxGetAppSettings();
		switch (nID) {
			case ID_SUB_POS_UP:
				s.m_VRSettings.SubpicShiftPos.y--;
				break;
			case ID_SUB_POS_DOWN:
				s.m_VRSettings.SubpicShiftPos.y++;
				break;
			case ID_SUB_POS_LEFT:
				s.m_VRSettings.SubpicShiftPos.x--;
				break;
			case ID_SUB_POS_RIGHT:
				s.m_VRSettings.SubpicShiftPos.x++;
				break;
			case ID_SUB_POS_RESTORE:
				s.m_VRSettings.SubpicShiftPos.x = 0;
				s.m_VRSettings.SubpicShiftPos.y = 0;
				break;
		}

		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	}
}

void CMainFrame::OnSubCopyClipboard()
{
	if (m_pCAP && GetPlaybackMode() == PM_FILE && m_pMS) {
		if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
			REFERENCE_TIME rtNow = 0;
			m_pMS->GetCurrentPosition(&rtNow);

			CString text;
			if (pRTS->GetText(rtNow, m_pCAP->GetFPS(), text)) {
				const int len = text.GetLength() + 1;
				if (HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR))) {
					if (WCHAR* sData = (WCHAR*)GlobalLock(hGlob)) {
						wcscpy_s(sData, len, text);
						GlobalUnlock(hGlob);

						if (OpenClipboard()) {
							if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, hGlob)) {
								hGlob = nullptr;
							}

							CloseClipboard();
						}
					}

					if (hGlob) {
						::GlobalFree(hGlob);
					}
				}
			}
		}
	}
}

//////////////////////////////////

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	CAtlArray<HMONITOR>* ml = (CAtlArray<HMONITOR>*)dwData;
	ml->Add(hMonitor);
	return TRUE;
}

void CMainFrame::SetDefaultWindowRect(int iMonitor)
{
	const CAppSettings& s = AfxGetAppSettings();

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
	const CRect rcLastWindowPos = s.rcLastWindowPos;

	if (s.HasFixedWindowSize()) {
		windowSize = s.sizeFixedWindow;
	} else if (s.bRememberWindowSize) {
		windowSize = rcLastWindowPos.Size();
	} else {
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
		CRect windowRect(rcLastWindowPos.TopLeft(), windowSize);
		if (CMonitors::IsOnScreen(windowRect)) {
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
		CRect windowRect(0, 0, max(windowSize.cx, mmi.ptMinTrackSize.x), max(windowSize.cy, mmi.ptMinTrackSize.y));
		monitor.CenterRectToMonitor(windowRect, TRUE);
		SetWindowPos(nullptr, windowRect.left, windowRect.top, windowSize.cx, windowSize.cy, SWP_NOZORDER | SWP_NOACTIVATE);
	}

	if (s.bRememberWindowSize && s.bRememberWindowPos) {
		UINT lastWindowType = s.nLastWindowType;
		if (lastWindowType == SIZE_MAXIMIZED) {
			ShowWindow(SW_MAXIMIZE);
		} else if (lastWindowType == SIZE_MINIMIZED) {
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
		if (s.IsD3DFullscreen()) {
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

	if (!m_bFullScreen && !IsZoomed() && !IsIconic() && !s.bRememberWindowSize) {
		CSize windowSize;

		if (s.HasFixedWindowSize()) {
			windowSize = s.sizeFixedWindow;
		} else if (s.bRememberWindowSize) {
			windowSize = s.rcLastWindowPos.Size();
		} else {
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
			MoveWindow(CRect(s.rcLastWindowPos.TopLeft(), windowSize));
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
		ShowControlBar(&m_wndNavigationBar, !m_wndNavigationBar.IsWindowVisible(), TRUE);
	}
}

CSize CMainFrame::GetVideoSize()
{
	bool fKeepAspectRatio = AfxGetAppSettings().fKeepAspectRatio;
	bool fCompMonDeskARDiff = AfxGetAppSettings().fCompMonDeskARDiff;

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
		CComQIPtr<IBasicVideo2> pBV2 = m_pBV;
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

	CSize& ar = AfxGetAppSettings().sizeAspectRatio;
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

	CAppSettings& s = AfxGetAppSettings();
	CRect r;
	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwRemoveEx = 0, dwAddEx = 0;
	MONITORINFO mi = { sizeof(mi) };

	HMONITOR hm		= MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	HMONITOR hm_cur = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

	CMonitors monitors;

	if (!m_bFullScreen) {
		if (s.bHidePlaylistFullScreen && m_wndPlaylistBar.IsVisible()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(true);
			ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
		}

		if (!m_bFirstFSAfterLaunchOnFullScreen) {
			GetWindowRect(&m_lastWindowRect);
		}
		if (s.AutoChangeFullscrRes.bEnabled == 1 && fSwitchScreenResWhenHasTo && (GetPlaybackMode() != PM_NONE)) {
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

		SetMenuBarVisibility(AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
	} else {
		if (s.AutoChangeFullscrRes.bEnabled == 1 && s.AutoChangeFullscrRes.bApplyDefault && s.AutoChangeFullscrRes.dmFullscreenRes[0].bChecked == 1) {
			SetDispMode(s.AutoChangeFullscrRes.dmFullscreenRes[0].dmFSRes, s.strFullScreenMonitor);
		}

		dwAdd = (s.iCaptionMenuMode == MODE_BORDERLESS ? 0 : s.iCaptionMenuMode == MODE_FRAMEONLY? WS_THICKFRAME: WS_CAPTION | WS_THICKFRAME);
		if (!m_bFirstFSAfterLaunchOnFullScreen) {
			r = m_lastWindowRect;
		}

		if (s.bHidePlaylistFullScreen && m_wndPlaylistBar.IsHiddenDueToFullscreen()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(false);
			ShowControlBar(&m_wndPlaylistBar, TRUE, TRUE);
		}
	}

	m_lastMouseMove.x = m_lastMouseMove.y = -1;

	bool fAudioOnly = m_bAudioOnly;
	m_bAudioOnly = true;

	m_bFullScreen = !m_bFullScreen;

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	ModifyStyleEx(dwRemoveEx, dwAddEx, SWP_NOZORDER);

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
				ShowControlBar(&m_wndNavigationBar, false, TRUE);
			} else if (nTimeOut > 0) {
				SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut * 1000, nullptr);
				SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut * 1000, 2000), nullptr);
			}
		} else {
			ShowControls(CS_NONE, false);
			ShowControlBar(&m_wndNavigationBar, false, TRUE);
		}

		if (s.fPreventMinimize) {
			if (hm != hm_cur) {
				ModifyStyle(WS_MINIMIZEBOX, 0, SWP_NOZORDER);
			}
		}
	} else {
		ModifyStyle(0, WS_MINIMIZEBOX, SWP_NOZORDER);
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		m_bHideCursor = false;
		ShowControls(s.nCS);
		if (GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1) {
			ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
		}
	}

	m_bAudioOnly = fAudioOnly;

	if (!m_bFullScreen) {
		SetMenuBarVisibility(s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU ?
							 AFX_MBV_KEEPVISIBLE : AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
	}

	if (m_bFirstFSAfterLaunchOnFullScreen) { // Play started in Fullscreen
		if (s.bRememberWindowSize || s.bRememberWindowPos) {
			r = s.rcLastWindowPos;
			if (!s.bRememberWindowPos) {
				hm = MonitorFromPoint( CPoint( 0,0 ), MONITOR_DEFAULTTOPRIMARY );
				GetMonitorInfoW(hm, &mi);
				CRect m_r = mi.rcMonitor;
				int left = m_r.left + (m_r.Width() - r.Width())/2;
				int top = m_r.top + (m_r.Height() - r.Height())/2;
				r = CRect(left, top, left + r.Width(), top + r.Height());
			}
			if (!s.bRememberWindowSize) {
				CSize vsize = GetVideoSize();
				r = CRect(r.left, r.top, r.left + vsize.cx, r.top + vsize.cy);
				ShowWindow(SW_HIDE);
			}
			SetWindowPos(nullptr, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
			if (!s.bRememberWindowSize) {
				ZoomVideoWindow();
				ShowWindow(SW_SHOW);
			}
		} else {
			if (m_LastWindow_HM != hm_cur) {
				GetMonitorInfoW(m_LastWindow_HM, &mi);
				r = mi.rcMonitor;
				ShowWindow(SW_HIDE);
				SetWindowPos(nullptr, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
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
		SetWindowPos(nullptr, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	SetAlwaysOnTop(s.iOnTop);

	MoveVideoWindow();

	if (bChangeMonitor && (!m_bToggleShader || !m_bToggleShaderScreenSpace)) { // Enabled shader ...
		SetShaders();
	}

	UpdateThumbarButton();
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
				m_pVMRWC->SetVideoClippingWindow(m_pVideoWnd->m_hWnd);
			}

			if (s.AutoChangeFullscrRes.bEnabled == 1 && s.AutoChangeFullscrRes.bApplyDefault && s.AutoChangeFullscrRes.dmFullscreenRes[0].bChecked == 1) {
				SetDispMode(s.AutoChangeFullscrRes.dmFullscreenRes[0].dmFSRes, s.strFullScreenMonitor, TRUE);
			}

			if (s.iShowOSD & OSD_ENABLE) {
				m_OSD.Start(m_pOSDWnd);
			}

			// Destroy the D3D Fullscreen window
			DestroyD3DWindow();

			MoveVideoWindow();
			RecalcLayout();
		} else {
			// Set the fullscreen display mode
			if (s.AutoChangeFullscrRes.bEnabled == 1 && fSwitchScreenResWhenHasTo) {
				AutoChangeMonitorMode();
			}

			// Create a new D3D Fullscreen window
			CreateFullScreenWindow();

			// Turn on D3D Fullscreen
			m_pD3DFS->SetD3DFullscreen(true);

			// Assign the windowed video frame and pass it to the relevant classes.
			m_pVideoWnd = m_pFullscreenWnd;
			if (m_pMFVDC) {
				m_pMFVDC->SetVideoWindow(m_pVideoWnd->m_hWnd);
			} else {
				m_pVMRWC->SetVideoClippingWindow(m_pVideoWnd->m_hWnd);
			}

			if ((s.iShowOSD & OSD_ENABLE) || s.bShowDebugInfo) {
				if (m_pMFVMB) {
					m_OSD.Start(m_pVideoWnd, m_pMFVMB);
				}
			}

			MoveVideoWindow();
		}
	}
}

void CMainFrame::AutoChangeMonitorMode()
{
	CAppSettings& s = AfxGetAppSettings();
	CString mf_hmonitor = s.strFullScreenMonitor;

	// EnumDisplayDevice... s.strFullScreenMonitorID == sDeviceID ???
	DISPLAY_DEVICE dd = { sizeof(dd) };

	DWORD dev = 0; // device index
	BOOL bMonValid = FALSE;
	CString sDeviceID, strMonName;
	while (EnumDisplayDevices(0, dev, &dd, 0)) {
		DISPLAY_DEVICE ddMon = { sizeof(ddMon) };

		DWORD devMon = 0;
		while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
				sDeviceID.Format(L"%s", ddMon.DeviceID);
				sDeviceID = sDeviceID.Mid (8, sDeviceID.Find (L"\\", 9) - 8);
				if (s.strFullScreenMonitorID == sDeviceID) {
					strMonName.Format(L"%s", ddMon.DeviceName);
					strMonName = strMonName.Left(12);
					mf_hmonitor = /*s.strFullScreenMonitor = */strMonName;
					bMonValid = TRUE;
					break;
				}
			}
			devMon++;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
 		}
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		dev++;
 	}

	// Set Display Mode
	if (s.AutoChangeFullscrRes.bEnabled && bMonValid == TRUE) {
		double dFPS = 0.0;
		if (m_dMediaInfoFPS > 0.9) {
			dFPS = m_dMediaInfoFPS;
		} else {
			const REFERENCE_TIME rtAvgTimePerFrame = std::llround(GetAvgTimePerFrame(FALSE) * 10000000i64);
			if (rtAvgTimePerFrame > 0) {
				dFPS = 10000000.0 / rtAvgTimePerFrame;
			}
		}

		if (dFPS == 0.0) {
			return;
		}

		for (int rs = 0; rs < MaxFpsCount ; rs++) {
			const fpsmode* fsmode = &s.AutoChangeFullscrRes.dmFullscreenRes[rs];
			if (fsmode->bValid
					&& fsmode->bChecked
					&& dFPS >= fsmode->vfr_from
					&& dFPS <= fsmode->vfr_to) {

				SetDispMode(s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes, mf_hmonitor, CanSwitchD3DFS());
				return;
			}
		}
	}
}

bool CMainFrame::GetCurDispMode(dispmode& dm, CString& DisplayName)
{
	return GetDispMode(ENUM_CURRENT_SETTINGS, dm, DisplayName);
}

bool CMainFrame::GetDispMode(int i, dispmode& dm, CString& DisplayName)
{
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	if (DisplayName == L"Current" || DisplayName.IsEmpty()) {
		CMonitor monitor = CMonitors::GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		monitor.GetName(DisplayName);
	}

	dm.bValid = !!EnumDisplaySettings(DisplayName, i, &devmode);

	if (dm.bValid) {
		dm.size = CSize(devmode.dmPelsWidth, devmode.dmPelsHeight);
		dm.bpp = devmode.dmBitsPerPel;
		dm.freq = devmode.dmDisplayFrequency;
		dm.dmDisplayFlags = devmode.dmDisplayFlags;
	}

	return dm.bValid;
}

void CMainFrame::SetDispMode(dispmode& dm, CString& DisplayName, BOOL bForceRegistryMode/* = FALSE*/)
{
	const CAppSettings& s = AfxGetAppSettings();

	dispmode currentdm;
	if (!s.AutoChangeFullscrRes.bEnabled
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

	DEVMODE dmScreenSettings = { 0 };
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
	if (!m_bDelaySetOutputRect && m_eMediaLoadState == MLS_LOADED && !m_bAudioOnly && IsWindowVisible()) {
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

		CRect vr;

		OAFilterState fs = GetMediaState();
		if (fs != State_Stopped || bForcedSetVideoRect || (fs == State_Stopped && m_fShockwaveGraph)) {
			const CSize arxy = GetVideoSize();

			double w = wr.Width();
			double h = wr.Height();
			const long wy = wr.Width() * arxy.cy;
			const long hx = wr.Height() * arxy.cx;

			switch (m_iVideoSize) {
			case DVS_HALF:
				w = arxy.cx / 2;
				h = arxy.cy / 2;
				break;
			case DVS_NORMAL:
				w = arxy.cx;
				h = arxy.cy;
				break;
			case DVS_DOUBLE:
				w = arxy.cx * 2;
				h = arxy.cy * 2;
				break;
			case DVS_FROMINSIDE:
				if (!m_fShockwaveGraph) {
					if (wy > hx) {
						w = (double)hx / arxy.cy;
					} else {
						h = (double)wy / arxy.cx;
					}

					if (m_bFullScreen || IsD3DFullScreenMode()) {
						const CAppSettings& s = AfxGetAppSettings();
						const double factor = (wy > hx) ? w / arxy.cx : h / arxy.cy;

						if (s.bNoSmallUpscale && factor > 1.0 && factor < 1.02
								|| s.bNoSmallDownscale && factor > 0.99 && factor < 1.0) {
							w = arxy.cx;
							h = arxy.cy;
						}
					}
				}
				break;
			case DVS_FROMOUTSIDE:
				if (!m_fShockwaveGraph) {
					if (wy < hx) {
						w = (double)hx / arxy.cy;
					} else {
						h = (double)wy / arxy.cx;
					}
				}
				break;
			case DVS_ZOOM1:
				if (!m_fShockwaveGraph) {
					if (wy > hx) {
						w = ((double)hx + (wy - hx) * 0.333) / arxy.cy;
						h = w * arxy.cy / arxy.cx;
					} else {
						h = ((double)wy + (hx - wy) * 0.333) / arxy.cx;
						w = h * arxy.cx / arxy.cy;
					}
				}
				break;
			case DVS_ZOOM2:
				if (!m_fShockwaveGraph) {
					if (wy > hx) {
						w = ((double)hx + (wy - hx) * 0.667) / arxy.cy;
						h = w * arxy.cy / arxy.cx;
					} else {
						h = ((double)wy + (hx - wy) * 0.667) / arxy.cx;
						w = h * arxy.cx / arxy.cy;
					}
				}
				break;
			}

			CSize size(m_ZoomX * w + 0.5, m_ZoomY * h + 0.5);

			// HACK: remove jitter of frame width
			if (abs(wr.Width() - size.cx) == 1) {
				size.cx = wr.Width();
			}

			CPoint pos(m_PosX * (wr.Width() * 3.0 - m_ZoomX * w) - wr.Width(),
					   m_PosY * (wr.Height() * 3.0 - m_ZoomY * h) - wr.Height());

			vr = CRect(pos, size);
		}

		if (m_pCAP) {
			m_pCAP->SetPosition(wr, vr);
			m_pCAP->Paint(true);
		} else {
			HRESULT hr;
			hr = m_pBV->SetDefaultSourcePosition();
			hr = m_pBV->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
			hr = m_pVW->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());

			if (m_pMFVDC) {
				m_pMFVDC->SetVideoPosition(nullptr, wr);
			}
		}

		m_wndView.SetVideoRect(wr);

		if (bShowStats && vr.Height() > 0) {
			CString info;
			info.Format(L"Pos %.2f %.2f, Zoom %.2f %.2f, AR %.2f", m_PosX, m_PosY, m_ZoomX, m_ZoomY, (float)vr.Width() / vr.Height());
			SendStatusMessage(info, 3000);
		}
	} else {
		m_wndView.SetVideoRect();
	}
}

void CMainFrame::SetPreviewVideoPosition()
{
	if (m_bUseSmartSeek) {
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

		const CPoint pos(m_PosX * (wr.Width() * 3 - w) - wr.Width(), m_PosY * (wr.Height() * 3 - h) - wr.Height());
		const CRect vr(pos, CSize(w, h));

		if (m_pMFVDC_preview) {
			m_pMFVDC_preview->SetVideoPosition(nullptr, wr);
		}

		m_pBV_preview->SetDefaultSourcePosition();
		m_pBV_preview->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
		m_pVW_preview->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());
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
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}

	CAppSettings& s = AfxGetAppSettings();

	if (scale <= 0) {
		scale =
			s.iZoomLevel == 0 ? 0.5 :
			s.iZoomLevel == 1 ? 1.0 :
			s.iZoomLevel == 2 ? 2.0 :
			s.iZoomLevel >= 3 ? GetZoomAutoFitScale() :
			1.0;
	}

	if (m_bFullScreen) {
		OnViewFullscreen();
	}

	MINMAXINFO mmi;
	OnGetMinMaxInfo(&mmi);

	CRect r;
	GetWindowRect(r);

	CSize videoSize = GetVideoSize();
	if (m_bAudioOnly) {
		scale = 1.0;
		videoSize = m_wndView.GetLogoSize();
		videoSize.cx = std::max(videoSize.cx, (LONG)DEFCLIENTW);
		videoSize.cy = std::max(videoSize.cy, (LONG)DEFCLIENTH);
	}

	CSize videoTargetSize(int(videoSize.cx * scale + 0.5), int(videoSize.cy * scale + 0.5));

	CRect decorationsRect;
	VERIFY(AdjustWindowRectEx(decorationsRect, GetWindowStyle(m_hWnd), !IsMenuHidden(), GetWindowExStyle(m_hWnd)));

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
	finalSize.cx = max(finalSize.cx, mmi.ptMinTrackSize.x);
	finalSize.cy = max(finalSize.cy, mmi.ptMinTrackSize.y);

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

	if ((m_bFullScreen || !s.HasFixedWindowSize()) && !IsD3DFullScreenMode()) {
		MoveWindow(r);
	}

	MoveVideoWindow();
}

double CMainFrame::GetZoomAutoFitScale()
{
	if (m_eMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return 1.0;
	}

	const CAppSettings& s = AfxGetAppSettings();
	CSize arxy = GetVideoSize();

	double fitscale = s.nAutoFitFactor / 100.0 * std::min((double)m_rcDesktop.Width() / arxy.cx, (double)m_rcDesktop.Height() / arxy.cy);

	if (s.iZoomLevel == 4 && fitscale > 1.0) {
		fitscale = 1.0;
	}

	return fitscale;
}

CRect CMainFrame::GetInvisibleBorderSize() const
{
	CRect invisibleBorders;
	if (SysVersion::IsWin10orLater()
			&& m_DwmGetWindowAttributeFnc && SUCCEEDED(m_DwmGetWindowAttributeFnc(GetSafeHwnd(), DWMWA_EXTENDED_FRAME_BOUNDS, &invisibleBorders, sizeof(RECT)))) {
		CRect windowRect;
		GetWindowRect(&windowRect);

		invisibleBorders.TopLeft() = invisibleBorders.TopLeft() - windowRect.TopLeft();
		invisibleBorders.BottomRight() = windowRect.BottomRight() - invisibleBorders.BottomRight();
	}

	return invisibleBorders;
}

void CMainFrame::RepaintVideo()
{
	if (!m_bDelaySetOutputRect && GetMediaState() != State_Running) {
		if (m_pCAP) {
			m_pCAP->Paint(false);
		} else if (m_pMFVDC) {
			m_pMFVDC->RepaintVideo();
		}
	}
}

ShaderC* CMainFrame::GetShader(LPCWSTR label)
{
	ShaderC* pShader = nullptr;

	for (auto& shader : m_ShaderCashe) {
		if (shader.label.CompareNoCase(label) == 0) {
			pShader = &shader;
			break;
		}
	}

	if (!pShader) {
		CString path;
		if (AfxGetMyApp()->GetAppSavePath(path)) {
			path.AppendFormat(L"Shaders\\%s.hlsl", label);
			if (::PathFileExistsW(path)) {
				CStdioFile file;
				if (file.Open(path, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText)) {
					ShaderC shader;
					shader.label = label;

					CString str;
					file.ReadString(str); // read first string
					if (str.Left(25) == L"// $MinimumShaderProfile:") {
						shader.profile = str.Mid(25).Trim(); // shader version property
					}
					else {
						file.SeekToBegin();
					}

					if (shader.profile == L"ps_3_sw") {
						shader.profile = L"ps_3_0";
					}
					if (shader.profile != L"ps_2_0"
							&& shader.profile != L"ps_2_a"
							&& shader.profile != L"ps_2_b"
							&& shader.profile != L"ps_3_0") {
						shader.profile = L"ps_2_0";
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

					m_ShaderCashe.push_back(shader);
					pShader = &m_ShaderCashe.back();
				}
			}
		}
	}

	return pShader;
}

bool CMainFrame::SaveShaderFile(ShaderC* shader)
{
	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		path.AppendFormat(L"Shaders\\%s.hlsl", shader->label);

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
				if ((*it).label.CompareNoCase(shader->label) == 0) {
					m_ShaderCashe.erase(it);
					break;
				}
			}

			return true;
		}
	}
	return false;
}

bool CMainFrame::DeleteShaderFile(LPCWSTR label)
{
	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		path.AppendFormat(L"Shaders\\%s.hlsl", label);

		if (!::PathFileExistsW(path) || ::DeleteFileW(path)) {
			// if the file is missing or deleted successfully, then remove it from the cache
			for (auto it = m_ShaderCashe.begin(), end = m_ShaderCashe.end(); it != end; ++it) {
				if ((*it).label.CompareNoCase(label) == 0) {
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
		CString path (appsavepath + L"Shaders\\" + (*it).label + L".hlsl");

		CFile file;
		if (file.Open(path, CFile::modeRead | CFile::modeCreate | CFile::shareDenyNone)) {
			ULONGLONG length = file.GetLength();
			FILETIME ftCreate = {}, ftAccess = {}, ftWrite = {};
			GetFileTime(file.m_hFile, &ftCreate, &ftAccess, &ftWrite);

			file.Close();

			if ((*it).length == length && CompareFileTime(&(*it).ftwrite, &ftWrite) == 0) {
				it++;
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

	CAppSettings& s = AfxGetAppSettings();

	m_pCAP->ClearPixelShaders(TARGET_FRAME);
	m_pCAP->ClearPixelShaders(TARGET_SCREEN);

	if (m_bToggleShader) {
		for (const auto& shader : s.ShaderList) {
			ShaderC* pShader = GetShader(shader);
			if (pShader) {
				CStringA profile = pShader->profile;
				CStringA srcdata = pShader->srcdata;

				HRESULT hr = m_pCAP->AddPixelShader(TARGET_FRAME, srcdata, profile);
				if (FAILED(hr)) {
					m_pCAP->ClearPixelShaders(TARGET_FRAME); // ???
					SendStatusMessage(ResStr(IDS_MAINFRM_73) + pShader->label, 3000);
					return; // ???
				}
			}
		}
	}

	if (m_bToggleShaderScreenSpace) {
		for (const auto& shader : s.ShaderListScreenSpace) {
			ShaderC* pShader = GetShader(shader);
			if (pShader) {
				CStringA profile = pShader->profile;
				CStringA srcdata = pShader->srcdata;

				HRESULT hr = m_pCAP->AddPixelShader(TARGET_SCREEN, srcdata, profile);
				if (FAILED(hr)) {
					m_pCAP->ClearPixelShaders(TARGET_SCREEN); // ???
					SendStatusMessage(ResStr(IDS_MAINFRM_73) + pShader->label, 3000);
					return; // ???
				}
			}
		}
	}
}

void CMainFrame::UpdateShaders(CString label)
{
	auto& shaders = AfxGetAppSettings().ShaderList;

	if (!m_pCAP) {
		return;
	}

	if (shaders.size() <= 1) {
		shaders.clear();
	}

	if (shaders.empty() && !label.IsEmpty()) {
		shaders.push_back(label);
	}

	bool fUpdate = shaders.empty();

	for (const auto& shader : shaders) {
		if (label == shader) {
			fUpdate = true;
			break;
		}
	}

	if (fUpdate) {
		SetShaders();
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

bool CMainFrame::IsRealEngineCompatible(CString strFilename) const
{
	// Real Media engine didn't support Unicode filename (nor filenames with # characters)
	for (int i=0; i<strFilename.GetLength(); i++) {
		WCHAR	Char = strFilename[i];
		if (Char<32 || Char>126 || Char==35) {
			return false;
		}
	}
	return true;
}

CString CMainFrame::OpenCreateGraphObject(OpenMediaData* pOMD)
{
	ASSERT(m_pGB == nullptr);

	m_fCustomGraph = false;
	m_fShockwaveGraph = false;

	CAppSettings& s = AfxGetAppSettings();

	m_pGB_preview = nullptr;

	m_bUseSmartSeek = s.fSmartSeek;
	if (OpenFileData* pFileData = dynamic_cast<OpenFileData*>(pOMD)) {
		CString fn = pFileData->fns.front();
		if (!fn.IsEmpty() && (fn.Find(L"://") >= 0)) { // disable SmartSeek for streaming data.
			m_bUseSmartSeek = false;
		}
	}

	if (OpenFileData* pFileData = dynamic_cast<OpenFileData*>(pOMD)) {
		engine_t engine = s.GetFileEngine(pFileData->fns.front());

		const CString ct = Content::GetType(pFileData->fns.front());

		if (ct == L"video/x-ms-asf") {
			// TODO: put something here to make the windows media source filter load later
		} else if (ct == L"application/x-shockwave-flash") {
			engine = ShockWave;
		}

		HRESULT hr = E_FAIL;
		CComPtr<IUnknown> pUnk;

		if (engine == ShockWave) {
			pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew DSObjects::CShockwaveGraph(m_pVideoWnd->m_hWnd, hr);
			if (!pUnk) {
				return ResStr(IDS_AG_OUT_OF_MEMORY);
			}

			if (SUCCEEDED(hr)) {
				m_pGB = CComQIPtr<IGraphBuilder>(pUnk);
			}
			if (FAILED(hr) || !m_pGB) {
				return ResStr(IDS_MAINFRM_77);
			}
			m_fShockwaveGraph = true;
		}

		m_fCustomGraph = m_fShockwaveGraph;

		if (!m_fCustomGraph) {
			m_pGB = DNew CFGManagerPlayer(L"CFGManagerPlayer", nullptr, m_pVideoWnd->m_hWnd);

			if (m_pGB && m_bUseSmartSeek) {
				// build graph for preview
				m_pGB_preview = DNew CFGManagerPlayer(L"CFGManagerPlayer", nullptr, m_wndPreView.GetVideoHWND(), true);
			}
		}
	} else if (OpenDVDData* pDVDData = dynamic_cast<OpenDVDData*>(pOMD)) {
		m_pGB = DNew CFGManagerDVD(L"CFGManagerDVD", nullptr, m_pVideoWnd->m_hWnd);

		if (m_bUseSmartSeek) {
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

	if (!m_pGB_preview) {
		m_bUseSmartSeek = false;
	}

	m_pGB->AddToROT();

	m_pMC = m_pGB;
	m_pME = m_pGB;
	m_pMS = m_pGB; // general
	m_pVW = m_pGB;
	m_pBV = m_pGB; // video
	m_pBA = m_pGB; // audio
	m_pFS = m_pGB;

	if (m_bUseSmartSeek) {
		m_pGB_preview->AddToROT();

		m_pMC_preview = m_pGB_preview;
		m_pME_preview = m_pGB_preview;
		m_pMS_preview = m_pGB_preview; // general

		m_pVW_preview = m_pGB_preview;
		m_pBV_preview = m_pGB_preview;

		m_pFS_preview = m_pGB_preview;
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

	if (CComQIPtr<IObjectWithSite> pObjectWithSite = m_pGB) {
		pObjectWithSite->SetSite(m_pProv);
	}

	m_pCB = DNew CDSMChapterBag(nullptr, nullptr);

	return L"";
}

HRESULT CMainFrame::PreviewWindowHide()
{
	HRESULT hr = S_OK;

	if (!m_bUseSmartSeek) {
		return E_FAIL;
	}

	if (m_wndPreView.IsWindowVisible()) {
		// Disable animation
		ANIMATIONINFO AnimationInfo;
		AnimationInfo.cbSize = sizeof(ANIMATIONINFO);
		::SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
		int WindowAnimationType = AnimationInfo.iMinAnimate;
		AnimationInfo.iMinAnimate = 0;
		::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

		m_wndPreView.ShowWindow(SW_HIDE);

		// Enable animation
		AnimationInfo.iMinAnimate = WindowAnimationType;
		::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

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
		DVD_PLAYBACK_LOCATION2 Loc, Loc2;
		double fps = 0;

		hr = m_pDVDI->GetCurrentLocation(&Loc);
		if (FAILED(hr)) {
			return hr;
		}

		hr = m_pDVDI_preview->GetCurrentLocation(&Loc2);

		fps = Loc.TimeCodeFlags == DVD_TC_FLAG_25fps ? 25.0
			: Loc.TimeCodeFlags == DVD_TC_FLAG_30fps ? 30.0
			: Loc.TimeCodeFlags == DVD_TC_FLAG_DropFrame ? 29.97
			: 25.0;

		DVD_HMSF_TIMECODE dvdTo = RT2HMSF(rtCur2, fps);

		if (FAILED(hr) || (Loc.TitleNum != Loc2.TitleNum)) {
			hr = m_pDVDC_preview->PlayTitle(Loc.TitleNum, DVD_CMD_FLAG_Flush, nullptr);
			if (FAILED(hr)) {
				return hr;
			}
			m_pDVDC_preview->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
			if (SUCCEEDED(hr)) {
				hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, nullptr);
				if (FAILED(hr)) {
					return hr;
				}
			} else {
				hr = m_pDVDC_preview->PlayChapterInTitle(Loc.TitleNum, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
				hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, nullptr);
				if (FAILED(hr)) {
					hr = m_pDVDC_preview->PlayAtTimeInTitle(Loc.TitleNum, &dvdTo, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, nullptr);
					if (FAILED(hr)) {
						return hr;
					}
				}
			}
		} else {
			hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, nullptr);
			if (FAILED(hr)) {
				return hr;
			}
		}

		m_pDVDI_preview->GetCurrentLocation(&Loc2);

		m_pDVDC_preview->Pause(FALSE);
		m_pMC_preview->Run();
	} else if (GetPlaybackMode() == PM_FILE && m_pMS_preview) {
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
	if (pOFD->fns.empty()) {
		return ResStr(IDS_MAINFRM_81);
	}

	CAppSettings& s = AfxGetAppSettings();

	bool bFirst = true;
	m_youtubeFields.Empty();
	m_youtubeUrllist.clear();

	m_strPlaybackRenderedPath.Empty();

	CString youtubeUrl;
	if (s.bYoutubePageParser && pOFD->fns.size() == 1) {
		CString fn = (CString)pOFD->fns.front();
		if (Youtube::CheckURL(fn)) {
			youtubeUrl = fn;
			std::list<CString> urls;
			if (Youtube::Parse_URL(fn, urls, m_youtubeFields, m_youtubeUrllist, pOFD->subs, pOFD->rtStart)) {
				Content::Online::Disconnect(fn);

				pOFD->fns.clear();

				for (const auto& url : urls) {
					pOFD->fns.push_back(url);
				}

				m_strPlaybackRenderedPath = pOFD->fns.front().GetName();
				m_wndPlaylistBar.SetCurLabel(m_youtubeFields.title);
			}
		}
	}

	for (auto& fi : pOFD->fns) {
		CString fn = fi.GetName();

		fn.Trim();
		if (fn.IsEmpty() && !bFirst) {
			break;
		}

		HRESULT hr = S_OK;

		if (!Content::Online::CheckConnect(fn)) {
			DLog(L"CMainFrame::OpenFile: Connection failed to %s", fn);
			hr = VFW_E_NOT_FOUND;
		}
		Content::Online::Disconnect(fn);

		CorrectAceStream(fn);

		if (SUCCEEDED(hr)) {
			WCHAR path[MAX_PATH] = { 0 };
			BOOL bIsDirSet       = FALSE;
			if (!::PathIsURLW(fn) && ::GetCurrentDirectoryW(MAX_PATH, path)) {
				bIsDirSet = ::SetCurrentDirectoryW(GetFolderOnly(fn));
			}

			if (bFirst) {
				hr = m_pGB->RenderFile(fn, nullptr);
			} else if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB) {
				hr = pGBA->RenderAudioFile(fn);
			}

			if (bIsDirSet) {
				::SetCurrentDirectoryW(path);
			}
		}

		if (FAILED(hr)) {
			if (bFirst) {
				if (s.fReportFailedPins && !IsD3DFullScreenMode()) {
					CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB;
					if (pGBDE && pGBDE->GetCount()) {
						CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
					}
				}

				CString err;

				switch (hr) {
					case E_ABORT:
						err = ResStr(IDS_MAINFRM_82);
						break;
					case E_FAIL:
					case E_POINTER:
					default:
						err = ResStr(IDS_MAINFRM_83);
						break;
					case E_INVALIDARG:
						err = ResStr(IDS_MAINFRM_84);
						break;
					case E_OUTOFMEMORY:
						err = ResStr(IDS_AG_OUT_OF_MEMORY);
						break;
					case VFW_E_CANNOT_CONNECT:
						err = ResStr(IDS_MAINFRM_86);
						break;
					case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
						err = ResStr(IDS_MAINFRM_87);
						break;
					case VFW_E_CANNOT_RENDER:
						err = ResStr(IDS_MAINFRM_88);
						break;
					case VFW_E_INVALID_FILE_FORMAT:
						err = ResStr(IDS_MAINFRM_89);
						break;
					case VFW_E_NOT_FOUND:
						err = ResStr(IDS_MAINFRM_90);
						break;
					case VFW_E_UNKNOWN_FILE_TYPE:
						err = ResStr(IDS_MAINFRM_91);
						break;
					case VFW_E_UNSUPPORTED_STREAM:
						err = ResStr(IDS_MAINFRM_92);
						break;
				}

				return err;
			}
		}

		if (!youtubeUrl.IsEmpty()) {
			fn = youtubeUrl;
		}

		if (m_strPlaybackRenderedPath.IsEmpty()) {
			m_strPlaybackRenderedPath = fn;
		}

		if (m_bUseSmartSeek && bFirst) {
			bool bIsVideo = false;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				// Checks if any Video Renderer is in the graph
				if (IsVideoRenderer(pBF)) {
					bIsVideo = true;
					break;
				}
			}
			EndEnumFilters;

			if (!bIsVideo || FAILED(m_pGB_preview->RenderFile(fn, nullptr))) {
				if (m_pGB_preview) {
					m_pMFVP_preview  = nullptr;
					m_pMFVDC_preview = nullptr;

					m_pMC_preview.Release();
					m_pME_preview.Release();
					m_pMS_preview.Release();
					m_pVW_preview.Release();
					m_pBV_preview.Release();
					m_pFS_preview.Release();

					if (m_pDVDC_preview) {
						m_pDVDC_preview.Release();
						m_pDVDI_preview.Release();
					}

					m_pGB_preview->RemoveFromROT();
					m_pGB_preview.Release();
					m_pGB_preview = nullptr;
				}
				m_bUseSmartSeek = false;
			}
		}

		if (s.bKeepHistory && pOFD->bAddRecent && fn.Find(L"pipe:") == -1) {
			CRecentFileList* pMRU = bFirst ? &s.MRU : &s.MRUDub;
			pMRU->ReadList();
			pMRU->Add(fn);
			pMRU->WriteList();
			if (IsLikelyFilePath(fn)) {
				// there should not be a URL, otherwise explorer dirtied HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
				SHAddToRecentDocs(SHARD_PATHW, fn); // remember the last open files (system) through the drag-n-drop
			}
		}

		if (bFirst) {
			pOFD->title = m_strPlaybackRenderedPath;
			{
				BeginEnumFilters(m_pGB, pEF, pBF);
				if (!m_pMainFSF) {
					m_pMainFSF = pBF;
				}
				if (!m_pKFI) {
					m_pKFI = pBF;
				}
				EndEnumFilters;

				m_pMainSourceFilter = FindFilter(__uuidof(CMpegSplitterFilter), m_pGB);
				if (!m_pMainSourceFilter) {
					m_pMainSourceFilter = FindFilter(__uuidof(CMpegSourceFilter), m_pGB);
				}
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

				if (m_pMainSourceFilter) {
					const CLSID clsid = GetCLSID(m_pMainSourceFilter);
					if (clsid == __uuidof(CMpegSourceFilter) || clsid == __uuidof(CMpegSplitterFilter)) {
						m_bMainIsMPEGSplitter = true;
					}
				}
			}

			if (fi.GetChapterCount()) {
				CComQIPtr<IDSMChapterBag> pCB;

				BeginEnumFilters(m_pGB, pE, pBF) {
					if (CComQIPtr<IDSMChapterBag> pCB2 = pBF) {
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

					ChaptersList chaplist;
					fi.GetChapters(chaplist);
					for (size_t i = 0; i < chaplist.size(); i++) {
						pCB->ChapAppend(chaplist[i].rt, chaplist[i].name);
					}
				}
			}
		}

		bFirst = false;

		if (m_fCustomGraph) {
			break;
		}
	}

	if (!pOFD->fns.empty()) {
		const CString fn = !youtubeUrl.IsEmpty() ? youtubeUrl : pOFD->fns.front();
		if (fn.Find(L"pipe:") == -1
				&& s.bKeepHistory && s.bRememberFilePos && !s.NewFile(fn)) {
			const FILE_POSITION* FilePosition = s.CurrentFilePosition();
			if (m_pMS && FilePosition->llPosition > 0) {
				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				if (rtDur > 0) {
					REFERENCE_TIME rtPos = FilePosition->llPosition;
					m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
				}
			}

			if (m_nAudioTrackStored == -1) {
				m_nAudioTrackStored = FilePosition->nAudioTrack;
			}

			if (m_nSubtitleTrackStored == -1) {
				m_nSubtitleTrackStored = FilePosition->nSubtitleTrack;
			}
		}
	}

	if (s.fReportFailedPins) {
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB;
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

	if (FindFilter(__uuidof(CShoutcastSource), m_pGB)) {
		m_fUpdateInfoBar = true;
	}

	SetPlaybackMode(PM_FILE);

	SetupChapters();
	LoadKeyFrames();

	return L"";
}

void CMainFrame::SetupChapters()
{
	if (!m_pCB) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {
		m_pCB->ChapRemoveAll();

		CInterfaceList<IBaseFilter> pBFs;
		BeginEnumFilters(m_pGB, pEF, pBF)
		pBFs.AddTail(pBF);
		EndEnumFilters;

		POSITION pos;

		pos = pBFs.GetHeadPosition();
		while (pos && !m_pCB->ChapGetCount()) {
			IBaseFilter* pBF = pBFs.GetNext(pos);

			CComQIPtr<IDSMChapterBag> pCB = pBF;
			if (!pCB) {
				continue;
			}

			for (DWORD i = 0, cnt = pCB->ChapGetCount(); i < cnt; i++) {
				REFERENCE_TIME rt;
				CComBSTR name;
				if (SUCCEEDED(pCB->ChapGet(i, &rt, &name))) {
					m_pCB->ChapAppend(rt, name);
				}
			}
		}

		pos = pBFs.GetHeadPosition();
		while (pos && !m_pCB->ChapGetCount()) {
			IBaseFilter* pBF = pBFs.GetNext(pos);

			CComQIPtr<IChapterInfo> pCI = pBF;
			if (!pCI) {
				continue;
			}

			CHAR iso6391[3];
			::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, iso6391, 3);
			CStringA iso6392 = ISO6391To6392(iso6391);
			if (iso6392.GetLength() < 3) {
				iso6392 = "eng";
			}

			UINT cnt = pCI->GetChapterCount(CHAPTER_ROOT_ID);
			for (UINT i = 1; i <= cnt; i++) {
				UINT cid = pCI->GetChapterId(CHAPTER_ROOT_ID, i);

				ChapterElement ce;
				if (pCI->GetChapterInfo(cid, &ce)) {
					char pl[3] = {iso6392[0], iso6392[1], iso6392[2]};
					char cc[] = "  ";
					CComBSTR name;
					name.Attach(pCI->GetChapterStringInfo(cid, pl, cc));
					m_pCB->ChapAppend(ce.rtStart, name);
				}
			}
		}

		pos = pBFs.GetHeadPosition();
		while (pos && !m_pCB->ChapGetCount()) {
			IBaseFilter* pBF = pBFs.GetNext(pos);

			CComQIPtr<IAMExtendedSeeking, &IID_IAMExtendedSeeking> pES = pBF;
			if (!pES) {
				continue;
			}

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
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pCB->ChapRemoveAll();

		WCHAR buff[MAX_PATH] = { 0 };
 		ULONG len = 0;
 		DVD_PLAYBACK_LOCATION2 loc;
		ULONG ulNumOfChapters = 0;
 		if (SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, _countof(buff), &len))
				&& SUCCEEDED(m_pDVDI->GetCurrentLocation(&loc))
				&& SUCCEEDED(m_pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters))
				&& ulNumOfChapters > 1) {
 			CStringW path;
			path.Format(L"%s\\video_ts.IFO", buff);

			ULONG VTSN, TTN;
			if (::PathFileExistsW(path) && CIfoFile::GetTitleInfo(path, loc.TitleNum, VTSN, TTN)) {
				path.Format(L"%s\\VTS_%02lu_0.IFO", buff, VTSN);

				CIfoFile ifo;
				CVobFile vob;
				if (::PathFileExistsW(path) && ifo.OpenIFO(path, &vob, TTN) && ulNumOfChapters == ifo.GetChaptersCount()) {
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

	m_pCB->ChapSort();
	m_wndSeekBar.SetChapterBag(m_pCB);
	m_OSD.SetChapterBag(m_pCB);
}

CString CMainFrame::OpenDVD(OpenDVDData* pODD)
{
	HRESULT hr = m_pGB->RenderFile(pODD->path, nullptr);

	CAppSettings& s = AfxGetAppSettings();

	if (s.fReportFailedPins) {
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB;
		if (pGBDE && pGBDE->GetCount()) {
			CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
		}
	}

	if (SUCCEEDED(hr) && m_bUseSmartSeek) {
		if (FAILED(hr = m_pGB_preview->RenderFile(pODD->path, nullptr))) {
			m_bUseSmartSeek = false;
		}
	}

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if ((m_pDVDC = pBF) && (m_pDVDI = pBF)) {
			break;
		}
	}
	EndEnumFilters;

	if (m_bUseSmartSeek) {
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
	if (SUCCEEDED(hr = m_pDVDI->GetDVDDirectory(buff, _countof(buff), &len))) {
		pODD->title = CString(buff);
		pODD->title.TrimRight('\\');
		if (pODD->bAddRecent) {
			AddRecent(pODD->title);
		}
	}

	// TODO: resetdvd
	m_pDVDC->SetOption(DVD_ResetOnStop, FALSE);
	m_pDVDC->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);

	if (m_bUseSmartSeek && m_pDVDC_preview) {
		m_pDVDC_preview->SetOption(DVD_ResetOnStop, FALSE);
		m_pDVDC_preview->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);
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

	pCGB = nullptr;
	pVidCap = nullptr;
	pAudCap = nullptr;

	if (FAILED(pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2))) {
		return ResStr(IDS_MAINFRM_99);
	}

	HRESULT hr;

	pCGB->SetFiltergraph(m_pGB);

	if (pVidCapTmp) {
		if (FAILED(hr = m_pGB->AddFilter(pVidCapTmp, vidfrname))) {
			return ResStr(IDS_CAPTURE_ERROR_VID_FILTER);
		}

		pVidCap = pVidCapTmp;

		if (!pAudCapTmp) {
			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))
					&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))
					&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC))
					&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap");
			} else {
				pAudCap = pVidCap;
			}
		} else {
			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))) {
				DLog(L"Warning: No IAMStreamConfig interface for vidcap capture");
			}
		}

		if (FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, nullptr, pVidCap, IID_IAMCrossbar, (void**)&pAMXBar))) {
			DLog(L"Warning: No IAMCrossbar interface was found");
		}

		if (FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, nullptr, pVidCap, IID_IAMTVTuner, (void**)&pAMTuner))) {
			DLog(L"Warning: No IAMTVTuner interface was found");
		}

		if (pAMTuner) { // load saved channel
			pAMTuner->put_CountryCode(AfxGetMyApp()->GetProfileInt(L"Capture", L"Country", 1));

			int vchannel = pODD->vchannel;
			if (vchannel < 0) {
				vchannel = AfxGetMyApp()->GetProfileInt(L"Capture\\" + CString(m_VidDispName), L"Channel", -1);
			}
			if (vchannel >= 0) {
				OAFilterState fs = State_Stopped;
				m_pMC->GetState(0, &fs);
				if (fs == State_Running) {
					m_pMC->Pause();
				}
				pAMTuner->put_Channel(vchannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
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

		pAudCap = pAudCapTmp;

		if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC))
				&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC))) {
			DLog(L"Warning: No IAMStreamConfig interface for vidcap");
		}
		/*
		CInterfaceArray<IAMAudioInputMixer> pAMAIM;

		BeginEnumPins(pAudCap, pEP, pPin)
		{
			PIN_DIRECTION dir;
			if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_INPUT)
				continue;

			if (CComQIPtr<IAMAudioInputMixer> pAIM = pPin)
				pAMAIM.Add(pAIM);
		}
		EndEnumPins;

		if (pAMASC)
		{
			m_wndCaptureBar.m_capdlg.SetupAudioControls(auddispname, pAMASC, pAMAIM);
		}
		*/
	}

	if (!(pVidCap || pAudCap)) {
		return ResStr(IDS_MAINFRM_108);
	}

	pODD->title = ResStr(IDS_CAPTURE_LIVE);

	SetPlaybackMode(PM_CAPTURE);

	return L"";
}

void CMainFrame::OpenCustomizeGraph()
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		return;
	}

	CleanGraph();

	if (GetPlaybackMode() == PM_FILE) {
		if (m_pCAP && AfxGetAppSettings().IsISRAutoLoadEnabled()) {
			AddTextPassThruFilter();
		}
	}

	CAppSettings& s = AfxGetAppSettings();
	const CRenderersSettings& rs = s.m_VRSettings;
	if (rs.iVideoRenderer == VIDRNDT_SYNC && rs.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
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
		mediaFilter = nullptr;
		refClock = nullptr;

		m_pRefClock->QueryInterface(IID_PPV_ARGS(&m_pSyncClock));

		CComQIPtr<ISyncClockAdviser> pAdviser = m_pCAP;
		if (pAdviser) {
			pAdviser->AdviseSyncClock(m_pSyncClock);
		}
	}

	if (GetPlaybackMode() == PM_DVD) {
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IDirectVobSub2> pDVS2 = pBF) {
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
	if (m_pMVRI) {
		// get initial rotation value for madVR
		int rotation;
		if (S_OK == (m_pMVRI->GetInt("rotation", &rotation))) {
			m_iDefRotation = rotation;
		}
	}

	m_bAudioOnly = true;

	if (m_pMFVDC) {// EVR
		m_bAudioOnly = false;
	} else if (m_pCAP) {
		CSize vs = m_pCAP->GetVideoSizeAR();
		m_bAudioOnly = (vs.cx <= 0 || vs.cy <= 0);
	} else {
		{
			long w = 0, h = 0;

			if (CComQIPtr<IBasicVideo> pBV = m_pGB) {
				pBV->GetVideoSize(&w, &h);
			}

			if (w > 0 && h > 0) {
				m_bAudioOnly = false;
			}
		}

		if (m_bAudioOnly) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				long w = 0, h = 0;

				if (CComQIPtr<IVideoWindow> pVW = pBF) {
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

	if (m_fShockwaveGraph) {
		m_bAudioOnly = false;
	}

	if (m_pCAP) {
		SetShaders();
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

		if (m_bUseSmartSeek) {
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
		if (pVidCap && pAMVSCCap) {
			CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = pVidCap;

			if (!pAMXBar && pVfwCD) {
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pVfwCD);
			} else {
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pAMXBar, pAMTuner);
			}
		}

		if (pAudCap && pAMASC) {
			CInterfaceArray<IAMAudioInputMixer> pAMAIM;

			BeginEnumPins(pAudCap, pEP, pPin) {
				if (CComQIPtr<IAMAudioInputMixer> pAIM = pPin) {
					pAMAIM.Add(pAIM);
				}
			}
			EndEnumPins;

			m_wndCaptureBar.m_capdlg.SetupAudioControls(m_AudDispName, pAMASC, pAMAIM);
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
		BeginEnumFilters(m_pGB, pEF, pBF) {

			if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
				if (!CheckMainFilter(pBF)) {
					continue;
				}

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
		EndEnumFilters;

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

		if (!m_pBI && (m_pBI = pBF)) {
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

	if (!m_fCustomGraph) {
		UINT id = IDB_NOAUDIO;

		BeginEnumFilters(m_pGB, pEF, pBF) {
			CComQIPtr<IBasicAudio> pBA = pBF;
			if (!pBA) {
				continue;
			}

			BeginEnumPins(pBF, pEP, pPin) {
				if (S_OK == m_pGB->IsPinDirection(pPin, PINDIR_INPUT)
						&& S_OK == m_pGB->IsPinConnected(pPin)) {
					AM_MEDIA_TYPE mt;
					memset(&mt, 0, sizeof(mt));
					pPin->ConnectionMediaType(&mt);

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
						id = NULL; // need comment for this
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

void CMainFrame::OpenSetupWindowTitle(CString fn)
{
	if (fn.IsEmpty()) {
		return;
	}

	CString title;
	title.LoadString(IDR_MAINFRAME);
	CString fname = fn;

	CAppSettings& s = AfxGetAppSettings();

	int i = s.iTitleBarTextStyle;

	if (!fn.IsEmpty()) {
		if (GetPlaybackMode() == PM_FILE) {
			if (m_LastOpenBDPath.GetLength() > 0) {
				CString fn2 = BLU_RAY;
				if (m_BDLabel.GetLength() > 0) {
					fn2.AppendFormat(L" \"%s\"", m_BDLabel);
				} else {
					MakeBDLabel(fn, fn2);
				}
				fn = fn2;
			} else {
				fn.Replace('\\', '/');
				CString fn2 = fn.Mid(fn.ReverseFind('/') + 1);
				if (!fn2.IsEmpty()) {
					fn = fn2;
				}
			}
		} else if (GetPlaybackMode() == PM_DVD) {
			fn = L"DVD";
			MakeDVDLabel(L"", fn);
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			fn = ResStr(IDS_CAPTURE_LIVE);
		}
	}

	m_strPlaybackLabel = fn;

	if (i == 1) {
		if (s.bTitleBarTextTitle) {
			if (!m_youtubeFields.title.IsEmpty()) {
				fn = m_youtubeFields.title;
			}

			bool bGetTitle = false;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
					if (!CheckMainFilter(pBF)) {
						continue;
					}

					CComBSTR bstr;
					if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
						fn = CString(bstr.m_str);
						bGetTitle = true;
						break;
					}
				}
			}
			EndEnumFilters;

			if (!bGetTitle) {
				CPlaylistItem pli;
				if (m_wndPlaylistBar.GetCur(pli) && !pli.m_fns.empty() && !pli.m_label.IsEmpty()) {
					fn = pli.m_label;
				}
			}
		}
		title = fn + L" - " + m_strTitle;
	} else if (i == 0) {
		title = fname + L" - " + m_strTitle;
	}

	CString curTitle;
	GetWindowTextW(curTitle);
	if (curTitle != title) {
		SetWindowTextW(title);
	}

	m_Lcd.SetMediaTitle(fn);
}

BOOL CMainFrame::SelectMatchTrack(std::vector<Stream>& Tracks, CString pattern, BOOL bExtPrior, size_t& nIdx)
{
	CharLower(pattern.GetBuffer());
	pattern.Replace(L"[fc]", L"forced");
	pattern.Replace(L"[def]", L"default");
	if (pattern.Find(L"forced") == -1) {
		pattern.Append(L" forced");
	}
	if (pattern.Find(L"default") == -1) {
		pattern.Append(L" default");
	}
	pattern.Trim();

	nIdx = 0;
	int tPos = 0;
	CString lang = pattern.Tokenize(L",; ", tPos);
	while (tPos != -1) {
		for (size_t iIndex = 0; iIndex < Tracks.size(); iIndex++) {
			if (bExtPrior && !Tracks[iIndex].Ext) {
				continue;
			}

			CString name(Tracks[iIndex].Name);
			CharLower(name.GetBuffer());

			CAtlList<CString> sl;
			Explode(lang, sl, L'|');
			POSITION pos = sl.GetHeadPosition();

			int nLangMatch = 0;
			while (pos) {
				CString subPattern = sl.GetNext(pos);

				if ((Tracks[iIndex].forced && subPattern == L"forced") || (Tracks[iIndex].def && subPattern == L"default")) {
					nLangMatch++;
					continue;
				}

				if (subPattern[0] == '!') {
					subPattern.Delete(0, 1);
					if (name.Find(subPattern) == -1) {
						nLangMatch++;
					}
				} else if (name.Find(subPattern) >= 0) {
					nLangMatch++;
				}
			}

			if (nLangMatch == sl.GetCount()) {
				nIdx = iIndex;
				return TRUE;
			}
		}

		lang = pattern.Tokenize(L",; ", tPos);
	}

	return FALSE;
}

void CMainFrame::OpenSetupAudioStream()
{
	if (m_eMediaLoadState != MLS_LOADING || GetPlaybackMode() != PM_FILE) {
		return;
	}

	if (m_nAudioTrackStored != -1) {
		SetAudioTrackIdx(m_nAudioTrackStored);
		return;
	}

	CAtlList<CString> extAudioList;
	CPlaylistItem pli;
	if (m_wndPlaylistBar.GetCur(pli)) {
		auto it = pli.m_fns.begin();
		++it; // skip main file
		for (; it != pli.m_fns.end(); ++it) {
			CString str = *it;
			extAudioList.AddTail(GetFileOnly(str));
		}
	}

	CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
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
			const bool bExternal = (extAudioList.Find(name) != nullptr);

			stream.Name  = name;
			stream.Index = i;
			stream.Ext   = bExternal;
			streams.push_back(stream);
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
			if (s.fPrioritizeExternalAudio && extAudioList.GetCount() > 0) {
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
			if (s.fPrioritizeExternalAudio && extAudioList.GetCount() > 0) {
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
	CAppSettings& s	= AfxGetAppSettings();
	CString pattern	= s.strSubtitlesLanguageOrder;

	if (subarray.size()) {
		if (s.fPrioritizeExternalSubtitles) { // try external sub ...
			size_t nIdx	= 0;
			BOOL bMatch = SelectMatchTrack(subarray, pattern, TRUE, nIdx);

			if (bMatch) {
				return nIdx;
 			} else {
				for (size_t iIndex = 0; iIndex < subarray.size(); iIndex++) {
					if (subarray[iIndex].Ext) {
						return iIndex;
					}
				}
			}
		}

		size_t nIdx = 0;
		BOOL bMatch = SelectMatchTrack(subarray, pattern, FALSE, nIdx);

		if (bMatch) {
			return nIdx;
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
			subarray.clear();

			int subcount = GetStreamCount(2);
			CComQIPtr<IAMStreamSelect> pSS	= m_pMainSourceFilter;
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
						subarray.push_back(substream);

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
							substream.Sel = subarray.size();
						}
						subarray.push_back(substream);

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

						if (CComQIPtr<IDirectVobSub3> pDVS3 = m_pDVS) {
							int nType = 0;
							pDVS3->get_LanguageType(i, &nType);
							substream.Ext = !!nType;
						}

						subarray.push_back(substream);

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
		subarray.clear();
		int checkedsplsub	= 0;
		int subIndex		= -1;
		int iNum			= 0;
		cntintsub			= 0;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (!pSS) {
			pSS = m_pGB;
		}
		if (pSS && !s.fDisableInternalSubtitles) {

			DWORD cStreams;
			if (SUCCEEDED(pSS->Count(&cStreams))) {

				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS;
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

						CString lang;
						if (lcid == 0) {
							lang = pName;
						} else {
							int len = GetLocaleInfoW(lcid, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
							lang.ReleaseBufferSetLength(max(len - 1, 0));
						}

						UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
						if (dwFlags) {
							checkedsplsub = subIndex;
						}
						cntintsub++;
						substream.Ext		= false;
						substream.Filter	= 1;
						substream.Num		= iNum++;
						substream.Index		= i;

						bool Forced, Def;
						SubFlags(lang, Forced, Def);

						substream.Name		= pName;
						substream.forced	= Forced;
						substream.def		= Def;

						subarray.push_back(substream);
					}

					if (pName) {
						CoTaskMemFree(pName);
					}
				}
			}
		}

		if (AfxGetAppSettings().fDisableInternalSubtitles) {
			m_pSubStreams.RemoveAll();
		}

		int splsubcnt = subarray.size();

		if (splsubcnt < 1) {
			POSITION pos = m_pSubStreams.GetHeadPosition();
			int tPos = -1;
			while (pos) {
				tPos++;
				CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);
				int i = pSubStream->GetStream();
				WCHAR* pName = nullptr;
				LCID lcid;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, &lcid))) {
					cntintsub++;

					CString lang;
					if (lcid == 0) {
						lang = CString(pName);
					} else {
						int len = GetLocaleInfoW(lcid, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
						lang.ReleaseBufferSetLength(max(len - 1, 0));
					}

					substream.Ext		= false;
					substream.Filter	= 2;
					substream.Num		= iNum++;
					substream.Index		= tPos;

					bool Forced, Def;
					SubFlags(lang, Forced, Def);

					substream.Name		= pName;
					substream.forced	= Forced;
					substream.def		= Def;

					subarray.push_back(substream);

					if (pName) {
						CoTaskMemFree(pName);
					}
				}
			}
		}

		ATL::CInterfaceList<ISubStream, &__uuidof(ISubStream)> subs;
		while (!m_pSubStreams.IsEmpty()) {
			subs.AddTail(m_pSubStreams.RemoveHead());
		}

		int iInternalSub = subs.GetCount();

		for (const auto& si : pOMD->subs) {
			LoadSubtitle(si);
		}

		POSITION pos = m_pSubStreams.GetHeadPosition();
		CComPtr<ISubStream> pSubStream;
		int tPos = -1;
		int extcnt = -1;
		for (size_t i = 0; i < subarray.size(); i++) {
			if (subarray[i].Filter == 2) extcnt++;
		}

		while (pos) {
			pSubStream = m_pSubStreams.GetNext(pos);
			tPos++;
			if (!pSubStream) continue;

			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				WCHAR* pName = nullptr;
				LCID lcid;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, &lcid))) {
					CString name(pName);

					substream.Ext		= true;
					substream.Filter	= 2;
					substream.Num		= iNum++;
					substream.Index		= tPos + (extcnt < 0 ? 0 : extcnt + 1);

					bool Forced, Def;
					SubFlags(name, Forced, Def);

					substream.Name		= name;
					substream.forced	= Forced;
					substream.def		= Def;

					subarray.push_back(substream);

					if (pName) {
						CoTaskMemFree(pName);
					}
				}
			}
		}

		pos = m_pSubStreams.GetHeadPosition();
		while (!subs.IsEmpty()) {
			pos = m_pSubStreams.InsertBefore(pos, subs.RemoveTail());
		}

		if (m_nSubtitleTrackStored != -1) {
			SetSubtitleTrackIdx(m_nSubtitleTrackStored);
			return;
		}

		if (!s.fUseInternalSelectTrackLogic) {
			if (s.fPrioritizeExternalSubtitles) {
				size_t cnt = subarray.size();
				for (size_t i = 0; i < cnt; i++) {
					if (subarray[i].Ext)	{
						checkedsplsub = i;
						break;
					}
				}
			}
			SelectSubtilesAMStream(checkedsplsub);
		} else {
			int cnt = subarray.size();
			size_t defsub = GetSubSelIdx();

			if (m_pMainSourceFilter && s.fDisableInternalSubtitles) {
				defsub++;
			}

			if (cnt > 0) {
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
		pFrame->IsMadVRExclusiveMode = true;
	} else if (event == ExclusiveModeIsAboutToBeLeft) {
		pFrame->IsMadVRExclusiveMode = false;
	}
	pFrame->FlyBarSetPos();
}

#define BREAK(msg) {err = msg; break;}
bool CMainFrame::OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD)
{
	ASSERT(m_eMediaLoadState == MLS_LOADING);

	SetAudioPicture(FALSE);

	m_fValidDVDOpen = false;

	OpenFileData* pFileData		= dynamic_cast<OpenFileData*>(pOMD.m_p);
	OpenDVDData* pDVDData		= dynamic_cast<OpenDVDData*>(pOMD.m_p);
	OpenDeviceData* pDeviceData	= dynamic_cast<OpenDeviceData*>(pOMD.m_p);
	ASSERT(pFileData || pDVDData || pDeviceData);

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();
	m_ExtSubPathsHandles.RemoveAll();
	m_EventSubChangeRefreshNotify.Set();

	m_PlaybackRate = 1.0;
	m_iDefRotation = 0;

	DXVAState::ClearState();

	m_bWasPausedOnMinimizedVideo = false;

	if (pFileData) {
		UINT index = 0;
		for (auto& fi : pFileData->fns) {
			if (::PathIsURLW(fi) && fi.GetName().Find(L"://") <= 0) {
				fi = L"http://" + fi.GetName();
			}

			DLog(L"--> CMainFrame::OpenMediaPrivate() - pFileData->fns[%d]:", index++);
			DLog(L"    %s", fi.GetName());
		}
	}

	CAppSettings& s = AfxGetAppSettings();

	CString mi_fn;
	for (;;) {
		if (pFileData) {
			if (pFileData->fns.empty()) {
				ASSERT(FALSE);
				break;
			}

			CString fn = pFileData->fns.front();

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

		if ((s.AutoChangeFullscrRes.bEnabled == 1 && (IsD3DFullScreenMode() || s.fLaunchfullscreen))
				|| s.AutoChangeFullscrRes.bEnabled == 2) {
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
					mi_fn = GetFolderOnly(mi_fn) + L"\\VTS_01_1.VOB";
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
			}
		}

		break;
	}

	// load fonts from 'fonts' folder
	if (pFileData) {
		const CString path =  GetFolderOnly(pFileData->fns.front()) + L"\\fonts\\";

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

	m_fUpdateInfoBar = false;
	BeginWaitCursor();

	for (;;) {
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		err = OpenCreateGraphObject(pOMD);
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

		m_pCAP = nullptr;

		m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRWC), FALSE); // might have IVMRMixerBitmap9, but not IVMRWindowlessControl9
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRMC9), TRUE);
		m_pMVRSR = m_pCAP;
		m_pMVRS  = m_pCAP;
		m_pMVRC  = m_pCAP;
		m_pMVRI  = m_pCAP;
		m_pMVRFG = m_pCAP;

		m_pMVTO = m_pCAP;

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

			m_pD3DFS = m_pMFVDC;
			m_pPlaybackNotify = m_pMFVDC;
		}

		if (m_bUseSmartSeek) {
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pMFVDC_preview), TRUE);
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pMFVP_preview), TRUE);

			if (m_pMFVDC_preview) {
				RECT Rect;
				m_wndPreView.GetClientRect(&Rect);
				m_pMFVDC_preview->SetVideoWindow(m_wndPreView.GetVideoHWND());
				m_pMFVDC_preview->SetVideoPosition(nullptr, &Rect);
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
			CComQIPtr<IBufferControl> pBC;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (pBC = pBF) {
					pBC->SetBufferDuration(s.iBufferDuration);
				}
			}
			EndEnumFilters;
			pBC.Release();
		}

		OpenCustomizeGraph();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		OpenSetupVideo();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		if ((s.iShowOSD & OSD_ENABLE) || s.bShowDebugInfo) { // Force OSD on when the debug switch is used
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

		OpenSetupWindowTitle(pOMD->title);

		OpenSetupSubStream(pOMD);

		m_pSwitcherFilter = FindSwitcherFilter();

		OpenSetupAudioStream();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		IsMadVRExclusiveMode = false;
		// madVR - register Callback function for detect Entered to ExclusiveMode
		m_pBFmadVR = FindFilter(CLSID_madVR, m_pGB);
		if (m_pBFmadVR) {
			CComQIPtr<IMadVRExclusiveModeCallback> pMVEMC = m_pBFmadVR;
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

		if (m_bUseSmartSeek && m_pMC_preview) {
			m_pMC_preview->Pause();
		}

		break;
	}

	EndWaitCursor();

	m_closingmsg = err;
	WPARAM wp    = (WPARAM)pOMD.Detach();
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

	if (!m_bGraphEventComplete && GetPlaybackMode() == PM_FILE) {
		CAppSettings& s = AfxGetAppSettings();
		if (s.bKeepHistory && s.bRememberFilePos) {
			if (FILE_POSITION* FilePosition = s.CurrentFilePosition()) {
				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				REFERENCE_TIME rtNow;
				m_pMS->GetCurrentPosition(&rtNow);
				FilePosition->llPosition = rtDur > 0 ? rtNow : 0;
				FilePosition->nAudioTrack = GetAudioTrackIdx();
				FilePosition->nSubtitleTrack = GetSubtitleTrackIdx();

				s.SaveCurrentFilePosition();
			}
		}
	}

	m_ExternalSubstreams.clear();

	m_strPlaybackLabel.Empty();
	m_strPlaybackRenderedPath.Empty();

	m_youtubeFields.Empty();
	m_youtubeUrllist.clear();
	AfxGetAppSettings().iYoutubeTagSelected = 0;

	m_OSD.Stop();
	OnPlayStop();

	m_pparray.RemoveAll();
	m_ssarray.RemoveAll();

	Content::Online::Clear();

	if (m_pMC) {
		OAFilterState fs;
		m_pMC->GetState(0, &fs);
		if (fs != State_Stopped) {
			m_pMC->Stop(); // Some filters, such as Microsoft StreamBufferSource, need to call IMediaControl::Stop() before close the graph;
		}
	}

	UnHookDirectXVideoDecoderService();

	// madVR - unregister Callback function
	if (m_pBFmadVR) {
		CComQIPtr<IMadVRExclusiveModeCallback> pMVEMC = m_pBFmadVR;
		if (pMVEMC) {
			pMVEMC->Unregister(MadVRExclusiveModeCallback, this);
		}
		m_pBFmadVR.Release();
	}
	IsMadVRExclusiveMode = false;

	m_fLiveWM = false;
	m_bEndOfStream = false;
	m_bGraphEventComplete = false;
	m_rtDurationOverride = -1;
	m_kfs.clear();
	m_pCB.Release();

	{
		CAutoLock cAutoLock(&m_csSubLock);
		m_pSubStreams.RemoveAll();
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

	pAMXBar.Release();
	pAMDF.Release();
	pAMVCCap.Release();
	pAMVCPrev.Release();
	pAMVSCCap.Release();
	pAMVSCPrev.Release();
	pAMASC.Release();
	pVidCap.Release();
	pAudCap.Release();
	pAMTuner.Release();
	pCGB.Release();

	m_pDVDC.Release();
	m_pDVDI.Release();
	m_pAMOP.Release();
	m_pBI.Release();
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
	m_pKFI.Release();

	if (m_pGB) {
		m_pGB->RemoveFromROT();
		m_pGB.Release();
	}

	if (m_pGB_preview) {
		PreviewWindowHide();
		m_pMFVP_preview.Release();
		m_pMFVDC_preview.Release();

		m_pFS_preview.Release();
		m_pMS_preview.Release();
		m_pBV_preview.Release();
		m_pVW_preview.Release();
		m_pME_preview.Release();
		m_pMC_preview.Release();

		if (m_pDVDC_preview) {
			m_pDVDC_preview.Release();
			m_pDVDI_preview.Release();
		}

		m_pGB_preview->RemoveFromROT();
		m_pGB_preview.Release();
	}

	m_pProv.Release();

	m_fShockwaveGraph = false;

	m_VidDispName.Empty();
	m_AudDispName.Empty();

	m_bMainIsMPEGSplitter = false;

	m_FontInstaller.UninstallFonts();

	DLog(L"CMainFrame::CloseMediaPrivate() : end");
}

#define Check_BD_DVD_Path(path) (::PathFileExistsW(path + L"index.bdmv") || ::PathFileExistsW(path + L"VIDEO_TS.IFO"))

static void RecurseAddDir(const CString& path, std::list<CString>& sl)
{
	WIN32_FIND_DATAW fd = {0};

	HANDLE hFind = FindFirstFileW(path + L"*.*", &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			CString f_name = fd.cFileName;

			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (f_name != L".") && (f_name != L"..")) {
				const CString fullpath = AddSlash(path + f_name);

				sl.push_back(fullpath);
				if (Check_BD_DVD_Path(fullpath)) {
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
	std::list<CString> tmp(sl);
	for (auto fn : tmp) {
		if (::PathIsDirectoryW(fn) && ::PathFileExistsW(fn)) {
			fn = AddSlash(fn);
			if (Check_BD_DVD_Path(fn)) {
				continue;
			}

			RecurseAddDir(AddSlash(fn), sl);
		}
	}
}

int CMainFrame::SearchInDir(const bool& bForward)
{
	std::list<CString> sl;

	CAppSettings& s = AfxGetAppSettings();
	CMediaFormats& mf = s.m_Formats;

	const CString dir  = AddSlash(GetFolderOnly(m_LastOpenFile));
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
			it++;
		}
	} else {
		if (it == sl.cbegin()) {
			if (s.fNextInDirAfterPlaybackLooped) {
				it = std::prev(sl.cend());
			} else {
				return 0;
			}
		} else {
			it--;
		}
	}

	m_wndPlaylistBar.Open(*it);
	OpenCurPlaylistItem();

	return sl.size();
}

void CMainFrame::DoTunerScan(TunerScanData* pTSD)
{
	if (GetPlaybackMode() == PM_CAPTURE) {
		CComQIPtr<IBDATuner>	pTun = m_pGB;
		if (pTun) {
			BOOLEAN				bPresent;
			BOOLEAN				bLocked;
			LONG				lStrength;
			LONG				lQuality;
			int					nProgress;
			int					nOffset = pTSD->Offset ? 3 : 1;
			LONG				lOffsets[3] = {0, pTSD->Offset, -pTSD->Offset};
			bool				bSucceeded;
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
					DiskType = L"DVD Video";
					break;
				case CDROM_BDVideo:
					DiskType = CString(BLU_RAY) + L" Disc";
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
				submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, str);
			}
		}
	}
}

void CMainFrame::SetupFiltersSubMenu()
{
	CMenu& submenu = m_filtersMenu;
	MakeEmptySubMenu(submenu);

	m_pparray.RemoveAll();
	m_ssarray.RemoveAll();

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
				AM_MEDIA_TYPE mt;
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
				AM_MEDIA_TYPE mt;
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

			CComQIPtr<ISpecifyPropertyPages> pSPP = pBF;
			m_pparray.Add(pBF);
			subMenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids, ResStr(IDS_MAINFRM_116));
			nPPages++;

			BeginEnumPins(pBF, pEP, pPin) {
				CString name = GetPinName(pPin);
				name.Replace(L"&", L"&&");

				if (pSPP = pPin) {
					CAUUID caGUID;
					caGUID.pElems = nullptr;
					if (SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0) {
						m_pparray.Add(pPin);
						subMenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids + nPPages, name + ResStr(IDS_MAINFRM_117));

						if (caGUID.pElems) {
							CoTaskMemFree(caGUID.pElems);
						}

						nPPages++;
					}
				}
			}
			EndEnumPins;

			CComQIPtr<IAMStreamSelect> pSS = pBF;
			if (pSS) {
				DWORD nStreams = 0;
				DWORD flags		= (DWORD)-1;
				DWORD group		= (DWORD)-1;
				DWORD prevgroup	= (DWORD)-1;
				WCHAR* wname	= nullptr;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);

				if (nStreams > 0 && nPPages > 0) {
					subMenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
				}

				UINT idlstart = idl;

				for (DWORD i = 0; i < nStreams; i++, pObj = nullptr, pUnk = nullptr) {
					m_ssarray.Add(pSS);

					flags = group = DWORD_MAX;
					wname = nullptr;
					pSS->Info(i, nullptr, &flags, nullptr, &group, &wname, &pObj, &pUnk);

					if (group != prevgroup && idl > idlstart) {
						subMenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
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
						swprintf_s(wname, count, L"%s %d", stream, std::min(i + 1, 999uL));
					}

					CString name(wname);
					name.Replace(L"&", L"&&");

					subMenu.AppendMenu(nflags, idl++, name);

					CoTaskMemFree(wname);
				}

				if (!nStreams) {
					pSS.Release();
				}
			}

			if (nPPages == 1 && !pSS) {
				submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids, name);
			} else {
				submenu.AppendMenu(MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, idf, name);

				if (nPPages > 0 || pSS) {
					MENUITEMINFO mii;
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_STATE | MIIM_SUBMENU;
					mii.fType = MF_POPUP;
					mii.hSubMenu = subMenu.Detach();
					mii.fState = (pSPP || pSS) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
					submenu.SetMenuItemInfo(idf, &mii, TRUE);
				}
			}

			ids += nPPages;
			idf++;
		}
		EndEnumFilters;
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

		if (!strSatellite.IsEmpty() || lr.resourceID == ID_LANGUAGE_ENGLISH) {
			HMODULE lib = LoadLibraryW(strSatellite);
			if (lib != nullptr || lr.resourceID == ID_LANGUAGE_ENGLISH) {
				if (lib) {
					FreeLibrary(lib);
				}
				UINT uFlags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (i == s.iLanguage) {
					uFlags |= MF_CHECKED | MFT_RADIOCHECK;
				}
				submenu.AppendMenu(uFlags, i + ID_LANGUAGE_ENGLISH, lr.name);
				iCount++;
			}
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
		submenu.AppendMenu(MF_SEPARATOR);
	}

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_AUDIO, ResStr(IDS_AG_LOAD_AUDIO));
}

void CMainFrame::SetupAudioRButtonMenu()
{
	CMenu& submenu = m_RButtonMenu;
	MakeEmptySubMenu(submenu);

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_AUDIO_OPTIONS, ResStr(IDS_SUBTITLES_OPTIONS));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_AUDIO, ResStr(IDS_AG_LOAD_AUDIO));
}

void CMainFrame::SetupSubtitlesSubMenu()
{
	SetupSubtitleTracksSubMenu();

	CMenu& submenu = m_SubtitlesMenu;
	if (submenu.GetMenuItemCount() == 0) {
		submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
	}

	submenu.AppendMenu(MF_SEPARATOR);
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_SUBTITLE, ResStr(IDS_AG_LOAD_SUBTITLE));


	CMenu submenu2;
	submenu2.CreatePopupMenu();
	submenu2.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_STYLES, ResStr(IDS_SUBTITLES_STYLES));
	submenu2.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_RELOAD, ResStr(IDS_SUBTITLES_RELOAD));
	submenu2.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_DEFSTYLE, ResStr(IDS_SUBTITLES_DEFAULT_STYLE));
	submenu2.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_FORCEDONLY, ResStr(IDS_SUBTITLES_FORCED));
	submenu.AppendMenu(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu2.Detach(), ResStr(IDS_AG_ADVANCED));

	auto SetFlags = [](int smode) {
		if (AfxGetAppSettings().m_VRSettings.iSubpicStereoMode == smode) {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED | MF_CHECKED | MFT_RADIOCHECK;
		} else {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		}
	};

	CMenu submenu3;
	submenu3.CreatePopupMenu();
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_NONE), ID_SUBTITLES_STEREO_DONTUSE, ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_SIDEBYSIDE), ID_SUBTITLES_STEREO_SIDEBYSIDE, ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_TOPANDBOTTOM), ID_SUBTITLES_STEREO_TOPBOTTOM, ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
	submenu.AppendMenu(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu3.Detach(), ResStr(IDS_SUBTITLES_STEREO));
}

void CMainFrame::SetupSubtitlesRButtonMenu()
{
	CMenu& submenu = m_RButtonMenu;
	MakeEmptySubMenu(submenu);

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_OPTIONS, ResStr(IDS_SUBTITLES_OPTIONS));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FILE_LOAD_SUBTITLE, ResStr(IDS_AG_LOAD_SUBTITLE));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_STYLES, ResStr(IDS_SUBTITLES_STYLES));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_RELOAD, ResStr(IDS_SUBTITLES_RELOAD));
	submenu.AppendMenu(MF_SEPARATOR);

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_DEFSTYLE, ResStr(IDS_SUBTITLES_DEFAULT_STYLE));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SUBTITLES_FORCEDONLY, ResStr(IDS_SUBTITLES_FORCED));
	submenu.AppendMenu(MF_SEPARATOR);

	auto SetFlags = [](int smode) {
		if (AfxGetAppSettings().m_VRSettings.iSubpicStereoMode == smode) {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED | MF_CHECKED | MFT_RADIOCHECK;
		}
		else {
			return MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		}
	};

	CMenu submenu3;
	submenu3.CreatePopupMenu();
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_NONE), ID_SUBTITLES_STEREO_DONTUSE, ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_SIDEBYSIDE), ID_SUBTITLES_STEREO_SIDEBYSIDE, ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
	submenu3.AppendMenu(SetFlags(SUBPIC_STEREO_TOPANDBOTTOM), ID_SUBTITLES_STEREO_TOPBOTTOM, ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
	submenu.AppendMenu(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)submenu3.Detach(), ResStr(IDS_SUBTITLES_STEREO));
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

		submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | (bIsDisabled ? MF_ENABLED : MF_CHECKED), ID_SUBTITLES_ENABLE, ResStr(IDS_AG_ENABLED));
		submenu.AppendMenu(MF_BYCOMMAND | MF_SEPARATOR | MF_ENABLED);

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
				str.ReleaseBufferSetLength(max(len - 1, 0));
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
			submenu.AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupSubtilesAMStreamSubMenu(CMenu& submenu, UINT id)
{
	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		if (m_pDVS) {
			CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

				bool fHideSubtitles = false;
				m_pDVS->get_HideSubtitles(&fHideSubtitles);
				submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED), ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
				submenu.AppendMenu(MF_SEPARATOR);

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
							submenu.AppendMenu(flags, id++, streamName);
						}

						submenu.AppendMenu(MF_SEPARATOR);
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
							submenu.AppendMenu(flags, id++, name);
						}
					}

					return;
				}

				int iSelectedLanguage = 0;
				m_pDVS->get_SelectedLanguage(&iSelectedLanguage);

				for (int i = 0; i < nLangs; i++) {
					WCHAR *pName = nullptr;
					m_pDVS->get_LanguageName(i, &pName);
					CString streamName(pName);
					CoTaskMemFree(pName);

					UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
					if (i == iSelectedLanguage) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}

					streamName.Replace(L"&", L"&&");
					submenu.AppendMenu(flags, id++, streamName);
				}
			}

			return;
		}

		POSITION pos = m_pSubStreams.GetHeadPosition();

		submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | (pos ? MF_ENABLED : MF_DISABLED), ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
		submenu.AppendMenu(MF_SEPARATOR);

		bool sep = false;
		int splcnt = 0;
		UINT baseid = id;
		int intsub = 0;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS && !AfxGetAppSettings().fDisableInternalSubtitles) {
			DWORD cStreams;
			if (!FAILED(pSS->Count(&cStreams))) {

				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS;
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
						submenu.AppendMenu(flags, id++, name);
						splcnt++;
					}
				}
			}
		}

		pos = m_pSubStreams.GetHeadPosition();
		CComPtr<ISubStream> pSubStream;
		int tPos = -1;
		if (splcnt > 0 && pos) {
			pSubStream = m_pSubStreams.GetNext(pos);
			tPos++;
		}

		while (pos) {
			pSubStream = m_pSubStreams.GetNext(pos);
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

					if ((splcnt > 0 || (cntintsub == intsub && cntintsub != 0)) && !sep) {
						submenu.AppendMenu(MF_SEPARATOR);
						sep = true;
					}

					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					if (cntintsub <= intsub) {
						name = L"* " + name;
					}
					submenu.AppendMenu(flags, id++, name);
					intsub++;
					CoTaskMemFree(pName);
				} else {
					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					CString sname;
					if (cntintsub <= intsub) {
						sname = L"* " + ResStr(IDS_AG_UNKNOWN);
					}
					submenu.AppendMenu(flags, id++, sname);
					intsub++;
				}
			}
		}

		if ((submenu.GetMenuItemCount() < 2) || (!m_pSubStreams.GetHeadPosition() && submenu.GetMenuItemCount() <= 3)) {
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

			submenu.AppendMenu(flags, id++, str);
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
			DWORD idx = 1;
			for (const auto& Item : m_BDPlaylists) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (idx == MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					idx = 0;
				}
				idx++;

				if (Item.m_strFileName == m_strPlaybackRenderedPath) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				const CString time = L"[" + ReftimeToString2(Item.Duration()) + L"]";
				submenu.AppendMenu(flags, id++, GetFileOnly(Item.m_strFileName) + '\t' + time);
			}
		} else if (m_youtubeUrllist.size() > 1) {
			DWORD idx = 1;
			for(auto item = m_youtubeUrllist.begin(); item != m_youtubeUrllist.end(); ++item) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (idx == MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					idx = 0;
				}
				idx++;

				if (item->url == m_strPlaybackRenderedPath) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				submenu.AppendMenu(flags, id++, item->title);
			}
		}

		REFERENCE_TIME rt = GetPos();
		DWORD j = m_pCB->ChapLookup(&rt, nullptr);

		if (m_pCB->ChapGetCount() > 1) {
			for (DWORD i = 0, idx = 0; i < m_pCB->ChapGetCount(); i++, id++, idx++) {
				rt = 0;
				CComBSTR bstr;
				if (FAILED(m_pCB->ChapGet(i, &rt, &bstr))) {
					continue;
				}

				CString name(bstr);
				name.Replace(L"&", L"&&");
				name.Replace('\t', ' ');

				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (i == j) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				if (idx == MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					idx = 0;
				}

				if (id != ID_NAVIGATE_CHAP_SUBITEM_START && i == 0) {
					//pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);
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
				submenu.AppendMenu(flags, id, name);
			}
		}

		if (m_wndPlaylistBar.GetCount() > 1) {
			POSITION pos = m_wndPlaylistBar.m_pl.GetHeadPosition();
			while (pos) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (pos == m_wndPlaylistBar.m_pl.GetPos()) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}
				if (id != ID_NAVIGATE_CHAP_SUBITEM_START && pos == m_wndPlaylistBar.m_pl.GetHeadPosition()) {
					submenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
				}
				CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);
				CString name = pli.GetLabel();
				name.Replace(L"&", L"&&");
				submenu.AppendMenu(flags, id++, name);
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

			submenu.AppendMenu(flags, id++, str);
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

			submenu.AppendMenu(flags, id++, str);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1) {
		CAppSettings& s = AfxGetAppSettings();

		for (auto& Channel : s.m_DVBChannels) {
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

			if ((UINT)Channel.GetPrefNumber() == s.nDVBLastChannel) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			submenu.AppendMenu(flags, ID_NAVIGATE_CHAP_SUBITEM_START + Channel.GetPrefNumber(), Channel.GetName());
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

	CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
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
				submenu.AppendMenu(MF_SEPARATOR);
			}
			dwPrevGroup = dwGroup;

			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
			if (dwFlags) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}

			name.Replace(L"&", L"&&");
			submenu.AppendMenu(flags, id++, name);
		}
	}
}

void CMainFrame::SelectAMStream(UINT id, DWORD dwSelGroup)
{
	CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
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
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
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
					auto it = pli.m_fns.begin();
					++it; // skip main file
					for (; it != pli.m_fns.end(); ++it) {
						CString str = (*it).GetName();
						if (!str.IsEmpty() && name == GetFileOnly(str)) {
							fExternal = true;
							break;
						}
					}
				}

				if (!sep && cStreams > 1 && fExternal) {
					if (submenu.GetMenuItemCount()) {
						submenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
					}
					sep = true;
				}

				name.Replace(L"&", L"&&");
				submenu.AppendMenu(flags, id++, name);
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
				str.ReleaseBufferSetLength(max(len-1, 0));
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

			submenu.AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupRecentFilesSubMenu()
{
	CMenu& submenu = m_recentfilesMenu;
	MakeEmptySubMenu(submenu);

	if (m_eMediaLoadState == MLS_LOADING) {
		return;
	}

	UINT id = ID_RECENT_FILE_START;
	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	MRU.ReadList();

	int mru_count = 0;
	for (int i = 0; i < MRU.GetSize(); i++) {
		if (!MRU[i].IsEmpty()) {
			mru_count++;
			break;
		}
	}
	if (mru_count) {
		submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_RECENT_FILES_CLEAR, ResStr(IDS_RECENT_FILES_CLEAR));
		submenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
	}

	for (int i = 0; i < MRU.GetSize(); i++) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		if (!MRU[i].IsEmpty()) {
			CString path(MRU[i]);
			if (path.Find(L"\\") != 0) {
				if (CheckBD(path)) {
					path.Format(L"%s - %s", BLU_RAY, path);
				} else if (CheckDVD(path)) {
					path = L"DVD - " + path;
				}
			}
			submenu.AppendMenu(flags, id, path);
		}
		id++;
	}
}

void CMainFrame::SetupFavoritesSubMenu()
{
	CMenu& submenu = m_favoritesMenu;
	MakeEmptySubMenu(submenu);

	CAppSettings& s = AfxGetAppSettings();

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ADD, ResStr(IDS_FAVORITES_ADD));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ORGANIZE, ResStr(IDS_FAVORITES_ORGANIZE));

	UINT nLastGroupStart = submenu.GetMenuItemCount();

	UINT id = ID_FAVORITES_FILE_START;

	std::list<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	for (const auto& item : sl) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString f_str = item;
		f_str.Replace(L"&", L"&&");
		f_str.Replace(L"\t", L" ");

		CAtlList<CString> sl;
		ExplodeEsc(f_str, sl, L';', 3);

		f_str = sl.RemoveHead();

		CString str;

		if (!sl.IsEmpty()) {
			// pos
			REFERENCE_TIME rt = 0;
			if (1 == swscanf_s(sl.GetHead(), L"%I64d", &rt) && rt > 0) {
				DVD_HMSF_TIMECODE hmsf = RT2HMSF(rt);
				str.Format(L"[%02u:%02u:%02u]", hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
			}

			// relative drive
			if (sl.GetCount() > 1) { // Here to prevent crash if old favorites settings are present
				sl.RemoveHeadNoReturn();

				BOOL bRelativeDrive = FALSE;
				if (swscanf_s(sl.GetHead(), L"%d", &bRelativeDrive) == 1) {
					if (bRelativeDrive) {
						str.Format(L"[RD]%s", str);
					}
				}
			}
			if (!str.IsEmpty()) {
				f_str.Format(L"%s\t%.14s", f_str, str);
			}
		}

		if (!f_str.IsEmpty()) {
			submenu.AppendMenu(flags, id, f_str);
		}

		id++;
	}

	if (id > ID_FAVORITES_FILE_START) {
		submenu.InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = submenu.GetMenuItemCount();

	id = ID_FAVORITES_DVD_START;

	s.GetFav(FAV_DVD, sl);

	for (const auto& item : sl) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString str = item;
		str.Replace(L"&", L"&&");

		CAtlList<CString> sl;
		ExplodeEsc(str, sl, L';', 4);
		size_t cnt = sl.GetCount();

		str = sl.RemoveHead();

		if (!str.IsEmpty()) {
			if (cnt == 4) {
				// pos
				CString posStr;
				REFERENCE_TIME rt = 0;
				if (1 == swscanf_s(sl.GetHead(), L"%I64d", &rt) && rt > 0) {
					DVD_HMSF_TIMECODE hmsf = RT2HMSF(rt);
					posStr.Format(L"[%02u:%02u:%02u]", hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
				}

				if (!posStr.IsEmpty()) {
					str.AppendFormat(L"\t%.14s", posStr);
				}
			}

			submenu.AppendMenu(flags, id, str);
		}

		id++;
	}

	if (id > ID_FAVORITES_DVD_START) {
		submenu.InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = submenu.GetMenuItemCount();

	id = ID_FAVORITES_DEVICE_START;

	s.GetFav(FAV_DEVICE, sl);

	for (const auto& item : sl) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString str = item;
		str.Replace(L"&", L"&&");

		CAtlList<CString> sl;
		ExplodeEsc(str, sl, L';', 2);

		str = sl.RemoveHead();

		if (!str.IsEmpty()) {
			submenu.AppendMenu(flags, id, str);
		}

		id++;
	}
}

void CMainFrame::SetupShadersSubMenu()
{
	CMenu& submenu = m_shadersMenu;
	MakeEmptySubMenu(submenu);

	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_TOGGLE, ResStr(IDS_SHADERS_TOGGLE));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_TOGGLE_SCREENSPACE, ResStr(IDS_SHADERS_TOGGLE_SCREENSPACE));
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_SELECT, ResStr(IDS_SHADERS_SELECT));
	submenu.AppendMenu(MF_SEPARATOR);
	submenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_VIEW_SHADEREDITOR, ResStr(IDS_SHADERS_EDIT));
}

/////////////

void CMainFrame::ShowControls(int nCS, bool fSave)
{
	int nCSprev = AfxGetAppSettings().nCS;
	int hbefore = 0, hafter = 0;

	m_pLastBar = nullptr;

	int i = 1;
	for (const auto& pBar : m_bars) {
		ShowControlBar(pBar, !!(nCS&i), TRUE);
		if (nCS&i) {
			m_pLastBar = pBar;
		}

		CSize s = pBar->CalcFixedLayout(FALSE, TRUE);
		if (nCSprev&i) {
			hbefore += s.cy;
		}
		if (nCS&i) {
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
		AfxGetAppSettings().nCS = nCS;
	}

	RecalcLayout();
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
				if (pDockingBar->IsHorzDocked()) {
					cSize.cy += pDockingBar->CalcFixedLayout(TRUE, TRUE).cy - GetSystemMetrics(SM_CYBORDER);
				} else if (pDockingBar->IsVertDocked()) {
					cSize.cx += pDockingBar->CalcFixedLayout(TRUE, FALSE).cx;
				}
			}
		}
	}
}

void CMainFrame::SetAlwaysOnTop(int i)
{
	AfxGetAppSettings().iOnTop = i;

	if (!m_bFullScreen && !IsD3DFullScreenMode()) {
		const CWnd* pInsertAfter = nullptr;

		if (i == 0) {
			pInsertAfter = &wndNoTopMost;
		} else if (i == 1) {
			pInsertAfter = &wndTopMost;
		} else if (i == 2) {
			pInsertAfter = GetMediaState() == State_Running ? &wndTopMost : &wndNoTopMost;
		} else { // if (i == 3)
			pInsertAfter = (GetMediaState() == State_Running && !m_bAudioOnly) ? &wndTopMost : &wndNoTopMost;
		}

		if (pInsertAfter) {
			SetWindowPos(pInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	} else if (m_bFullScreen && !(GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

ISubStream *InsertSubStream(CInterfaceList<ISubStream> *subStreams, const CComPtr<ISubStream> &theSubStream)
{
	return subStreams->GetAt(subStreams->AddTail(theSubStream));
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
			AM_MEDIA_TYPE mt;
			if (FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo
					|| FAILED(pPin->ConnectionMediaType(&mt))
					|| (!CMediaTypeEx(mt).ValidateSubtitle())) {
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
			name.Format(L"TextPassThru%08x", pTPTF);
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

bool CMainFrame::LoadSubtitle(CSubtitleItem subItem, ISubStream **actualStream)
{
	CAppSettings& s = AfxGetAppSettings();

	if (!s.IsISRAutoLoadEnabled()) {
		return false;
	}

	CComPtr<ISubStream> pSubStream;
	CString fname = subItem;

	if (GetFileExt(fname).MakeLower() == L".mks" && s.IsISRAutoLoadEnabled()) {
		if (CComQIPtr<IGraphBuilderSub> pGBS = m_pGB) {
			HRESULT hr = pGBS->RenderSubFile(fname);
			if (SUCCEEDED(hr)) {
				size_t c = m_pSubStreams.GetCount();
				AddTextPassThruFilter();

				if (m_pSubStreams.GetCount() > c) {
					ISubStream *r = m_pSubStreams.GetTail();
					if (actualStream != nullptr) {
						*actualStream = r;

						s.fEnableSubtitles = true;
					}

					return true;
				}

				return false;
			}
		}
	}

	// TMP: maybe this will catch something for those who get a runtime error dialog when opening subtitles from cds
	try {

		CString videoName = GetPlaybackMode() == PM_FILE ? GetCurFileName() : L"";
		if (!pSubStream) {
			CAutoPtr<CSupSubFile> pSSF(DNew CSupSubFile(&m_csSubLock));
			if (pSSF && GetFileExt(fname).MakeLower() == L".sup") {
				if (pSSF->Open(fname, subItem.GetTitle(), videoName)) {
					pSubStream = pSSF.Detach();
				}
			}
		}

		if (!pSubStream) {
			CAutoPtr<CVobSubFile> pVSF(DNew CVobSubFile(&m_csSubLock));
			if (GetFileExt(fname).MakeLower() == L".idx" && pVSF && pVSF->Open(fname) && pVSF->GetStreamCount() > 0) {
				pSubStream = pVSF.Detach();
			}
		}

		if (!pSubStream) {
			CAutoPtr<CRenderedTextSubtitle> pRTS(DNew CRenderedTextSubtitle(&m_csSubLock));
			if (pRTS && pRTS->Open(fname, DEFAULT_CHARSET, subItem.GetTitle(), videoName) && pRTS->GetStreamCount() > 0) {
				pSubStream = pRTS.Detach();
			}
		}
	} catch (CException* e) {
		e->Delete();
	}

	if (pSubStream) {
		ISubStream *r = InsertSubStream(&m_pSubStreams, pSubStream);
		m_ExternalSubstreams.push_back(r);
		if (actualStream != nullptr) {
			*actualStream = r;

			s.fEnableSubtitles = true;
		}

		if (subChangeNotifyThread.joinable() && !::PathIsURLW(fname)) {
			BOOL bExists = FALSE;
			for (INT_PTR idx = 0; idx < m_ExtSubFiles.GetCount(); idx++) {
				if (fname == m_ExtSubFiles[idx]) {
					bExists = TRUE;
					break;
				}
			}

			if (!bExists) {
				m_ExtSubFiles.Add(fname);
				CFileStatus status;
				if (CFileGetStatus(fname, status)) {
					m_ExtSubFilesTime.Add(status.m_mtime);
				} else {
					m_ExtSubFilesTime.Add(CTime(0));
				}
			}

			fname.Replace('\\', '/');
			fname = fname.Left(fname.ReverseFind('/')+1);

			bExists = FALSE;
			for (INT_PTR idx = 0; idx < m_ExtSubPaths.GetCount(); idx++) {
				if (fname == m_ExtSubPaths[idx]) {
					bExists = TRUE;
					break;
				}
			}

			if (!bExists) {
				HANDLE h = FindFirstChangeNotificationW(fname, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
				if (h != INVALID_HANDLE_VALUE) {
					m_ExtSubPaths.Add(fname);
					m_ExtSubPathsHandles.Add(h);
					m_EventSubChangeRefreshNotify.Set();
				}
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

	POSITION pos = m_pSubStreams.GetHeadPosition();
	while (pos && i >= 0) {
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

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

	s.m_VRSettings.bSubpicPosRelative	= 0;
	s.m_VRSettings.SubpicShiftPos = {0, 0};

	{
		CAutoLock cAutoLock(&m_csSubLock);

		if (pSubStream) {
			CLSID clsid;
			pSubStream->GetClassID(&clsid);

			if (clsid == __uuidof(CVobSubFile) || clsid == __uuidof(CVobSubStream)) {
				if (auto pVSS = dynamic_cast<CVobSubSettings*>((ISubStream*)pSubStream)) {
					pVSS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos);
				}
			} else if (clsid == __uuidof(CRenderedTextSubtitle)) {
				CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

				STSStyle style = s.subdefstyle;

				if (fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle) {
					pRTS->SetDefaultStyle(style);
				}

				if (pRTS->GetDefaultStyle(style) && style.relativeTo == 2) {
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
			} else if (clsid == __uuidof(CRenderedHdmvSubtitle) || clsid == __uuidof(CSupSubFile)) {
				s.m_VRSettings.bSubpicPosRelative = s.subdefstyle.relativeTo;
			}

			CComQIPtr<ISubRenderOptions> pSRO = m_pCAP;

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

				POSITION pos = m_pSubStreams.GetHeadPosition();
				while (pos) {
					CComPtr<ISubStream> pSubStream2 = m_pSubStreams.GetNext(pos);

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
			POSITION pos = m_pSubStreams.GetHeadPosition();
			while (pos && i >= 0) {
				CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

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
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while (pos) {
		POSITION cur = pos;
		if (pSubStreamOld == m_pSubStreams.GetNext(pos)) {
			m_pSubStreams.SetAt(cur, pSubStreamNew);
			UpdateSubtitle();
			break;
		}
	}
}

void CMainFrame::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
	if (m_pCAP) {
		if (nSubtitleId == -1 || nSubtitleId == (DWORD_PTR)(ISubStream*)m_pCurrentSubStream) {
			m_pCAP->Invalidate(rtInvalidate);
		}
	}
}

void CMainFrame::ReloadSubtitle()
{
	{
		CAutoLock cAutoLock(&m_csSubLock);

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while (pos) {
			m_pSubStreams.GetNext(pos)->Reload();
		}
	}

	UpdateSubtitle();

	if (AfxGetAppSettings().fUseSybresync) {
		m_wndSubresyncBar.ReloadSubtitle();
	}
}

void CMainFrame::UpdateSubDefaultStyle()
{
	CAppSettings& s = AfxGetAppSettings();
	if (auto pRTS = dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
		{
			CAutoLock cAutoLock(&m_csSubLock);

			pRTS->SetOverride(s.fUseDefaultSubtitlesStyle, s.subdefstyle);
			pRTS->Deinit();
		}
		InvalidateSubtitle();
		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	} else if (dynamic_cast<CRenderedHdmvSubtitle*>((ISubStream*)m_pCurrentSubStream)
			|| dynamic_cast<CSupSubFile*>((ISubStream*)m_pCurrentSubStream)) {
		s.m_VRSettings.bSubpicPosRelative = s.subdefstyle.relativeTo;
		InvalidateSubtitle();
		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	}
}

void CMainFrame::SetAudioTrackIdx(int index)
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			pSS->Enable(index, AMSTREAMSELECTENABLE_ENABLE);
		}
	}
}

void CMainFrame::SetSubtitleTrackIdx(int index)
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE) {
		if (index != -1) {
			AfxGetAppSettings().fEnableSubtitles = true;
		}
		SelectSubtilesAMStream(index);
	}
}

int CMainFrame::GetAudioTrackIdx()
{
	if (/*m_eMediaLoadState == MLS_LOADED && */GetPlaybackMode() == PM_FILE) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			for (int i = 0; i < (int)cStreams; i++) {
				DWORD dwFlags = 0;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, nullptr, nullptr, nullptr))) {
					return -1;
				}
				if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
					return i;
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
		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS && !AfxGetAppSettings().fDisableInternalSubtitles) {
			DWORD cStreams = 0;
			if (SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0) {
				CComQIPtr<IBaseFilter> pBF = pSS;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
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
	__int64 stop;
	m_wndSeekBar.GetRange(stop);
	return (m_eMediaLoadState == MLS_LOADED ? stop : 0);
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
	REFERENCE_TIME rtDuration;
	m_wndSeekBar.GetRange(rtDuration);
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
	OAFilterState fs = GetMediaState();

	if (rtPos < 0) {
		rtPos = 0;
	}

	m_nStepForwardCount = 0;
	__int64 stop	= 0;
	if (GetPlaybackMode() != PM_CAPTURE) {
		m_wndSeekBar.GetRange(stop);
		if (rtPos > stop) {
			rtPos = stop;
		}
		m_wndStatusBar.SetStatusTimer(rtPos, stop, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
		if (bShowOSD && stop > 0 && (AfxGetAppSettings().iShowOSD & OSD_SEEKTIME)) {
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

	OnTimer(TIMER_STREAMPOSPOLLER);
	OnTimer(TIMER_STREAMPOSPOLLER2);

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

		if (m_fBuffering) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF) {
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
		if (CComQIPtr<IMatroskaSplitterFilter> pMatroskaSplitterFilter = m_pKFI) {
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

		if (m_fBuffering) {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF) {
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
		if (pBF == pVidCap || pBF == pAudCap) {
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
		if (CComQIPtr<IAMBufferNegotiation> pAMBN = pPin) {
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
				&& SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, TRUE, 0, &pPin))) {
			CComPtr<IBaseFilter> pDVSplitter;
			hr = pDVSplitter.CoCreateInstance(CLSID_DVSplitter);
			hr = m_pGB->AddFilter(pDVSplitter, L"DV Splitter");

			hr = pCGB->RenderStream(nullptr, &MEDIATYPE_Interleaved, pPin, nullptr, pDVSplitter);

			pPin = nullptr;
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Video, TRUE, 0, &pPin);
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Audio, TRUE, 0, &pDVAudPin);

			CComPtr<IBaseFilter> pDVDec;
			hr = pDVDec.CoCreateInstance(CLSID_DVVideoCodec);
			hr = m_pGB->AddFilter(pDVDec, L"DV Video Decoder");

			hr = m_pGB->ConnectFilter(pPin, pDVDec);

			pPin = nullptr;
			hr = pCGB->FindPin(pDVDec, PINDIR_OUTPUT, nullptr, &MEDIATYPE_Video, TRUE, 0, &pPin);
		} else if (FAILED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, TRUE, 0, &pPin))) {
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
		} else if (FAILED(pCGB->FindPin(pAudCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, TRUE, 0, &pPin))) {
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
	if (!pCGB) {
		return false;
	}

	SaveMediaState;

	HRESULT hr;

	m_pGB->NukeDownstream(pVidCap);
	m_pGB->NukeDownstream(pAudCap);

	CleanGraph();

	if (pAMVSCCap) {
		hr = pAMVSCCap->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	}
	if (pAMVSCPrev) {
		hr = pAMVSCPrev->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	}
	if (pAMASC) {
		hr = pAMASC->SetFormat(&m_wndCaptureBar.m_capdlg.m_mta);
	}

	CComPtr<IBaseFilter> pVidBuffer = m_wndCaptureBar.m_capdlg.m_pVidBuffer;
	CComPtr<IBaseFilter> pAudBuffer = m_wndCaptureBar.m_capdlg.m_pAudBuffer;
	CComPtr<IBaseFilter> pVidEnc = m_wndCaptureBar.m_capdlg.m_pVidEnc;
	CComPtr<IBaseFilter> pAudEnc = m_wndCaptureBar.m_capdlg.m_pAudEnc;
	CComPtr<IBaseFilter> pMux = m_wndCaptureBar.m_capdlg.m_pMux;
	CComPtr<IBaseFilter> pDst = m_wndCaptureBar.m_capdlg.m_pDst;
	CComPtr<IBaseFilter> pAudMux = m_wndCaptureBar.m_capdlg.m_pAudMux;
	CComPtr<IBaseFilter> pAudDst = m_wndCaptureBar.m_capdlg.m_pAudDst;

	bool fFileOutput = (pMux && pDst) || (pAudMux && pAudDst);
	bool fCapture = (fVCapture || fACapture);

	if (pAudCap) {
		AM_MEDIA_TYPE* pmt = &m_wndCaptureBar.m_capdlg.m_mta;
		int ms = (fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput) ? AUDIOBUFFERLEN : 60;
		if (pMux != pAudMux && fACapture) {
			SetLatency(pAudCap, -1);
		} else if (pmt->pbFormat) {
			SetLatency(pAudCap, ((WAVEFORMATEX*)pmt->pbFormat)->nAvgBytesPerSec * ms / 1000);
		}
	}

	CComPtr<IPin> pVidCapPin, pVidPrevPin, pAudCapPin, pAudPrevPin;
	BuildToCapturePreviewPin(pVidCap, &pVidCapPin, &pVidPrevPin, pAudCap, &pAudCapPin, &pAudPrevPin);

	//if (pVidCap)
	{
		bool fVidPrev = pVidPrevPin && fVPreview;
		bool fVidCap = pVidCapPin && fVCapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fVidOutput;

		if (fVPreview == 2 && !fVidCap && pVidCapPin) {
			pVidPrevPin = pVidCapPin;
			pVidCapPin = nullptr;
		}

		if (fVidPrev) {
			m_pCAP = nullptr;
			m_pGB->Render(pVidPrevPin);
			m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
		}

		if (fVidCap) {
			IBaseFilter* pBF[3] = {pVidBuffer, pVidEnc, pMux};
			HRESULT hr = BuildCapture(pVidCapPin, pBF, MEDIATYPE_Video, &m_wndCaptureBar.m_capdlg.m_mtcv);
			UNREFERENCED_PARAMETER(hr);
		}

		pAMDF = nullptr;
		pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMDroppedFrames, (void**)&pAMDF);
	}

	//if (pAudCap)
	{
		bool fAudPrev = pAudPrevPin && fAPreview;
		bool fAudCap = pAudCapPin && fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput;

		if (fAPreview == 2 && !fAudCap && pAudCapPin) {
			pAudPrevPin = pAudCapPin;
			pAudCapPin = nullptr;
		}

		if (fAudPrev) {
			m_pGB->Render(pAudPrevPin);
		}

		if (fAudCap) {
			IBaseFilter* pBF[3] = {pAudBuffer, pAudEnc, pAudMux ? pAudMux : pMux};
			HRESULT hr = BuildCapture(pAudCapPin, pBF, MEDIATYPE_Audio, &m_wndCaptureBar.m_capdlg.m_mtca);
			UNREFERENCED_PARAMETER(hr);
		}
	}

	if ((pVidCap || pAudCap) && fCapture && fFileOutput) {
		if (pMux != pDst) {
			hr = m_pGB->AddFilter(pDst, L"File Writer V/A");
			hr = m_pGB->ConnectFilter(GetFirstPin(pMux, PINDIR_OUTPUT), pDst);
		}

		if (CComQIPtr<IConfigAviMux> pCAM = pMux) {
			int nIn, nOut, nInC, nOutC;
			CountPins(pMux, nIn, nOut, nInC, nOutC);
			pCAM->SetMasterStream(nInC-1);
			//pCAM->SetMasterStream(-1);
			pCAM->SetOutputCompatibilityIndex(FALSE);
		}

		if (CComQIPtr<IConfigInterleaving> pCI = pMux) {
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
	hr = pCGB->ControlStream(&PIN_CATEGORY_CAPTURE, nullptr, nullptr, nullptr, &stop, 0, 0); // stop in the infinite

	CleanGraph();

	OpenSetupVideo();
	OpenSetupAudio();
	OpenSetupStatsBar();
	OpenSetupStatusBar();

	RestoreMediaState;

	return true;
}

bool CMainFrame::StartCapture()
{
	if (!pCGB || m_fCapturing) {
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

	m_fCapturing = true;

	return true;
}

bool CMainFrame::StopCapture()
{
	if (!pCGB || !m_fCapturing) {
		return false;
	}

	if (!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) {
		return false;
	}

	HRESULT hr;

	m_wndStatusBar.SetStatusMessage(ResStr(IDS_CONTROLS_COMPLETING));

	m_fCapturing = false;

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
	CAppSettings& s = AfxGetAppSettings();
	s.iSelectedVideoRenderer = -1;

	CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), GetModalParent(), idPage);

	m_bInOptions = true;
	INT_PTR dResult = options.DoModal();
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

	Invalidate();
	m_wndStatusBar.Relayout();
	m_wndPlaylistBar.Invalidate();

	m_bInOptions = false;
}

void CMainFrame::StartWebServer(int nPort)
{
	if (!m_pWebServer) {
		m_pWebServer.Attach(DNew CWebServer(this, nPort));
	}
}

void CMainFrame::StopWebServer()
{
	if (m_pWebServer) {
		m_pWebServer.Free();
	}
}

CString CMainFrame::GetStatusMessage()
{
	CString str;
	m_wndStatusBar.m_status.GetWindowTextW(str);
	return str;
}

void CMainFrame::SendStatusMessage(CString msg, int nTimeOut)
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

	if (pli.m_fns.size()
			&& (OpenIso(pli.m_fns.front(), rtStart) || OpenBD(pli.m_fns.front(), rtStart, bAddRecent))) {
		return TRUE;
	}

	CAutoPtr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	if (p) {
		p->bAddRecent = bAddRecent;
		OpenMedia(p);
	}

	return TRUE;
}

BOOL CMainFrame::OpenFile(const CString fname, REFERENCE_TIME rtStart/* = INVALID_TIME*/, BOOL bAddRecent/* = TRUE*/)
{
	CAutoPtr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	if (p) {
		auto pFileData = dynamic_cast<OpenFileData*>(p.m_p);
		if (pFileData->fns.empty()) {
			pFileData->fns.push_front(fname);
		} else {
			pFileData->fns.front() = fname;
		}
		p->bAddRecent = bAddRecent;
		OpenMedia(p);
		return TRUE;
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

void CMainFrame::OpenMedia(CAutoPtr<OpenMediaData> pOMD)
{
	auto pFileData		= dynamic_cast<const OpenFileData*>(pOMD.m_p);
	auto pDeviceData	= dynamic_cast<const OpenDeviceData*>(pOMD.m_p);

	// shortcut
	if (pDeviceData
			&& m_eMediaLoadState == MLS_LOADED && pAMTuner
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

	bool bDirectShow	= pFileData && !pFileData->fns.empty() && s.GetFileEngine(pFileData->fns.front()) == DirectShow;
	bool bUseThread		= m_pGraphThread && s.fEnableWorkerThreadForOpening && (bDirectShow || !pFileData) && !pDeviceData;

	// create d3dfs window if launching in fullscreen and d3dfs is enabled
	if (s.IsD3DFullscreen() && (m_bStartInD3DFullscreen || s.fLaunchfullscreen)) {
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
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_OPEN, 0, (LPARAM)pOMD.Detach());
		m_bOpenedThruThread = true;
	} else {
		OpenMediaPrivate(pOMD);
		m_bOpenedThruThread = false;
	}
}

bool CMainFrame::ResetDevice()
{
	if (m_pCAP) {
		return m_pCAP->ResetDevice();
	}
	return true;
}

bool CMainFrame::DisplayChange()
{
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

	m_bNextIsOpened = bNextIsOpened;

	if (m_eMediaLoadState == MLS_LOADING) {
		m_fOpeningAborted = true;

		if (m_pGB) {
			m_pGB->Abort();    // TODO: lock on graph objects somehow, this is not thread safe
		}

		if (m_pGraphThread) {
			BeginWaitCursor();
			if (WaitForSingleObject(m_hGraphThreadEventOpen, 5000) == WAIT_TIMEOUT) {
				MessageBeep(MB_ICONEXCLAMATION);
				DLog(L"CRITICAL ERROR: !!! Must kill opener thread !!!");
				TerminateThread(m_pGraphThread->m_hThread, 0xDEAD);

				m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));
				if (m_pGraphThread) {
					m_pGraphThread->SetMainFrame(this);
				}

				m_bOpenedThruThread = false;
			}
			EndWaitCursor();

			MSG msg;
			if (PeekMessage(&msg, m_hWnd, WM_POSTOPEN, WM_POSTOPEN, PM_REMOVE | PM_NOYIELD)) {
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
				PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
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

void CMainFrame::StartTunerScan(CAutoPtr<TunerScanData> pTSD)
{
	if (m_pGraphThread) {
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_TUNER_SCAN, 0, (LPARAM)pTSD.Detach());
	} else {
		DoTunerScan(pTSD);
	}
}

void CMainFrame::StopTunerScan()
{
	m_bStopTunerScan = true;
}

void CMainFrame::DisplayCurrentChannelOSD()
{
	CAppSettings&	s		 = AfxGetAppSettings();
	CDVBChannel*	pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	CString			osd;
	int				i		 = osd.Find(L"\n");
	PresentFollowing NowNext;

	if (pChannel != nullptr) {
		// Get EIT information:
		CComQIPtr<IBDATuner> pTun = m_pGB;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}
		NowNext.cPresent.Insert(0,L" ");
		osd = pChannel->GetName()+ NowNext.cPresent;
		if (i > 0) {
			osd.Delete(i, osd.GetLength()-i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd ,3500);
	}
}

void CMainFrame::DisplayCurrentChannelInfo()
{
	CAppSettings&	 s		 = AfxGetAppSettings();
	CDVBChannel*	 pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	CString			 osd;
	PresentFollowing NowNext;

	if (pChannel != nullptr) {
		// Get EIT information:
		CComQIPtr<IBDATuner> pTun = m_pGB;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}

		osd = NowNext.cPresent + L". " + NowNext.StartTime + L" - " + NowNext.Duration + L". " + NowNext.SummaryDesc +L" ";
		int	 i = osd.Find(L"\n");
		if (i > 0) {
			osd.Delete(i, osd.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd ,8000, 12);
	}
}

void CMainFrame::SetLoadState(MPC_LOADSTATE iState)
{
	m_eMediaLoadState = iState;
	SendAPICommand(CMD_STATE, L"%d", m_eMediaLoadState);

	switch (m_eMediaLoadState) {
		case MLS_CLOSED:
			DLog(L"CMainFrame::SetLoadState() : CLOSED");
			break;
		case MLS_LOADING:
			DLog(L"CMainFrame::SetLoadState() : LOADING");
			break;
		case MLS_LOADED:
			DLog(L"CMainFrame::SetLoadState() : LOADED");
			break;
		case MLS_CLOSING:
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
		SetThreadExecutionState(iState == PS_PLAY ? ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
	} else {
		SetThreadExecutionState(iState == PS_PLAY ? ES_CONTINUOUS | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
	}

	UpdateThumbarButton();
}

bool CMainFrame::CreateFullScreenWindow()
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

	return !!m_pFullscreenWnd->CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"", ResStr(IDS_MAINFRM_136),
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

void CMainFrame::DestroyD3DWindow()
{
	if (IsD3DFullScreenMode()) {
		m_pFullscreenWnd->ShowWindow(SW_HIDE);
		m_pFullscreenWnd->DestroyWindow();

		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		m_bHideCursor = false;

		SetFocus();
	}
}

void CMainFrame::SetupEVRColorControl()
{
	if (m_pMFVP) {
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Brightness, m_ColorCintrol.GetEVRColorControl(ProcAmp_Brightness)))) return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Contrast,   m_ColorCintrol.GetEVRColorControl(ProcAmp_Contrast))))   return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Hue,        m_ColorCintrol.GetEVRColorControl(ProcAmp_Hue))))        return;
		if (FAILED(m_pMFVP->GetProcAmpRange(DXVA2_ProcAmp_Saturation, m_ColorCintrol.GetEVRColorControl(ProcAmp_Saturation)))) return;
		m_ColorCintrol.EnableEVRColorControl();

		SetColorControl(ProcAmp_All, AfxGetAppSettings().iBrightness, AfxGetAppSettings().iContrast, AfxGetAppSettings().iHue, AfxGetAppSettings().iSaturation);
	}
}

void CMainFrame::SetupVMR9ColorControl()
{
	if (m_pVMRMC9) {
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorCintrol.GetVMR9ColorControl(ProcAmp_Brightness)))) return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorCintrol.GetVMR9ColorControl(ProcAmp_Contrast))))   return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorCintrol.GetVMR9ColorControl(ProcAmp_Hue))))        return;
		if (FAILED(m_pVMRMC9->GetProcAmpControlRange(0, m_ColorCintrol.GetVMR9ColorControl(ProcAmp_Saturation)))) return;
		m_ColorCintrol.EnableVMR9ColorControl();

		SetColorControl(ProcAmp_All, AfxGetAppSettings().iBrightness, AfxGetAppSettings().iContrast, AfxGetAppSettings().iHue, AfxGetAppSettings().iSaturation);
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
		VMR9ProcAmpControl ClrControl = m_ColorCintrol.GetVMR9ProcAmpControl(flags, brightness, contrast, hue, saturation);
		m_pVMRMC9->SetProcAmpControl(0, &ClrControl);
	}
	else if (m_pMFVP) {
		DXVA2_ProcAmpValues ClrValues = m_ColorCintrol.GetEVRProcAmpValues(brightness, contrast, hue, saturation);
		m_pMFVP->SetProcAmpValues(flags, &ClrValues);
	}
}

void CMainFrame::SetClosedCaptions(bool enable)
{
	if (m_pLN21) {
		m_pLN21->SetServiceState(enable ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off);
	}
}

LPCTSTR CMainFrame::GetDVDAudioFormatName(DVD_AudioAttributes& ATR) const
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
			if (nID == ID_SUB_DELAY_DOWN) {
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
		if (m_pSubStreams.IsEmpty()) {
			SendStatusMessage(ResStr(IDS_SUBTITLES_ERROR), 3000);
			return;
		}

		int newDelay;
		int oldDelay = m_pCAP->GetSubtitleDelay();

		if (nID == ID_SUB_DELAY_DOWN) {
			newDelay = oldDelay - AfxGetAppSettings().nSubDelayInterval;
		} else {
			newDelay = oldDelay + AfxGetAppSettings().nSubDelayInterval;
		}

		SetSubtitleDelay(newDelay);
	}
}

afx_msg void CMainFrame::OnLanguage(UINT nID)
{
	CMenu	defaultMenu;
	CMenu*	oldMenu;

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
	m_popupMenu.LoadMenu(IDR_POPUP);
	m_popupMainMenu.DestroyMenu();
	m_popupMainMenu.LoadMenu(IDR_POPUPMAIN);

	oldMenu = GetMenu();
	defaultMenu.LoadMenu(IDR_MAINFRAME);
	if (oldMenu) {
		// Attach the new menu to the window only if there was a menu before
		SetMenu(&defaultMenu);
		// and then destroy the old one
		oldMenu->DestroyMenu();
	}
	m_hMenuDefault = defaultMenu.Detach();

	// Re-create Win 7 TaskBar preview button for change button hint
	CreateThumbnailToolbar();

	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();
	OnTimer(TIMER_STATS);

	AfxGetAppSettings().SaveSettings();
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
			SendAPICommand(CMD_VERSION, MPC_VERSION_SVN_WSTR);
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
		vswprintf_s(buff, _countof(buff), fmt, args);

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
			if (!pli.m_fns.empty()) {
				label = !pli.m_label.IsEmpty() ? pli.m_label : pli.m_fns.front().GetName();

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
		POSITION pos = m_pSubStreams.GetHeadPosition();
		if (pos) {
			while (pos) {
				CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

				for (int i = 0, j = pSubStream->GetStreamCount(); i < j; i++) {
					WCHAR* pName = nullptr;
					if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, nullptr))) {
						CString name(pName);
						if (!strSubs.IsEmpty()) {
							strSubs.Append (L"|");
						}
						name.Replace(L"|", L"\\|");
						strSubs.AppendFormat(L"%s", name);
						CoTaskMemFree(pName);
					}
				}
			}
			if (AfxGetAppSettings().fEnableSubtitles) {
				if (m_iSubtitleSel >= 0) {
					strSubs.AppendFormat(L"|%i", m_iSubtitleSel);
				} else {
					strSubs.Append(L"|-1");
				}
			} else {
				strSubs.Append (L"|-1");
			}
		} else {
			strSubs.Append (L"-1");
		}
	} else {
		strSubs.Append (L"-2");
	}

	SendAPICommand(CMD_LISTSUBTITLETRACKS, L"%s", strSubs);
}

void CMainFrame::SendAudioTracksToApi()
{
	CString strAudios;

	if (m_eMediaLoadState == MLS_LOADED) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;

		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			int currentStream = -1;
			for (int i = 0; i < (int)cStreams; i++) {
				DWORD dwFlags  = DWORD_MAX;
				WCHAR* pszName = nullptr;
				if (FAILED(pSS->Info(i, nullptr, &dwFlags, nullptr, nullptr, &pszName, nullptr, nullptr)) || !pszName) {
					return;
				}
				if (dwFlags == AMSTREAMSELECTINFO_EXCLUSIVE) {
					currentStream = i;
				}
				CString name(pszName);
				if (!strAudios.IsEmpty()) {
					strAudios.Append (L"|");
				}
				name.Replace(L"|", L"\\|");
				strAudios.AppendFormat(L"%s", name);

				CoTaskMemFree(pszName);
			}
			strAudios.AppendFormat(L"|%i", currentStream);

		} else {
			strAudios.Append(L"-1");
		}
	} else {
		strAudios.Append(L"-2");
	}

	SendAPICommand(CMD_LISTAUDIOTRACKS, L"%s", strAudios);
}

void CMainFrame::SendPlaylistToApi()
{
	CString strPlaylist;

	POSITION pos = m_wndPlaylistBar.m_pl.GetHeadPosition();
	while (pos) {
		CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);

		if (pli.m_type == CPlaylistItem::file) {
			for (const auto& fi : pli.m_fns) {
				CString fn = fi.GetName();
				if (!strPlaylist.IsEmpty()) {
					strPlaylist.Append (L"|");
				}
				fn.Replace(L"|", L"\\|");
				strPlaylist.AppendFormat(L"%s", fn);
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
		path = AddSlash(path);
		s.strLastOpenDir = path;

		std::list<CString> sl;
		sl.push_back(path);
		if (recur) {
			RecurseAddDir(path, sl);
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
}

HRESULT CMainFrame::CreateThumbnailToolbar()
{
	if (!SysVersion::IsWin7orLater() || !AfxGetAppSettings().fUseWin7TaskBar) {
		return E_FAIL;
	}

	if (m_pTaskbarList) {
		m_pTaskbarList->Release();
	}

	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbarList));
	if (SUCCEEDED(hr)) {
		CMPCPngImage mpc_png;
		BYTE* pData;
		int width, height, bpp;

		HBITMAP hB = mpc_png.TypeLoadImage(IMG_TYPE::PNG, &pData, &width, &height, &bpp, nullptr, IDB_W7_TOOLBAR, 0, 0, 0, 0);
		if (!hB) {
			m_pTaskbarList->Release();
			return E_FAIL;
		}

		// Check dimensions
		BITMAP bi = {0};
		GetObjectW((HANDLE)hB, sizeof(bi), &bi);
		if (bi.bmHeight == 0) {
			DeleteObject(hB);
			m_pTaskbarList->Release();
			return E_FAIL;
		}

		int nI = bi.bmWidth/bi.bmHeight;
		HIMAGELIST himl = ImageList_Create(bi.bmHeight, bi.bmHeight, ILC_COLOR32, nI, 0);

		// Add the bitmap
		ImageList_Add(himl, hB, 0);
		DeleteObject(hB);
		hr = m_pTaskbarList->ThumbBarSetImageList(m_hWnd, himl);

		if (SUCCEEDED(hr)) {
			THUMBBUTTON buttons[5] = {};

			// PREVIOUS
			buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[0].dwFlags = THBF_DISABLED;
			buttons[0].iId = IDTB_BUTTON3;
			buttons[0].iBitmap = 0;
			StringCchCopyW(buttons[0].szTip, _countof(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

			// STOP
			buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[1].iId = IDTB_BUTTON1;
			buttons[1].iBitmap = 1;
			StringCchCopyW(buttons[1].szTip, _countof(buttons[1].szTip), ResStr(IDS_AG_STOP));

			// PLAY/PAUSE
			buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[2].iId = IDTB_BUTTON2;
			buttons[2].iBitmap = 3;
			StringCchCopyW(buttons[2].szTip, _countof(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

			// NEXT
			buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[3].dwFlags = THBF_DISABLED;
			buttons[3].iId = IDTB_BUTTON4;
			buttons[3].iBitmap = 4;
			StringCchCopyW(buttons[3].szTip, _countof(buttons[3].szTip), ResStr(IDS_AG_NEXT));

			// FULLSCREEN
			buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[4].dwFlags = THBF_DISABLED;
			buttons[4].iId = IDTB_BUTTON5;
			buttons[4].iBitmap = 5;
			StringCchCopyW(buttons[4].szTip, _countof(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

			hr = m_pTaskbarList->ThumbBarAddButtons(m_hWnd, ARRAYSIZE(buttons), buttons);
		}
		ImageList_Destroy(himl);
	}

	UpdateThumbarButton();

	return hr;
}

HRESULT CMainFrame::UpdateThumbarButton()
{
	if (!m_pTaskbarList) {
		return E_FAIL;
	}

	if (!AfxGetAppSettings().fUseWin7TaskBar) {
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

		HRESULT hr = m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, ARRAYSIZE(buttons), buttons);
		return hr;
	}

	THUMBBUTTON buttons[5] = {};

	buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[0].dwFlags = (AfxGetAppSettings().fDontUseSearchInFolder && m_wndPlaylistBar.GetCount() <= 1 && (m_pCB && m_pCB->ChapGetCount() <= 1)) ? THBF_DISABLED : THBF_ENABLED;
	buttons[0].iId = IDTB_BUTTON3;
	buttons[0].iBitmap = 0;
	StringCchCopyW(buttons[0].szTip, _countof(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

	buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[1].iId = IDTB_BUTTON1;
	buttons[1].iBitmap = 1;
	StringCchCopyW(buttons[1].szTip, _countof(buttons[1].szTip), ResStr(IDS_AG_STOP));

	buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[2].iId = IDTB_BUTTON2;
	buttons[2].iBitmap = 3;
	StringCchCopyW(buttons[2].szTip, _countof(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

	buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[3].dwFlags = (AfxGetAppSettings().fDontUseSearchInFolder && m_wndPlaylistBar.GetCount() <= 1 && (m_pCB && m_pCB->ChapGetCount() <= 1)) ? THBF_DISABLED : THBF_ENABLED;
	buttons[3].iId = IDTB_BUTTON4;
	buttons[3].iBitmap = 4;
	StringCchCopyW(buttons[3].szTip, _countof(buttons[3].szTip), ResStr(IDS_AG_NEXT));

	buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[4].dwFlags = THBF_ENABLED;
	buttons[4].iId = IDTB_BUTTON5;
	buttons[4].iBitmap = 5;
	StringCchCopyW(buttons[4].szTip, _countof(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

	HICON hIcon = nullptr;

	if (m_eMediaLoadState == MLS_LOADED) {
		OAFilterState fs = GetMediaState();
		if (fs == State_Running) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 2;

			hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_TB_PLAY), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, m_wndSeekBar.HasDuration() ? TBPF_NORMAL : TBPF_NOPROGRESS);
		} else if (fs == State_Stopped) {
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_TB_STOP), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
		} else if (fs == State_Paused) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_TB_PAUSE), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		}

		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) {
			buttons[0].dwFlags = THBF_DISABLED;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[3].dwFlags = THBF_DISABLED;
		}

		m_pTaskbarList->SetOverlayIcon(m_hWnd, hIcon, L"");

		if (hIcon != nullptr) {
			DestroyIcon( hIcon );
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

	HRESULT hr = m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, ARRAYSIZE(buttons), buttons);

	UpdateThumbnailClip();

	return hr;
}

HRESULT CMainFrame::UpdateThumbnailClip()
{
	CheckPointer(m_pTaskbarList, E_FAIL);

	CRect r;
	m_wndView.GetClientRect(&r);
	if (!IsMenuHidden()) {
		r.OffsetRect(0, GetSystemMetrics(SM_CYMENU));
	}

	if (!AfxGetAppSettings().fUseWin7TaskBar || m_eMediaLoadState != MLS_LOADED || (m_bAudioOnly && !SysVersion::IsWin10orLater()) || m_bFullScreen || IsD3DFullScreenMode() || r.IsRectEmpty()) {
		return m_pTaskbarList->SetThumbnailClip(m_hWnd, nullptr);
	}

	return m_pTaskbarList->SetThumbnailClip(m_hWnd, &r);
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
	if (enable && !AfxGetAppSettings().ShaderListScreenSpace.empty()) {
		m_bToggleShaderScreenSpace = true;
		SetShaders();
	} else {
		m_bToggleShaderScreenSpace = false;
		if (m_pCAP) {
			m_pCAP->ClearPixelShaders(TARGET_SCREEN);
		}
	}
}

BOOL CMainFrame::OpenBD(CString path, REFERENCE_TIME rtStart/* = INVALID_TIME*/, BOOL bAddRecent/* = TRUE*/)
{
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();

	const CString originalPath(path);

	path.TrimRight('\\');
	if (path.Right(5).MakeLower() == L".bdmv") {
		path.Truncate(path.ReverseFind('\\'));
	} else if (path.Right(5).MakeLower() == L".mpls") {
		path.Truncate(path.Find(L"\\PLAYLIST"));
	} else if (::PathIsDirectoryW(path + L"\\BDMV")) {
		path += L"\\BDMV";
	}

	if (::PathIsDirectoryW(path + L"\\PLAYLIST") && ::PathIsDirectoryW(path + L"\\STREAM")) {
		CHdmvClipInfo	ClipInfo;
		CString			strPlaylistFile;
		CHdmvClipInfo::CPlaylist MainPlaylist;

		if (SUCCEEDED(ClipInfo.FindMainMovie(path, strPlaylistFile, MainPlaylist, m_BDPlaylists))) {
			if (path.Right(5).MakeUpper() == L"\\BDMV") {
				path.Truncate(path.GetLength() - 5);
				CString infFile = path + L"\\disc.inf";
				if (::PathFileExistsW(infFile)) {
					CTextFile cf;
					if (cf.Open(infFile)) {
						CString line;
						while (cf.ReadString(line)) {
							CAtlList<CString> sl;
							Explode(line, sl, L'=');
							if (sl.GetCount() == 2 && CString(sl.GetHead().Trim()).MakeLower() == L"label") {
								m_BDLabel = sl.GetTail();
								break;
							}
						}
					}
				}
			}

			if (bAddRecent) {
				AddRecent(originalPath.Right(5).MakeLower() == L".mpls" ? originalPath : path);
			}

			SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_bIsBDPlay = TRUE;
			m_LastOpenBDPath = path;

			if (OpenFile(originalPath.Right(5).MakeLower() == L".mpls" ? originalPath : strPlaylistFile, rtStart, FALSE) == TRUE) {
				return TRUE;
			}
		}
	}

	m_bIsBDPlay = FALSE;
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();
	return FALSE;
}

BOOL CMainFrame::CheckBD(CString path)
{
	path.TrimRight('\\');
	if (path.Right(5).MakeLower() == L".bdmv") {
		path.Truncate(path.ReverseFind('\\'));
	} else if (path.Right(5).MakeLower() == L".mpls") {
		path.Truncate(path.Find(L"\\PLAYLIST"));
	} else if (::PathIsDirectoryW(path + L"\\BDMV")) {
		path += L"\\BDMV";
	}

	return ::PathFileExistsW(path + L"\\index.bdmv");
}

BOOL CMainFrame::CheckDVD(CString path)
{
	path.TrimRight('\\');
	if (path.Right(13).MakeLower() == L"\\video_ts.ifo") {
		path.Truncate(path.ReverseFind('\\'));
	} else if (::PathIsDirectoryW(path + L"\\VIDEO_TS")) {
		path += L"\\VIDEO_TS";
	}

	return ::PathFileExistsW(path + L"\\VIDEO_TS.IFO");
}

void CMainFrame::SetStatusMessage(CString m_msg)
{
	if (m_OldMessage != m_msg) {
		m_wndStatusBar.SetStatusMessage(m_msg);
	}
	m_OldMessage = m_msg;
}

CString CMainFrame::FillMessage()
{
	CString msg;

	if (!m_playingmsg.IsEmpty()) {
		msg = m_playingmsg;
	} else if (m_fCapturing) {
		msg = ResStr(IDS_CONTROLS_CAPTURING);

		if (pAMDF) {
			long lDropped = 0;
			pAMDF->GetNumDropped(&lDropped);
			long lNotDropped = 0;
			pAMDF->GetNumNotDropped(&lNotDropped);

			if ((lDropped + lNotDropped) > 0) {
				CString str;
				str.Format(ResStr(IDS_MAINFRM_37), lDropped + lNotDropped, lDropped);
				msg += str;
			}
		}

		CComPtr<IPin> pPin;
		if (SUCCEEDED(pCGB->FindPin(m_wndCaptureBar.m_capdlg.m_pDst, PINDIR_INPUT, nullptr, nullptr, FALSE, 0, &pPin))) {
			LONGLONG size = 0;
			if (CComQIPtr<IStream> pStream = pPin) {
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
				CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
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
				if (CComQIPtr<IBufferFilter> pVB = m_wndCaptureBar.m_capdlg.m_pVidBuffer) {
					nFreeVidBuffers = pVB->GetFreeBuffers();
				}
				if (CComQIPtr<IBufferFilter> pAB = m_wndCaptureBar.m_capdlg.m_pAudBuffer) {
					nFreeAudBuffers = pAB->GetFreeBuffers();
				}

				CString str;
				str.Format(ResStr(IDS_MAINFRM_42), nFreeVidBuffers, nFreeAudBuffers);
				msg += str;
			}
		}
	} else if (m_fBuffering) {
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF) {
				long BufferingProgress = 0;
				if (SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && BufferingProgress > 0) {
					msg.Format(ResStr(IDS_CONTROLS_BUFFERING), BufferingProgress);

					__int64 stop;
					m_wndSeekBar.GetRange(stop);
					m_fLiveWM = (stop == 0);
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

		if (m_fUpdateInfoBar) {
			OpenSetupInfoBar();
			OpenSetupWindowTitle(m_strPlaybackRenderedPath);
		}
	}

	return msg;
}

bool CMainFrame::CanPreviewUse()
{
	return (m_bUseSmartSeek
			&& m_eMediaLoadState == MLS_LOADED
			&& (GetPlaybackMode() == PM_DVD || GetPlaybackMode() == PM_FILE)
			&& !m_bAudioOnly
			&& AfxGetAppSettings().fSmartSeek);
}

bool CheckCoverImgExist(CString &path, CString name) {
	CPath coverpath;
	coverpath.Combine(path, name);

	if (coverpath.FileExists() ||
		(coverpath.RenameExtension(L".jpeg") && coverpath.FileExists()) ||
		(coverpath.RenameExtension(L".png")  && coverpath.FileExists())) {
			path.SetString(coverpath);
			return true;
	}
	return false;
}

CString GetCoverImgFromPath(CString fullfilename)
{
	CString path = fullfilename.Left(fullfilename.ReverseFind('\\') + 1);


	if (CheckCoverImgExist(path, L"cover.jpg")) {
		return path;
	}

	if (CheckCoverImgExist(path, L"folder.jpg")) {
		return path;
	}

	CPath dir(path);
	dir.RemoveBackslash();
	int k = dir.FindFileName();
	if (k >= 0) {
		if (CheckCoverImgExist(path, CString(dir).Right(k) + L".jpg")) {
			return path;
		}
	}

	if (CheckCoverImgExist(path, L"front.jpg")) {
		return path;
	}

	CString fname = fullfilename.Mid(path.GetLength());
	fname = fname.Left(fname.ReverseFind('.'));
	if (CheckCoverImgExist(path, fname + L".jpg")) {
		return path;
	}

	if (CheckCoverImgExist(path, L"cover\\front.jpg")) {
		return path;
	}

	if (CheckCoverImgExist(path, L"covers\\front.jpg")) {
		return path;
	}

	if (CheckCoverImgExist(path, L"box.jpg")) {
		return path;
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

	if (m_DwmSetWindowAttributeFnc && m_DwmSetIconicThumbnailFnc) {
		// for customize an iconic thumbnail and a live preview in Windows 7/8
		const BOOL set = s.fUseWin7TaskBar && m_bAudioOnly && IsSomethingLoaded() && show;
		m_DwmSetWindowAttributeFnc(GetSafeHwnd(), DWMWA_HAS_ICONIC_BITMAP, &set, sizeof(set));
		m_DwmSetWindowAttributeFnc(GetSafeHwnd(), DWMWA_FORCE_ICONIC_REPRESENTATION, &set, sizeof(set));
	}

	m_InternalImage.Destroy();
	if (m_InternalImageSmall) {
		m_InternalImageSmall.Detach();
	}

	if (m_bAudioOnly && IsSomethingLoaded() && show) {

		bool bLoadRes = false;
		if (s.nAudioWindowMode == 1) {
			// load image from DSMResource to show in preview & logo;
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IDSMResourceBag> pRB = pBF)
					if (pRB && CheckMainFilter(pBF) && pRB->ResGetCount() > 0) {
						for (DWORD i = 0; i < pRB->ResGetCount() && bLoadRes == false; i++) {
							CComBSTR name, desc, mime;
							BYTE* pData = nullptr;
							DWORD len = 0;
							if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, nullptr))) {
								if (CString(mime).Trim() == L"image/jpeg"
									|| CString(mime).Trim() == L"image/jpg"
									|| CString(mime).Trim() == L"image/png") {

									HGLOBAL hBlock = ::GlobalAlloc(GMEM_MOVEABLE, len);
									if (hBlock != nullptr) {
										LPVOID lpResBuffer = ::GlobalLock(hBlock);
										ASSERT(lpResBuffer != nullptr);
										memcpy(lpResBuffer, pData, len);

										IStream* pStream = nullptr;
										if (SUCCEEDED(::CreateStreamOnHGlobal(hBlock, TRUE, &pStream))) {
											m_InternalImage.Load(pStream);
											pStream->Release();
											bLoadRes = true;
										}

										::GlobalUnlock(hBlock);
										::GlobalFree(hBlock);
									}
								}
								CoTaskMemFree(pData);
							}
						}
					}
			}
			EndEnumFilters;

			if (!bLoadRes) {
				// try to load image from file in the same dir that media file to show in preview & logo;
				CString img_fname = GetCoverImgFromPath(m_strPlaybackRenderedPath);

				if (!img_fname.IsEmpty()) {
					if (SUCCEEDED(m_InternalImage.Load(img_fname))) {
						bLoadRes = true;
					}
				}
			}
		}

		if (!bLoadRes) {
			m_InternalImage.LoadFromResource(IDB_W7_AUDIO);
			m_InternalImageSmall.Attach(m_InternalImage);
		} else {
			BITMAP bm;
			if (GetObjectW(m_InternalImage, sizeof(bm), &bm) && bm.bmWidth) {
				const int nWidth = 256;
				if ((abs(bm.bmHeight) <= nWidth) && (bm.bmWidth <= nWidth)) {
					m_InternalImageSmall.Attach(m_InternalImage);
				} else {
					// Resize image to improve speed of show TaskBar preview

					int h = std::min(abs(bm.bmHeight), (long)nWidth);
					int w = MulDiv(h, bm.bmWidth, abs(bm.bmHeight));
					h     = MulDiv(w, abs(bm.bmHeight), bm.bmWidth);

					CDC *screenDC = GetDC();
					CDC *pMDC = DNew CDC;
					pMDC->CreateCompatibleDC(screenDC);

					CBitmap *pb = DNew CBitmap;
					pb->CreateCompatibleBitmap(screenDC, w, h);

					CBitmap *pob = pMDC->SelectObject(pb);
					SetStretchBltMode(pMDC->m_hDC, STRETCH_HALFTONE);
					m_InternalImage.StretchBlt(pMDC->m_hDC, 0, 0, w, h, 0, 0, bm.bmWidth, abs(bm.bmHeight), SRCCOPY);
					pMDC->SelectObject(pob);

					m_InternalImageSmall.Attach((HBITMAP)(pb->Detach()));
					pb->DeleteObject();
					ReleaseDC(screenDC);

					delete pMDC;
					delete pb;
				}
			}
		}
	}

	m_wndView.Invalidate();

	return S_OK;
}

LRESULT CMainFrame::OnDwmSendIconicThumbnail(WPARAM wParam, LPARAM lParam)
{
	if (!IsSomethingLoaded() || !m_bAudioOnly || !m_DwmSetIconicThumbnailFnc || !m_InternalImageSmall) {
		return 0;
	}

	long nWidth  = HIWORD(lParam);
	long nHeight = LOWORD(lParam);

	if (m_ThumbCashedBitmap && m_ThumbCashedSize != CSize(nWidth, nHeight)) {
		::DeleteObject(m_ThumbCashedBitmap);
		m_ThumbCashedBitmap = nullptr;
	}

	if (!m_ThumbCashedBitmap) {
		HDC hdcMem = CreateCompatibleDC(nullptr);

		if (hdcMem) {
			BITMAP bm;
			GetObjectW(m_InternalImageSmall, sizeof(bm), &bm);

			int h = min(abs(bm.bmHeight), nHeight);
			int w = MulDiv(h, bm.bmWidth, abs(bm.bmHeight));
			h     = MulDiv(w, abs(bm.bmHeight), bm.bmWidth);

			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth       = nWidth;
			bmi.bmiHeader.biHeight      = -nHeight;
			bmi.bmiHeader.biPlanes      = 1;
			bmi.bmiHeader.biBitCount    = 32;

			VOID* pvBits = nullptr;
			m_ThumbCashedBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
			if (m_ThumbCashedBitmap) {
				SelectObject(hdcMem, m_ThumbCashedBitmap);

				int x = nWidth / 2  - w / 2;
				int y = nHeight / 2 - h / 2;
				CRect r = CRect(CPoint(x, y), CSize(w, h));

				SetStretchBltMode(hdcMem, STRETCH_HALFTONE);
				m_InternalImageSmall.StretchBlt(hdcMem, r, CRect(0, 0, bm.bmWidth, abs(bm.bmHeight)));
			}

			DeleteDC(hdcMem);
		}
	}

	HRESULT hr = E_FAIL;

	if (m_ThumbCashedBitmap) {
		hr = m_DwmSetIconicThumbnailFnc(m_hWnd, m_ThumbCashedBitmap, 0);
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
	if (!IsSomethingLoaded() || !m_bAudioOnly || !m_DwmSetIconicLivePreviewBitmapFnc) {
		return 0;
	}

	CAppSettings& s = AfxGetAppSettings();

	CRect rectClient(0, 0, 0, 0);
	if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
		GetWindowRect(&rectClient);
	} else {
		GetClientRect(&rectClient);
	}

	POINT offset = {0, 0};
	DWORD style = GetStyle();
	if ( style & WS_CAPTION ) {
		offset.x = (rectClient.left) - GetSystemMetrics(SM_CXSIZEFRAME);
		offset.y = (rectClient.top)  - (GetSystemMetrics(SM_CYCAPTION) + SM_CYSIZEFRAME);
	} else if (style & WS_THICKFRAME) {
		offset.x = (rectClient.left) - GetSystemMetrics(SM_CXSIZEFRAME);
		offset.y = (rectClient.top)  - GetSystemMetrics(SM_CYSIZEFRAME);
	}

	HRESULT hr = E_FAIL;

	if (isWindowMinimized && m_CaptureWndBitmap) {
		hr = m_DwmSetIconicLivePreviewBitmapFnc(m_hWnd, m_CaptureWndBitmap, nullptr, (style & WS_CAPTION || style & WS_THICKFRAME) ? DWM_SIT_DISPLAYFRAME : 0);
	} else {
		HBITMAP hBitmap = CreateCaptureDIB(rectClient.Width(), rectClient.Height());
		if (hBitmap) {
			if (style & WS_CAPTION || style & WS_THICKFRAME) {
				hr = m_DwmSetIconicLivePreviewBitmapFnc(m_hWnd, hBitmap, nullptr, DWM_SIT_DISPLAYFRAME);
			} else {
				hr = m_DwmSetIconicLivePreviewBitmapFnc(m_hWnd, hBitmap, &offset, 0);
			}
			::DeleteObject(hBitmap);
		}
	}

	if (FAILED(hr)) {
		// TODO ...
	}

	return 0;
}

HBITMAP CMainFrame::CreateCaptureDIB(int nWidth, int nHeight)
{
	CAppSettings& s = AfxGetAppSettings();

	HBITMAP hbm = nullptr;
	CWindowDC hDCw(this), hDCc(this);
	HDC hdcMem = CreateCompatibleDC(hDCc);

	bool bCaptionWithMenu = (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) ? true : false;
	DWORD style = GetStyle();

	if (hdcMem != nullptr) {

		BITMAPINFO bmi;
		ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		if (bCaptionWithMenu) {
			bmi.bmiHeader.biWidth	= nWidth - (GetSystemMetrics(SM_CXSIZEFRAME) * 2);
			bmi.bmiHeader.biHeight	= -(nHeight - GetSystemMetrics(SM_CYCAPTION) - (GetSystemMetrics(SM_CYSIZEFRAME) * 2));
		} else {
			bmi.bmiHeader.biWidth	= nWidth;
			bmi.bmiHeader.biHeight	= -nHeight;
		}
		bmi.bmiHeader.biPlanes		= 1;
		bmi.bmiHeader.biBitCount	= 32;

		PBYTE pbDS = nullptr;
		hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, nullptr, 0);
		if (hbm) {
			SelectObject(hdcMem, hbm);
			if (bCaptionWithMenu) {
				BitBlt(
					hdcMem,
					0,
					0,
					nWidth + (GetSystemMetrics(SM_CXSIZEFRAME) * 2),
					nHeight + (GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME)),
					hDCw,
					GetSystemMetrics(SM_CXSIZEFRAME),
					GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME),
					MERGECOPY
				);
			} else {
				BitBlt(
					hdcMem,
					0,
					0,
					nWidth,
					nHeight,
					hDCc,
					0,
					0,
					MERGECOPY
				);
			}
		}
		DeleteDC(hdcMem);
	}

	return hbm;
}

void CMainFrame::CreateCaptureWindow()
{
	CAppSettings& s = AfxGetAppSettings();
	CRect rectClient(0, 0, 0, 0);

	if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
		GetWindowRect(&rectClient);
	} else {
		GetClientRect(&rectClient);
	}

	m_CaptureWndBitmap = CreateCaptureDIB(rectClient.Width(), rectClient.Height());
}

const CString CMainFrame::GetStrForTitle()
{
	if (m_eMediaLoadState == MLS_LOADED) {
		if (!m_youtubeFields.title.IsEmpty()) {
			return m_youtubeFields.title;
		} else if (m_bIsBDPlay || GetPlaybackMode() == PM_DVD) {
			return m_strPlaybackLabel;
		} else {
			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
					if (!CheckMainFilter(pBF)) {
						continue;
					}

					CComBSTR bstr;
					if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
						return CString(bstr.m_str);
					}
				}
			}
			EndEnumFilters;
		}

		CPlaylistItem pli;
		if (m_wndPlaylistBar.GetCur(pli) && !pli.m_label.IsEmpty()) {
			return pli.m_label;
		}

		return m_strPlaybackLabel;
	}

	return L"";
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

void CMainFrame::subChangeNotifySetupThread(CAtlArray<HANDLE>& handles)
{
	for (size_t i = 2; i < handles.GetCount(); i++) {
		FindCloseChangeNotification(handles[i]);
	}
	handles.RemoveAll();
	handles.Add(m_EventSubChangeStopNotify);
	handles.Add(m_EventSubChangeRefreshNotify);
	handles.Append(m_ExtSubPathsHandles);
}

void CMainFrame::subChangeNotifyThreadFunction()
{
	CAtlArray<HANDLE> handles;
	subChangeNotifySetupThread(handles);

	while (true) {
		DWORD idx = WaitForMultipleObjects(handles.GetCount(), handles.GetData(), FALSE, INFINITE);
		if (idx == WAIT_OBJECT_0) { // m_hStopNotifyRenderThreadEvent
			break;
		} else if (idx == (WAIT_OBJECT_0 + 1)) { // m_hRefreshNotifyRenderThreadEvent
			subChangeNotifySetupThread(handles);
		} else if (idx > (WAIT_OBJECT_0 + 1) && idx < (WAIT_OBJECT_0 + handles.GetCount())) {
			if (FindNextChangeNotification(handles[idx - WAIT_OBJECT_0]) == FALSE) {
				break;
			}

			BOOL bChanged = FALSE;
			for (INT_PTR idx = 0; idx < m_ExtSubFiles.GetCount(); idx++) {
				CFileStatus status;
				CString fn = m_ExtSubFiles[idx];
				if (CFileGetStatus(fn, status) && m_ExtSubFilesTime[idx] != status.m_mtime) {
					m_ExtSubFilesTime[idx] = status.m_mtime;
					bChanged = TRUE;
				}
			}

			if (bChanged) {
				ReloadSubtitle();
			}
		} else {
			DLog(L"CMainFrame::NotifyRenderThread() : %s", GetLastErrorMsg(L"WaitForMultipleObjects"));
			ASSERT(FALSE);
			break;
		}
	}

	for (size_t i = 2; i < handles.GetCount(); i++) {
		FindCloseChangeNotification(handles[i]);
	}
}

int CMainFrame::GetStreamCount(DWORD dwSelGroup)
{
	int streamcount = 0;
	if (CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter) {
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

void CMainFrame::AddSubtitlePathsAddons(CString FileName)
{
	CString tmp(AddSlash(GetFolderOnly(FileName)).MakeUpper());
	CAppSettings& s = AfxGetAppSettings();

	if (std::find(s.slSubtitlePathsAddons.cbegin(), s.slSubtitlePathsAddons.cend(), tmp) == s.slSubtitlePathsAddons.cend()) {
		s.slSubtitlePathsAddons.push_back(tmp);
	}
}

void CMainFrame::AddAudioPathsAddons(CString FileName)
{
	CString tmp(AddSlash(GetFolderOnly(FileName)).MakeUpper());
	CAppSettings& s = AfxGetAppSettings();

	if (std::find(s.slAudioPathsAddons.cbegin(), s.slAudioPathsAddons.cend(), tmp) == s.slAudioPathsAddons.cend()) {
		s.slAudioPathsAddons.push_back(tmp);
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
			&& m_pDVDI && SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, _countof(buff), &len))) {
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
			fn = CString(pFN);
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

BOOL CMainFrame::OpenIso(CString pathName, REFERENCE_TIME rtStart/* = INVALID_TIME*/)
{
	if (m_DiskImage.CheckExtension(pathName)) {
		SendMessageW(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		WCHAR diskletter = m_DiskImage.MountDiskImage(pathName);
		if (diskletter) {
			if (OpenBD(CString(diskletter) + L":\\", rtStart, FALSE)) {
				AddRecent(pathName);
				return TRUE;
			}

			if (::PathFileExistsW(CString(diskletter) + L":\\VIDEO_TS\\VIDEO_TS.IFO")) {
				CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
				p->path = CString(diskletter) + L":\\";
				p->bAddRecent = FALSE;
				OpenMedia(p);

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

void CMainFrame::AddRecent(CString pathName)
{
	if (AfxGetAppSettings().bKeepHistory) {
		CRecentFileList* pMRU = &AfxGetAppSettings().MRU;
		pMRU->ReadList();
		pMRU->Add(pathName);
		pMRU->WriteList();
		if (IsLikelyFilePath(pathName)) {
			// there should not be a URL, otherwise explorer dirtied HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
			SHAddToRecentDocs(SHARD_PATHW, pathName); // remember the last open files (system) through the drag-n-drop
		}
	}
}

void CMainFrame::CheckMenuRadioItem(UINT first, UINT last, UINT check)
{
	m_popupMenu.CheckMenuRadioItem(first, last, check, MF_BYCOMMAND);
	m_popupMainMenu.CheckMenuRadioItem(first, last, check, MF_BYCOMMAND);
	::CheckMenuRadioItem(m_hMenuDefault, first, last, check, MF_BYCOMMAND);
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
				AM_MEDIA_TYPE mt;
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

BOOL CMainFrame::OpenYoutubePlaylist(CString url, BOOL bOnlyParse/* = FALSE*/)
{
	if (AfxGetAppSettings().bYoutubeLoadPlaylist && Youtube::CheckPlaylist(url)) {
		Youtube::YoutubePlaylist youtubePlaylist;
		int idx_CurrentPlay = 0;
		if (Youtube::Parse_Playlist(url, youtubePlaylist, idx_CurrentPlay)) {
			if (!bOnlyParse) {
				m_wndPlaylistBar.Empty();
			}

			CFileItemList fis;
			for(auto item = youtubePlaylist.begin(); item != youtubePlaylist.end(); ++item) {
				CFileItem fi(item->url, item->title);
				fis.push_back(fi);
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
	if (!AfxGetAppSettings().bAddSimilarFiles
			|| fns.size() != 1
			|| !::PathFileExistsW(fns.front())
			|| ::PathIsDirectoryW(fns.front())) {
		return FALSE;
	}

	CString fname = fns.front();
	const CString path = AddSlash(GetFolderOnly(fname));
	fname = GetFileOnly(fname);
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
		L"\\d{5}\\.mpls";       // Blu-ray playlist
	const std::wregex excludeMaskRe(excludeMask, std::wregex::icase);
	if (std::regex_match((LPCTSTR)fname, excludeMaskRe)) {
		return FALSE;
	};

	const LPCWSTR excludeWords[][2] = {
		{L"720p",  L"<seven_hundred_twenty>"},
		{L"1080p", L"<thousand_and_eighty>"},
		{L"1440p", L"<thousand_four_hundred_forty>"},
		{L"2160p", L"<two_thousand_one_hundred_and_sixty>"},
	};

	int excludeWordsNum = -1;
	for (int i = 0; i < _countof(excludeWords); i++) {
		const auto& words = excludeWords[i];

		CString tmp(name);
		tmp.MakeLower();
		const int n = tmp.Find(words[0]);
		if (n >= 0) {
			name.Replace(words[0], words[1]);
			excludeWordsNum = i;
			break;
		}
	}

	const std::wregex replace_spec(LR"([\.\(\)\[\]\{\}\+])", std::wregex::icase);
	std::wstring regExp = std::regex_replace(name.GetString(), replace_spec, L"\\$&");

	const std::wregex replace_digit(LR"((\d+ *- *\d+|\d+))", std::wregex::icase);
	regExp = std::regex_replace(regExp, replace_digit, L"(\\d+ *- *\\d+|\\d+)");

	if (excludeWordsNum != -1) {
		const auto& words = excludeWords[excludeWordsNum];
		CString tmp(regExp.c_str());
		tmp.Replace(words[1], words[0]);
		regExp = tmp;
	}

	regExp += L".*";
	if (!ext.IsEmpty()) {
		regExp += L"\\" + ext;
	}

	const std::wregex mask(regExp, std::wregex::icase);

	std::vector<CString> files;
	WIN32_FIND_DATAW wfd = { 0 };
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

	if (!files.empty()) {
		bool bFoundCurFile = false;
		for (size_t i = 0; i < files.size(); i++) {
			const CString& fn = files[i];
			if (bFoundCurFile) {
				fns.push_back(path + fn);
			} else {
				bFoundCurFile = (fn == fname);
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
		} else if (!m_pSubStreams.IsEmpty()) {
			bEnabled = TRUE;
		} else if (m_pDVDI) {
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			bEnabled = SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled)) && (ulStreamsAvailable > 0);
		}
	}

	m_wndToolBar.m_bSubtitleEnable = bEnabled;
}

#pragma region GraphThread

//
// CGraphThread
//

IMPLEMENT_DYNCREATE(CGraphThread, CWinThread)

BOOL CGraphThread::InitInstance()
{
	SetThreadName((DWORD)-1, "GraphThread");
	AfxSocketInit();
	return SUCCEEDED(CoInitialize(0)) ? TRUE : FALSE;
}

int CGraphThread::ExitInstance()
{
	CoUninitialize();
	return __super::ExitInstance();
}

BEGIN_MESSAGE_MAP(CGraphThread, CWinThread)
	ON_THREAD_MESSAGE(TM_EXIT, OnExit)
	ON_THREAD_MESSAGE(TM_OPEN, OnOpen)
	ON_THREAD_MESSAGE(TM_CLOSE, OnClose)
	ON_THREAD_MESSAGE(TM_RESET, OnReset)
	ON_THREAD_MESSAGE(TM_TUNER_SCAN, OnTunerScan)
	ON_THREAD_MESSAGE(TM_DISPLAY_CHANGE, OnDisplayChange)
END_MESSAGE_MAP()

void CGraphThread::OnExit(WPARAM wParam, LPARAM lParam)
{
	PostQuitMessage(0);
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

void CGraphThread::OnOpen(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		ASSERT(WaitForSingleObject(m_pMainFrame->m_hGraphThreadEventOpen, 0) == WAIT_TIMEOUT);
		if (m_pMainFrame->m_eMediaLoadState == MLS_LOADING) {
			CAutoPtr<OpenMediaData> pOMD((OpenMediaData*)lParam);
			m_pMainFrame->OpenMediaPrivate(pOMD);
		}
		m_pMainFrame->m_hGraphThreadEventOpen.SetEvent();
	}
}

void CGraphThread::OnClose(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		ASSERT(WaitForSingleObject(m_pMainFrame->m_hGraphThreadEventClose, 0) == WAIT_TIMEOUT);
		if (m_pMainFrame->m_eMediaLoadState == MLS_CLOSING) {
			m_pMainFrame->CloseMediaPrivate();
		}
		m_pMainFrame->m_hGraphThreadEventClose.SetEvent();
	}
}

void CGraphThread::OnReset(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		BOOL* b = (BOOL*)wParam;
		BOOL bResult = m_pMainFrame->ResetDevice();
		if (b) {
			*b = bResult;
		}
	}
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

void CGraphThread::OnTunerScan(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		CAutoPtr<TunerScanData> pTSD((TunerScanData*)lParam);
		m_pMainFrame->DoTunerScan(pTSD);
	}
}

void CGraphThread::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainFrame) {
		m_pMainFrame->DisplayChange();
	}
	if (CAMEvent* e = (CAMEvent*)lParam) {
		e->Set();
	}
}

#pragma endregion GraphThread
