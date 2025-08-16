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

#pragma once

#include "SettingsDefines.h"
#include "FakeFilterMapper2.h"
#include "FilterEnum.h"
#include "VideoSettings.h"
#include "Subtitles/STS.h"
#include "MediaFormats.h"
#include "DVBChannel.h"
#include <afxsock.h>
#include "FileItem.h"

constexpr auto EXTENDED_PATH_PREFIX = LR"(\\?\)";

#define ENABLE_ASSFILTERMOD 0

// flags for CAppSettings::nCS
#define CS_NONE			0
#define CS_SEEKBAR		(1 << 0)
#define CS_TOOLBAR		(1 << 1)
#define CS_INFOBAR		(1 << 2)
#define CS_STATSBAR		(1 << 3)
#define CS_STATUSBAR	(1 << 4)

constexpr auto CLSW_NONE               = 0ull;
constexpr auto CLSW_OPEN               = 1ull;
constexpr auto CLSW_PLAY               = (CLSW_OPEN << 1);
constexpr auto CLSW_CLOSE              = (CLSW_PLAY << 1);
constexpr auto CLSW_STANDBY            = (CLSW_CLOSE << 1);
constexpr auto CLSW_HIBERNATE          = (CLSW_STANDBY << 1);
constexpr auto CLSW_SHUTDOWN           = (CLSW_HIBERNATE << 1);
constexpr auto CLSW_LOGOFF             = (CLSW_SHUTDOWN << 1);
constexpr auto CLSW_LOCK               = (CLSW_LOGOFF << 1);
constexpr auto CLSW_AFTERPLAYBACK_MASK = (CLSW_CLOSE | CLSW_STANDBY | CLSW_SHUTDOWN | CLSW_HIBERNATE | CLSW_LOGOFF | CLSW_LOCK);
constexpr auto CLSW_FULLSCREEN         = (CLSW_LOCK << 1);
constexpr auto CLSW_NEW                = (CLSW_FULLSCREEN << 1);
constexpr auto CLSW_HELP               = (CLSW_NEW << 1);
constexpr auto CLSW_DVD                = (CLSW_HELP << 1);
constexpr auto CLSW_CD                 = (CLSW_DVD << 1);
constexpr auto CLSW_ADD                = (CLSW_CD << 1);
constexpr auto CLSW_MINIMIZED          = (CLSW_ADD << 1);
constexpr auto CLSW_CLIPBOARD          = (CLSW_MINIMIZED << 1);

constexpr auto CLSW_REGEXTVID          = (CLSW_CLIPBOARD << 1);
constexpr auto CLSW_REGEXTAUD          = (CLSW_REGEXTVID << 1);
constexpr auto CLSW_REGEXTPL           = (CLSW_REGEXTAUD << 1);
constexpr auto CLSW_UNREGEXT           = (CLSW_REGEXTPL << 1);

constexpr auto CLSW_STARTVALID         = (CLSW_UNREGEXT << 1);
constexpr auto CLSW_NOFOCUS            = (CLSW_STARTVALID << 1);
constexpr auto CLSW_FIXEDSIZE          = (CLSW_NOFOCUS << 1);
constexpr auto CLSW_MONITOR            = (CLSW_FIXEDSIZE << 1);
constexpr auto CLSW_D3DFULLSCREEN      = (CLSW_MONITOR << 1);
constexpr auto CLSW_ADMINOPTION        = (CLSW_D3DFULLSCREEN << 1);
constexpr auto CLSW_SLAVE              = (CLSW_ADMINOPTION << 1);
constexpr auto CLSW_AUDIORENDERER      = (CLSW_SLAVE << 1);
constexpr auto CLSW_RESET              = (CLSW_AUDIORENDERER << 1);
constexpr auto CLSW_RANDOMIZE          = (CLSW_RESET << 1);
constexpr auto CLSW_VOLUME             = (CLSW_RANDOMIZE << 1);
constexpr auto CLSW_DEVICE             = (CLSW_VOLUME << 1);
constexpr auto CLSW_UNRECOGNIZEDSWITCH = (CLSW_DEVICE << 1);

#define APP_RECENTFILES_MIN		5
#define APP_RECENTFILES_DEF		20
#define APP_RECENTFILES_MAX		100

#define APP_WEBSRVPORT_MIN		1
#define APP_WEBSRVPORT_DEF		13579
#define APP_WEBSRVPORT_MAX		65535

#define APP_WEBSRVQUALITY_MIN	70
#define APP_WEBSRVQUALITY_DEF	85
#define APP_WEBSRVQUALITY_MAX	100

#define APP_AUDIOTIMESHIFT_MIN	(-10*60*1000) // -10 munutes
#define APP_AUDIOTIMESHIFT_MAX	(10*60*1000)  // +10 munutes

#define APP_BUFDURATION_MIN		 1000
#define APP_BUFDURATION_DEF		 3000
#define APP_BUFDURATION_MAX		15000

#define APP_NETTIMEOUT_MIN		 2
#define APP_NETTIMEOUT_DEF		10
#define APP_NETTIMEOUT_MAX		60

#define APP_NETRECEIVETIMEOUT_MIN	 2
#define APP_NETRECEIVETIMEOUT_DEF	10
#define APP_NETRECEIVETIMEOUT_MAX	10

#define APP_AUDIOLEVEL_MAX		 10.0
#define APP_AUDIOLEVEL_MIN		-10.0
#define APP_AUDIOGAIN_MAX		 10.0
#define APP_AUDIOGAIN_MIN		 -3.0

#define APP_THUMBWIDTH_MIN		256
#define APP_THUMBWIDTH_MAX		5120

enum {
	MODE_SHOWCAPTIONMENU,
	MODE_HIDEMENU,
	MODE_FRAMEONLY,
	MODE_BORDERLESS,
	MODE_COUNT
}; // flags for Caption & Menu Mode

enum {
	SUBRNDT_NONE = 0,
	SUBRNDT_ISR,
	SUBRNDT_VSFILTER,
	SUBRNDT_XYSUBFILTER,
	SUBRNDT_ASSFILTERMOD
};

enum : int {
	STEREO3D_AUTO = 0,
	STEREO3D_MONO,
	STEREO3D_ROWINTERLEAVED,
	STEREO3D_ROWINTERLEAVED_2X,
	STEREO3D_HALFOVERUNDER,
	STEREO3D_OVERUNDER,
};

// Enumeration for MCE remote control (careful : add 0x010000 for all keys!)
enum MCE_RAW_INPUT {
	MCE_DETAILS				= 0x010209,
	MCE_GUIDE				= 0x01008D,
	MCE_TVJUMP				= 0x010025,
	MCE_STANDBY				= 0x010082,
	MCE_OEM1				= 0x010080,
	MCE_OEM2				= 0x010081,
	MCE_MYTV				= 0x010046,
	MCE_MYVIDEOS			= 0x01004A,
	MCE_MYPICTURES			= 0x010049,
	MCE_MYMUSIC				= 0x010047,
	MCE_RECORDEDTV			= 0x010048,
	MCE_DVDANGLE			= 0x01004B,
	MCE_DVDAUDIO			= 0x01004C,
	MCE_DVDMENU				= 0x010024,
	MCE_DVDSUBTITLE			= 0x01004D,
	MCE_RED					= 0x01005B,
	MCE_GREEN				= 0x01005C,
	MCE_YELLOW				= 0x01005D,
	MCE_BLUE				= 0x01005E,
	MCE_MEDIA_NEXTTRACK		= 0x0100B5,
	MCE_MEDIA_PREVIOUSTRACK	= 0x0100B6,
};

#define AUDRNDT_NULL_COMP   L"Null Audio Renderer (Any)"
#define AUDRNDT_NULL_UNCOMP L"Null Audio Renderer (Uncompressed)"
#define AUDRNDT_MPC         L"MPC Audio Renderer"

#define DEFAULT_AUDIO_PATHS    L".;.\\audio;.\\fandub"
#define DEFAULT_SUBTITLE_PATHS L".;.\\subtitles;.\\subs;.\\*sub"
#define DEFAULT_JUMPDISTANCE_1  1000
#define DEFAULT_JUMPDISTANCE_2  5000
#define DEFAULT_JUMPDISTANCE_3 20000

enum dvstype {
	DVS_HALF,
	DVS_NORMAL,
	DVS_DOUBLE,
	DVS_STRETCH,
	DVS_FROMINSIDE,
	DVS_FROMOUTSIDE,
	DVS_ZOOM1,
	DVS_ZOOM2
};

enum favtype {
	FAV_FILE,
	FAV_DVD,
};

enum {
	TIME_TOOLTIP_ABOVE_SEEKBAR,
	TIME_TOOLTIP_BELOW_SEEKBAR
};

enum engine_t {
	DirectShow = 0,
	ShockWave
};

enum : int {
	TEXTBAR_EMPTY = 0,
	TEXTBAR_FILENAME,
	TEXTBAR_TITLE,
	TEXTBAR_FULLPATH,
};

enum : int {
	STARTUPWND_DEFAULT = 0,
	STARTUPWND_REMLAST,
	STARTUPWND_SPECIFIED,
};

enum : int {
	PLAYBACKWND_NONE = 0,
	PLAYBACKWND_SCALEVIDEO,
	PLAYBACKWND_FITSCREEN,
	PLAYBACKWND_FITSCREENLARGER,
};

inline static const std::vector<int> s_CommonVideoHeights = { 0, 240, 360, 480, 720, 1080, 1440, 2160, 2880, 4320 };

struct ShaderC {
	CString   label;
	CString   profile;
	CString   srcdata;
	ULONGLONG length = 0;
	FILETIME  ftwrite = {0,0};

	bool Match(LPCWSTR _label, const bool _bD3D11) const {
		return (label.CompareNoCase(_label) == 0 && (_bD3D11 == (profile == "ps_4_0")));
	}
};

#pragma pack(push, 1)

struct dispmode {
	bool bValid = false;
	CSize size = 0;
	int bpp = 0;
	int freq = 0;
	DWORD dmDisplayFlags = 0;

	bool operator == (const dispmode& dm) const {
		return (bValid == dm.bValid && size == dm.size && bpp == dm.bpp && freq == dm.freq && dmDisplayFlags == dm.dmDisplayFlags);
	};

	bool operator < (const dispmode& dm) const {
		bool bRet = false;

		// Ignore bValid when sorting
		if (size.cx < dm.size.cx) {
			bRet = true;
		} else if (size.cx == dm.size.cx) {
			if (size.cy < dm.size.cy) {
				bRet = true;
			} else if (size.cy == dm.size.cy) {
				if (freq < dm.freq) {
					bRet = true;
				} else if (freq == dm.freq) {
					if (bpp < dm.bpp) {
						bRet = true;
					} else if (bpp == dm.bpp) {
						bRet = (dmDisplayFlags & DM_INTERLACED) && !(dm.dmDisplayFlags & DM_INTERLACED);
					}
				}
			}
		}

		return bRet;
	};
};

struct fpsmode {
	double vfr_from = 0.000;
	double vfr_to = 0.000;
	bool bChecked = false;
	dispmode dmFSRes;
	bool bValid = false;

	void Empty() {
		memset(this, 0, sizeof(*this));
	}
};

#pragma pack(pop)

class wmcmd : public ACCEL
{
	ACCEL backup_accel;
	UINT  backup_appcmd;
	UINT  backup_mouse;
	UINT  backup_mouseFS;

public:
	DWORD dwname;
	UINT appcmd;
	enum {
		NONE,
		LDOWN,
		LUP,
		LDBLCLK,
		MDOWN,
		MUP,
		MDBLCLK,
		RDOWN,
		RUP,
		RDBLCLK,
		X1DOWN,
		X1UP,
		X1DBLCLK,
		X2DOWN,
		X2UP,
		X2DBLCLK,
		WUP,
		WDOWN,
		WLEFT,
		WRIGHT,
		LAST
	};
	UINT mouse;
	UINT mouseFS;
	CStringA rmcmd;
	int rmrepcnt;

	wmcmd(WORD cmd = 0) {
		this->cmd = cmd;
	}

	wmcmd(WORD cmd,  DWORD dwname,
		WORD key = 0, BYTE fVirt = 0,
		UINT appcmd = 0,
		UINT mouse = NONE, UINT mouseFS = NONE,
		LPCSTR rmcmd = "", int rmrepcnt = 5)
	{
		this->cmd      = cmd;
		this->key      = key;
		this->fVirt    = fVirt|FVIRTKEY|FNOINVERT;
		this->dwname   = dwname;
		this->appcmd   = backup_appcmd  = appcmd;
		this->mouse    = backup_mouse   = mouse;
		this->mouseFS  = backup_mouseFS = mouseFS;
		this->rmcmd    = rmcmd;
		this->rmrepcnt = rmrepcnt;
		backup_accel = *this;
	}

	bool operator == (const wmcmd& wc) const {
		return(cmd > 0 && cmd == wc.cmd);
	}

	CString GetName() const {
		return ResStr (dwname);
	}

	void Restore() {
		*(ACCEL*)this = backup_accel;
		appcmd  = backup_appcmd;
		mouse   = backup_mouse;
		mouseFS = backup_mouseFS;
		rmcmd.Empty();
		rmrepcnt = 5;
	}

	bool IsModified() const {
		return (memcmp((const ACCEL*)this, &backup_accel, sizeof(ACCEL))
			|| appcmd  != backup_appcmd
			|| mouse   != backup_mouse
			|| mouseFS != backup_mouseFS
			|| !rmcmd.IsEmpty()
			|| rmrepcnt != 5);
	}
};

class CRemoteCtrlClient : public CAsyncSocket
{
protected:
	CCritSec m_csLock;
	CWnd* m_pWnd = nullptr;
	enum {DISCONNECTED, CONNECTED, CONNECTING} m_nStatus = DISCONNECTED;
	CString m_addr;

	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);

	virtual void OnCommand(CStringA str) PURE;

	void ExecuteCommand(CStringA cmd, int repcnt);

public:
	CRemoteCtrlClient();
	void SetHWND(HWND hWnd);
	void Connect(CString addr);
	void DisConnect();
	int GetStatus() const { return(m_nStatus); }
};

class CWinLircClient : public CRemoteCtrlClient
{
protected:
	virtual void OnCommand(CStringA str);

public:
	CWinLircClient();
};


class CFiltersPrioritySettings {
public:
	std::map<CString, CLSID> values;

	CFiltersPrioritySettings() {
		SetDefault();
	}
	void SetDefault() {
		static LPCWSTR formats[] = {L"http", L"udp", L"avi", L"mkv", L"mpegts", L"mpeg", L"mp4", L"flv", L"wmv"};

		values.clear();
		for (const auto& format : formats) {
			values[format] = CLSID_NULL;
		}
	};

	void LoadSettings();
	void SaveSettings();
};

typedef std::list<CString> cmdLine;

#define MaxFullScreenModes 30
#define MaxMonitorId 10u
struct fullScreenRes {
	std::vector<fpsmode> dmFullscreenRes;
	CString monitorId;
};

class CAppSettings
{
	bool bInitialized;

public:
	bool bResetSettings;

	// cmdline params
	UINT64 nCLSwitches;
	std::list<CString>	slFilters;
	std::list<CString>	slFiles, slDubs;
	CSubtitleItemList	slSubs;

	// Initial position (used by command line flags)
	__int64				rtShift;
	__int64				rtStart;
	ULONG				lDVDTitle;
	ULONG				lDVDChapter;
	DVD_HMSF_TIMECODE	DVDPosition;
	bool				bStartMainTitle;
	bool				bNormalStartDVD;

	bool				bPasteClipboardURL;

	int				iMonitor;

	void			ParseCommandLine(cmdLine& cmdln);

	// Added a Debug display to the screen (/debug option)
	bool			bShowDebugInfo;
	int				iAdminOption;

	// Player
	int				iMultipleInst;
	bool			bTrayIcon;
	union {
		struct{
			UINT Enable : 1;
			UINT FileName : 1;
			UINT SeekTime : 1;
		};
		UINT value;
	} ShowOSD;

	bool			bHideCDROMsSubMenu;
	DWORD			dwPriority;
	int				iTitleBarTextStyle;
	int				iSeekBarTextStyle;
	bool			bKeepHistory;
	int				iRecentFilesNumber;
	bool			bRecentFilesMenuEllipsis;
	bool			bRecentFilesShowUrlTitle;
	unsigned		nHistoryEntriesMax;
	bool			bRememberDVDPos;
	bool			bRememberFilePos;
	bool			bRememberPlaylistItems;
	bool			bSavePnSZoom;
	float			dZoomX;
	float			dZoomY;

	// Formats
	CMediaFormats	m_Formats;
	bool			bAssociatedWithIcons;
	// file/dir context menu
	bool			bSetContextFiles;
	bool			bSetContextDir;

	// Keys
	std::vector<wmcmd> wmcmds;
	HACCEL			hAccel;
	bool			bWinLirc;
	CString			strWinLircAddr;
	CWinLircClient	WinLircClient;
	bool			bGlobalMedia;
	int				AccelTblColWidths[6];

	// Mouse
	UINT			nMouseLeftClick;
	bool			bMouseLeftClickOpenRecent;
	UINT			nMouseLeftDblClick;
	bool			bMouseEasyMove;
	UINT			nMouseRightClick;
	struct MOUSE_ASSIGNMENT {
		UINT normal;
		UINT ctrl;
		UINT shift;
		UINT rbtn;
	};
	MOUSE_ASSIGNMENT MouseMiddleClick;
	MOUSE_ASSIGNMENT MouseX1Click;
	MOUSE_ASSIGNMENT MouseX2Click;
	MOUSE_ASSIGNMENT MouseWheelUp;
	MOUSE_ASSIGNMENT MouseWheelDown;
	MOUSE_ASSIGNMENT MouseWheelLeft;
	MOUSE_ASSIGNMENT MouseWheelRight;

	// Logo
	int				nLogoId;
	bool			bLogoExternal;
	CString			strLogoFileName;

	// Window size
	int				nStartupWindowMode;
	CSize			szSpecifiedWndSize;
	int				nPlaybackWindowMode;
	int				nAutoScaleFactor;
	int				nAutoFitFactor;
	bool			bResetWindowAfterClosingFile;
	bool			bRememberWindowPos;
	CPoint			ptLastWindowPos;
	CSize			szLastWindowSize;
	UINT			nLastWindowType;
	bool			bLimitWindowProportions;
	bool			bSnapToDesktopEdges;

	CSize sizeFixedWindow; // not saved. from command line
	bool HasFixedWindowSize() const { return sizeFixedWindow.cx > 0 || sizeFixedWindow.cy > 0; }

	// Web Inteface
	bool			fEnableWebServer;
	int				nWebServerPort;
	int				nWebServerQuality;
	int				nCmdlnWebServerPort;
	bool			fWebServerUseCompression;
	bool			fWebServerLocalhostOnly;
	bool			bWebUIEnablePreview;
	bool			fWebServerPrintDebugInfo;
	CString			strWebRoot, strWebDefIndex;
	CString			strWebServerCGI;

	// Playback
	int				nVolume;
	int				nVolumeStep;
	bool			fMute;
	int				nBalance;
	int				nLoops;
	bool			fLoopForever;
	bool			fRewind;
	int				nSpeedStep;
	bool			bSpeedNotReset;
	bool			fUseInternalSelectTrackLogic;
	CStringW		strSubtitlesLanguageOrder;
	CStringW		strAudiosLanguageOrder;
	bool			bRememberSelectedTracks;
	int				nAudioWindowMode;
	int				nAddSimilarFiles;
	bool			fEnableWorkerThreadForOpening;
	bool			fReportFailedPins;

	// Audio
	bool			fAutoloadAudio;
	bool			fPrioritizeExternalAudio;
	CString			strAudioPaths;

	// DVD-Video
	bool			bUseDVDPath;
	CString			strDVDPath;
	LCID			idMenuLang, idAudioLang, idSubtitlesLang;
	bool			bClosedCaptions;

	// Output
	CRenderersSettings m_VRSettings;
	int				iSelectedVideoRenderer;

	bool			fDualAudioOutput;
	CString			strAudioRendererDisplayName;
	CString			strAudioRendererDisplayName2;

	int				iStereo3DMode;
	bool			bStereo3DSwapLR;

	// Fullscreen
	bool			fLaunchfullscreen;
	bool			fShowBarsWhenFullScreen;
	int				nShowBarsWhenFullScreenTimeOut;
	bool			fExitFullScreenAtTheEnd;
	bool			fExitFullScreenAtFocusLost;
	CStringW		strFullScreenMonitor;
	CStringW		strFullScreenMonitorID;

	struct t_fullScreenModes {
		std::vector<fullScreenRes> res;
		BOOL bEnabled;
		bool bApplyDefault;
	} fullScreenModes;

	bool			fRestoreResAfterExit;
	dispmode		dmFSMonOnLaunch;
	CString			strFSMonOnLaunch;
	int				iDMChangeDelay;

	CString strTabs;

	// Sync Renderer Settings

	// Capture (BDA configuration)
	// BDA configuration
	int				iDefaultCaptureDevice; // Default capture device (analog=0, 1=digital)
	CString			strAnalogVideo;
	CString			strAnalogAudio;
	int				iAnalogCountry;
	CString			strBDANetworkProvider;
	CString			strBDATuner;
	CString			strBDAReceiver;
	//CString		strBDAStandard;
	int				iBDAScanFreqStart;
	int				iBDAScanFreqEnd;
	int				iBDABandwidth;
	bool			fBDAUseOffset;
	int				iBDAOffset;
	bool			fBDAIgnoreEncryptedChannels;
	int				nDVBLastChannel;
	std::list<CDVBChannel> m_DVBChannels;

	// Internal Filters
	bool			SrcFilters[SRC_COUNT];
	bool			VideoFilters[VDEC_COUNT];
	bool			AudioFilters[ADEC_COUNT];
	int				iBufferDuration;
	int				iNetworkTimeout;
	int				iNetworkReceiveTimeout;

	// Audio Switcher
	bool			bAudioMixer;
	int				nAudioMixerLayout;
	bool			bAudioStereoFromDecoder;
	bool			bAudioBassRedirect;
	double			dAudioCenter_dB;
	double			dAudioSurround_dB;
	double			dAudioGain_dB;
	bool			bAudioAutoVolumeControl;
	bool			bAudioNormBoost;
	int				iAudioNormLevel;
	int				iAudioNormRealeaseTime;
	int				iAudioSampleFormats;
	bool			bAudioTimeShift;
	int				iAudioTimeShift;
	bool			bAudioFilters;
	CStringA		strAudioFilter1;
	bool			bAudioFiltersNotForStereo;

	// External Filters
	std::list<std::unique_ptr<FilterOverride>> m_ExternalFilters;

	// Subtitles
	int				iSubtitleRenderer;
	CString			strSubtitlePaths;
	bool			fPrioritizeExternalSubtitles;
	bool			fDisableInternalSubtitles;
	bool			fAutoReloadExtSubtitles;
	bool			fUseSybresync;

	// Subtitles - Rendering
	bool			fOverridePlacement;
	int				nHorPos, nVerPos;
	int				nSubDelayInterval;

	// Subtitles - Default Style
	STSStyle		subdefstyle;

	// Interface
	bool			bUseDarkTheme;
	int				nThemeBrightness;
	int				nThemeRed;
	int				nThemeGreen;
	int				nThemeBlue;
	bool			bDarkMenu;
	bool			bDarkMenuBlurBehind;
	bool			bDarkTitle;
	COLORREF		clrFaceABGR;
	COLORREF		clrOutlineABGR;

	bool			bShowMilliSecs;
	bool			fUseTimeTooltip;
	int				nTimeTooltipPosition;
	bool			fSmartSeek;
	bool			bSmartSeekOnline;
	int				iSmartSeekSize;
	int				iSmartSeekVR;
	bool			fChapterMarker;
	bool			fFlybar;
	int				iPlsFontPercent;
	int				iToolbarSize;
	bool			fFlybarOnTop;
	bool			fUseWin7TaskBar;

	// OSD
	CString			strOSDFont;
	int				nOSDSize;
	bool			bOSDFontShadow;
	bool			bOSDFontAA;
	int				nOSDTransparent;
	int				nOSDBorder;
	COLORREF		clrFontABGR;
	COLORREF		clrGrad1ABGR;
	COLORREF		clrGrad2ABGR;

	int				iDlgPropX;
	int				iDlgPropY;

	// Miscellaneous
	int				nJumpDistS;
	int				nJumpDistM;
	int				nJumpDistL;
	bool			fFastSeek;
	bool			fDontUseSearchInFolder;
	bool			fPreventMinimize;
	bool			bPauseMinimizedVideo;
	bool			bHideWindowedMousePointer;
	int				nMinMPlsDuration;
	bool			fLCDSupport;
	bool			bWinMediaControls;
	bool			fMiniDump;
	bool			bUpdaterAutoCheck;
	int				nUpdaterDelay;
	time_t			tUpdaterLastCheck;
	CStringW		strFFmpegExePath;

	// MENUS
	// View
	int				iCaptionMenuMode; // normal -> hidemenu -> frameonly -> borderless
	bool			fHideNavigation;
	UINT			nCS; // Control state for toolbars
	// Language
	int				iLanguage;
	int				iCurrentLanguage;
	// Subtitles menu
	bool			fEnableSubtitles;
	bool			fUseDefaultSubtitlesStyle;
	bool			fForcedSubtitles;
	// Video Frame
	int				iDefaultVideoSize;
	bool			bNoSmallUpscale;
	bool			bNoSmallDownscale;
	bool			fKeepAspectRatio;
	CSize			sizeAspectRatio;
	bool			fCompMonDeskARDiff;
	// Pan&Scan
	CString			strPnSPreset;
	CStringArray	m_pnspresets;
	// On top menu
	int				iOnTop;
	// After Playback
	bool			fExitAfterPlayback;
	bool			bCloseFileAfterPlayback;
	bool			fNextInDirAfterPlayback;
	bool			fNextInDirAfterPlaybackLooped;

	// WINDOWS
	// Add Favorite
	bool			bFavRememberPos;
	bool			bFavRelativeDrive;
	// Save Image...
	CStringW		strSnapShotPath, strSnapShotExt;
	bool			bSnapShotSubtitles;
	// Save Thumbnails...
	int				iThumbRows, iThumbCols, iThumbWidth, iThumbQuality, iThumbLevelPNG;
	// Save Subtitle
	bool			bSubSaveExternalStyleFile;
	// Shader Combiner
	bool			bToggleShader;
	bool			bToggleShaderScreenSpace;
	std::list<CString> ShaderList;
	std::list<CString> ShaderListScreenSpace;
	std::list<CString> Shaders11PostScale;
	// Playlist (contex menu)
	bool			bShufflePlaylistItems;
	bool			bHidePlaylistFullScreen;
	bool			bShowPlaylistTooltip;
	bool			bShowPlaylistSearchBar;
	bool			bPlaylistNextOnError;
	bool			bPlaylistSkipInvalid;
	bool			bPlaylistDetermineDuration;

	// OTHER STATES
	CString			strLastOpenFile; // not saved
	CString			strLastOpenDir;
	CString			strLastSavedPlaylistDir;

	UINT			nLastUsedPage;
	bool			bRemainingTime;
	bool			bShowZeroHours;

	CString			strTimeOnSeekBar;
	bool			bStatusBarIsVisible;

	HWND			hMasterWnd;

	bool			ExclusiveFSAllowed() const;
	CString			SelectedAudioRenderer() const;

	// list of temporary files
	std::list<CString> slTMPFilesList;

	CString			strLastOpenFilterDir;

	// youtube
	bool			bYoutubePageParser;
	struct {
		int		vfmt;
		int		res;
		bool	fps60;
		bool	hdr;
		int		afmt;
	} YoutubeFormat;
	CStringW		strYoutubeAudioLang;
	bool			bYoutubeLoadPlaylist;
	int				iYoutubeTagSelected = 0; // not saved
	std::map<CString, CString> youtubeSignatureCache;

	bool			bYDLEnable;
	CStringW		strYDLExePath;
	int				iYDLMaxHeight;
	bool			bYDLMaximumQuality;

	CStringW		strAceStreamAddress;
	CStringW		strTorrServerAddress;

	CStringW		strUserAgent;

	DWORD			nLastFileInfoPage;

	bool			IsISRSelect() const;
	bool			IsISRAutoLoadEnabled() const;

	bool			bOSDRemainingTime;
	bool			bOSDLocalTime;
	bool			bOSDFileName;

	int				HistoryColWidths[3];

	void			SavePlaylistTabSetting();

	void			LoadFormats(const bool bLoadLanguage);
	void			SaveFormats();

	int				nCmdVolume;

private :
	LPCWSTR			SrcFiltersKeys[SRC_COUNT];
	LPCWSTR			VideoFiltersKeys[VDEC_COUNT];
	LPCWSTR			AudioFiltersKeys[ADEC_COUNT];

	__int64			ConvertTimeToMSec(CString& time) const;
	void			ExtractDVDStartPos(CString& strParam);

	void			CreateCommands();

public:
	CAppSettings();
	virtual ~CAppSettings();
	void			ResetSettings();
	void			LoadSettings(bool bForce = false);
	void			SaveSettings();
	void			SaveExternalFilters();

	CDVBChannel*	FindChannelByPref(int nPrefNumber);

	int				GetMultiInst();
	engine_t		GetFileEngine(CString path);

	CFiltersPrioritySettings	FiltersPriority;

	std::list<CString>			slSubtitlePathsAddons;
	std::list<CString>			slAudioPathsAddons;
};

CStringW ParseFileName(const CStringW& param);