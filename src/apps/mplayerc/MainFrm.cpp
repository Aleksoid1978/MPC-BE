/*
 * (C) 2003-2006 Gabest
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

#include "stdafx.h"
#include "MainFrm.h"
#include <atlconv.h>
#include <atlrx.h>
#include <atlsync.h>
#include <afxtaskdialog.h>
#include <afxglobals.h>

#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include "../../DSUtil/Log.h"
#include "../../DSUtil/FileVersionInfo.h"
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
#include "OpenDirHelper.h"
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

#include "FGManager.h"
#include "FGManagerBDA.h"
#include "TextPassThruFilter.h"
#include "../../filters/filters.h"
#include "../../filters/filters/InternalPropertyPage.h"
#include <AllocatorCommon7.h>
#include <AllocatorCommon.h>
#include <SyncAllocatorPresenter.h>
#include "ComPropertyPage.h"
#include "LcdSupport.h"
#include "SettingsDefines.h"
#include <IPinHook.h>
#include <comdef.h>
#include <dwmapi.h>

#include "OpenImage.h"
#include "PlayerYouTube.h"

#include "../../Subtitles/RenderedHdmvSubtitle.h"
#include "../../Subtitles/SupSubFile.h"
#include "../../Subtitles/XSUBSubtitle.h"

#include "MultiMonitor.h"
#include <mvrInterfaces.h>

#define DEFCLIENTW		292
#define DEFCLIENTH		200
#define MENUBARBREAK	30

#define BLU_RAY			L"Blu-ray"

static UINT s_uTaskbarRestart		= RegisterWindowMessage(_T("TaskbarCreated"));
static UINT s_uTBBC					= RegisterWindowMessage(_T("TaskbarButtonCreated"));
static UINT WM_NOTIFYICON			= RegisterWindowMessage(_T("MYWM_NOTIFYICON"));
static UINT s_uQueryCancelAutoPlay	= RegisterWindowMessage(_T("QueryCancelAutoPlay"));

class __declspec(uuid("5933BB4F-EC4D-454E-8E11-B74DDA92E6F9")) ChaptersSouce : public CSource, public IDSMChapterBagImpl
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

public:
	ChaptersSouce();
	virtual ~ChaptersSouce() {}

	DECLARE_IUNKNOWN;
};

ChaptersSouce::ChaptersSouce() : CSource(NAME("Chapters Source"), NULL, __uuidof(this))
{
}

STDMETHODIMP ChaptersSouce::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

class CSubClock : public CUnknown, public ISubClock
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
		return
			QI(ISubClock)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	REFERENCE_TIME m_rt;

public:
	CSubClock() : CUnknown(NAME("CSubClock"), NULL) {
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
	if (m_iMediaLoadState == MLS_LOADED) __rt = GetPos(); \
 \
	if (__fs != State_Stopped) \
		SendMessage(WM_COMMAND, ID_PLAY_STOP); \


#define RestoreMediaState \
	if (m_iMediaLoadState == MLS_LOADED) \
	{ \
		SeekTo(__rt); \
 \
		if (__fs == State_Stopped) \
			SendMessage(WM_COMMAND, ID_PLAY_STOP); \
		else if (__fs == State_Paused) \
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE); \
		else if (__fs == State_Running) \
			SendMessage(WM_COMMAND, ID_PLAY_PLAY); \
	} \

static CString FormatStreamName(CString name, LCID lcid, int id)
{
	CString str;

	CString lcname = CString(name).MakeLower();
	if (lcname.Find(L" off") >= 0) {
		str = ResStr(IDS_AG_DISABLED);
	} else if (lcid == 0) {
		str.Format(ResStr(IDS_AG_UNKNOWN), id);
	} else {
		int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, str.GetBuffer(64), 64);
		str.ReleaseBufferSetLength(max(len - 1, 0));
	}

	CString lcstr = CString(str).MakeLower();
	if (str.IsEmpty() || lcname.Find(lcstr) >= 0) {
		str = name;
	} else if (!name.IsEmpty()) {
		str = CString(name) + L" (" + str + L")";
	}

	return str;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
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
	ON_WM_NCLBUTTONDOWN()
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
	ON_COMMAND_RANGE(ID_DVD_ANGLE_NEXT, ID_DVD_ANGLE_PREV, OnDvdAngle)
	ON_COMMAND_RANGE(ID_DVD_AUDIO_NEXT, ID_DVD_AUDIO_PREV, OnDvdAudio)
	ON_COMMAND_RANGE(ID_DVD_SUB_NEXT, ID_DVD_SUB_PREV, OnDvdSub)
	ON_COMMAND(ID_DVD_SUB_ONOFF, OnDvdSubOnOff)

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
	ON_COMMAND(ID_FILE_SAVE_IMAGE_AUTO, OnFileSaveImageAuto)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_IMAGE_AUTO, OnUpdateFileSaveImage)
	ON_COMMAND(ID_FILE_SAVE_THUMBNAILS, OnFileSaveThumbnails)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_THUMBNAILS, OnUpdateFileSaveThumbnails)
	ON_COMMAND(ID_FILE_LOAD_SUBTITLE, OnFileLoadSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_SUBTITLE, OnUpdateFileLoadSubtitle)
	ON_COMMAND(ID_FILE_SAVE_SUBTITLE, OnFileSaveSubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_SUBTITLE, OnUpdateFileSaveSubtitle)
	ON_COMMAND(ID_FILE_LOAD_AUDIO, OnFileLoadAudio)
	ON_COMMAND(ID_FILE_ISDB_SEARCH, OnFileISDBSearch)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_SEARCH, OnUpdateFileISDBSearch)
	ON_COMMAND(ID_FILE_ISDB_UPLOAD, OnFileISDBUpload)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_UPLOAD, OnUpdateFileISDBUpload)
	ON_COMMAND(ID_FILE_ISDB_DOWNLOAD, OnFileISDBDownload)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_DOWNLOAD, OnUpdateFileISDBDownload)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPLAYLIST, OnUpdateFileClose)
	ON_COMMAND(ID_FILE_CLOSEMEDIA, OnFileCloseMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEMEDIA, OnUpdateFileClose)

	ON_COMMAND(ID_VIEW_CAPTIONMENU, OnViewCaptionmenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTIONMENU, OnUpdateViewCaptionmenu)
	ON_COMMAND_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnViewControlBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnUpdateViewControlBar)
	ON_COMMAND(ID_VIEW_SUBRESYNC, OnViewSubresync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUBRESYNC, OnUpdateViewSubresync)
	ON_COMMAND(ID_VIEW_PLAYLIST, OnViewPlaylist)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAYLIST, OnUpdateViewPlaylist)
	ON_COMMAND(ID_VIEW_EDITLISTEDITOR, OnViewEditListEditor)
	ON_COMMAND(ID_EDL_IN, OnEDLIn)
	ON_UPDATE_COMMAND_UI(ID_EDL_IN, OnUpdateEDLIn)
	ON_COMMAND(ID_EDL_OUT, OnEDLOut)
	ON_UPDATE_COMMAND_UI(ID_EDL_OUT, OnUpdateEDLOut)
	ON_COMMAND(ID_EDL_NEWCLIP, OnEDLNewClip)
	ON_UPDATE_COMMAND_UI(ID_EDL_NEWCLIP, OnUpdateEDLNewClip)
	ON_COMMAND(ID_EDL_SAVE, OnEDLSave)
	ON_UPDATE_COMMAND_UI(ID_EDL_SAVE, OnUpdateEDLSave)
	ON_COMMAND(ID_VIEW_CAPTURE, OnViewCapture)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTURE, OnUpdateViewCapture)
	ON_COMMAND(ID_VIEW_SHADEREDITOR, OnViewShaderEditor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHADEREDITOR, OnUpdateViewShaderEditor)
	ON_COMMAND(ID_VIEW_PRESETS_MINIMAL, OnViewMinimal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_MINIMAL, OnUpdateViewMinimal)
	ON_COMMAND(ID_VIEW_PRESETS_COMPACT, OnViewCompact)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_COMPACT, OnUpdateViewCompact)
	ON_COMMAND(ID_VIEW_PRESETS_NORMAL, OnViewNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_NORMAL, OnUpdateViewNormal)
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
	ON_COMMAND_RANGE(ID_PANSCAN_ROTATEXP, ID_PANSCAN_ROTATEZM, OnViewRotate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANSCAN_ROTATEXP, ID_PANSCAN_ROTATEZM, OnUpdateViewRotate)
	ON_COMMAND_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnViewAspectRatio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnUpdateViewAspectRatio)
	ON_COMMAND(ID_ASPECTRATIO_NEXT, OnViewAspectRatioNext)
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

	ON_COMMAND(ID_VIEW_D3DFULLSCREEN, OnViewD3DFullScreen)
	ON_COMMAND(ID_VIEW_DISABLEDESKTOPCOMPOSITION, OnViewDisableDesktopComposition)
	ON_COMMAND(ID_VIEW_ALTERNATIVEVSYNC, OnViewAlternativeVSync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_D3DFULLSCREEN, OnUpdateViewD3DFullscreen)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DISABLEDESKTOPCOMPOSITION, OnUpdateViewDisableDesktopComposition)

	ON_COMMAND(ID_VIEW_RESET_DEFAULT, OnViewResetDefault)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RESET_DEFAULT, OnUpdateViewReset)

	ON_COMMAND(ID_VIEW_VSYNCOFFSET_INCREASE, OnViewVSyncOffsetIncrease)
	ON_COMMAND(ID_VIEW_VSYNCOFFSET_DECREASE, OnViewVSyncOffsetDecrease)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_TOGGLE, OnUpdateShaderToggle)
	ON_COMMAND(ID_SHADERS_TOGGLE, OnShaderToggle)
	ON_UPDATE_COMMAND_UI(ID_SHADERS_TOGGLE_SCREENSPACE, OnUpdateShaderToggleScreenSpace)
	ON_COMMAND(ID_SHADERS_TOGGLE_SCREENSPACE, OnShaderToggleScreenSpace)

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
	ON_COMMAND(ID_PLAY_PLAYPAUSE, OnPlayPlaypause)
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
	ON_COMMAND_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnPlayAudioOption)
	ON_COMMAND_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnPlayAudio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnUpdatePlayAudioOption)
	ON_COMMAND(ID_MENU_NAVIGATE_AUDIO, OnMenuNavAudio)
	ON_COMMAND(ID_MENU_NAVIGATE_SUBTITLES, OnMenuNavSubtitle)
	ON_COMMAND(ID_MENU_NAVIGATE_JUMPTO, OnMenuNavJumpTo)
	ON_COMMAND(ID_MENU_RECENT_FILES, OnMenuRecentFiles)

	ON_UPDATE_COMMAND_UI_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnUpdatePlayAudio)
	ON_COMMAND_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnPlaySubtitles)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnUpdatePlaySubtitles)

	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SUBP_SUBITEM_START, ID_NAVIGATE_SUBP_SUBITEM_END, OnUpdateNavMixSubtitles)

	ON_COMMAND_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnSelectStream)
	ON_COMMAND_RANGE(ID_VOLUME_UP, ID_VOLUME_MUTE, OnPlayVolume)
    ON_COMMAND_RANGE(ID_VOLUME_GAIN_INC, ID_VOLUME_GAIN_MAX, OnPlayVolumeGain)
	ON_COMMAND(ID_NORMALIZE, OnAutoVolumeControl)
	ON_UPDATE_COMMAND_UI(ID_NORMALIZE, OnUpdateNormalizeVolume)
	ON_COMMAND_RANGE(ID_COLOR_BRIGHTNESS_INC, ID_COLOR_RESET, OnPlayColor)
	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_DONOTHING, OnAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_DONOTHING, OnUpdateAfterplayback)
	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, OnAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, OnUpdateAfterplayback)

	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnNavigateSkip)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnUpdateNavigateSkip)
	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACKFILE, ID_NAVIGATE_SKIPFORWARDFILE, OnNavigateSkipFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACKFILE, ID_NAVIGATE_SKIPFORWARDFILE, OnUpdateNavigateSkipFile)
	ON_COMMAND_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnNavigateMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnUpdateNavigateMenu)
	ON_COMMAND_RANGE(ID_NAVIGATE_AUDIO_SUBITEM_START, ID_NAVIGATE_AUDIO_SUBITEM_END, OnNavigateAudioMix)

	ON_COMMAND_RANGE(ID_NAVIGATE_SUBP_SUBITEM_START, ID_NAVIGATE_SUBP_SUBITEM_END, OnNavigateSubpic)
	ON_COMMAND_RANGE(ID_NAVIGATE_ANGLE_SUBITEM_START, ID_NAVIGATE_ANGLE_SUBITEM_END, OnNavigateAngle)
	ON_COMMAND_RANGE(ID_NAVIGATE_CHAP_SUBITEM_START, ID_NAVIGATE_CHAP_SUBITEM_END, OnNavigateChapters)
	ON_COMMAND_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnNavigateMenuItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnUpdateNavigateMenuItem)
	ON_COMMAND(ID_NAVIGATE_TUNERSCAN, OnTunerScan)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_TUNERSCAN, OnUpdateTunerScan)

	ON_COMMAND(ID_FAVORITES_ADD, OnFavoritesAdd)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ADD, OnUpdateFavoritesAdd)
	ON_COMMAND(ID_FAVORITES_QUICKADDFAVORITE, OnFavoritesQuickAddFavorite)
	ON_COMMAND(ID_FAVORITES_ORGANIZE, OnFavoritesOrganize)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ORGANIZE, OnUpdateFavoritesOrganize)
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
	ON_COMMAND_RANGE(ID_SUB_POS_UP, IDS_SUB_POS_RESTORE, OnSubtitlePos)

	ON_WM_WTSSESSION_CHANGE()
END_MESSAGE_MAP()

#ifdef _DEBUG
const TCHAR *GetEventString(LONG evCode)
{
#define UNPACK_VALUE(VALUE) case VALUE: return _T( #VALUE );
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
	return _T("UNKNOWN");
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	m_iMediaLoadState(MLS_CLOSED),
	m_iPlaybackMode(PM_NONE),
	m_PlaybackRate(1.0),
	m_rtDurationOverride(-1),
	m_fFullScreen(false),
	m_fFirstFSAfterLaunchOnFS(false),
	m_fHideCursor(false),
	m_lastMouseMove(-1, -1),
	m_lastMouseMoveFullScreen(-1, -1),
	m_pLastBar(NULL),
	m_nLoops(0),
	m_iSubtitleSel(-1),
	m_ZoomX(1), m_ZoomY(1), m_PosX(0.5), m_PosY(0.5),
	m_AngleX(0), m_AngleY(0), m_AngleZ(0),
	m_fCustomGraph(false),
	m_fShockwaveGraph(false),
	m_fFrameSteppingActive(false),
	m_fEndOfStream(false),
	m_fCapturing(false),
	m_fLiveWM(false),
	m_fOpeningAborted(false),
	m_fBuffering(false),
	m_fileDropTarget(this),
	m_fTrayIcon(false),
	m_pFullscreenWnd(NULL),
	m_pVideoWnd(NULL),
	m_pOSDWnd(NULL),
	m_bRemainingTime(false),
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
	m_pTaskbarList(NULL),
	m_pGraphThread(NULL),
	m_bOpenedThruThread(false),
	m_nMenuHideTick(0),
	m_bWasSnapped(false),
	m_nWasSetDispMode(0),
	m_bIsBDPlay(FALSE),
	m_bClosingState(false),
	m_bUseSmartSeek(false),
	m_flastnID(0),
	bDVDMenuClicked(false),
	m_bfirstPlay(false),
	m_dwLastRun(0),
	IsMadVRExclusiveMode(false),
	m_fYoutubeThreadWork(TH_CLOSE),
	m_YoutubeThread(NULL),
	m_YoutubeCurrent(0),
	m_YoutubeTotal(0),
	m_pBFmadVR(NULL),
	m_hDWMAPI(0),
	m_hWtsLib(0),
	m_CaptureWndBitmap(NULL),
	m_ThumbCashedBitmap(NULL),
	m_DebugMonitor(::GetCurrentProcessId()),
	m_nSelSub2(-1),
	m_hNotifyRenderThread(NULL),
	m_hStopNotifyRenderThreadEvent(NULL),
	m_hRefreshNotifyRenderThreadEvent(NULL),
	m_nMainFilterId(NULL),
	m_hGraphThreadEventOpen(FALSE, TRUE),
	m_hGraphThreadEventClose(FALSE, TRUE),
	m_DwmSetWindowAttributeFnc(NULL),
	m_DwmSetIconicThumbnailFnc(NULL),
	m_DwmSetIconicLivePreviewBitmapFnc(NULL),
	m_DwmInvalidateIconicBitmapsFnc(NULL),
	m_OSD(this),
	m_wndToolBar(this),
	m_wndSeekBar(this)
{
	m_Lcd.SetVolumeRange(0, 100);
	m_LastSaveTime.QuadPart = 0;
	m_ThumbCashedSize.SetSize(0, 0);
}

CMainFrame::~CMainFrame()
{
	//m_owner.DestroyWindow();
	//delete m_pFullscreenWnd; // double delete see CMainFrame::OnDestroy
}

DWORD WINAPI CMainFrame::NotifyRenderThreadEntryPoint(LPVOID lpParameter)
{
	return ((CMainFrame*)lpParameter)->NotifyRenderThread();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	m_popupMenu.LoadMenu(IDR_POPUP);
	m_popupMainMenu.LoadMenu(IDR_POPUPMAIN);

	AppSettings& s = AfxGetAppSettings();

	HDC hdc = ::GetDC(NULL);
	s.scalefont = 1.0 * GetDeviceCaps(hdc, LOGPIXELSY) / 96.0;
	::ReleaseDC(0, hdc);

	// create a Main View Window
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						  CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL)) {
		TRACE(_T("Failed to create Main View Window\n"));
		return -1;
	}

	// Should never be RTLed
	m_wndView.ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	// Create FlyBar Window
	CreateFlyBar();
	// Create OSD Window
	CreateOSDBar();

	// Create Preview Window
	DWORD style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	if (!m_wndPreView.CreateEx(WS_EX_TOPMOST, AfxRegisterWndClass(0), NULL, style, CRect(0, 0, 160, 109), this, 0, NULL)) {
		TRACE(_T("Failed to create Preview Window\n"));
		m_wndView.DestroyWindow();
		return -1;
	} else {
		m_wndPreView.ShowWindow(SW_HIDE);
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
		TRACE0("Failed to create all control bars\n");
		return -1;      // fail to create
	}

	m_pFullscreenWnd = DNew CFullscreenWnd(this);

	m_bars.AddTail(&m_wndSeekBar);
	m_bars.AddTail(&m_wndToolBar);
	m_bars.AddTail(&m_wndInfoBar);
	m_bars.AddTail(&m_wndStatsBar);
	m_bars.AddTail(&m_wndStatusBar);

	m_wndSeekBar.Enable(false);

	// dockable bars

	EnableDocking(CBRS_ALIGN_ANY);

	m_dockingbars.RemoveAll();

	m_wndSubresyncBar.Create(this, AFX_IDW_DOCKBAR_TOP, &m_csSubLock);
	m_wndSubresyncBar.SetBarStyle(m_wndSubresyncBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSubresyncBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndSubresyncBar.SetHeight(200);
	m_dockingbars.AddTail(&m_wndSubresyncBar);

	m_wndPlaylistBar.Create(this, AFX_IDW_DOCKBAR_BOTTOM);
	m_wndPlaylistBar.SetBarStyle(m_wndPlaylistBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPlaylistBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndPlaylistBar.SetHeight(100);
	m_dockingbars.AddTail(&m_wndPlaylistBar);
	m_wndPlaylistBar.LoadPlaylist(GetRecentFile());

	m_wndEditListEditor.Create(this, AFX_IDW_DOCKBAR_RIGHT);
	m_wndEditListEditor.SetBarStyle(m_wndEditListEditor.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndEditListEditor.EnableDocking(CBRS_ALIGN_ANY);
	m_dockingbars.AddTail(&m_wndEditListEditor);
	m_wndEditListEditor.SetHeight(100);

	m_wndCaptureBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndCaptureBar.SetBarStyle(m_wndCaptureBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCaptureBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.AddTail(&m_wndCaptureBar);

	m_wndNavigationBar.Create(this, AFX_IDW_DOCKBAR_LEFT);
	m_wndNavigationBar.SetBarStyle(m_wndNavigationBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndNavigationBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	m_dockingbars.AddTail(&m_wndNavigationBar);

	m_wndShaderEditorBar.Create(this, AFX_IDW_DOCKBAR_TOP);
	m_wndShaderEditorBar.SetBarStyle(m_wndShaderEditorBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndShaderEditorBar.EnableDocking(CBRS_ALIGN_ANY);
	m_dockingbars.AddTail(&m_wndShaderEditorBar);

	// Hide all dockable bars by default
	POSITION pos = m_dockingbars.GetHeadPosition();
	while (pos) {
		m_dockingbars.GetNext(pos)->ShowWindow(SW_HIDE);
	}

	m_fileDropTarget.Register(this);

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);

	ShowControls(s.nCS);

	SetAlwaysOnTop(s.iOnTop);

	ShowTrayIcon(s.fTrayIcon);

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

	// reload Shaders
	{
		CString		strList = s.strShaderList;
		CString		strRes;
		int			curPos= 0;

		strRes = strList.Tokenize (_T("|"), curPos);
		while (strRes.GetLength() > 0) {
			m_shaderlabels.AddTail (strRes);
			strRes = strList.Tokenize(_T("|"),curPos);
		}
	}
	{
		CString		strList = s.strShaderListScreenSpace;
		CString		strRes;
		int			curPos= 0;

		strRes = strList.Tokenize (_T("|"), curPos);
		while (strRes.GetLength() > 0) {
			m_shaderlabelsScreenSpace.AddTail (strRes);
			strRes = strList.Tokenize(_T("|"),curPos);
		}
	}

	m_bToggleShader = s.fToggleShader;
	m_bToggleShaderScreenSpace = s.fToggleShaderScreenSpace;

#ifdef _WIN64
	m_strTitle.Format(L"%s x64 - v%s", ResStr(IDR_MAINFRAME), CString(MPC_VERSION_STR));
#else
	m_strTitle.Format(L"%s - v%s", ResStr(IDR_MAINFRAME), CString(MPC_VERSION_STR));
#endif
#if (MPC_VERSION_STATUS == 0)
	m_strTitle.AppendFormat(L" (build %d) beta",  MPC_VERSION_REV);
#endif
#if ENABLE_2PASS_RESIZE
	m_strTitle.Append(L" 2passresize"); // test build
#endif

	SetWindowText(m_strTitle);
	m_Lcd.SetMediaTitle(m_strTitle);

	m_hWnd_toolbar = m_wndToolBar.GetSafeHwnd();

	WTSRegisterSessionNotification();

	CStringW strFS = s.strFullScreenMonitor;
	GetCurDispMode(s.dm_def, strFS);

	if (IsWinSevenOrLater()) {
		m_hDWMAPI = LoadLibrary(L"dwmapi.dll");
		if (m_hDWMAPI) {
			(FARPROC &)m_DwmSetWindowAttributeFnc			= GetProcAddress(m_hDWMAPI, "DwmSetWindowAttribute");
			(FARPROC &)m_DwmSetIconicThumbnailFnc			= GetProcAddress(m_hDWMAPI, "DwmSetIconicThumbnail");
			(FARPROC &)m_DwmSetIconicLivePreviewBitmapFnc	= GetProcAddress(m_hDWMAPI, "DwmSetIconicLivePreviewBitmap");
			(FARPROC &)m_DwmInvalidateIconicBitmapsFnc		= GetProcAddress(m_hDWMAPI, "DwmInvalidateIconicBitmaps");
		}
	}

	// Windows 8 - if app is not pinned on the taskbar, it's not receive "TaskbarButtonCreated" message. Bug ???
	if (IsWinEightOrLater()) {
		CreateThumbnailToolbar();
	}

	m_DiskImage.Init();

	m_hStopNotifyRenderThreadEvent		= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRefreshNotifyRenderThreadEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);

	DWORD ThreadId;
	m_hNotifyRenderThread = ::CreateThread(NULL, 0, NotifyRenderThreadEntryPoint, (LPVOID)this, 0, &ThreadId);
	if (m_hNotifyRenderThread) {
		SetThreadPriority(m_hNotifyRenderThread, THREAD_PRIORITY_LOWEST);
	}

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
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_EXIT, 0, (LPARAM)&e);
		if (!e.Wait(5000)) {
			TRACE(_T("ERROR: Must call TerminateThread() on CMainFrame::m_pGraphThread->m_hThread\n"));
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

	if (AfxGetAppSettings().fReset) {
		ShellExecute(NULL, _T("open"), GetProgramPath(TRUE), _T("/reset"), NULL, SW_SHOWNORMAL);
	}

	if (m_hNotifyRenderThread) {
		SetEvent(m_hStopNotifyRenderThreadEvent);
		if (WaitForSingleObject(m_hNotifyRenderThread, 3000) == WAIT_TIMEOUT) {
			TerminateThread(m_hNotifyRenderThread, 0xDEAD);
		}

		SAFE_CLOSE_HANDLE(m_hNotifyRenderThread);
	}

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();

	SAFE_CLOSE_HANDLE(m_hStopNotifyRenderThreadEvent);
	SAFE_CLOSE_HANDLE(m_hRefreshNotifyRenderThreadEvent);

	__super::OnDestroy();
}

void CMainFrame::OnClose()
{
	DbgLog((LOG_TRACE, 3, L"CMainFrame::OnClose() : start"));

	m_bClosingState = true;

	KillTimer(TIMER_FLYBARWINDOWHIDER);
	KillTimer(TIMER_EXCLUSIVEBARHIDER);
	DestroyOSDBar();
	DestroyFlyBar();

	AppSettings& s = AfxGetAppSettings();

	// save shaders list
	{
		POSITION	pos;
		CString		strList;

		pos = m_shaderlabels.GetHeadPosition();
		while (pos) {
			strList += m_shaderlabels.GetAt (pos) + "|";
			m_dockingbars.GetNext(pos);
		}
		s.strShaderList = strList;
	}
	{
		POSITION	pos;
		CString		strList;

		pos = m_shaderlabelsScreenSpace.GetHeadPosition();
		while (pos) {
			strList += m_shaderlabelsScreenSpace.GetAt (pos) + "|";
			m_dockingbars.GetNext(pos);
		}
		s.strShaderListScreenSpace = strList;
	}

	s.fToggleShader = m_bToggleShader;
	s.fToggleShaderScreenSpace = m_bToggleShaderScreenSpace;

	s.dZoomX = m_ZoomX;
	s.dZoomY = m_ZoomY;

	m_wndPlaylistBar.SavePlaylist();

	SaveControlBars();

	CloseMedia();

	ShowWindow(SW_HIDE);

	s.WinLircClient.DisConnect();
	s.UIceClient.DisConnect();

	if (s.AutoChangeFullscrRes.bSetGlobal && s.AutoChangeFullscrRes.bEnabled == 2) {
		SetDispMode(s.dm_def, s.strFullScreenMonitor);
	}

	if (m_hDWMAPI) {
		FreeLibrary(m_hDWMAPI);
	}

	m_InternalImage.Destroy();
	m_InternalImageSmall.Destroy();
	if (m_ThumbCashedBitmap) {
		::DeleteObject(m_ThumbCashedBitmap);
	}

	while (s.slTMPFilesList.GetCount()) {
		CString tmpFileName = s.slTMPFilesList.RemoveHead();
		if (::PathFileExists(tmpFileName)) {
			DeleteFile(tmpFileName);
		}
	}

	DbgLog((LOG_TRACE, 3, L"CMainFrame::OnClose() : end"));
	__super::OnClose();
}

DROPEFFECT CMainFrame::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

DROPEFFECT CMainFrame::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return (pDataObject->IsDataAvailable(CF_URL) || pDataObject->IsDataAvailable(CF_HDROP)) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

BOOL CMainFrame::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	BOOL bResult = FALSE;
	CAtlList<CString> slFiles;

    if (pDataObject->IsDataAvailable(CF_HDROP)) {
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_HDROP)) {
			if (HDROP hDrop = (HDROP)GlobalLock(hGlobal)) {
				UINT nFiles = ::DragQueryFile(hDrop, UINT_MAX, NULL, 0);
				for (UINT iFile = 0; iFile < nFiles; iFile++) {
					CString fn;
					fn.ReleaseBuffer(::DragQueryFile(hDrop, iFile, fn.GetBuffer(MAX_PATH), MAX_PATH));
					slFiles.AddTail(fn);
				}
				::DragFinish(hDrop);
				DropFiles(slFiles);
				bResult = TRUE;
			}
			GlobalUnlock(hGlobal);
		}
	} else if (pDataObject->IsDataAvailable(CF_URL)) {
		FORMATETC fmt = {CF_URL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		if (HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_URL, &fmt)) {
			LPCSTR pText = (LPCSTR)GlobalLock(hGlobal);
			if (AfxIsValidString(pText)) {
				CAtlList<CString> sl;
				sl.AddTail(pText);
				DropFiles(sl);
				GlobalUnlock(hGlobal);
				bResult = TRUE;
			}
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
	return NULL;
}

void CMainFrame::RestoreControlBars()
{
	POSITION pos = m_dockingbars.GetHeadPosition();
	while (pos) {
		CPlayerBar* pBar = dynamic_cast<CPlayerBar*>(m_dockingbars.GetNext(pos));

		if (pBar) {
			pBar->LoadState(this);
		}
	}
}

void CMainFrame::SaveControlBars()
{
	POSITION pos = m_dockingbars.GetHeadPosition();
	while (pos) {
		CPlayerBar* pBar = dynamic_cast<CPlayerBar*>(m_dockingbars.GetNext(pos));

		if (pBar) {
			pBar->SaveState();
		}
	}
}

LRESULT CMainFrame::OnTaskBarRestart(WPARAM, LPARAM)
{
	m_fTrayIcon = false;
	ShowTrayIcon(AfxGetAppSettings().fTrayIcon);

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
			PostMessage(WM_COMMAND, ID_FILE_OPENMEDIA);
			break;

		case WM_RBUTTONDOWN: {
			POINT p;
			GetCursorPos(&p);
			SetForegroundWindow();
			m_popupMainMenu.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NOANIMATION, p.x, p.y, GetModalParent());
			PostMessage(WM_NULL);
			break;
		}

		case WM_MOUSEMOVE: {
			CString str;
			GetWindowText(str);
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
		DbgLog((LOG_TRACE, 3, L"CMainFrame: block autopay for %C:\\", (PCHAR)wParam + 'A'));
		return TRUE;
	} else {
		DbgLog((LOG_TRACE, 3, L"CMainFrame: allow autopay for %C:\\", (PCHAR)wParam + 'A'));
		return FALSE;
	}
}

void CMainFrame::ShowTrayIcon(bool fShow)
{
	BOOL bWasVisible = ShowWindow(SW_HIDE);
	NOTIFYICONDATA tnid;

	ZeroMemory(&tnid, sizeof(NOTIFYICONDATA));
	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = m_hWnd;
	tnid.uID = IDR_MAINFRAME;

	if (fShow) {
		if (!m_fTrayIcon) {
			tnid.hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
			tnid.uCallbackMessage = WM_NOTIFYICON;
			StringCchCopy(tnid.szTip, _countof(tnid.szTip), TEXT("MPC-BE"));
			Shell_NotifyIcon(NIM_ADD, &tnid);

			m_fTrayIcon = true;
		}
	} else {
		if (m_fTrayIcon) {
			Shell_NotifyIcon(NIM_DELETE, &tnid);

			m_fTrayIcon = false;
		}
	}

	if (bWasVisible) {
		ShowWindow(SW_SHOW);
	}
}

void CMainFrame::SetTrayTip(CString str)
{
	NOTIFYICONDATA tnid;
	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = m_hWnd;
	tnid.uID = IDR_MAINFRAME;
	tnid.uFlags = NIF_TIP;
	StringCchCopy(tnid.szTip, _countof(tnid.szTip), str);
	Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!__super::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	cs.lpszClass = _T(MPC_WND_CLASS_NAME);

	return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		/*		if (m_fShockwaveGraph
				&& (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT
					|| pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN))
					return FALSE;
		*/
		if (pMsg->wParam == VK_ESCAPE) {
			bool fEscapeNotAssigned = !AssignedToCmd(VK_ESCAPE, m_fFullScreen, false);

			if (fEscapeNotAssigned) {
				if (m_fFullScreen) {
					OnViewFullscreen();
					if (m_iMediaLoadState == MLS_LOADED) {
						PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
					}
					return TRUE;
				//} else if (IsCaptionHidden()) {
				//	PostMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
				//	return TRUE;
				}
			}
		} else if (pMsg->wParam == VK_LEFT && pAMTuner) {
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			return TRUE;
		} else if (pMsg->wParam == VK_RIGHT && pAMTuner) {
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	__super::RecalcLayout(bNotify);

	m_wndSeekBar.HideToolTip();

	CRect r;
	GetWindowRect(&r);
	MINMAXINFO mmi;
	memset(&mmi, 0, sizeof(mmi));
	SendMessage(WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
	r |= CRect(r.TopLeft(), CSize(r.Width(), mmi.ptMinTrackSize.y));
	MoveWindow(&r);
	FlyBarSetPos();
	OSDBarSetPos();
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

	POSITION pos = m_bars.GetHeadPosition();
	while (pos) {
		if (m_bars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) {
			return TRUE;
		}
	}

	pos = m_dockingbars.GetHeadPosition();
	while (pos) {
		if (m_dockingbars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) {
			return TRUE;
		}
	}

	// otherwise, do default handling
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	AppSettings &s	= AfxGetAppSettings();
	DWORD style		= GetStyle();

	lpMMI->ptMinTrackSize.x = 16;
	if (!IsMenuHidden()) {
		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		// Calculate menu's horizontal length in pixels
		lpMMI->ptMinTrackSize.x = GetSystemMetrics(SM_CYMENU)/2; //free space after menu
		CRect r;
		for (int i = 0; ::GetMenuItemRect(m_hWnd, mbi.hMenu, i, &r); i++) {
			lpMMI->ptMinTrackSize.x += r.Width();
		}
	}
	if (IsWindow(m_wndToolBar.m_hWnd) && m_wndToolBar.IsVisible()) {
		lpMMI->ptMinTrackSize.x = max(m_wndToolBar.GetMinWidth(), lpMMI->ptMinTrackSize.x);
	}

	lpMMI->ptMinTrackSize.y = 0;
	if (style & WS_CAPTION ) {
		lpMMI->ptMinTrackSize.y += GetSystemMetrics(SM_CYCAPTION);
		if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
			lpMMI->ptMinTrackSize.y += GetSystemMetrics(SM_CYMENU);    //(mbi.rcBar.bottom - mbi.rcBar.top);
		}
		//else MODE_HIDEMENU
	}

	if (style & WS_THICKFRAME) {
		lpMMI->ptMinTrackSize.x += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
		lpMMI->ptMinTrackSize.y += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
		if ( (style & WS_CAPTION) == 0 ) {
			lpMMI->ptMinTrackSize.x -= 2;
			lpMMI->ptMinTrackSize.y -= 2;
		}
	}

	POSITION pos = m_bars.GetHeadPosition();
	while (pos) {
		CControlBar *pCB = m_bars.GetNext( pos );
		if (!IsWindow(pCB->m_hWnd) || !pCB->IsVisible()) {
			continue;
		}
		lpMMI->ptMinTrackSize.y += pCB->CalcFixedLayout(TRUE, TRUE).cy;
	}

	pos = m_dockingbars.GetHeadPosition();
	while (pos) {
		CSizingControlBar *pCB = m_dockingbars.GetNext( pos );
		if (IsWindow(pCB->m_hWnd) && pCB->IsWindowVisible() && !pCB->IsFloating()) {
			lpMMI->ptMinTrackSize.y += pCB->CalcFixedLayout(TRUE, TRUE).cy - 2;    // 2 is a magic value from CSizingControlBar class, i guess this should be GetSystemMetrics( SM_CXBORDER ) or similar
		}
	}
	if (lpMMI->ptMinTrackSize.y<16) {
		lpMMI->ptMinTrackSize.y = 16;
	}

	FlyBarSetPos();
	OSDBarSetPos();

	__super::OnGetMinMaxInfo(lpMMI);
}

void CMainFrame::CreateFlyBar()
{
	if (AfxGetAppSettings().fFlybar) {
		if (SUCCEEDED(m_wndFlyBar.Create(this))) {
			SetTimer(TIMER_FLYBARWINDOWHIDER, 250, NULL);
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
		if (m_fFullScreen && boolVal) {
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

	const AppSettings& s = AfxGetAppSettings();
	if (s.iCaptionMenuMode == MODE_FRAMEONLY || s.iCaptionMenuMode == MODE_BORDERLESS || m_fFullScreen) {

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
		::PostMessage(m_OSD.m_hWnd, WM_OSD_DRAW, WPARAM(0), LPARAM(0));
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

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	RECT rcWork = mi.rcWork;

	if (IsZoomed() // window is maximized
		|| (rcWindow.top == rcWork.top && rcWindow.bottom == rcWork.bottom) // window is aero snapped (???)
		|| m_fFullScreen) { // window is fullscreen

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

BOOL CMainFrame::isSnapClose( int a, int b )
{
	snap_Margin = GetSystemMetrics(SM_CYCAPTION);
	return (abs( a - b ) < snap_Margin);
}

void CMainFrame::OnMove(int x, int y)
{
	__super::OnMove(x, y);

	//MoveVideoWindow(); // This isn't needed, based on my limited tests. If it is needed then please add a description the scenario(s) where it is needed.
	m_wndView.Invalidate();
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if (!m_fFirstFSAfterLaunchOnFS && !m_fFullScreen && wp.flags != WPF_RESTORETOMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED) {
		GetWindowRect(AfxGetAppSettings().rcLastWindowPos);
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

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST), &mi);
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
	bool fCtrl = !!(GetAsyncKeyState(VK_CONTROL)&0x80000000);

	POINT cur_pos;
	GetCursorPos(&cur_pos);

	if (m_bWndZoomed){
		ClipRectToMonitor(pRect);
		SetWindowPos(NULL, pRect->left, pRect->top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		m_bWndZoomed = false;
		snap_x = cur_pos.x - pRect->left;
		snap_y = cur_pos.y - pRect->top;

		return;
	}

	if (AfxGetAppSettings().fSnapToDesktopEdges && !fCtrl) {

		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		RECT rcWork = mi.rcWork;
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

	if (m_OSD && m_OSD.IsWindowVisible() && AfxGetAppSettings().IsD3DFullscreen()) {
		m_OSD.OnSize(nType, cx, cy);
	}
	if (nType == SIZE_RESTORED && m_fTrayIcon) {
		ShowWindow(SW_SHOW);
	}

	if (!m_fFirstFSAfterLaunchOnFS && IsWindowVisible() && !m_fFullScreen) {
		AppSettings& s = AfxGetAppSettings();
		if (nType != SIZE_MAXIMIZED && nType != SIZE_MINIMIZED) {
			GetWindowRect(s.rcLastWindowPos);
		}
		s.nLastWindowType = nType;
	}

	// maximized window in MODE_FRAMEONLY|MODE_BORDERLESS is not correct;
	AppSettings& s = AfxGetAppSettings();
	if (nType == SIZE_MAXIMIZED && (s.iCaptionMenuMode == MODE_FRAMEONLY || s.iCaptionMenuMode == MODE_BORDERLESS)) {
		CRect r; GetWindowRect(&r);
		MONITORINFO mi; mi.cbSize = sizeof(mi);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		RECT rcWork = mi.rcWork;
		r.bottom = rcWork.bottom;
		if (s.iCaptionMenuMode == MODE_FRAMEONLY) {
			SetWindowPos(NULL, r.left, r.top, r.right - r.left, r.bottom - r.top + GetSystemMetrics(SM_CYSIZEFRAME), SWP_NOZORDER | SWP_NOACTIVATE);
		} else if (s.iCaptionMenuMode == MODE_BORDERLESS) {
			SetWindowPos(NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	if (nType != SIZE_MINIMIZED) {
		FlyBarSetPos();
		OSDBarSetPos();
	}
}

void CMainFrame::OnSizing(UINT fwSide, LPRECT pRect)
{
	__super::OnSizing(fwSide, pRect);

	AppSettings& s = AfxGetAppSettings();

	bool fCtrl = !!(GetAsyncKeyState(VK_CONTROL)&0x80000000);

	if (m_iMediaLoadState != MLS_LOADED || m_fFullScreen
			|| s.iDefaultVideoSize == DVS_STRETCH
			|| (fCtrl == s.fLimitWindowProportions)) {	// remember that fCtrl is initialized with !!whatever(), same with fLimitWindowProportions
		return;
	}

	CSize wsize(pRect->right - pRect->left, pRect->bottom - pRect->top);
	CSize vsize = GetVideoSize();
	CSize fsize(0, 0);

	if (!vsize.cx || !vsize.cy) {
		return;
	}

	// TODO
	{
		DWORD style = GetStyle();

		// This doesn't give correct menu pixel size
		//MENUBARINFO mbi;
		//memset(&mbi, 0, sizeof(mbi));
		//mbi.cbSize = sizeof(mbi);
		//::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		if (style & WS_THICKFRAME) {
			fsize.cx += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
			fsize.cy += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
			if ( (style & WS_CAPTION) == 0 ) {
				fsize.cx -= 2;
				fsize.cy -= 2;
			}
		}

		if ( style & WS_CAPTION ) {
			fsize.cy += GetSystemMetrics( SM_CYCAPTION );
			if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
				fsize.cy += GetSystemMetrics( SM_CYMENU );    //mbi.rcBar.bottom - mbi.rcBar.top;
			}
			//else MODE_HIDEMENU
		}

		POSITION pos = m_bars.GetHeadPosition();
		while ( pos ) {
			CControlBar * pCB = m_bars.GetNext( pos );
			if ( IsWindow(pCB->m_hWnd) && pCB->IsVisible() ) {
				fsize.cy += pCB->CalcFixedLayout(TRUE, TRUE).cy;
			}
		}

		pos = m_dockingbars.GetHeadPosition();
		while ( pos ) {
			CSizingControlBar *pCB = m_dockingbars.GetNext( pos );

			if ( IsWindow(pCB->m_hWnd) && pCB->IsWindowVisible() && !pCB->IsFloating() ) {
				if ( pCB->IsHorzDocked() ) {
					fsize.cy += pCB->CalcFixedLayout(TRUE, TRUE).cy - 2;
				} else if ( pCB->IsVertDocked() ) {
					fsize.cx += pCB->CalcFixedLayout(TRUE, FALSE).cx;
				}
			}
		}
	}

	wsize -= fsize;

	bool fWider = wsize.cy < wsize.cx;

	wsize.SetSize(
		wsize.cy * vsize.cx / vsize.cy,
		wsize.cx * vsize.cy / vsize.cx);

	wsize += fsize;

	if (fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM || (!fWider && (fwSide == WMSZ_TOPRIGHT || fwSide == WMSZ_BOTTOMRIGHT))) {
		pRect->right = pRect->left + wsize.cx;
	} else if (fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT || (fWider && (fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_BOTTOMRIGHT))) {
		pRect->bottom = pRect->top + wsize.cy;
	} else if (!fWider && (fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_BOTTOMLEFT)) {
		pRect->left = pRect->right - wsize.cx;
	} else if (fWider && (fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT)) {
		pRect->top = pRect->bottom - wsize.cy;
	}
	FlyBarSetPos();
	OSDBarSetPos();
}

void CMainFrame::OnDisplayChange() // untested, not sure if it's working...
{
	TRACE(_T("*** CMainFrame::OnDisplayChange()\n"));

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);
	if (m_pFullscreenWnd && m_pFullscreenWnd->IsWindow()) {
		MONITORINFO		MonitorInfo;
		HMONITOR		hMonitor;
		ZeroMemory (&MonitorInfo, sizeof(MonitorInfo));
		MonitorInfo.cbSize	= sizeof(MonitorInfo);
		hMonitor			= MonitorFromWindow (m_pFullscreenWnd->m_hWnd, 0);
		if (GetMonitorInfo (hMonitor, &MonitorInfo)) {
			CRect MonitorRect = CRect (MonitorInfo.rcMonitor);
			m_fullWndSize.cx	= MonitorRect.Width();
			m_fullWndSize.cy	= MonitorRect.Height();
			m_pFullscreenWnd->SetWindowPos (NULL,
											MonitorRect.left,
											MonitorRect.top,
											MonitorRect.Width(),
											MonitorRect.Height(), SWP_NOZORDER);
			MoveVideoWindow();
		}
	}

	AppSettings& s = AfxGetAppSettings();
	if (s.iDSVideoRendererType != VIDRNDT_DS_MADVR && s.iDSVideoRendererType != VIDRNDT_DS_DXR) {
		IDirect3D9* pD3D9 = NULL;
		DWORD nPCIVendor = 0;

		pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
		if (pD3D9) {
			D3DADAPTER_IDENTIFIER9 adapterIdentifier;
			if (pD3D9->GetAdapterIdentifier(GetAdapter(pD3D9, m_hWnd), 0, &adapterIdentifier) == S_OK) {
				nPCIVendor = adapterIdentifier.VendorId;
			}
			pD3D9->Release();
		}

		if (nPCIVendor == 0x8086) { // Disable ResetDevice for Intel, until can fix ...
			return;
		}
	}

	if (m_iMediaLoadState == MLS_LOADED) {
		if (m_pGraphThread) {
			m_pGraphThread->PostThreadMessage(CGraphThread::TM_DISPLAY_CHANGE, 0, 0);
		} else {
			DisplayChange();
		}
	}
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	// Only stop screensaver if video playing; allow for audio only
	if ((GetMediaState() == State_Running && !m_bAudioOnly) && (((nID & 0xFFF0) == SC_SCREENSAVE) || ((nID & 0xFFF0) == SC_MONITORPOWER))) {
		TRACE(_T("SC_SCREENSAVE, nID = %d, lParam = %d\n"), nID, lParam);
		return;
	} else if ((nID & 0xFFF0) == SC_MINIMIZE && m_fTrayIcon) {
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
	} else if ((nID & 0xFFF0) == SC_MAXIMIZE && m_fFullScreen) {
		ToggleFullscreen(true, true);
	}

	__super::OnSysCommand(nID, lParam);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	__super::OnActivateApp(bActive, dwThreadID);

	if (AfxGetAppSettings().iOnTop || !AfxGetAppSettings().fExitFullScreenAtFocusLost) {
		return;
	}

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

	if (!bActive && (mi.dwFlags & MONITORINFOF_PRIMARY) && m_fFullScreen && m_iMediaLoadState == MLS_LOADED) {
		bool fExitFullscreen = true;

		if (CWnd* pWnd = GetForegroundWindow()) {
			HMONITOR hMonitor1 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			HMONITOR hMonitor2 = MonitorFromWindow(pWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
			CMonitors monitors;
			if (hMonitor1 && hMonitor2 && ((hMonitor1 != hMonitor2) || (monitors.GetCount()>1))) {
				fExitFullscreen = false;
			}

			CString title;
			pWnd->GetWindowText(title);

			CString module;

			DWORD pid;
			GetWindowThreadProcessId(pWnd->m_hWnd, &pid);

			if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
				HMODULE hMod;
				DWORD cbNeeded;

				if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
					module.ReleaseBufferSetLength(GetModuleFileNameEx(hProcess, hMod, module.GetBuffer(_MAX_PATH), _MAX_PATH));
				}

				CloseHandle(hProcess);
			}

			CString str;
			str.Format(ResStr(IDS_MAINFRM_2), GetFileOnly(module).MakeLower(), title);
			SendStatusMessage(str, 5000);
		}

		if (fExitFullscreen) {
			OnViewFullscreen();
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
		AppSettings& s = AfxGetAppSettings();

		BOOL fRet = FALSE;

		POSITION pos = s.wmcmds.GetHeadPosition();
		while (pos) {
			wmcmd& wc = s.wmcmds.GetNext(pos);
			if (wc.appcmd == cmd && TRUE == SendMessage(WM_COMMAND, wc.cmd)) {
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
	AppSettings&	s			= AfxGetAppSettings();
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
			POSITION pos = s.wmcmds.GetHeadPosition();
			while (pos) {
				wmcmd& wc = s.wmcmds.GetNext(pos);
				if (wc.appcmd == nMceCmd) {
					SendMessage(WM_COMMAND, wc.cmd);
					break;
				}
			}
			break;
	}
}

LRESULT CMainFrame::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	AppSettings& s = AfxGetAppSettings();
	BOOL fRet = FALSE;

	if (GetActiveWindow() == this || s.fGlobalMedia == TRUE) {
		POSITION pos = s.wmcmds.GetHeadPosition();

		while (pos) {
			wmcmd& wc = s.wmcmds.GetNext(pos);
			if (wc.appcmd == wParam && TRUE == SendMessage(WM_COMMAND, wc.cmd)) {
				fRet = TRUE;
			}
		}
	}

	return fRet;
}

bool g_bNoDuration				= false;
bool g_bExternalSubtitleTime	= false;

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent) {

		case TIMER_EXCLUSIVEBARHIDER:
			if (m_pFullscreenWnd->IsWindow()) {
				CPoint p;
				GetCursorPos(&p);
				CWnd* pWnd = WindowFromPoint(p);
				bool bD3DFS = AfxGetAppSettings().fIsFSWindow;
				if (pWnd == m_pFullscreenWnd && GetCursor() != NULL) {
					if (!bD3DFS) {
						AfxGetAppSettings().fIsFSWindow = true;
					}
				} else {
					if (bD3DFS) {
						AfxGetAppSettings().fIsFSWindow = false;
						m_OSD.HideExclusiveBars();
					}
				}
			}
			break;
		case TIMER_FLYBARWINDOWHIDER:

			if (m_wndView &&
						(AfxGetAppSettings().iCaptionMenuMode == MODE_FRAMEONLY
						|| AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS
						|| m_fFullScreen)) {

				CPoint p;
				GetCursorPos(&p);

				CRect r_wndView, r_wndFlybar, r_ShowFlybar;
				m_wndView.GetWindowRect(&r_wndView);
				m_wndFlyBar.GetWindowRect(&r_wndFlybar);

				r_ShowFlybar = r_wndView;
				r_ShowFlybar.bottom = r_wndFlybar.bottom + r_wndFlybar.Height();

				if (FlyBarSetPos() == 0) break;

				if (r_ShowFlybar.PtInRect(p) && !m_fHideCursor && !m_wndFlyBar.IsWindowVisible()) {
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
			if (m_iMediaLoadState == MLS_LOADED) {

				if (m_fFullScreen) {
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
						m_pMS->GetCurrentPosition(&rtNow);
						m_pMS->GetDuration(&rtDur);

						// autosave subtitle sync after play
						if ((m_nCurSubtitle >= 0) && (m_rtCurSubPos != rtNow)) {
							if (m_lSubtitleShift != 0) {
								if (m_wndSubresyncBar.SaveToDisk()) {
									m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_AG_SUBTITLES_SAVED), 500);
								} else {
									m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_MAINFRM_4));
								}
							}
							m_nCurSubtitle		= -1;
							m_lSubtitleShift	= 0;
						}

						if (!m_fEndOfStream) {
							AppSettings& s = AfxGetAppSettings();
							FILE_POSITION*	FilePosition = s.CurrentFilePosition();
							if (FilePosition) {
								FilePosition->llPosition = rtNow;

								LARGE_INTEGER time;
								QueryPerformanceCounter(&time);
								LARGE_INTEGER freq;
								QueryPerformanceFrequency(&freq);
								if ((time.QuadPart - m_LastSaveTime.QuadPart) >= 30 * freq.QuadPart) { // save every half of minute
									m_LastSaveTime = time;
									if (s.fKeepHistory && s.fRememberFilePos) {
										s.SaveCurrentFilePosition();
									}
								}
							}
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
				m_wndSeekBar.SetRange(0, rtDur);
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
			if (m_iMediaLoadState == MLS_LOADED) {
				if (AfxGetAppSettings().nCS < CS_STATUSBAR) {
					AfxGetAppSettings().bStatusBarIsVisible = false;
				} else {
					AfxGetAppSettings().bStatusBarIsVisible = true;
				}

				if (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing) {
					CString str = ResStr(IDS_CAPTURE_LIVE);

					long lChannel = 0, lVivSub = 0, lAudSub = 0;
					if (pAMTuner
							&& m_wndCaptureBar.m_capdlg.IsTunerActive()
							&& SUCCEEDED(pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub))) {
						CString ch;
						ch.Format(_T(" (ch%d)"), lChannel);
						str += ch;
					}

					m_wndStatusBar.SetStatusTimer(str);
				} else {
					CString str_temp;
					bool bmadvr = (AfxGetAppSettings().iDSVideoRendererType == VIDRNDT_DS_MADVR);

					if (m_bOSDLocalTime) {
						str_temp = GetSystemLocalTime();
					}

					if (m_bRemainingTime) {
						str_temp.GetLength() > 0 ? str_temp += L"\n" + m_wndStatusBar.GetStatusTimer() : str_temp = m_wndStatusBar.GetStatusTimer();
					}

					if (m_bOSDFileName) {
						str_temp.GetLength() > 0 ? str_temp += L"\n" + m_strCurPlaybackLabel : str_temp = m_strCurPlaybackLabel;
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
			if (m_pFullscreenWnd->IsWindow()) {
				TRACE ("==> HIDE!\n");
				if (!m_bInOptions && pWnd == m_pFullscreenWnd) {
					m_pFullscreenWnd->ShowCursor(false);
				}
				KillTimer(TIMER_FULLSCREENMOUSEHIDER);
			} else {
				CWnd* pWnd = WindowFromPoint(p);
				if (pWnd && !m_bInOptions && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside)) {
					m_fHideCursor = true;
					SetCursor(NULL);
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

				/*
				Reproduce:
				1. Start a video
				2. Pause video
				3. Hibernate computer
				4. Start computer again
				MPC-BE window should now be hung

				Stack dump from a Windows 7 64-bit machine:
				Thread 1:
				ntdll_77d30000!ZwWaitForSingleObject+0x15
				ntdll_77d30000!RtlpWaitOnCriticalSection+0x13e
				ntdll_77d30000!RtlEnterCriticalSection+0x150
				QUARTZ!CBlockLock<CKsOpmLib>::CBlockLock<CKsOpmLib>+0x14 <- Lock
				QUARTZ!CImageSync::get_AvgFrameRate+0x24
				QUARTZ!CVMRFilter::get_AvgFrameRate+0x31
				mpc_hc!CMainFrame::OnTimer+0xb80
				mpc_hc!CWnd::OnWndMsg+0x3e8
				mpc_hc!CWnd::WindowProc+0x24
				mpc_hc!CMainFrame::WindowProc+0x15e
				mpc_hc!AfxCallWndProc+0xac
				mpc_hc!AfxWndProc+0x36
				USER32!InternalCallWinProc+0x23
				USER32!UserCallWinProcCheckWow+0x109
				USER32!DispatchMessageWorker+0x3bc
				USER32!DispatchMessageW+0xf
				mpc_hc!AfxInternalPumpMessage+0x40
				mpc_hc!CWinThread::Run+0x5b
				mpc_hc!AfxWinMain+0x69
				mpc_hc!__tmainCRTStartup+0x11a

				Thread 2:
				ntdll_77d30000!ZwWaitForSingleObject+0x15
				ntdll_77d30000!RtlpWaitOnCriticalSection+0x13e
				ntdll_77d30000!RtlEnterCriticalSection+0x150
				QUARTZ!CBlockLock<CKsOpmLib>::CBlockLock<CKsOpmLib>+0x14 <- Lock
				QUARTZ!VMR9::CVMRFilter::NonDelegatingQueryInterface+0x1b
				mpc_hc!DSObjects::COuterVMR9::NonDelegatingQueryInterface+0x5b
				mpc_hc!CMacrovisionKicker::NonDelegatingQueryInterface+0xdc
				QUARTZ!CImageSync::QueryInterface+0x16
				QUARTZ!VMR9::CVMRFilter::CIImageSyncNotifyEvent::QueryInterface+0x19
				mpc_hc!DSObjects::CVMR9AllocatorPresenter::PresentImage+0xa9
				QUARTZ!VMR9::CVMRFilter::CIVMRImagePresenter::PresentImage+0x2c
				QUARTZ!VMR9::CImageSync::DoRenderSample+0xd5
				QUARTZ!VMR9::CImageSync::ReceiveWorker+0xad
				QUARTZ!VMR9::CImageSync::Receive+0x46
				QUARTZ!VMR9::CVideoMixer::CompositeTheStreamsTogether+0x24f
				QUARTZ!VMR9::CVideoMixer::MixerThread+0x184
				QUARTZ!VMR9::CVideoMixer::MixerThreadProc+0xd
				KERNEL32!BaseThreadInitThunk+0xe
				ntdll_77d30000!__RtlUserThreadStart+0x70
				ntdll_77d30000!_RtlUserThreadStart+0x1b

				There can be a bug in QUARTZ or more likely MPC-BE is doing something wrong
				*/
				m_pQP->get_AvgFrameRate(&val); // We hang here due to a lock that never gets released.
				info.Format(_T("%d.%02d %s"), val / 100, val % 100, rate);
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
				info.Format(_T("%d ms"), val);
				m_wndStatsBar.SetLine(ResStr(IDS_STATSBAR_JITTER), info);
			}

			if (m_pBI) {
				CAtlList<CString> sl;

				for (int i = 0, j = m_pBI->GetCount(); i < j; i++) {
					int samples, size;
					if (S_OK == m_pBI->GetStatus(i, samples, size)) {
						CString str;
						str.Format(_T("[%d]: %03d/%d KB"), i, samples, size / 1024);
						sl.AddTail(str);
					}
				}

				if (!sl.IsEmpty()) {
					CString str;
					str.Format(_T("%s (p%d)"), Implode(sl, ' '), m_pBI->GetPriority());

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
					CAtlList<CString> sl;

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
							str.Format(_T("[%d]: %d/%d Kb/s"), i, avg, cur);
						} else {
							str.Format(_T("[%d]: %d Kb/s"), i, avg);
						}
						sl.AddTail(str);
					}

					if (!sl.IsEmpty()) {
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
						int len = GetLocaleInfo(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
						lang.ReleaseBufferSetLength(max(len-1, 0));
					} else {
						lang.Format(ResStr(IDS_AG_UNKNOWN), ulCurrent+1);
					}

					switch (AATR.LanguageExtension) {
						case DVD_AUD_EXT_NotSpecified:
						default:
							break;
						case DVD_AUD_EXT_Captions:
							lang += _T(" (Captions)");
							break;
						case DVD_AUD_EXT_VisuallyImpaired:
							lang += _T(" (Visually Impaired)");
							break;
						case DVD_AUD_EXT_DirectorComments1:
							lang += _T(" (Director Comments 1)");
							break;
						case DVD_AUD_EXT_DirectorComments2:
							lang += _T(" (Director Comments 2)");
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
					int len = GetLocaleInfo(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(max(len-1, 0));

					switch (SATR.LanguageExtension) {
						case DVD_SP_EXT_NotSpecified:
						default:
							break;
						case DVD_SP_EXT_Caption_Normal:
							lang += _T("");
							break;
						case DVD_SP_EXT_Caption_Big:
							lang += _T(" (Big)");
							break;
						case DVD_SP_EXT_Caption_Children:
							lang += _T(" (Children)");
							break;
						case DVD_SP_EXT_CC_Normal:
							lang += _T(" (CC)");
							break;
						case DVD_SP_EXT_CC_Big:
							lang += _T(" (CC Big)");
							break;
						case DVD_SP_EXT_CC_Children:
							lang += _T(" (CC Children)");
							break;
						case DVD_SP_EXT_Forced:
							lang += _T(" (Forced)");
							break;
						case DVD_SP_EXT_DirectorComments_Normal:
							lang += _T(" (Director Comments)");
							break;
						case DVD_SP_EXT_DirectorComments_Big:
							lang += _T(" (Director Comments, Big)");
							break;
						case DVD_SP_EXT_DirectorComments_Children:
							lang += _T(" (Director Comments, Children)");
							break;
					}

					if (bIsDisabled) {
						lang = _T("-");
					}

					Subtitles.Format(_T("%s"),
									 lang);
				}

				m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), Subtitles);
			}

			if (GetMediaState() == State_Running && !m_bAudioOnly) {
				UINT fSaverActive = 0;
				if (SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&fSaverActive, 0)) {
					SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
					SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
				}

				fSaverActive = 0;
				if (SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, (PVOID)&fSaverActive, 0)) {
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
				}
				// prevent screensaver activate, monitor sleep/turn off after playback
				// SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			}
		}
		break;
		case TIMER_STATUSERASER: {
			KillTimer(TIMER_STATUSERASER);
			m_playingmsg.Empty();
		}
		break;
	}

	__super::OnTimer(nIDEvent);
}

bool CMainFrame::DoAfterPlaybackEvent()
{
	AppSettings& s = AfxGetAppSettings();

	bool fExit = (s.nCLSwitches & CLSW_CLOSE) || s.fExitAfterPlayback;

	if (s.nCLSwitches & CLSW_STANDBY) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		SetSystemPowerState(TRUE, FALSE);
		fExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	} else if (s.nCLSwitches & CLSW_HIBERNATE) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		SetSystemPowerState(FALSE, FALSE);
		fExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	} else if (s.nCLSwitches & CLSW_SHUTDOWN) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF | EWX_FORCEIFHUNG, 0);
		fExit = true;
	} else if (s.nCLSwitches & CLSW_LOGOFF) {
		SetPrivilege(SE_SHUTDOWN_NAME);
		ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG, 0);
		fExit = true;
	} else if (s.nCLSwitches & CLSW_LOCK) {
		LockWorkStation();
	}

	if (fExit) {
		SendMessage(WM_COMMAND, ID_FILE_EXIT);
	}

	return fExit;
}

//
// graph event EC_COMPLETE handler
//
bool CMainFrame::GraphEventComplete()
{
	AppSettings& s = AfxGetAppSettings();
	FILE_POSITION*	FilePosition = s.CurrentFilePosition();
	if (FilePosition) {
		FilePosition->llPosition = 0;

		QueryPerformanceCounter(&m_LastSaveTime);
		if (s.fKeepHistory && s.fRememberFilePos) {
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
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);
			} else {
				LONGLONG pos = 0;
				m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

				if (GetMediaState() == State_Paused) {
					SendMessage(WM_COMMAND, ID_PLAY_PLAY);
				}
			}
		} else {
			int NextMediaExist = 0;
			if (s.fNextInDirAfterPlayback) {
				NextMediaExist = SearchInDir(true);
			}
			if (!s.fNextInDirAfterPlayback || !(NextMediaExist > 1)) {
				if (s.fRewind) {
					SendMessage(WM_COMMAND, ID_PLAY_STOP);
				} else {
					m_fEndOfStream = true;
					SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
				}
				m_OSD.ClearMessage();

				if (m_fFullScreen && s.fExitFullScreenAtTheEnd) {
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
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			m_nLoops = nLoops;
		} else {
			if (m_fFullScreen && s.fExitFullScreenAtTheEnd) {
				OnViewFullscreen();
			}

			if (s.fRewind) {
				SendMessage(WM_COMMAND, ID_PLAY_STOP);
			} else {
				m_fEndOfStream = true;
				PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
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
	AppSettings& s = AfxGetAppSettings();
	HRESULT hr = S_OK;

	LONG evCode = 0;
	LONG_PTR evParam1, evParam2;
	while (m_pME && SUCCEEDED(m_pME->GetEvent(&evCode, &evParam1, &evParam2, 0))) {
#ifdef _DEBUG
		DbgLog((LOG_TRACE, 3, L"--> CMainFrame::OnGraphNotify on thread: %d; event: 0x%08x (%s)", GetCurrentThreadId(), evCode, GetEventString(evCode)));
#endif
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
			case EC_PAUSED:
				g_bExternalPaused = true;
				break;
			case EC_COMPLETE:
				if (!GraphEventComplete()) {
					return hr;
				}
				break;
			case EC_ERRORABORT:
				DbgLog((LOG_TRACE, 3, L"\thr = %08x", (HRESULT)evParam1));
				break;
			case EC_BUFFERING_DATA:
				DbgLog((LOG_TRACE, 3, L"\t%d, %d", evParam1, evParam2));

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
						SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
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

							Domain = _T("First Play");

							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}

							if (m_pDVDI && SUCCEEDED (m_pDVDI->GetDiscID (NULL, &llDVDGuid))) {
								m_fValidDVDOpen = true;

								if ( s.fShowDebugInfo ) {
									m_OSD.DebugMessage(_T("DVD Title: %d"), s.lDVDTitle);
								}

								if (s.lDVDTitle != 0) {
									s.NewDvd (llDVDGuid);
									// Set command line position
									hr = m_pDVDC->PlayTitle(s.lDVDTitle, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

									if ( s.fShowDebugInfo ) {
										m_OSD.DebugMessage(_T("PlayTitle: 0x%08X"), hr);
										m_OSD.DebugMessage(_T("DVD Chapter: %d"), s.lDVDChapter);
									}

									if (s.lDVDChapter > 1) {
										hr = m_pDVDC->PlayChapterInTitle(s.lDVDTitle, s.lDVDChapter, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

										if ( s.fShowDebugInfo ) {
											m_OSD.DebugMessage(_T("PlayChapterInTitle: 0x%08X"), hr);
										}
									} else {
										// Trick: skip trailers with some DVDs
										hr = m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

										if ( s.fShowDebugInfo ) {
											m_OSD.DebugMessage(_T("Resume: 0x%08X"), hr);
										}

										// If the resume call succeeded, then we skip PlayChapterInTitle
										// and PlayAtTimeInTitle.
										if ( hr == S_OK ) {
											// This might fail if the Title is not available yet?
											hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																   DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

											if ( s.fShowDebugInfo ) {
												m_OSD.DebugMessage(_T("PlayAtTime: 0x%08X"), hr);
											}
										} else {
											if ( s.fShowDebugInfo )
												m_OSD.DebugMessage(_T("Timecode requested: %02d:%02d:%02d.%03d"),
																   s.DVDPosition.bHours, s.DVDPosition.bMinutes,
																   s.DVDPosition.bSeconds, s.DVDPosition.bFrames);

											// Always play chapter 1 (for now, until something else dumb happens)
											hr = m_pDVDC->PlayChapterInTitle(s.lDVDTitle, 1,
																			 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

											if ( s.fShowDebugInfo ) {
												m_OSD.DebugMessage(_T("PlayChapterInTitle: 0x%08X"), hr);
											}

											// This might fail if the Title is not available yet?
											hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																	 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

											if ( s.fShowDebugInfo ) {
												m_OSD.DebugMessage(_T("PlayAtTime: 0x%08X"), hr);
											}

											if ( hr != S_OK ) {
												hr = m_pDVDC->PlayAtTimeInTitle(s.lDVDTitle, &s.DVDPosition,
																				DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

												if ( s.fShowDebugInfo ) {
													m_OSD.DebugMessage(_T("PlayAtTimeInTitle: 0x%08X"), hr);
												}
											}
										} // Resume

										hr = m_pDVDC->PlayAtTime(&s.DVDPosition,
																 DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

										if ( s.fShowDebugInfo ) {
											m_OSD.DebugMessage(_T("PlayAtTime: %d"), hr);
										}
									}

									m_iDVDTitle	  = s.lDVDTitle;
									s.lDVDTitle   = 0;
									s.lDVDChapter = 0;
								} else if (pDVDData->pDvdState) {
									// Set position from favorite
									VERIFY(SUCCEEDED(m_pDVDC->SetState(pDVDData->pDvdState, DVD_CMD_FLAG_Block, NULL)));
									// We don't want to restore the position from the favorite
									// if the playback is reinitialized so we clear the saved state
									pDVDData->pDvdState.Release();
								} else if (s.fKeepHistory && s.fRememberDVDPos && !s.NewDvd(llDVDGuid)) {
									// Set last remembered position (if founded...)
									DVD_POSITION* DvdPos = s.CurrentDVDPosition();

									if (SUCCEEDED(hr = m_pDVDC->PlayTitle(DvdPos->lTitle, DVD_CMD_FLAG_Flush, NULL))) {
										if (SUCCEEDED(hr = m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL))) {
											hr = m_pDVDC->PlayAtTime(&DvdPos->Timecode, DVD_CMD_FLAG_Flush, NULL);
										} else {
											if (SUCCEEDED(hr = m_pDVDC->PlayChapterInTitle(DvdPos->lTitle, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL))) {
												if (FAILED(hr = m_pDVDC->PlayAtTime(&DvdPos->Timecode, DVD_CMD_FLAG_Flush, NULL))) {
													hr = m_pDVDC->PlayAtTimeInTitle(DvdPos->lTitle, &DvdPos->Timecode, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
												}
											}
										}
									}

									if (SUCCEEDED(hr)) {
										m_iDVDTitle = DvdPos->lTitle;
									}
								} else if (s.fStartMainTitle && s.fNormalStartDVD) {
									m_pDVDC->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
								}
								s.fNormalStartDVD = true;
								if (s.fRememberZoomLevel && !m_fFullScreen && !s.IsD3DFullscreen()) { // Hack to the normal initial zoom for DVD + DXVA ...
									ZoomVideoWindow();
								}
							}
							break;
						case DVD_DOMAIN_VideoManagerMenu:
							Domain = _T("Video Manager Menu");
							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}
							break;
						case DVD_DOMAIN_VideoTitleSetMenu:
							Domain = _T("Video Title Set Menu");
							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}
							break;
						case DVD_DOMAIN_Title:
							Domain.Format(ResStr(IDS_AG_TITLE), m_iDVDTitle);
							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}
							DVD_POSITION* DvdPos;
							DvdPos = s.CurrentDVDPosition();
							if (DvdPos) {
								DvdPos->lTitle = m_iDVDTitle;
							}

							if (!m_fValidDVDOpen) {
								m_fValidDVDOpen = true;
								m_pDVDC->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
							}

							break;
						case DVD_DOMAIN_Stop:
							Domain = ResStr(IDS_AG_STOP);
							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}
							break;
						default:
							Domain = _T("-");
							if ( s.fShowDebugInfo ) {
								m_OSD.DebugMessage(_T("%s"), Domain);
							}
							break;
					}

					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), Domain);

	#if 0	// UOPs debug traces
					if (hr == VFW_E_DVD_OPERATION_INHIBITED) {
						ULONG UOPfields = 0;
						pDVDI->GetCurrentUOPS(&UOPfields);
						CString message;
						message.Format( _T("UOP bitfield: 0x%08X; domain: %s"), UOPfields, Domain);
						m_OSD.DisplayMessage( OSD_TOPLEFT, message );
					} else {
						m_OSD.DisplayMessage( OSD_TOPRIGHT, Domain );
					}
	#endif

					MoveVideoWindow(); // AR might have changed
					SetupChapters();
				}
				break;
			case EC_DVD_CURRENT_HMSF_TIME:
				g_bExternalPaused = false;
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
					DbgLog((LOG_TRACE, 3, L"\t%d, %d", evParam1, evParam2));

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
							err.AppendFormat(_T(" (%u \x2260 %u)"), LOWORD(evParam2), HIWORD(evParam2));
							break;
						case DVD_ERROR_LowParentalLevel:
							err = ResStr(IDS_MAINFRM_20);
							break;
						case DVD_ERROR_MacrovisionFail:
							err = ResStr(IDS_MAINFRM_21);
							break;
						case DVD_ERROR_IncompatibleSystemAndDecoderRegions:
							err = ResStr(IDS_MAINFRM_22);
							err.AppendFormat(_T(" (%u \x2260 %u)"), LOWORD(evParam2), HIWORD(evParam2));
							break;
						case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
							err = ResStr(IDS_MAINFRM_23);
							err.AppendFormat(_T(" (%u \x2260 %u)"), LOWORD(evParam2), HIWORD(evParam2));
							break;
					}

					SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

					m_closingmsg = err;
				}
				break;
			case EC_DVD_WARNING:
				if (m_pDVDC) {
					DbgLog((LOG_TRACE, 3, L"\t%d, %d", evParam1, evParam2));
				}
				break;
			case EC_VIDEO_SIZE_CHANGED: {
				DbgLog((LOG_TRACE, 3, L"\t%dx%d", CSize(evParam1)));

				WINDOWPLACEMENT wp;
				wp.length = sizeof(wp);
				GetWindowPlacement(&wp);

				CSize size(evParam1);
				m_bAudioOnly = (size.cx <= 0 || size.cy <= 0);

				if (s.fRememberZoomLevel
						&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)) {
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
				OnTimer(TIMER_STREAMPOSPOLLER);
				OnTimer(TIMER_STREAMPOSPOLLER2);
				SetupChapters();
				LoadKeyFrames();
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
					SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					m_closingmsg = !str.IsEmpty() ? str : L"Unspecified graph error";
					m_wndPlaylistBar.SetCurValid(false);
					return hr;
				}
				break;
			case EC_DVD_PLAYBACK_RATE_CHANGE:
				if (m_pDVDC) {
					if (m_fCustomGraph && s.AutoChangeFullscrRes.bEnabled == 1 &&
							m_fFullScreen && m_iDVDDomain == DVD_DOMAIN_Title) {
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
	OAFilterState fs = State_Stopped;
	m_pMC->GetState(0, &fs);
	if (fs == State_Running) {
		if (GetPlaybackMode() != PM_CAPTURE) {
			m_pMC->Pause();
		} else {
			m_pMC->Stop(); // Capture mode doesn't support pause
		}
	}

	if (m_OSD.GetOSDType() != OSD_TYPE_GDI) {
		m_OSD.HideMessage(true);
	}

	BOOL bResult = false;
	if (m_bOpenedThruThread) {
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_RESET, (WPARAM)&bResult, (LPARAM)&e);
		e.Wait();
	} else {
		ResetDevice();
	}

	if (m_OSD.GetOSDType() != OSD_TYPE_GDI) {
		m_OSD.HideMessage(false);
	}

	if (fs == State_Running) {
		m_pMC->Run();
	}
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

	AppSettings& s = AfxGetAppSettings();

	if (!m_closingmsg.IsEmpty()) {
		CString aborted(ResStr(IDS_AG_ABORTED));

		CloseMedia();

		if (m_closingmsg != aborted) {

			if (OpenFileData *pFileData	= dynamic_cast<OpenFileData*>(pOMD.m_p)) {
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

	m_flastnID = 0;

	RecalcLayout();
	if (IsWindow(m_wndToolBar.m_hWnd) && m_wndToolBar.IsVisible()) {
		m_wndToolBar.Invalidate();
	}

	if (m_bAudioOnly) {
		SetDwmPreview();
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
	if (m_iMediaLoadState == MLS_LOADING) {
		return FALSE;
	}

	SetFocus();

	CRect r;
	if (m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->GetClientRect(r);
	} else {
		m_wndView.GetClientRect(r);
		m_wndView.MapWindowPoints(this, &r);
	}

	if (id != wmcmd::WDOWN && id != wmcmd::WUP && !r.PtInRect(point)) {
		return FALSE;
	}

	BOOL ret = FALSE;

	WORD cmd = AssignedToCmd(id, m_fFullScreen);
	if (cmd) {
		SendMessage(WM_COMMAND, cmd);
		ret = TRUE;
	}

	return ret;
}

static bool s_fLDown = false;

void CMainFrame::OnNcLButtonDown(UINT nFlags, CPoint point)
{
	__super::OnNcLButtonDown(nFlags, point);

	s_fLDown = false;
}

static BOOL bFullScreen_LDOWN = FALSE;

void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (((GetTickCount()-m_nMenuHideTick)<100)) return;

	if (!m_pFullscreenWnd->IsWindow() || !m_OSD.OnLButtonDown (nFlags, point)) {
		SetFocus();

		bDVDMenuClicked = false;
		bDVDButtonAtPosition = false;

		if (GetPlaybackMode() == PM_DVD) {
			CRect vid_rect = m_wndView.GetVideoRect();
			m_wndView.MapWindowPoints(this, &vid_rect);

			CPoint pDVD = point - vid_rect.TopLeft();

			ULONG pulButtonIndex;
			if (SUCCEEDED(m_pDVDI->GetButtonAtPosition(pDVD, &pulButtonIndex))) {
				bDVDButtonAtPosition = true;
			}

			if (SUCCEEDED(m_pDVDC->ActivateAtPosition(pDVD))
					|| m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu
					|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu) {
				bDVDMenuClicked = true;
			}
		}

		CPoint p;
		GetCursorPos(&p);

		CRect r(0,0,0,0);
		if (m_pFullscreenWnd && ::IsWindow(m_pFullscreenWnd->m_hWnd)) {
			m_pFullscreenWnd->GetWindowRect(r);
		}

		CWnd* pWnd = WindowFromPoint(p);
		bool bFSWnd = false;
		if (pWnd && m_pFullscreenWnd == pWnd && r.PtInRect(p)) {
			bFSWnd = true;
		}

		s_fLDown = true;

		bool fLeftDownMouseBtnUnassigned = !AssignedToCmd(wmcmd::LDOWN, m_fFullScreen);
		if (!fLeftDownMouseBtnUnassigned && (m_fFullScreen || bFSWnd)) {
			bFullScreen_LDOWN = TRUE;
			OnButton(wmcmd::LDOWN, nFlags, point);
			return;
		}

		if (m_fFullScreen || bFSWnd) {
 			return;
		}

		templclick = false;
		SetCapture();
		return;

		__super::OnLButtonDown(nFlags, point);
	}
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	if (bFullScreen_LDOWN) {
		bFullScreen_LDOWN = FALSE;
		return;
	}

	if (!m_pFullscreenWnd->IsWindow() || !m_OSD.OnLButtonUp (nFlags, point)) {

		if (bDVDMenuClicked) {
			PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
			return;
		}

		if (!s_fLDown) {
			return;
		}

		CPoint p;
		GetCursorPos(&p);
		CWnd* pWnd = WindowFromPoint(p);
		CRect r(0,0,0,0);
		if (m_pFullscreenWnd && ::IsWindow(m_pFullscreenWnd->m_hWnd)) {
			m_pFullscreenWnd->GetWindowRect(r);
		}

		bool bFSWnd = false;
		if (pWnd && m_pFullscreenWnd == pWnd && r.PtInRect(p)) {
			bFSWnd = true;
		}

		bool fLeftDownMouseBtnUnassigned = !AssignedToCmd(wmcmd::LDOWN, m_fFullScreen);
		if (!fLeftDownMouseBtnUnassigned && !m_fFullScreen && !bFSWnd) {
			OnButton(wmcmd::LDOWN, nFlags, point);
			return;
 		}

		bool fLeftUpMouseBtnUnassigned = !AssignedToCmd(wmcmd::LUP, m_fFullScreen);
		if (!fLeftUpMouseBtnUnassigned && !m_fFullScreen) {
			OnButton(wmcmd::LUP, nFlags, point);
			return;
 		}

		__super::OnLButtonUp(nFlags, point);
	}
}

void CMainFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{

	if (bDVDButtonAtPosition) {
		PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		return;
	}

	if (s_fLDown) {
		OnButton(wmcmd::LDOWN, nFlags, point);
		s_fLDown = false;
	}

	if (!OnButton(wmcmd::LDBLCLK, nFlags, point)) {
		__super::OnLButtonDblClk(nFlags, point);
	}
}

void CMainFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendMessage(WM_CANCELMODE);
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
	SendMessage(WM_MBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
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
	if (WindowFromPoint(p) != m_pFullscreenWnd && !OnButton(wmcmd::RUP, nFlags, point)) {
		__super::OnRButtonUp(nFlags, point);
	}
}

void CMainFrame::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessage(WM_RBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	if (!OnButton(wmcmd::RDBLCLK, nFlags, point)) {
		__super::OnRButtonDblClk(nFlags, point);
	}
}

LRESULT CMainFrame::OnXButtonDown(WPARAM wParam, LPARAM lParam)
{
	SendMessage(WM_CANCELMODE);
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
	SendMessage(WM_XBUTTONDOWN, wParam, lParam);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam);
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DBLCLK : fwButton == XBUTTON2 ? wmcmd::X2DBLCLK : wmcmd::NONE,
					GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{

	if (m_wndPreView && m_wndPreView.IsWindowVisible()) {

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
				SendMessage(WM_SYSCOMMAND, SC_RESTORE, -1);
				RECT r;	GetWindowRect(&r);
				ClipRectToMonitor(&r);
				SetWindowPos(NULL, rc_forceNP.left, rc_forceNP.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, NULL);
 	}
	templclick = true;

	if (!m_OSD.OnMouseMove (nFlags, point)) {
		if (GetPlaybackMode() == PM_DVD) {
			CRect vid_rect = m_wndView.GetVideoRect();
			m_wndView.MapWindowPoints(this, &vid_rect);

			CPoint vp = point - vid_rect.TopLeft();
			ULONG pulButtonIndex;

			if (!m_fHideCursor) {
				SetCursor(LoadCursor(NULL, SUCCEEDED(m_pDVDI->GetButtonAtPosition(vp, &pulButtonIndex)) ? IDC_HAND : IDC_ARROW));
			}
			m_pDVDC->SelectAtPosition(vp);
		}

		CSize diff = m_lastMouseMove - point;
		AppSettings& s = AfxGetAppSettings();

		if (m_pFullscreenWnd->IsWindow() && (abs(diff.cx) + abs(diff.cy)) >= 1) {
			m_pFullscreenWnd->ShowCursor(true);

			// hide the cursor if we are not in the DVD menu
			if ((GetPlaybackMode() == PM_FILE) || (GetPlaybackMode() == PM_DVD)) {
				KillTimer(TIMER_FULLSCREENMOUSEHIDER);
				SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
			}
		} else if (m_fFullScreen && (abs(diff.cx) + abs(diff.cy)) >= 1) {
			int nTimeOut = s.nShowBarsWhenFullScreenTimeOut;

			if (nTimeOut < 0) {
				m_fHideCursor = false;
				if (s.fShowBarsWhenFullScreen) {
					ShowControls(s.nCS);
					if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && s.iDefaultCaptureDevice == 1) {
						m_wndNavigationBar.m_navdlg.UpdateElementList();
						m_wndNavigationBar.ShowControls(this, TRUE);
					}
				}

				KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
				SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
			} else if (nTimeOut == 0) {
				CRect r;
				GetClientRect(r);
				r.top = r.bottom;

				POSITION pos = m_bars.GetHeadPosition();
				for (int i = 1; pos; i <<= 1) {
					CControlBar* pNext = m_bars.GetNext(pos);
					CSize size = pNext->CalcFixedLayout(FALSE, TRUE);
					if (s.nCS&i) {
						r.top -= size.cy;
					}
				}


				// HACK: the controls would cover the menu too early hiding some buttons
				if (GetPlaybackMode() == PM_DVD
						&& (m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu
							|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu)) {
					r.top = r.bottom - 10;
				}

				m_fHideCursor = false;

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

					m_fHideCursor = false;

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

				SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
			} else {
				m_fHideCursor = false;
				if (s.fShowBarsWhenFullScreen) {
					ShowControls(s.nCS);
				}

				SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut*1000, NULL);
				SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut*1000, 2000), NULL);
			}
		}

		m_lastMouseMove = point;

		__super::OnMouseMove(nFlags, point);
	}
}

LRESULT CMainFrame::OnNcHitTest(CPoint point)
{
	LRESULT nHitTest = __super::OnNcHitTest(point);
	return /*((IsCaptionHidden()) && nHitTest == HTCLIENT) ? HTCAPTION : */nHitTest;
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar->IsKindOf(RUNTIME_CLASS(CVolumeCtrl))) {
		OnPlayVolume(0);
	} else if (pScrollBar->IsKindOf(RUNTIME_CLASS(CPlayerSeekBar)) && m_iMediaLoadState == MLS_LOADED) {
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

		CMenu* pSubMenu = NULL;

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
			UINT fState = (m_iMediaLoadState == MLS_LOADED
						   && (1/*GetPlaybackMode() == PM_DVD *//*|| (GetPlaybackMode() == PM_FILE && m_PlayList.GetCount() > 0)*/))
						  ? MF_ENABLED
						  : (MF_DISABLED | MF_GRAYED);
			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
			continue;
		}
		if (firstSubItemID == ID_VIEW_VF_HALF				// is "Video Frame" submenu
				|| firstSubItemID == ID_VIEW_INCSIZE		// is "Pan&Scan" submenu
				|| firstSubItemID == ID_ASPECTRATIO_SOURCE	// is "Override Aspect Ratio" submenu
				|| firstSubItemID == ID_VIEW_ZOOM_50) {		// is "Zoom" submenu
			UINT fState = (m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly)
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
		CMenu* pSubMenu = NULL;

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
			SetupAudioOptionSubMenu();
			pSubMenu = &m_audiosMenu;
		} else if (itemID == ID_SUBTITLES) {
			SetupSubtitlesSubMenu();
			pSubMenu = &m_subtitlesMenu;
		} else if (itemID == ID_AUDIOLANGUAGE) {
			SetupNavMixAudioSubMenu();
			pSubMenu = &m_navMixAudioMenu;
		} else if (itemID == ID_SUBTITLELANGUAGE) {
			SetupNavMixSubtitleSubMenu();
			pSubMenu = &m_navMixSubtitleMenu;
		} else if (itemID == ID_VIDEOANGLE) {

			CString menu_str = GetPlaybackMode() == PM_DVD ? ResStr(IDS_MENU_VIDEO_ANGLE) : ResStr(IDS_MENU_VIDEO_STREAM);

			mii.fMask = MIIM_STRING;
			mii.dwTypeData = (LPTSTR)(LPCTSTR)menu_str;
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);

			SetupVideoStreamsSubMenu();
			pSubMenu = &m_videoStreamsMenu;
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
		str += _T("\t") + key;

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
		AppSettings& s = AfxGetAppSettings();
		int i = 0, j = s.m_pnspresets.GetCount();
		for (; i < j; i++) {
			int k = 0;
			CString label = s.m_pnspresets[i].Tokenize(_T(","), k);
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

	m_nMenuHideTick = GetTickCount();
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
	m_fHideCursor = false;

	MSG msg;
	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NOANIMATION, point.x + 1, point.y + 1, this);
	PeekMessage(&msg, this->m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE); //remove the click LMB, which closes the popup menu

	s_fLDown = false;
	if (m_fFullScreen) {
		SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);    //need when working with menus and use the keyboard only
	}

	return TRUE;
}

void CMainFrame::OnMenuPlayerShort()
{
	if (IsMenuHidden() || m_pFullscreenWnd->IsWindow()) {
		OnMenu(m_popupMainMenu.GetSubMenu(0));
	} else {
		OnMenu(m_popupMenu.GetSubMenu(0));
	}
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

void CMainFrame::OnUpdatePlayerStatus(CCmdUI* pCmdUI)
{
	if (m_iMediaLoadState == MLS_LOADING) {
		pCmdUI->SetText(ResStr(IDS_CONTROLS_OPENING));
		if (AfxGetAppSettings().fUseWin7TaskBar && m_pTaskbarList) {
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		}

		SetStatusMessage(ResStr(IDS_CONTROLS_OPENING));
	} else if (m_iMediaLoadState == MLS_LOADED) {
		CString msg = FillMessage();

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
					GetDXVAStatus()) {
				msg.AppendFormat(_T(" [%s]"), GetDXVAVersion());
			}
		}

		pCmdUI->SetText(msg);
		SetStatusMessage(msg);
	} else if (m_iMediaLoadState == MLS_CLOSING) {
		pCmdUI->SetText(ResStr(IDS_CONTROLS_CLOSING));
		if (AfxGetAppSettings().fUseWin7TaskBar && m_pTaskbarList) {
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		}

		SetStatusMessage(ResStr(IDS_CONTROLS_CLOSING));
	} else {
		pCmdUI->SetText(m_closingmsg);
		SetStatusMessage(m_closingmsg);
	}
}

void CMainFrame::OnFilePostOpenMedia(CAutoPtr<OpenMediaData> pOMD)
{
	ASSERT(m_iMediaLoadState == MLS_LOADING);
	SetLoadState(MLS_LOADED);

	// remember OpenMediaData for later use
	m_lastOMD.Free();
	m_lastOMD.Attach(pOMD.Detach());

	if (m_bIsBDPlay == FALSE) {
		m_MPLSPlaylist.RemoveAll();
		m_LastOpenBDPath.Empty();
	}

	AppSettings& s = AfxGetAppSettings();

	if (s.fEnableEDLEditor) {
		m_wndEditListEditor.OpenFile(m_lastOMD->title);
	}

	m_AngleZ = 0;
	if (OpenFileData *pFileData = dynamic_cast<OpenFileData*>(m_lastOMD.m_p)) {
		// Rotation flag;
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IPropertyBag> pPB = pBF) {
				CComVariant var;
				if (SUCCEEDED(pPB->Read(CComBSTR(_T("ROTATION")), &var, NULL))) {
					CString fstr = var.bstrVal;
					if (!fstr.IsEmpty()) {
						int rotationValue = 0;
						if ((_stscanf_s(fstr, _T("%d"), &rotationValue) == 1) && rotationValue) {
							m_AngleZ = -rotationValue;
						}
					}
				}
				break;
			}
		}
		EndEnumFilters;
	}

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
	if (!m_youtubeFields.title.IsEmpty()) {
		CPlaylistItem pli;
		if (m_wndPlaylistBar.GetCur(pli)) {
			m_wndPlaylistBar.SetCurLabel(m_youtubeFields.title);
		}
	}

	// Waffs : PnS command line
	if (!s.strPnSPreset.IsEmpty()) {
		for (int i = 0; i < s.m_pnspresets.GetCount(); i++) {
			int j = 0;
			CString str = s.m_pnspresets[i];
			CString label = str.Tokenize(_T(","), j);
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

	m_nCurSubtitle		= -1;
	m_lSubtitleShift	= 0;
	if (m_pCAP) {
		m_pCAP->SetSubtitleDelay(0);
	}

	OnTimer(TIMER_STREAMPOSPOLLER);
	OnTimer(TIMER_STREAMPOSPOLLER2);

	if (s.fLaunchfullscreen && !s.IsD3DFullscreen() && !m_fFullScreen && !m_bAudioOnly) {
		ToggleFullscreen(true, true);
	}

	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(&wp);

		// Workaround to avoid MadVR freezing when switching channels in PM_CAPTURE mode:
		if (IsWindowVisible() && s.fRememberZoomLevel
				&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)
				&& GetPlaybackMode() == PM_CAPTURE && s.iDSVideoRendererType == VIDRNDT_DS_MADVR) {
			ShowWindow(SW_MAXIMIZE);
			wp.showCmd = SW_SHOWMAXIMIZED;
		}

		// restore magnification
		if (IsWindowVisible() && s.fRememberZoomLevel
				&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED)) {
			ZoomVideoWindow(false);
		}
	}

	m_bfirstPlay	= true;
	m_LastOpenFile	= m_lastOMD->title;

	if (!(s.nCLSwitches & CLSW_OPEN) && (s.nLoops > 0)) {
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);
	} else {
		SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	}
	s.nCLSwitches &= ~CLSW_OPEN;

	SendNowPlayingToApi();

	if (s.AutoChangeFullscrRes.bEnabled == 1 && m_fFullScreen) {
		AutoChangeMonitorMode();
	}
	if (m_fFullScreen && s.fRememberZoomLevel) {
		m_fFirstFSAfterLaunchOnFS = true;
	}

	// Ensure the dynamically added menu items are updated
	SetupFiltersSubMenu();
	SetupSubtitlesSubMenu();
	SetupNavMixAudioSubMenu();
	SetupNavMixSubtitleSubMenu();
	SetupVideoStreamsSubMenu();
	SetupNavChaptersSubMenu();
	SetupRecentFilesSubMenu();

	// correct window size if "Limit window proportions on resize" enable.
	if (!s.fRememberZoomLevel) {
		CRect r;
		GetWindowRect(&r);
		OnSizing(WMSZ_LEFT, r);
		MoveWindow(r);
	}
}

void CMainFrame::OnFilePostCloseMedia()
{
	DbgLog((LOG_TRACE, 3, L"CMainFrame::OnFilePostCloseMedia() : start"));

	SetPlaybackMode(PM_NONE);
	SetLoadState(MLS_CLOSED);

	if (m_closingmsg.IsEmpty()) {
		m_closingmsg = ResStr(IDS_CONTROLS_CLOSED);
	}

	if (IsD3DFullScreenMode()) {
		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		m_fHideCursor = false;
	}
	m_wndView.SetVideoRect();

	AfxGetAppSettings().nCLSwitches &= CLSW_OPEN|CLSW_PLAY|CLSW_AFTERPLAYBACK_MASK|CLSW_NOFOCUS;
	AfxGetAppSettings().ResetPositions();

	if (AfxGetAppSettings().fShowOSD) {
		m_OSD.Start(m_pOSDWnd);
	}

	if (m_YoutubeThread) {
		m_fYoutubeThreadWork = TH_CLOSE;
		if (WaitForSingleObject(m_YoutubeThread->m_hThread, 3000) == WAIT_TIMEOUT) {
			TerminateThread(m_YoutubeThread->m_hThread, 0xDEAD);
		}
		m_YoutubeThread = NULL;

		// delete the temporary file because we do not use it anymore
		if (::PathFileExists(m_YoutubeFile) && DeleteFile(m_YoutubeFile)) {
			POSITION pos = AfxGetAppSettings().slTMPFilesList.Find(m_YoutubeFile);
			if (pos) {
				AfxGetAppSettings().slTMPFilesList.RemoveAt(pos);
			}
		}
	}
	m_YoutubeTotal		= 0;
	m_YoutubeCurrent	= 0;

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();
	m_ExtSubPathsHandles.RemoveAll();
	SetEvent(m_hRefreshNotifyRenderThreadEvent);

	m_wndSeekBar.Enable(false);
	m_wndSeekBar.SetRange(0, 0);
	m_wndSeekBar.SetPos(0);
	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();
	m_wndStatusBar.Clear();
	m_wndStatusBar.ShowTimer(false);
	m_wndStatusBar.Relayout();

	if (AfxGetAppSettings().fEnableEDLEditor) {
		m_wndEditListEditor.CloseFile();
	}

	if (IsWindow(m_wndSubresyncBar.m_hWnd)) {
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE);
	}
	SetSubtitle(NULL);

	if (IsWindow(m_wndCaptureBar.m_hWnd)) {
		ShowControlBar(&m_wndCaptureBar, FALSE, TRUE);
		m_wndCaptureBar.m_capdlg.SetupVideoControls(_T(""), NULL, NULL, NULL);
		m_wndCaptureBar.m_capdlg.SetupAudioControls(_T(""), NULL, CInterfaceArray<IAMAudioInputMixer>());
	}

	RecalcLayout();
	UpdateWindow();

	SetWindowText(m_strTitle);
	m_Lcd.SetMediaTitle(LPCTSTR(m_strTitle));

	SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

	m_bIsBDPlay = FALSE;

	// Ensure the dynamically added menu items are cleared
	SetupFiltersSubMenu();
	SetupSubtitlesSubMenu();
	SetupNavMixAudioSubMenu();
	SetupNavMixSubtitleSubMenu();
	SetupVideoStreamsSubMenu();
	SetupNavChaptersSubMenu();

	UnloadExternalObjects();

	if (m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->ShowWindow(SW_HIDE);
		m_pFullscreenWnd->DestroyWindow();
	}

	SetDwmPreview(FALSE);
	m_wndToolBar.SwitchTheme();

	if (m_bNeedUnmountImage) {
		m_DiskImage.UnmountDiskImage();
	}
	m_bNeedUnmountImage = TRUE;

	SetCurrentDirectory(GetProgramPath()); // It is necessary to unlock the folder after opening files from explorer.

	SetThreadExecutionState(ES_CONTINUOUS);

	DbgLog((LOG_TRACE, 3, L"CMainFrame::OnFilePostCloseMedia() : end"));
}

void CMainFrame::OnBossKey()
{
	if (m_wndFlyBar && m_wndFlyBar.IsWindowVisible()) {
		m_wndFlyBar.ShowWindow(SW_HIDE);
	}
	// Disable animation
	ANIMATIONINFO AnimationInfo;
	AnimationInfo.cbSize = sizeof(ANIMATIONINFO);
	::SystemParametersInfo(SPI_GETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
	int WindowAnimationType = AnimationInfo.iMinAnimate;
	AnimationInfo.iMinAnimate = 0;
	::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);

	SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	if (m_fFullScreen) {
		SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
	}
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);

	// Enable animation
	AnimationInfo.iMinAnimate = WindowAnimationType;
	::SystemParametersInfo(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &AnimationInfo, 0);
}

void CMainFrame::OnStreamAudio(UINT nID)
{
	nID -= ID_STREAM_AUDIO_NEXT;

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {

		Stream as;
		CAtlArray<Stream> MixAS;
		int iSel	= -1;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS) {
			DWORD cStreamsS = 0;
			if (SUCCEEDED(pSS->Count(&cStreamsS)) && cStreamsS > 0) {
				for (int i = 0; i < (int)cStreamsS; i++) {
					//iSel = 0;
					DWORD dwFlags = DWORD_MAX;
					DWORD dwGroup = DWORD_MAX;
					if (FAILED(pSS->Info(i, NULL, &dwFlags, NULL, &dwGroup, NULL, NULL, NULL))) {
						return;
					}

					if (dwGroup == 1) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
							iSel = MixAS.GetCount();
							//iSel = 1;
						}
						as.Filter	= 1;
						as.Index	= i;
						as.Sel		= iSel;
						as.Num++;
						MixAS.Add(as);
					}
				}
			}
		}

		CComQIPtr<IAMStreamSelect> pSSa = m_pSwitcherFilter;
		if (pSSa) {
			DWORD cStreamsA = 0;
			int i;
			MixAS.GetCount() > 0 ? i = 1 : i = 0;
			if (SUCCEEDED(pSSa->Count(&cStreamsA)) && cStreamsA > 0) {
				for (i; i < (int)cStreamsA; i++) {
					//iSel = 0;
					DWORD dwFlags = DWORD_MAX;
					if (FAILED(pSSa->Info(i, NULL, &dwFlags, NULL, NULL, NULL, NULL, NULL))) {
						return;
					}

					if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
						//iSel = 1;
						iSel = MixAS.GetCount();
						for (size_t i = 0; i < MixAS.GetCount(); i++) {
							if (MixAS[i].Sel == 1) {
								MixAS[i].Sel = 0;
							}
						}
					}

					as.Filter	= 2;
					as.Index	= i;
					as.Sel		= iSel;
					as.Num++;
					MixAS.Add(as);
				}
			}
		}


		int cnt = MixAS.GetCount();
		if (cnt > 1) {
			int nNewStream2 = MixAS[(iSel + (nID == 0 ? 1 : cnt - 1)) % cnt].Num;
			int iF;
			int nNewStream;

			for (size_t i = 0; i < MixAS.GetCount(); i++) {
				if (MixAS[i].Num == nNewStream2) {
					iF			= MixAS[i].Filter;
					nNewStream	= MixAS[i].Index;
					break;
				}
			}

			WCHAR* pszName = NULL;

			bool ExtStream = false;
			if (iF == 1) { // Splitter Audio Tracks

				for (size_t i = 0; i < MixAS.GetCount(); i++) {
					if (MixAS[i].Sel == iSel && MixAS[i].Filter == 2) {
						ExtStream = true;
						break;
					}
				}

				if (ExtStream) { // return from external audiotrack (of the AudioSwitcher) to internal audiotrack (of Splitter) without bug
					pSSa->Enable(0, AMSTREAMSELECTENABLE_ENABLE);
				}
				pSS->Enable(nNewStream, AMSTREAMSELECTENABLE_ENABLE);

				if (SUCCEEDED(pSS->Info(nNewStream, NULL, NULL, NULL, NULL, &pszName, NULL, NULL))) {
					CString	strMessage;
					CString audio_stream = pszName;
					int k = audio_stream.Find(_T("Audio - "));
					if (k >= 0) {
						audio_stream = audio_stream.Right(audio_stream.GetLength() - k - 8);
					}
					strMessage.Format(ResStr(IDS_AUDIO_STREAM), audio_stream);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			} else if (iF == 2) { // AudioSwitcher Audio Tracks

				pSSa->Enable(nNewStream, AMSTREAMSELECTENABLE_ENABLE);

				if (SUCCEEDED(pSSa->Info(nNewStream, NULL, NULL, NULL, NULL, &pszName, NULL, NULL))) {
					CString	strMessage;
					strMessage.Format(ResStr(IDS_AUDIO_STREAM), pszName);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}

			if (pszName) {
				CoTaskMemFree(pszName);
			}
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		SendMessage(WM_COMMAND, ID_DVD_AUDIO_NEXT+nID);
	}
}

void CMainFrame::OnStreamSub(UINT nID)
{
	nID -= ID_STREAM_SUB_NEXT;

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

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

							if (FAILED(pSS->Info(i, NULL, &dwFlags, NULL, &dwGroup, NULL, NULL, NULL))) {
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

					WCHAR* pName = NULL;
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

							if (FAILED(pSS->Info(i, NULL, NULL, NULL, &dwGroup, &pName, NULL, NULL))
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
								int k = sub_stream.Find(_T("Subtitle - "));
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

				WCHAR* pName = NULL;
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
		CAtlArray<Stream> MixSS;
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
					if (FAILED(pSS->Info(i, NULL, &dwFlags, NULL, &dwGroup, NULL, NULL, NULL))) {
						return;
					}

					if (dwGroup == 2) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE)) {
							iSel = MixSS.GetCount();
						}
						ss.Filter	= 1;
						ss.Index	= i;
						ss.Sel		= iSel;
						ss.Num++;
						MixSS.Add(ss);
					}
				}
			}
		}

		splcnt = MixSS.GetCount();

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
				if (m_iSubtitleSel == subcnt) iSel = MixSS.GetCount();
				ss.Sel		= iSel;
				MixSS.Add(ss);
			}
		}

		int cnt = MixSS.GetCount();
		if (cnt > 1) {
			int nNewStream2 = MixSS[(iSel + (nID == 0 ? 1 : cnt - 1)) % cnt].Num;
			int iF;
			int nNewStream;

			for (size_t i = 0; i < MixSS.GetCount(); i++) {
				if (MixSS[i].Num == nNewStream2) {
					iF			= MixSS[i].Filter;
					nNewStream	= MixSS[i].Index;
					break;
				}
			}

			bool ExtStream = false;
			if (iF == 1) { // Splitter Subtitle Tracks

				for (size_t i = 0; i < MixSS.GetCount(); i++) {
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

				WCHAR* pszName = NULL;
				if (SUCCEEDED(pSS->Info(nNewStream, NULL, NULL, NULL, NULL, &pszName, NULL, NULL))) {
					CString	strMessage;
					CString sub_stream = pszName;
					int k = sub_stream.Find(_T("Subtitle - "));
					if (k >= 0) {
						sub_stream = sub_stream.Right(sub_stream.GetLength() - k - 8);
					}
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), sub_stream);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);

					if (pszName) {
						CoTaskMemFree(pszName);
					}
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
	} else if (GetPlaybackMode() == PM_DVD) {
		SendMessage(WM_COMMAND, ID_DVD_SUB_NEXT+nID);
	}
}

void CMainFrame::OnStreamSubOnOff()
{
	if (m_iMediaLoadState != MLS_LOADED) {
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
				WCHAR* pName = NULL;
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
	} else if (GetPlaybackMode() == PM_DVD) {
		SendMessage(WM_COMMAND, ID_DVD_SUB_ONOFF);
	}
}

void CMainFrame::OnDvdAngle(UINT nID)
{
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_pDVDI && m_pDVDC) {
		ULONG ulAnglesAvailable, ulCurrentAngle;
		if (SUCCEEDED(m_pDVDI->GetCurrentAngle(&ulAnglesAvailable, &ulCurrentAngle)) && ulAnglesAvailable > 1) {
			ulCurrentAngle += (nID == ID_DVD_ANGLE_NEXT) ? 1 : -1;
			if (ulCurrentAngle > ulAnglesAvailable) {
				ulCurrentAngle = 1;
			} else if (ulCurrentAngle < 1) {
				ulCurrentAngle = ulAnglesAvailable;
			}
			m_pDVDC->SelectAngle(ulCurrentAngle, DVD_CMD_FLAG_Block, NULL);

			CString osdMessage;
			osdMessage.Format(ResStr(IDS_AG_ANGLE), ulCurrentAngle);
			m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
		}
	}
}

void CMainFrame::OnDvdAudio(UINT nID)
{
	HRESULT		hr;
	nID -= ID_DVD_AUDIO_NEXT;

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_pDVDI && m_pDVDC) {
		ULONG nStreamsAvailable, nCurrentStream;
		if (SUCCEEDED(m_pDVDI->GetCurrentAudio(&nStreamsAvailable, &nCurrentStream)) && nStreamsAvailable > 1) {
			DVD_AudioAttributes		AATR;
			UINT					nNextStream = (nCurrentStream+(nID==0?1:nStreamsAvailable-1))%nStreamsAvailable;

			hr = m_pDVDC->SelectAudioStream(nNextStream, DVD_CMD_FLAG_Block, NULL);
			if (SUCCEEDED(m_pDVDI->GetAudioAttributes(nNextStream, &AATR))) {
				CString lang;
				CString	strMessage;
				if (AATR.Language) {
					int len = GetLocaleInfo(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(max(len-1, 0));
				} else {
					lang.Format(ResStr(IDS_AG_UNKNOWN), nNextStream+1);
				}

				CString format = GetDVDAudioFormatName(AATR);
				CString str("");

				if (!format.IsEmpty()) {
					str.Format(ResStr(IDS_MAINFRM_11),
							   lang,
							   format,
							   AATR.dwFrequency,
							   AATR.bQuantization,
							   AATR.bNumberOfChannels,
							   (AATR.bNumberOfChannels > 1 ? ResStr(IDS_MAINFRM_13) : ResStr(IDS_MAINFRM_12)));
					str += FAILED(hr) ? _T(" [") + ResStr(IDS_AG_ERROR) + _T("] ") : _T("");
					strMessage.Format(ResStr(IDS_AUDIO_STREAM), str);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}
		}
	}
}

void CMainFrame::OnDvdSub(UINT nID)
{
	HRESULT		hr;
	nID -= ID_DVD_SUB_NEXT;

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_pDVDI && m_pDVDC) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
				&& ulStreamsAvailable > 1) {
			//UINT nNextStream = (ulCurrentStream+(nID==0?1:ulStreamsAvailable-1))%ulStreamsAvailable;
			int nNextStream;

			if (!bIsDisabled) {
				nNextStream = ulCurrentStream+ (nID==0?1:-1);
			} else {
				nNextStream = (nID==0?0:ulStreamsAvailable-1);
			}

			if (!bIsDisabled && ((nNextStream < 0) || ((ULONG)nNextStream >= ulStreamsAvailable))) {
				m_pDVDC->SetSubpictureState(FALSE, DVD_CMD_FLAG_Block, NULL);
				m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_SUBTITLE_STREAM_OFF));
			} else {
				hr = m_pDVDC->SelectSubpictureStream(nNextStream, DVD_CMD_FLAG_Block, NULL);

				DVD_SubpictureAttributes SATR;
				m_pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, NULL);
				if (SUCCEEDED(m_pDVDI->GetSubpictureAttributes(nNextStream, &SATR))) {
					CString lang;
					CString	strMessage;
					int len = GetLocaleInfo(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					lang.ReleaseBufferSetLength(max(len-1, 0));
					lang += FAILED(hr) ? _T(" [") + ResStr(IDS_AG_ERROR) + _T("] ") : _T("");
					strMessage.Format(ResStr(IDS_SUBTITLE_STREAM), lang);
					m_OSD.DisplayMessage(OSD_TOPLEFT, strMessage);
				}
			}
		}
	}
}

void CMainFrame::OnDvdSubOnOff()
{
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_pDVDI && m_pDVDC) {
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
			m_pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

//
// menu item handlers
//

// file

void CMainFrame::OnFileOpenQuick()
{
	if (m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

	CString filter;
	CAtlArray<CString> mask;
	s.m_Formats.GetFilter(filter, mask);

	DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR;
	if (!s.fKeepHistory) {
		dwFlags |= OFN_DONTADDTORECENT;
	}

	COpenFileDlg fd(mask, true, NULL, NULL, dwFlags, filter, GetModalParent());
	if (fd.DoModal() != IDOK) {
		return;
	}

	CAtlList<CString> fns;

	POSITION pos = fd.GetStartPosition();
	while (pos) {
		fns.AddTail(fd.GetNextPathName(pos));
	}

	bool fMultipleFiles = false;

	if (fns.GetCount() > 1
			|| fns.GetCount() == 1
			&& (fns.GetHead()[fns.GetHead().GetLength()-1] == '\\'
				|| fns.GetHead()[fns.GetHead().GetLength()-1] == '*')) {
		fMultipleFiles = true;
	}

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	if (fns.GetCount() == 1) {
		if (OpenBD(fns.GetHead())) {
			return;
		}
	}

	m_wndPlaylistBar.Open(fns, fMultipleFiles);

	if (m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating()) {
		//ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem();
}

void CMainFrame::OnFileOpenMedia()
{
	if (m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar) || m_pFullscreenWnd->IsWindow()) {
		return;
	}

	static COpenDlg dlg;
	if (IsWindow(dlg.GetSafeHwnd()) && dlg.IsWindowVisible()) {
		dlg.SetForegroundWindow();
		return;
	}
	if (dlg.DoModal() != IDOK || dlg.m_fns.GetCount() == 0) {
		return;
	}

	if (!dlg.m_fAppendPlaylist) {
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	}

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	if (!dlg.m_fMultipleFiles) {
		if (OpenBD(dlg.m_fns.GetHead())) {
			return;
		}
	}

	CString fn = dlg.m_fns.GetHead();
	if (OpenYoutubePlaylist(fn)) {
		return;
	}

	if (dlg.m_fAppendPlaylist) {
		m_wndPlaylistBar.Append(dlg.m_fns, dlg.m_fMultipleFiles);
		return;
	}

	m_wndPlaylistBar.Open(dlg.m_fns, dlg.m_fMultipleFiles);
	OpenCurPlaylistItem();
}

void CMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState != MLS_LOADING);

	if (pCmdUI->m_nID == ID_FILE_OPENISO && pCmdUI->m_pMenu != NULL && !m_DiskImage.DriveAvailable()) {
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID, MF_BYCOMMAND);
	}
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS)
{
	if (m_bClosingState) {
		return FALSE;
	}

	AppSettings& s = AfxGetAppSettings();

	if (pCDS->dwData != 0x6ABE51 || pCDS->cbData < sizeof(DWORD)) {
		if (s.hMasterWnd) {
			ProcessAPICommand(pCDS);
			return TRUE;
		} else {
			return FALSE;
		}
	}

	DWORD len		= *((DWORD*)pCDS->lpData);
	TCHAR* pBuff	= (TCHAR*)((DWORD*)pCDS->lpData + 1);
	TCHAR* pBuffEnd	= (TCHAR*)((BYTE*)pBuff + pCDS->cbData - sizeof(DWORD));

	CAtlList<CString> cmdln;

	while (len-- > 0) {
		CString str;
		while (pBuff < pBuffEnd && *pBuff) {
			str += *pBuff++;
		}
		pBuff++;
		cmdln.AddTail(str);
	}

	s.ParseCommandLine(cmdln);

	if (s.nCLSwitches & CLSW_SLAVE) {
		SendAPICommand(CMD_CONNECT, L"%d", PtrToInt(GetSafeHwnd()));
	}

	POSITION pos = s.slFilters.GetHeadPosition();
	while (pos) {
		CString fullpath = MakeFullPath(s.slFilters.GetNext(pos));

		CString path = AddSlash(GetFolderOnly(fullpath));

		WIN32_FIND_DATA fd = {0};
		HANDLE hFind = FindFirstFile(fullpath, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				CFilterMapper2 fm2(false);
				fm2.Register(path + fd.cFileName);
				while (!fm2.m_filters.IsEmpty()) {
					if (FilterOverride* f = fm2.m_filters.RemoveTail()) {
						f->fTemporary = true;

						bool fFound = false;

						POSITION pos2 = s.m_filters.GetHeadPosition();
						while (pos2) {
							FilterOverride* f2 = s.m_filters.GetNext(pos2);
							if (f2->type == FilterOverride::EXTERNAL && !f2->path.CompareNoCase(f->path)) {
								fFound = true;
								break;
							}
						}

						if (!fFound) {
							CAutoPtr<FilterOverride> p(f);
							s.m_filters.AddHead(p);
						}
					}
				}
			} while (FindNextFile(hFind, &fd));

			FindClose(hFind);
		}
	}

	bool fSetForegroundWindow = false;

	if ((s.nCLSwitches & CLSW_DVD) && !s.slFiles.IsEmpty()) {
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
		if (p) {
			p->path = s.slFiles.GetHead();
			p->subs.AddTailList(&s.slSubs);
		}
		OpenMedia(p);
	} else if (s.nCLSwitches & CLSW_CD) {
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		CAtlList<CString> sl;

		if (!s.slFiles.IsEmpty()) {
			GetCDROMType(s.slFiles.GetHead()[0], sl);
		} else {
			CString dir;
			dir.ReleaseBufferSetLength(GetCurrentDirectory(_MAX_PATH, dir.GetBuffer(_MAX_PATH)));

			GetCDROMType(dir[0], sl);
			for (TCHAR drive = 'C'; sl.IsEmpty() && drive <= 'Z'; drive++) {
				GetCDROMType(drive, sl);
			}
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	} else if (!s.slFiles.IsEmpty()) {
		if (s.slFiles.GetCount() == 1
				&& (OpenBD(s.slFiles.GetHead())
					|| OpenIso(s.slFiles.GetHead())
					|| OpenYoutubePlaylist(s.slFiles.GetHead()))) {
			;
		} else if (s.slFiles.GetCount() == 1 && ::PathIsDirectory(s.slFiles.GetHead() + _T("\\VIDEO_TS"))) {
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			fSetForegroundWindow = true;

			CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
			if (p) {
				p->path = s.slFiles.GetHead();
				p->subs.AddTailList(&s.slSubs);
			}
			OpenMedia(p);
		} else {
			CAtlList<CString> sl;
			sl.AddTailList(&s.slFiles);

			ParseDirs(sl);
			bool fMulti = sl.GetCount() > 1;

			if (!fMulti) {
				sl.AddTailList(&s.slDubs);
			}

			if (m_dwLastRun && ((GetTickCount() - m_dwLastRun) < 500)) {
				s.nCLSwitches |= CLSW_ADD;
			}
			m_dwLastRun = GetTickCount();

			if ((s.nCLSwitches & CLSW_ADD) && m_wndPlaylistBar.GetCount() > 0) {
				m_wndPlaylistBar.Append(sl, fMulti, &s.slSubs);

				if (s.nCLSwitches&(CLSW_OPEN | CLSW_PLAY)) {
					m_wndPlaylistBar.SetLast();
					OpenCurPlaylistItem();
				}
			} else {
				//UINT nCLSwitches = s.nCLSwitches;	// backup cmdline params

				//SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
				fSetForegroundWindow = true;

				//s.nCLSwitches = nCLSwitches;		// restore cmdline params
				m_wndPlaylistBar.Open(sl, fMulti, &s.slSubs);

				OpenCurPlaylistItem((s.nCLSwitches & CLSW_STARTVALID) ? s.rtStart : INVALID_TIME);

				s.nCLSwitches &= ~CLSW_STARTVALID;
				s.rtStart = 0;
			}
		}
	} else {
		if ((s.nCLSwitches & CLSW_PLAY) && m_wndPlaylistBar.GetCount() > 0) {
			OpenCurPlaylistItem();
		}
		s.nCLSwitches = CLSW_NONE;
	}

	if (fSetForegroundWindow && !(s.nCLSwitches & CLSW_NOFOCUS)) {
		SetForegroundWindow();
	}
	s.nCLSwitches &= ~CLSW_NOFOCUS;

	UpdateThumbarButton();

	return TRUE;
}

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData)
{
	switch (uMsg) {
		case BFFM_INITIALIZED:
			//Initial directory is set here
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE,(LPARAM)(LPCTSTR)AfxGetAppSettings().strDVDPath);
			break;
		default:
			break;
	}
	return 0;
}

void CMainFrame::OnFileOpenDVD()
{
	if (m_iMediaLoadState == MLS_LOADING) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();
	CString strTitle = ResStr(IDS_MAINFRM_46);
	CString path;
	if (IsWinVistaOrLater()) {
		CFileDialog dlg(TRUE);
		IFileOpenDialog *openDlgPtr = dlg.GetIFileOpenDialog();

		if (openDlgPtr != NULL) {
			openDlgPtr->SetTitle(strTitle);

			CComPtr<IShellItem> psiFolder;
			if (SUCCEEDED(afxGlobalData.ShellCreateItemFromParsingName(s.strDVDPath, NULL, IID_PPV_ARGS(&psiFolder)))) {
				openDlgPtr->SetFolder(psiFolder);
			}

			openDlgPtr->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			if (FAILED(openDlgPtr->Show(m_hWnd))) {
				openDlgPtr->Release();
				return;
			}

			psiFolder = NULL;
			if (SUCCEEDED(openDlgPtr->GetResult(&psiFolder))) {
				LPWSTR folderpath = NULL;
				if(SUCCEEDED(psiFolder->GetDisplayName(SIGDN_FILESYSPATH, &folderpath))) {
					path = folderpath;
					CoTaskMemFree(folderpath);
				}
			}

			openDlgPtr->Release();
		}
	} else {
		TCHAR _path[_MAX_PATH] = { 0 };

		BROWSEINFO bi;
		bi.hwndOwner = m_hWnd;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = _path;
		bi.lpszTitle = strTitle;
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
		bi.lpfn = BrowseCallbackProc;
		bi.lParam = 0;
		bi.iImage = 0;

		static PCIDLIST_ABSOLUTE iil;
		iil = SHBrowseForFolder(&bi);
		if (iil) {
			SHGetPathFromIDList(iil, _path);
			path = _path;
		}
	}

	if (!path.IsEmpty()) {
		s.strDVDPath = AddSlash(path);
		if (!OpenBD(path)) {
			CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
			p->path = AddSlash(path);
			p->path.Replace('/', '\\');

			OpenMedia(p);
		}
	}
}

void CMainFrame::OnFileOpenIso()
{
	if (m_iMediaLoadState == MLS_LOADING) {
		return;
	}

	if (m_DiskImage.DriveAvailable()) {
		DWORD dwFlags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_ENABLEINCLUDENOTIFY | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;

		CString szFilter;
		szFilter.Format(_T("Image Files (%s)|%s||"), m_DiskImage.GetExts(), m_DiskImage.GetExts());
		CFileDialog fd(TRUE, NULL, NULL, dwFlags, szFilter);
		if (fd.DoModal() != IDOK) {
			return;
		}

		CString pathName = fd.GetPathName();
		OpenIso(pathName);
	}
}

void CMainFrame::OnFileOpenDevice()
{
	AppSettings& s = AfxGetAppSettings();

	if (m_iMediaLoadState == MLS_LOADING) {
		return;
	}

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
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
	if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && m_iMediaLoadState == MLS_LOADED && s.iDefaultCaptureDevice == 1) {
		m_wndNavigationBar.m_navdlg.UpdateElementList();
		ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
	}
}

void CMainFrame::OnFileOpenCD(UINT nID)
{
	nID -= ID_FILE_OPEN_CD_START;

	nID++;
	for (TCHAR drive = 'C'; drive <= 'Z'; drive++) {
		CAtlList<CString> sl;

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
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			SetForegroundWindow();
			ShowWindow(SW_SHOW);

			if (CDRom_t == CDROM_BDVideo && sl.GetCount() == 1) {
				if (OpenBD(sl.GetHead())) {
					return;
				}
			}

			m_wndPlaylistBar.Open(sl, true);
			OpenCurPlaylistItem();

			break;
		}
	}
}

void CMainFrame::OnFileReOpen()
{
	if (m_iMediaLoadState == MLS_LOADING) {
		return;
	}

	if (!m_LastOpenBDPath.IsEmpty() && OpenBD(m_LastOpenBDPath)) {
		return;
	}

	OpenCurPlaylistItem();
}

void CMainFrame::DropFiles(CAtlList<CString>& slFiles)
{
	SetForegroundWindow();

	if (slFiles.IsEmpty()) {
		return;
	}

	if (m_wndPlaylistBar.IsWindowVisible()) {
		m_wndPlaylistBar.DropFiles(slFiles);
		return;
	}
	
	BOOL bIsValidSubExtAll = TRUE;

	POSITION pos = slFiles.GetHeadPosition();
	while (pos) {
		CString ext = GetFileExt(slFiles.GetNext(pos)).Mid(1); // extension without a dot
		ext.MakeLower();

		bool validate_ext = false;
		for (size_t i = 0; i < Subtitle::subTypesExt.size(); i++) {
			if (ext == Subtitle::subTypesExt[i] &&
					!(m_pDVS && ext == "mks")) {
				validate_ext = true;
				break;
			}
		}

		if (!validate_ext) {
			bIsValidSubExtAll = FALSE;
			break;
		}
	}

	if (bIsValidSubExtAll && m_iMediaLoadState == MLS_LOADED && (m_pCAP || m_pDVS)) {

		POSITION pos = slFiles.GetHeadPosition();
		while (pos) {
			CString fname = slFiles.GetNext(pos);
			BOOL b_SubLoaded = FALSE;

			if (m_pDVS) {
				if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)fname))) {
					m_pDVS->put_SelectedLanguage(0);
					m_pDVS->put_HideSubtitles(true);
					m_pDVS->put_HideSubtitles(false);

					b_SubLoaded = TRUE;
				}
			} else {
				ISubStream *pSubStream = NULL;
				if (LoadSubtitle(fname, &pSubStream)) {
					SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
					b_SubLoaded = TRUE;

					AddSubtitlePathsAddons(fname);
				}
			}

			if (b_SubLoaded) {
				SendStatusMessage(GetFileOnly(fname) + ResStr(IDS_MAINFRM_47), 3000);
				if (m_pDVS) {
					return;
				}
			}
		}

		return;
	}

	if (m_iMediaLoadState == MLS_LOADING) {
		return;
	}

	{
		CString path = slFiles.GetHead();
		if (OpenBD(path) || (slFiles.GetCount() == 1 && OpenIso(path))) {
			return;
		}
	}

	m_wndPlaylistBar.Open(slFiles, true);
	OpenCurPlaylistItem();
}

void CMainFrame::OnFileSaveAs()
{
	AppSettings& s = AfxGetAppSettings();

	CString ext, ext_list, in = m_strUrl, out = in;

	if (!m_youtubeFields.fname.IsEmpty()) {
		out = GetAltFileName();
		ext = GetFileExt(out).MakeLower();
	} else {
		int find = out.Find(_T("://"));
		if (find < 0) {
			ext = GetFileExt(out).MakeLower();
			out = GetFileOnly(out);
			if (ext == _T(".cda")) {
				out = out.Left(out.GetLength()-4) + _T(".wav");
			} else if (ext == _T(".ifo")) {
				out = out.Left(out.GetLength()-4) + _T(".vob");
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
		}
	}

	if (!ext.IsEmpty()) {
		ext_list.Format(_T("Media (*%s)|*%s|"), ext, ext);
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

	if (CTaskDialog::IsSupported()) {
		CSaveTaskDlg dlg(in, name, p);
		dlg.DoModal();
	} else {
		CSaveDlg dlg(in, name, p);
		dlg.DoModal();
	}

	if (fs == State_Running) {
		m_pMC->Run();
	}
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	if (m_iMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_FILE) {
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(TRUE);
}

bool CMainFrame::GetDIB(BYTE** ppData, long& size, bool fSilent)
{
	if (!ppData) {
		return false;
	}

	*ppData = NULL;
	size = 0;

	OAFilterState fs = GetMediaState();

	if (!(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly && (fs == State_Paused || fs == State_Running))) {
		return false;
	}

	if (fs == State_Running && !m_pCAP) {
		m_pMC->Pause();
		GetMediaState(); // wait for completion of the pause command
	}

	HRESULT hr = S_OK;
	CString errmsg;

	do {
		if (m_pCAP) {
			hr = m_pCAP->GetDIB(NULL, (DWORD*)&size);
			if (FAILED(hr)) {
				errmsg.Format(ResStr(IDS_MAINFRM_49), hr);
				break;
			}

			*ppData = DNew BYTE[size];
			if (!(*ppData)) {
				return false;
			}

			hr = m_pCAP->GetDIB(*ppData, (DWORD*)&size);
			if (FAILED(hr)) {
				OnPlayPause();
				GetMediaState(); // Pause and retry to support ffdshow queuing.
				int retry = 0;
				while (FAILED(hr) && retry < 20) {
					hr = m_pCAP->GetDIB(*ppData, (DWORD*)&size);
					if (SUCCEEDED(hr)) {
						break;
					}
					Sleep(1);
					retry++;
				}
				if (FAILED(hr)) {
					errmsg.Format(ResStr(IDS_MAINFRM_49), hr);
					break;
				}
			}
		} else if (m_pMFVDC) {
			// Capture with EVR
			BITMAPINFOHEADER	bih = {sizeof(BITMAPINFOHEADER)};
			BYTE*				pDib;
			DWORD				dwSize;
			REFERENCE_TIME		rtImage = 0;
			hr = m_pMFVDC->GetCurrentImage (&bih, &pDib, &dwSize, &rtImage);
			if (FAILED(hr) || dwSize == 0) {
				errmsg.Format(ResStr(IDS_MAINFRM_51), hr);
				break;
			}

			size = (long)dwSize + sizeof(BITMAPINFOHEADER);
			*ppData = DNew BYTE[size];
			if (!(*ppData)) {
				return false;
			}
			memcpy_s(*ppData, size, &bih, sizeof(BITMAPINFOHEADER));
			memcpy_s(*ppData+sizeof(BITMAPINFOHEADER), size - sizeof(BITMAPINFOHEADER), pDib, dwSize);
			CoTaskMemFree(pDib);
		} else {
			hr = m_pBV->GetCurrentImage(&size, NULL);
			if (FAILED(hr) || size == 0) {
				errmsg.Format(ResStr(IDS_MAINFRM_51), hr);
				break;
			}

			*ppData = DNew BYTE[size];
			if (!(*ppData)) {
				return false;
			}

			hr = m_pBV->GetCurrentImage(&size, (long*)*ppData);
			if (FAILED(hr)) {
				errmsg.Format(ResStr(IDS_MAINFRM_51), hr);
				break;
			}
		}
	} while (0);

	if (!fSilent) {
		if (!errmsg.IsEmpty()) {
			AfxMessageBox(errmsg, MB_OK);
		}
	}

	if (fs == State_Running && GetMediaState() != State_Running) {
		m_pMC->Run();
	}

	if (FAILED(hr)) {
		if (*ppData) {
			ASSERT(0);
			delete [] *ppData;
			*ppData = NULL;
		}
		return false;
	}

	return true;
}

void CMainFrame::SaveDIB(LPCTSTR fn, BYTE* pData, long size)
{
	AppSettings& s = AfxGetAppSettings();

	CString ext = GetFileExt(fn).MakeLower();

	if (ext == _T(".bmp")) {
		BMPDIB(fn, pData, L"", 0, 0, 0, 0);
	} else if (ext == _T(".png")) {
		PNGDIB(fn, pData, max(1, min(9, s.iThumbLevelPNG)));
	} else if (ext == _T(".jpg")) {
		BMPDIB(fn, pData, L"image/jpeg", s.iThumbQuality, 0, 0, 0);
	}

	CString fName(fn);
	fName.Replace(_T("\\\\"), _T("\\"));

	CPath p(fName);
	if (CDC* pDC = m_wndStatusBar.GetDC()) {
		CRect r;
		m_wndStatusBar.GetClientRect(r);
		p.CompactPath(pDC->m_hDC, r.Width());
		m_wndStatusBar.ReleaseDC(pDC);
	}

	SendStatusMessage((LPCTSTR)p, 3000);
}

void CMainFrame::SaveImage(LPCTSTR fn)
{
	BYTE* pData = NULL;
	long size = 0;

	if (GetDIB(&pData, size)) {
		SaveDIB(fn, pData, size);
		delete [] pData;

		m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_IMAGE_SAVED), 3000);
	}
}

void CMainFrame::SaveThumbnails(LPCTSTR fn)
{
	if (!m_pMC || !m_pMS || GetPlaybackMode() != PM_FILE /*&& GetPlaybackMode() != PM_DVD*/) {
		return;
	}

	REFERENCE_TIME rtPos = GetPos();
	REFERENCE_TIME rtDur = GetDur();

	if (rtDur <= 0) {
		AfxMessageBox(ResStr(IDS_MAINFRM_54));
		return;
	}

	//pMC->Pause();

	if (GetMediaState() != State_Paused) {
		SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	}
	GetMediaState(); // wait for completion of the pause command

	//

	CSize framesize, dar;

	if (m_pCAP) {
		framesize = m_pCAP->GetVideoSize(false);
		dar = m_pCAP->GetVideoSize(true);
	} else if (m_pMFVDC) {
		m_pMFVDC->GetNativeVideoSize(&framesize, &dar);
	} else {
		m_pBV->GetVideoSize(&framesize.cx, &framesize.cy);

		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = m_pBV;
		if (pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0) {
			dar.SetSize(arx, ary);
		}
	}

	if (framesize.cx <= 0 || framesize.cy <= 0) {
		AfxMessageBox(ResStr(IDS_MAINFRM_55));
		return;
	}

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if (GetPlaybackMode() == PM_DVD && SUCCEEDED(m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
		dar.SetSize(VATR.ulAspectX, VATR.ulAspectY);
	}

	if (dar.cx > 0 && dar.cy > 0) {
		framesize.cx = MulDiv(framesize.cy, dar.cx, dar.cy);
	}

	AppSettings& s = AfxGetAppSettings();

	const int infoheight = 70;
	const int margin	= 10;
	const int width		= min(max(s.iThumbWidth, 256), 2560);
	const int cols		= min(max(s.iThumbCols, 1), 10);
	const int rows		= min(max(s.iThumbRows, 1), 20);

	CSize thumbsize;
	thumbsize.cx		= (width - margin) / cols - margin;
	thumbsize.cy		= MulDiv(thumbsize.cx, framesize.cy, framesize.cx);

	const int height	= infoheight + margin + (thumbsize.cy + margin) * rows;
	const int dibsize	= sizeof(BITMAPINFOHEADER) + width * height * 4;

	CAutoVectorPtr<BYTE> dib;
	if (!dib.Allocate(dibsize)) {
		AfxMessageBox(ResStr(IDS_MAINFRM_56));
		return;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(BYTE*)dib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize			= sizeof(BITMAPINFOHEADER);
	bih->biWidth		= width;
	bih->biHeight		= height;
	bih->biPlanes		= 1;
	bih->biBitCount		= 32;
	bih->biCompression	= BI_RGB;
	bih->biSizeImage	= DIBSIZE(*bih);
	memsetd(bih + 1, 0xffffff, bih->biSizeImage);

	SubPicDesc spd;
	spd.w		= width;
	spd.h		= height;
	spd.bpp		= 32;
	spd.pitch	= -width * 4;
	spd.vidrect	= CRect(0, 0, width, height);
	spd.bits	= (BYTE*)(bih + 1) + (width * 4) * (height - 1);

	{
		BYTE* p = (BYTE*)spd.bits;
		for (int y = 0; y < spd.h; y++, p += spd.pitch)
			for (int x = 0; x < spd.w; x++) {
				((DWORD*)p)[x] = 0x010101 * (0xe0 + 0x08*y/spd.h + 0x18*(spd.w-x)/spd.w);
			}
	}

	CCritSec csSubLock;
	RECT bbox;

	for (int i = 1, pics = cols*rows; i <= pics; i++) {
		REFERENCE_TIME rt = rtDur * i / (pics+1);
		DVD_HMSF_TIMECODE hmsf = RT2HMS_r(rt);

		SeekTo(rt);

		m_nVolumeBeforeFrameStepping = m_wndToolBar.Volume;
		m_pBA->put_Volume(-10000);

		// Number of steps you need to do more than one for some decoders.
		// TODO - maybe need to find another way to get correct frame ???
		HRESULT hr = m_pFS ? m_pFS->Step(2, NULL) : E_FAIL;

		if (FAILED(hr)) {
			m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);
			AfxMessageBox(ResStr(IDS_FRAME_STEP_ERROR_RENDERER), MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		HANDLE hGraphEvent = NULL;
		m_pME->GetEventHandle((OAEVENT*)&hGraphEvent);

		while (hGraphEvent && WaitForSingleObject(hGraphEvent, INFINITE) == WAIT_OBJECT_0) {
			LONG evCode = 0;
			LONG_PTR evParam1, evParam2;
			while (m_pME && SUCCEEDED(m_pME->GetEvent(&evCode, &evParam1, &evParam2, 0))) {
				m_pME->FreeEventParams(evCode, evParam1, evParam2);
				if (EC_STEP_COMPLETE == evCode) {
					hGraphEvent = NULL;
				}
			}
		}

		m_pBA->put_Volume(m_nVolumeBeforeFrameStepping);

		int col = (i-1)%cols;
		int row = (i-1)/cols;

		CPoint p(margin + col * (thumbsize.cx + margin), infoheight + margin + row * (thumbsize.cy + margin));
		CRect r(p, thumbsize);

		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = DNew STSStyle();
		style->marginRect.SetRectEmpty();
		rts.AddStyle(_T("thumbs"), style);

		CStringW str;
		str.Format(L"{\\an7\\1c&Hffffff&\\4a&Hb0&\\bord1\\shad4\\be1}{\\p1}m %d %d l %d %d %d %d %d %d{\\p}",
				   r.left, r.top, r.right, r.top, r.right, r.bottom, r.left, r.bottom);
		rts.Add(str, true, 0, 1, _T("thumbs"));
		str.Format(L"{\\an3\\1c&Hffffff&\\3c&H000000&\\alpha&H80&\\fs16\\b1\\bord2\\shad0\\pos(%d,%d)}%02d:%02d:%02d",
				   r.right-5, r.bottom-3, hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
		rts.Add(str, true, 1, 2, _T("thumbs"));

		rts.Render(spd, 0, 25, bbox);

		BYTE* pData = NULL;
		long size = 0;
		if (!GetDIB(&pData, size)) {
			return;
		}

		BITMAPINFO* bi = (BITMAPINFO*)pData;

		if (bi->bmiHeader.biBitCount != 32) {
			CString str;
			str.Format(ResStr(IDS_MAINFRM_57), bi->bmiHeader.biBitCount);
			AfxMessageBox(str);
			delete [] pData;
			return;
		}

		int sw = bi->bmiHeader.biWidth;
		int sh = abs(bi->bmiHeader.biHeight);
		int sp = sw*4;
		const BYTE* src = pData + sizeof(bi->bmiHeader);
		if (bi->bmiHeader.biHeight >= 0) {
			src += sp*(sh-1);
			sp = -sp;
		}

		int dp = spd.pitch;
		BYTE* dst = (BYTE*)spd.bits + spd.pitch*r.top + r.left*4;

		for (DWORD h = r.bottom - r.top, y = 0, yd = (sh<<8)/h; h > 0; y += yd, h--) {
			DWORD yf = y&0xff;
			DWORD yi = y>>8;

			DWORD* s0 = (DWORD*)(src + (int)yi*sp);
			DWORD* s1 = (DWORD*)(src + (int)yi*sp + sp);
			DWORD* d = (DWORD*)dst;

			for (DWORD w = r.right - r.left, x = 0, xd = (sw<<8)/w; w > 0; x += xd, w--) {
				DWORD xf = x&0xff;
				DWORD xi = x>>8;

				DWORD c0 = s0[xi];
				DWORD c1 = s0[xi+1];
				DWORD c2 = s1[xi];
				DWORD c3 = s1[xi+1];

				c0 = ((c0&0xff00ff) + ((((c1&0xff00ff) - (c0&0xff00ff)) * xf) >> 8)) & 0xff00ff
					 | ((c0&0x00ff00) + ((((c1&0x00ff00) - (c0&0x00ff00)) * xf) >> 8)) & 0x00ff00;

				c2 = ((c2&0xff00ff) + ((((c3&0xff00ff) - (c2&0xff00ff)) * xf) >> 8)) & 0xff00ff
					 | ((c2&0x00ff00) + ((((c3&0x00ff00) - (c2&0x00ff00)) * xf) >> 8)) & 0x00ff00;

				c0 = ((c0&0xff00ff) + ((((c2&0xff00ff) - (c0&0xff00ff)) * yf) >> 8)) & 0xff00ff
					 | ((c0&0x00ff00) + ((((c2&0x00ff00) - (c0&0x00ff00)) * yf) >> 8)) & 0x00ff00;

				*d++ = c0;
			}

			dst += dp;
		}

		rts.Render(spd, 10000, 25, bbox);

		delete [] pData;
	}

	{
		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = DNew STSStyle();
		style->marginRect.SetRect(margin, margin, margin, height-infoheight-margin);
		rts.AddStyle(_T("thumbs"), style);

		CStringW str;
		str.Format(L"{\\an9\\fs%d\\b1\\bord0\\shad0\\1c&Hffffff&}%s", infoheight-10, width >= 550 ? L"MPC-BE" : L"MPC");

		rts.Add(str, true, 0, 1, _T("thumbs"), _T(""), _T(""), CRect(0,0,0,0), -1);

		DVD_HMSF_TIMECODE hmsf = RT2HMS_r(rtDur);

		CStringW fn = GetFileOnly(GetCurFileName());
		if (!m_youtubeFields.fname.IsEmpty()) {
			fn = GetAltFileName();
		}

		CStringW fs;
		WIN32_FIND_DATA wfd;
		HANDLE hFind = FindFirstFile(GetCurFileName(), &wfd);
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose(hFind);

			__int64 size = (__int64(wfd.nFileSizeHigh)<<32)|wfd.nFileSizeLow;
			const int MAX_FILE_SIZE_BUFFER = 65;
			WCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
			StrFormatByteSizeW(size, szFileSize, MAX_FILE_SIZE_BUFFER);
			CString szByteSize;
			szByteSize.Format(L"%I64d", size);
			fs.Format(ResStr(IDS_MAINFRM_58), szFileSize, FormatNumber(szByteSize));
		}

		CStringW ar;
		if (dar.cx > 0 && dar.cy > 0 && dar.cx != framesize.cx && dar.cy != framesize.cy) {
			ar.Format(L"(%d:%d)", dar.cx, dar.cy);
		}

		str.Format(ResStr(IDS_MAINFRM_59),
				   CompactPath(fn, 90), fs, framesize.cx, framesize.cy, ar, hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
		rts.Add(str, true, 0, 1, _T("thumbs"));

		rts.Render(spd, 0, 25, bbox);
	}

	SaveDIB(fn, (BYTE*)dib, dibsize);

	SeekTo(rtPos);

	m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_THUMBS_SAVED), 3000);
}

static CString MakeSnapshotFileName(LPCTSTR prefix)
{
	CTime t = CTime::GetCurrentTime();
	CString fn;
	fn.Format(_T("%s_[%s]%s"), prefix, t.Format(_T("%Y.%m.%d_%H.%M.%S")), AfxGetAppSettings().strSnapShotExt);
	return fn;
}

BOOL CMainFrame::IsRendererCompatibleWithSaveImage()
{
	BOOL result = TRUE;
	AppSettings& s = AfxGetAppSettings();

	if (m_fShockwaveGraph) {
		AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_SHOCKWAVE), MB_ICONEXCLAMATION | MB_OK);
		result = FALSE;
	} else if (s.iDSVideoRendererType == VIDRNDT_DS_OVERLAYMIXER) {
		AfxMessageBox(ResStr(IDS_SCREENSHOT_ERROR_OVERLAY), MB_ICONEXCLAMATION | MB_OK);
		result = FALSE;
	// the latest madVR build v0.84.0 now supports screenshots.
	} else if (s.iDSVideoRendererType == VIDRNDT_DS_MADVR) {
		CRegKey key;
		CString clsid = _T("{E1A8B82A-32CE-4B0D-BE0D-AA68C772E423}");

		TCHAR buff[256];
		ULONG len = sizeof(buff);
		memset(buff, 0, len);

		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("CLSID\\") + clsid + _T("\\InprocServer32"), KEY_READ)
				&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len)) {
			VS_FIXEDFILEINFO	FileInfo;
			FullFileInfo		fullFileInfo;
			if (CFileVersionInfo::Create(buff, FileInfo)) {
				result = FALSE;
				WORD versionLS = (WORD)FileInfo.dwFileVersionLS & 0x0000FFFF;
				WORD versionMS = (WORD)FileInfo.dwFileVersionMS & 0x0000FFFF;
				if ((versionLS == 0 && versionMS >= 84) || (versionLS > 1)) {
					result = TRUE;
				}
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
	CString posstr = _T("");
	if ((GetPlaybackMode() == PM_FILE) || (GetPlaybackMode() == PM_DVD)) {
		__int64 start, stop, pos;
		m_wndSeekBar.GetRange(start, stop);
		pos = m_wndSeekBar.GetPosReal();

		DVD_HMSF_TIMECODE tcNow = RT2HMSF(pos);
		DVD_HMSF_TIMECODE tcDur = RT2HMSF(stop);

		if (tcDur.bHours > 0 || (pos >= stop && tcNow.bHours > 0)) {
			posstr.Format(_T("%02d.%02d.%02d"), tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
		} else {
			posstr.Format(_T("%02d.%02d"), tcNow.bMinutes, tcNow.bSeconds);
		}
	}

	return posstr;
}

CString CMainFrame::CreateSnapShotFileName()
{
	CString path(AfxGetAppSettings().strSnapShotPath);
	if (!::PathFileExists(path)) {
		TCHAR szPath[MAX_PATH] = { 0 };
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, szPath))) {
			path = CString(szPath) + L"\\";
		}
	}

	CString prefix = _T("snapshot");
	if (GetPlaybackMode() == PM_FILE) {
		CString filename = GetFileOnly(GetCurFileName());
		FixFilename(filename); // need for URLs
		if (!m_youtubeFields.fname.IsEmpty()) {
			filename = GetAltFileName();
		}

		prefix.Format(_T("%s_snapshot_%s"), filename, GetVidPos());
	} else if (GetPlaybackMode() == PM_DVD) {
		prefix.Format(_T("snapshot_dvd_%s"), GetVidPos());
	}

	CPath psrc;
	psrc.Combine(path, MakeSnapshotFileName(prefix));

	return CString(psrc);
}

void CMainFrame::OnFileSaveImage()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CSaveImageDialog fd(
		s.iThumbQuality, s.iThumbLevelPNG,
		0, CreateSnapShotFileName(),
		_T("BMP - Windows Bitmap (*.bmp)|*.bmp|JPG - JPEG Image (*.jpg)|*.jpg|PNG - Portable Network Graphics (*.png)|*.png||"), GetModalParent());

	if (s.strSnapShotExt == _T(".bmp")) {
		fd.m_pOFN->nFilterIndex = 1;
	} else if (s.strSnapShotExt == _T(".jpg")) {
		fd.m_pOFN->nFilterIndex = 2;
	} else if (s.strSnapShotExt == _T(".png")) {
		fd.m_pOFN->nFilterIndex = 3;
	}

	if (fd.DoModal() != IDOK) {
		return;
	}

	if (fd.m_pOFN->nFilterIndex == 1) {
		s.strSnapShotExt = _T(".bmp");
	} else if (fd.m_pOFN->nFilterIndex == 2) {
		s.strSnapShotExt = _T(".jpg");
	} else if (fd.m_pOFN->nFilterIndex == 3) {
		s.strSnapShotExt = _T(".png");
	}

	s.iThumbQuality		= fd.m_quality;
	s.iThumbLevelPNG	= fd.m_levelPNG;

	CString pdst = fd.GetPathName();
	if (GetFileExt(pdst).MakeLower() != s.strSnapShotExt) {
		pdst = RenameFileExt(pdst, s.strSnapShotExt);
	}

	s.strSnapShotPath = GetFolderOnly(pdst);
	SaveImage(pdst);
}

void CMainFrame::OnFileSaveImageAuto()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	SaveImage(CreateSnapShotFileName());
}

void CMainFrame::OnUpdateFileSaveImage(CCmdUI* pCmdUI)
{
	OAFilterState fs = GetMediaState();
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly && (fs == State_Paused || fs == State_Running));
}

void CMainFrame::OnFileSaveThumbnails()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if (!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CPath psrc(s.strSnapShotPath);
	CStringW prefix = _T("thumbs");
	if (GetPlaybackMode() == PM_FILE) {
		CString path = GetFileOnly(GetCurFileName());
		if (!m_youtubeFields.fname.IsEmpty()) {
			path = GetAltFileName();
		}

		prefix.Format(_T("%s_thumbs"), path);
	}
	psrc.Combine(s.strSnapShotPath, MakeSnapshotFileName(prefix));

	CSaveThumbnailsDialog fd(
		s.iThumbRows, s.iThumbCols, s.iThumbWidth, s.iThumbQuality, s.iThumbLevelPNG,
		0, (LPCTSTR)psrc,
		_T("BMP - Windows Bitmap (*.bmp)|*.bmp|JPG - JPEG Image (*.jpg)|*.jpg|PNG - Portable Network Graphics (*.png)|*.png||"), GetModalParent());

	if (s.strSnapShotExt == _T(".bmp")) {
		fd.m_pOFN->nFilterIndex = 1;
	} else if (s.strSnapShotExt == _T(".jpg")) {
		fd.m_pOFN->nFilterIndex = 2;
	} else if (s.strSnapShotExt == _T(".png")) {
		fd.m_pOFN->nFilterIndex = 3;
	}

	if (fd.DoModal() != IDOK) {
		return;
	}

	if (fd.m_pOFN->nFilterIndex == 1) {
		s.strSnapShotExt = _T(".bmp");
	} else if (fd.m_pOFN->nFilterIndex == 2) {
		s.strSnapShotExt = _T(".jpg");
	} else if (fd.m_pOFN->nFilterIndex == 3) {
		s.strSnapShotExt = _T(".png");
	}

	s.iThumbRows		= fd.m_rows;
	s.iThumbCols		= fd.m_cols;
	s.iThumbWidth		= fd.m_width;
	s.iThumbQuality		= fd.m_quality;
	s.iThumbLevelPNG	= fd.m_levelPNG;

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

	int iProgress;
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
					&& !m_bAudioOnly
					&& (GetPlaybackMode() == PM_FILE /*|| GetPlaybackMode() == PM_DVD*/)
					&& !GetBufferingProgress(&iProgress));
	UNREFERENCED_PARAMETER(iProgress);

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

	static CString szFilter;
	for (size_t idx = 0; idx < Subtitle::subTypesExt.size(); idx++) {
		szFilter += (idx == 0 ? L"." : L" .") + CString(Subtitle::subTypesExt[idx]);
	}
	szFilter += L"|";

	for (size_t idx = 0; idx < Subtitle::subTypesExt.size(); idx++) {
		szFilter += (idx == 0 ? L"*." : L";*.") + CString(Subtitle::subTypesExt[idx]);
	}
	szFilter += L"||";

	CFileDialog fd(TRUE, NULL, NULL,
				   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT,
				   szFilter, GetModalParent(), 0);
	fd.m_ofn.lpstrInitialDir = GetFolderOnly(GetCurFileName()).AllocSysString();

	if (fd.DoModal() != IDOK) {
		return;
	}

	CAtlList<CString> fns;
	POSITION pos = fd.GetStartPosition();
	while (pos) {
		fns.AddTail(fd.GetNextPathName(pos));
	}

	if (m_pDVS) {
		if (SUCCEEDED(m_pDVS->put_FileName((LPWSTR)(LPCWSTR)fns.GetHead()))) {
			m_pDVS->put_SelectedLanguage(0);
			m_pDVS->put_HideSubtitles(true);
			m_pDVS->put_HideSubtitles(false);
		}
		return;
	}

	pos = fns.GetHeadPosition();
	while (pos) {
		CString fname = fns.GetNext(pos);
		ISubStream *pSubStream = NULL;
		if (LoadSubtitle(fname, &pSubStream)) {
			SetSubtitle(pSubStream); // the subtitle at the insert position according to LoadSubtitle()
			AddSubtitlePathsAddons(fname);
		}
	}
}

void CMainFrame::OnUpdateFileLoadSubtitle(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && /*m_pCAP &&*/ !m_bAudioOnly);
}

void CMainFrame::OnFileLoadAudio()
{
	if (m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar) || m_pFullscreenWnd->IsWindow()) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

	CString filter;
	CAtlArray<CString> mask;
	s.m_Formats.GetAudioFilter(filter, mask);

	CString path = AddSlash(GetFolderOnly(GetCurFileName()));

	CFileDialog fd(TRUE, NULL, NULL,
				   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT,
				   filter, GetModalParent(), 0);

	fd.m_ofn.lpstrInitialDir = path;

	if (fd.DoModal() != IDOK) {
		return;
	}

	CPlaylistItem* pli = m_wndPlaylistBar.GetCur();
	if (pli && pli->m_fns.GetCount()) {
		POSITION pos = fd.GetStartPosition();
		while (pos) {
			CString fname = fd.GetNextPathName(pos);

			if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB) {
				HRESULT hr = pGBA->RenderAudioFile(fname);
				if (SUCCEEDED(hr)) {
					pli->m_fns.AddTail(fname);
					AddAudioPathsAddons(fname);

					CComQIPtr<IAMStreamSelect> pSSa = FindSwitcherFilter();
					if (pSSa) {
						DWORD cStreams = 0;
						if (SUCCEEDED(pSSa->Count(&cStreams)) && cStreams > 0) {
							pSSa->Enable(cStreams - 1, AMSTREAMSELECTENABLE_ENABLE);
						}
					}
				}
			}
		}
	}

	m_pSwitcherFilter.Release();
	m_pSwitcherFilter = FindSwitcherFilter();
}

void CMainFrame::OnFileSaveSubtitle()
{
	if (m_pCurrentSubStream) {
		OpenMediaData *pOMD = m_wndPlaylistBar.GetCurOMD();
		CString suggestedFileName("");
		if (OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD)) {
			// HACK: get the file name from the current playlist item
			suggestedFileName = GetCurFileName();
			suggestedFileName = suggestedFileName.Left(suggestedFileName.ReverseFind('.'));	// exclude the extension, it will be auto completed
		}

		if (auto pVSF = dynamic_cast<CVobSubFile*>((ISubStream*)m_pCurrentSubStream)) {
			// remember to set lpszDefExt to the first extension in the filter so that the save dialog autocompletes the extension
			// and tracks attempts to overwrite in a graceful manner
			CFileDialog fd(FALSE, _T("idx"), suggestedFileName,
						   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
						   _T("VobSub (*.idx, *.sub)|*.idx;*.sub||"), GetModalParent());

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
			filter += _T("SubRip (*.srt)|*.srt|");
			filter += _T("SubStation Alpha (*.ssa)|*.ssa|");
			filter += _T("Advanced SubStation Alpha (*.ass)|*.ass|");
			filter += _T("MicroDVD (*.sub)|*.sub|");
			filter += _T("SAMI (*.smi)|*.smi|");
			filter += _T("PowerDivX (*.psb)|*.psb|");
			filter += _T("|");

			AppSettings& s = AfxGetAppSettings();

			// same thing as in the case of CVobSubFile above for lpszDefExt
			CSaveTextFileDialog fd(pRTS->m_encoding, _T("srt"), suggestedFileName, filter, GetModalParent(), FALSE, s.bSubSaveExternalStyleFile);

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
	ShellExecute(m_hWnd, _T("open"), CString(url+args), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnUpdateFileISDBSearch(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnFileISDBUpload()
{
	CStringA url = "http://" + AfxGetAppSettings().strISDb + "/ul.php?";
	CStringA args = makeargs(m_wndPlaylistBar.m_pl);
	ShellExecute(m_hWnd, _T("open"), CString(url+args), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnUpdateFileISDBUpload(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_wndPlaylistBar.GetCount() > 0);
}

void CMainFrame::OnFileISDBDownload()
{
	AppSettings& s = AfxGetAppSettings();
	filehash fh;
	if (!::mpc_filehash((CString)GetCurFileName(), fh)) {
		MessageBeep(UINT_MAX);
		return;
	}

	try {
		CStringA url = "http://" + s.strISDb + "/index.php?";
		CStringA args;
		args.Format("player=mpc&name[0]=%s&size[0]=%016I64x&hash[0]=%016I64x",
					UrlEncode(CStringA(fh.name), true), fh.size, fh.mpc_filehash);
		url.Append(args);

		CSubtitleDlDlg dlg(GetModalParent(), url);
		dlg.DoModal();
	} catch (CInternetException* ie) {
		ie->Delete();
		return;
	}
}

void CMainFrame::OnUpdateFileISDBDownload(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && GetPlaybackMode() != PM_CAPTURE && (m_pCAP || m_pDVS) && !m_bAudioOnly);
}

void CMainFrame::OnFileProperties()
{
	CPPageFileInfoSheet m_fileinfo(GetPlaybackMode() == PM_FILE ? GetCurFileName() : GetCurDVDPath(TRUE), this, GetModalParent());
	m_fileinfo.DoModal();
}

void CMainFrame::OnUpdateFileProperties(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED &&
				   (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD));
}

void CMainFrame::OnFileCloseMedia()
{
	CloseMedia();
}

void CMainFrame::OnUpdateViewTearingTest(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	bool supported = (s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ||
					  s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS ||
					  s.iDSVideoRendererType == VIDRNDT_DS_SYNC) &&
					 r.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D;

	pCmdUI->Enable(supported && m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(AfxGetMyApp()->m_Renderers.m_fTearingTest);
}

void CMainFrame::OnViewTearingTest()
{
	AfxGetMyApp()->m_Renderers.m_fTearingTest = !AfxGetMyApp()->m_Renderers.m_fTearingTest;
}

void CMainFrame::OnUpdateViewDisplayStats(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	bool supported = (s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ||
					  s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS ||
					  s.iDSVideoRendererType == VIDRNDT_DS_SYNC) &&
					 r.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D;

	pCmdUI->Enable(supported && m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(supported && (AfxGetMyApp()->m_Renderers.m_fDisplayStats));
}

void CMainFrame::OnViewResetStats()
{
	AfxGetMyApp()->m_Renderers.m_bResetStats = true; // Reset by "consumer"
}

void CMainFrame::OnViewDisplayStatsSC()
{
	if (!AfxGetMyApp()->m_Renderers.m_fDisplayStats) {
		AfxGetMyApp()->m_Renderers.m_bResetStats = true; // to Reset statistics on first call ...
	}

	++AfxGetMyApp()->m_Renderers.m_fDisplayStats;
	if (AfxGetMyApp()->m_Renderers.m_fDisplayStats > 3) {
		AfxGetMyApp()->m_Renderers.m_fDisplayStats = 0;
	}

	RepaintVideo();
}

void CMainFrame::OnUpdateViewD3DFullscreen(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	bool supported = ((s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ||
					   s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS ||
					   s.iDSVideoRendererType == VIDRNDT_DS_SYNC) &&
					  r.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D);

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(s.fD3DFullscreen);
}

void CMainFrame::OnUpdateViewDisableDesktopComposition(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	bool supported = ((s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ||
					   s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS ||
					   s.iDSVideoRendererType == VIDRNDT_DS_SYNC) &&
					  r.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D &&
					  (IsWinVista() || IsWinSeven()));

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(r.m_AdvRendSets.iVMRDisableDesktopComposition);
}

void CMainFrame::OnUpdateViewEnableFrameTimeCorrection(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	bool supported = ((s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM) &&
					  r.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D);

	pCmdUI->Enable(supported);
	pCmdUI->SetCheck(r.m_AdvRendSets.iEVREnableFrameTimeCorrection);
}

void CMainFrame::OnViewVSync()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.iVMR9VSync = !s.m_AdvRendSets.iVMR9VSync;
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.m_AdvRendSets.iVMR9VSync ? ResStr(IDS_OSD_RS_VSYNC_ON) : ResStr(IDS_OSD_RS_VSYNC_OFF));
}

void CMainFrame::OnViewVSyncAccurate()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.iVMR9VSyncAccurate = !s.m_AdvRendSets.iVMR9VSyncAccurate;
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.m_AdvRendSets.iVMR9VSyncAccurate ? ResStr(IDS_OSD_RS_ACCURATE_VSYNC_ON) : ResStr(IDS_OSD_RS_ACCURATE_VSYNC_OFF));
}

void CMainFrame::OnViewD3DFullScreen()
{
	AppSettings& s = AfxGetAppSettings();
	s.fD3DFullscreen = !s.fD3DFullscreen;
	s.SaveSettings();
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.fD3DFullscreen ? ResStr(IDS_OSD_RS_D3D_FULLSCREEN_ON) : ResStr(IDS_OSD_RS_D3D_FULLSCREEN_OFF));
}

void CMainFrame::OnViewDisableDesktopComposition()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.iVMRDisableDesktopComposition = !s.m_AdvRendSets.iVMRDisableDesktopComposition;
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.m_AdvRendSets.iVMRDisableDesktopComposition ? ResStr(IDS_OSD_RS_NO_DESKTOP_COMP_ON) : ResStr(IDS_OSD_RS_NO_DESKTOP_COMP_OFF));
}

void CMainFrame::OnViewAlternativeVSync()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.fVMR9AlterativeVSync = !s.m_AdvRendSets.fVMR9AlterativeVSync;
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.m_AdvRendSets.fVMR9AlterativeVSync ? ResStr(IDS_OSD_RS_ALT_VSYNC_ON) : ResStr(IDS_OSD_RS_ALT_VSYNC_OFF));
}

void CMainFrame::OnViewResetDefault()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.SetDefault();
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_OSD_RS_RESET_DEFAULT));
}

void CMainFrame::OnUpdateViewReset(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	bool supported = s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ||
					 s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS ||
					 s.iDSVideoRendererType == VIDRNDT_DS_SYNC;
	pCmdUI->Enable(supported);
}

void CMainFrame::OnViewEnableFrameTimeCorrection()
{
	CRenderersSettings& s = AfxGetAppSettings().m_RenderersSettings;
	s.m_AdvRendSets.iEVREnableFrameTimeCorrection = !s.m_AdvRendSets.iEVREnableFrameTimeCorrection;
	s.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT,
						 s.m_AdvRendSets.iEVREnableFrameTimeCorrection ? ResStr(IDS_OSD_RS_FT_CORRECTION_ON) : ResStr(IDS_OSD_RS_FT_CORRECTION_OFF));
}

void CMainFrame::OnViewVSyncOffsetIncrease()
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	CString strOSD;
	if (s.iDSVideoRendererType == VIDRNDT_DS_SYNC) {
		r.m_AdvRendSets.fTargetSyncOffset = r.m_AdvRendSets.fTargetSyncOffset - 0.5; // Yeah, it should be a "-"
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), r.m_AdvRendSets.fTargetSyncOffset);
	} else {
		++r.m_AdvRendSets.iVMR9VSyncOffset;
		strOSD.Format(ResStr(IDS_OSD_RS_VSYNC_OFFSET), r.m_AdvRendSets.iVMR9VSyncOffset);
	}
	r.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
}

void CMainFrame::OnViewVSyncOffsetDecrease()
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	CString strOSD;
	if (s.iDSVideoRendererType == VIDRNDT_DS_SYNC) {
		r.m_AdvRendSets.fTargetSyncOffset = r.m_AdvRendSets.fTargetSyncOffset + 0.5;
		strOSD.Format(ResStr(IDS_OSD_RS_TARGET_VSYNC_OFFSET), r.m_AdvRendSets.fTargetSyncOffset);
	} else {
		--r.m_AdvRendSets.iVMR9VSyncOffset;
		strOSD.Format(ResStr(IDS_OSD_RS_VSYNC_OFFSET), r.m_AdvRendSets.iVMR9VSyncOffset);
	}
	r.UpdateData(true);
	m_OSD.DisplayMessage(OSD_TOPRIGHT, strOSD);
}

void CMainFrame::OnViewRemainingTime()
{
	m_bRemainingTime = !m_bRemainingTime;

	if (!m_bRemainingTime) {
		m_OSD.ClearMessage();
	}

	OnTimer(TIMER_STREAMPOSPOLLER2);
}


CString CMainFrame::GetSystemLocalTime()
{
	CString strResult;

	SYSTEMTIME st;
	GetLocalTime(&st);
	strResult.Format(_T("%02d:%02d"),st.wHour,st.wMinute);

	return strResult;
}

void CMainFrame::OnUpdateViewOSDLocalTime(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable(s.fShowOSD && (m_iMediaLoadState != MLS_CLOSED));
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
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable(s.fShowOSD && (m_iMediaLoadState != MLS_CLOSED));
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

void CMainFrame::OnUpdateShaderToggle(CCmdUI* pCmdUI)
{
	if (m_shaderlabels.IsEmpty()) {
		pCmdUI->Enable(FALSE);
		m_bToggleShader = false;
		pCmdUI->SetCheck (0);
	} else {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck (m_bToggleShader);
	}
}

void CMainFrame::OnUpdateShaderToggleScreenSpace(CCmdUI* pCmdUI)
{
	if (m_shaderlabelsScreenSpace.IsEmpty()) {
		pCmdUI->Enable(FALSE);
		m_bToggleShaderScreenSpace = false;
		pCmdUI->SetCheck(0);
	} else {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bToggleShaderScreenSpace);
	}
}

void CMainFrame::OnShaderToggle()
{
	m_bToggleShader = !m_bToggleShader;
	if (m_bToggleShader) {
		SetShaders();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_65));
	} else {
		if (m_pCAP) {
			m_pCAP->SetPixelShader(NULL, NULL);
		}
		RepaintVideo();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_66));
	}
}

void CMainFrame::OnShaderToggleScreenSpace()
{
	m_bToggleShaderScreenSpace = !m_bToggleShaderScreenSpace;
	if (m_bToggleShaderScreenSpace) {
		SetShaders();
		m_OSD.DisplayMessage(OSD_TOPRIGHT, ResStr(IDS_MAINFRM_PPONSCR));
	} else {
		if (m_pCAP2) {
			m_pCAP2->SetPixelShader2(NULL, NULL, true);
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
	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	RestoreDefaultWindowRect();
}

void CMainFrame::OnUpdateFileClose(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED || m_iMediaLoadState == MLS_LOADING);
}

// view

void CMainFrame::OnViewCaptionmenu()
{
	AppSettings& s = AfxGetAppSettings();
	s.iCaptionMenuMode++;
	s.iCaptionMenuMode %= MODE_COUNT; // three states: normal->borderless->frame only->

	if (m_fFullScreen) {
		return;
	}

	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwFlags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER;

	DWORD dwMenuFlags = GetMenuBarVisibility();

	CRect wr;
	GetWindowRect(&wr);

	switch (s.iCaptionMenuMode) {
		case MODE_SHOWCAPTIONMENU:	// borderless -> normal
			dwAdd = WS_CAPTION | WS_THICKFRAME;
			dwMenuFlags = AFX_MBV_KEEPVISIBLE;
			wr.right  += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
			wr.bottom += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
			wr.bottom += GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
			break;

		case MODE_HIDEMENU:			// normal -> hidemenu
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			wr.bottom -= GetSystemMetrics(SM_CYMENU);
			break;

		case MODE_FRAMEONLY:		// hidemenu -> frameonly
			dwRemove = WS_CAPTION;
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			wr.right  -= 2;
			wr.bottom -= GetSystemMetrics(SM_CYCAPTION) + 2;
			break;

		case MODE_BORDERLESS:		// frameonly -> borderless
			dwRemove = WS_THICKFRAME;
			dwMenuFlags = AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10;
			wr.right  -= GetSystemMetrics(SM_CXSIZEFRAME) * 2 - 2;
			wr.bottom -= GetSystemMetrics(SM_CYSIZEFRAME) * 2 - 2;
			break;
	}

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	if (IsZoomed()) { // If the window is maximized, we want it to stay maximized.
		dwFlags |= SWP_NOSIZE;
	}
	
	SetMenuBarVisibility(dwMenuFlags);
	// NOTE: wr.left and wr.top are ignored due to SWP_NOMOVE flag
	SetWindowPos(NULL, wr.left, wr.top, wr.Width(), wr.Height(), dwFlags);

	MoveVideoWindow();
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
	pCmdUI->Enable(m_iMediaLoadState == MLS_CLOSED && m_iMediaLoadState != MLS_LOADED
				   || m_iMediaLoadState == MLS_LOADED /*&& (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_CAPTURE)*/);
}

void CMainFrame::OnViewEditListEditor()
{
	AppSettings& s = AfxGetAppSettings();

	if (s.fEnableEDLEditor || (AfxMessageBox(ResStr(IDS_MB_SHOW_EDL_EDITOR), MB_ICONQUESTION | MB_YESNO) == IDYES)) {
		s.fEnableEDLEditor = true;
		ShowControlBar(&m_wndEditListEditor, !m_wndEditListEditor.IsWindowVisible(), TRUE);
	}
}

void CMainFrame::OnEDLIn()
{
	if (AfxGetAppSettings().fEnableEDLEditor && (m_iMediaLoadState == MLS_LOADED) && (GetPlaybackMode() == PM_FILE)) {
		REFERENCE_TIME		rt;

		m_pMS->GetCurrentPosition(&rt);
		m_wndEditListEditor.SetIn(rt);
	}
}

void CMainFrame::OnUpdateEDLIn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_wndEditListEditor.IsWindowVisible());
}

void CMainFrame::OnEDLOut()
{
	if (AfxGetAppSettings().fEnableEDLEditor && (m_iMediaLoadState == MLS_LOADED) && (GetPlaybackMode() == PM_FILE)) {
		REFERENCE_TIME		rt;

		m_pMS->GetCurrentPosition(&rt);
		m_wndEditListEditor.SetOut(rt);
	}
}

void CMainFrame::OnUpdateEDLOut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_wndEditListEditor.IsWindowVisible());
}

void CMainFrame::OnEDLNewClip()
{
	if (AfxGetAppSettings().fEnableEDLEditor && (m_iMediaLoadState == MLS_LOADED) && (GetPlaybackMode() == PM_FILE)) {
		REFERENCE_TIME		rt;

		m_pMS->GetCurrentPosition(&rt);
		m_wndEditListEditor.NewClip(rt);
	}
}

void CMainFrame::OnUpdateEDLNewClip(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_wndEditListEditor.IsWindowVisible());
}

void CMainFrame::OnEDLSave()
{
	if (AfxGetAppSettings().fEnableEDLEditor && (m_iMediaLoadState == MLS_LOADED) && (GetPlaybackMode() == PM_FILE)) {
		m_wndEditListEditor.Save();
	}
}

void CMainFrame::OnUpdateEDLSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_wndEditListEditor.IsWindowVisible());
}

// Navigation menu
void CMainFrame::OnViewNavigation()
{
	AppSettings& s = AfxGetAppSettings();
	s.fHideNavigation = !s.fHideNavigation;
	m_wndNavigationBar.m_navdlg.UpdateElementList();
	if (GetPlaybackMode() == PM_CAPTURE) {
		ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
	}
}

void CMainFrame::OnUpdateViewNavigation(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!AfxGetAppSettings().fHideNavigation);
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1);
}

void CMainFrame::OnViewCapture()
{
	ShowControlBar(&m_wndCaptureBar, !m_wndCaptureBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewCapture(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptureBar.IsWindowVisible());
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_CAPTURE);
}

void CMainFrame::OnViewShaderEditor()
{
	ShowControlBar(&m_wndShaderEditorBar, !m_wndShaderEditorBar.IsWindowVisible(), TRUE);
	AfxGetAppSettings().fShadersNeedSave = true;
}

void CMainFrame::OnUpdateViewShaderEditor(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndShaderEditorBar.IsWindowVisible());
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnViewMinimal()
{
	while (AfxGetAppSettings().iCaptionMenuMode!=MODE_BORDERLESS) {
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_NONE);
}

void CMainFrame::OnUpdateViewMinimal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewCompact()
{
	while (AfxGetAppSettings().iCaptionMenuMode!=MODE_FRAMEONLY) {
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_SEEKBAR|CS_TOOLBAR);
}

void CMainFrame::OnUpdateViewCompact(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewNormal()
{
	while (AfxGetAppSettings().iCaptionMenuMode!=MODE_SHOWCAPTIONMENU) {
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	}
	ShowControls(CS_SEEKBAR|CS_TOOLBAR|CS_STATUSBAR);
}

void CMainFrame::OnUpdateViewNormal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewFullscreen()
{
	ToggleFullscreen(true, true);
}

void CMainFrame::OnViewFullscreenSecondary()
{
	ToggleFullscreen(true, false);
}

void CMainFrame::OnUpdateViewFullscreen(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly || m_fFullScreen);
	//pCmdUI->SetCheck(m_fFullScreen);
}

void CMainFrame::OnMoveWindowToPrimaryScreen()
{
	AppSettings& s = AfxGetAppSettings();
	if (m_fFullScreen) ToggleFullscreen(false,false);
	if (!IsD3DFullScreenMode()){
		HMONITOR hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
		int x, y;
		CRect r;
		GetClientRect(r);

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &mi);

		x = (mi.rcWork.left+mi.rcWork.right-r.Width())/2; // Center main window
		y = (mi.rcWork.top+mi.rcWork.bottom-r.Height())/2;

		SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
}

void CMainFrame::OnViewZoomAutoFit()
{
	ZoomVideoWindow(true, GetZoomAutoFitScale());
	m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_ZOOM_AUTO), 3000);
}

void CMainFrame::OnViewDefaultVideoFrame(UINT nID)
{
	AfxGetAppSettings().iDefaultVideoSize = nID - ID_VIEW_VF_HALF;
	m_ZoomX = m_ZoomY = 1;
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);

	if (AfxGetAppSettings().iDefaultVideoSize == (pCmdUI->m_nID - ID_VIEW_VF_HALF)) {
		CheckMenuRadioItem(ID_VIEW_VF_HALF, ID_VIEW_VF_ZOOM2, pCmdUI->m_nID);
	}
}

void CMainFrame::OnViewSwitchVideoFrame()
{
	AppSettings& s = AfxGetAppSettings();
	int vs = s.iDefaultVideoSize;
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
	s.iDefaultVideoSize = vs;
	m_ZoomX = m_ZoomY = 1;
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnViewKeepaspectratio()
{
	AppSettings& s = AfxGetAppSettings();
	s.fKeepAspectRatio = !s.fKeepAspectRatio;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fKeepAspectRatio);
}

void CMainFrame::OnViewCompMonDeskARDiff()
{
	AppSettings& s = AfxGetAppSettings();
	s.fCompMonDeskARDiff = !s.fCompMonDeskARDiff;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fCompMonDeskARDiff);
}

void CMainFrame::OnViewPanNScan(UINT nID)
{
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	int x = 0, y = 0;
	int dx = 0, dy = 0;

	switch (nID) {
		case ID_VIEW_RESET:
			m_ZoomX = m_ZoomY = 1.0;
			m_PosX = m_PosY = 0.5;
			m_AngleX = m_AngleY = m_AngleZ = 0;
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
		m_PosX = max(m_PosX - 0.005*m_ZoomX, 0);
	} else if (dx > 0 && m_PosX < 1) {
		m_PosX = min(m_PosX + 0.005*m_ZoomX, 1);
	}

	if (dy < 0 && m_PosY > 0) {
		m_PosY = max(m_PosY - 0.005*m_ZoomY, 0);
	} else if (dy > 0 && m_PosY < 1) {
		m_PosY = min(m_PosY + 0.005*m_ZoomY, 1);
	}

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);
}

void CMainFrame::OnViewPanNScanPresets(UINT nID)
{
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

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
	for (CString token = str.Tokenize(_T(","), i); !token.IsEmpty(); token = str.Tokenize(_T(","), i), j++) {
		float f = 0;
		if (_stscanf_s(token, _T("%f"), &f) != 1) {
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

	m_PosX = min(max(m_PosX, 0), 1);
	m_PosY = min(max(m_PosY, 0), 1);
	m_ZoomX = min(max(m_ZoomX, 0.2), 3);
	m_ZoomY = min(max(m_ZoomY, 0.2), 3);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI)
{
	int nID = pCmdUI->m_nID - ID_PANNSCAN_PRESETS_START;
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly && nID >= 0 && nID <= AfxGetAppSettings().m_pnspresets.GetCount());
}

void CMainFrame::OnViewRotate(UINT nID)
{
	if (!m_pCAP) {
		return;
	}

	switch (nID) {
	case ID_PANSCAN_ROTATEXP:
		m_AngleX += 2;
		if (m_AngleX >= 360) { m_AngleX -= 360; }
		break;
	case ID_PANSCAN_ROTATEXM:
		m_AngleX -= 2;
		if (m_AngleX <= -360) { m_AngleX += 360; }
		break;
	case ID_PANSCAN_ROTATEYP:
		m_AngleY += 2;
		if (m_AngleY >= 360) { m_AngleY -= 360; }
		break;
	case ID_PANSCAN_ROTATEYM:
		m_AngleY -= 2;
		if (m_AngleY <= -360) { m_AngleY += 360; }
		break;
	case ID_PANSCAN_ROTATEZP:
		m_AngleZ += 2;
		if (m_AngleZ >= 360) { m_AngleZ -= 360; }
		break;
	case ID_PANSCAN_ROTATEZM:
		m_AngleZ -= 2;
		if (m_AngleZ <= -360) { m_AngleZ += 360; }
		break;
	default:
		return;
	}

	m_pCAP->SetVideoAngle(Vector(Vector::DegToRad(m_AngleX), Vector::DegToRad(m_AngleY), Vector::DegToRad(m_AngleZ)));

	CString info;
	info.Format(_T("x: %d°, y: %d°, z: %d°"), m_AngleX, m_AngleY, m_AngleZ);
	SendStatusMessage(info, 3000);
}

void CMainFrame::OnUpdateViewRotate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly && m_pCAP);
}

// FIXME
const static SIZE s_ar[] = {{0, 0}, {4, 3}, {5, 4}, {16, 9}, {235, 100}, {185, 100}};

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
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly);

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
	ShowOptions();
}

// play

void CMainFrame::OnPlayPlay()
{
	if (m_iMediaLoadState == MLS_CLOSED) {
		m_bfirstPlay = false;
		OpenCurPlaylistItem();
		return;
	}

	if (m_iMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE) {
			if (m_fEndOfStream) {
				SendMessage(WM_COMMAND, ID_PLAY_STOP);
				SendMessage(WM_COMMAND, ID_PLAY_PLAYPAUSE);
			}
			m_pMS->SetRate(m_PlaybackRate);
			m_pMC->Run();
		} else if (GetPlaybackMode() == PM_DVD) {
			if (m_PlaybackRate >= 0.0) {
				m_pDVDC->PlayForwards(m_PlaybackRate, DVD_CMD_FLAG_Block, NULL);
			} else {
				m_pDVDC->PlayBackwards(abs(m_PlaybackRate), DVD_CMD_FLAG_Block, NULL);
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

	MoveVideoWindow();
	m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_PLAYING), 3000);
	SetPlayState(PS_PLAY);

	OnTimer(TIMER_STREAMPOSPOLLER);

	SetupEVRColorControl(); // can be configured when streaming begins

	CString strOSD;
	if (m_bfirstPlay) {
		m_bfirstPlay = false;

		if (!m_youtubeFields.title.IsEmpty()) {
			strOSD = m_youtubeFields.title;
		}

		if (strOSD.IsEmpty()) {
			if (GetPlaybackMode() == PM_FILE) {
				strOSD = GetCurFileName();
				if (m_LastOpenBDPath.GetLength() > 0) {
					strOSD = ResStr(ID_PLAY_PLAY);
					int i = strOSD.Find('\n');
					if (i > 0) {
						strOSD.Delete(i, strOSD.GetLength()-i);
					}
					strOSD.AppendFormat(L" %s", BLU_RAY);
					if (m_BDLabel.GetLength() > 0) {
						strOSD.AppendFormat(L" \"%s\"", m_BDLabel);
					} else {
						MakeBDLabel(GetCurFileName(), strOSD);
					}
				} else if (strOSD.GetLength() > 0) {
					strOSD.TrimRight('/');
					strOSD.Replace('\\', '/');
					strOSD = strOSD.Mid(strOSD.ReverseFind('/') + 1);
				}
			} else if (GetPlaybackMode() == PM_DVD) {
				strOSD = ResStr(ID_PLAY_PLAY);
				int i = strOSD.Find('\n');
				if (i > 0) {
					strOSD.Delete(i, strOSD.GetLength()-i);
				}
				strOSD += L" DVD";

				MakeDVDLabel(strOSD);
			}
		}
	}

	if (strOSD.IsEmpty()) {
		strOSD = ResStr(ID_PLAY_PLAY);
		int i = strOSD.Find('\n');
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
	}
	m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
}

void CMainFrame::OnPlayPauseI()
{
	OAFilterState fs = GetMediaState();

	if (m_iMediaLoadState == MLS_LOADED && fs == State_Stopped) {
		MoveVideoWindow();
	}

	if (m_iMediaLoadState == MLS_LOADED) {
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

	CString strOSD = ResStr(ID_PLAY_PAUSE);
	int i = strOSD.Find(_T("\n"));
	if (i > 0) {
		strOSD.Delete(i, strOSD.GetLength() - i);
	}
	m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
	m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_PAUSED), 3000);

	SetPlayState(PS_PAUSE);
}

void CMainFrame::OnPlayPause()
{
	// Support ffdshow queuing.
	// To avoid black out on pause, we have to lock g_ffdshowReceive to synchronize with ReceiveMine.
	if (queue_ffdshow_support) {
		CAutoLock lck(&g_ffdshowReceive);
		return OnPlayPauseI();
	}
	OnPlayPauseI();
}

void CMainFrame::OnPlayPlaypause()
{
	OAFilterState fs = GetMediaState();
	if (fs == State_Running) {
		SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	} else if (fs == State_Stopped || fs == State_Paused) {
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);
	}
}

void CMainFrame::OnPlayStop()
{
	if (m_iMediaLoadState == MLS_LOADED) {
		if (GetPlaybackMode() == PM_FILE) {
			LONGLONG pos = 0;
			m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			m_pMC->Stop();

			// BUG: after pause or stop the netshow url source filter won't continue
			// on the next play command, unless we cheat it by setting the file name again.
			//
			// Note: WMPx may be using some undocumented interface to restart streaming.

			BeginEnumFilters(m_pGB, pEF, pBF) {
				CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF;
				CComQIPtr<IFileSourceFilter> pFSF = pBF;
				if (pAMNS && pFSF) {
					WCHAR* pFN = NULL;
					AM_MEDIA_TYPE mt;
					if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
						pFSF->Load(pFN, NULL);
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
	}

	m_nLoops = 0;

	if (m_hWnd) {
		KillTimersStop();
		MoveVideoWindow();

		if (m_iMediaLoadState == MLS_LOADED) {
			__int64 start, stop;
			m_wndSeekBar.GetRange(start, stop);
			if (GetPlaybackMode() != PM_CAPTURE) {
				m_wndStatusBar.SetStatusTimer(m_wndSeekBar.GetPosReal(), stop, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
			}

			SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
		}
	}

	if (!m_fEndOfStream) {
		CString strOSD = ResStr(ID_PLAY_STOP);
		int i = strOSD.Find(_T("\n"));
		if (i > 0) {
			strOSD.Delete(i, strOSD.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, strOSD, 3000);
		m_Lcd.SetStatusMessage(ResStr(IDS_CONTROLS_STOPPED), 3000);
	} else {
		m_fEndOfStream = false;
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
	} else if (pCmdUI->m_nID == ID_PLAY_PLAY && m_wndPlaylistBar.GetCount() > 0) {
		fEnable = true;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayFrameStep(UINT nID)
{
	m_OSD.EnableShowMessage(false);

	if (m_pFS && nID == ID_PLAY_FRAMESTEP) {
		if (GetMediaState() != State_Paused && !queue_ffdshow_support) {
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
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

		m_pFS->Step(1, NULL);
	} else if (S_OK == m_pMS->IsFormatSupported(&TIME_FORMAT_FRAME)) {
		if (GetMediaState() != State_Paused) {
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
		}

		if (SUCCEEDED(m_pMS->SetTimeFormat(&TIME_FORMAT_FRAME))) {
			REFERENCE_TIME rtCurPos;

			if (SUCCEEDED(m_pMS->GetCurrentPosition(&rtCurPos))) {
				rtCurPos += (nID == ID_PLAY_FRAMESTEP) ? 1 : -1;

				m_pMS->SetPositions(&rtCurPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			}
			m_pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
		}
	} else {
		if (GetMediaState() != State_Paused) {
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
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

	if (m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly &&
			(GetPlaybackMode() != PM_DVD || m_iDVDDomain == DVD_DOMAIN_Title) &&
			GetPlaybackMode() != PM_CAPTURE &&
			!m_fLiveWM) {
		if (S_OK == m_pMS->IsFormatSupported(&TIME_FORMAT_FRAME)) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEP) {
			fEnable = TRUE;
		} else if (pCmdUI->m_nID == ID_PLAY_FRAMESTEPCANCEL && m_pFS && m_pFS->CanStep(0, NULL) == S_OK) {
			fEnable = TRUE;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlaySeek(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();

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

void CMainFrame::SetTimersPlay()
{
	SetTimer(TIMER_STREAMPOSPOLLER, 40, NULL);
	SetTimer(TIMER_STREAMPOSPOLLER2, 500, NULL);
	SetTimer(TIMER_STATS, 1000, NULL);
}

void CMainFrame::KillTimersStop()
{
	KillTimer(TIMER_STREAMPOSPOLLER2);
	KillTimer(TIMER_STREAMPOSPOLLER);
	KillTimer(TIMER_STATS);
}

static int rangebsearch(REFERENCE_TIME val, CAtlArray<REFERENCE_TIME>& rta)
{
	int i = 0, j = rta.GetCount() - 1, ret = -1;

	if (j >= 0 && val >= rta[j]) {
		return(j);
	}

	while (i < j) {
		int mid = (i + j) >> 1;
		REFERENCE_TIME midt = rta[mid];
		if (val == midt) {
			ret = mid;
			break;
		} else if (val < midt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (val > midt) {
			ret = mid;
			if (i == mid) {
				mid++;
			}
			i = mid;
		}
	}

	return(ret);
}

void CMainFrame::OnPlaySeekKey(UINT nID)
{
	if (!m_kfs.empty()) {
		bool bSeekingForward = (nID == ID_PLAY_SEEKKEYFORWARD);
		const REFERENCE_TIME rtPos = m_wndSeekBar.GetPos();
		REFERENCE_TIME rtSeekTo = rtPos - (bSeekingForward ? 0 : (GetMediaState() == State_Running) ? 10000000 : 10000);
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

	if (m_iMediaLoadState == MLS_LOADED && (fs == State_Paused || fs == State_Running)) {
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
	if ((m_iMediaLoadState != MLS_LOADED) || m_pFullscreenWnd->IsWindow()) {
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

	REFERENCE_TIME start, dur = -1;
	m_wndSeekBar.GetRange(start, dur);
	CGoToDlg dlg(m_wndSeekBar.GetPos(), dur, atpf > 0 ? (1.0/atpf) : 0);
	if (IDOK != dlg.DoModal() || dlg.m_time < 0) {
		return;
	}

	SeekTo(dlg.m_time);
}

void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (m_iMediaLoadState == MLS_LOADED) {
		fEnable = true;
		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) {
			fEnable = false;
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			fEnable = false;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayChangeRate(UINT nID)
{
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (GetPlaybackMode() == PM_CAPTURE) {
		if (GetMediaState() != State_Running) {
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);
		}

		long lChannelMin = 0, lChannelMax = 0;
		pAMTuner->ChannelMinMax(&lChannelMin, &lChannelMax);
		long lChannel = 0, lVivSub = 0, lAudSub = 0;
		pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);

		long lFreqOrg = 0, lFreqNew = -1;
		pAMTuner->get_VideoFrequency(&lFreqOrg);

		//long lSignalStrength;
		do {
			if (nID == ID_PLAY_DECRATE) {
				lChannel--;
			} else if (nID == ID_PLAY_INCRATE) {
				lChannel++;
			}

			//if (lChannel < lChannelMin) lChannel = lChannelMax;
			//if (lChannel > lChannelMax) lChannel = lChannelMin;

			if (lChannel < lChannelMin || lChannel > lChannelMax) {
				break;
			}

			if (FAILED(pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT))) {
				break;
			}

			long flFoundSignal;
			pAMTuner->AutoTune(lChannel, &flFoundSignal);

			pAMTuner->get_VideoFrequency(&lFreqNew);
		} while (FALSE);
		/*			SUCCEEDED(pAMTuner->SignalPresent(&lSignalStrength))
					&& (lSignalStrength != AMTUNER_SIGNALPRESENT || lFreqNew == lFreqOrg));*/
		return;
	}

	if (GetPlaybackMode() != PM_FILE && GetPlaybackMode() != PM_DVD || nID != ID_PLAY_INCRATE && nID != ID_PLAY_DECRATE) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

	HRESULT hr = E_FAIL;
	double PlaybackRate = 0;
	if (GetPlaybackMode() == PM_FILE) {
		if (nID == ID_PLAY_INCRATE) {
			PlaybackRate = GatNextRate(m_PlaybackRate, s.nSpeedStep / 100.0);
		} else if (nID == ID_PLAY_DECRATE) {
			PlaybackRate = GatPreviousRate(m_PlaybackRate, s.nSpeedStep / 100.0);
		}
		hr = m_pMS->SetRate(PlaybackRate);
	}
	else if (GetPlaybackMode() == PM_DVD) {
		if (nID == ID_PLAY_INCRATE) {
			PlaybackRate = GatNextDVDRate(m_PlaybackRate);
		} else if (nID == ID_PLAY_DECRATE) {
			PlaybackRate = GatPreviousDVDRate(m_PlaybackRate);
		}
		if (PlaybackRate >= 0.0) {
			hr = m_pDVDC->PlayForwards(PlaybackRate, DVD_CMD_FLAG_Block, NULL);
		} else {
			hr = m_pDVDC->PlayBackwards(abs(PlaybackRate), DVD_CMD_FLAG_Block, NULL);
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

	if (m_iMediaLoadState == MLS_LOADED) {
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
	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	if (m_PlaybackRate != 1.0) {
		HRESULT hr = E_FAIL;

		if (GetPlaybackMode() == PM_FILE) {
			hr = m_pMS->SetRate(1.0);
		} else if (GetPlaybackMode() == PM_DVD) {
			hr = m_pDVDC->PlayForwards(1.0, DVD_CMD_FLAG_Block, NULL);
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
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_PlaybackRate != 1.0);
}

void CMainFrame::SetAudioDelay(REFERENCE_TIME rtShift)
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB)) {
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
	if (CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB)) {
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
		CComPtr<IPropertyPage> pPP = DNew CInternalPropertyPageTempl<CPinInfoWnd>(NULL, &hr);
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
		if (IDOK != CShaderCombineDlg(m_shaderlabels, m_shaderlabelsScreenSpace, GetModalParent()).DoModal()) {
			return;
		}
	}

	SetShaders();
}

void CMainFrame::OnPlayAudio(UINT nID)
{
	int i = (int)nID - (ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;

	if (i == -1) {
		ShowOptions(CPPageAudio::IDD);
	} else if (i >= 0 && pSS) {
		pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnPlayAudioOption(UINT nID)
{
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);
	if (i == -1) {
 		ShowOptions(CPPageAudio::IDD);
	} else if (i == 0) {
		OnFileLoadAudio();
 	}
}

void CMainFrame::OnUpdatePlayAudioOption(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);

	if (i == -1) {
		pCmdUI->Enable(TRUE);
	} else if (i == 0) {
		pCmdUI->Enable(GetPlaybackMode() == PM_FILE/* && !m_bAudioOnly*/);
	}
}

void CMainFrame::OnUpdatePlayAudio(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;

	/*if (i == -1)
	{
		// TODO****
	}
	else*/
	if (i >= 0 && pSS) {
		DWORD flags = DWORD_MAX;

		if (SUCCEEDED(pSS->Info(i, NULL, &flags, NULL, NULL, NULL, NULL, NULL))) {
			if (flags & AMSTREAMSELECTINFO_EXCLUSIVE) {
				//pCmdUI->SetRadio(TRUE);
			} else if (flags & AMSTREAMSELECTINFO_ENABLED) {
				pCmdUI->SetCheck(TRUE);
			} else {
				pCmdUI->SetCheck(FALSE);
			}
		} else {
			pCmdUI->Enable(FALSE);
		}
	}
}

void CMainFrame::OnPlaySubtitles(UINT nID)
{
	int i = (int)nID - (10 + ID_SUBTITLES_SUBITEM_START); // currently the subtitles submenu contains 5 items, apart from the actual subtitles list

	AppSettings& s = AfxGetAppSettings();

	if (i == -10) {
		// options
		ShowOptions(CPPageSubtitles::IDD);
	} else if (i == -9) {
		OnFileLoadSubtitle();
	} else if (i == -8) {
		// styles
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
			int i = m_style.Find(_T("&"));
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
	} else if (i == -7) {
		// reload
		ReloadSubtitle();
	} else if (i == -6) {

		OnNavMixStreamSubtitleSelectSubMenu(-1, 2);

	} else if (i == -5) {
		// override default style
		// TODO: default subtitles style toggle here
		s.fUseDefaultSubtitlesStyle = !s.fUseDefaultSubtitlesStyle;
		UpdateSubtitle();
	} else if (i == -4) {
		if (m_pDVS) {
			bool fBuffer, fOnlyShowForcedSubs, fPolygonize;
			m_pDVS->get_VobSubSettings(&fBuffer, &fOnlyShowForcedSubs, &fPolygonize);
			fOnlyShowForcedSubs = !fOnlyShowForcedSubs;
			m_pDVS->put_VobSubSettings(fBuffer, fOnlyShowForcedSubs, fPolygonize);
			
			return;
		}

		s.fForcedSubtitles = !s.fForcedSubtitles;
		g_bForcedSubtitle = s.fForcedSubtitles;
		if (m_pCAP) {
			m_pCAP->Invalidate();
		}
	} else if (i == -3) {
		s.m_RenderersSettings.bStereoDisabled	= TRUE;
		s.m_RenderersSettings.bSideBySide		= FALSE;
		s.m_RenderersSettings.bTopAndBottom		= FALSE;

		if (m_pCAP) {
			m_pCAP->Invalidate();
		}

		CString osd = ResStr(IDS_SUBTITLES_STEREO);
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd, 3000);
	} else if (i == -2) {
		s.m_RenderersSettings.bStereoDisabled	= FALSE;
		s.m_RenderersSettings.bSideBySide		= TRUE;
		s.m_RenderersSettings.bTopAndBottom		= FALSE;

		if (m_pCAP) {
			m_pCAP->Invalidate();
		}

		CString osd = ResStr(IDS_SUBTITLES_STEREO);
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd, 3000);
	} else if (i == -1) {
		s.m_RenderersSettings.bStereoDisabled	= FALSE;
		s.m_RenderersSettings.bSideBySide		= FALSE;
		s.m_RenderersSettings.bTopAndBottom		= TRUE;

		if (m_pCAP) {
			m_pCAP->Invalidate();
		}

		CString osd = ResStr(IDS_SUBTITLES_STEREO);
		osd.AppendFormat(L": %s", ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd, 3000);
	}
}

void CMainFrame::OnUpdateNavMixSubtitles(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (1 + ID_NAVIGATE_SUBP_SUBITEM_START);

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		if (m_pDVS) {
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
				bool fHideSubtitles = false;
				m_pDVS->get_HideSubtitles(&fHideSubtitles);
				pCmdUI->Enable();
				if (i == -1) {
					pCmdUI->SetCheck(!fHideSubtitles);
				} else {
					pCmdUI->Enable(!fHideSubtitles);
				}
			}

			return;
		}

		pCmdUI->Enable(m_pCAP && !m_bAudioOnly);

		if (i == -1) {	// enabled
			pCmdUI->SetCheck(AfxGetAppSettings().fEnableSubtitles);
		} else if (i >= 0) {
			pCmdUI->Enable(AfxGetAppSettings().fEnableSubtitles);
		}
	}
}

void CMainFrame::OnUpdatePlaySubtitles(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (10 + ID_SUBTITLES_SUBITEM_START);

	AppSettings& s = AfxGetAppSettings();

	pCmdUI->Enable(m_pCAP && !m_bAudioOnly && GetPlaybackMode() != PM_DVD);

	if (i == -10) {
		pCmdUI->Enable(GetPlaybackMode() == PM_NONE || (m_pCAP && !m_pDVS && !m_bAudioOnly)) ;
	} else if (i == -9) {
		pCmdUI->Enable((m_pCAP || m_pDVS) && !m_bAudioOnly && GetPlaybackMode() != PM_DVD);
	} else if (i == -8) {
		// styles
		pCmdUI->Enable(FALSE);
		if (dynamic_cast<CRenderedTextSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
			pCmdUI->Enable(TRUE);
		}
	} else if (i == -7) {
		if (m_pDVS) {
			pCmdUI->SetCheck(FALSE);
			pCmdUI->Enable(FALSE);
			return;
		}
	} else if (i == -6) {
		// enabled
		if (m_pDVS) {
			bool fHideSubtitles = false;
			m_pDVS->get_HideSubtitles(&fHideSubtitles);

			pCmdUI->Enable();
			pCmdUI->SetCheck(!fHideSubtitles);
			return;
		}
		pCmdUI->SetCheck(s.fEnableSubtitles);
	} else if (i == -5) {
		// override
		if (m_pDVS) {
			pCmdUI->SetCheck(FALSE);
			pCmdUI->Enable(FALSE);
			return;
		}

		// TODO: foxX - default subtitles style toggle here; still wip
		pCmdUI->SetCheck(s.fUseDefaultSubtitlesStyle);
		pCmdUI->Enable(s.fEnableSubtitles && m_pCAP && !m_bAudioOnly && GetPlaybackMode() != PM_DVD);
	} else if (i == -4) {
		if (m_pDVS) {
			bool fOnlyShowForcedSubs = false;
			bool fBuffer, fPolygonize;
			m_pDVS->get_VobSubSettings(&fBuffer, &fOnlyShowForcedSubs, &fPolygonize);

			pCmdUI->Enable();
			pCmdUI->SetCheck(fOnlyShowForcedSubs);
			return;
		}

		pCmdUI->SetCheck(s.fForcedSubtitles);
		pCmdUI->Enable(s.fEnableSubtitles && m_pCAP && !m_bAudioOnly && GetPlaybackMode() != PM_DVD);
	} else if (i == -3) {
		if (s.m_RenderersSettings.bStereoDisabled) {
			CheckMenuRadioItem(ID_SUBTITLES_SUBITEM_START + 7, ID_SUBTITLES_SUBITEM_START + 9, pCmdUI->m_nID);
		}
		pCmdUI->Enable(TRUE);
		//BOOL bEnabled = s.fEnableSubtitles && m_pCAP && !m_pDVS && !m_bAudioOnly && GetPlaybackMode() != PM_DVD;
		//pCmdUI->Enable(bEnabled);
	} else if (i == -2) {
		if (s.m_RenderersSettings.bSideBySide) {
			CheckMenuRadioItem(ID_SUBTITLES_SUBITEM_START + 7, ID_SUBTITLES_SUBITEM_START + 9, pCmdUI->m_nID);
		}
		//BOOL bEnabled = s.fEnableSubtitles && m_pCAP && !m_pDVS && !m_bAudioOnly && GetPlaybackMode() != PM_DVD;
		//pCmdUI->Enable(bEnabled);
		pCmdUI->Enable(TRUE);
	} else if (i == -1) {
		if (s.m_RenderersSettings.bTopAndBottom) {
			CheckMenuRadioItem(ID_SUBTITLES_SUBITEM_START + 7, ID_SUBTITLES_SUBITEM_START + 9, pCmdUI->m_nID);
		}
		//BOOL bEnabled = s.fEnableSubtitles && m_pCAP && !m_pDVS && !m_bAudioOnly && GetPlaybackMode() != PM_DVD;
		//pCmdUI->Enable(bEnabled);
		pCmdUI->Enable(TRUE);
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
		if (!FAILED(pAMSS->Info(nID - i, NULL, NULL, NULL, &dwGroup, NULL, NULL, NULL))) {
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
					if (SUCCEEDED(pSSA->Info(n, NULL, &flags, NULL, NULL, NULL, NULL, NULL))) {
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
	m_navMixAudioMenu.DestroyMenu();
	SetupNavMixAudioSubMenu();
	OnMenu(&m_navMixAudioMenu);
}

void CMainFrame::OnMenuNavSubtitle()
{
	m_navMixSubtitleMenu.DestroyMenu();
	SetupNavMixSubtitleSubMenu();
	OnMenu(&m_navMixSubtitleMenu);
}

void CMainFrame::OnMenuNavAudioOptions()
{
	SetupAudioOptionSubMenu();
	OnMenu(&m_audiosMenu);
}

void CMainFrame::OnMenuNavSubtitleOptions()
{
	SetupSubtitlesSubMenu();
	OnMenu(&m_subtitlesMenu);
}

void CMainFrame::OnMenuNavJumpTo()
{
	m_chaptersMenu.DestroyMenu();
	SetupNavChaptersSubMenu();
	OnMenu(&m_chaptersMenu);
}

void CMainFrame::OnMenuRecentFiles()
{
	m_recentfilesMenu.DestroyMenu();
	SetupRecentFilesSubMenu();
	OnMenu(&m_recentfilesMenu);
}

void CMainFrame::OnUpdateMenuNavSubtitle(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (/*IsSomethingLoaded() && */m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly) {
		fEnable = true;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnUpdateMenuNavAudio(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if (/*IsSomethingLoaded() && */m_iMediaLoadState == MLS_LOADED/* && !m_bAudioOnly*/) {
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

	if (m_iMediaLoadState == MLS_LOADED) {
		m_pBA->put_Volume(m_wndToolBar.Volume);
		//strVolume.Format(L"Vol : %d dB", m_wndToolBar.Volume / 100);
		SendStatusMessage(strVolume, 3000);
	}

	m_OSD.DisplayMessage(OSD_TOPLEFT, strVolume, 3000);
	m_Lcd.SetVolume((m_wndToolBar.Volume > -10000 ? m_wndToolBar.m_volctrl.GetPos() : 1));
}

void CMainFrame::OnPlayVolumeGain(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();
	if (s.bAudioAutoVolumeControl) {
		return;
	}

	if (CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB)) {
		switch (nID) {
		case ID_VOLUME_GAIN_INC:
			s.fAudioGain_dB = floor(s.fAudioGain_dB * 2) / 2 + 0.5f;
			if (s.fAudioGain_dB > 10.0f) {
				s.fAudioGain_dB = 10.0f;
			}
			break;
		case ID_VOLUME_GAIN_DEC:
			s.fAudioGain_dB = ceil(s.fAudioGain_dB * 2) / 2 - 0.5f;
			if (s.fAudioGain_dB < -3.0f) {
				s.fAudioGain_dB = -3.0f;
			}
			break;
		case ID_VOLUME_GAIN_OFF:
			s.fAudioGain_dB = 0.0f;
			break;
		case ID_VOLUME_GAIN_MAX:
			s.fAudioGain_dB = 10.0f;
			break;
		}
		pASF->SetAudioGain(s.fAudioGain_dB);

		CString osdMessage;
		osdMessage.Format(IDS_GAIN_OSD, s.fAudioGain_dB);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnAutoVolumeControl()
{
	if (CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB)) {
		AppSettings& s = AfxGetAppSettings();

		s.bAudioAutoVolumeControl = !s.bAudioAutoVolumeControl;
		pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);

		CString osdMessage = ResStr(s.bAudioAutoVolumeControl ? IDS_OSD_AUTOVOLUMECONTROL_ON : IDS_OSD_AUTOVOLUMECONTROL_OFF);
		m_OSD.DisplayMessage(OSD_TOPLEFT, osdMessage);
	}
}

void CMainFrame::OnUpdateNormalizeVolume(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable();
}

void CMainFrame::OnPlayColor(UINT nID)
{
	if (m_pVMRMC9 || m_pMFVP) {
		AppSettings& s = AfxGetAppSettings();
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
			brightness ? tmp.Format(_T("%+d"), brightness) : tmp = _T("0");
			str.Format(IDS_OSD_BRIGHTNESS, tmp);
			break;

		case ID_COLOR_CONTRAST_INC:
			contrast += 2;
		case ID_COLOR_CONTRAST_DEC:
			contrast -= 1;
			SetColorControl(ProcAmp_Contrast, brightness, contrast, hue, saturation);
			contrast ? tmp.Format(_T("%+d"), contrast) : tmp = _T("0");
			str.Format(IDS_OSD_CONTRAST, tmp);
			break;

		case ID_COLOR_HUE_INC:
			hue += 2;
		case ID_COLOR_HUE_DEC:
			hue -= 1;
			SetColorControl(ProcAmp_Hue, brightness, contrast, hue, saturation);
			hue ? tmp.Format(_T("%+d"), hue) : tmp = _T("0");
			str.Format(IDS_OSD_HUE, tmp);
			break;

		case ID_COLOR_SATURATION_INC:
			saturation += 2;
		case ID_COLOR_SATURATION_DEC:
			saturation -= 1;
			SetColorControl(ProcAmp_Saturation, brightness, contrast, hue, saturation);
			saturation ? tmp.Format(_T("%+d"), saturation) : tmp = _T("0");
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
	AppSettings& s = AfxGetAppSettings();
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
			s.fNextInDirAfterPlayback = true;
			s.fNextInDirAfterPlaybackLooped = false;
			s.fExitAfterPlayback = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_NEXT);
			break;
		case ID_AFTERPLAYBACK_NEXT_LOOPED:
			s.fNextInDirAfterPlayback = true;
			s.fNextInDirAfterPlaybackLooped = true;
			s.fExitAfterPlayback = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_NEXT);
			break;
		case ID_AFTERPLAYBACK_EXIT:
			s.fExitAfterPlayback = true;
			s.fNextInDirAfterPlayback = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_EXIT);
			break;
		case ID_AFTERPLAYBACK_EVERYTIMEDONOTHING:
			s.fExitAfterPlayback = false;
			s.fNextInDirAfterPlayback = false;
			osdMsg = ResStr(IDS_AFTERPLAYBACK_DONOTHING);
			break;
	}

	m_OSD.DisplayMessage(OSD_TOPLEFT, osdMsg);
}

void CMainFrame::OnUpdateAfterplayback(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	bool fChecked = false;

	switch (pCmdUI->m_nID) {
		case ID_AFTERPLAYBACK_CLOSE:
			fChecked = !!(s.nCLSwitches & CLSW_CLOSE);
			break;
		case ID_AFTERPLAYBACK_STANDBY:
			fChecked = !!(s.nCLSwitches & CLSW_STANDBY);
			break;
		case ID_AFTERPLAYBACK_HIBERNATE:
			fChecked = !!(s.nCLSwitches & CLSW_HIBERNATE);
			break;
		case ID_AFTERPLAYBACK_SHUTDOWN:
			fChecked = !!(s.nCLSwitches & CLSW_SHUTDOWN);
			break;
		case ID_AFTERPLAYBACK_LOGOFF:
			fChecked = !!(s.nCLSwitches & CLSW_LOGOFF);
			break;
		case ID_AFTERPLAYBACK_LOCK:
			fChecked = !!(s.nCLSwitches & CLSW_LOCK);
			break;
		case ID_AFTERPLAYBACK_DONOTHING:
			fChecked = !(s.nCLSwitches & CLSW_AFTERPLAYBACK_MASK);
			break;
		case ID_AFTERPLAYBACK_EXIT:
			fChecked = !!s.fExitAfterPlayback;
			break;
		case ID_AFTERPLAYBACK_NEXT:
			fChecked = !!s.fNextInDirAfterPlayback && !s.fNextInDirAfterPlaybackLooped;
			break;
		case ID_AFTERPLAYBACK_NEXT_LOOPED:
			fChecked = !!s.fNextInDirAfterPlayback && !!s.fNextInDirAfterPlaybackLooped;
			break;
		case ID_AFTERPLAYBACK_EVERYTIMEDONOTHING:
			fChecked = !s.fExitAfterPlayback && !s.fNextInDirAfterPlayback;
			break;
	}

	if (fChecked) {
		if (pCmdUI->m_nID >= ID_AFTERPLAYBACK_EXIT && pCmdUI->m_nID <= ID_AFTERPLAYBACK_EVERYTIMEDONOTHING) {
			CheckMenuRadioItem(ID_AFTERPLAYBACK_EXIT, ID_AFTERPLAYBACK_EVERYTIMEDONOTHING, pCmdUI->m_nID);
		} else {
			CheckMenuRadioItem(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_DONOTHING, pCmdUI->m_nID);
		}
	}
}

// navigate
void CMainFrame::OnNavigateSkip(UINT nID)
{
	if (GetPlaybackMode() == PM_FILE) {
		SetupChapters();

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
				if (i < (int)nChapters) {
					m_pCB->ChapGet(i, &rt, &name);
				}
			}

			if (i >= 0 && (DWORD)i < nChapters) {
				SeekTo(rt);
				SendStatusMessage(ResStr(IDS_AG_CHAPTER2) + CString(name), 3000);

				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				CString m_strOSD;
				m_strOSD.Format(_T("%s/%s %s%d/%d - \"%s\""), ReftimeToString2(rt), ReftimeToString2(rtDur), ResStr(IDS_AG_CHAPTER2), i + 1, nChapters, name);
				m_OSD.DisplayMessage(OSD_TOPLEFT, m_strOSD, 3000);
				return;
			}
		}

		if (nID == ID_NAVIGATE_SKIPBACK) {
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACKFILE);
		} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARDFILE);
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		SetupChapters();

		m_PlaybackRate = 1.0;

		if (GetMediaState() != State_Running) {
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);
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
				m_pDVDC->PlayChapterInTitle(Location.TitleNum - 1, ulNumOfChapters, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
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
					m_pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
				} else {
					m_pDVDC->ReplayChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
				}
			}
		} else if (nID == ID_NAVIGATE_SKIPFORWARD) {
			if (Location.ChapterNum == ulNumOfChapters && Location.TitleNum < ulNumOfTitles) {
				m_pDVDC->PlayChapterInTitle(Location.TitleNum + 1, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
			} else {
				m_pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
			}
		}

		if ((m_pDVDI->GetCurrentLocation(&Location) == S_OK))
		{
			m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
			CString m_strTitle;
			m_strTitle.Format(IDS_AG_TITLE2, Location.TitleNum, ulNumOfTitles);
			__int64 start, stop;
			m_wndSeekBar.GetRange(start, stop);

			CString m_strOSD;
			if (stop > 0) {
				DVD_HMSF_TIMECODE stopHMSF = RT2HMS_r(stop);
				m_strOSD.Format(_T("%s/%s %s, %s%02d/%02d"), DVDtimeToString(Location.TimeCode, stopHMSF.bHours > 0), DVDtimeToString(stopHMSF),
								m_strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			} else {
				m_strOSD.Format(_T("%s, %s%02d/%02d"), m_strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			}

			m_OSD.DisplayMessage(OSD_TOPLEFT, m_strOSD, 3000);
		}

		/*
		if (nID == ID_NAVIGATE_SKIPBACK)
			pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block, NULL);
		else if (nID == ID_NAVIGATE_SKIPFORWARD)
			pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block, NULL);
		*/
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		if (AfxGetAppSettings().iDefaultCaptureDevice == 1) {
			CComQIPtr<IBDATuner> pTun = m_pGB;
			if (pTun) {
				int nCurrentChannel;
				AppSettings& s = AfxGetAppSettings();

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
	}
}

void CMainFrame::OnUpdateNavigateSkip(CCmdUI* pCmdUI)
{
	// moved to the timer callback function, that runs less frequent
	//if (GetPlaybackMode() == PM_FILE) SetupChapters();

	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
				   && ((GetPlaybackMode() == PM_DVD
						&& m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu
						&& m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu)
					   || (GetPlaybackMode() == PM_FILE  && !AfxGetAppSettings().fDontUseSearchInFolder)
					   || (GetPlaybackMode() == PM_FILE  && AfxGetAppSettings().fDontUseSearchInFolder && (m_wndPlaylistBar.GetCount() > 1 || m_pCB->ChapGetCount() > 1))
					   || (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing)));
}

void CMainFrame::OnNavigateSkipFile(UINT nID)
{
	if (GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_CAPTURE) {
		if (m_wndPlaylistBar.GetCount() == 1) {
			if (GetPlaybackMode() == PM_CAPTURE || AfxGetAppSettings().fDontUseSearchInFolder) {
				SendMessage(WM_COMMAND, ID_PLAY_STOP); // do not remove this, unless you want a circular call with OnPlayPlay()
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);
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
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
				   && ((GetPlaybackMode() == PM_FILE && (m_wndPlaylistBar.GetCount() > 1 || !AfxGetAppSettings().fDontUseSearchInFolder))
					   || (GetPlaybackMode() == PM_CAPTURE && !m_fCapturing && m_wndPlaylistBar.GetCount() > 1)));
}

void CMainFrame::OnNavigateMenu(UINT nID)
{
	nID -= ID_NAVIGATE_TITLEMENU;

	if (m_iMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_DVD) {
		return;
	}

	m_PlaybackRate = 1.0;

	if (GetMediaState() != State_Running) {
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);
	}

	m_pDVDC->ShowMenu((DVD_MENU_ID)(nID+2), DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
}

void CMainFrame::OnUpdateNavigateMenu(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_TITLEMENU;
	ULONG ulUOPs;

	if (m_iMediaLoadState != MLS_LOADED || GetPlaybackMode() != PM_DVD
			|| FAILED(m_pDVDI->GetCurrentUOPS(&ulUOPs))) {
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(!(ulUOPs & (UOP_FLAG_ShowMenu_Title << nID)));
}

void CMainFrame::OnNavigateAudioMix(UINT nID)
{
	nID -= ID_NAVIGATE_AUDIO_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		OnNavMixStreamSelectSubMenu(nID, 1);
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectAudioStream(nID, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnNavigateSubpic(UINT nID)
{
	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		OnNavMixStreamSubtitleSelectSubMenu(nID - (ID_NAVIGATE_SUBP_SUBITEM_START + 1), 2);
	} else if (GetPlaybackMode() == PM_DVD) {
		int i = (int)nID - (ID_NAVIGATE_SUBP_SUBITEM_START + 1);

		if (i == -1) {
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			if (SUCCEEDED(m_pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))) {
				m_pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, NULL);
			}
		} else {
			m_pDVDC->SelectSubpictureStream(i, DVD_CMD_FLAG_Block, NULL);
			m_pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnNavMixStreamSubtitleSelectSubMenu(UINT id, DWORD dwSelGroup)
{
	bool bSplitterMenu = false;
	int splsubcnt = 0;
	int i = (int)id;

	if (GetPlaybackMode() == PM_FILE && m_pDVS) {
		if (i == -1) {
			bool fHideSubtitles = false;
			m_pDVS->get_HideSubtitles(&fHideSubtitles);
			fHideSubtitles = !fHideSubtitles;
			m_pDVS->put_HideSubtitles(fHideSubtitles);
		} else {
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

				int subcount = GetStreamCount(2);
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

						if (FAILED(pSS->Info(m, NULL, NULL, NULL, &dwGroup, NULL, NULL, NULL))) {
							continue;
						}

						if (dwGroup != 2) {
							continue;
						}

						if (id == 0) {
							pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
							return;
						}

						id--;
					}
				}

				if (i <= (nLangs-1)) {
					m_pDVS->put_SelectedLanguage(i);
				}
			}
		}

		return;
	}

	if ((GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) && i >= 0) {

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS) {
			DWORD cStreams = 0;
			if (!FAILED(pSS->Count(&cStreams))) {

				BOOL bIsHaali = FALSE;
				CComQIPtr<IBaseFilter> pBF = pSS;
				if (GetCLSID(pBF) == CLSID_HaaliSplitterAR || GetCLSID(pBF) == CLSID_HaaliSplitter) {
					bIsHaali = TRUE;
					cStreams--;
				}

				for (DWORD m = 0, j = cStreams; m < j; m++) {
					DWORD dwGroup = DWORD_MAX;

					if (FAILED(pSS->Info(m, NULL, NULL, NULL, &dwGroup, NULL, NULL, NULL))) {
						continue;
					}

					if (dwGroup != 2) {
						continue;
					}
					splsubcnt++;

					bSplitterMenu = true;
					if (id == 0) {
						pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
						break;
					}

					id--;
				}
			}
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectSubpictureStream(id, DVD_CMD_FLAG_Block, NULL);
		return;
	}

	if (i == -1) {
		AfxGetAppSettings().fEnableSubtitles = !AfxGetAppSettings().fEnableSubtitles;
		// enable

		if (!AfxGetAppSettings().fEnableSubtitles) {
			m_nSelSub2		= m_iSubtitleSel;
			m_iSubtitleSel	= -1;
		} else {
			m_iSubtitleSel = m_nSelSub2;
		}
		UpdateSubtitle();
	} else if (i >= 0) {
		int m = splsubcnt - (splsubcnt > 0 ? 1 : 0);
		m = i - m;

		m_iSubtitleSel = m;
		m_nSelSub2 = m_iSubtitleSel;
		if (!AfxGetAppSettings().fEnableSubtitles) {
			m_iSubtitleSel = -1;
		}
		UpdateSubtitle();
	}
}

void CMainFrame::OnNavigateAngle(UINT nID)
{
	nID -= ID_NAVIGATE_ANGLE_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		OnNavStreamSelectSubMenu(nID, 0);
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectAngle(nID+1, DVD_CMD_FLAG_Block, NULL);

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

		if (!m_MPLSPlaylist.IsEmpty() && id < m_MPLSPlaylist.GetCount()) {
			POSITION pos = m_MPLSPlaylist.GetHeadPosition();
			UINT idx = 0;
			while (pos) {
				CHdmvClipInfo::PlaylistItem* Item = m_MPLSPlaylist.GetNext(pos);
				if (idx == id) {
					m_bNeedUnmountImage = FALSE;
					SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
					m_bIsBDPlay = TRUE;

					CAtlList<CString> sl;
					sl.AddTail(Item->m_strFileName);
					m_wndPlaylistBar.Open(sl, false);
					OpenCurPlaylistItem(INVALID_TIME, !m_DiskImage.DriveAvailable());
					return;
				}
				idx++;
			}
		}

		if (!m_MPLSPlaylist.IsEmpty()) {
			id -= m_MPLSPlaylist.GetCount();
		}

		if (id >= 0 && id < (UINT)m_pCB->ChapGetCount() && m_pCB->ChapGetCount() > 1) {
			REFERENCE_TIME rt;
			CComBSTR name;
			if (SUCCEEDED(m_pCB->ChapGet(id, &rt, &name))) {
				SeekTo(rt);
				SendStatusMessage(ResStr(IDS_AG_CHAPTER2) + CString(name), 3000);

				REFERENCE_TIME rtDur;
				m_pMS->GetDuration(&rtDur);
				CString m_strOSD;
				m_strOSD.Format(_T("%s/%s %s%d/%d - \"%s\""), ReftimeToString2(rt), ReftimeToString2(rtDur), ResStr(IDS_AG_CHAPTER2), id+1, m_pCB->ChapGetCount(), name);
				m_OSD.DisplayMessage(OSD_TOPLEFT, m_strOSD, 3000);
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
			m_pDVDC->PlayTitle(nID, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL); // sometimes this does not do anything ...
			m_pDVDC->PlayChapterInTitle(nID, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL); // ... but this does!
		} else {
			nID -= ulNumOfTitles;

			if (nID <= ulNumOfChapters) {
				m_pDVDC->PlayChapter(nID, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
			}
		}

		if ((m_pDVDI->GetCurrentLocation(&Location) == S_OK)) {
			m_pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);
			CString m_strTitle;
			m_strTitle.Format(IDS_AG_TITLE2, Location.TitleNum, ulNumOfTitles);
			__int64 start, stop;
			m_wndSeekBar.GetRange(start, stop);

			CString m_strOSD;
			if (stop > 0) {
				DVD_HMSF_TIMECODE stopHMSF = RT2HMS_r(stop);
				m_strOSD.Format(_T("%s/%s %s, %s%02d/%02d"), DVDtimeToString(Location.TimeCode, stopHMSF.bHours > 0), DVDtimeToString(stopHMSF),
								m_strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			} else {
				m_strOSD.Format(_T("%s, %s%02d/%02d"), m_strTitle, ResStr(IDS_AG_CHAPTER2), Location.ChapterNum, ulNumOfChapters);
			}

			m_OSD.DisplayMessage(OSD_TOPLEFT, m_strOSD, 3000);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		AppSettings& s = AfxGetAppSettings();

		nID -= ID_NAVIGATE_CHAP_SUBITEM_START;

		if (s.iDefaultCaptureDevice == 1) {
			CComQIPtr<IBDATuner>	pTun = m_pGB;
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
	nID -= ID_NAVIGATE_MENU_LEFT;

	if (GetPlaybackMode() == PM_DVD) {
		switch (nID) {
			case 0:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Left);
				break;
			case 1:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Right);
				break;
			case 2:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Upper);
				break;
			case 3:
				m_pDVDC->SelectRelativeButton(DVD_Relative_Lower);
				break;
			case 4:
				if (m_iDVDDomain != DVD_DOMAIN_Title) {	// for the remote control
					m_pDVDC->ActivateButton();
				} else {
					OnPlayPlay();
				}
				break;
			case 5:
				m_pDVDC->ReturnFromSubmenu(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
				break;
			case 6:
				m_pDVDC->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
				break;
			default:
				break;
		}
	} else if (GetPlaybackMode() == PM_FILE) {
		OnPlayPlay();
	}
}

void CMainFrame::OnUpdateNavigateMenuItem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_iMediaLoadState == MLS_LOADED) && ((GetPlaybackMode() == PM_DVD) || (GetPlaybackMode() == PM_FILE)));
}

void CMainFrame::OnTunerScan()
{
	CTunerScanDlg		Dlg;
	Dlg.DoModal();
}

void CMainFrame::OnUpdateTunerScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_iMediaLoadState == MLS_LOADED) &&
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
	CDVDStateStream() : CUnknown(NAME("CDVDStateStream"), NULL) {
		m_pos = 0;
	}

	DECLARE_IUNKNOWN;

	CAtlArray<BYTE> m_data;

	// ISequentialStream
	STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) {
		__int64 cbRead = min((__int64)(m_data.GetCount() - m_pos), (__int64)cb);
		cbRead = max(cbRead, 0);
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
			m_data.Add(*p++);
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
	AppSettings& s = AfxGetAppSettings();

	if (GetPlaybackMode() == PM_FILE) {
		CString fn = GetCurFileName();

		CString desc = fn;
		desc.Replace('\\', '/');
		int i = desc.Find(_T("://")), j = desc.Find(_T("?")), k = desc.ReverseFind('/');
		if (i >= 0) {
			desc = j >= 0 ? desc.Left(j) : desc;
		} else if (k >= 0) {
			desc = desc.Mid(k+1);
		}

		CAtlList<CString> descList;
		descList.AddTail(desc);

		if (m_LastOpenBDPath.GetLength() > 0) {
			CString fn2 = BLU_RAY;
			if (m_BDLabel.GetLength() > 0) {
				fn2.AppendFormat(L" \"%s\"", m_BDLabel);
			} else {
				MakeBDLabel(fn, fn2);
			}
			descList.AddHead(fn2);
		}

		CFavoriteAddDlg dlg(descList, fn);
		if (dlg.DoModal() != IDOK) {
			return;
		}

		// Name
		CString str = dlg.m_name;
		str.Remove(';');

		// RememberPos
		CString pos(_T("0"));
		if (dlg.m_bRememberPos) {
			pos.Format(_T("%I64d"), GetPos());
		}

		str += ';';
		str += pos;

		// RelativeDrive
		CString relativeDrive;
		relativeDrive.Format( _T("%d"), dlg.m_bRelativeDrive );

		str += ';';
		str += relativeDrive;

		// Paths
		if (m_LastOpenBDPath.GetLength() > 0) {
			str += _T(";") + m_LastOpenBDPath;
		} else {
			CPlaylistItem pli;
			if (m_wndPlaylistBar.GetCur(pli)) {
				POSITION pos = pli.m_fns.GetHeadPosition();
				while (pos) {
					str += _T(";") + pli.m_fns.GetNext(pos).GetName();
				}
			}
		}

		s.AddFav(FAV_FILE, str);
	} else if (GetPlaybackMode() == PM_DVD) {
		WCHAR path[MAX_PATH] = { 0 };
		ULONG len = 0;
		if (SUCCEEDED(m_pDVDI->GetDVDDirectory(path, _countof(path), &len))) {
			CString fn = path;
			fn.TrimRight(_T("/\\"));

			DVD_PLAYBACK_LOCATION2 Location;
			m_pDVDI->GetCurrentLocation(&Location);
			CString desc;
			desc.Format(_T("%s - T%02d C%02d - %02d:%02d:%02d"), fn, Location.TitleNum, Location.ChapterNum,
				Location.TimeCode.bHours, Location.TimeCode.bMinutes, Location.TimeCode.bSeconds);

			CAtlList<CString> fnList;
			fnList.AddTail(fn);

			CFavoriteAddDlg dlg(fnList, desc);
			if (dlg.DoModal() != IDOK) {
				return;
			}

			// Name
			CString str = dlg.m_name;
			str.Remove(';');

			// RememberPos
			CString pos(_T("0"));
			if (dlg.m_bRememberPos) {
				CDVDStateStream stream;
				stream.AddRef();

				CComPtr<IDvdState> pStateData;
				CComQIPtr<IPersistStream> pPersistStream;
				if (SUCCEEDED(m_pDVDI->GetState(&pStateData))
						&& (pPersistStream = pStateData)
						&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream))) {
					pos = BinToCString(stream.m_data.GetData(), stream.m_data.GetCount());
				}
			}

			str += ';';
			str += pos;

			// Paths
			str += ';';
			str += fn;

			s.AddFav(FAV_DVD, str);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		// TODO
	}
}

void CMainFrame::OnUpdateFavoritesAdd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetPlaybackMode() == PM_FILE || GetPlaybackMode() == PM_DVD);
}

// TODO: OnFavoritesAdd and OnFavoritesQuickAddFavorite use nearly the same code, do something about it
void CMainFrame::OnFavoritesQuickAddFavorite()
{
	AppSettings& s = AfxGetAppSettings();

	CString osdMsg;

	if (GetPlaybackMode() == PM_FILE) {
		CString fn = GetCurFileName();

		CString desc = fn;
		if (m_LastOpenBDPath.GetLength() > 0) {
			desc = BLU_RAY;
			if (m_BDLabel.GetLength() > 0) {
				desc.AppendFormat(L" \"%s\"", m_BDLabel);
			} else {
				MakeBDLabel(fn, desc);
			}
		} else {
			desc.Replace('\\', '/');
			int i = desc.Find(_T("://")), j = desc.Find(_T("?")), k = desc.ReverseFind('/');
			if (i >= 0) {
				desc = j >= 0 ? desc.Left(j) : desc;
			} else if (k >= 0) {
				desc = desc.Mid(k+1);
			}
		}

		CString fn_with_pos(desc);
		if (s.bFavRememberPos) {
			fn_with_pos.Format(_T("%s - %s"), desc, GetVidPos());    // Add file position (time format) so it will be easier to organize later
		}

		// Name
		CString str = fn_with_pos;
		str.Remove(';');

		// RememberPos
		CString pos(_T("0"));
		if (s.bFavRememberPos) {
			pos.Format(_T("%I64d"), GetPos());
		}

		str += ';';
		str += pos;

		// RelativeDrive
		CString relativeDrive;
		relativeDrive.Format( _T("%d"), s.bFavRelativeDrive );

		str += ';';
		str += relativeDrive;

		// Paths
		if (m_LastOpenBDPath.GetLength() > 0) {
			str += _T(";") + m_LastOpenBDPath;
		} else {
			CPlaylistItem pli;
			if (m_wndPlaylistBar.GetCur(pli)) {
				POSITION pos = pli.m_fns.GetHeadPosition();
				while (pos) {
					str += _T(";") + pli.m_fns.GetNext(pos).GetName();
				}
			}
		}

		s.AddFav(FAV_FILE, str);

		osdMsg = ResStr(IDS_FILE_FAV_ADDED);
	} else if (GetPlaybackMode() == PM_DVD) {
		WCHAR path[MAX_PATH] = { 0 };
		ULONG len = 0;
		if (SUCCEEDED(m_pDVDI->GetDVDDirectory(path, _countof(path), &len))) {
			CString fn = path;
			fn.TrimRight(_T("/\\"));

			DVD_PLAYBACK_LOCATION2 Location;
			m_pDVDI->GetCurrentLocation(&Location);
			CString desc;
			desc.Format(_T("%s - T%02d C%02d - %02d:%02d:%02d"), fn, Location.TitleNum, Location.ChapterNum,
				Location.TimeCode.bHours, Location.TimeCode.bMinutes, Location.TimeCode.bSeconds);

			// Name
			CString str = s.bFavRememberPos ? desc : fn;
			str.Remove(';');

			// RememberPos
			CString pos(_T("0"));
			if (s.bFavRememberPos) {
				CDVDStateStream stream;
				stream.AddRef();

				CComPtr<IDvdState> pStateData;
				CComQIPtr<IPersistStream> pPersistStream;
				if (SUCCEEDED(m_pDVDI->GetState(&pStateData))
						&& (pPersistStream = pStateData)
						&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream))) {
					pos = BinToCString(stream.m_data.GetData(), stream.m_data.GetCount());
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
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		// TODO
	}

	SendStatusMessage(osdMsg, 3000);
	m_OSD.DisplayMessage(OSD_TOPLEFT, osdMsg, 3000);
}

void CMainFrame::OnFavoritesOrganize()
{
	CFavoriteOrganizeDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnUpdateFavoritesOrganize(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();

	CAtlList<CString> sl;
	s.GetFav(FAV_FILE, sl);
	bool enable = !sl.IsEmpty();
	if (!enable) {
		s.GetFav(FAV_DVD, sl);
		enable = !sl.IsEmpty();
	}

	pCmdUI->Enable(enable);
}

void CMainFrame::OnRecentFileClear()
{
	if (IDYES != AfxMessageBox(ResStr(IDS_RECENT_FILES_QUESTION), MB_YESNO)) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

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
	HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, NULL, CLSCTX_INPROC_SERVER);
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

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	if (POSITION pos = sl.FindIndex(nID)) {
		PlayFavoriteFile(sl.GetAt(pos));
	}
}

void CMainFrame::PlayFavoriteFile(CString fav)
{
	CAtlList<CString> args;
	REFERENCE_TIME rtStart = 0;
	BOOL bRelativeDrive = FALSE;

	ExplodeEsc(fav, args, _T(';'));
	CString desc = args.RemoveHead(); // desc / name
	_stscanf_s(args.RemoveHead(), _T("%I64d"), &rtStart);    // pos
	_stscanf_s(args.RemoveHead(), _T("%d"), &bRelativeDrive);    // relative drive
	rtStart = max(rtStart, 0ll);

	// NOTE: This is just for the favorites but we could add a global settings that does this always when on. Could be useful when using removable devices.
	//       All you have to do then is plug in your 500 gb drive, full with movies and/or music, start MPC-BE (from the 500 gb drive) with a preloaded playlist and press play.
	if (bRelativeDrive) {
		// Get the drive MPC-BE is on and apply it to the path list
		CPath exeDrive(GetProgramPath(TRUE));

		if (exeDrive.StripToRoot()) {
			POSITION pos = args.GetHeadPosition();

			while (pos != NULL) {
				CString &stringPath = args.GetNext(pos);
				CPath path(stringPath);

				int rootLength = path.SkipRoot();

				if (path.StripToRoot()) {
					if (_tcsicmp(exeDrive, path) != 0) { // Do we need to replace the drive letter ?
						// Replace drive letter
						CString newPath(exeDrive);
						newPath += stringPath.Mid(rootLength);//newPath += stringPath.Mid( 3 );

						stringPath = newPath;
					}
				}
			}
		}
	}

	if (::PathIsDirectory(args.GetHead()) && OpenBD(args.GetHead(), rtStart)) {
		return;
	}

	m_wndPlaylistBar.Open(args, false);
	if (GetPlaybackMode() == PM_FILE && args.GetHead() == m_lastOMD->title) {
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

	if (OpenBD(str) || OpenIso(str)) {
		return;
	}

	if (!m_wndPlaylistBar.SelectFileInPlaylist(str)) {
		CAtlList<CString> fns;
		fns.AddTail(str);
		m_wndPlaylistBar.Open(fns, false);
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

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	if (POSITION pos = sl.FindIndex(nID)) {
		PlayFavoriteDVD(sl.GetAt(pos));
	}
}

void CMainFrame::PlayFavoriteDVD(CString fav)
{
	CAtlList<CString> args;
	CString fn;
	CDVDStateStream stream;

	stream.AddRef();

	ExplodeEsc(fav, args, _T(';'), 3);
	args.RemoveHeadNoReturn(); // desc / name
	CString state = args.RemoveHead(); // state
	if (state != _T("0")) {
	    CStringToBin(state, stream.m_data);
	}
	fn = args.RemoveHead(); // path

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	CComPtr<IDvdState> pDvdState;
	HRESULT hr = OleLoadFromStream((IStream*)&stream, IID_PPV_ARGS(&pDvdState));
	UNREFERENCED_PARAMETER(hr);

	AfxGetAppSettings().fNormalStartDVD = false;

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
	ShellExecute(m_hWnd, _T("open"), _T(MPC_VERSION_COMMENTS), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnHelpCheckForUpdate()
{
	UpdateChecker updatechecker;
	updatechecker.CheckForUpdate();
}

/*
void CMainFrame::OnHelpDocumentation()
{
	ShellExecute(m_hWnd, _T("open"), _T(""), NULL, NULL, SW_SHOWDEFAULT);
}
*/

void CMainFrame::OnHelpToolbarImages()
{
	ShellExecute(m_hWnd, _T("open"), _T("http://dev.mpc-next.ru/index.php?board=44.0"), NULL, NULL, SW_SHOWDEFAULT);
}

/*
void CMainFrame::OnHelpDonate()
{
	ShellExecute(m_hWnd, _T("open"), _T(""), NULL, NULL, SW_SHOWDEFAULT);
}
*/

void CMainFrame::OnSubtitlePos(UINT nID)
{
	if (m_pCAP) {
		AppSettings& s = AfxGetAppSettings();
		switch (nID) {
			case ID_SUB_POS_UP:
				s.m_RenderersSettings.nShiftPos.y--;
				break;
			case ID_SUB_POS_DOWN:
				s.m_RenderersSettings.nShiftPos.y++;
				break;
			case ID_SUB_POS_LEFT:
				s.m_RenderersSettings.nShiftPos.x--;
				break;
			case ID_SUB_POS_RIGHT:
				s.m_RenderersSettings.nShiftPos.x++;
				break;
			case ID_SUB_POS_RESTORE:
				s.m_RenderersSettings.nShiftPos.x = 0;
				s.m_RenderersSettings.nShiftPos.y = 0;
				break;
		}

		if (GetMediaState() != State_Running) {
			m_pCAP->Invalidate();
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
	AppSettings& s = AfxGetAppSettings();
	int w, h, x, y;

	if (s.HasFixedWindowSize()) {
		w = s.sizeFixedWindow.cx;
		h = s.sizeFixedWindow.cy;
	} else if (s.fRememberWindowSize) {
		w = s.rcLastWindowPos.Width();
		h = s.rcLastWindowPos.Height();
	} else {
		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		CSize logosize = m_wndView.GetLogoSize();
		int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
		int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

		if (GetSystemMetrics(SM_REMOTESESSION)) {
			_DEFCLIENTH = 0;
		}

		DWORD style = GetStyle();

		w = _DEFCLIENTW + r1.Width() - r2.Width();
		h = _DEFCLIENTH + r1.Height() - r2.Height();

		if (s.iCaptionMenuMode != MODE_BORDERLESS) {
			w += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
			h += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
			if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU || s.iCaptionMenuMode == MODE_HIDEMENU) {
				w -= 2;
				h -= 2;
			}
		}

		if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU || s.iCaptionMenuMode == MODE_HIDEMENU) {
			h += GetSystemMetrics(SM_CYCAPTION);
			if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
				h += GetSystemMetrics(SM_CYMENU);
			}
		}
	}

	bool inmonitor = false;
	if (s.fRememberWindowPos) {
		CMonitor monitor;
		CMonitors monitors;
		POINT ptA;
		ptA.x = s.rcLastWindowPos.TopLeft().x;
		ptA.y = s.rcLastWindowPos.TopLeft().y;
		inmonitor = (ptA.x<0 || ptA.y<0);
		if (!inmonitor) {
			for ( int i = 0; i < monitors.GetCount(); i++ ) {
				monitor = monitors.GetMonitor( i );
				if (monitor.IsOnMonitor(ptA)) {
					inmonitor = true;
					break;
				}
			}
		}
	}

	if (s.fRememberWindowPos && inmonitor) {
		x = s.rcLastWindowPos.TopLeft().x;
		y = s.rcLastWindowPos.TopLeft().y;
	} else {
		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

		if (iMonitor > 0) {
			iMonitor--;
			CAtlArray<HMONITOR> ml;
			EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&ml);
			if ((size_t)iMonitor < ml.GetCount()) {
				hMonitor = ml[iMonitor];
			}
		}

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &mi);

		x = (mi.rcWork.left + mi.rcWork.right - w) / 2; // Center main window
		y = (mi.rcWork.top + mi.rcWork.bottom - h) / 2; // no need to call CenterWindow()
	}

	UINT lastWindowType = s.nLastWindowType;
	MoveWindow(x, y, w, h);

	if (s.iCaptionMenuMode != MODE_SHOWCAPTIONMENU) {
		if (s.iCaptionMenuMode == MODE_FRAMEONLY) {
			ModifyStyle(WS_CAPTION, 0, SWP_NOZORDER);
		} else if (s.iCaptionMenuMode == MODE_BORDERLESS) {
			ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0, SWP_NOZORDER);
		}
		SetMenuBarVisibility(AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);
	}

	if (!s.fRememberWindowPos) {
		CenterWindow();
	}

	if (s.fRememberWindowSize && s.fRememberWindowPos) {
		if (lastWindowType == SIZE_MAXIMIZED) {
			ShowWindow(SW_MAXIMIZE);
		} else if (lastWindowType == SIZE_MINIMIZED) {
			ShowWindow(SW_MINIMIZE);
		}
	}

	if (s.fSavePnSZoom) {
		m_ZoomX = s.dZoomX;
		m_ZoomY = s.dZoomY;
	}
}

void CMainFrame::SetDefaultFullscreenState()
{
	AppSettings& s = AfxGetAppSettings();

	// Waffs : fullscreen command line
	if (!(s.nCLSwitches & CLSW_ADD) && (s.nCLSwitches & CLSW_FULLSCREEN) && !s.slFiles.IsEmpty()) {
		ToggleFullscreen(true, true);
		SetCursor(NULL);
		s.nCLSwitches &= ~CLSW_FULLSCREEN;
		m_fFirstFSAfterLaunchOnFS = true;
	}
}

void CMainFrame::RestoreDefaultWindowRect()
{
	AppSettings& s = AfxGetAppSettings();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if (!m_fFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED
			//&& (GetExStyle()&WS_EX_APPWINDOW)
			&& !s.fRememberWindowSize) {
		int x, y, w, h;

		if (s.HasFixedWindowSize()) {
			w = s.sizeFixedWindow.cx;
			h = s.sizeFixedWindow.cy;
		} else {
			CRect r1, r2;
			GetClientRect(&r1);
			m_wndView.GetClientRect(&r2);

			CSize logosize = m_wndView.GetLogoSize();
			int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
			int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

			DWORD style = GetStyle();
			w = _DEFCLIENTW + r1.Width() - r2.Width();
			h = _DEFCLIENTH + r1.Height() - r2.Height();

			if (style & WS_THICKFRAME) {
				w += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
				h += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
				if ( (style & WS_CAPTION) == 0 ) {
					w -= 2;
					h -= 2;
				}
			}

			if (style & WS_CAPTION) {
				h += GetSystemMetrics(SM_CYCAPTION);
				if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
					h += GetSystemMetrics(SM_CYMENU);
				}
				//else MODE_HIDEMENU
			}
		}

		if (s.fRememberWindowPos) {
			x = s.rcLastWindowPos.TopLeft().x;
			y = s.rcLastWindowPos.TopLeft().y;
		} else {
			CRect r;
			GetWindowRect(r);

			x = r.CenterPoint().x - w/2; // Center window here
			y = r.CenterPoint().y - h/2; // no need to call CenterWindow()
		}

		MoveWindow(x, y, w, h);

		if (!s.fRememberWindowPos) {
			CenterWindow();
		}
	}
}

OAFilterState CMainFrame::GetMediaState()
{
	OAFilterState ret = -1;
	if (m_iMediaLoadState == MLS_LOADED) {
		m_pMC->GetState(0, &ret);
	}
	return ret;
}

void CMainFrame::SetPlaybackMode(int iNewStatus)
{
	m_iPlaybackMode = iNewStatus;
	if (m_wndNavigationBar.IsWindowVisible() && GetPlaybackMode() != PM_CAPTURE) {
		ShowControlBar(&m_wndNavigationBar, !m_wndNavigationBar.IsWindowVisible(), TRUE);
	}
}

CSize CMainFrame::GetVideoSize()
{
	bool fKeepAspectRatio = AfxGetAppSettings().fKeepAspectRatio;
	bool fCompMonDeskARDiff = AfxGetAppSettings().fCompMonDeskARDiff;

	CSize ret(0,0);
	if (m_iMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return ret;
	}

	CSize wh(0, 0), arxy(0, 0);

	if (m_pCAP) {
		wh = m_pCAP->GetVideoSize(false);
		arxy = m_pCAP->GetVideoSize(fKeepAspectRatio);
	} else if (m_pMFVDC) {
		m_pMFVDC->GetNativeVideoSize(&wh, &arxy);	// TODO : check AR !!
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

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if (GetPlaybackMode() == PM_DVD && SUCCEEDED(m_pDVDI->GetCurrentVideoAttributes(&VATR))) {
		arxy.SetSize(VATR.ulAspectX, VATR.ulAspectY);
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
	if (m_pFullscreenWnd->IsWindow()) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();
	CRect r;
	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwRemoveEx = 0, dwAddEx = 0;
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);

	HMONITOR hm		= MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	HMONITOR hm_cur = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

	CMonitors monitors;

	if (!m_fFullScreen) {
		if (s.bHidePlaylistFullScreen && m_wndPlaylistBar.IsVisible()) {
			m_wndPlaylistBar.SetHiddenDueToFullscreen(true);
			ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
		}

		if (!m_fFirstFSAfterLaunchOnFS) {
			GetWindowRect(&m_lastWindowRect);
		}
		if (s.AutoChangeFullscrRes.bEnabled == 1 && fSwitchScreenResWhenHasTo && (GetPlaybackMode() != PM_NONE)) {
			AutoChangeMonitorMode();
		}
		m_LastWindow_HM = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

		CString str;
		CMonitor monitor;
		if (s.strFullScreenMonitor == _T("Current")) {
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
		GetMonitorInfo(hm, &mi);
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
		if (!m_fFirstFSAfterLaunchOnFS) {
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

	m_fFullScreen		= !m_fFullScreen;

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	ModifyStyleEx(dwRemoveEx, dwAddEx, SWP_NOZORDER);

	static bool m_Change_Monitor = false;
	// try disable shader when move from one monitor to other ...
	if (m_fFullScreen) {
		m_Change_Monitor = (hm != hm_cur);
		if ((m_Change_Monitor) && (!m_bToggleShader)) {
			if (m_pCAP) {
				m_pCAP->SetPixelShader(NULL, NULL);
			}
		}
		if (m_Change_Monitor && m_bToggleShaderScreenSpace) {
			if (m_pCAP2) {
				m_pCAP2->SetPixelShader2(NULL, NULL, true);
			}
		}
	} else {
		if (m_Change_Monitor && m_bToggleShader) {
			if (m_pCAP) {
				m_pCAP->SetPixelShader(NULL, NULL);
			}
		}
	}

	if (m_fFullScreen) {
		m_fHideCursor = true;
		if (s.fShowBarsWhenFullScreen) {
			int nTimeOut = s.nShowBarsWhenFullScreenTimeOut;
			if (nTimeOut == 0) {
				ShowControls(CS_NONE, false);
				ShowControlBar(&m_wndNavigationBar, false, TRUE);
			} else if (nTimeOut > 0) {
				SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut * 1000, NULL);
				SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut * 1000, 2000), NULL);
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
		m_fHideCursor = false;
		ShowControls(s.nCS);
		if (GetPlaybackMode() == PM_CAPTURE && s.iDefaultCaptureDevice == 1) {
			ShowControlBar(&m_wndNavigationBar, !s.fHideNavigation, TRUE);
		}
	}

	m_bAudioOnly = fAudioOnly;

	if (m_fFirstFSAfterLaunchOnFS) { //Play started in Fullscreen
		if (s.fRememberWindowSize || s.fRememberWindowPos) {
			r = s.rcLastWindowPos;
			if (!s.fRememberWindowPos) {
				hm = MonitorFromPoint( CPoint( 0,0 ), MONITOR_DEFAULTTOPRIMARY );
				GetMonitorInfo(hm, &mi);
				CRect m_r = mi.rcMonitor;
				int left = m_r.left + (m_r.Width() - r.Width())/2;
				int top = m_r.top + (m_r.Height() - r.Height())/2;
				r = CRect(left, top, left + r.Width(), top + r.Height());
			}
			if (!s.fRememberWindowSize) {
				CSize vsize = GetVideoSize();
				r = CRect(r.left, r.top, r.left + vsize.cx, r.top + vsize.cy);
				ShowWindow(SW_HIDE);
			}
			SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
			if (!s.fRememberWindowSize) {
				ZoomVideoWindow();
				ShowWindow(SW_SHOW);
			}
		} else {
			if (m_LastWindow_HM != hm_cur) {
				GetMonitorInfo(m_LastWindow_HM, &mi);
				r = mi.rcMonitor;
				ShowWindow(SW_HIDE);
				SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
			}
			ZoomVideoWindow();
			if (m_LastWindow_HM != hm_cur) {
				ShowWindow(SW_SHOW);
			}
		}
		m_fFirstFSAfterLaunchOnFS = false;
	} else {
		SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	SetAlwaysOnTop(s.iOnTop);

	if (!m_fFullScreen) {
		SetMenuBarVisibility(s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU ?
							 AFX_MBV_KEEPVISIBLE : AFX_MBV_DISPLAYONFOCUS | AFX_MBV_DISPLAYONF10);
	}

	MoveVideoWindow();

	if ((m_Change_Monitor) && (!m_bToggleShader || !m_bToggleShaderScreenSpace)) { // Enabled shader ...
		SetShaders();
	}

	UpdateThumbarButton();
}

void CMainFrame::AutoChangeMonitorMode()
{
	AppSettings& s = AfxGetAppSettings();
	CStringW mf_hmonitor = s.strFullScreenMonitor;

	// EnumDisplayDevice... s.strFullScreenMonitorID == sDeviceID ???
	bool bHasRule = 0;
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	DWORD dev = 0; // device index
	int iMonValid = 0;
	CString sDeviceID, strMonName;
	while (EnumDisplayDevices(0, dev, &dd, 0)) {
		DISPLAY_DEVICE ddMon;
		ZeroMemory(&ddMon, sizeof(ddMon));
		ddMon.cb = sizeof(ddMon);
		DWORD devMon = 0;
		while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
				sDeviceID.Format(L"%s", ddMon.DeviceID);
				sDeviceID = sDeviceID.Mid (8, sDeviceID.Find (L"\\", 9) - 8);
				if (s.strFullScreenMonitorID == sDeviceID) {
					strMonName.Format(L"%s", ddMon.DeviceName);
					strMonName = strMonName.Left(12);
					mf_hmonitor = /*s.strFullScreenMonitor = */strMonName;
					iMonValid = 1;
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

	CStringW strCurFS = mf_hmonitor;
	dispmode dmCur;
	GetCurDispMode(dmCur, strCurFS);

	// Set Display Mode

	if (s.AutoChangeFullscrRes.bEnabled == 1 && iMonValid == 1) {
		double MediaFPS = 0.0;
		if (s.IsD3DFullscreen() && miFPS > 0.9) {
			MediaFPS = miFPS;
		} else {
			const REFERENCE_TIME rtAvgTimePerFrame = std::llround(GetAvgTimePerFrame() * 10000000i64);
			if (rtAvgTimePerFrame > 0) {
				MediaFPS = 10000000.0 / rtAvgTimePerFrame;
			}
		}

		for (int rs = 0; rs < MaxFpsCount ; rs++) {
			if (s.AutoChangeFullscrRes.dmFullscreenRes[rs].bValid
					&& s.AutoChangeFullscrRes.dmFullscreenRes[rs].bChecked
					&& MediaFPS >= s.AutoChangeFullscrRes.dmFullscreenRes[rs].vfr_from
					&& MediaFPS <= s.AutoChangeFullscrRes.dmFullscreenRes[rs].vfr_to) {

				SetDispMode(s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes, mf_hmonitor);
				return;
			}
		}

	} else if (s.AutoChangeFullscrRes.bEnabled == 2) {

		if (iMonValid == 1 && s.dFPS >= 1){
			for (int rs = 0; rs < MaxFpsCount ; rs++) {
				if (s.AutoChangeFullscrRes.dmFullscreenRes[rs].bValid
					&& s.AutoChangeFullscrRes.dmFullscreenRes[rs].bChecked
					&& s.dFPS >= s.AutoChangeFullscrRes.dmFullscreenRes[rs].vfr_from
					&& s.dFPS <= s.AutoChangeFullscrRes.dmFullscreenRes[rs].vfr_to) {

					if ((s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.size != dmCur.size)
						|| (s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.bpp != dmCur.bpp)
						|| (s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.freq != dmCur.freq)) {
							SetDispMode(s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes, mf_hmonitor);
							m_nWasSetDispMode = 1;

					} else if ((s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.size == s.dm_def.size)
						&& (s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.bpp == s.dm_def.bpp)
						&& (s.AutoChangeFullscrRes.dmFullscreenRes[rs].dmFSRes.freq == s.dm_def.freq)) {

						if (m_nWasSetDispMode == 1) {
							ChangeDisplaySettingsEx(mf_hmonitor, NULL, NULL, 0, NULL);
							m_nWasSetDispMode = 0;
						}
					}
					bHasRule = 1;
 					break;
				}
			}
			if (bHasRule == 0) {
				if ((dmCur.size == s.dm_def.size) && (dmCur.bpp == s.dm_def.bpp) && (dmCur.freq == s.dm_def.freq)) {
					ChangeDisplaySettingsEx(mf_hmonitor, NULL, NULL, 0, NULL);
				} else {
					SetDispMode(dmCur, mf_hmonitor);
				}
			}
		}
	}
}

void CMainFrame::MoveVideoWindow(bool fShowStats)
{
	if (m_iMediaLoadState == MLS_LOADED && !m_bAudioOnly && IsWindowVisible()) {
		CRect wr, wr2;

		if (m_pFullscreenWnd->IsWindow()) {
			m_pFullscreenWnd->GetClientRect(&wr);
		} else if (!m_fFullScreen) {
			m_wndView.GetClientRect(&wr);
		} else {
			GetWindowRect(&wr);
			// it's code need for work in fullscreen on secondary monitor;
			CRect r;
			m_wndView.GetWindowRect(&r);
			wr -= r.TopLeft();
		}

		CRect vr = CRect(0,0,0,0);

		OAFilterState fs = GetMediaState();
		if (fs == State_Paused || fs == State_Running || (fs == State_Stopped && m_fShockwaveGraph)) {
			CSize arxy = GetVideoSize();

			dvstype iDefaultVideoSize = (dvstype)AfxGetAppSettings().iDefaultVideoSize;

			CSize ws =
				iDefaultVideoSize == DVS_HALF ? CSize(arxy.cx/2, arxy.cy/2) :
				iDefaultVideoSize == DVS_NORMAL ? arxy :
				iDefaultVideoSize == DVS_DOUBLE ? CSize(arxy.cx*2, arxy.cy*2) :
				wr.Size();
			int w = ws.cx;
			int h = ws.cy;

			if (!m_fShockwaveGraph) {
				if (iDefaultVideoSize == DVS_FROMINSIDE || iDefaultVideoSize == DVS_FROMOUTSIDE ||
						iDefaultVideoSize == DVS_ZOOM1 || iDefaultVideoSize == DVS_ZOOM2) {
					int dh = ws.cy;
					int dw = MulDiv(dh, arxy.cx, arxy.cy);

					int i = 0;
					switch (iDefaultVideoSize) {
						case DVS_ZOOM1:
							i = 1;
							break;
						case DVS_ZOOM2:
							i = 2;
							break;
						case DVS_FROMOUTSIDE:
							i = 3;
							break;
					}
					int minw = dw;
					int maxw = dw;
					if (ws.cx < dw) {
						minw = ws.cx;
					} else if (ws.cx > dw) {
						maxw = ws.cx;
					}

					float scale = i / 3.0f;
					w = minw + (maxw - minw) * scale;
					h = MulDiv(w, arxy.cy, arxy.cx);
				}
			}

			CSize size(
				(int)(m_ZoomX*w),
				(int)(m_ZoomY*h));

			CPoint pos(
				(int)(m_PosX*(wr.Width()*3 - m_ZoomX*w) - wr.Width()),
				(int)(m_PosY*(wr.Height()*3 - m_ZoomY*h) - wr.Height()));

			vr = CRect(pos, size);
		}

		// What does this do exactly ?
		// Add comments when you add this kind of code !
		//wr |= CRect(0,0,0,0);
		//vr |= CRect(0,0,0,0);

		if (m_pCAP) {
			m_pCAP->SetPosition(wr, vr);
			m_pCAP->SetVideoAngle(Vector(Vector::DegToRad(m_AngleX), Vector::DegToRad(m_AngleY), Vector::DegToRad(m_AngleZ)));
		} else {
			HRESULT hr;
			hr = m_pBV->SetDefaultSourcePosition();
			hr = m_pBV->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
			hr = m_pVW->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());

			if (m_pMFVDC) {
				m_pMFVDC->SetVideoPosition (NULL, wr);
			}
		}

		m_wndView.SetVideoRect(wr);

		if (m_bUseSmartSeek && m_wndPreView) {
			m_wndPreView.GetVideoRect(&wr2);
			CRect vr2 = CRect(0,0,0,0);

			CSize ws2 = wr2.Size();
			int w2 = ws2.cx;
			int h2 = ws2.cy;

			CSize arxy = GetVideoSize();
			{
				int dh = ws2.cy;
				int dw = MulDiv(dh, arxy.cx, arxy.cy);

				int i = 0;
				int minw = dw;
				int maxw = dw;
				if (ws2.cx < dw) {
					minw = ws2.cx;
				} else if (ws2.cx > dw) {
					maxw = ws2.cx;
				}

				float scale = i / 3.0f;
				w2 = minw + (maxw - minw) * scale;
				h2 = MulDiv(w2, arxy.cy, arxy.cx);
			}

			CSize size2(
				(int)(w2),
				(int)(h2));

			CPoint pos2(
				(int)(m_PosX*(wr2.Width()*3 - w2) - wr2.Width()),
				(int)(m_PosY*(wr2.Height()*3 - h2) - wr2.Height()));

			vr2 = CRect(pos2, size2);

			if (m_pMFVDC_preview) {
				m_pMFVDC_preview->SetVideoPosition (NULL, wr2);
			}

			m_pBV_preview->SetDefaultSourcePosition();
			m_pBV_preview->SetDestinationPosition(vr2.left, vr2.top, vr2.Width(), vr2.Height());
			m_pVW_preview->SetWindowPosition(wr2.left, wr2.top, wr2.Width(), wr2.Height());
		}

		if (fShowStats && vr.Height() > 0) {
			CString info;
			info.Format(_T("Pos %.2f %.2f, Zoom %.2f %.2f, AR %.2f"), m_PosX, m_PosY, m_ZoomX, m_ZoomY, (float)vr.Width()/vr.Height());
			SendStatusMessage(info, 3000);
		}
	} else {
		m_wndView.SetVideoRect();
	}
}

void CMainFrame::HideVideoWindow(bool fHide)
{
	CRect wr;
	if (m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->GetClientRect(&wr);
	} else if (!m_fFullScreen) {
		m_wndView.GetClientRect(&wr);
	} else {
		GetWindowRect(&wr);

		// this code is needed to work in fullscreen on secondary monitor
		CRect r;
		m_wndView.GetWindowRect(&r);
		wr -= r.TopLeft();
	}

	CRect vr = CRect(0,0,0,0);
	if (m_pCAP) {
		if (fHide) {
			m_pCAP->SetPosition(wr, vr);    // hide
		} else {
			m_pCAP->SetPosition(wr, wr);    // show
		}
	}
}

void CMainFrame::ZoomVideoWindow(bool snap, double scale)
{
	if (m_iMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

	if (scale <= 0) {
		scale =
			s.iZoomLevel == 0 ? 0.5 :
			s.iZoomLevel == 1 ? 1.0 :
			s.iZoomLevel == 2 ? 2.0 :
			s.iZoomLevel == 3 ? GetZoomAutoFitScale() :
			1.0;
	}

	if (m_fFullScreen) {
		OnViewFullscreen();
	}

	MINMAXINFO mmi;
	OnGetMinMaxInfo(&mmi);

	CRect r;
	GetWindowRect(r);
	int w = 0, h = 0;

	if (!m_bAudioOnly) {
		CSize arxy = GetVideoSize();

		long lWidth = int(arxy.cx * scale + 0.5);
		long lHeight = int(arxy.cy * scale + 0.5);

		DWORD style = GetStyle();

		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		w = r1.Width() - r2.Width() + lWidth;
		h = r1.Height() - r2.Height() + lHeight;

		if (style & WS_THICKFRAME) {
			w += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
			h += GetSystemMetrics(SM_CYSIZEFRAME) * 2;
			if ( (style & WS_CAPTION) == 0 ) {
				w -= 2;
				h -= 2;
			}
		}

		if ( style & WS_CAPTION ) {
			h += GetSystemMetrics( SM_CYCAPTION );
			if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
				h += GetSystemMetrics( SM_CYMENU );
			}
			//else MODE_HIDEMENU
		}

		if (GetPlaybackMode() == PM_CAPTURE && !s.fHideNavigation && !m_fFullScreen && !m_wndNavigationBar.IsVisible()) {
			CSize r = m_wndNavigationBar.CalcFixedLayout(FALSE, TRUE);
			w += r.cx;
		}

		w = max(w, mmi.ptMinTrackSize.x);
		h = max(h, mmi.ptMinTrackSize.y);
	} else {
		w = r.Width(); // mmi.ptMinTrackSize.x;
		h = mmi.ptMinTrackSize.y;
	}

	// Prevention of going beyond the scopes of screen
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	w = min(w, (mi.rcWork.right - mi.rcWork.left));
	h = min(h, (mi.rcWork.bottom - mi.rcWork.top));

	if (!s.fRememberWindowPos) {
		bool isSnapped = false;

		if (snap && s.fSnapToDesktopEdges && m_bWasSnapped) { // check if snapped to edges
			isSnapped = (r.left == mi.rcWork.left) || (r.top == mi.rcWork.top)
						|| (r.right == mi.rcWork.right) || (r.bottom == mi.rcWork.bottom);
		}

		if (isSnapped) { // prefer left, top snap to right, bottom snap
			if (r.left == mi.rcWork.left) {}
			else if (r.right == mi.rcWork.right) {
				r.left = r.right - w;
			}

			if (r.top == mi.rcWork.top) {}
			else if (r.bottom == mi.rcWork.bottom) {
				r.top = r.bottom - h;
			}
		} else {	// center window
			CPoint cp = r.CenterPoint();
			r.left = cp.x - w/2;
			r.top = cp.y - h/2;
			m_bWasSnapped = false;
		}
	}

	r.right = r.left + w;
	r.bottom = r.top + h;

	if (r.right > mi.rcWork.right) {
		r.OffsetRect(mi.rcWork.right-r.right, 0);
	}
	if (r.left < mi.rcWork.left) {
		r.OffsetRect(mi.rcWork.left-r.left, 0);
	}
	if (r.bottom > mi.rcWork.bottom) {
		r.OffsetRect(0, mi.rcWork.bottom-r.bottom);
	}
	if (r.top < mi.rcWork.top) {
		r.OffsetRect(0, mi.rcWork.top-r.top);
	}

	if ((m_fFullScreen || !s.HasFixedWindowSize()) && !m_pFullscreenWnd->IsWindow()) {
		MoveWindow(r);
	}

	//ShowWindow(SW_SHOWNORMAL);

	MoveVideoWindow();
}

double CMainFrame::GetZoomAutoFitScale()
{
	if (m_iMediaLoadState != MLS_LOADED || m_bAudioOnly) {
		return 1.0;
	}

	const CAppSettings& s = AfxGetAppSettings();
	CSize arxy = GetVideoSize();

	double sx = s.nAutoFitFactor / 100.0 * m_rcDesktop.Width() / arxy.cx;
	double sy = s.nAutoFitFactor / 100.0 * m_rcDesktop.Height() / arxy.cy;

	return sx < sy ? sx : sy;
}

void CMainFrame::RepaintVideo()
{
	if (GetMediaState() != State_Running) {
		if (m_pCAP) {
			m_pCAP->Paint(false);
		} else if (m_pMFVDC) {
			m_pMFVDC->RepaintVideo();
		}
	}
}

void CMainFrame::SetShaders()
{
	if (!m_pCAP) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();

	CAtlStringMap<const AppSettings::Shader*> s2s;

	POSITION pos = s.m_shaders.GetHeadPosition();
	while (pos) {
		const AppSettings::Shader* pShader = &s.m_shaders.GetNext(pos);
		s2s[pShader->label] = pShader;
	}

	m_pCAP->SetPixelShader(NULL, NULL);
	if (m_pCAP2) {
		m_pCAP2->SetPixelShader2(NULL, NULL, true);
	}

	for (int i = 0; i < 2; ++i) {
		if (i == 0 && !m_bToggleShader) {
			continue;
		}
		if (i == 1 && !m_bToggleShaderScreenSpace) {
			continue;
		}
		CAtlList<CString> labels;

		CAtlList<CString> *pLabels;
		if (i == 0) {
			pLabels = &m_shaderlabels;
		} else {
			if (!m_pCAP2) {
				break;
			}
			pLabels = &m_shaderlabelsScreenSpace;
		}

		pos = pLabels->GetHeadPosition();
		while (pos) {
			const AppSettings::Shader* pShader = NULL;
			if (s2s.Lookup(pLabels->GetNext(pos), pShader)) {
				CStringA target = pShader->target;
				CStringA srcdata = pShader->srcdata;

				HRESULT hr;
				if (i == 0) {
					hr = m_pCAP->SetPixelShader(srcdata, target);
				} else {
					hr = m_pCAP2->SetPixelShader2(srcdata, target, true);
				}

				if (FAILED(hr)) {
					m_pCAP->SetPixelShader(NULL, NULL);
					if (m_pCAP2) {
						m_pCAP2->SetPixelShader2(NULL, NULL, true);
					}
					SendStatusMessage(ResStr(IDS_MAINFRM_73) + pShader->label, 3000);
					return;
				}

				labels.AddTail(pShader->label);
			}
		}

		if (m_iMediaLoadState == MLS_LOADED) {
			CString str = Implode(labels, '|');
			str.Replace(_T("|"), _T(", "));
			SendStatusMessage(ResStr(IDS_AG_SHADER) + str, 3000);
		}
	}
}

void CMainFrame::UpdateShaders(CString label)
{
	if (!m_pCAP) {
		return;
	}

	if (m_shaderlabels.GetCount() <= 1) {
		m_shaderlabels.RemoveAll();
	}

	if (m_shaderlabels.IsEmpty() && !label.IsEmpty()) {
		m_shaderlabels.AddTail(label);
	}

	bool fUpdate = m_shaderlabels.IsEmpty();

	POSITION pos = m_shaderlabels.GetHeadPosition();
	while (pos) {
		if (label == m_shaderlabels.GetNext(pos)) {
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

	if (m_iMediaLoadState == MLS_LOADED) {
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
	ASSERT(m_pGB == NULL);

	m_fCustomGraph = false;
	m_fShockwaveGraph = false;

	AppSettings& s = AfxGetAppSettings();

	m_pGB_preview = NULL;

	if (s.IsD3DFullscreen() &&
			((s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS) ||
			 (s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM) ||
			 (s.iDSVideoRendererType == VIDRNDT_DS_MADVR) ||
			 (s.iDSVideoRendererType == VIDRNDT_DS_SYNC))) {
		CreateFullScreenWindow();
		m_pVideoWnd		= m_pFullscreenWnd;
		m_bUseSmartSeek	= false;
		s.fIsFSWindow = true;
		SetTimer(TIMER_EXCLUSIVEBARHIDER, 1000, NULL);
	} else {
		m_pVideoWnd		= &m_wndView;

		m_bUseSmartSeek = s.fSmartSeek && !s.fD3DFullscreen;
		s.fIsFSWindow = false;
		if (OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD)) {
			CString fn = p->fns.GetHead();
			if (!fn.IsEmpty() && (fn.Find(_T("://")) >= 0)) { // disable SmartSeek for streaming data.
				m_bUseSmartSeek = false;
			}
		}
	}

	if (OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD)) {
		engine_t engine = s.GetRtspEngine(p->fns.GetHead());

		CStringA ct = GetContentType(p->fns.GetHead());

		if (ct == "video/x-ms-asf") {
			// TODO: put something here to make the windows media source filter load later
		} else if (ct == "application/x-shockwave-flash") {
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
			m_pGB = DNew CFGManagerPlayer(_T("CFGManagerPlayer"), NULL, m_pVideoWnd->m_hWnd);

			// Graph for preview
			if (m_bUseSmartSeek && m_wndPreView) {
				m_pGB_preview = DNew CFGManagerPlayer(_T("CFGManagerPlayer"), NULL, m_wndPreView.GetVideoHWND(), true);
			}
		}
	} else if (OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD)) {
		m_pGB = DNew CFGManagerDVD(_T("CFGManagerDVD"), NULL, m_pVideoWnd->m_hWnd);

		if (m_bUseSmartSeek && m_wndPreView) {
			m_pGB_preview = DNew CFGManagerDVD(_T("CFGManagerDVD"), NULL, m_wndPreView.GetVideoHWND(), true);
		}
	} else if (OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD)) {
		if (s.iDefaultCaptureDevice == 1) {
			m_pGB = DNew CFGManagerBDA(_T("CFGManagerBDA"), NULL, m_pVideoWnd->m_hWnd);
		} else {
			m_pGB = DNew CFGManagerCapture(_T("CFGManagerCapture"), NULL, m_pVideoWnd->m_hWnd);
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

	m_pCB = DNew CDSMChapterBag(NULL, NULL);

	return _T("");
}

HRESULT CMainFrame::PreviewWindowHide()
{
	HRESULT hr = S_OK;

	if (!(m_bUseSmartSeek && m_wndPreView)) {
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
	HRESULT hr = S_OK;

	if (!m_bUseSmartSeek || !AfxGetAppSettings().fSmartSeek || !m_wndPreView || m_bAudioOnly || m_pFullscreenWnd->IsWindow()) {
		return E_FAIL;
	}

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

			if (FAILED(hr)) {
				//LOG2FILE(_T("DVD - GetCurrentLocation() failed [%d : %d]"), Loc.TitleNum, Loc.ChapterNum);
			} else {
				//LOG2FILE(_T("DVD - TitleNum changed : Main :%d, Preview :%d"), Loc.TitleNum, Loc2.TitleNum);
			}

			hr = m_pDVDC_preview->PlayTitle(Loc.TitleNum, DVD_CMD_FLAG_Flush, NULL);
			if (FAILED(hr)) {
				return hr;
			}
			m_pDVDC_preview->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
			if (SUCCEEDED(hr)) {
				hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, NULL);
				if (FAILED(hr)) {
					return hr;
				}
			} else {
				hr = m_pDVDC_preview->PlayChapterInTitle(Loc.TitleNum, 1, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
				hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, NULL);
				if (FAILED(hr)) {
					hr = m_pDVDC_preview->PlayAtTimeInTitle(Loc.TitleNum, &dvdTo, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
					if (FAILED(hr)) {
						return hr;
					}
				}
			}
		} else {
			hr = m_pDVDC_preview->PlayAtTime(&dvdTo, DVD_CMD_FLAG_Flush, NULL);
			if (FAILED(hr)) {
				return hr;
			}
		}

		m_pDVDI_preview->GetCurrentLocation(&Loc2);
		//LOG2FILE(_T("DVD - Текущее воспроизведение : Main [%d : %d], Preview [%d : %d]"), Loc.TitleNum, Loc.ChapterNum, Loc2.TitleNum, Loc2.ChapterNum);

		m_pDVDC_preview->Pause(FALSE);
		m_pMC_preview->Run();
	} else if (GetPlaybackMode() == PM_FILE && m_pMS_preview) {
		hr = m_pMS_preview->SetPositions(&rtCur2, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
	}

	if (FAILED(hr)) {
		return hr;
	}

	/*
	if (GetPlaybackMode() == PM_FILE) {
		hr = pFS2 ? pFS2->Step(2, NULL) : E_FAIL;
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

static UINT YoutubeThreadProc(LPVOID pParam)
{
	return (static_cast<CMainFrame*>(pParam))->YoutubeThreadProc();
}

UINT CMainFrame::YoutubeThreadProc()
{
	AppSettings& sApp = AfxGetAppSettings();
	HINTERNET f, s = InternetOpen(L"MPC-BE", 0, NULL, NULL, 0);
	if (s) {
#ifdef _DEBUG
		CString tmp = m_YoutubeFile;
#endif
		f = InternetOpenUrl(s, m_YoutubeFile, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
		if (f) {
			TCHAR QuerySizeText[100] = { 0 };
			DWORD dw = _countof(QuerySizeText) * sizeof(TCHAR);
			if (HttpQueryInfo(f, HTTP_QUERY_CONTENT_LENGTH, QuerySizeText, &dw, 0)) {
				QWORD val = 0;
				if (1 == swscanf_s(QuerySizeText, L"%I64d", &val)) {
					m_YoutubeTotal = val;
				}
			}

			if (GetTemporaryFilePath(GetFileExt(GetAltFileName()).MakeLower(), m_YoutubeFile)) {
				HANDLE hFile;
				hFile = CreateFile(m_YoutubeFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0 ,CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					sApp.slTMPFilesList.AddTail(m_YoutubeFile);

					HANDLE hMapping = INVALID_HANDLE_VALUE;
					if (m_YoutubeTotal > 0) {
						ULARGE_INTEGER usize;
						usize.QuadPart = m_YoutubeTotal;
						hMapping = CreateFileMapping(hFile, 0, PAGE_READWRITE, usize.HighPart, usize.LowPart, NULL);
						if (hMapping != INVALID_HANDLE_VALUE) {
							CloseHandle(hMapping);
						}
					}

					QWORD dwWaitSize = MEGABYTE;
					switch (sApp.iYoutubeMemoryType) {
						case 0:
							if (m_YoutubeTotal && sApp.iYoutubePercentMemory && sApp.iYoutubePercentMemory <= 100) {
								dwWaitSize = m_YoutubeTotal * ((float)sApp.iYoutubePercentMemory / 100);
							}
							break;
						case 1:
							if (sApp.iYoutubeMbMemory) {
								dwWaitSize = m_YoutubeTotal ? min(sApp.iYoutubeMbMemory * MEGABYTE, m_YoutubeTotal) : sApp.iYoutubeMbMemory;
							}
							break;
					}

					DWORD dwBytesWritten	= 0;
					DWORD dwBytesRead		= 0;
					DWORD dataSize			= 0;
					BYTE buf[64 * KILOBYTE];
					while (InternetReadFile(f, (LPVOID)buf, sizeof(buf), &dwBytesRead) && dwBytesRead && m_fYoutubeThreadWork != TH_ERROR && m_fYoutubeThreadWork != TH_CLOSE) {
						if (FALSE == WriteFile(hFile, (LPCVOID)buf, dwBytesRead, &dwBytesWritten, NULL) || dwBytesRead != dwBytesWritten) {
							break;
						}

						m_YoutubeCurrent += dwBytesRead;
						if (m_YoutubeCurrent >= dwWaitSize) {
							m_fYoutubeThreadWork = TH_WORK;
						}
					}

					if (m_fYoutubeThreadWork == TH_START) {
						m_fYoutubeThreadWork = TH_WORK;
					}
#ifdef _DEBUG
					if (m_YoutubeCurrent) {
						LOG2FILE(_T("Open Youtube, GOOD from \'%s\'"), tmp);
					} else {
						LOG2FILE(_T("Open Youtube, FAILED from \'%s\'"), tmp);
					}
#endif
					CloseHandle(hFile);
				} else {
					m_fYoutubeThreadWork = TH_ERROR;
				}
			} else {
				m_fYoutubeThreadWork = TH_ERROR;
			}
			InternetCloseHandle(f);
		} else {
			m_fYoutubeThreadWork = TH_ERROR;
		}
		InternetCloseHandle(s);
	} else {
		m_fYoutubeThreadWork = TH_ERROR;
	}

	if (m_fYoutubeThreadWork != TH_ERROR) {
		m_fYoutubeThreadWork = TH_CLOSE;
	}

	return (UINT)m_fYoutubeThreadWork;
}

CString CMainFrame::OpenFile(OpenFileData* pOFD)
{
	if (pOFD->fns.IsEmpty()) {
		return ResStr(IDS_MAINFRM_81);
	}

	m_YoutubeFile.Empty();
	m_YoutubeThread			= NULL;
	m_fYoutubeThreadWork	= TH_CLOSE;
	m_YoutubeTotal			= 0;
	m_YoutubeCurrent		= 0;

	m_nMainFilterId			= NULL;

	AppSettings& s = AfxGetAppSettings();

	bool bFirst = true;

	POSITION pos = pOFD->fns.GetHeadPosition();
	while (pos) {
		CFileItem fi = pOFD->fns.GetNext(pos);
		CString fn = fi;

		fn.Trim();
		if (fn.IsEmpty() && !bFirst) {
			break;
		}

		m_strUrl.Empty();
		m_youtubeFields.Empty();

		HRESULT hr = S_OK;
		bool extimage = false;

		CString local(fn);
		local.MakeLower();
		bool validateUrl = true;

		if (!PlayerYouTubeCheck(fn)) {
			if (local.Find(_T("http://")) == 0 || local.Find(_T("www.")) == 0) {
				// validate url before try to opening
				if (local.Find(_T("www.")) == 0) {
					local = _T("http://") + local;
				}

				CMPCSocket socket;
				if (socket.Create()) {
					socket.SetTimeOut(3000);
					if (!socket.Connect(local, TRUE)) {
						validateUrl = false;
					}

					socket.Close();
				}
			}
		}

		if (!validateUrl) {
			hr = VFW_E_NOT_FOUND;
		} else {
			CString tmpName = m_strUrl = fn;
			if (PlayerYouTubeCheck(fn)) {
				tmpName = PlayerYouTube(fn, &m_youtubeFields, &pOFD->subs);
				m_strUrl = tmpName;

				if (s.iYoutubeSource == 0 && CString(tmpName).MakeLower().Find(L".m3u8") == -1) {
					if (!m_youtubeFields.fname.IsEmpty() && ::PathIsURL(tmpName)) {
						m_fYoutubeThreadWork = TH_START;
						m_YoutubeFile = tmpName;
						m_YoutubeThread = AfxBeginThread(::YoutubeThreadProc, static_cast<LPVOID>(this), THREAD_PRIORITY_ABOVE_NORMAL);
						while (m_fYoutubeThreadWork == TH_START && !m_fOpeningAborted) {
							Sleep(50);
						}
						if (m_fOpeningAborted) {
							m_fYoutubeThreadWork = TH_ERROR;
							hr = E_ABORT;
						}

						if (m_fYoutubeThreadWork != TH_ERROR && ::PathFileExists(m_YoutubeFile)) {
							tmpName = m_YoutubeFile;
						} else {
							tmpName.Empty();
						}
					}
				}

				if (CString(tmpName).MakeLower().Find(L".m3u8") > 0) {
					CAtlList<CString> fns;
					fns.AddTail(tmpName);
					m_wndPlaylistBar.Open(fns, false);
					m_wndPlaylistBar.SetFirstSelected();
					m_strUrl = tmpName = m_wndPlaylistBar.GetCurFileName();
				}
			}

			if (SUCCEEDED(hr)) {
				TCHAR path[MAX_PATH]	= {0};
				BOOL bIsDirSet			= FALSE;
				if (!::PathIsURL(tmpName) && GetCurrentDirectory(sizeof(path), path)) {
					bIsDirSet = SetCurrentDirectory(GetFolderOnly(tmpName));
				}

				if (bFirst) {
					hr = m_pGB->RenderFile(tmpName, NULL);
				} else if (CComQIPtr<IGraphBuilderAudio> pGBA = m_pGB) {
					hr = pGBA->RenderAudioFile(tmpName);
				}

				if (bIsDirSet) {
					SetCurrentDirectory(path);
				}
			}
		}

		if (m_strUrl.IsEmpty()) {
			m_strUrl = fn;
		}

		if (FAILED(hr)) {
			if (bFirst) {
				if (s.fReportFailedPins && !(m_pFullscreenWnd && m_pFullscreenWnd->IsWindow())) {
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

		if ((CString(fn).MakeLower().Find(_T("://"))) < 0 && bFirst) {
			if (s.fKeepHistory && s.fRememberFilePos && !s.NewFile(fn)) {
				REFERENCE_TIME	rtPos = s.CurrentFilePosition()->llPosition;
				if (m_pMS) {
					m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
				}
			}
		}
		QueryPerformanceCounter(&m_LastSaveTime);

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

			if (!bIsVideo || FAILED(m_pGB_preview->RenderFile(fn, NULL))) {
				if (m_pGB_preview) {
					m_pMFVP_preview  = NULL;
					m_pMFVDC_preview = NULL;

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
					m_pGB_preview = NULL;
				}
				m_bUseSmartSeek = false;
			}
		}

		if (s.fKeepHistory && pOFD->bAddRecent) {
			CRecentFileList* pMRU = bFirst ? &s.MRU : &s.MRUDub;
			pMRU->ReadList();
			pMRU->Add(fn);
			pMRU->WriteList();
			if (fn.Mid(1, 2) == L":\\" || fn.Left(2) == L"\\\\") { // stupid path detector
				// there should not be a URL, otherwise explorer dirtied HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
				SHAddToRecentDocs(SHARD_PATH, fn);
			}
		}

		if (bFirst) {
			pOFD->title = (m_youtubeFields.fname.IsEmpty() ? m_strUrl : m_youtubeFields.fname);
			{
				BeginEnumFilters(m_pGB, pEF, pBF);
				if (m_pMainFSF = pBF) {
					break;
				}
				EndEnumFilters;

				m_pMainSourceFilter = FindFilter(__uuidof(CMpegSplitterFilter), m_pGB);
				if (!m_pMainSourceFilter) {
					m_pMainSourceFilter = FindFilter(__uuidof(CMpegSourceFilter), m_pGB);
				}
				if (!m_pMainSourceFilter) {
					m_pMainSourceFilter = FindFilter(CLSID_OggSplitter, m_pGB);
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

						m_pCB->ChapAppend(REFERENCE_TIME(MarkerTime*10000000), name);
					}
				}
			}
		}

		pos = pBFs.GetHeadPosition();
		while (pos && !m_pCB->ChapGetCount()) {
			IBaseFilter* pBF = pBFs.GetNext(pos);

			if (GetCLSID(pBF) != CLSID_OggSplitter) {
				continue;
			}

			BeginEnumPins(pBF, pEP, pPin) {
				if (m_pCB->ChapGetCount()) {
					break;
				}

				if (CComQIPtr<IPropertyBag> pPB = pPin) {
					for (int i = 1; ; i++) {
						CStringW str;
						CComVariant var;

						var.Clear();
						str.Format(L"CHAPTER%02d", i);
						if (S_OK != pPB->Read(str, &var, NULL)) {
							break;
						}

						int h, m, s, ms;
						WCHAR wc;
						if (7 != swscanf_s(CStringW(var), L"%d%c%d%c%d%c%d", &h, &wc, sizeof(WCHAR), &m, &wc, sizeof(WCHAR), &s, &wc, sizeof(WCHAR), &ms)) {
							break;
						}

						CStringW name;
						name.Format(L"Chapter %d", i);
						var.Clear();
						str += L"NAME";
						if (S_OK == pPB->Read(str, &var, NULL)) {
							name = var;
						}

						m_pCB->ChapAppend(10000i64*(((h*60 + m)*60 + s)*1000 + ms), name);
					}
				}
			}
			EndEnumPins;
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pCB->ChapRemoveAll();

		WCHAR buff[MAX_PATH] = { 0 };
 		ULONG len = 0;
 		DVD_PLAYBACK_LOCATION2 loc;
		ULONG ulNumOfChapters = 0;
 		if (SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, _countof(buff), &len))
			&& SUCCEEDED(m_pDVDI->GetCurrentLocation(&loc))
			&& SUCCEEDED(m_pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters))) {
 			CStringW path;
			path.Format(L"%s\\video_ts.IFO", buff);

			ULONG VTSN, TTN;
			if (::PathFileExists(path) && CVobFile::GetTitleInfo(path, loc.TitleNum, VTSN, TTN)) {
				path.Format(L"%s\\VTS_%02lu_0.IFO", buff, VTSN);

				CAtlList<CString> files;
				CVobFile vob;
				if (::PathFileExists(path) && vob.OpenIFO(path, files, TTN) && ulNumOfChapters + 1 == vob.GetChaptersCount()) {
					for (UINT i = 0; i < vob.GetChaptersCount(); i++) {
						REFERENCE_TIME rt = vob.GetChapterOffset(i);
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
	HRESULT hr = m_pGB->RenderFile(pODD->path, NULL);

	AppSettings& s = AfxGetAppSettings();

	if (s.fReportFailedPins) {
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = m_pGB;
		if (pGBDE && pGBDE->GetCount()) {
			CMediaTypesDlg(pGBDE, GetModalParent()).DoModal();
		}
	}

	if (SUCCEEDED(hr) && m_bUseSmartSeek) {
		if (FAILED(hr = m_pGB_preview->RenderFile(pODD->path, NULL))) {
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
	HRESULT hr = m_pGB->RenderFile(L"", NULL);
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

	pCGB = NULL;
	pVidCap = NULL;
	pAudCap = NULL;

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
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))
					&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))) {
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC))
					&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC))) {
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap"));
			} else {
				pAudCap = pVidCap;
			}
		} else {
			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))) {
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
			}

			if (FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))) {
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
			}
		}

		if (FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMCrossbar, (void**)&pAMXBar))) {
			TRACE(_T("Warning: No IAMCrossbar interface was found\n"));
		}

		if (FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMTVTuner, (void**)&pAMTuner))) {
			TRACE(_T("Warning: No IAMTVTuner interface was found\n"));
		}

		if (pAMTuner) { // load saved channel
			pAMTuner->put_CountryCode(AfxGetApp()->GetProfileInt(_T("Capture"), _T("Country"), 1));

			int vchannel = pODD->vchannel;
			if (vchannel < 0) {
				vchannel = AfxGetApp()->GetProfileInt(_T("Capture\\") + CString(m_VidDispName), _T("Channel"), -1);
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
			TRACE(_T("Warning: No IAMStreamConfig interface for vidcap"));
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

	return _T("");
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

	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& r = s.m_RenderersSettings;
	if (r.m_AdvRendSets.bSynchronizeVideo && s.iDSVideoRendererType == VIDRNDT_DS_SYNC) {
		HRESULT hr;
		m_pRefClock = DNew CSyncClockFilter(NULL, &hr);
		CStringW name;
		name = L"SyncClock Filter";
		m_pGB->AddFilter(m_pRefClock, name);

		CComPtr<IReferenceClock> refClock;
		m_pRefClock->QueryInterface(IID_PPV_ARGS(&refClock));
		CComPtr<IMediaFilter> mediaFilter;
		m_pGB->QueryInterface(IID_PPV_ARGS(&mediaFilter));
		mediaFilter->SetSyncSource(refClock);
		mediaFilter = NULL;
		refClock = NULL;

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

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if (GetCLSID(pBF) == CLSID_OggSplitter) {
			if (CComQIPtr<IAMStreamSelect> pSS = pBF) {
				LCID idAudio = s.idAudioLang;
				if (!idAudio) {
					idAudio = GetUserDefaultLCID();
				}
				LCID idSub = s.idSubtitlesLang;
				if (!idSub) {
					idSub = GetUserDefaultLCID();
				}

				DWORD cnt = 0;
				pSS->Count(&cnt);
				for (DWORD i = 0; i < cnt; i++) {
					LCID lcid = DWORD_MAX;
					WCHAR* pszName = NULL;
					if (SUCCEEDED(pSS->Info((long)i, NULL, NULL, &lcid, NULL, &pszName, NULL, NULL))) {
						CStringW name(pszName), sound(ResStr(IDS_AG_SOUND)), subtitle(L"Subtitle");

						if (idAudio != (LCID)-1 && (idAudio&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
								&& name.GetLength() > sound.GetLength()
								&& !name.Left(sound.GetLength()).CompareNoCase(sound)) {
							if (SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE))) {
								idAudio = (LCID)-1;
							}
						}

						if (idSub != (LCID)-1 && (idSub&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
								&& name.GetLength() > subtitle.GetLength()
								&& !name.Left(subtitle.GetLength()).CompareNoCase(subtitle)
								&& name.Mid(subtitle.GetLength()).Trim().CompareNoCase(L"off")) {
							if (SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE))) {
								idSub = (LCID)-1;
							}
						}

						if (pszName) {
							CoTaskMemFree(pszName);
						}
					}
				}
			}
		}
	}
	EndEnumFilters;

	CleanGraph();
}

void CMainFrame::OpenSetupVideo()
{
	m_bAudioOnly = true;

	if (m_pMFVDC) {	// EVR
		m_bAudioOnly = false;
	} else if (m_pCAP) {
		CSize vs = m_pCAP->GetVideoSize();
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

		if (m_bUseSmartSeek && m_wndPreView) {
			m_pVW_preview->put_Owner((OAHWND)m_wndPreView.GetVideoHWND());
			m_pVW_preview->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		}
	}

	if (m_bAudioOnly && m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->DestroyWindow();
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
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DESCRIPTION), bstr.m_str);
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
						id = NULL;
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

	m_strFnFull = fn;

	AppSettings& s = AfxGetAppSettings();

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

			MakeDVDLabel(fn);
		} else if (GetPlaybackMode() == PM_CAPTURE) {
			fn = ResStr(IDS_CAPTURE_LIVE);
		}
	}

	m_strCurPlaybackLabel = fn;

	if (i == 1) {
		if (s.fTitleBarTextTitle) {
			if (!m_youtubeFields.title.IsEmpty()) {
				fn = m_youtubeFields.title;
			}

			BeginEnumFilters(m_pGB, pEF, pBF) {
				if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
					if (!CheckMainFilter(pBF)) {
						continue;
					}

					CComBSTR bstr;
					if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
						fn = CString(bstr.m_str);
						bstr.Empty();
						break;
					}
				}
			}
			EndEnumFilters;
		}
		title = fn + L" - " + m_strTitle;
	} else if (i == 0) {
		title = fname + L" - " + m_strTitle;
	}

	SetWindowText(title);
	m_Lcd.SetMediaTitle(LPCTSTR(fn));
}

#define BREAK(msg) {err = msg; break;}

BOOL CMainFrame::SelectMatchTrack(CAtlArray<Stream>& Tracks, CString pattern, BOOL bExtPrior, size_t& nIdx)
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
		for (size_t iIndex = 0; iIndex < Tracks.GetCount(); iIndex++) {
			if (bExtPrior && !Tracks[iIndex].Ext) {
				continue;
			}

			CString name(Tracks[iIndex].Name);
			CharLower(name.GetBuffer());

			CAtlList<CString> sl;
			Explode(lang, sl, '|');
			POSITION pos = sl.GetHeadPosition();

			int nLangMatch = 0;
			while (pos) {
				CString subPattern = sl.GetNext(pos);

				if ((Tracks[iIndex].forced && subPattern == _T("forced")) || (Tracks[iIndex].def && subPattern == _T("default"))) {
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

		lang = pattern.Tokenize(_T(",; "), tPos);
	}

	return FALSE;
}

void CMainFrame::OpenSetupAudioStream()
{
	if (m_iMediaLoadState != MLS_LOADING) {
		return;
	}

	if (GetPlaybackMode() == PM_FILE) {

		CAtlList<CString> extAudioList;
		CPlaylistItem pli;
		if (m_wndPlaylistBar.GetCur(pli)) {
			POSITION pos = pli.m_fns.GetHeadPosition();
			// skip main file
			pli.m_fns.GetNext(pos);
			while (pos) {
				CString str = pli.m_fns.GetNext(pos);
				extAudioList.AddTail(GetFileOnly(str));
			}
		}

		// build list of audio stream
		Stream as;
		CAtlArray<Stream> MixAS;
		int iSel	= -1;

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS) {
			//CComQIPtr<ITrackInfo> pInfo = pSS;
			DWORD cStreamsS = 0;
			if (SUCCEEDED(pSS->Count(&cStreamsS)) && cStreamsS > 0) {
				for (int i = 0; i < (int)cStreamsS; i++) {
					//iSel = 0;
					DWORD dwFlags	= DWORD_MAX;
					DWORD dwGroup	= DWORD_MAX;
					WCHAR* pszName	= NULL;
					if (FAILED(pSS->Info(i, NULL, &dwFlags, NULL, &dwGroup, &pszName, NULL, NULL))) {
						continue;
					}

					if (dwGroup == 1) {
						if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
							iSel = MixAS.GetCount();
							//iSel = 1;
						}

						as.forced = as.def = false;
						/*
						if (pInfo) {
							TrackElement TrackInfo;
							TrackInfo.Size = sizeof(TrackInfo);
							if (pInfo->GetTrackInfo((UINT)i, &TrackInfo) && TrackInfo.Type == TypeAudio) {
								as.def		= !!TrackInfo.FlagDefault;
								as.forced	= !!TrackInfo.FlagForced;
							}
						}
						*/

						as.Filter	= 1;
						as.Index	= i;
						as.Num++;
						as.Sel		= iSel;
						as.Name		= CString(pszName);
						MixAS.Add(as);
					}

					if (pszName) {
						CoTaskMemFree(pszName);
					}
				}
			}
		}

		CComQIPtr<IAMStreamSelect> pSSa = m_pSwitcherFilter;
		if (pSSa) {

			/*
			UINT maxAudioTrack = 0;
			CComQIPtr<ITrackInfo> pInfo = FindFilter(__uuidof(CMatroskaSourceFilter), m_pGB);
			if (!pInfo) {
				pInfo = FindFilter(__uuidof(CMatroskaSplitterFilter), m_pGB);
			}
			if (pInfo) {
				for (UINT i = 0; i < pInfo->GetTrackCount(); i++) {
					TrackElement TrackInfo;
					TrackInfo.Size = sizeof(TrackInfo);
					if (pInfo->GetTrackInfo(i, &TrackInfo) && TrackInfo.Type == TypeAudio) {
						maxAudioTrack = max(maxAudioTrack, i);
					}
				}
			}
			*/

			DWORD cStreamsA = 0;
			int i = MixAS.GetCount() > 0 ? 1 : 0;
			if (SUCCEEDED(pSSa->Count(&cStreamsA)) && cStreamsA > 0) {
				for (i; i < (int)cStreamsA; i++) {
					//iSel = 0;
					DWORD dwFlags	= DWORD_MAX;
					WCHAR* pszName	= NULL;
					if (FAILED(pSSa->Info(i, NULL, &dwFlags, NULL, NULL, &pszName, NULL, NULL))) {
						continue;
					}

					as.forced = as.def = false;
					/*
					UINT l = i + 1;
					if (pInfo && l <= maxAudioTrack) {
						TrackElement TrackInfo;
						TrackInfo.Size = sizeof(TrackInfo);
						if (pInfo->GetTrackInfo(l, &TrackInfo) && TrackInfo.Type == TypeAudio) {
							as.def		= !!TrackInfo.FlagDefault;
							as.forced	= !!TrackInfo.FlagForced;
						}
					}
					*/

					if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
						//iSel = 1;
						iSel = MixAS.GetCount();
						for (size_t i = 0; i < MixAS.GetCount(); i++) {
							if (MixAS[i].Sel == 1) {
								MixAS[i].Sel = 0;
							}
						}
					}

					as.Filter	= 2;
					as.Index	= i;
					as.Num++;
					as.Sel		= iSel;
					as.Name		= CString(pszName);
					as.Ext		= extAudioList.Find(as.Name) != NULL;
					MixAS.Add(as);

					if (pszName) {
						CoTaskMemFree(pszName);
					}
				}
			}
		}

#ifdef DEBUG
		DbgLog((LOG_TRACE, 3, L"Audio Track list :"));
		for (size_t i = 0; i < MixAS.GetCount(); i++) {
			DbgLog((LOG_TRACE, 3, L"	%s, type = %s", MixAS[i].Name, MixAS[i].Filter == 1 ? L"Splitter" : L"AudioSwitcher"));
		}
#endif

		AppSettings& s = AfxGetAppSettings();
		if (!s.fUseInternalSelectTrackLogic) {
			if (s.fPrioritizeExternalAudio && extAudioList.GetCount() > 0 && pSSa) {
				for (size_t i = 0; i < MixAS.GetCount(); i++) {
					if (MixAS[i].Ext) {
						Stream as = MixAS[i];
						pSSa->Enable(as.Index, AMSTREAMSELECTENABLE_ENABLE);
						return;
					}
				}
			}
		} else if (MixAS.GetCount() > 1) {
			CString pattern = s.strAudiosLanguageOrder;
			if (s.fPrioritizeExternalAudio && extAudioList.GetCount() > 0 && pSSa) {
				size_t nIdx	= 0;
				BOOL bMatch = SelectMatchTrack(MixAS, pattern, TRUE, nIdx);

				Stream as;
				if (bMatch) {
					as = MixAS[nIdx];
				} else {
					for (size_t i = 0; i < MixAS.GetCount(); i++) {
						if (MixAS[i].Ext) {
							as = MixAS[i];
							break;
						}
					}
				}

				if (as.Ext) {
					pSSa->Enable(as.Index, AMSTREAMSELECTENABLE_ENABLE);
				}

				return;
			}

			size_t nIdx	= 0;
			BOOL bMatch = SelectMatchTrack(MixAS, pattern, FALSE, nIdx);

			if (bMatch) {
				Stream as = MixAS[nIdx];
				if (as.Filter == 1) {
					pSS->Enable(as.Index, AMSTREAMSELECTENABLE_ENABLE);
				} else if (as.Filter == 2) {
					pSSa->Enable(as.Index, AMSTREAMSELECTENABLE_ENABLE);
				}
				return;
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
	AppSettings& s	= AfxGetAppSettings();
	CString pattern	= s.strSubtitlesLanguageOrder;

	if (subarray.GetCount()) {
		if (s.fPrioritizeExternalSubtitles) { // try external sub ...
			size_t nIdx	= 0;
			BOOL bMatch = SelectMatchTrack(subarray, pattern, TRUE, nIdx);

			if (bMatch) {
				return nIdx;
 			} else {
				for (size_t iIndex = 0; iIndex < subarray.GetCount(); iIndex++) {
					if (subarray[iIndex].Ext) {
						return iIndex;
					}
				}
			}
		}

		size_t nIdx	= 0;
		BOOL bMatch = SelectMatchTrack(subarray, pattern, FALSE, nIdx);

		if (bMatch) {
			return nIdx;
		}
	}

	return 0;
}

void CMainFrame::OpenSetupSubStream(OpenMediaData* pOMD)
{
	AppSettings& s = AfxGetAppSettings();

	BeginEnumFilters(m_pGB, pEF, pBF) {
		if (m_pDVS = pBF) {
			break;
		}
	}
	EndEnumFilters;

	if (m_pDVS) {
		int nLangs;
		if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {
			subarray.RemoveAll();

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
						subarray.Add(substream);

						CoTaskMemFree(pName);
					}
				}

				DWORD cStreams;
				pSS->Count(&cStreams);
				for (int i = 0, j = cStreams; i < j; i++) {
					DWORD dwFlags	= DWORD_MAX;
					DWORD dwGroup	= DWORD_MAX;
					LCID lcid		= DWORD_MAX;
					WCHAR* pszName = NULL;

					if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
							|| !pszName) {
						continue;
					}

					CString name(pszName);

					if (pszName) {
						CoTaskMemFree(pszName);
					}

					if (dwGroup != 2) {
						continue;
					}

					CString str = FormatStreamName(name, lcid, i);

					Stream substream;
					substream.Filter	= 2;
					substream.Num++;
					substream.Index++;
					substream.Name		= str;
					if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE)) {
						substream.Sel = subarray.GetCount();
					}
					subarray.Add(substream);


				}
			} else {
				for (int i = 0; i < nLangs; i++) {
					WCHAR *pName;
					if (SUCCEEDED(m_pDVS->get_LanguageName(i, &pName)) && pName) {
						Stream substream;
						substream.Filter	= 1;
						substream.Num		= i;
						substream.Index		= i;
						substream.Name		= pName;

						if (CComQIPtr<IDirectVobSub3> pDVS3 = m_pDVS) {
							int nType = 0;
							pDVS3->get_LanguageType(i, &nType);
							substream.Ext = !!nType;
						}

						subarray.Add(substream);

						CoTaskMemFree(pName);
					}
				}
			}

			if (s.fUseInternalSelectTrackLogic) {
				OnNavMixStreamSubtitleSelectSubMenu(GetSubSelIdx(), 2);
			} else if (subcount && !s.fPrioritizeExternalSubtitles) {
				m_pDVS->put_SelectedLanguage(nLangs - 1);
			}
		}

		return;
	}

	if (m_pCAP && !m_bAudioOnly) {
		Stream substream;
		subarray.RemoveAll();
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
					WCHAR* pName	= NULL;

					if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pName, NULL, NULL)) || !pName) {
						continue;
					}

					if (dwGroup == 2) {
						subIndex++;

						CString lang;
						if (lcid == 0) {
							lang = pName;
						} else {
							int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
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

						subarray.Add(substream);
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

		int splsubcnt	= subarray.GetCount();

		if (splsubcnt < 1) {
			POSITION pos = m_pSubStreams.GetHeadPosition();
			int tPos = -1;
			while (pos) {
				tPos++;
				CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);
				int i = pSubStream->GetStream();
				WCHAR* pName = NULL;
				LCID lcid;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, &lcid))) {
					cntintsub++;

					CString lang;
					if (lcid == 0) {
						lang = CString(pName);
					} else {
						int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
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

					subarray.Add(substream);

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

		POSITION pos = pOMD->subs.GetHeadPosition();
		while (pos) {
			LoadSubtitle(pOMD->subs.GetNext(pos));
		}

		pos = m_pSubStreams.GetHeadPosition();
		CComPtr<ISubStream> pSubStream;
		int tPos = -1;
		int extcnt = -1;
		for (size_t i = 0; i < subarray.GetCount(); i++) {
			if (subarray[i].Filter == 2) extcnt++;
		}

		int intsub = 0;

		while (pos) {
			pSubStream = m_pSubStreams.GetNext(pos);
			tPos++;
			if (!pSubStream) continue;

			for (int i = 0; i < pSubStream->GetStreamCount(); i++) {
				WCHAR* pName = NULL;
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

					subarray.Add(substream);

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

		if (!s.fUseInternalSelectTrackLogic) {
			if (s.fPrioritizeExternalSubtitles) {
				size_t cnt = subarray.GetCount();
				for (size_t i = 0; i < cnt; i++) {
					if (subarray[i].Ext)	{
						checkedsplsub = i;
						break;
					}
				}
			}
			OnNavMixStreamSubtitleSelectSubMenu(checkedsplsub, 2);
		} else {
			int cnt = subarray.GetCount();
			size_t defsub = GetSubSelIdx();

			if (m_pMainSourceFilter && s.fDisableInternalSubtitles) {
				defsub++;
			}

			if (cnt > 0) {
				OnNavMixStreamSubtitleSelectSubMenu(defsub, 2);
			}
		}
	}
}

void __stdcall MadVRExclusiveModeCallback(LPVOID context, int event)
{
	CString _event;
	switch (event) {
		case ExclusiveModeIsAboutToBeEntered	: _event = _T("ExclusiveModeIsAboutToBeEntered"); break;
		case ExclusiveModeWasJustEntered		: _event = _T("ExclusiveModeWasJustEntered"); break;
		case ExclusiveModeIsAboutToBeLeft		: _event = _T("ExclusiveModeIsAboutToBeLeft"); break;
		case ExclusiveModeWasJustLeft			: _event = _T("ExclusiveModeWasJustLeft"); break;
		default									: _event = _T("Unknown event"); break;
	}
	TRACE(_T("MadVRExclusiveModeCallback() : event = %ws\n"), _event);

	if (event == ExclusiveModeIsAboutToBeEntered) {
		((CMainFrame*)context)->IsMadVRExclusiveMode = true;
	} else if (event == ExclusiveModeIsAboutToBeLeft) {
		((CMainFrame*)context)->IsMadVRExclusiveMode = false;
	}
	((CMainFrame*)context)->FlyBarSetPos();
}

bool CMainFrame::OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD)
{
	ASSERT(m_iMediaLoadState == MLS_LOADING);

	SetDwmPreview(FALSE);

	m_fValidDVDOpen	= false;

	OpenFileData *pFileData		= dynamic_cast<OpenFileData*>(pOMD.m_p);
	OpenDVDData* pDVDData		= dynamic_cast<OpenDVDData*>(pOMD.m_p);
	OpenDeviceData* pDeviceData	= dynamic_cast<OpenDeviceData*>(pOMD.m_p);
	ASSERT(pFileData || pDVDData || pDeviceData);

	m_ExtSubFiles.RemoveAll();
	m_ExtSubFilesTime.RemoveAll();
	m_ExtSubPaths.RemoveAll();
	m_ExtSubPathsHandles.RemoveAll();
	SetEvent(m_hRefreshNotifyRenderThreadEvent);

	m_PlaybackRate = 1.0;

	ClearDXVAState();

#ifdef _DEBUG
	if (pFileData) {
		POSITION pos = pFileData->fns.GetHeadPosition();
		UINT index = 0;
		while (pos) {
			CString path = pFileData->fns.GetNext(pos);
			DbgLog((LOG_TRACE, 3, _T("--> CMainFrame::OpenMediaPrivate() - pFileData->fns[%d]:"), index));
			DbgLog((LOG_TRACE, 3, _T("	%s"), path.GetString()));
			index++;
		}
	}
#endif

	AppSettings& s = AfxGetAppSettings();

	CString mi_fn;
	for (;;) {
		if (pFileData) {
			if (pFileData->fns.IsEmpty()) {
				ASSERT(FALSE);
				break;
			}

			CString fn = pFileData->fns.GetHead();

			int i = fn.Find(_T(":\\"));
			if (i > 0) {
				CString drive = fn.Left(i+2);
				UINT type = GetDriveType(drive);
				CAtlList<CString> sl;
				if (type == DRIVE_REMOVABLE || (type == DRIVE_CDROM && GetCDROMType(drive[0], sl) != CDROM_Audio)) {
					int ret = IDRETRY;
					while (ret == IDRETRY) {
						WIN32_FIND_DATA findFileData;
						HANDLE h = FindFirstFile(fn, &findFileData);
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

		miFPS	= 0.0;
		s.dFPS	= 0.0;

		if ((s.AutoChangeFullscrRes.bEnabled == 1 && s.IsD3DFullscreen()) || s.AutoChangeFullscrRes.bEnabled == 2) {
			// DVD
			if (pDVDData) {
				mi_fn = pDVDData->path;
				CString ext = GetFileExt(mi_fn);
				if (ext.IsEmpty()) {
					if (mi_fn.Right(10) == _T("\\VIDEO_TS\\")) {
						mi_fn = mi_fn + _T("VTS_01_1.VOB");
					} else {
						mi_fn = mi_fn + _T("\\VIDEO_TS\\VTS_01_1.VOB");
					}
				} else if (ext == _T(".IFO")) {
					mi_fn = GetFolderOnly(mi_fn) + _T("\\VTS_01_1.VOB");
				}
			} else {
				CString ext = GetFileExt(mi_fn);
				// BD
				if (ext == _T(".mpls")) {
					CHdmvClipInfo ClipInfo;
					CHdmvClipInfo::CPlaylist CurPlaylist;
					REFERENCE_TIME rtDuration;
					if (SUCCEEDED(ClipInfo.ReadPlaylist(mi_fn, rtDuration, CurPlaylist))) {
						mi_fn = CurPlaylist.GetHead()->m_strFileName;
					}
				} else if (ext == _T(".IFO")) {
					// DVD structure
					CString sVOB = mi_fn;

					for (int i = 0; i < 100; i++) {
						sVOB = mi_fn;
						CString vob;
						vob.Format(_T("%d.VOB"), i);
						sVOB.Replace(_T("0.IFO"), vob);

						if (::PathFileExists(sVOB)) {
							mi_fn = sVOB;
							break;
						}
					}
				}
			}

			// Get FPS
			MediaInfo MI;
			MI.Option(_T("ParseSpeed"), _T("0"));
			if (MI.Open(mi_fn.GetString())) {
				for (int i = 0; i < 2; i++) {
					CString strFPS = MI.Get(Stream_Video, 0, _T("FrameRate"), Info_Text, Info_Name).c_str();
					if (strFPS.IsEmpty() || _wtof(strFPS) > 200.0) {
						strFPS = MI.Get(Stream_Video, 0, _T("FrameRate_Original"), Info_Text, Info_Name).c_str();
					}
					CString strST = MI.Get(Stream_Video, 0, _T("ScanType"), Info_Text, Info_Name).c_str();
					CString strSO = MI.Get(Stream_Video, 0, _T("ScanOrder"), Info_Text, Info_Name).c_str();

					double nFactor = 1.0;

					// 2:3 pulldown
					if (strFPS == _T("29.970") && (strSO == _T("2:3 Pulldown") || (strST == _T("Progressive") && (strSO == _T("TFF") || strSO == _T("BFF") || strSO == _T("2:3 Pulldown"))))) {
						strFPS = _T("23.976");
					} else if (strST == _T("Interlaced") || strST == _T("MBAFF")) {
						// double fps for Interlaced video.
						nFactor = 2.0;
					}
					miFPS = _wtof(strFPS);
					if (miFPS < 30.0 && nFactor > 1.0) {
						miFPS *= nFactor;
					}

					if (miFPS > 0.9) {
						break;
					}

					MI.Close();
					MI.Option(_T("ParseSpeed"), _T("0.5"));
					if (!MI.Open(mi_fn.GetString())) {
						break;
					}
				}
				s.dFPS	= miFPS;

				AutoChangeMonitorMode();

				if (s.fLaunchfullscreen && !s.IsD3DFullscreen() && !m_fFullScreen && !m_bAudioOnly ) {
					ToggleFullscreen(true, true);
				}
			}
		}

		break;
	}

	CString err, aborted(ResStr(IDS_AG_ABORTED));

	m_fUpdateInfoBar = false;
	BeginWaitCursor();

	for (;;) {
		CComPtr<IVMRMixerBitmap9>		pVMB;
		CComPtr<IMFVideoMixerBitmap>	pMFVMB;
		CComPtr<IMadVRTextOsd>			pMVTO;
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

		m_pCAP2	= NULL;
		m_pCAP	= NULL;

		m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP2), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pVMRMC9), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&pVMB), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&pMFVMB), TRUE);
		pMVTO = m_pCAP;

		SetupVMR9ColorControl();

		// === EVR !
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVDC), TRUE);
		m_pGB->FindInterface(IID_PPV_ARGS(&m_pMFVP), TRUE);
		if (m_pMFVDC) {
			RECT Rect;
			::GetClientRect (m_pVideoWnd->m_hWnd, &Rect);
			m_pMFVDC->SetVideoWindow (m_pVideoWnd->m_hWnd);
			m_pMFVDC->SetVideoPosition(NULL, &Rect);
		}

		if (m_bUseSmartSeek && m_wndPreView) {
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pMFVDC_preview), TRUE);
			m_pGB_preview->FindInterface(IID_PPV_ARGS(&m_pMFVP_preview), TRUE);

			if (m_pMFVDC_preview) {
				RECT Rect2;
				::GetClientRect (m_wndPreView.GetVideoHWND(), &Rect2);
				m_pMFVDC_preview->SetVideoWindow (m_wndPreView.GetVideoHWND());
				m_pMFVDC_preview->SetVideoPosition(NULL, &Rect2);
			}
		}

		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (m_pLN21 = pBF) {
				m_pLN21->SetServiceState(s.fClosedCaptions ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off);
				break;
			}
		}
		EndEnumFilters;

		OpenCustomizeGraph();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		OpenSetupVideo();
		if (m_fOpeningAborted) {
			BREAK(aborted)
		}

		if (s.fShowOSD || s.fShowDebugInfo) { // Force OSD on when the debug switch is used

			m_OSD.Stop();

			if (s.IsD3DFullscreen() && !m_bAudioOnly) {
				if (pMVTO) {
					m_OSD.Start(m_pVideoWnd, pMVTO);
				} else if (pVMB) {
					m_OSD.Start(m_pVideoWnd, pVMB);
				} else if (pMFVMB) {
					m_OSD.Start(m_pVideoWnd, pMFVMB);
				}
			} else {
				if (pMVTO) {
					m_OSD.Start(m_pVideoWnd, pMVTO);
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

		m_bUseReclock = (FindFilter(CLSID_ReClock, m_pGB) != NULL);

		if (pFileData) {
			if (pFileData->rtStart > 0 && m_pMS) {
				REFERENCE_TIME rtPos = pFileData->rtStart;
				VERIFY(SUCCEEDED(m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning)));
			}
		}

		if (m_bUseSmartSeek && m_pMC_preview) {
			m_pMC_preview->Pause();
		}

		break;
	}

	EndWaitCursor();

	m_closingmsg	= err;
	WPARAM wp		= (WPARAM)pOMD.Detach();
	if (::GetCurrentThreadId() == AfxGetApp()->m_nThreadID) {
		OnPostOpen(wp, NULL);
	} else {
		PostMessage(WM_POSTOPEN, wp, NULL);
	}

	return (err.IsEmpty());
}

void CMainFrame::CloseMediaPrivate()
{
	DbgLog((LOG_TRACE, 3, L"CMainFrame::CloseMediaPrivate() : start"));

	ASSERT(m_iMediaLoadState == MLS_CLOSING);

	m_strCurPlaybackLabel.Empty();
	m_strFnFull.Empty();
	m_strUrl.Empty();
	m_youtubeFields.Empty();

	m_OSD.Stop();
	OnPlayStop();

	m_pparray.RemoveAll();
	m_ssarray.RemoveAll();

	if (m_pMC) {
		OAFilterState fs;
		m_pMC->GetState(0, &fs);
		if (fs != State_Stopped) {
			m_pMC->Stop(); // Some filters, such as Microsoft StreamBufferSource, need to call IMediaControl::Stop() before close the graph;
		}
	}

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
	m_fEndOfStream = false;
	m_rtDurationOverride = -1;
	m_kfs.clear();
	m_pCB.Release();

	{
		CAutoLock cAutoLock(&m_csSubLock);
		m_pSubStreams.RemoveAll();
	}
	m_pSubClock.Release();

	//if (pVW) pVW->put_Visible(OAFALSE);
	//if (pVW) pVW->put_MessageDrain((OAHWND)NULL), pVW->put_Owner((OAHWND)NULL);

	// IMPORTANT: IVMRSurfaceAllocatorNotify/IVMRSurfaceAllocatorNotify9 has to be released before the VMR/VMR9, otherwise it will crash in Release()
	m_pCAP2.Release();
	m_pCAP.Release();
	m_pVMRMC9.Release();
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

	if (m_pGB) {
		m_pGB->RemoveFromROT();
		m_pGB.Release();
	}

	if (m_pGB_preview) {
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

	DbgLog((LOG_TRACE, 3, L"CMainFrame::CloseMediaPrivate() : end"));
}

void CMainFrame::ParseDirs(CAtlList<CString>& sl)
{
	POSITION pos = sl.GetHeadPosition();

	while (pos) {
		CString fn = sl.GetNext(pos);
		WIN32_FIND_DATA fd = {0};
		HANDLE hFind = FindFirstFile(fn, &fd);

		if (hFind != INVALID_HANDLE_VALUE) {
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				if (fn[fn.GetLength()-1] != '\\') {
					fn += '\\';
				}

				COpenDirHelper::RecurseAddDir(fn, &sl);
			}

			FindClose(hFind);
		}
	}
}

struct fileName {
	CString fn;
};
static int compare(const void* arg1, const void* arg2)
{
	return StrCmpLogicalW(((fileName*)arg1)->fn, ((fileName*)arg2)->fn);
}

int CMainFrame::SearchInDir(bool DirForward)
{
	// Use CStringElementTraitsI so that the search is case insensitive
	CAtlList<CString, CStringElementTraitsI<CString>> sl;
	CAtlArray<fileName> f_array;

	AppSettings& s = AfxGetAppSettings();
	CMediaFormats& mf = s.m_Formats;

	CString dir		= AddSlash(GetFolderOnly(m_LastOpenFile));
	CString mask	= dir + L"*.*";
	WIN32_FIND_DATA fd;
	HANDLE h = FindFirstFile(mask, &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			}

			CString fn		= fd.cFileName;
			CString ext		= fn.Mid(fn.ReverseFind('.')).MakeLower();
			CString path	= dir + fd.cFileName;
			if (mf.FindExt(ext) && mf.GetCount() > 0) {
				fileName f_name;
				f_name.fn = path;
				f_array.Add(f_name);
			}
		} while (FindNextFile(h, &fd));
		FindClose(h);
	}

	if (f_array.GetCount() == 1) {
		return 1;
	}

	qsort(f_array.GetData(), f_array.GetCount(), sizeof(fileName), compare);
	for (size_t i = 0; i < f_array.GetCount(); i++) {
		sl.AddTail(f_array[i].fn);
	}

	POSITION Pos = sl.Find(m_LastOpenFile);
	if (Pos == NULL) {
		return 0;
	}

	if (DirForward) {
		if (Pos == sl.GetTailPosition()) {
			if (s.fNextInDirAfterPlaybackLooped) {
				Pos = sl.GetHeadPosition();
			} else {
				return 0;
			}
		} else {
			sl.GetNext(Pos);
		}
	} else {
		if (Pos == sl.GetHeadPosition()) {
			if (s.fNextInDirAfterPlaybackLooped) {
				Pos = sl.GetTailPosition();
			} else {
				return 0;
			}
		} else {
			sl.GetPrev(Pos);
		}
	}

	CAtlList<CString> Play_sl;
	Play_sl.AddHead(sl.GetAt(Pos));
	m_wndPlaylistBar.Open(Play_sl, false);
	OpenCurPlaylistItem();

	return (sl.GetCount());
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
						::SendMessage (pTSD->Hwnd, WM_TUNER_STATS, lStrength, lQuality);
						pTun->Scan (ulFrequency + lOffsets[nOffsetPos], pTSD->Hwnd);
						bSucceeded = true;
					}
				}

				nProgress = MulDiv(ulFrequency-pTSD->FrequencyStart, 100, pTSD->FrequencyStop-pTSD->FrequencyStart);
				::SendMessage (pTSD->Hwnd, WM_TUNER_SCAN_PROGRESS, nProgress,0);
				::SendMessage (pTSD->Hwnd, WM_TUNER_STATS, lStrength, lQuality);

				if (m_bStopTunerScan) {
					break;
				}
			}

			::SendMessage (pTSD->Hwnd, WM_TUNER_SCAN_END, 0, 0);
		}
	}
}

// dynamic menus

void CMainFrame::SetupOpenCDSubMenu()
{
	CMenu* pSub = &m_openCDsMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState == MLS_LOADING || AfxGetAppSettings().fHideCDROMsSubMenu) {
		return;
	}

	UINT id = ID_FILE_OPEN_CD_START;

	for (TCHAR drive = 'C'; drive <= 'Z'; drive++) {
		CAtlList<CString> files;
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
			str.Format(_T("%s (%c:)"), label, drive);
			if (!str.IsEmpty()) {
				pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, str);
			}
		}
	}
}

void CMainFrame::SetupFiltersSubMenu()
{
	CMenu* pSub = &m_filtersMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	m_pparray.RemoveAll();
	m_ssarray.RemoveAll();

	if (m_iMediaLoadState == MLS_LOADED) {
		UINT idf = 0;
		UINT ids = ID_FILTERS_SUBITEM_START;
		UINT idl = ID_FILTERSTREAMS_SUBITEM_START;

		BeginEnumFilters(m_pGB, pEF, pBF) {
			CString name(GetFilterName(pBF));
			if (name.GetLength() >= 43) {
				name = name.Left(40) + _T("...");
			}

			CLSID clsid = GetCLSID(pBF);
			if (clsid == CLSID_AVIDec) {
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if (pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
					DWORD c = ((VIDEOINFOHEADER*)mt.pbFormat)->bmiHeader.biCompression;
					switch (c) {
						case BI_RGB:
							name += _T(" (RGB)");
							break;
						case BI_RLE4:
							name += _T(" (RLE4)");
							break;
						case BI_RLE8:
							name += _T(" (RLE8)");
							break;
						case BI_BITFIELDS:
							name += _T(" (BITF)");
							break;
						default:
							name.Format(_T("%s (%c%c%c%c)"),
										CString(name), (TCHAR)(c & 0xff), (TCHAR)((c >> 8) & 0xff), (TCHAR)((c >> 16) & 0xff), (TCHAR)((c >> 24) & 0xff));
							break;
					}
				}
			} else if (clsid == CLSID_ACMWrapper) {
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if (pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
					WORD c = ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag;
					name.Format(_T("%s (0x%04x)"), CString(name), (int)c);
				}
			} else if (clsid == __uuidof(CTextPassThruFilter)
					|| clsid == __uuidof(CNullTextRenderer)
					|| clsid == __uuidof(ChaptersSouce)
					|| clsid == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) { // ISCR
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
				name.Replace(_T("&"), _T("&&"));

				if (pSPP = pPin) {
					CAUUID caGUID;
					caGUID.pElems = NULL;
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
				WCHAR* wname	= NULL;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);

				if (nStreams > 0 && nPPages > 0) {
					subMenu.AppendMenu(MF_SEPARATOR | MF_ENABLED);
				}

				UINT idlstart = idl;

				for (DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL) {
					m_ssarray.Add(pSS);

					flags = group = DWORD_MAX;
					wname = NULL;
					pSS->Info(i, NULL, &flags, NULL, &group, &wname, &pObj, &pUnk);

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
						swprintf_s(wname, count, L"%s %d", stream, min(i + 1,999));
					}

					CString name(wname);
					name.Replace(_T("&"), _T("&&"));

					subMenu.AppendMenu(nflags, idl++, name);

					CoTaskMemFree(wname);
				}

				if (!nStreams) {
					pSS.Release();
				}
			}

			if (nPPages == 1 && !pSS) {
				pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ids, name);
			} else {
				pSub->AppendMenu(MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, idf, name);

				if (nPPages > 0 || pSS) {
					MENUITEMINFO mii;
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_STATE | MIIM_SUBMENU;
					mii.fType = MF_POPUP;
					mii.hSubMenu = subMenu.Detach();
					mii.fState = (pSPP || pSS) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
					pSub->SetMenuItemInfo(idf, &mii, TRUE);
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
	CMenu* pSub = &m_languageMenu;
	int iCount = 0;
	const AppSettings& s = AfxGetAppSettings();

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	for (size_t i = 0; i < CMPlayerCApp::languageResourcesCount; i++) {

		const LanguageResource& lr	= CMPlayerCApp::languageResources[i];
		CString strSatellite		= CMPlayerCApp::GetSatelliteDll(i);

		if (!strSatellite.IsEmpty() || lr.resourceID == ID_LANGUAGE_ENGLISH) {
			HMODULE lib = LoadLibrary(strSatellite);
			if (lib != NULL || lr.resourceID == ID_LANGUAGE_ENGLISH) {
				if (lib) {
					FreeLibrary(lib);
				}
				UINT uFlags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (i == s.iLanguage) {
					uFlags |= MF_CHECKED | MFT_RADIOCHECK;
				}
				pSub->AppendMenu(uFlags, i + ID_LANGUAGE_ENGLISH, lr.name);
				iCount++;
			}
		}
	}

	if (iCount <= 1) {
		pSub->RemoveMenu(0, MF_BYPOSITION);
	}
}

void CMainFrame::SetupAudioSwitcherSubMenu()
{
	CMenu* pSub = &m_audiosMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState == MLS_LOADED) {
		UINT id = ID_AUDIO_SUBITEM_START;

		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;
		if (pSS) {
			DWORD cStreams = 0;
			if (SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0) {
				pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_OPTIONS));
				pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);

				for (DWORD i = 0; i < cStreams; i++) {
					WCHAR* pName = NULL;
					if (FAILED(pSS->Info(i, NULL, NULL, NULL, NULL, &pName, NULL, NULL))) {
						break;
					}

					CString name(pName);
					name.Replace(_T("&"), _T("&&"));

					pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, name);

					CoTaskMemFree(pName);
				}
			}
		}
	}
}

void CMainFrame::SetupAudioOptionSubMenu()
{
	CMenu* pSub = &m_audiosMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	UINT id = ID_AUDIO_SUBITEM_START;

	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_OPTIONS));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_AG_LOAD_AUDIO));
}

void CMainFrame::SetupSubtitlesSubMenu()
{
	CMenu* pSub = &m_subtitlesMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	UINT id = ID_SUBTITLES_SUBITEM_START;

	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_OPTIONS));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_AG_LOAD_SUBTITLE));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_STYLES));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_RELOAD));
	pSub->AppendMenu(MF_SEPARATOR);

	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_ENABLE));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_DEFAULT_STYLE));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_FORCED));
	pSub->AppendMenu(MF_SEPARATOR);

	CMenu subMenu;
	subMenu.CreatePopupMenu();
	subMenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_STEREO_DONTUSE));
	subMenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_STEREO_SIDEBYSIDE));
	subMenu.AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, id++, ResStr(IDS_SUBTITLES_STEREO_TOPANDBOTTOM));
	pSub->AppendMenu(MF_STRING | MF_POPUP | MF_ENABLED, (UINT_PTR)subMenu.Detach(), ResStr(IDS_SUBTITLES_STEREO));
}

void CMainFrame::SetupNavMixSubtitleSubMenu()
{
	CMenu* pSub = &m_navMixSubtitleMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_NAVIGATE_SUBP_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		SetupNavMixStreamSubtitleSelectSubMenu(pSub, id, 2);
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

		pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | (bIsDisabled?0:MF_CHECKED), id++, ResStr(IDS_AG_ENABLED));
		pSub->AppendMenu(MF_BYCOMMAND | MF_SEPARATOR | MF_ENABLED);

		for (ULONG i = 0; i < ulStreamsAvailable; i++) {
			LCID Language;
			if (FAILED(m_pDVDI->GetSubpictureLanguage(i, &Language))) {
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
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(max(len-1, 0));
			} else {
				str.Format(ResStr(IDS_AG_UNKNOWN), i+1);
			}

			DVD_SubpictureAttributes ATR;
			if (SUCCEEDED(m_pDVDI->GetSubpictureAttributes(i, &ATR))) {
				switch (ATR.LanguageExtension) {
					case DVD_SP_EXT_NotSpecified:
					default:
						break;
					case DVD_SP_EXT_Caption_Normal:
						str += _T("");
						break;
					case DVD_SP_EXT_Caption_Big:
						str += _T(" (Big)");
						break;
					case DVD_SP_EXT_Caption_Children:
						str += _T(" (Children)");
						break;
					case DVD_SP_EXT_CC_Normal:
						str += _T(" (CC)");
						break;
					case DVD_SP_EXT_CC_Big:
						str += _T(" (CC Big)");
						break;
					case DVD_SP_EXT_CC_Children:
						str += _T(" (CC Children)");
						break;
					case DVD_SP_EXT_Forced:
						str += _T(" (Forced)");
						break;
					case DVD_SP_EXT_DirectorComments_Normal:
						str += _T(" (Director Comments)");
						break;
					case DVD_SP_EXT_DirectorComments_Big:
						str += _T(" (Director Comments, Big)");
						break;
					case DVD_SP_EXT_DirectorComments_Children:
						str += _T(" (Director Comments, Children)");
						break;
				}
			}

			str.Replace(_T("&"), _T("&&"));

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavMixStreamSubtitleSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup)
{

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		if (m_pDVS) {
			CComQIPtr<IAMStreamSelect> pSS	= m_pMainSourceFilter;
			int nLangs;
			if (SUCCEEDED(m_pDVS->get_LanguageCount(&nLangs)) && nLangs) {

				bool fHideSubtitles = false;
				m_pDVS->get_HideSubtitles(&fHideSubtitles);
				pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED), id++, ResStr(IDS_SUBTITLES_ENABLE));
				pSub->AppendMenu(MF_SEPARATOR);

				int subcount = GetStreamCount(dwSelGroup);
				if (subcount) {
					int iSelectedLanguage = 0;
					m_pDVS->get_SelectedLanguage(&iSelectedLanguage);

					if (nLangs > 1) {
						for (int i = 0; i < nLangs - 1; i++) {
							WCHAR *pName = NULL;
							m_pDVS->get_LanguageName(i, &pName);
							CString streamName(pName);
							CoTaskMemFree(pName);

							UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
							if (i == iSelectedLanguage) {
								flags |= MF_CHECKED | MFT_RADIOCHECK;
							}

							streamName.Replace(_T("&"), _T("&&"));
							pSub->AppendMenu(flags, id++, streamName);
						}

						pSub->AppendMenu(MF_SEPARATOR);
					}

					DWORD cStreams;
					pSS->Count(&cStreams);
					UINT baseid = id;

					for (int i = 0, j = cStreams; i < j; i++) {
						DWORD dwFlags	= DWORD_MAX;
						DWORD dwGroup	= DWORD_MAX;
						LCID lcid		= DWORD_MAX;
						WCHAR* pszName = NULL;

						if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
								|| !pszName) {
							continue;
						}

						CString name(pszName);

						if (pszName) {
							CoTaskMemFree(pszName);
						}

						if (dwGroup != dwSelGroup) {
							continue;
						}

						CString str = FormatStreamName(name, lcid, id - baseid);

						UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
						if (dwFlags && iSelectedLanguage == (nLangs - 1)) {
							flags |= MF_CHECKED | MFT_RADIOCHECK;
						}

						str.Replace(_T("&"), _T("&&"));
						pSub->AppendMenu(flags, id++, str);
					}

					return;
				}

				int iSelectedLanguage = 0;
				m_pDVS->get_SelectedLanguage(&iSelectedLanguage);

				for (int i = 0; i < nLangs; i++) {
					WCHAR *pName = NULL;
					m_pDVS->get_LanguageName(i, &pName);
					CString streamName(pName);
					CoTaskMemFree(pName);

					UINT flags = MF_BYCOMMAND | MF_STRING | (!fHideSubtitles ? MF_ENABLED : MF_DISABLED);
					if (i == iSelectedLanguage) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}

					streamName.Replace(_T("&"), _T("&&"));
					pSub->AppendMenu(flags, id++, streamName);
				}
			}

			return;
		}

		POSITION pos = m_pSubStreams.GetHeadPosition();

		pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | (pos ? MF_ENABLED : MF_DISABLED), id++, ResStr(IDS_SUBTITLES_ENABLE));
		pSub->AppendMenu(MF_SEPARATOR);

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
					DWORD dwFlags	= DWORD_MAX;
					DWORD dwGroup	= DWORD_MAX;
					LCID lcid		= DWORD_MAX;
					WCHAR* pszName	= NULL;

					if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
							|| !pszName) {
						continue;
					}

					CString name(pszName);

					if (pszName) {
						CoTaskMemFree(pszName);
					}

					if (dwGroup != dwSelGroup) {
						continue;
					}

					CString str = FormatStreamName(name, lcid, id - baseid);

					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
					if (dwFlags) {
						if (m_iSubtitleSel == 0) {
							flags |= MF_CHECKED | MFT_RADIOCHECK;
						}

					}

					str.Replace(_T("&"), _T("&&"));
					intsub++;
					pSub->AppendMenu(flags, id++, str);
					splcnt++;
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

				WCHAR* pName = NULL;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL))) {
					CString name(pName);
					name.Replace(_T("&"), _T("&&"));

					if ((splcnt > 0 || (cntintsub == intsub && cntintsub != 0)) && !sep) {
						pSub->AppendMenu(MF_SEPARATOR);
						sep = true;
					}

					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					if (cntintsub <= intsub) {
						name = _T("* ") + name;
					}
					pSub->AppendMenu(flags, id++, name);
					intsub++;
					CoTaskMemFree(pName);
				} else {
					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

					if (m_iSubtitleSel == tPos) {
						flags |= MF_CHECKED | MFT_RADIOCHECK;
					}
					CString sname;
					if (cntintsub <= intsub) {
						sname = _T("* ") + ResStr(IDS_AG_UNKNOWN);
					}
					pSub->AppendMenu(flags, id++, sname);
					intsub++;
				}
			}
		}

		if ((pSub->GetMenuItemCount() < 2) || (!m_pSubStreams.GetHeadPosition() && pSub->GetMenuItemCount() <= 3)) {
			while (pSub->RemoveMenu(0, MF_BYPOSITION));
		}
	}
}

void CMainFrame::SetupVideoStreamsSubMenu()
{
	CMenu* pSub = &m_videoStreamsMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_NAVIGATE_ANGLE_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		SetupNavStreamSelectSubMenu(pSub, id, 0);
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

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

static CString StripPath(CString path)
{
	CString p = path;
	p.Replace('\\', '/');
	p = p.Mid(p.ReverseFind('/')+1);
	return(p.IsEmpty() ? path : p);
}

void CMainFrame::SetupNavChaptersSubMenu()
{
	CMenu* pSub = &m_chaptersMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_NAVIGATE_CHAP_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE) {
		if (m_MPLSPlaylist.GetCount() > 1) {
			DWORD idx = 1;
			POSITION pos = m_MPLSPlaylist.GetHeadPosition();
			while (pos) {
				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (id != ID_NAVIGATE_CHAP_SUBITEM_START && pos == m_MPLSPlaylist.GetHeadPosition()) {
					//pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);
					flags |= MF_MENUBARBREAK;
				}
				if (idx == MENUBARBREAK) {
					flags |= MF_MENUBARBREAK;
					idx = 0;
				}
				idx++;

				CHdmvClipInfo::PlaylistItem* Item = m_MPLSPlaylist.GetNext(pos);
				CString time = _T("[") + ReftimeToString2(Item->Duration()) + _T("]");
				CString name = StripPath(Item->m_strFileName);

				if (name == m_wndPlaylistBar.m_pl.GetHead().GetLabel()) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				name.Replace(_T("&"), _T("&&"));
				pSub->AppendMenu(flags, id++, name + '\t' + time);
			}
		}

		SetupChapters();
		REFERENCE_TIME rt = GetPos();
		DWORD j = m_pCB->ChapLookup(&rt, NULL);

		if (m_pCB->ChapGetCount() > 1) {
			for (DWORD i = 0, idx = 0; i < m_pCB->ChapGetCount(); i++, id++, idx++) {
				rt = 0;
				CComBSTR bstr;
				if (FAILED(m_pCB->ChapGet(i, &rt, &bstr))) {
					continue;
				}

				CString time = _T("[") + ReftimeToString2(rt) + _T("]");

				CString name = CString(bstr);
				name.Replace(_T("&"), _T("&&"));
				name.Replace(_T("\t"), _T(" "));

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
					if (m_MPLSPlaylist.GetCount() > 1) {
						flags |= MF_MENUBARBREAK;
					}
				}
				pSub->AppendMenu(flags, id, name + '\t' + time);
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
					pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);
				}
				CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);
				CString name = pli.GetLabel();
				name.Replace(_T("&"), _T("&&"));
				pSub->AppendMenu(flags, id++, name);
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

			pSub->AppendMenu(flags, id++, str);
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

			pSub->AppendMenu(flags, id++, str);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1) {
		AppSettings& s = AfxGetAppSettings();

		POSITION pos = s.m_DVBChannels.GetHeadPosition();
		while (pos) {
			CDVBChannel& Channel = s.m_DVBChannels.GetNext(pos);
			UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

			if ((UINT)Channel.GetPrefNumber() == s.nDVBLastChannel) {
				flags |= MF_CHECKED | MFT_RADIOCHECK;
			}
			pSub->AppendMenu(flags, ID_NAVIGATE_CHAP_SUBITEM_START + Channel.GetPrefNumber(), Channel.GetName());
		}
	}
}

IBaseFilter* CMainFrame::FindSwitcherFilter()
{
	IBaseFilter* pSF = NULL;

	pSF = FindFilter(__uuidof(CAudioSwitcherFilter), m_pGB);
	if (!pSF) {
		pSF = FindFilter(CLSID_MorganSwitcher, m_pGB);
	}

	return pSF;
}

void CMainFrame::SetupNavStreamSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup)
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
		DWORD dwFlags	= DWORD_MAX;
		DWORD dwGroup	= DWORD_MAX;
		LCID lcid		= DWORD_MAX;
		WCHAR* pszName	= NULL;

		if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
				|| !pszName) {
			continue;
		}

		CString name(pszName);

		if (pszName) {
			CoTaskMemFree(pszName);
		}

		if (dwGroup != dwSelGroup) {
			continue;
		}

		if (dwPrevGroup != -1 && dwPrevGroup != dwGroup) {
			pSub->AppendMenu(MF_SEPARATOR);
		}
		dwPrevGroup = dwGroup;

		CString str = FormatStreamName(name, lcid, id - baseid);

		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		if (dwFlags) {
			flags |= MF_CHECKED | MFT_RADIOCHECK;
		}

		str.Replace(_T("&"), _T("&&"));
		pSub->AppendMenu(flags, id++, str);
	}
}

void CMainFrame::OnNavStreamSelectSubMenu(UINT id, DWORD dwSelGroup)
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

		if (FAILED(pSS->Info(i, NULL, NULL, NULL, &dwGroup, NULL, NULL, NULL))) {
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

void CMainFrame::SetupNavMixStreamSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup)
{
	bool bSetCheck = true;
	CComQIPtr<IAMStreamSelect> pSSA = m_pSwitcherFilter;
	if (pSSA) {
		DWORD cStreamsA = 0;
		if (SUCCEEDED(pSSA->Count(&cStreamsA)) && cStreamsA > 1) {
			for (DWORD n = 1; n < cStreamsA; n++) {
				DWORD dwFlags = DWORD_MAX;
				if (SUCCEEDED(pSSA->Info(n, NULL, &dwFlags, NULL, NULL, NULL, NULL, NULL))) {
					if (dwFlags & AMSTREAMSELECTINFO_EXCLUSIVE/* ||flags&AMSTREAMSELECTINFO_ENABLED*/) {
						bSetCheck = false;
						break;
					}
				}
			}
		}
	}

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		UINT baseid = id;
		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;

		if (pSS) {
			DWORD cStreams;
			if (!FAILED(pSS->Count(&cStreams))) {
				DWORD dwPrevGroup = (DWORD)-1;

				for (int i = 0, j = cStreams; i < j; i++) {
					DWORD dwFlags	= DWORD_MAX;
					DWORD dwGroup	= DWORD_MAX;
					LCID lcid		= DWORD_MAX;
					WCHAR* pszName = NULL;

					if (FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
							|| !pszName) {
						continue;
					}

					CString name(pszName);

					if (pszName) {
						CoTaskMemFree(pszName);
					}

					if (dwGroup != dwSelGroup) {
						continue;
					}

					if (dwPrevGroup != -1 && dwPrevGroup != dwGroup) {
						pSub->AppendMenu(MF_SEPARATOR);
					}

					dwPrevGroup = dwGroup;
					CString str = FormatStreamName(name, lcid, id - baseid);

					UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
					if (dwFlags) {
						if (bSetCheck == true) {
							flags |= MF_CHECKED | MFT_RADIOCHECK;
						}
					}

					str.Replace(_T("&"), _T("&&"));
					pSub->AppendMenu(flags, id++, str);
				}
			}
		}
	}

	if (pSSA) {
		DWORD cStreamsA = 0;
		bool sp = false;
		if (SUCCEEDED(pSSA->Count(&cStreamsA)) && cStreamsA > 0) {
			bool sep = false;
			DWORD i = 0;
			if (pSub->GetMenuItemCount() > 0) {// do not include menu item "Options" and separator
				i = 1;
				sp = true;
			}
			for (i; i < cStreamsA; i++) {
				WCHAR* pName	= NULL;
				DWORD dwFlags	= DWORD_MAX;
				if (FAILED(pSSA->Info(i, NULL, &dwFlags, NULL, NULL, &pName, NULL, NULL))) {
					break;
				}

				CString name(pName);
				CoTaskMemFree(pName);

				UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
				if (dwFlags) {
					flags |= MF_CHECKED | MFT_RADIOCHECK;
				}

				bool fExternal = false;
				CString str;
				CPlaylistItem pli;
				if (m_wndPlaylistBar.GetCur(pli)) {
					POSITION pos = pli.m_fns.GetHeadPosition();
					// skip main file
					pli.m_fns.GetNext(pos);
					while (pos) {
						str = pli.m_fns.GetNext(pos).GetName();
						if (str.GetLength() > 0 && name == GetFileOnly(str)) {
							fExternal = true;
							break;
						}
					}
				}

				if (!sep && cStreamsA > 1 && fExternal) {
					if (pSub->GetMenuItemCount()) {
						pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);
					}
					sep = true;
				}

				name.Replace(_T("&"), _T("&&"));
				pSub->AppendMenu(flags, id++, name);
			}
			sep = false;
		}
	}
}

void CMainFrame::SetupNavMixAudioSubMenu()
{
	CMenu* pSub = &m_navMixAudioMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState != MLS_LOADED) {
		return;
	}

	UINT id = ID_NAVIGATE_AUDIO_SUBITEM_START;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {
		SetupNavMixStreamSelectSubMenu(pSub, id, 1);
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
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
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
						str += _T(" (Captions)");
						break;
					case DVD_AUD_EXT_VisuallyImpaired:
						str += _T(" (Visually Impaired)");
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

			str.Replace(_T("&"), _T("&&"));

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::OnNavMixStreamSelectSubMenu(UINT id, DWORD dwSelGroup)
{
	bool bSplitterMenu = false;
	int nID = (int)id;

	CComQIPtr<IAMStreamSelect> pSSA = m_pSwitcherFilter;

	if (GetPlaybackMode() == PM_FILE || (GetPlaybackMode() == PM_CAPTURE && AfxGetAppSettings().iDefaultCaptureDevice == 1)) {

		CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter;
		if (pSS) {
			DWORD cStreams = 0;
			if (!FAILED(pSS->Count(&cStreams))) {
				for (int m = 0, j = cStreams; m < j; m++) {
					DWORD dwFlags = DWORD_MAX;
					DWORD dwGroup = DWORD_MAX;

					if (FAILED(pSS->Info(m, NULL, &dwFlags, NULL, &dwGroup, NULL, NULL, NULL))) {
						continue;
					}

					if (dwGroup != dwSelGroup) {
						continue;
					}

					bSplitterMenu = true;
					if (nID == 0) {

						bool bExternalTrack = false;
						if (pSSA) {
							DWORD cStreamsA = 0;
							if (SUCCEEDED(pSSA->Count(&cStreamsA)) && cStreamsA > 1) {
								for (DWORD n = 1; n < cStreamsA; n++) {
									DWORD flags = DWORD_MAX;
									if (SUCCEEDED(pSSA->Info(n, NULL, &flags, NULL, NULL, NULL, NULL, NULL))) {
										if (flags & AMSTREAMSELECTINFO_EXCLUSIVE/* ||flags&AMSTREAMSELECTINFO_ENABLED*/) {
											bExternalTrack = true;
											break;
										}
									}
								}
							}
						}

						if (bExternalTrack) {
							pSSA->Enable(0, AMSTREAMSELECTENABLE_ENABLE);
							pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
							return;
						}
						pSS->Enable(m, AMSTREAMSELECTENABLE_ENABLE);
						return;
					}

					nID--;
				}
			}
		}
	} else if (GetPlaybackMode() == PM_DVD) {
		m_pDVDC->SelectAudioStream(id, DVD_CMD_FLAG_Block, NULL);
		return;
	}

	if (nID >= 0 && pSSA) {
		DWORD cStreamsA = 0;
		if (SUCCEEDED(pSSA->Count(&cStreamsA)) && cStreamsA > 0) {
			UINT i = (UINT)nID;

			if (bSplitterMenu && (cStreamsA > 1)) {
				i++;
			}

			pSSA->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
		}
	}
}

void CMainFrame::SetupRecentFilesSubMenu()
{
	CMenu* pSub = &m_recentfilesMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	if (m_iMediaLoadState == MLS_LOADING) {
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
		pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_RECENT_FILES_CLEAR, ResStr(IDS_RECENT_FILES_CLEAR));
		pSub->AppendMenu(MF_SEPARATOR | MF_ENABLED);
	}

	for (int i = 0; i < MRU.GetSize(); i++) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;
		if (!MRU[i].IsEmpty()) {
			CString path(MRU[i]);
			if (path.Find(L"\\") != 0 && ::PathIsDirectory(path)) {
				CString tmp = path + L"\\VIDEO_TS.IFO";
				if (::PathFileExists(tmp)) {
					path = L"DVD - " + path;
				} else {
					tmp = path + L"\\BDMV\\index.bdmv";
					if (::PathFileExists(tmp)) {
						path.Format(L"%s - %s", BLU_RAY, path);
					}
				}
			}

			pSub->AppendMenu(flags, id, path);
		}
		id++;
	}
}

void CMainFrame::SetupFavoritesSubMenu()
{
	CMenu* pSub = &m_favoritesMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	AppSettings& s = AfxGetAppSettings();

	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ADD, ResStr(IDS_FAVORITES_ADD));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_FAVORITES_ORGANIZE, ResStr(IDS_FAVORITES_ORGANIZE));

	UINT nLastGroupStart = pSub->GetMenuItemCount();

	UINT id = ID_FAVORITES_FILE_START;

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	POSITION pos = sl.GetHeadPosition();
	while (pos) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString f_str = sl.GetNext(pos);
		f_str.Replace(_T("&"), _T("&&"));
		f_str.Replace(_T("\t"), _T(" "));

		CAtlList<CString> sl;
		ExplodeEsc(f_str, sl, _T(';'), 3);

		f_str = sl.RemoveHead();

		CString str;

		if (!sl.IsEmpty()) {
			// pos
			REFERENCE_TIME rt = 0;
			if (1 == _stscanf_s(sl.GetHead(), _T("%I64d"), &rt) && rt > 0) {
				DVD_HMSF_TIMECODE hmsf = RT2HMSF(rt);
				str.Format(_T("[%02u:%02u:%02u]"), hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
			}

			// relative drive
			if ( sl.GetCount() > 1 ) { // Here to prevent crash if old favorites settings are present
				sl.RemoveHead();

				BOOL bRelativeDrive = FALSE;
				if ( _stscanf_s(sl.GetHead(), _T("%d"), &bRelativeDrive) == 1 ) {
					if (bRelativeDrive) {
						str.Format(_T("[RD]%s"), CString(str));
					}
				}
			}
			if (!str.IsEmpty()) {
				f_str.Format(_T("%s\t%.14s"), CString(f_str), CString(str));
			}
		}

		if (!f_str.IsEmpty()) {
			pSub->AppendMenu(flags, id, f_str);
		}

		id++;
	}

	if (id > ID_FAVORITES_FILE_START) {
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DVD_START;

	s.GetFav(FAV_DVD, sl);

	pos = sl.GetHeadPosition();
	while (pos) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString str = sl.GetNext(pos);
		str.Replace(_T("&"), _T("&&"));

		CAtlList<CString> sl;
		ExplodeEsc(str, sl, _T(';'), 2);

		str = sl.RemoveHead();

		if (!sl.IsEmpty()) {
			// TODO
		}

		if (!str.IsEmpty()) {
			pSub->AppendMenu(flags, id, str);
		}

		id++;
	}

	if (id > ID_FAVORITES_DVD_START) {
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR | MF_ENABLED | MF_BYPOSITION);
	}

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DEVICE_START;

	s.GetFav(FAV_DEVICE, sl);

	pos = sl.GetHeadPosition();
	while (pos) {
		UINT flags = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

		CString str = sl.GetNext(pos);
		str.Replace(_T("&"), _T("&&"));

		CAtlList<CString> sl;
		ExplodeEsc(str, sl, _T(';'), 2);

		str = sl.RemoveHead();

		if (!str.IsEmpty()) {
			pSub->AppendMenu(flags, id, str);
		}

		id++;
	}
}

void CMainFrame::SetupShadersSubMenu()
{
	CMenu* pSub = &m_shadersMenu;

	if (!IsMenu(pSub->m_hMenu)) {
		pSub->CreatePopupMenu();
	} else while (pSub->RemoveMenu(0, MF_BYPOSITION));

	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_TOGGLE, ResStr(IDS_SHADERS_TOGGLE));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_TOGGLE_SCREENSPACE, ResStr(IDS_SHADERS_TOGGLE_SCREENSPACE));
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SHADERS_SELECT, ResStr(IDS_SHADERS_SELECT));
	pSub->AppendMenu(MF_SEPARATOR);
	pSub->AppendMenu(MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_VIEW_SHADEREDITOR, ResStr(IDS_SHADERS_EDIT));
}

/////////////

void CMainFrame::ShowControls(int nCS, bool fSave)
{
	int nCSprev = AfxGetAppSettings().nCS;
	int hbefore = 0, hafter = 0;

	m_pLastBar = NULL;

	POSITION pos = m_bars.GetHeadPosition();
	for (int i = 1; pos; i <<= 1) {
		CControlBar* pNext = m_bars.GetNext(pos);
		ShowControlBar(pNext, !!(nCS&i), TRUE);
		if (nCS&i) {
			m_pLastBar = pNext;
		}

		CSize s = pNext->CalcFixedLayout(FALSE, TRUE);
		if (nCSprev&i) {
			hbefore += s.cy;
		}
		if (nCS&i) {
			hafter += s.cy;
		}
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	if (wp.showCmd != SW_SHOWMAXIMIZED && !m_fFullScreen) {
		CRect r;
		GetWindowRect(r);
		MoveWindow(r.left, r.top, r.Width(), r.Height()+(hafter-hbefore));
	}

	if (fSave) {
		AfxGetAppSettings().nCS = nCS;
	}

	RecalcLayout();
}

void CMainFrame::SetAlwaysOnTop(int i)
{
	AfxGetAppSettings().iOnTop = i;

	if (!m_fFullScreen) {
		const CWnd* pInsertAfter = NULL;

		if (i == 0) {
			pInsertAfter = &wndNoTopMost;
		} else if (i == 1) {
			pInsertAfter = &wndTopMost;
		} else if (i == 2) {
			pInsertAfter = GetMediaState() == State_Running ? &wndTopMost : &wndNoTopMost;
		} else { // if (i == 3)
			pInsertAfter = (GetMediaState() == State_Running && !m_bAudioOnly) ? &wndTopMost : &wndNoTopMost;
		}

		SetWindowPos(pInsertAfter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	} else if (!(GetWindowLongPtr(m_hWnd, GWL_EXSTYLE)&WS_EX_TOPMOST)) {
		if (!AfxGetAppSettings().IsD3DFullscreen()) {
			SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		}
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
			if (SUCCEEDED(pPinTo->QueryPinInfo(&pi)) &&
				GetCLSID(pi.pFilter) != __uuidof(CNullTextRenderer)
					&& GetCLSID(pi.pFilter) != GUIDFromCString(_T("{04FE9017-F873-410E-871E-AB91661A4EF7}"))	// ffdshow video decoder
					&& GetCLSID(pi.pFilter) != GUIDFromCString(_T("{DBF9000E-F08C-4858-B769-C914A0FBB1D7}"))) {	// ffdshow subtitles filter
				continue;
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

			if (FAILED(hr = m_pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), NULL))
					|| FAILED(hr = m_pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), pPinTo, NULL))) {
				hr = m_pGB->ConnectDirect(pPin, pPinTo, NULL);
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
	AppSettings& s = AfxGetAppSettings();

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
					if (actualStream != NULL) {
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
			if (pSSF && GetFileExt(fname).MakeLower() == _T(".sup")) {
				if (pSSF->Open(fname, subItem.GetTitle(), videoName)) {
					pSubStream = pSSF.Detach();
				}
			}
		}

		if (!pSubStream) {
			CAutoPtr<CVobSubFile> pVSF(DNew CVobSubFile(&m_csSubLock));
			if (GetFileExt(fname).MakeLower() == _T(".idx") && pVSF && pVSF->Open(fname) && pVSF->GetStreamCount() > 0) {
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
		if (actualStream != NULL) {
			*actualStream = r;

			s.fEnableSubtitles = true;
		}

		if (m_hNotifyRenderThread) {
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
				HANDLE h = FindFirstChangeNotification(fname, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
				if (h != INVALID_HANDLE_VALUE) {
					m_ExtSubPaths.Add(fname);
					m_ExtSubPathsHandles.Add(h);
					SetEvent(m_hRefreshNotifyRenderThreadEvent);
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
				WCHAR* pName = NULL;
				if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL))) {
					CString	strMessage;
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

	m_pCAP->SetSubPicProvider(NULL);
}

void CMainFrame::SetSubtitle(ISubStream* pSubStream, int iSubtitleSel/* = -1*/, bool fApplyDefStyle/* = false*/)
{
	AppSettings& s = AfxGetAppSettings();

	s.m_RenderersSettings.bPositionRelative	= 0;
	s.m_RenderersSettings.nShiftPos = {0, 0};

	{
		CAutoLock cAutoLock(&m_csSubLock);
		
		if (pSubStream) {
			CLSID clsid;
			pSubStream->GetClassID(&clsid);

			if (clsid == __uuidof(CVobSubFile)) {
				CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

				if (fApplyDefStyle) {
					pVSF->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos, 1, 1);
				}
			} else if (clsid == __uuidof(CVobSubStream)) {
				CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)pSubStream;

				if (fApplyDefStyle) {
					pVSS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos, 1, 1);
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

				if (m_pCAP) {
					bool bKeepAspectRatio = s.fKeepAspectRatio;
					CSize szAspectRatio = m_pCAP->GetVideoSize(true);
					CSize szVideoFrame;
					if (CComQIPtr<IMadVRInfo> pMVRI = m_pCAP) {
						// Use IMadVRInfo to get size. See http://bugs.madshi.net/view.php?id=180
						pMVRI->GetSize("originalVideoSize", &szVideoFrame);
						bKeepAspectRatio = true;
					} else {
						szVideoFrame = m_pCAP->GetVideoSize(false);
					}

					pRTS->m_ePARCompensationType = CSimpleTextSubtitle::EPARCompensationType::EPCTAccurateSize_ISR;
					if (szAspectRatio.cx && szAspectRatio.cy && szVideoFrame.cx && szVideoFrame.cy && bKeepAspectRatio) {
						pRTS->m_dPARCompensation = ((double)szAspectRatio.cx / szAspectRatio.cy) /
													((double)szVideoFrame.cx / szVideoFrame.cy);
					} else {
						pRTS->m_dPARCompensation = 1.0;
					}
				}

				pRTS->Deinit();
			} else if (clsid == __uuidof(CRenderedHdmvSubtitle) || clsid == __uuidof(CSupSubFile)) {
				s.m_RenderersSettings.bPositionRelative	= s.subdefstyle.relativeTo;
			}
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
	} else if (dynamic_cast<CRenderedHdmvSubtitle*>((ISubStream*)m_pCurrentSubStream)) {
		s.m_RenderersSettings.bPositionRelative	= s.subdefstyle.relativeTo;
		InvalidateSubtitle();
		if (GetMediaState() != State_Running) {
			m_pCAP->Paint(false);
		}
	}
}

void CMainFrame::SetSubtitleTrackIdx(int index)
{
	if (m_iMediaLoadState == MLS_LOADED) {
		if (index < 0) {
			m_iSubtitleSel ^= 0x80000000;
		} else {
			POSITION pos = m_pSubStreams.FindIndex(index);
			if (pos) {
				m_iSubtitleSel = index;
			}
		}
		UpdateSubtitle();
		AfxGetAppSettings().fEnableSubtitles = !(m_iSubtitleSel & 0x80000000);
	}
}

void CMainFrame::SetAudioTrackIdx(int index)
{
	if (m_iMediaLoadState == MLS_LOADED) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;

		DWORD cStreams = 0;
		DWORD dwFlags = AMSTREAMSELECTENABLE_ENABLE;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams)))
			if ((index >= 0) && (index < ((int)cStreams))) {
				pSS->Enable(index, dwFlags);
			}
	}
}

REFERENCE_TIME CMainFrame::GetPos()
{
	return(m_iMediaLoadState == MLS_LOADED ? m_wndSeekBar.GetPos() : 0);
}

REFERENCE_TIME CMainFrame::GetDur()
{
	__int64 start, stop;
	m_wndSeekBar.GetRange(start, stop);
	return(m_iMediaLoadState == MLS_LOADED ? stop : 0);
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
	CComQIPtr<IKeyFrameInfo> pKFI;
	BeginEnumFilters(m_pGB, pEF, pBF);
	if (pKFI = pBF) {
		break;
	}
	EndEnumFilters;

	UINT nKFs = 0;
	m_kfs.clear();
	if (pKFI && S_OK == pKFI->GetKeyFrameCount(nKFs) && nKFs > 1) {
		UINT k = nKFs;
		m_kfs.resize(k);
		if (FAILED(pKFI->GetKeyFrames(&TIME_FORMAT_MEDIA_TIME, m_kfs.data(), k)) || k != nKFs) {
			m_kfs.clear();
		}
	}
}

REFERENCE_TIME CMainFrame::GetClosestKeyFrame(REFERENCE_TIME rtTarget) const
{
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
	__int64 start	= 0;
	__int64 stop	= 0;
	if (GetPlaybackMode() != PM_CAPTURE) {
		m_wndSeekBar.GetRange(start, stop);
		if (rtPos > stop) {
			rtPos = stop;
		}
		m_wndStatusBar.SetStatusTimer(rtPos, stop, !!m_wndSubresyncBar.IsWindowVisible(), GetTimeFormat());
		if (bShowOSD) {
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
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
		}

		if (!ValidateSeek(rtPos, stop)) {
			return;
		}

		m_pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

		if (m_bUseReclock) {
			// Crazy ReClock require delay between seek operation or causes crash in EVR :)
			Sleep(50);
		}
	} else if (GetPlaybackMode() == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title) {
		DVD_HMSF_TIMECODE tc = RT2HMSF(rtPos);
		m_pDVDC->PlayAtTime(&tc, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);

		if (fs == State_Paused) {
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);
			Sleep(100);
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
		}
	} else if (GetPlaybackMode() == PM_CAPTURE) {
		//TRACE(_T("Warning (CMainFrame::SeekTo): Trying to seek in capture mode"));
	}
	m_fEndOfStream = false;

	OnTimer(TIMER_STREAMPOSPOLLER);
	OnTimer(TIMER_STREAMPOSPOLLER2);

	SendCurrentPositionToApi(true);
}

bool CMainFrame::ValidateSeek(REFERENCE_TIME rtPos, REFERENCE_TIME rtStop)
{
	// Disable seeking while buffering data if the seeking position is more than a loading progress
	if (m_iMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_FILE && (rtPos > 0) && (rtStop > 0) && (rtPos <= rtStop)) {
		if (m_pAMOP) {
			__int64 t = 0, c = 0;
			if ((SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) || SUCCEEDED(QueryProgressYoutube(&t, &c))) && t > 0 && c < t) {
				int Query_percent	= c*100/t;
				int Seek_percent	= rtPos*100/rtStop;
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
						int Seek_percent = rtPos*100/rtStop;
						if (Seek_percent > BufferingProgress) {
							return false;
						}

					}
					break;
				}
			}
			EndEnumFilters;
		}

		__int64 t = 0, c = 0;
		if (SUCCEEDED(QueryProgressYoutube(&t, &c)) && t > 0 && c < t) {
			int Query_percent	= c*100/t;
			int Seek_percent	= rtPos*100/rtStop;
			if (Seek_percent > Query_percent) {
				return false;
			}
		}
	}

	return true;
}

bool CMainFrame::GetBufferingProgress(int* iProgress)
{
	if (iProgress) {
		*iProgress = 0;
	}

	int Progress = 0;
	if (m_iMediaLoadState == MLS_LOADED && GetPlaybackMode() == PM_FILE) {
		if (m_pAMOP) {
			__int64 t = 0, c = 0;
			if ((SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) || SUCCEEDED(QueryProgressYoutube(&t, &c))) && t > 0 && c < t) {
				Progress = c*100/t;
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

		__int64 t = 0, c = 0;
		if (SUCCEEDED(QueryProgressYoutube(&t, &c)) && t > 0 && c < t) {
			Progress = c*100/t;
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
			TRACE(L"Removing from graph : %s\n", GetFilterName(pBF));

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
			MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		hr = m_pGB->ConnectFilter(pPin, pBuff);
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_CONNECT_BUFF), type);
			MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		pPin = GetFirstPin(pBuff, PINDIR_OUTPUT);
	}

	if (pEnc) {
		hr = m_pGB->AddFilter(pEnc, prefix + L"Encoder");
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_ADD_ENCODER), type);
			MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		hr = m_pGB->ConnectFilter(pPin, pEnc);
		if (FAILED(hr)) {
			err.Format(ResStr(IDS_CAPTURE_ERROR_CONNECT_ENC), type);
			MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
			return hr;
		}

		pPin = GetFirstPin(pEnc, PINDIR_OUTPUT);

		if (CComQIPtr<IAMStreamConfig> pAMSC = pPin) {
			if (pmt->majortype == majortype) {
				hr = pAMSC->SetFormat(pmt);
				if (FAILED(hr)) {
					err.Format(ResStr(IDS_CAPTURE_ERROR_COMPRESSION), type);
					MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
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
			MessageBox(err, ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
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

	*ppVidCapPin = *ppVidPrevPin = NULL;
	*ppAudCapPin = *ppAudPrevPin = NULL;

	CComPtr<IPin> pDVAudPin;

	if (pVidCap) {
		CComPtr<IPin> pPin;
		if (!pAudCap // only look for interleaved stream when we don't use any other audio capture source
				&& SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, TRUE, 0, &pPin))) {
			CComPtr<IBaseFilter> pDVSplitter;
			hr = pDVSplitter.CoCreateInstance(CLSID_DVSplitter);
			hr = m_pGB->AddFilter(pDVSplitter, L"DV Splitter");

			hr = pCGB->RenderStream(NULL, &MEDIATYPE_Interleaved, pPin, NULL, pDVSplitter);

			pPin = NULL;
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pDVAudPin);

			CComPtr<IBaseFilter> pDVDec;
			hr = pDVDec.CoCreateInstance(CLSID_DVVideoCodec);
			hr = m_pGB->AddFilter(pDVDec, L"DV Video Decoder");

			hr = m_pGB->ConnectFilter(pPin, pDVDec);

			pPin = NULL;
			hr = pCGB->FindPin(pDVDec, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
		} else if (FAILED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, TRUE, 0, &pPin))) {
			MessageBox(ResStr(IDS_CAPTURE_ERROR_VID_CAPT_PIN), ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
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
			MessageBox(ResStr(IDS_CAPTURE_ERROR_AUD_CAPT_PIN), ResStr(IDS_CAPTURE_ERROR), MB_ICONERROR | MB_OK);
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
			pVidCapPin = NULL;
		}

		if (fVidPrev) {
			m_pCAP = NULL;
			m_pCAP2 = NULL;
			m_pGB->Render(pVidPrevPin);
			m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP), TRUE);
			m_pGB->FindInterface(IID_PPV_ARGS(&m_pCAP2), TRUE);
		}

		if (fVidCap) {
			IBaseFilter* pBF[3] = {pVidBuffer, pVidEnc, pMux};
			HRESULT hr = BuildCapture(pVidCapPin, pBF, MEDIATYPE_Video, &m_wndCaptureBar.m_capdlg.m_mtcv);
			UNREFERENCED_PARAMETER(hr);
		}

		pAMDF = NULL;
		pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMDroppedFrames, (void**)&pAMDF);
	}

	//if (pAudCap)
	{
		bool fAudPrev = pAudPrevPin && fAPreview;
		bool fAudCap = pAudCapPin && fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput;

		if (fAPreview == 2 && !fAudCap && pAudCapPin) {
			pAudPrevPin = pAudCapPin;
			pAudCapPin = NULL;
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
	hr = pCGB->ControlStream(&PIN_CATEGORY_CAPTURE, NULL, NULL, NULL, &stop, 0, 0); // stop in the infinite

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

	SendMessage(WM_COMMAND, ID_PLAY_PLAY);

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
	AppSettings& s = AfxGetAppSettings();

	CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), m_pGB, GetModalParent(), idPage);

	m_bInOptions = true;
	INT_PTR dResult = options.DoModal();
	if (s.fReset) {
		PostMessage(WM_CLOSE);
		return;
	}

	if (dResult == IDOK) {
		m_bInOptions = false;
		if (!m_fFullScreen) {
			SetAlwaysOnTop(s.iOnTop);
		}

		m_wndView.LoadLogo();
	}
	
	Invalidate();
	m_wndStatusBar.Relayout();
	m_wndPlaylistBar.Invalidate();

	OpenSetupWindowTitle(m_strFnFull);

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
	m_wndStatusBar.m_status.GetWindowText(str);
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
	SetTimer(TIMER_STATUSERASER, nTimeOut, NULL);
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

	CAutoPtr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	if (p) {
		p->bAddRecent = bAddRecent;
		OpenMedia(p);
	}

	return TRUE;
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
	// shortcut
	if (OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p)) {
		if (m_iMediaLoadState == MLS_LOADED && pAMTuner
				&& m_VidDispName == p->DisplayName[0] && m_AudDispName == p->DisplayName[1]) {
			m_wndCaptureBar.m_capdlg.SetVideoInput(p->vinput);
			m_wndCaptureBar.m_capdlg.SetVideoChannel(p->vchannel);
			m_wndCaptureBar.m_capdlg.SetAudioInput(p->ainput);
			return;
		}
	}

	if (m_iMediaLoadState != MLS_CLOSED) {
		CloseMedia();
	}

	AppSettings& s = AfxGetAppSettings();

	bool fUseThread = m_pGraphThread && s.fEnableWorkerThreadForOpening
					  // don't use a worker thread in D3DFullscreen mode except madVR
					  && (!s.IsD3DFullscreen() || s.iDSVideoRendererType == VIDRNDT_DS_MADVR);

	if (OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p)) {
		if (p->fns.GetCount() > 0) {
			engine_t e = s.GetRtspEngine(p->fns.GetHead());
			if (e != DirectShow /*&& e != RealMedia && e != QuickTime*/) {
				fUseThread = false;
			}
		}
	} else if (OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p)) {
		fUseThread = false;
	}

	SetLoadState(MLS_LOADING);

	if (fUseThread) {
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

void CMainFrame::CloseMedia()
{
	if (m_iMediaLoadState == MLS_CLOSING || m_iMediaLoadState == MLS_CLOSED) {
		return;
	}

	DbgLog((LOG_TRACE, 3, L"CMainFrame::CloseMedia() : start"));

	if (m_iMediaLoadState == MLS_LOADING) {
		m_fOpeningAborted = true;

		if (m_pGB) {
			m_pGB->Abort();    // TODO: lock on graph objects somehow, this is not thread safe
		}

		if (m_pGraphThread) {
			BeginWaitCursor();
			if (WaitForSingleObject(m_hGraphThreadEventOpen, 5000) == WAIT_TIMEOUT) {
				MessageBeep(MB_ICONEXCLAMATION);
				TRACE(_T("CRITICAL ERROR: !!! Must kill opener thread !!!\n"));
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

	//PostMessage(WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(0, 0));

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
				PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
			} else {
				ASSERT(FALSE);
			}
		}
	} else {
		CloseMediaPrivate();
	}

	OnFilePostCloseMedia();

	DbgLog((LOG_TRACE, 3, L"CMainFrame::CloseMedia() : end"));
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
	AppSettings&	s		 = AfxGetAppSettings();
	CDVBChannel*	pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	CString			osd;
	int				i		 = osd.Find(_T("\n"));
	PresentFollowing NowNext;

	if (pChannel != NULL) {
		// Get EIT information:
		CComQIPtr<IBDATuner> pTun = m_pGB;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}
		NowNext.cPresent.Insert(0,_T(" "));
		osd = pChannel->GetName()+ NowNext.cPresent;
		if (i > 0) {
			osd.Delete(i, osd.GetLength()-i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd ,3500);
	}
}

void CMainFrame::DisplayCurrentChannelInfo()
{
	AppSettings&	 s		 = AfxGetAppSettings();
	CDVBChannel*	 pChannel = s.FindChannelByPref(s.nDVBLastChannel);
	CString			 osd;
	PresentFollowing NowNext;

	if (pChannel != NULL) {
		// Get EIT information:
		CComQIPtr<IBDATuner> pTun = m_pGB;
		if (pTun) {
			pTun->UpdatePSI(NowNext);
		}

		osd = NowNext.cPresent + _T(". ") + NowNext.StartTime + _T(" - ") + NowNext.Duration + _T(". ") + NowNext.SummaryDesc +_T(" ");
		int	 i = osd.Find(_T("\n"));
		if (i > 0) {
			osd.Delete(i, osd.GetLength() - i);
		}
		m_OSD.DisplayMessage(OSD_TOPLEFT, osd ,8000, 12);
	}
}

void CMainFrame::SetLoadState(MPC_LOADSTATE iState)
{
	m_iMediaLoadState = iState;
	SendAPICommand(CMD_STATE, L"%d", m_iMediaLoadState);

	switch (m_iMediaLoadState) {
		case MLS_CLOSED:
			DbgLog((LOG_TRACE, 3, L"CMainFrame::SetLoadState() : CLOSED"));
			break;
		case MLS_LOADING:
			DbgLog((LOG_TRACE, 3, L"CMainFrame::SetLoadState() : LOADING"));
			break;
		case MLS_LOADED:
			DbgLog((LOG_TRACE, 3, L"CMainFrame::SetLoadState() : LOADED"));
			break;
		case MLS_CLOSING:
			DbgLog((LOG_TRACE, 3, L"CMainFrame::SetLoadState() : CLOSING"));
			break;
		default:
			break;
	}
}

void CMainFrame::SetPlayState(MPC_PLAYSTATE iState)
{
	m_Lcd.SetPlayState((CMPC_Lcd::PlayState)iState);
	SendAPICommand(CMD_PLAYMODE, L"%d", iState);

	if (m_fEndOfStream) {
		SendAPICommand(CMD_NOTIFYENDOFSTREAM, L"\0");    // do not pass NULL here!
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
	HMONITOR		hMonitor;
	MONITORINFOEX	MonitorInfo;
	CRect			MonitorRect;

	if (m_pFullscreenWnd->IsWindow()) {
		m_pFullscreenWnd->DestroyWindow();
	}

	ZeroMemory (&MonitorInfo, sizeof(MonitorInfo));
	MonitorInfo.cbSize	= sizeof(MonitorInfo);

	CMonitors monitors;
	CString str;
	CMonitor monitor;
	AppSettings& s = AfxGetAppSettings();
	hMonitor = NULL;

	if (!s.iMonitor) {
		if (s.strFullScreenMonitor == _T("Current")) {
			hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		} else {
			for ( int i = 0; i < monitors.GetCount(); i++ ) {
				monitor = monitors.GetMonitor( i );
				monitor.GetName(str);

				if ((monitor.IsMonitor()) && (s.strFullScreenMonitor == str)) {
					hMonitor = monitor;
					break;
				}
			}
			if (!hMonitor) {
				hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			}
		}
	} else {
		hMonitor = MonitorFromWindow (m_hWnd, 0);
	}
	if (GetMonitorInfo (hMonitor, &MonitorInfo)) {
		MonitorRect = CRect (MonitorInfo.rcMonitor);
		// Window creation
		DWORD dwStyle		= WS_POPUP  | WS_VISIBLE ;
		m_fullWndSize.cx	= MonitorRect.Width();
		m_fullWndSize.cy	= MonitorRect.Height();

		m_pFullscreenWnd->CreateEx (WS_EX_TOPMOST | WS_EX_TOOLWINDOW, _T(""), ResStr(IDS_MAINFRM_136), dwStyle, MonitorRect.left, MonitorRect.top, MonitorRect.Width(), MonitorRect.Height(), NULL, NULL, NULL);
		//SetWindowLongPtr(m_pFullscreenWnd->m_hWnd, GWL_EXSTYLE, WS_EX_TOPMOST); // TODO : still freezing sometimes...
		/*
		CRect r;
		GetWindowRect(r);

		int x = MonitorRect.left + (MonitorRect.Width()/2)-(r.Width()/2);
		int y = MonitorRect.top + (MonitorRect.Height()/2)-(r.Height()/2);
		int w = r.Width();
		int h = r.Height();
		MoveWindow(x, y, w, h);
		*/
	}

	return m_pFullscreenWnd->IsWindow();
}

bool CMainFrame::IsD3DFullScreenMode() const
{
	return m_pFullscreenWnd->IsWindow();
};

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
		brightness = min(max(-100, brightness), 100);
	}
	if (flags & ProcAmp_Contrast) {
		contrast = min(max(-100, contrast), 100);
	}
	if (flags & ProcAmp_Hue) {
		hue = min(max(-180, hue), 180);
	}
	if (flags & ProcAmp_Saturation) {
		saturation = min(max(-100, saturation), 100);
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
			return _T("AC3");
		case DVD_AudioFormat_MPEG1:
		case DVD_AudioFormat_MPEG1_DRC:
			return _T("MPEG1");
		case DVD_AudioFormat_MPEG2:
		case DVD_AudioFormat_MPEG2_DRC:
			return _T("MPEG2");
		case DVD_AudioFormat_LPCM:
			return _T("LPCM");
		case DVD_AudioFormat_DTS:
			return _T("DTS");
		case DVD_AudioFormat_SDDS:
			return _T("SDDS");
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
			m_pMS->SetPositions(&m_rtCurSubPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
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
				m_pMS->SetPositions(&m_rtCurSubPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
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
		MessageBox(_T("The Hebrew translation will be correctly displayed (with a right-to-left layout) after restarting the application.\n"),
				   _T("MPC-BE"), MB_ICONINFORMATION | MB_OK);
	}

	CMPlayerCApp::SetLanguage(nID);

	m_openCDsMenu.DestroyMenu();
	m_filtersMenu.DestroyMenu();
	m_subtitlesMenu.DestroyMenu();
	m_audiosMenu.DestroyMenu();
	m_navMixAudioMenu.DestroyMenu();
	m_navMixSubtitleMenu.DestroyMenu();
	m_videoStreamsMenu.DestroyMenu();
	m_chaptersMenu.DestroyMenu();
	m_favoritesMenu.DestroyMenu();
	m_shadersMenu.DestroyMenu();
	m_recentfilesMenu.DestroyMenu();
	m_languageMenu.DestroyMenu();

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

	AfxGetAppSettings().SaveSettings();
}

void CMainFrame::ProcessAPICommand(COPYDATASTRUCT* pCDS)
{
	CAtlList<CString>	fns;
	REFERENCE_TIME		rtPos	= 0;

	switch (pCDS->dwData) {
		case CMD_OPENFILE :
			fns.AddHead ((LPCWSTR)pCDS->lpData);
			m_wndPlaylistBar.Open(fns, false);
			OpenCurPlaylistItem();
			break;
		case CMD_STOP :
			OnPlayStop();
			break;
		case CMD_CLOSEFILE :
			CloseMedia();
			break;
		case CMD_PLAYPAUSE :
			OnPlayPlaypause();
			break;
		case CMD_ADDTOPLAYLIST :
			fns.AddHead ((LPCWSTR)pCDS->lpData);
			m_wndPlaylistBar.Append(fns, true);
			break;
		case CMD_STARTPLAYLIST :
			OpenCurPlaylistItem();
			break;
		case CMD_CLEARPLAYLIST :
			m_wndPlaylistBar.Empty();
			break;
		case CMD_SETPOSITION :
			rtPos = 10000 * REFERENCE_TIME(_wtof((LPCWSTR)pCDS->lpData)*1000); //with accuracy of 1 ms
			// imianz: quick and dirty trick
			// Pause->SeekTo->Play (in place of SeekTo only) seems to prevents in most cases
			// some strange video effects on avi files (ex. locks a while and than running fast).
			if (!m_bAudioOnly && GetMediaState()==State_Running) {
				SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
				SeekTo(rtPos);
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);
			} else {
				SeekTo(rtPos);
			}
			// show current position overridden by play command
			m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 2000);
			break;
		case CMD_SETAUDIODELAY :
			rtPos			= _wtol ((LPCWSTR)pCDS->lpData) * 10000;
			SetAudioDelay (rtPos);
			break;
		case CMD_SETSUBTITLEDELAY :
			SetSubtitleDelay(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_SETINDEXPLAYLIST :
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
		case CMD_GETCURRENTPOSITION :
			SendCurrentPositionToApi();
			break;
		case CMD_GETNOWPLAYING:
			SendNowPlayingToApi();
			break;
		case CMD_JUMPOFNSECONDS :
			JumpOfNSeconds(_wtoi((LPCWSTR)pCDS->lpData));
			break;
		case CMD_GETPLAYLIST :
			SendPlaylistToApi();
			break;
		case CMD_JUMPFORWARDMED :
			OnPlaySeek(ID_PLAY_SEEKFORWARDMED);
			break;
		case CMD_JUMPBACKWARDMED :
			OnPlaySeek(ID_PLAY_SEEKBACKWARDMED);
			break;
		case CMD_TOGGLEFULLSCREEN :
			OnViewFullscreen();
			break;
		case CMD_INCREASEVOLUME :
			m_wndToolBar.m_volctrl.IncreaseVolume();
			break;
		case CMD_DECREASEVOLUME :
			m_wndToolBar.m_volctrl.DecreaseVolume();
			break;
		case CMD_SHADER_TOGGLE :
			OnShaderToggle();
			break;
		case CMD_CLOSEAPP :
			PostMessage(WM_CLOSE);
			break;
		case CMD_OSDSHOWMESSAGE:
			ShowOSDCustomMessageApi((MPC_OSDDATA *)pCDS->lpData);
			break;
	}
}

void CMainFrame::SendAPICommand(MPCAPI_COMMAND nCommand, LPCWSTR fmt, ...)
{
	AppSettings&	s = AfxGetAppSettings();

	if (s.hMasterWnd) {
		COPYDATASTRUCT	CDS;
		TCHAR			buff[800];

		va_list args;
		va_start(args, fmt);
		_vstprintf_s(buff, _countof(buff), fmt, args);

		CDS.cbData = (_tcslen (buff) + 1) * sizeof(TCHAR);
		CDS.dwData = nCommand;
		CDS.lpData = (LPVOID)buff;

		::SendMessage(s.hMasterWnd, WM_COPYDATA, (WPARAM)GetSafeHwnd(), (LPARAM)&CDS);

		va_end(args);
	}
}

void CMainFrame::SendNowPlayingToApi()
{
	if (!AfxGetAppSettings().hMasterWnd) {
		return;
	}

	if (m_iMediaLoadState == MLS_LOADED) {
		CPlaylistItem	pli;
		CString			title, author, description;
		CString			label;
		long			lDuration = 0;
		REFERENCE_TIME	rtDur;

		if (GetPlaybackMode() == PM_FILE) {
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_TITLE), title);
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_AUTHOR), author);
			m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_DESCRIPTION), description);

			m_wndPlaylistBar.GetCur(pli);
			if (!pli.m_fns.IsEmpty()) {
				label = !pli.m_label.IsEmpty() ? pli.m_label : pli.m_fns.GetHead().GetName();

				m_pMS->GetDuration(&rtDur);
				DVD_HMSF_TIMECODE tcDur = RT2HMSF(rtDur);
				lDuration = tcDur.bHours*60*60 + tcDur.bMinutes*60 + tcDur.bSeconds;
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
						title = _T("DVD - Stopped");
						break;
					case DVD_DOMAIN_FirstPlay:
						title = _T("DVD - FirstPlay");
						break;
					case DVD_DOMAIN_VideoManagerMenu:
						title = _T("DVD - RootMenu");
						break;
					case DVD_DOMAIN_VideoTitleSetMenu:
						title = _T("DVD - TitleMenu");
						break;
					case DVD_DOMAIN_Title:
						title = _T("DVD - Title");
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
						lDuration = tcDur.bHours*60*60 + tcDur.bMinutes*60 + tcDur.bSeconds;
					}

					// build string
					// DVD - xxxxx|currenttitle|numberofchapters|currentchapter|titleduration
					author.Format(L"%d", Location.TitleNum);
					description.Format(L"%d", ulNumOfChapters);
					label.Format(L"%d", Location.ChapterNum);
				}
			}
		}

		title.Replace(L"|", L"\\|");
		author.Replace(L"|", L"\\|");
		description.Replace(L"|", L"\\|");
		label.Replace(L"|", L"\\|");

		CStringW buff;
		buff.Format(L"%s|%s|%s|%s|%d", title, author, description, label, lDuration);

		SendAPICommand(CMD_NOWPLAYING, buff);
		SendSubtitleTracksToApi();
		SendAudioTracksToApi();
	}
}

void CMainFrame::SendSubtitleTracksToApi()
{
	CStringW	strSubs;
	POSITION	pos = m_pSubStreams.GetHeadPosition();

	if (m_iMediaLoadState == MLS_LOADED) {
		if (pos) {
			while (pos) {
				CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

				for (int i = 0, j = pSubStream->GetStreamCount(); i < j; i++) {
					WCHAR* pName = NULL;
					if (SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL))) {
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
	SendAPICommand(CMD_LISTSUBTITLETRACKS, strSubs);
}

void CMainFrame::SendAudioTracksToApi()
{
	CStringW	strAudios;

	if (m_iMediaLoadState == MLS_LOADED) {
		CComQIPtr<IAMStreamSelect> pSS = m_pSwitcherFilter;

		DWORD cStreams = 0;
		if (pSS && SUCCEEDED(pSS->Count(&cStreams))) {
			int currentStream = -1;
			for (int i = 0; i < (int)cStreams; i++) {
				DWORD dwFlags	= DWORD_MAX;
				WCHAR* pszName	= NULL;
				if (FAILED(pSS->Info(i, NULL, &dwFlags, NULL, NULL, &pszName, NULL, NULL))) {
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

				if (pszName) {
					CoTaskMemFree(pszName);
				}
			}
			strAudios.AppendFormat(L"|%i", currentStream);

		} else {
			strAudios.Append(L"-1");
		}
	} else {
		strAudios.Append(L"-2");
	}
	SendAPICommand(CMD_LISTAUDIOTRACKS, strAudios);
}

void CMainFrame::SendPlaylistToApi()
{
	CStringW		strPlaylist;
	int index;

	POSITION pos = m_wndPlaylistBar.m_pl.GetHeadPosition(), pos2;
	while (pos) {
		CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);

		if (pli.m_type == CPlaylistItem::file) {
			pos2 = pli.m_fns.GetHeadPosition();
			while (pos2) {
				CString fn = pli.m_fns.GetNext(pos2);
				if (!strPlaylist.IsEmpty()) {
					strPlaylist.Append (L"|");
				}
				fn.Replace(L"|", L"\\|");
				strPlaylist.AppendFormat(L"%s", fn);
			}
		}
	}
	index = m_wndPlaylistBar.GetSelIdx();
	if (strPlaylist.IsEmpty()) {
		strPlaylist.Append(L"-1");
	} else {
		strPlaylist.AppendFormat(L"|%i", index);
	}
	SendAPICommand(CMD_PLAYLIST, strPlaylist);
}

void CMainFrame::SendCurrentPositionToApi(bool fNotifySeek)
{
	if (!AfxGetAppSettings().hMasterWnd) {
		return;
	}

	if (m_iMediaLoadState == MLS_LOADED) {
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
	if (m_iMediaLoadState == MLS_LOADED) {
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
				SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
			}
			SeekTo(rtCur);
			if (!m_bAudioOnly) {
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);
				// show current position overridden by play command
				m_OSD.DisplayMessage(OSD_TOPLEFT, m_wndStatusBar.GetStatusTimer(), 2000);
			}
		}
	}
}

void CMainFrame::OnFileOpenDirectory()
{
	if (m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();
	CString strTitle = ResStr(IDS_MAINFRM_DIR_TITLE);
	CString path;

	if (IsWinVistaOrLater()) {
		CFileDialog dlg(TRUE);
		dlg.AddCheckButton(IDS_MAINFRM_DIR_CHECK, ResStr(IDS_MAINFRM_DIR_CHECK), TRUE);
		IFileOpenDialog *openDlgPtr = dlg.GetIFileOpenDialog();

		if (openDlgPtr != NULL) {
			CComPtr<IShellItem> psiFolder;
			if (SUCCEEDED(afxGlobalData.ShellCreateItemFromParsingName(s.strLastOpenDir, NULL, IID_PPV_ARGS(&psiFolder)))) {
				openDlgPtr->SetFolder(psiFolder);
			}

			openDlgPtr->SetTitle(strTitle);
			openDlgPtr->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			if (FAILED(openDlgPtr->Show(m_hWnd))) {
				openDlgPtr->Release();
				return;
			}

			if (SUCCEEDED(openDlgPtr->GetResult(&psiFolder))) {
				LPWSTR folderpath = NULL;
				if(SUCCEEDED(psiFolder->GetDisplayName(SIGDN_FILESYSPATH, &folderpath))) {
					path = folderpath;
					CoTaskMemFree(folderpath);
				}
			}

			openDlgPtr->Release();

			BOOL recur = TRUE;
			dlg.GetCheckButtonState(IDS_MAINFRM_DIR_CHECK, recur);
			COpenDirHelper::m_incl_subdir = !!recur;
		} else {
			return;
		}
	} else {
		CString filter;
		CAtlArray<CString> mask;
		s.m_Formats.GetFilter(filter, mask);

		COpenDirHelper::strLastOpenDir = s.strLastOpenDir;

		TCHAR _path[_MAX_PATH];
		COpenDirHelper::m_incl_subdir = TRUE;

		BROWSEINFO bi;
		bi.hwndOwner      = m_hWnd;
		bi.pidlRoot       = NULL;
		bi.pszDisplayName = _path;
		bi.lpszTitle      = strTitle;
		bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_STATUSTEXT;
		bi.lpfn           = COpenDirHelper::BrowseCallbackProcDIR;
		bi.lParam         = 0;
		bi.iImage         = 0;

		static PCIDLIST_ABSOLUTE iil;
		iil = SHBrowseForFolder(&bi);
		if (iil) {
			SHGetPathFromIDList(iil, _path);
		} else {
			return;
		}
		path = _path;
	}

	path = AddSlash(path);
	s.strLastOpenDir = path;

	CAtlList<CString> sl;
	sl.AddTail(path);
	if (COpenDirHelper::m_incl_subdir) {
		COpenDirHelper::RecurseAddDir(path, &sl);
	}

	if (m_wndPlaylistBar.IsWindowVisible()) {
		m_wndPlaylistBar.Append(sl, true);
	} else {
		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
}

HRESULT CMainFrame::CreateThumbnailToolbar()
{
	if (!IsWinSevenOrLater() || !AfxGetAppSettings().fUseWin7TaskBar) {
		return E_FAIL;
	}

	if (m_pTaskbarList) {
		m_pTaskbarList->Release();
	}

	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbarList));
	if (SUCCEEDED(hr)) {
		CMPCPngImage mpc_png;
		BYTE* pData;
		int width, height, bpp;

		HBITMAP hB = mpc_png.TypeLoadImage(IMG_TYPE::PNG, &pData, &width, &height, &bpp, NULL, IDB_W7_TOOLBAR, 0, 0, 0, 0);
		if (!hB) {
			m_pTaskbarList->Release();
			return E_FAIL;
		}

		// Check dimensions
		BITMAP bi = {0};
		GetObject((HANDLE)hB, sizeof(bi), &bi);
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
			StringCchCopy(buttons[0].szTip, _countof(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

			// STOP
			buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[1].iId = IDTB_BUTTON1;
			buttons[1].iBitmap = 1;
			StringCchCopy(buttons[1].szTip, _countof(buttons[1].szTip), ResStr(IDS_AG_STOP));

			// PLAY/PAUSE
			buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[2].iId = IDTB_BUTTON2;
			buttons[2].iBitmap = 3;
			StringCchCopy(buttons[2].szTip, _countof(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

			// NEXT
			buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[3].dwFlags = THBF_DISABLED;
			buttons[3].iId = IDTB_BUTTON4;
			buttons[3].iBitmap = 4;
			StringCchCopy(buttons[3].szTip, _countof(buttons[3].szTip), ResStr(IDS_AG_NEXT));

			// FULLSCREEN
			buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
			buttons[4].dwFlags = THBF_DISABLED;
			buttons[4].iId = IDTB_BUTTON5;
			buttons[4].iBitmap = 5;
			StringCchCopy(buttons[4].szTip, _countof(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

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
		m_pTaskbarList->SetOverlayIcon(m_hWnd, NULL, L"");
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
	StringCchCopy(buttons[0].szTip, _countof(buttons[0].szTip), ResStr(IDS_AG_PREVIOUS));

	buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[1].iId = IDTB_BUTTON1;
	buttons[1].iBitmap = 1;
	StringCchCopy(buttons[1].szTip, _countof(buttons[1].szTip), ResStr(IDS_AG_STOP));

	buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[2].iId = IDTB_BUTTON2;
	buttons[2].iBitmap = 3;
	StringCchCopy(buttons[2].szTip, _countof(buttons[2].szTip), ResStr(IDS_AG_PLAYPAUSE));

	buttons[3].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[3].dwFlags = (AfxGetAppSettings().fDontUseSearchInFolder && m_wndPlaylistBar.GetCount() <= 1 && (m_pCB && m_pCB->ChapGetCount() <= 1)) ? THBF_DISABLED : THBF_ENABLED;
	buttons[3].iId = IDTB_BUTTON4;
	buttons[3].iBitmap = 4;
	StringCchCopy(buttons[3].szTip, _countof(buttons[3].szTip), ResStr(IDS_AG_NEXT));

	buttons[4].dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
	buttons[4].dwFlags = THBF_ENABLED;
	buttons[4].iId = IDTB_BUTTON5;
	buttons[4].iBitmap = 5;
	StringCchCopy(buttons[4].szTip, _countof(buttons[4].szTip), ResStr(IDS_AG_FULLSCREEN));

	HICON hIcon = NULL;

	if (m_iMediaLoadState == MLS_LOADED) {
		OAFilterState fs = GetMediaState();
		if (fs == State_Running) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 2;

			hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_TB_PLAY), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, m_wndSeekBar.HasDuration() ? TBPF_NORMAL : TBPF_NOPROGRESS);
		} else if (fs == State_Stopped) {
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_TB_STOP), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
		} else if (fs == State_Paused) {
			buttons[1].dwFlags = THBF_ENABLED;
			buttons[2].dwFlags = THBF_ENABLED;
			buttons[2].iBitmap = 3;

			hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_TB_PAUSE), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		}

		if (m_bAudioOnly) {
			buttons[4].dwFlags = THBF_DISABLED;
		}

		if (GetPlaybackMode() == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) {
			buttons[0].dwFlags = THBF_DISABLED;
			buttons[1].dwFlags = THBF_DISABLED;
			buttons[2].dwFlags = THBF_DISABLED;
			buttons[3].dwFlags = THBF_DISABLED;
		}

		m_pTaskbarList->SetOverlayIcon(m_hWnd, hIcon, L"");

		if (hIcon != NULL) {
			DestroyIcon( hIcon );
		}
	} else {
		buttons[0].dwFlags = THBF_DISABLED;
		buttons[1].dwFlags = THBF_DISABLED;
		buttons[2].dwFlags = m_wndPlaylistBar.GetCount() > 0 ? THBF_ENABLED : THBF_DISABLED;
		buttons[3].dwFlags = THBF_DISABLED;
		buttons[4].dwFlags = THBF_DISABLED;

		m_pTaskbarList->SetOverlayIcon(m_hWnd, NULL, L"");
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}

	HRESULT hr = m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, ARRAYSIZE(buttons), buttons);

	UpdateThumbnailClip();

	return hr;
}

HRESULT CMainFrame::UpdateThumbnailClip()
{
	CheckPointer(m_pTaskbarList, E_FAIL);

	const CAppSettings& s = AfxGetAppSettings();

	CRect r;
	m_wndView.GetClientRect(&r);
	if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
		r.OffsetRect(0, GetSystemMetrics(SM_CYMENU));
	}

	if (!s.fUseWin7TaskBar || m_iMediaLoadState != MLS_LOADED || m_bAudioOnly || m_fFullScreen || IsD3DFullScreenMode() || r.Width() <= 0 || r.Height() <= 0) {
		return m_pTaskbarList->SetThumbnailClip(m_hWnd, NULL);
	}

	return m_pTaskbarList->SetThumbnailClip(m_hWnd, &r);
}

LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_COMMAND) && (THBN_CLICKED == HIWORD(wParam))) {
		int const wmId = LOWORD(wParam);
		switch (wmId) {
			case IDTB_BUTTON1:
				SendMessage(WM_COMMAND, ID_PLAY_STOP);
				break;

			case IDTB_BUTTON2: {
					OAFilterState fs = GetMediaState();
					if (fs != -1) {
						SendMessage(WM_COMMAND, ID_PLAY_PLAYPAUSE);
					} else {
						SendMessage(WM_COMMAND, ID_PLAY_PLAY);
					}
				}
				break;

			case IDTB_BUTTON3:
				SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
				break;

			case IDTB_BUTTON4:
				SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
				break;

			case IDTB_BUTTON5:
				WINDOWPLACEMENT wp;
				GetWindowPlacement(&wp);
				if (wp.showCmd == SW_SHOWMINIMIZED) {
					SendMessage(WM_SYSCOMMAND, SC_RESTORE, -1);
				}
				SetForegroundWindow();

				SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
				break;

			default:
				break;
		}
		return 0;
	}

	return __super::WindowProc(message, wParam, lParam);
}

#if (_MSC_VER < 1800)
UINT CMainFrame::OnPowerBroadcast(UINT nPowerEvent, UINT nEventData)
#else
UINT CMainFrame::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
#endif
{
	static BOOL bWasPausedBeforeSuspention;
	OAFilterState mediaState;

	switch (nPowerEvent) {
		case PBT_APMSUSPEND: // System is suspending operation.
			TRACE("OnPowerBroadcast() - suspending\n"); // For user tracking

			bWasPausedBeforeSuspention = FALSE; // Reset value

			mediaState = GetMediaState();
			if (mediaState == State_Running) {
				bWasPausedBeforeSuspention = TRUE;
				SendMessage(WM_COMMAND, ID_PLAY_PAUSE); // Pause
			}
			break;
		case PBT_APMRESUMEAUTOMATIC: // Operation is resuming automatically from a low-power state. This message is sent every time the system resumes.
			TRACE("OnPowerBroadcast() - resuming\n"); // For user tracking

			// Resume if we paused before suspension.
			if (bWasPausedBeforeSuspention) {
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);    // Resume
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
			TRACE(_T("OnSessionChange() - Lock session\n"));

			bWasPausedBeforeSessionChange = FALSE;
			if (GetMediaState() == State_Running && !m_bAudioOnly) {
				bWasPausedBeforeSessionChange = TRUE;
				SendMessage( WM_COMMAND, ID_PLAY_PAUSE );
			}
			break;
		case WTS_SESSION_UNLOCK:
			TRACE(_T("OnSessionChange() - UnLock session\n"));

			if (bWasPausedBeforeSessionChange) {
				SendMessage( WM_COMMAND, ID_PLAY_PLAY );
			}
			break;
	}
}

#define NOTIFY_FOR_THIS_SESSION 0
void CMainFrame::WTSRegisterSessionNotification()
{
	typedef BOOL (WINAPI *WTSREGISTERSESSIONNOTIFICATION)(HWND, DWORD);

	m_hWtsLib = LoadLibrary(L"wtsapi32.dll");

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
	if (enable && !m_shaderlabels.IsEmpty()) {
		m_bToggleShader = true;
		SetShaders();
	} else {
		m_bToggleShader = false;
		if (m_pCAP) {
			m_pCAP->SetPixelShader(NULL, NULL);
		}
	}
}

void CMainFrame::EnableShaders2(bool enable)
{
	if (enable && !m_shaderlabelsScreenSpace.IsEmpty()) {
		m_bToggleShaderScreenSpace = true;
		SetShaders();
	} else {
		m_bToggleShaderScreenSpace = false;
		if (m_pCAP2) {
			m_pCAP2->SetPixelShader2(NULL, NULL, true);
		}
	}
}

BOOL CMainFrame::OpenBD(CString path, REFERENCE_TIME rtStart/* = INVALID_TIME*/, BOOL bAddRecent/* = TRUE*/)
{
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();

	path.TrimRight('\\');
	if (path.Right(5).MakeLower() == L".bdmv") {
		path.Truncate(path.ReverseFind('\\'));
	} else if (::PathIsDirectory(path + L"\\BDMV")) {
		path += L"\\BDMV";
	}

	if (::PathIsDirectory(path + L"\\PLAYLIST") && ::PathIsDirectory(path + L"\\STREAM")) {
		CHdmvClipInfo	ClipInfo;
		CString			strPlaylistFile;
		CHdmvClipInfo::CPlaylist MainPlaylist;

		if (SUCCEEDED(ClipInfo.FindMainMovie(path, strPlaylistFile, MainPlaylist, m_MPLSPlaylist))) {
			if (path.Right(5).MakeUpper() == L"\\BDMV") {
				path.Truncate(path.GetLength() - 5);
				CString infFile = path + L"\\disc.inf";
				if (::PathFileExists(infFile)) {
					CTextFile cf;
					if (cf.Open(infFile)) {
						CString line;
						while (cf.ReadString(line)) {
							CAtlList<CString> sl;
							Explode(line, sl, '=');
							if (sl.GetCount() == 2 && CString(sl.GetHead().Trim()).MakeLower() == L"label") {
								m_BDLabel = sl.GetTail();
								break;
							}
						}
					}
				}
			}

			if (bAddRecent) {
				AddRecent(path);
			}

			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_bIsBDPlay = TRUE;
			m_LastOpenBDPath = path;

			CAtlList<CString> sl;
			sl.AddTail(strPlaylistFile);
			m_wndPlaylistBar.Open(sl, false);
			if (OpenCurPlaylistItem(rtStart, FALSE)) {
				return TRUE;
			}
		}
	}

	m_bIsBDPlay = FALSE;
	m_BDLabel.Empty();
	m_LastOpenBDPath.Empty();
	return FALSE;
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
		if (SUCCEEDED(pCGB->FindPin(m_wndCaptureBar.m_capdlg.m_pDst, PINDIR_INPUT, NULL, NULL, FALSE, 0, &pPin))) {
			LONGLONG size = 0;
			if (CComQIPtr<IStream> pStream = pPin) {
				pStream->Commit(STGC_DEFAULT);

				WIN32_FIND_DATA findFileData;
				HANDLE h = FindFirstFile(m_wndCaptureBar.m_capdlg.m_file, &findFileData);
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

					__int64 start = 0, stop = 0;
					m_wndSeekBar.GetRange(start, stop);
					m_fLiveWM = (stop == start);
				}
				break;
			}
		}
		EndEnumFilters;
	} else if (m_pAMOP || SUCCEEDED(QueryProgressYoutube(NULL, NULL))) {
		__int64 t = 0, c = 0;
		if (SUCCEEDED(m_pMS->GetDuration(&t)) && t > 0 && SUCCEEDED(m_pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t) {
			msg.Format(ResStr(IDS_CONTROLS_BUFFERING), c * 100 / t);
		}

		if (SUCCEEDED(QueryProgressYoutube(&t, &c))) {
			msg.Format(ResStr(IDS_CONTROLS_BUFFERING), c * 100 / t);
		}

		if (m_fUpdateInfoBar) {
			OpenSetupInfoBar();
			OpenSetupWindowTitle(m_strFnFull);
		}
	}

	return msg;
}

bool CMainFrame::CanPreviewUse()
{
	return ((m_bUseSmartSeek
			&& m_iMediaLoadState == MLS_LOADED)
			&& (GetPlaybackMode() == PM_DVD || GetPlaybackMode() == PM_FILE)
			&& !m_bAudioOnly
			&& AfxGetAppSettings().fSmartSeek
			&& m_wndPreView) ? 1 : 0;
}

bool CheckCoverImgExist(CString &path, CString name) {
	CPath coverpath;
	coverpath.Combine(path, name);

	if (coverpath.FileExists() ||
		(coverpath.RenameExtension(_T(".jpeg")) && coverpath.FileExists()) ||
		(coverpath.RenameExtension(_T(".png"))  && coverpath.FileExists())) {
			path.SetString(coverpath);
			return true;
	}
	return false;
}

CString GetCoverImgFromPath(CString fullfilename)
{
	CString path = fullfilename.Left(fullfilename.ReverseFind('\\') + 1);


	if (CheckCoverImgExist(path, _T("cover.jpg"))) {
		return path;
	}

	if (CheckCoverImgExist(path, _T("folder.jpg"))) {
		return path;
	}

	CPath dir(path);
	dir.RemoveBackslash();
	int k = dir.FindFileName();
	if (k >= 0) {
		if (CheckCoverImgExist(path, CString(dir).Right(k) + _T(".jpg"))) {
			return path;
		}
	}

	if (CheckCoverImgExist(path, _T("front.jpg"))) {
		return path;
	}

	CString fname = fullfilename.Mid(path.GetLength());
	fname = fname.Left(fname.ReverseFind('.'));
	if (CheckCoverImgExist(path, fname + _T(".jpg"))) {
		return path;
	}

	if (CheckCoverImgExist(path, _T("cover\\front.jpg"))) {
		return path;
	}

	if (CheckCoverImgExist(path, _T("covers\\front.jpg"))) {
		return path;
	}

	if (CheckCoverImgExist(path, _T("box.jpg"))) {
		return path;
	}

	return _T("");
}

/* this is for custom draw in windows 7 preview */
HRESULT CMainFrame::SetDwmPreview(BOOL show)
{
	BOOL set = AfxGetAppSettings().fUseWin7TaskBar && m_bAudioOnly && IsSomethingLoaded();
	if (!show) { // forcing off custom preview bitmap ...
		set = show;
	}

	if (m_ThumbCashedBitmap) {
		::DeleteObject(m_ThumbCashedBitmap);
		m_ThumbCashedBitmap = NULL;
	}
	m_ThumbCashedSize.SetSize(0, 0);

	if (IsWinSevenOrLater() && m_DwmSetWindowAttributeFnc && m_DwmSetIconicThumbnailFnc) {
		m_DwmSetWindowAttributeFnc(GetSafeHwnd(), DWMWA_HAS_ICONIC_BITMAP, &set, sizeof(set));
		m_DwmSetWindowAttributeFnc(GetSafeHwnd(), DWMWA_FORCE_ICONIC_REPRESENTATION, &set, sizeof(set));
	}

	bool bLoadRes		= false;
	m_bInternalImageRes	= false;
	m_InternalImage.Destroy();
	if (m_InternalImageSmall) {
		m_InternalImageSmall.Detach();
	}

	bool extimage = false;

	if (show && OpenImageCheck(m_strFnFull)) {
		m_InternalImage.Attach(OpenImage(m_strFnFull));
		bLoadRes = true;
		extimage = true;
	}

	if (extimage || (m_bAudioOnly && IsSomethingLoaded() && show)) {

		// load image from DSMResource to show in preview & logo;
		BeginEnumFilters(m_pGB, pEF, pBF) {
			if (CComQIPtr<IDSMResourceBag> pRB = pBF)
				if (pRB && CheckMainFilter(pBF) && pRB->ResGetCount() > 0) {
					for (DWORD i = 0; i < pRB->ResGetCount(); i++) {
						CComBSTR name, desc, mime;
						BYTE* pData = NULL;
						DWORD len = 0;
						if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, NULL))) {
							if (CString(mime).Trim() == _T("image/jpeg")
								|| CString(mime).Trim() == _T("image/jpg")
								|| CString(mime).Trim() == _T("image/png")) {

								HGLOBAL hBlock = ::GlobalAlloc(GMEM_MOVEABLE, len);
								if (hBlock != NULL) {
									IStream* pStream = NULL;
									LPVOID lpResBuffer = ::GlobalLock(hBlock);
									ASSERT (lpResBuffer != NULL);
									memcpy(lpResBuffer, pData, len);

									if (SUCCEEDED(::CreateStreamOnHGlobal(hBlock, TRUE, &pStream))) {
										m_InternalImage.Load(pStream);
										pStream->Release();
										bLoadRes = true;
									}

									::GlobalUnlock(hBlock);
									::GlobalFree(hBlock);
								}

								break;
							}
							CoTaskMemFree(pData);
						}
					}
				}
		}
		EndEnumFilters;

		if (!bLoadRes) {
			// try to load image from file in the same dir that media file to show in preview & logo;
			CString img_fname = GetCoverImgFromPath(m_strFnFull);

			if (!img_fname.IsEmpty()) {
				if(SUCCEEDED(m_InternalImage.Load(img_fname))) {
					bLoadRes = true;
				}
			}
		}

		if (!bLoadRes) {
			m_InternalImage.LoadFromResource(IDB_W7_AUDIO);
			m_bInternalImageRes = true;
			m_InternalImageSmall.Attach(m_InternalImage);
		} else {
			BITMAP bm;
			if (GetObject(m_InternalImage, sizeof(bm), &bm) && bm.bmWidth) {
				const int nWidth = 256;
				if ((abs(bm.bmHeight) <= nWidth) && (bm.bmWidth <= nWidth)) {
					m_InternalImageSmall.Attach(m_InternalImage);
				} else {
					// Resize image to improve speed of show TaskBar preview

					int h	= min(abs(bm.bmHeight), nWidth);
					int w	= MulDiv(h, bm.bmWidth, abs(bm.bmHeight));
					h		= MulDiv(w, abs(bm.bmHeight), bm.bmWidth);

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

	int nWidth	= HIWORD(lParam);
	int nHeight	= LOWORD(lParam);

	if (m_ThumbCashedBitmap && m_ThumbCashedSize != CSize(nWidth, nHeight)) {
		::DeleteObject(m_ThumbCashedBitmap);
		m_ThumbCashedBitmap = NULL;
	}

	if (!m_ThumbCashedBitmap) {
		HDC hdcMem = CreateCompatibleDC(NULL);

		if (hdcMem) {
			BITMAP bm;
			GetObject(m_InternalImageSmall, sizeof(bm), &bm);

			int h	= min(abs(bm.bmHeight), nHeight);
			int w	= MulDiv(h, bm.bmWidth, abs(bm.bmHeight));
			h		= MulDiv(w, abs(bm.bmHeight), bm.bmWidth);

			BITMAPINFO bmi;
			ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
			bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth		= m_bInternalImageRes ? nWidth : w;
			bmi.bmiHeader.biHeight		= -nHeight;
			bmi.bmiHeader.biPlanes		= 1;
			bmi.bmiHeader.biBitCount	= 32;

			PBYTE pbDS = NULL;
			m_ThumbCashedBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, NULL, NULL);
			if (m_ThumbCashedBitmap) {
				SelectObject(hdcMem, m_ThumbCashedBitmap);

				int x = m_bInternalImageRes ? ((nWidth - w) / 2) : 0;
				int y = (nHeight - h) / 2;
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

	AppSettings& s = AfxGetAppSettings();

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
		hr = m_DwmSetIconicLivePreviewBitmapFnc(m_hWnd, m_CaptureWndBitmap, NULL, (style & WS_CAPTION || style & WS_THICKFRAME) ? DWM_SIT_DISPLAYFRAME : 0);
	} else {
		HBITMAP hBitmap = CreateCaptureDIB(rectClient.Width(), rectClient.Height());
		if (hBitmap) {
			if (style & WS_CAPTION || style & WS_THICKFRAME) {
				hr = m_DwmSetIconicLivePreviewBitmapFnc(m_hWnd, hBitmap, NULL, DWM_SIT_DISPLAYFRAME);
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
	AppSettings& s = AfxGetAppSettings();

	HBITMAP hbm = NULL;
	CWindowDC hDCw(this), hDCc(this);
	HDC hdcMem = CreateCompatibleDC(hDCc);

	bool bCaptionWithMenu = (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) ? true : false;
	DWORD style = GetStyle();

	if (hdcMem != NULL) {

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

		PBYTE pbDS = NULL;
		hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (VOID**)&pbDS, NULL, NULL);
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
	AppSettings& s = AfxGetAppSettings();
	CRect rectClient(0, 0, 0, 0);

	if (s.iCaptionMenuMode == MODE_SHOWCAPTIONMENU) {
		GetWindowRect(&rectClient);
	} else {
		GetClientRect(&rectClient);
	}

	m_CaptureWndBitmap = CreateCaptureDIB(rectClient.Width(), rectClient.Height());
}

CString CMainFrame::GetStrForTitle()
{
	AppSettings& s = AfxGetAppSettings();

	if (s.iTitleBarTextStyle == 1) {
		if (s.fTitleBarTextTitle) {
			if (!m_youtubeFields.title.IsEmpty()) {
				return m_youtubeFields.title;
			} else if (m_bIsBDPlay || GetPlaybackMode() == PM_DVD) {
				return m_strCurPlaybackLabel;
			} else {
				BeginEnumFilters(m_pGB, pEF, pBF) {
					if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
						if (!CheckMainFilter(pBF)) {
							continue;
						}

						CComBSTR bstr;
						if (SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length()) {
							CString title(bstr.m_str);
							bstr.Empty();
							return title;
						}
					}
				}
				EndEnumFilters;
			}

			if (m_iMediaLoadState == MLS_LOADED) {
				CPlaylistItem pli;
				if (m_wndPlaylistBar.GetCur(pli)) {
					return pli.GetLabel();
				}
			}
		}
		return m_strCurPlaybackLabel;
	} else {
		return m_strFnFull;
	}
}

CString CMainFrame::GetAltFileName()
{
	CString ret;
	if (!m_youtubeFields.fname.IsEmpty()) {
		ret = m_youtubeFields.fname;
		FixFilename(ret);
	}

	return ret;
}

void CMainFrame::SetupNotifyRenderThread(CAtlArray<HANDLE>& handles)
{
	for (size_t i = 2; i < handles.GetCount(); i++) {
		FindCloseChangeNotification(handles[i]);
	}
	handles.RemoveAll();
	handles.Add(m_hStopNotifyRenderThreadEvent);
	handles.Add(m_hRefreshNotifyRenderThreadEvent);
	handles.Append(m_ExtSubPathsHandles);
}

DWORD CMainFrame::NotifyRenderThread()
{
	CAtlArray<HANDLE> handles;
	SetupNotifyRenderThread(handles);

	while (true) {
		DWORD idx = WaitForMultipleObjects(handles.GetCount(), handles.GetData(), FALSE, INFINITE);
		if (idx == WAIT_OBJECT_0) { // m_hStopNotifyRenderThreadEvent
			break;
		} else if (idx == (WAIT_OBJECT_0 + 1)) { // m_hRefreshNotifyRenderThreadEvent
			SetupNotifyRenderThread(handles);
		} else if (idx > (WAIT_OBJECT_0 + 1) && idx < (WAIT_OBJECT_0 + handles.GetCount())) {
			if (FindNextChangeNotification(handles[idx - WAIT_OBJECT_0]) == FALSE) {
				break;
			}

			if (AfxGetAppSettings().fAutoReloadExtSubtitles) {

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
			}
		} else {
			DbgLog((LOG_TRACE, 3, L"CMainFrame::NotifyRenderThread() : %s", GetLastErrorMsg(L"WaitForMultipleObjects")));
			ASSERT(FALSE);
			break;
		}
	}

	for (size_t i = 2; i < handles.GetCount(); i++) {
		FindCloseChangeNotification(handles[i]);
	}

	return 0;
}

int CMainFrame::GetStreamCount(DWORD dwSelGroup)
{
	int streamcount = 0;
	if (CComQIPtr<IAMStreamSelect> pSS = m_pMainSourceFilter) {
		DWORD cStreams;
		if (!FAILED(pSS->Count(&cStreams))) {
			for (int i = 0, j = cStreams; i < j; i++) {
				DWORD dwGroup = DWORD_MAX;
				if (FAILED(pSS->Info(i, NULL, NULL, NULL, &dwGroup, NULL, NULL, NULL))) {
					continue;
				}

				if (dwGroup != dwSelGroup) {
					continue;
				}

				streamcount++;
			}
		}
	}

	return streamcount;
}

void CMainFrame::AddSubtitlePathsAddons(CString FileName)
{
	CString tmp(AddSlash(GetFolderOnly(FileName)).MakeUpper());
	AppSettings& s = AfxGetAppSettings();

	POSITION pos = s.slSubtitlePathsAddons.Find(tmp);
	if (!pos) {
		s.slSubtitlePathsAddons.AddTail(tmp);
	}
}

void CMainFrame::AddAudioPathsAddons(CString FileName)
{
	CString tmp(AddSlash(GetFolderOnly(FileName)).MakeUpper());
	AppSettings& s = AfxGetAppSettings();

	POSITION pos = s.slAudioPathsAddons.Find(tmp);
	if (!pos) {
		s.slAudioPathsAddons.AddTail(tmp);
	}
}

BOOL CMainFrame::CheckMainFilter(IBaseFilter* pBF)
{
	CString fName = GetCurFileName();
	if (CString(fName).MakeLower().Find(L"youtube") >= 0) {
		return TRUE;
	}

	if (m_nMainFilterId) {
		if (m_nMainFilterId == (DWORD_PTR)pBF) {
			return TRUE;
		}

		return FALSE;
	}

	m_nMainFilterId = (DWORD_PTR)pBF;
	while (pBF) {
		if (CComQIPtr<IFileSourceFilter> pFSF = pBF) {
			if (pFSF == m_pMainFSF) {
				return TRUE;
			}

			break;
		}

		IPin* pPin = GetFirstPin(pBF);

		pPin = GetUpStreamPin(pBF, pPin);
		if (pPin) {
			pBF = GetFilterFromPin(pPin);
		}
	}

	m_nMainFilterId = NULL;
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
			TCHAR drive = fn2[0];
			CAtlList<CString> sl;
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

void CMainFrame::MakeDVDLabel(CString& label, CString* pDVDlabel)
{
	WCHAR buff[MAX_PATH] = { 0 };
	ULONG len = 0;
	if (m_pDVDI && SUCCEEDED(m_pDVDI->GetDVDDirectory(buff, _countof(buff), &len))) {
		CString DVDPath(buff);
		DVDPath.Replace('\\', '/');
		int pos = DVDPath.Find(L"/VIDEO_TS");
		if (pos > 1) {
			DVDPath.Delete(pos, DVDPath.GetLength() - pos);
			CString fn2 = DVDPath.Mid(DVDPath.ReverseFind('/') + 1);

			if (pDVDlabel) {
				*pDVDlabel = fn2;
			}

			if (fn2.GetLength() == 2 && fn2[fn2.GetLength() - 1] == ':') {
				TCHAR drive = fn2[0];
				CAtlList<CString> sl;
				cdrom_t CDRom_t = GetCDROMType(drive, sl);
				if (CDRom_t == CDROM_DVDVideo) {
					CString DVDLabel = GetDriveLabel(drive);
					if (DVDLabel.GetLength() > 0) {
						fn2.AppendFormat(L" (%s)", DVDLabel);
						if (pDVDlabel) {
							*pDVDlabel = DVDLabel;
						}
					}
				}
			}

			if (!fn2.IsEmpty()) {
				label.AppendFormat(L" \"%s\"", fn2);
			}
		}
	}
}

CString CMainFrame::GetCurFileName()
{
	CString fn = m_wndPlaylistBar.GetCurFileName();
	if (fn.IsEmpty() && m_pMainFSF) {
		LPOLESTR pFN = NULL;
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
				if (::PathFileExists(path) && CVobFile::GetTitleInfo(path, loc.TitleNum, VTSN, TTN)) {
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

BOOL CMainFrame::OpenIso(CString pathName)
{
	if (m_DiskImage.CheckExtension(pathName)) {
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		TCHAR diskletter = m_DiskImage.MountDiskImage(pathName);
		if (diskletter) {

			if (OpenBD(CString(diskletter) + L":\\", INVALID_TIME, FALSE)) {
				AddRecent(pathName);
				return TRUE;
			}

			if (::PathFileExists(CString(diskletter) + L":\\VIDEO_TS\\VIDEO_TS.IFO")) {
				CAutoPtr<OpenDVDData> p(DNew OpenDVDData());
				p->path = CString(diskletter) + L":\\";
				p->bAddRecent = FALSE;
				OpenMedia(p);

				AddRecent(pathName);
				return TRUE;
			}

			if (pathName.Right(7).MakeLower() == L".iso.wv") {
				WIN32_FIND_DATA fd;
				HANDLE hFind = FindFirstFile(CString(diskletter) + L":\\*.wv", &fd);
				if (hFind != INVALID_HANDLE_VALUE) {
					CAtlList<CString> sl;
					sl.AddTail(CString(diskletter) + L":\\" + fd.cFileName);
					FindClose(hFind);

					m_wndPlaylistBar.Open(sl, false);
					OpenCurPlaylistItem(INVALID_TIME, FALSE);

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
	if (AfxGetAppSettings().fKeepHistory) {
		CRecentFileList* pMRU = &AfxGetAppSettings().MRU;
		pMRU->ReadList();
		pMRU->Add(pathName);
		pMRU->WriteList();
		SHAddToRecentDocs(SHARD_PATH, pathName);
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

REFTIME CMainFrame::GetAvgTimePerFrame() const
{
	REFTIME refAvgTimePerFrame = 0.0;

	if (FAILED(m_pBV->get_AvgTimePerFrame(&refAvgTimePerFrame))) {
		if (m_pCAP) {
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

BOOL CMainFrame::OpenYoutubePlaylist(CString url)
{
	if (AfxGetAppSettings().bYoutubeLoadPlaylist && PlayerYouTubePlaylistCheck(url)) {
		YoutubePlaylist youtubePlaylist;
		int idx_CurrentPlay = 0;
		if (PlayerYouTubePlaylist(url, youtubePlaylist, idx_CurrentPlay)) {
			m_wndPlaylistBar.Empty();

			POSITION pos = youtubePlaylist.GetHeadPosition();
			while (pos) {
				YoutubePlaylistItem& item = youtubePlaylist.GetNext(pos);
				CAtlList<CString> fns;
				fns.AddHead(item.url);
				m_wndPlaylistBar.Append(fns, false, NULL, false);
				m_wndPlaylistBar.SetLast();
				m_wndPlaylistBar.SetCurLabel(item.title);
			}
			m_wndPlaylistBar.SetSelIdx(idx_CurrentPlay, true);

			OpenCurPlaylistItem();
			return TRUE;
		}
	}

	return FALSE;
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
		if (m_pMainFrame->m_iMediaLoadState == MLS_LOADING) {
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
		if (m_pMainFrame->m_iMediaLoadState == MLS_CLOSING) {
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
}

#pragma endregion GraphThread
