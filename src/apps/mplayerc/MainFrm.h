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

#include "Misc.h"
#include "PlayerChildView.h"
#include "PlayerPreView.h"
#include "PlayerFlyBar.h"
#include "PlayerSeekBar.h"
#include "PlayerToolBar.h"
#include "PlayerInfoBar.h"
#include "PlayerStatusBar.h"
#include "PlayerSubresyncBar.h"
#include "PlayerPlaylistBar.h"
#include "PlayerCaptureBar.h"
#include "PlayerNavigationBar.h"
#include "PlayerShaderEditorBar.h"
#include "PPageSheet.h"
#include "PPageFileInfoSheet.h"
#include "OpenMediaData.h"
#include "FileDropTarget.h"
#include "KeyProvider.h"
#include "PlayerYouTube.h"
#include "SvgHelper.h"
#include "HistoryDlg.h"

#include "mplayerc.h"
#include "HistoryFile.h"

#include "DSUtil/DSMPropertyBag.h"
#include "DSUtil/FontInstaller.h"
#include "DSUtil/SimpleBuffer.h"
#include "SubPic/ISubPic.h"

#include "BaseGraph.h"
#include "ShockwaveGraph.h"

#include <IChapterInfo.h>
#include <IKeyFrameInfo.h>
#include <IBufferInfo.h>
#include <ID3DFullscreenControl.h>
#include <HighDPI.h>

#include "WebServer.h"
#include <d3d9.h>
#include <vmr9.h>
#include <evr.h>
#include <evr9.h>
#include <Il21dec.h>
#include "OSD.h"
#include "LcdSupport.h"
#include "MpcApi.h"
#include "filters/renderer/SyncClock/SyncClock.h"
#include "filters/transform/DecSSFilter/IfoFile.h"
#include "IDirectVobSub.h"
#include <ExtLib/ui/sizecbar/scbarg.h>
#include <afxinet.h>
#include "ColorControl.h"
#include "RateControl.h"
#include "DiskImage.h"
#include <filters/renderer/VideoRenderers/AllocatorCommon.h>
#include "MediaControls.h"
#include <limits>
#include <atomic>

#define USE_MEDIAINFO_STATIC
#include <MediaInfo/MediaInfo.h>
using namespace MediaInfoLib;

#include <filesystem>

class CFullscreenWnd;
struct ID3DFullscreenControl;
struct TunerScanData;
class CComPropertySheet;

enum PMODE {
	PM_NONE,
	PM_FILE,
	PM_DVD,
	PM_CAPTURE
};

interface __declspec(uuid("6E8D4A21-310C-11d0-B79A-00AA003767A7")) // IID_IAMLine21Decoder
IAMLine21Decoder_2 :
public IAMLine21Decoder {};

/*
class CKeyFrameFinderThread : public CWinThread, public CCritSec
{
	DECLARE_DYNCREATE(CKeyFrameFinderThread);

public:
	CKeyFrameFinderThread() {}

	CUIntArray m_kfs; // protected by (CCritSec*)this

	BOOL InitInstance();
	int ExitInstance();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnIndex(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBreak(WPARAM wParam, LPARAM lParam);
};
*/

interface ISubClock;

class CMainFrame : public CFrameWnd, public CDropTarget, public CDPI
{
	friend class CPPageFileInfoSheet;
	friend class CPPageLogo;
	friend class CChildView;
	friend class CThumbsTaskDlg;
	friend class CShaderEditorDlg;
	friend class CTextPassThruFilter;
	friend class CWebClientSocket;
	friend class CGraphThread;
	friend class CPPageSubtitles;
	friend class CPPageSoundProcessing;
	friend class CPPageInterface;
	friend class CMPlayerCApp;
	friend class CPlayerPlaylistBar;
	friend class CPPageSync;
	friend class CPPageVideo;
	friend class CMediaControls;

	// TODO: wrap these graph objects into a class to make it look cleaner

	CComPtr<IGraphBuilder2>         m_pGB;
	CComQIPtr<IMediaControl>        m_pMC;
	CComQIPtr<IMediaEventEx>        m_pME;

	CComQIPtr<IVideoWindow>         m_pVW;
	CComQIPtr<IBasicVideo>          m_pBV;
	CComQIPtr<IBasicAudio>          m_pBA;
	CComQIPtr<IMediaSeeking>        m_pMS;
	CComQIPtr<IVideoFrameStep>      m_pFS;
	CComQIPtr<IQualProp, &IID_IQualProp> m_pQP;
	CComQIPtr<IAMOpenProgress>      m_pAMOP;

	CComQIPtr<IDvdControl2>         m_pDVDC;
	CComQIPtr<IDvdInfo2>            m_pDVDI;

	// SmarkSeek
	CComPtr<IGraphBuilder2>         m_pGB_preview;
	CComQIPtr<IMediaControl>        m_pMC_preview;
	CComQIPtr<IMediaSeeking>        m_pMS_preview;
	CComQIPtr<IVideoWindow>         m_pVW_preview;
	CComQIPtr<IBasicVideo>          m_pBV_preview;
	CComQIPtr<IDvdControl2>         m_pDVDC_preview;
	CComQIPtr<IDvdInfo2>            m_pDVDI_preview; // VtX: usually not necessary but may sometimes be necessary.
	CComPtr<IMFVideoDisplayControl> m_pMFVDC_preview;
	CComPtr<IAllocatorPresenter>    m_pCAP_preview;
	//

	CComPtr<ICaptureGraphBuilder2>  m_pCGB;
	CStringW                        m_VidDispName, m_AudDispName;
	CComPtr<IBaseFilter>            m_pVidCap, m_pAudCap;
	CComPtr<IAMStreamConfig>        m_pAMVSCCap, m_pAMVSCPrev, m_pAMASC;
	CComPtr<IAMCrossbar>            m_pAMXBar;
	CComPtr<IAMTVTuner>             m_pAMTuner;
	CComPtr<IAMDroppedFrames>       m_pAMDF;

	CComPtr<IVMRMixerControl9>      m_pVMRMC9;
	CComPtr<IMFVideoDisplayControl> m_pMFVDC;
	CComPtr<IMFVideoProcessor>      m_pMFVP;
	CComPtr<IMFVideoMixerBitmap>    m_pMFVMB;

	CComPtr<IAMLine21Decoder_2>     m_pLN21;
	CComPtr<IVMRWindowlessControl9> m_pVMRWC;

	CComPtr<ID3DFullscreenControl>	m_pD3DFS;

	CComPtr<IPlaybackNotify>		m_pPlaybackNotify;

	CComPtr<IMadVRTextOsd>			m_pMVTO;

	CComPtr<IAllocatorPresenter>	m_pCAP;
	CLSID m_clsidCAP = GUID_NULL;

	CComPtr<IMadVRSubclassReplacement> m_pMVRSR;
	CComPtr<IMadVRSettings> m_pMVRS;
	CComPtr<IMadVRCommand> m_pMVRC;
	CComPtr<IMadVRInfo> m_pMVRI;
	CComPtr<IMadVRFrameGrabber> m_pMVRFG;

	CComQIPtr<IBaseFilter>			m_pMainSourceFilter;
	CComQIPtr<IBaseFilter>			m_pSwitcherFilter;
	CComQIPtr<IFileSourceFilter>	m_pMainFSF;
	CComQIPtr<IKeyFrameInfo>		m_pKFI;
	CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> m_pAMMC[2];

	CComQIPtr<IDirectVobSub>		m_pDVS;

	bool							m_bMainIsMPEGSplitter;

	// external fonts

	CFontInstaller m_FontInstaller;

	// subtitles

	CCritSec m_csSubLock;
	std::list<CComPtr<ISubStream>> m_pSubStreams;
	std::list<ISubStream*> m_ExternalSubstreams;
	int m_iSubtitleSel; // if (m_iSubtitleSel & 0x80000000) - disabled
	CComPtr<ISubStream> m_pCurrentSubStream;

	// windowing
	int m_iVideoSize;
	CRect m_lastWindowRect;
	CPoint m_lastMouseMove, m_lastMouseMoveFullScreen;

	CRect m_rcDesktop;

	void ShowControls(int nCS, bool fSave = true);
	void CalcControlsSize(CSize& cSize);

	bool m_bDelaySetOutputRect = false;
	void SetDefaultWindowRect(int iMonitor, const bool bLastCall);
	void SetDefaultFullscreenState();
	void RestoreDefaultWindowRect();
	void ZoomVideoWindow(bool snap = true, double scale = -1);
	double GetZoomAutoFitScale();
	CRect GetInvisibleBorderSize() const;
	void ClampWindowRect(RECT& windowRect);

	void SetAlwaysOnTop(int i);

	COLORREF m_colMenuBk;
	HBRUSH m_hMainMenuBrush = nullptr;
	HBRUSH m_hPopupMenuBrush = nullptr;

	COLORREF m_colTitleBk = {};
	const COLORREF m_colTitleBkSystem = 0xFFFFFFFF;

	CMenu m_popupMainMenu;
	CMenu m_popupMenu;
	CMenu m_openCDsMenu;
	CMenu m_recentfilesMenu;
	CMenu m_languageMenu;
	CMenu m_filtersMenu;
	CMenu m_shadersMenu;
	CMenu m_AudioMenu;
	CMenu m_SubtitlesMenu;
	CMenu m_VideoStreamsMenu;
	CMenu m_chaptersMenu;
	CMenu m_favoritesMenu;
	CMenu m_RButtonMenu;
	CMenu m_VideoFrameMenu;
	CMenu m_PanScanMenu;
	CMenu m_ShadersMenu;
	CMenu m_AfterPlaybackMenu;
	CMenu m_NavigateMenu;

	// dynamic menus
	void MakeEmptySubMenu(CMenu& menu);
	void SetupOpenCDSubMenu();
	void SetupFiltersSubMenu();
	void SetupAudioSubMenu();
	void SetupAudioRButtonMenu();
	void SetupSubtitlesSubMenu();
	void SetupSubtitlesRButtonMenu();
	void SetupAudioTracksSubMenu();
	void SetupSubtitleTracksSubMenu();
	void SetupVideoStreamsSubMenu();
	void SetupNavChaptersSubMenu();
	void SetupFavoritesSubMenu();
	void SetupRecentFilesSubMenu();
	void SetupLanguageMenu();

	IBaseFilter* FindSwitcherFilter();
	void SetupAMStreamSubMenu(CMenu& submenu, UINT id, DWORD dwSelGroup);
	void SelectAMStream(UINT id, DWORD dwSelGroup);

	#define SUBTITLE_GROUP 2
	void SetupSubtilesAMStreamSubMenu(CMenu& submenu, UINT id);
	void SelectSubtilesAMStream(UINT id);

	std::vector<CComQIPtr<IUnknown, &IID_IUnknown>> m_pparray;
	std::vector<CComQIPtr<IAMStreamSelect>> m_ssarray;

	struct Stream {
		int  Filter = 0;
		int  Index  = 0;
		int  Num    = -1;
		int  Sel    = -1;
		bool Ext    = false;
		bool forced = false;
		bool def    = false;
		CString Name;
	};

	BOOL SelectMatchTrack(const std::vector<Stream>& Tracks, CString pattern, const BOOL bExtPrior, size_t& nIdx, bool bAddForcedAndDefault = true);

	std::vector<Stream> m_SubtitlesStreams;
	void SubFlags(CString strname, bool& forced, bool& def);
	size_t GetSubSelIdx();
	int m_nInternalSubsCount;
	int m_nSelSub2;

	static constexpr size_t NO_SUBTITLES_INDEX = std::numeric_limits<UINT>::max();

	// chapters (file mode)
	CComPtr<IDSMChapterBag> m_pCB;
	ULONG m_chapterTitleNum = 0;
	void SetupChapters();

	//

	void AddTextPassThruFilter();

	int m_nLoops;
	REFERENCE_TIME m_abRepeatPositionA, m_abRepeatPositionB, m_fileEndPosition;
	bool m_abRepeatPositionAEnabled, m_abRepeatPositionBEnabled;

	bool m_bCustomGraph;
	bool m_bShockwaveGraph;

	CComPtr<ISubClock> m_pSubClock;

	bool m_bFrameSteppingActive;
	int m_nStepForwardCount;
	REFERENCE_TIME m_rtStepForwardStart;
	int m_nVolumeBeforeFrameStepping;

	bool m_bEndOfStream;
	bool m_bGraphEventComplete;

	bool m_bBuffering;

	bool m_bLiveWM;

	void SendStatusMessage(const CString& msg, const int nTimeOut);
	void SendStatusCompactPath(const CString& path);

	CString m_playingmsg, m_closingmsg;

	REFERENCE_TIME m_rtDurationOverride;

	CComPtr<IUnknown> m_pProv;

	void CleanGraph();

	void ShowOptions(int idPage = 0);

	HRESULT GetDisplayedImage(CSimpleBlock<BYTE>& dib, CString& errmsg);
	HRESULT GetCurrentFrame(CSimpleBlock<BYTE>& dib, CString& errmsg);
	HRESULT GetOriginalFrame(CSimpleBlock<BYTE>& dib, CString& errmsg);
	HRESULT RenderCurrentSubtitles(BYTE* pData);
	bool SaveDIB(LPCWSTR fn, BYTE* pData, long size);
	bool IsRendererCompatibleWithSaveImage();
	void SaveImage(LPCWSTR fn, bool displayed);
	void SaveThumbnails(LPCWSTR fn);
	//

	std::unique_ptr<CWebServer> m_pWebServer;
	PMODE m_ePlaybackMode;
	ULONG m_lCurrentChapter;
	ULONG m_lChapterStartTime;

	bool m_bWasPausedOnMinimizedVideo = false;

	bool m_bBeginCapture = false;
	CPoint m_beginCapturePoint;

public:
	enum {
		TIMER_STREAMPOSPOLLER = 1,
		TIMER_STREAMPOSPOLLER2,
		TIMER_FULLSCREENCONTROLBARHIDER,
		TIMER_MOUSEHIDER,
		TIMER_STATS,
		TIMER_STATUSERASER,
		TIMER_FLYBARWINDOWHIDER,
		TIMER_DM_AUTOCHANGING,
		TIMER_PAUSE
	};

	void SetColorMenu();
	void SetColorMenu(CMenu& menu);

	void SetColorTitle(const bool bSystemOnly = false);

	void StartWebServer(int nPort);
	void StopWebServer();

	PMODE GetPlaybackMode() const {
		return m_ePlaybackMode;
	}
	void SetPlaybackMode(PMODE eNewStatus);
	bool IsMuted() {
		return m_wndToolBar.GetVolume() == -10000;
	}
	int GetVolume() {
		return m_wndToolBar.m_volctrl.GetPos();
	}
	void SetBalance(int balance);

	HWND m_hWnd_toolbar;

public:
	CMainFrame();
	DECLARE_DYNAMIC(CMainFrame)

	// Attributes
public:
	bool m_bFullScreen;
	bool m_bFullScreenChangingMode;
	bool m_bFirstFSAfterLaunchOnFullScreen;
	bool m_bStartInD3DFullscreen;
	bool m_bHideCursor;

	CComPtr<IBaseFilter> m_pRefClock; // Adjustable reference clock. GothSync
	CComPtr<ISyncClock> m_pSyncClock;

	bool IsFrameLessWindow() const {
		return(m_bFullScreen || AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS);
	}
	bool IsCaptionHidden() const {//If no caption, there is no menu bar. But if is no menu bar, then the caption can be.
		return(!m_bFullScreen && AfxGetAppSettings().iCaptionMenuMode > MODE_HIDEMENU);//!=MODE_SHOWCAPTIONMENU && !=MODE_HIDEMENU
	}
	bool IsMainMenuVisible() const {
		return GetMenuBarVisibility() == AFX_MBV_KEEPVISIBLE;
	}
	bool IsSomethingLoaded() const {
		return((m_eMediaLoadState == MLS_LOADING || m_eMediaLoadState == MLS_LOADED) && !IsD3DFullScreenMode());
	}
	bool IsPlaylistEmpty() {
		return(m_wndPlaylistBar.GetCount() == 0);
	}
	bool IsInteractiveVideo() const {
		return(m_bShockwaveGraph);
	}

	bool IsD3DFullScreenMode() const;
	bool CursorOnD3DFullScreenWindow() const;
	bool CanShowDialog() const;
	void DestroyD3DWindow();

	CControlBar* m_pLastBar;

	static bool GetCurDispMode(dispmode& dm, const CString& DisplayName);
	static bool GetDispMode(const DWORD iModeNum, dispmode& dm, const CString& DisplayName);

	void UpdateWindowTitle();

private:
	void SetDispMode(const dispmode& dm, const CString& DisplayName, const BOOL bForceRegistryMode = FALSE);

	MPC_LOADSTATE	m_eMediaLoadState;
	bool			m_bClosingState;
	bool			m_bAudioOnly;

	BOOL			m_bNextIsOpened = FALSE;

	CString					m_LastOpenFile;
	std::unique_ptr<OpenMediaData> m_lastOMD;

	CString m_LastOpenBDPath, m_BDLabel;
	HMONITOR m_LastWindow_HM;

	double m_PlaybackRate;

	double m_ZoomX, m_ZoomY, m_PosX, m_PosY;
	int m_iDefRotation;

	SessionInfo m_SessionInfo;
	DVD_DOMAIN m_iDVDDomain;
	DWORD m_iDVDTitle = 0;
	DWORD m_iDVDTitleForHistory = 0;
	bool m_bDVDStillOn    = false;
	bool m_bDVDRestorePos = false;
	std::vector<CStringW> m_RecentPaths; // used in SetupRecentFilesSubMenu and OnRecentFile
	std::list<SessionInfo> m_FavFiles;   // used in SetupFavoritesSubMenu and OnFavoritesFile
	std::list<SessionInfo> m_FavDVDs;    // used in SetupFavoritesSubMenu and OnFavoritesDVD

	CStringW m_FileName;
	bool m_bUpdateTitle = false;
	void ClearPlaybackInfo()
	{
		m_SessionInfo.NewPath("");
		m_FileName.Empty();
		m_bUpdateTitle = false;
	}
	LPCWSTR GetFileNameOrTitleOrPath() {
		return
			m_FileName.GetLength() ? m_FileName.GetString() :
			m_SessionInfo.Title.GetLength() ? m_SessionInfo.Title.GetString() :
			m_SessionInfo.Path.GetString();
	}
	LPCWSTR GetTitleOrFileNameOrPath() {
		return
			m_SessionInfo.Title.GetLength() ? m_SessionInfo.Title.GetString() :
			m_FileName.GetLength() ? m_FileName.GetString() :
			m_SessionInfo.Path.GetString();
	}

	CString m_strPlaybackRenderedPath;

	bool m_bOpening = false;

	// Operations
	bool OpenMediaPrivate(std::unique_ptr<OpenMediaData>& pOMD);
	void CloseMediaPrivate();
	void DoTunerScan(TunerScanData* pTSD);

	CWnd *GetModalParent() { return this; }

	CString OpenCreateGraphObject(OpenMediaData* pOMD);
	CString OpenFile(OpenFileData* pOFD);
	CString OpenDVD(OpenDVDData* pODD);
	CString OpenCapture(OpenDeviceData* pODD);
	HRESULT OpenBDAGraph();
	void OpenCustomizeGraph();
	void OpenSetupVideo();
	void OpenSetupAudio();
	void OpenSetupAudioStream();
	void OpenSetupSubStream(OpenMediaData* pOMD);
	void OpenSetupInfoBar();
	void OpenSetupStatsBar();
	void OpenSetupStatusBar();
	// void OpenSetupToolBar();
	void OpenSetupCaptureBar();

	void AutoChangeMonitorMode();
	double m_dMediaInfoFPS;
	bool m_bNeedAutoChangeMonitorMode = false;

	bool GraphEventComplete();

	CGraphThread* m_pGraphThread;
	bool m_bOpenedThruThread;

	void LoadKeyFrames();
	std::vector<REFERENCE_TIME> m_kfs;

	bool m_fOpeningAborted;
	bool m_bWasSnapped;

	UINT	m_flastnID;
	bool	m_bfirstPlay;

	struct touchPoint {
		LONG  x_start = 0;
		LONG  y_start = 0;
		LONG  x_end   = 0;
		LONG  y_end   = 0;
		DWORD dwID    = DWORD_MAX;
	};

	struct touchScreen {
		#define MAXTOUCHPOINTS 10

		touchPoint point[MAXTOUCHPOINTS];
		bool moving = false;

		int FindById(const DWORD dwID) const {
			for (int i = 0; i < MAXTOUCHPOINTS; i++) {
				if (point[i].dwID == dwID) {
					return i;
				}
			}

			return -1;
		}

		const int Add(const DWORD dwID, const LONG x, const LONG y) {
			if (FindById(dwID) == -1) {
				for (int i = 0; i < MAXTOUCHPOINTS; i++) {
					if (point[i].dwID == DWORD_MAX) {
						point[i].dwID    = dwID;
						point[i].x_start = x;
						point[i].y_start = y;

						return i;
					}
				}
			}

			return -1;
		}

		const int Down(const DWORD dwID, const LONG x, const LONG y) {
			const int index = FindById(dwID);
			if (index != -1) {
				point[index].x_end = x;
				point[index].y_end = y;
			}

			return index;
		}

		const int Count() {
			int cnt = 0;
			for (int i = 0; i < MAXTOUCHPOINTS; i++) {
				if (point[i].dwID != DWORD_MAX) {
					cnt++;
				}
			}

			return cnt;
		}

		const int CountEnd() {
			int cnt = 0;
			for (int i = 0; i < MAXTOUCHPOINTS; i++) {
				if (point[i].dwID != DWORD_MAX && point[i].x_end != 0) {
					cnt++;
				}
			}

			return cnt;
		}

		const bool Delete(const DWORD dwID) {
			const int index = FindById(dwID);
			return Delete(index);
		}

		const bool Delete(const int index) {
			if (index >= 0 && index < MAXTOUCHPOINTS
					&& point[index].dwID != DWORD_MAX) {
				point[index].x_start = 0;
				point[index].y_start = 0;
				point[index].x_end   = 0;
				point[index].y_end   = 0;
				point[index].dwID    = DWORD_MAX;

				return true;
			}
			return false;
		}

		void Empty() {
			for (int i = 0; i < MAXTOUCHPOINTS; i++) {
				point[i].x_start = 0;
				point[i].y_start = 0;
				point[i].x_end   = 0;
				point[i].y_end   = 0;
				point[i].dwID    = DWORD_MAX;
			}

			moving = false;
		}
	};
	touchScreen m_touchScreen;

public:
	BOOL OpenCurPlaylistItem(REFERENCE_TIME rtStart = INVALID_TIME, BOOL bAddRecent = TRUE);
	BOOL OpenFile(const CString fname, REFERENCE_TIME rtStart = INVALID_TIME, BOOL bAddRecent = TRUE);
	void OpenMedia(std::unique_ptr<OpenMediaData> pOMD);
	void PlayFavoriteFile(SessionInfo fav);
	void PlayFavoriteDVD(SessionInfo fav);
	bool ResizeDevice();
	bool ResetDevice();
	bool DisplayChange();
	void CloseMedia(BOOL bNextIsOpened = FALSE);
	void StartTunerScan(std::unique_ptr<TunerScanData>& pTSD);
	void StopTunerScan();

	void AddCurDevToPlaylist();

	bool m_bTrayIcon;
	void ShowTrayIcon(bool fShow);
	void SetTrayTip(CString str);

	CSize GetVideoSize();

	COLORREF ColorBrightness(int lSkale, COLORREF color);

	void ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo);
	void ToggleD3DFullscreen(bool fSwitchScreenResWhenHasTo);

	void MoveVideoWindow(bool bShowStats = false, bool bForcedSetVideoRect = false);
	void SetPreviewVideoPosition();

	void RepaintVideo(const bool bForceRepaint = false);
	void HideVideoWindow(bool fHide);

	OAFilterState GetMediaState();
	REFERENCE_TIME GetPos(), GetDur();
	bool GetNeighbouringKeyFrames(REFERENCE_TIME rtTarget, std::pair<REFERENCE_TIME, REFERENCE_TIME>& keyframes) const;
	REFERENCE_TIME const GetClosestKeyFrame(REFERENCE_TIME rtTarget);
	void SeekTo(REFERENCE_TIME rt, bool bShowOSD = true);
	bool ValidateSeek(REFERENCE_TIME rtPos, REFERENCE_TIME rtStop);
	void MatroskaLoadKeyFrames();

	bool GetBufferingProgress(int* Progress = nullptr);

	void ApplySubpicSettings();
	void ApplyExraRendererSettings();

	// subtitle streams order function
	bool LoadSubtitle(const CExtraFileItem& subItem, ISubStream **actualStream = nullptr, bool bAutoLoad = true);

	void UpdateSubtitle(bool fDisplayMessage = false, bool fApplyDefStyle = false);
	void SetSubtitle(ISubStream* pSubStream, int iSubtitleSel = -1, bool fApplyDefStyle = false);
	void ReplaceSubtitle(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew);
	void InvalidateSubtitle(DWORD_PTR nSubtitleId = -1, REFERENCE_TIME rtInvalidate = -1);
	void ReloadSubtitle();
	void ToggleSubtitleOnOff(bool bDisplayMessage = false);

	void UpdateSubDefaultStyle();

	void SetAudioTrackIdx(int index);
	void SetSubtitleTrackIdx(int index);

	int  GetAudioTrackIdx();
	int  GetSubtitleTrackIdx();

	bool m_bToggleShader;
	bool m_bToggleShaderScreenSpace;
	std::list<ShaderC> m_ShaderCashe;
	ShaderC* GetShader(LPCWSTR label, bool bD3D11);
	bool SaveShaderFile(ShaderC* shader, bool bD3D11);
	bool DeleteShaderFile(LPCWSTR label, bool bD3D11);
	void TidyShaderCashe();
	void SetShaders();

	// capturing
	bool m_bCapturing;
	HRESULT BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt); // pBF: 0 buff, 1 enc, 2 mux, pmt is for 1 enc
	bool BuildToCapturePreviewPin(
		IBaseFilter* pVidCap, IPin** pVidCapPin, IPin** pVidPrevPin,
		IBaseFilter* pAudCap, IPin** pAudCapPin, IPin** pAudPrevPin);
	bool BuildGraphVideoAudio(int fVPreview, bool fVCapture, int fAPreview, bool fACapture);
	bool StartCapture();
	bool StopCapture();

	bool DoAfterPlaybackEvent();
	void ParseDirs(std::list<CString>& sl);
	int SearchInDir(const bool bForward);

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void RecalcLayout(BOOL bNotify = TRUE);

	BOOL OnTouchInput(CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput);

	LPCWSTR GetTextForBar(int style);
	void UpdateTitle();

	// Dvb capture
	void DisplayCurrentChannelOSD();
	void DisplayCurrentChannelInfo();

	bool CheckABRepeat(REFERENCE_TIME& aPos, REFERENCE_TIME& bPos, bool& aEnabled, bool& bEnabled);
	void PerformABRepeat();
	void DisableABRepeat();

	// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private: // control bar embedded members

	CChildView m_wndView;

	CPlayerSeekBar m_wndSeekBar;
	CPlayerInfoBar m_wndInfoBar;
	CPlayerInfoBar m_wndStatsBar;
	CPlayerStatusBar m_wndStatusBar;
	std::vector<CControlBar*> m_bars;

	CPlayerSubresyncBar m_wndSubresyncBar;
	CPlayerCaptureBar m_wndCaptureBar;
	CPlayerNavigationBar m_wndNavigationBar;
	CPlayerShaderEditorBar m_wndShaderEditorBar;
	std::vector<CSizingControlBar*> m_dockingbars;

	std::vector<CSizingControlBar*> m_dockingbarsVisible;

	std::unique_ptr<CHistoryDlg> m_pHistoryDlg;

	CFileDropTarget m_fileDropTarget;
	// TODO
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	DROPEFFECT OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	void OnDragLeave();
	DROPEFFECT OnDragScroll(DWORD dwKeyState, CPoint point);

	const UINT CF_URLA = RegisterClipboardFormatW(CFSTR_INETURLA);
	const UINT CF_URLW = RegisterClipboardFormatW(CFSTR_INETURLW);
	void DropFiles(std::list<CString>& slFiles);

	void RestoreControlBars();
	void SaveControlBars();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();

	afx_msg LRESULT OnTaskBarRestart(WPARAM, LPARAM);
	afx_msg LRESULT OnNotifyIcon(WPARAM, LPARAM);
	afx_msg LRESULT OnTaskBarThumbnailsCreate(WPARAM, LPARAM);
	afx_msg LRESULT OnQueryCancelAutoPlay(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDwmSendIconicThumbnail(WPARAM, LPARAM);
	afx_msg LRESULT OnDwmSendIconicLivePreviewBitmap(WPARAM, LPARAM);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT nSide, LPRECT pRect);
	afx_msg void OnDisplayChange();
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnQueryEndSession();

	LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnDwmCompositionChanged(WPARAM wParam, LPARAM lParam);

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg LRESULT OnAppCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnRawInput(UINT nInputcode, HRAWINPUT hRawInput);

	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResizeDevice(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetDevice(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnPostOpen(WPARAM wParam, LPARAM lParam);

	afx_msg void SaveAppSettings();

	BOOL MouseMessage(UINT id, UINT nFlags, CPoint point);

	afx_msg void OnEnterSizeMove();
	afx_msg void ClipRectToMonitor(LPRECT prc);
	int snap_Margin, snap_x, snap_y;
	BOOL isSnapClose(int a, int b);
	BOOL m_bWndZoomed;
	RECT rc_forceNP;

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnXButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXButtonUp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnUnInitMenuPopup(CMenu* pPopupMenu, UINT nFlags);

	BOOL OnMenu(CMenu* pMenu);
	afx_msg void OnMenuPlayerShort();
	afx_msg void OnMenuPlayerLong();
	afx_msg void OnMenuFilters();

	afx_msg void OnMenuNavAudio();
	afx_msg void OnMenuNavSubtitle();
	afx_msg void OnMenuNavAudioOptions();
	afx_msg void OnMenuNavSubtitleOptions();
	afx_msg void OnMenuNavJumpTo();
	afx_msg void OnMenuRecentFiles();
	afx_msg void OnMenuFavorites();
	afx_msg void OnUpdateMenuNavSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuNavAudio(CCmdUI* pCmdUI);
	afx_msg void OnMenuAfterPlayback();

	afx_msg void OnUpdatePlayerStatus(CCmdUI* pCmdUI);

	afx_msg void OnBossKey();

	afx_msg void OnStreamAudio(UINT nID);
	afx_msg void OnStreamSub(UINT nID);
	afx_msg void OnStreamSubOnOff();
	afx_msg void OnStreamVideo(UINT nID);

	// menu item handlers

	afx_msg void OnFileOpenQuick();
	afx_msg void OnFileOpenMedia();
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);

	LRESULT OnMPCVRSwitchFullscreen(WPARAM wParam, LPARAM lParam);
	LRESULT OnDXVAStateChange(WPARAM wParam, LPARAM lParam);

private:
	std::mutex m_mutex_cmdLineQueue;
	std::deque<std::vector<BYTE>> m_cmdLineQueue;
	LRESULT HandleCmdLine(WPARAM wParam, LPARAM lParam);
	CAMEvent m_EventCmdLineQueue;
	CAMEvent m_ExitCmdLineQueue;
	std::thread cmdLineThread;
	void cmdLineThreadFunction();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);

	CMPC_Lcd m_Lcd;

	LRESULT OnRestore(WPARAM wParam, LPARAM lParam);

public:
	afx_msg void OnFileOpenDVD();
	afx_msg void OnFileOpenIso();
	afx_msg void OnFileOpenDevice();
	afx_msg void OnFileOpenCD(UINT nID);
	afx_msg void OnFileReOpen();
	afx_msg void OnFileSaveAs();
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveImage();
	afx_msg void OnUpdateFileSaveImage(CCmdUI* pCmdUI);
	afx_msg void OnAutoSaveImage();
	afx_msg void OnAutoSaveDisplay();
	afx_msg void OnCopyImage();
	afx_msg void OnFileSaveThumbnails();
	afx_msg void OnUpdateFileSaveThumbnails(CCmdUI* pCmdUI);
	afx_msg void OnFileLoadSubtitle();
	afx_msg void OnUpdateFileLoadSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveSubtitle();
	afx_msg void OnUpdateFileSaveSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnFileLoadAudio();
	afx_msg void OnUpdateFileLoadAudio(CCmdUI* pCmdUI);
	afx_msg void OnFileProperties();
	afx_msg void OnUpdateFileProperties(CCmdUI* pCmdUI);
	afx_msg void OnFileClosePlaylist();
	afx_msg void OnFileCloseMedia(); // no menu item
	afx_msg void OnUpdateFileClose(CCmdUI* pCmdUI);
	afx_msg void OnRepeatForever();
	afx_msg void OnUpdateRepeatForever(CCmdUI* pCmdUI);
	afx_msg void OnABRepeat(UINT nID);
	afx_msg void OnUpdateABRepeat(CCmdUI* pCmdUI);

	afx_msg void OnViewCaptionmenu();
	afx_msg void OnViewNavigation();
	afx_msg void OnUpdateViewCaptionmenu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewNavigation(CCmdUI* pCmdUI);
	afx_msg void OnViewControlBar(UINT nID);
	afx_msg void OnUpdateViewControlBar(CCmdUI* pCmdUI);
	afx_msg void OnViewSubresync();
	afx_msg void OnUpdateViewSubresync(CCmdUI* pCmdUI);
	afx_msg void OnViewPlaylist();
	afx_msg void OnUpdateViewPlaylist(CCmdUI* pCmdUI);
	afx_msg void OnViewCapture();
	afx_msg void OnUpdateViewCapture(CCmdUI* pCmdUI);
	afx_msg void OnViewShaderEditor();
	afx_msg void OnUpdateViewShaderEditor(CCmdUI* pCmdUI);
	afx_msg void OnViewMinimal();
	afx_msg void OnViewCompact();
	afx_msg void OnViewNormal();
	afx_msg void OnViewFullscreen();
	afx_msg void OnViewFullscreenSecondary();
	afx_msg void OnMoveWindowToPrimaryScreen();

	void ResetMenu();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	afx_msg void OnViewZoom(UINT nID);
	afx_msg void OnUpdateViewZoom(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomAutoFit();
	afx_msg void OnViewDefaultVideoFrame(UINT nID);
	afx_msg void OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI);
	afx_msg void OnViewSwitchVideoFrame();
	afx_msg void OnViewKeepaspectratio();
	afx_msg void OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI);
	afx_msg void OnViewCompMonDeskARDiff();
	afx_msg void OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScan(UINT nID);
	afx_msg void OnUpdateViewPanNScan(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScanPresets(UINT nID);
	afx_msg void OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI);
	afx_msg void OnViewRotate(UINT nID);
	afx_msg void OnViewFlip();
	afx_msg void OnViewAspectRatio(UINT nID);
	afx_msg void OnUpdateViewAspectRatio(CCmdUI* pCmdUI);
	afx_msg void OnViewAspectRatioNext();
	afx_msg void OnUpdateViewStereo3DMode(CCmdUI* pCmdUI);
	afx_msg void OnViewStereo3DMode(UINT nID);
	afx_msg void OnUpdateViewSwapLeftRight(CCmdUI* pCmdUI);
	afx_msg void OnViewSwapLeftRight();
	afx_msg void OnViewOntop(UINT nID);
	afx_msg void OnUpdateViewOntop(CCmdUI* pCmdUI);
	afx_msg void OnViewOptions();
	afx_msg void OnUpdateViewTearingTest(CCmdUI* pCmdUI);
	afx_msg void OnViewTearingTest();
	afx_msg void OnUpdateViewDisplayStats(CCmdUI* pCmdUI);
	afx_msg void OnViewResetStats();
	afx_msg void OnViewDisplayStatsSC();

	afx_msg void OnUpdateViewD3DFullscreen(CCmdUI* pCmdUI);

	afx_msg void OnUpdateViewEnableFrameTimeCorrection(CCmdUI* pCmdUI);
	afx_msg void OnViewVSync();
	afx_msg void OnViewVSyncInternal();

	afx_msg void OnViewD3DFullScreen();
	afx_msg void OnViewResetDefault();
	afx_msg void OnUpdateViewResetDefault(CCmdUI* pCmdUI);

	afx_msg void OnViewEnableFrameTimeCorrection();
	afx_msg void OnViewVSyncOffsetIncrease();
	afx_msg void OnViewVSyncOffsetDecrease();
	afx_msg void OnUpdateShaderToggle1(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShaderToggle2(CCmdUI* pCmdUI);
	afx_msg void OnShaderToggle1();
	afx_msg void OnShaderToggle2();
	afx_msg void OnViewRemainingTime();

	afx_msg void OnUpdateViewOSDLocalTime(CCmdUI* pCmdUI);
	afx_msg void OnViewOSDLocalTime();
	afx_msg void OnUpdateViewOSDFileName(CCmdUI* pCmdUI);
	afx_msg void OnViewOSDFileName();

	afx_msg void OnD3DFullscreenToggle();
	afx_msg void OnGotoSubtitle(UINT nID);
	afx_msg void OnShiftSubtitle(UINT nID);
	afx_msg void OnSubtitleDelay(UINT nID);

	afx_msg void OnPlayPlay();
	afx_msg void OnPlayPause();
	afx_msg void OnPlayPlayPause();
	afx_msg void OnPlayStop();
	afx_msg void OnUpdatePlayPauseStop(CCmdUI* pCmdUI);
	afx_msg void OnPlayFrameStep(UINT nID);
	afx_msg void OnUpdatePlayFrameStep(CCmdUI* pCmdUI);
	afx_msg void OnPlaySeek(UINT nID);
	afx_msg void OnPlaySeekBegin();
	afx_msg void OnPlaySeekKey(UINT nID); // no menu item
	afx_msg void OnUpdatePlaySeek(CCmdUI* pCmdUI);
	afx_msg void OnPlayGoto();
	afx_msg void OnUpdateGoto(CCmdUI* pCmdUI);
	afx_msg void SetPlayingRate(double rate);
	afx_msg void OnPlayChangeRate(UINT nID);
	afx_msg void OnUpdatePlayChangeRate(CCmdUI* pCmdUI);
	afx_msg void OnPlayResetRate();
	afx_msg void OnUpdatePlayResetRate(CCmdUI* pCmdUI);
	afx_msg void OnPlayChangeAudDelay(UINT nID);
	afx_msg void OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI);
	afx_msg void OnPlayFiltersCopyToClipboard();
	afx_msg void OnPlayFilters(UINT nID);
	afx_msg void OnUpdatePlayFilters(CCmdUI* pCmdUI);
	afx_msg void OnSelectShaders();

	afx_msg void OnMenuAudioOption();
	afx_msg void OnMenuSubtitlesOption();
	afx_msg void OnMenuSubtitlesEnable();
	afx_msg void OnUpdateSubtitlesEnable(CCmdUI* pCmdUI);
	afx_msg void OnMenuSubtitlesStyle();
	afx_msg void OnUpdateSubtitlesStyle(CCmdUI* pCmdUI);
	afx_msg void OnMenuSubtitlesReload();
	afx_msg void OnUpdateSubtitlesReload(CCmdUI* pCmdUI);
	afx_msg void OnMenuSubtitlesDefStyle();
	afx_msg void OnUpdateSubtitlesDefStyle(CCmdUI* pCmdUI);
	afx_msg void OnMenuSubtitlesForcedOnly();
	afx_msg void OnUpdateSubtitlesForcedOnly(CCmdUI* pCmdUI);
	afx_msg void OnStereoSubtitles(UINT nID);

	afx_msg void OnUpdateNavMixSubtitles(CCmdUI* pCmdUI);

	afx_msg void OnSelectStream(UINT nID);
	afx_msg void OnPlayVolume(UINT nID);
	afx_msg void OnPlayVolumeGain(UINT nID);
	afx_msg void OnAutoVolumeControl();
	afx_msg void OnPlayCenterLevel(UINT nID);
	afx_msg void OnPlayColor(UINT nID);
	afx_msg void OnAfterplayback(UINT nID);
	afx_msg void OnUpdateAfterplayback(CCmdUI* pCmdUI);

	afx_msg void OnNavigateSkip(UINT nID);
	afx_msg void OnUpdateNavigateSkip(CCmdUI* pCmdUI);
	afx_msg void OnNavigateSkipFile(UINT nID);
	afx_msg void OnUpdateNavigateSkipFile(CCmdUI* pCmdUI);
	afx_msg void OnNavigateMenu(UINT nID);
	afx_msg void OnUpdateNavigateMenu(CCmdUI* pCmdUI);
	afx_msg void OnNavigateAudio(UINT nID);
	afx_msg void OnNavigateSubpic(UINT nID);
	afx_msg void OnNavigateAngle(UINT nID);
	afx_msg void OnNavigateChapters(UINT nID);
	afx_msg void OnNavigateMenuItem(UINT nID);
	afx_msg void OnUpdateNavigateMenuItem(CCmdUI* pCmdUI);
	afx_msg void OnTunerScan();
	afx_msg void OnUpdateTunerScan(CCmdUI* pCmdUI);

	afx_msg void OnFavoritesAdd();
	afx_msg void OnUpdateFavoritesAdd(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesQuickAdd();
	afx_msg void OnFavoritesOrganize();
	afx_msg void OnFavoritesFile(UINT nID);
	afx_msg void OnFavoritesDVD(UINT nID);
	afx_msg void OnShowHistory();
	afx_msg void OnRecentFileClear();
	afx_msg void OnUpdateRecentFileClear(CCmdUI* pCmdUI);
	afx_msg void OnRecentFile(UINT nID);

	afx_msg void OnHelpHomepage();
	afx_msg void OnHelpCheckForUpdate();
	//afx_msg void OnHelpDocumentation();
	afx_msg void OnHelpToolbarImages();
	//afx_msg void OnHelpDonate();

	// Subtitle position
	afx_msg void OnSubtitlePos(UINT nID);
	afx_msg void OnSubtitleSize(UINT nID);

	afx_msg void OnSubCopyClipboard();

	afx_msg void OnAddToPlaylistFromClipboard();

	afx_msg void OnChangeMouseEasyMove();

	afx_msg void OnPlaylistOpenFolder();

	afx_msg void OnClose();

	afx_msg void OnLanguage(UINT nID);

	CString UpdatePlayerStatus();

	void OnFilePostOpenMedia(std::unique_ptr<OpenMediaData>& pOMD);
	void OnFilePostCloseMedia();

	// Main Window
	CWnd*			m_pVideoWnd;
	CWnd*			m_pOSDWnd;

	CMPCGradient m_BackGroundGradient; // used in some toolbars

	CPlayerToolBar		m_wndToolBar;
	CPlayerPlaylistBar	m_wndPlaylistBar;
	CFlyBar				m_wndFlyBar;
	CPreView			m_wndPreView; // SmartSeek
	bool				m_bWndPreViewOn = false;

	bool m_bIsMadVRExclusiveMode = false;
	bool m_bIsMPCVRExclusiveMode = false;

	void CreateFlyBar();
	bool FlyBarSetPos();
	void DestroyFlyBar();

	void CreateOSDBar();
	bool OSDBarSetPos();
	void DestroyOSDBar();

	void ReleasePreviewGraph();
	HRESULT PreviewWindowHide();
	HRESULT PreviewWindowShow(REFERENCE_TIME rtCur2);
	bool CanPreviewUse();
	bool TogglePreview();

	CFullscreenWnd* m_pFullscreenWnd;

	CSvgImage  m_svgFlybar;
	CSvgImage  m_svgTaskbarButtons;
	CImageList m_pTaskbarStateIconsImages;
	COSD       m_OSD;

	CString		GetSystemLocalTime();
	int			m_nCurSubtitle;
	long		m_lSubtitleShift;
	__int64		m_rtCurSubPos;

	Youtube::YoutubeFields m_youtubeFields;
	Youtube::YoutubeUrllist m_youtubeUrllist;
	Youtube::YoutubeUrllist m_youtubeAudioUrllist;
	std::vector<uint8_t> m_youtubeThumbnailData;
	bool m_bYoutubeOpened = false;
	std::atomic_bool m_bYoutubeOpening = false;

	const CString GetAltFileName();

	bool		m_bInOptions;
	bool		m_bStopTunerScan;

	MPC_LOADSTATE GetLoadState() { return m_eMediaLoadState; }
	void		SetLoadState(MPC_LOADSTATE iState);
	void		SetPlayState(MPC_PLAYSTATE iState);
	BOOL		CreateFullScreenWindow();
	void		SetupEVRColorControl();
	void		SetupVMR9ColorControl();
	void		SetColorControl(DWORD flags, int& brightness, int& contrast, int& hue, int& saturation);
	void		SetClosedCaptions(bool enable);
	LPCWSTR		GetDVDAudioFormatName(DVD_AudioAttributes& ATR) const;
	void		SetAudioDelay(REFERENCE_TIME rtShift);
	void		SetSubtitleDelay(int delay_ms);

	void		SetTimersPlay();
	void		KillTimersStop();

	// MPC API functions
	void		ProcessAPICommand(COPYDATASTRUCT* pCDS);
	void		SendAPICommand(MPCAPI_COMMAND nCommand, LPCWSTR fmt, ...);
	void		SendNowPlayingToApi();
	void		SendSubtitleTracksToApi();
	void		SendAudioTracksToApi();
	void		SendPlaylistToApi();
	afx_msg void OnFileOpenDirectory();

	void		SendCurrentPositionToApi(bool fNotifySeek = false);
	void		ShowOSDCustomMessageApi(MPC_OSDDATA *osdData);
	void		JumpOfNSeconds(int seconds);

	// Win 7 TaskBar/Thumbnail support
	ITaskbarList3* m_pTaskbarList;
	HRESULT		CreateThumbnailToolbar();
	HRESULT		UpdateThumbarButton();
	HRESULT		UpdateThumbnailClip();

	HRESULT		(__stdcall * m_pfnDwmSetIconicThumbnail)(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags);
	HRESULT		(__stdcall * m_pfnDwmSetIconicLivePreviewBitmap)(HWND hwnd, HBITMAP hbmp, __in_opt POINT *pptClient, DWORD dwSITFlags);
	HRESULT		(__stdcall * m_pfnDwmInvalidateIconicBitmaps)(HWND hwnd);
	BOOL		m_bDesktopCompositionEnabled = TRUE;

	HBITMAP		m_CaptureWndBitmap;
	bool		m_isWindowMinimized = false;

	HBITMAP		CreateCaptureDIB(const int x, const int y, const int w, const int h);
	void		CreateCaptureWindow();

	HRESULT		SetAudioPicture(BOOL show = TRUE);
	CComPtr<IWICBitmap> m_pMainBitmap;
	CComPtr<IWICBitmap> m_pMainBitmapSmall;

	HBITMAP		m_ThumbCashedBitmap;
	CSize		m_ThumbCashedSize;

	void		AddFavorite(bool bDisplayMessage = false, bool bShowDialog = true);

private:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	void WTSRegisterSessionNotification();
	void WTSUnRegisterSessionNotification();

	CString m_OldMessage;
	void SetStatusMessage(const CString& msg);
	CString FillMessage();

	bool m_bValidDVDOpen;

	CComPtr<IBaseFilter> m_pBFmadVR;

	HMODULE m_hWtsLib;

	struct filepathtime_t {
		CStringW path;
		std::filesystem::file_time_type time;
		filepathtime_t(const CStringW& _path, std::filesystem::file_time_type _time)
			: path(_path)
			, time(_time)
		{}
	};
	std::vector<filepathtime_t> m_ExtSubFiles;
	std::vector<CStringW> m_ExtSubPaths;

	void subChangeNotifyThreadStart();
	void subChangeNotifySetupThread(std::vector<HANDLE>& handles);

	CAMEvent    m_EventSubChangeStopNotify;
	CAMEvent    m_EventSubChangeRefreshNotify;
	std::thread subChangeNotifyThread;
	void        subChangeNotifyThreadFunction();

	::CEvent m_hGraphThreadEventOpen;
	::CEvent m_hGraphThreadEventClose;

public:
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);
	afx_msg void OnSessionChange(UINT nSessionState, UINT nId);
	afx_msg void OnSettingChange(UINT, LPCTSTR);

	void EnableShaders1(bool enable);
	void EnableShaders2(bool enable);

	CHdmvClipInfo::CPlaylist m_BDPlaylists;
	BOOL m_bIsBDPlay;
	BOOL OpenBD(const CString& path, REFERENCE_TIME rtStart, BOOL bAddRecent);

	// TRUE if the file name is "index.bdmv"
	BOOL IsBDStartFile(const CString& path);
	BOOL IsBDPlsFile(const CString& path);
	// BD path can be supplemented with "index.bdmv" if necessary
	BOOL CheckBD(CString& path);

	// TRUE if the file name is "VIDEO_TS.IFO"
	BOOL IsDVDStartFile(const CString& path);
	// DVD path can be supplemented with "VIDEO_TS.IFO" if necessary
	BOOL CheckDVD(CString& path);

private:
	int			GetStreamCount(DWORD dwSelGroup);

	BOOL		m_bLeftMouseDown			= FALSE;
	BOOL		m_bLeftMouseDownFullScreen	= FALSE;
	bool		m_bWaitingRButtonUp = false;

	int			m_nAudioTrackStored    = -1;
	int			m_nSubtitleTrackStored = -1;
	bool		m_bRememberSelectedTracks = true;

	bool		m_bUseReclock = false;

public:
	BOOL		CheckMainFilter(IBaseFilter* pBF);

	void		AddSubtitlePathsAddons(LPCWSTR FileName);
	void		AddAudioPathsAddons(LPCWSTR FileName);

	void		MakeBDLabel(CString path, CString& label, CString* pBDlabel = nullptr);
	void		MakeDVDLabel(CString path, CString& label, CString* pDVDlabel = nullptr);

	CString		GetCurFileName();
	CString		GetCurDVDPath(BOOL bSelectCurTitle = FALSE);

	GUID		GetTimeFormat();

	CColorControl	m_ColorControl;

	void		StopAutoHideCursor();
	void		StartAutoHideCursor();

	const bool	GetFromClipboard(std::list<CString>& sl) const;

	void		ShowControlBarInternal(CControlBar* pBar, BOOL bShow);

	void		SetColor();

private:
	CDiskImage	m_DiskImage;
	BOOL		m_bNeedUnmountImage = TRUE;
	BOOL		OpenIso(const CString& pathName, REFERENCE_TIME rtStart = INVALID_TIME);

	void		AddRecent(const CString& pathName);

	CStringW	GetVidPos();
	CStringW	CreateSnapShotFileName();

	REFTIME		GetAvgTimePerFrame(BOOL bUsePCAP = TRUE) const;

	BOOL		OpenYoutubePlaylist(const CString& url, BOOL bOnlyParse = FALSE);

	BOOL		AddSimilarFiles(std::list<CString>& fns);

	void		SetToolBarAudioButton();
	void		SetToolBarSubtitleButton();

	bool		CanSwitchD3DFS();

	bool		m_bAltDownClean = false;

	CComPropertySheet* m_pFilterPropSheet = nullptr;

	static inline CMainFrame* m_pThis = nullptr;
	static inline HHOOK m_MenuHook = nullptr;
	static LRESULT CALLBACK MenuHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	bool m_bInMenu = false;

	bool IsNavigateSkipEnabled();

	CMediaControls m_CMediaControls;

	bool m_bIsLiveOnline = false;

	void SaveHistory();
};
