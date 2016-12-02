/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "FilterEnum.h"
#include "../../filters/renderer/VideoRenderers/RenderersSettings.h"
#include "../../Subtitles/STS.h"
#include "MediaFormats.h"
#include "DVBChannel.h"
#include <afxsock.h>

#define ENABLE_ASSFILTERMOD 0

// flags for CAppSettings::nCS
#define CS_NONE			0
#define CS_SEEKBAR		(1 << 0)
#define CS_TOOLBAR		(1 << 1)
#define CS_INFOBAR		(1 << 2)
#define CS_STATSBAR		(1 << 3)
#define CS_STATUSBAR	(1 << 4)

#define CLSW_NONE				0
#define CLSW_OPEN				(1 << 0)
#define CLSW_PLAY				(1 << 1)
#define CLSW_CLOSE				(1 << 2)
#define CLSW_STANDBY			(1 << 3)
#define CLSW_HIBERNATE			(1 << 4)
#define CLSW_SHUTDOWN			(1 << 5)
#define CLSW_LOGOFF				(1 << 6)
#define CLSW_LOCK				(1 << 7)
#define CLSW_AFTERPLAYBACK_MASK	(CLSW_CLOSE|CLSW_STANDBY|CLSW_SHUTDOWN|CLSW_HIBERNATE|CLSW_LOGOFF|CLSW_LOCK)
#define CLSW_FULLSCREEN			(1 << 8)
#define CLSW_NEW				(1 << 9)
#define CLSW_HELP				(1 << 10)
#define CLSW_DVD				(1 << 11)
#define CLSW_CD					(1 << 12)
#define CLSW_ADD				(1 << 13)
#define CLSW_MINIMIZED			(1 << 14)

#define CLSW_REGEXTVID			(1 << 16)
#define CLSW_REGEXTAUD			(1 << 17)
#define CLSW_REGEXTPL			(1 << 18)
#define CLSW_UNREGEXT			(1 << 19)

#define CLSW_STARTVALID			(1 << 21)
#define CLSW_NOFOCUS			(1 << 22)
#define CLSW_FIXEDSIZE			(1 << 23)
#define CLSW_MONITOR			(1 << 24)
#define CLSW_D3DFULLSCREEN		(1 << 25)
#define CLSW_ADMINOPTION		(1 << 26)
#define CLSW_SLAVE				(1 << 27)
#define CLSW_AUDIORENDERER		(1 << 28)
#define CLSW_RESET				(1 << 29)
#define CLSW_UNRECOGNIZEDSWITCH	(1 << 30)

#define APP_RECENTFILES_MIN		5
#define APP_RECENTFILES_DEF		20
#define APP_RECENTFILES_MAX		50
#define APP_WEBSRVPORT_MIN		1
#define APP_WEBSRVPORT_DEF		13579
#define APP_WEBSRVPORT_MAX		65535
#define APP_WEBSRVQUALITY_MIN	70
#define APP_WEBSRVQUALITY_DEF	85
#define APP_WEBSRVQUALITY_MAX	100
#define APP_AUDIOTIMESHIFT_MIN	(-10*60*1000) // -10 munutes
#define APP_AUDIOTIMESHIFT_MAX	(10*60*1000)  // +10 munutes

#define OSD_ENABLE		(1 << 0)
#define OSD_FILENAME	(1 << 1)
#define OSD_SEEKTIME	(1 << 2)

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
#define DEFAULT_SUBTITLE_PATHS L".;.\\subtitles;.\\subs;.\\sub"
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
	FAV_DEVICE
};

#define MAX_DVD_POSITION 50
struct DVD_POSITION {
	ULONGLONG         llDVDGuid = 0;
	ULONG             lTitle    = 0;
	DVD_HMSF_TIMECODE Timecode  = { 0 };
};

#define MAX_FILE_POSITION 50
struct FILE_POSITION {
	CString  strFile;
	LONGLONG llPosition     = 0;
	int      nAudioTrack    = -1;
	int      nSubtitleTrack = -1;
};

enum {
	TIME_TOOLTIP_ABOVE_SEEKBAR,
	TIME_TOOLTIP_BELOW_SEEKBAR
};

enum engine_t {
	DirectShow = 0,
	ShockWave
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

#define MaxFpsCount 30
struct AChFR { //AutoChangeFullscrRes
	int bEnabled = 0;
	fpsmode dmFullscreenRes[MaxFpsCount];
	bool bApplyDefault = false;
	bool bSetGlobal = false; // not used
};

struct AccelTbl {
	bool bEnable;
	int cmd;
	int key;
	int id;
	int mwnd;
	int mfs;
	int appcmd;
	int remcmd;
	int repcnt;
};

#pragma pack(pop)

class wmcmd : public ACCEL
{
	ACCEL backup;
	UINT appcmdorg;
	UINT mouseorg;
	UINT mouseFSorg;

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
		LAST
	};
	UINT mouse;
	UINT mouseFS;
	CStringA rmcmd;
	int rmrepcnt;

	wmcmd(WORD cmd = 0) {
		this->cmd = cmd;
	}

	wmcmd(WORD cmd, WORD key, BYTE fVirt, DWORD dwname, UINT appcmd = 0, UINT mouse = NONE, UINT mouseFS = NONE, LPCSTR rmcmd = "", int rmrepcnt = 5) {
		this->cmd = cmd;
		this->key = key;
		this->fVirt = fVirt;
		this->appcmd = appcmdorg = appcmd;
		this->dwname = dwname;
		this->mouse = mouseorg = mouse;
		this->mouseFS = mouseFSorg = mouseFS;
		this->rmcmd = rmcmd;
		this->rmrepcnt = rmrepcnt;
		backup = *this;
	}

	bool operator == (const wmcmd& wc) const {
		return(cmd > 0 && cmd == wc.cmd);
	}

	CString GetName() const {
		return ResStr (dwname);
	}

	void Restore() {
		*(ACCEL*)this = backup;
		appcmd = appcmdorg;
		mouse = mouseorg;
		mouseFS = mouseFSorg;
		rmcmd.Empty();
		rmrepcnt = 5;
	}

	bool IsModified() const {
		return (memcmp((const ACCEL*)this, &backup, sizeof(ACCEL)) || appcmd != appcmdorg || mouse != mouseorg || mouseFS != mouseFSorg || !rmcmd.IsEmpty() || rmrepcnt != 5);
	}
};

class CRemoteCtrlClient : public CAsyncSocket
{
protected:
	CCritSec m_csLock;
	CWnd* m_pWnd;
	enum {DISCONNECTED, CONNECTED, CONNECTING} m_nStatus;
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

class CUIceClient : public CRemoteCtrlClient
{
protected:
	virtual void OnCommand(CStringA str);

public:
	CUIceClient();
};


class CFiltersPrioritySettings {
	template <class T = CString, class S = CString>
	class CAtlStringMap : public CAtlMap<S, T, CStringElementTraits<S> > {};

public:
	CAtlStringMap<CLSID> values;

	CFiltersPrioritySettings() {
		SetDefault();
	}
	void SetDefault() {
		static CString formats[] = {L"http", L"avi", L"mkv", L"mpegts", L"mpeg", L"mp4", L"flv", L"wmv"};

		values.RemoveAll();
		for (size_t i = 0; i < _countof(formats); i++) {
			values[formats[i]] = CLSID_NULL;
		}
	};

	void LoadSettings() {
		SetDefault();

		POSITION pos = values.GetStartPosition();
		while (pos) {
			CAtlStringMap<CLSID>::CPair* pPair = values.GetNext(pos);
			pPair->m_value = GUIDFromCString(AfxGetApp()->GetProfileString(IDS_R_FILTERS_PRIORITY, pPair->m_key, CStringFromGUID(pPair->m_value)));
		}
	};

	void SaveSettings() {
		POSITION pos = values.GetStartPosition();
		while (pos) {
			CAtlStringMap<CLSID>::CPair* pPair = values.GetNext(pos);
			AfxGetApp()->WriteProfileString(IDS_R_FILTERS_PRIORITY, pPair->m_key, CStringFromGUID(pPair->m_value));
		}
	};
};

class CSubtitleItem
{
	CString m_fn;
	CString m_title;

public:
	CSubtitleItem() {};
	CSubtitleItem(const CString& fname, const CString& title = L"") {
		m_fn = fname;
		m_title = title;
	}
	CSubtitleItem(const WCHAR* fname, const WCHAR* title = L"") {
		m_fn = fname;
		m_title = title;
	}

	const CSubtitleItem& operator = (const CString& fname) {
		m_fn = fname;

		return *this;
	}

	operator CString() const {
		return m_fn;
	}

	operator LPCTSTR() const {
		return m_fn;
	}

	void SetName(CString name) {
		m_fn = name;
	}

	CString GetName() const {
		return m_fn;
	};

	// Title
	void SetTitle(CString title) {
		m_title = title;
	}

	CString GetTitle() const {
		return m_title;
	};
};
typedef CAtlList<CSubtitleItem> CSubtitleItemList;

class CAppSettings
{
	bool	fInitialized;

	class CRecentFileAndURLList : public CRecentFileList
	{
	public:
		CRecentFileAndURLList(UINT nStart, LPCTSTR lpszSection,
							  LPCTSTR lpszEntryFormat, int nSize,
							  int nMaxDispLen = AFX_ABBREV_FILENAME_LEN);

		virtual void Add(LPCTSTR lpszPathName); // we have to override CRecentFileList::Add because the original version can't handle URLs

		void SetSize(int nSize);
	};

public:

	bool fReset;

	bool fShadersNeedSave;

	// cmdline params
	UINT nCLSwitches;
	CAtlList<CString>	slFiles, slDubs, slFilters;
	CSubtitleItemList	slSubs;

	// Initial position (used by command line flags)
	__int64				rtShift;
	__int64				rtStart;
	ULONG				lDVDTitle;
	ULONG				lDVDChapter;
	DVD_HMSF_TIMECODE	DVDPosition;
	bool				fStartMainTitle;
	bool				fNormalStartDVD;

	CSize sizeFixedWindow;
	bool HasFixedWindowSize() const { return sizeFixedWindow.cx > 0 || sizeFixedWindow.cy > 0; }
	//int			iFixedWidth, iFixedHeight;
	int				iMonitor;

	CString			ParseFileName(CString const& param);
	void			ParseCommandLine(CAtlList<CString>& cmdln);

	// Added a Debug display to the screen (/debug option)
	bool			fShowDebugInfo;
	int				iAdminOption;
	int				iDXVer;


	// Player
	int				iMultipleInst;
	bool			fTrayIcon;
	int				iShowOSD;
	bool			fLimitWindowProportions;
	bool			fSnapToDesktopEdges;
	bool			fHideCDROMsSubMenu;
	DWORD			dwPriority;
	int				iTitleBarTextStyle;
	bool			fTitleBarTextTitle;
	bool			fKeepHistory;
	int				iRecentFilesNumber;
	CRecentFileAndURLList MRU;
	CRecentFileAndURLList MRUDub;
	bool			fRememberDVDPos;
	bool			fRememberFilePos;
	bool			bRememberPlaylistItems;
	bool			fRememberWindowPos;
	bool			fRememberWindowSize;
	bool			fSavePnSZoom;
	float			dZoomX;
	float			dZoomY;

	// Formats
	CMediaFormats	m_Formats;
	bool			fAssociatedWithIcons;
	// file/dir context menu
	bool			bSetContextFiles;
	bool			bSetContextDir;

	// Keys
	CList<wmcmd>	wmcmds;
	HACCEL			hAccel;
	bool			fWinLirc;
	CString			strWinLircAddr;
	CWinLircClient	WinLircClient;
	bool			fUIce;
	CString			strUIceAddr;
	CUIceClient		UIceClient;
	bool			fGlobalMedia;

	// Logo
	UINT			nLogoId;
	bool			fLogoExternal;
	CString			strLogoFileName;

	// Web Inteface
	BOOL			fEnableWebServer;
	int				nWebServerPort;
	int				nWebServerQuality;
	int				nCmdlnWebServerPort;
	bool			fWebServerUseCompression;
	bool			fWebServerLocalhostOnly;
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
	bool			fRememberZoomLevel;
	int				iZoomLevel;
	int				nAutoFitFactor;
	bool			fUseInternalSelectTrackLogic;
	CStringW		strSubtitlesLanguageOrder;
	CStringW		strAudiosLanguageOrder;
	int				nAudioWindowMode;
	bool			bAddSimilarFiles;
	bool			fEnableWorkerThreadForOpening;
	bool			fReportFailedPins;

	// Audio
	bool			fAutoloadAudio;
	bool			fPrioritizeExternalAudio;
	CString			strAudioPaths;

	// DVD-Video
	bool			fUseDVDPath;
	CString			strDVDPath;
	LCID			idMenuLang, idAudioLang, idSubtitlesLang;
	bool			fClosedCaptions;

	// Output
	CRenderersSettings m_VRSettings;
	int				iSelectedVideoRenderer;

	bool			fDualAudioOutput;
	CString			strAudioRendererDisplayName;
	CString			strSecondAudioRendererDisplayName;

	bool			fD3DFullscreen;
	int				iStereo3DMode;
	bool			bStereo3DSwapLR;

	// Color control
	int				iBrightness;
	int				iContrast;
	int				iHue;
	int				iSaturation;

	// Fullscreen
	bool			fLaunchfullscreen;
	bool			fShowBarsWhenFullScreen;
	int				nShowBarsWhenFullScreenTimeOut;
	bool			fExitFullScreenAtTheEnd;
	bool			fExitFullScreenAtFocusLost;
	CStringW		strFullScreenMonitor;
	CStringW		strFullScreenMonitorID;
	AChFR			AutoChangeFullscrRes;
	AccelTbl		AccelTblColWidth;
	bool			fRestoreResAfterExit;
	dispmode		dm_def;
	int				iDMChangeDelay;

	// Sync Renderer Settings

	// Capture (BDA configuration)
	// BDA configuration
	int				iDefaultCaptureDevice;		// Default capture device (analog=0, 1=digital)
	CString			strAnalogVideo;
	CString			strAnalogAudio;
	int				iAnalogCountry;
	CString			strBDANetworkProvider;
	CString			strBDATuner;
	CString			strBDAReceiver;
	//CString			strBDAStandard;
	int				iBDAScanFreqStart;
	int				iBDAScanFreqEnd;
	int				iBDABandwidth;
	bool			fBDAUseOffset;
	int				iBDAOffset;
	bool			fBDAIgnoreEncryptedChannels;
	UINT			nDVBLastChannel;
	CAtlList<CDVBChannel> m_DVBChannels;

	// Internal Filters
	bool			SrcFilters[SRC_LAST];
	bool			DXVAFilters[VDEC_DXVA_LAST];
	bool			VideoFilters[VDEC_LAST];
	bool			AudioFilters[ADEC_LAST];
	int				iBufferDuration;

	// Audio Switcher
	bool			bAudioMixer;
	int				nAudioMixerLayout;
	bool			bAudioBassRedirect;
	float			fAudioGain_dB;
	bool			bAudioAutoVolumeControl;
	bool			bAudioNormBoost;
	int				iAudioNormLevel;
	int				iAudioNormRealeaseTime;
	bool			bAudioTimeShift;
	int				iAudioTimeShift;

	// External Filters
	CAutoPtrList<FilterOverride> m_filters;

	// Subtitles
	int				iSubtitleRenderer;
	CString			strSubtitlePaths;
	bool			fPrioritizeExternalSubtitles;
	bool			fDisableInternalSubtitles;
	bool			fAutoReloadExtSubtitles;
	bool			fUseSybresync;
	CString			strISDb;

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
	int				nOSDTransparent;
	int				nOSDBorder;

	COLORREF		clrFaceABGR;
	COLORREF		clrOutlineABGR;
	COLORREF		clrFontABGR;
	COLORREF		clrGrad1ABGR;
	COLORREF		clrGrad2ABGR;

	bool			fUseTimeTooltip;
	int				nTimeTooltipPosition;
	bool			fFileNameOnSeekBar;
	bool			fSmartSeek;
	int				iSmartSeekSize;
	bool			fChapterMarker;
	bool			fFlybar;
	bool			fFontShadow;
	bool			fFontAA;
	bool			fFlybarOnTop;
	bool			fUseWin7TaskBar;
	int				nOSDSize;
	CString			strOSDFont;

	int				iDlgPropX;
	int				iDlgPropY;

	// Miscellaneous
	int				nJumpDistS;
	int				nJumpDistM;
	int				nJumpDistL;
	bool			fDontUseSearchInFolder;
	bool			fPreventMinimize;
	bool			bPauseMinimizedVideo;
	bool			fLCDSupport;
	bool			fFastSeek;
	bool			fMiniDump;
	bool			bUpdaterAutoCheck;
	int				nUpdaterDelay;
	time_t			tUpdaterLastCheck;

	// MENUS
	// View
	int				iCaptionMenuMode; // normal -> hidemenu -> frameonly -> borderless
	bool			fHideNavigation;
	UINT			nCS; // Control state for toolbars
	// Language
	int				iLanguage;
	// Subtitles menu
	bool			fEnableSubtitles;
	bool			fUseDefaultSubtitlesStyle;
	bool			fForcedSubtitles;
	// Video Frame
	int				iDefaultVideoSize;
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
	bool			fNextInDirAfterPlayback;
	bool			fNextInDirAfterPlaybackLooped;

	// WINDOWS
	// Add Favorite
	bool			bFavRememberPos;
	bool			bFavRelativeDrive;
	// Save Image...
	CString			strSnapShotPath, strSnapShotExt;
	// Save Thumbnails...
	int				iThumbRows, iThumbCols, iThumbWidth, iThumbQuality, iThumbLevelPNG;
	// Save Subtitle
	bool			bSubSaveExternalStyleFile;
	// Shader Editor
	struct Shader {
		CString		label;
		CString		target;
		CString		srcdata;
	};
	CAtlList<Shader> m_shaders;
	// Shader Combiner
	bool			fToggleShader;
	bool			fToggleShaderScreenSpace;
	CString			strShaderList;
	CString			strShaderListScreenSpace;
	// Playlist (contex menu)
	bool			bShufflePlaylistItems;
	bool			bHidePlaylistFullScreen;

	// OTHER STATES
	CString			strLastOpenDir;
	CString			strLastSavedPlaylistDir;

	UINT			nLastWindowType;
	CRect			rcLastWindowPos;
	UINT			nLastUsedPage;
	bool			fRemainingTime;

	CString			strTimeOnSeekBar;
	bool			bStatusBarIsVisible;

	//bool			fMonitorAutoRefreshRate;
	bool			fEnableEDLEditor;

	HWND			hMasterWnd;

	bool			IsD3DFullscreen() const;
	CString			SelectedAudioRenderer() const;
	void			ResetPositions();
	DVD_POSITION*	CurrentDVDPosition();
	bool			NewDvd(ULONGLONG llDVDGuid);
	FILE_POSITION*	CurrentFilePosition();
	bool			NewFile(LPCTSTR strFileName);
	bool			RemoveFile(LPCTSTR strFileName);

	void			SaveCurrentDVDPosition();
	void			ClearDVDPositions();
	void			SaveCurrentFilePosition();
	void			ClearFilePositions();

	void			DeserializeHex(LPCTSTR strVal, BYTE* pBuffer, int nBufSize);
	CString			SerializeHex(BYTE* pBuffer, int nBufSize) const;

	// list of temporary files
	CAtlList<CString> slTMPFilesList;

	CString			strLastOpenFilterDir;

	// youtube
	bool			bYoutubePageParser;
	struct {
		int		fmt;
		int		res;
		bool	fps60;
		bool	hdr;
	} YoutubeFormat;
	bool			bYoutubeLoadPlaylist;
	int				iYoutubeTagSelected = 0; // not saved

	DWORD			nLastFileInfoPage;

	bool			IsISRSelect() const;
	bool			IsISRAutoLoadEnabled() const;

	bool			bOSDRemainingTime;

private :
	DVD_POSITION	DvdPosition[MAX_DVD_POSITION];
	int				nCurrentDvdPosition;
	FILE_POSITION	FilePosition[MAX_FILE_POSITION];
	int				nCurrentFilePosition;

	CString			SrcFiltersKeys[SRC_LAST];
	CString			DXVAFiltersKeys[VDEC_DXVA_LAST];
	CString			VideoFiltersKeys[VDEC_LAST];
	CString			AudioFiltersKeys[ADEC_LAST];

	__int64			ConvertTimeToMSec(CString& time) const;
	void			ExtractDVDStartPos(CString& strParam);

	void			CreateCommands();

public:
	CAppSettings();
	virtual ~CAppSettings();
	void			SaveSettings();
	void			SaveExternalFilters();
	void			LoadSettings(bool bForce = false);
	void			SaveShaders();
	void			LoadShaders();

	void			GetFav(favtype ft, CAtlList<CString>& sl);
	void			SetFav(favtype ft, CAtlList<CString>& sl);
	void			AddFav(favtype ft, CString s);
	CDVBChannel*	FindChannelByPref(int nPrefNumber);

	int				GetMultiInst();
	engine_t		GetFileEngine(CString path);

public:
	CFiltersPrioritySettings	FiltersPrioritySettings;

	CAtlList<CString>			slSubtitlePathsAddons;
	CAtlList<CString>			slAudioPathsAddons;
};
