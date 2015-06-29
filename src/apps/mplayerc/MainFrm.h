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

#pragma once

#include <atlbase.h>

#include "PngImage.h"
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
#include "EditListEditor.h"
#include "PPageSheet.h"
#include "PPageFileInfoSheet.h"
#include "FileDropTarget.h"
#include "KeyProvider.h"

#include "../../DSUtil/DSMPropertyBag.h"
#include "../../SubPic/ISubPic.h"

#include "BaseGraph.h"
#include "ShockwaveGraph.h"

#include <IChapterInfo.h>
#include <IKeyFrameInfo.h>
#include <IBufferInfo.h>

#include "WebServer.h"
#include <d3d9.h>
#include <vmr9.h>
#include <evr.h>
#include <evr9.h>
#include <Il21dec.h>
#include "OSD.h"
#include "LcdSupport.h"
#include "MpcApi.h"
#include "../../filters/renderer/SyncClock/SyncClock.h"
#include "../../filters/transform/DecSSFilter/IfoFile.h"
#include "../../filters/transform/VSFilter/IDirectVobSub.h"
#include <sizecbar/scbarg.h>
#include <afxinet.h>
#include <afxmt.h>
#include "ColorControl.h"
#include "RateControl.h"
#include "DiskImage.h"

#define USE_MEDIAINFO_STATIC
#include <MediaInfo/MediaInfo.h>
using namespace MediaInfoLib;

class CFullscreenWnd;
struct ID3DFullscreenControl;

enum PMODE {
	PM_NONE,
	PM_FILE,
	PM_DVD,
	PM_CAPTURE
};

interface __declspec(uuid("6E8D4A21-310C-11d0-B79A-00AA003767A7")) // IID_IAMLine21Decoder
IAMLine21Decoder_2 :
public IAMLine21Decoder {};

class OpenMediaData
{
public:
	//	OpenMediaData() {}
	virtual ~OpenMediaData() {} // one virtual funct is needed to enable rtti
	CString title;
	CSubtitleItemList subs;

	BOOL bAddRecent = TRUE;
};

class OpenFileData : public OpenMediaData
{
public:
	OpenFileData() : rtStart(INVALID_TIME) {}
	CFileItemList fns;
	REFERENCE_TIME rtStart;
};

class OpenDVDData : public OpenMediaData
{
public:
	// OpenDVDData() {}
	CString path;
	CComPtr<IDvdState> pDvdState;
};

class OpenDeviceData : public OpenMediaData
{
public:
	OpenDeviceData() {
		vinput = vchannel = ainput = -1;
	}
	CStringW DisplayName[2];
	int vinput, vchannel, ainput;
};

class TunerScanData
{
public :
	ULONG	FrequencyStart;
	ULONG	FrequencyStop;
	ULONG	Bandwidth;
	LONG	Offset;
	HWND	Hwnd;
};

class CMainFrame;

class CGraphThread : public CWinThread
{
	CMainFrame* m_pMainFrame;

	DECLARE_DYNCREATE(CGraphThread);

public:
	CGraphThread() : m_pMainFrame(NULL) {}

	void SetMainFrame(CMainFrame* pMainFrame) {
		m_pMainFrame = pMainFrame;
	}

	BOOL InitInstance();
	int ExitInstance();

	enum {
		TM_EXIT = WM_APP,
		TM_OPEN,
		TM_CLOSE,
		TM_RESET,
		TM_TUNER_SCAN,
		TM_DISPLAY_CHANGE
	};
	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnOpen(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose(WPARAM wParam, LPARAM lParam);
	afx_msg void OnReset(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTunerScan(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDisplayChange(WPARAM wParam, LPARAM lParam);
};

/*
class CKeyFrameFinderThread : public CWinThread, public CCritSec
{
	DECLARE_DYNCREATE(CKeyFrameFinderThread);

public:
	CKeyFrameFinderThread() {}

	CUIntArray m_kfs; // protected by (CCritSec*)this

	BOOL InitInstance();
	int ExitInstance();

	enum {TM_EXIT=WM_APP, TM_INDEX, TM_BREAK};
	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnIndex(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBreak(WPARAM wParam, LPARAM lParam);
};
*/

interface ISubClock;

class CMainFrame : public CFrameWnd, public CDropTarget
{
	enum {
		TIMER_STREAMPOSPOLLER = 1,
		TIMER_STREAMPOSPOLLER2,
		TIMER_FULLSCREENCONTROLBARHIDER,
		TIMER_FULLSCREENMOUSEHIDER,
		TIMER_STATS,
		TIMER_LEFTCLICK,
		TIMER_STATUSERASER,
		TIMER_FLYBARWINDOWHIDER
	};

	friend class CPPageFileInfoSheet;
	friend class CPPageLogo;
	friend class CSubtitleDlDlg;
	friend class CFullscreenWnd;
	friend class COSD;
	friend class CChildView;

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
	CComQIPtr<IBufferInfo>          m_pBI;
	CComQIPtr<IAMOpenProgress>      m_pAMOP;

	CComQIPtr<IDvdControl2>         m_pDVDC;
	CComQIPtr<IDvdInfo2>            m_pDVDI;

	// SmarkSeek
	CComPtr<IGraphBuilder2>         m_pGB_preview;
	CComQIPtr<IMediaControl>        m_pMC_preview;
	CComQIPtr<IMediaEventEx>        m_pME_preview;
	CComQIPtr<IMediaSeeking>        m_pMS_preview;
	CComQIPtr<IVideoWindow>         m_pVW_preview;
	CComQIPtr<IBasicVideo>          m_pBV_preview;
	CComQIPtr<IVideoFrameStep>      m_pFS_preview;
	CComQIPtr<IDvdControl2>         m_pDVDC_preview;
	CComQIPtr<IDvdInfo2>            m_pDVDI_preview; // VtX: usually not necessary but may sometimes be necessary.
	CComPtr<IMFVideoDisplayControl> m_pMFVDC_preview;
	CComPtr<IMFVideoProcessor>      m_pMFVP_preview;
	//

	CComPtr<ICaptureGraphBuilder2>  pCGB;
	CStringW                        m_VidDispName, m_AudDispName;
	CComPtr<IBaseFilter>            pVidCap, pAudCap;
	CComPtr<IAMVideoCompression>    pAMVCCap, pAMVCPrev;
	CComPtr<IAMStreamConfig>        pAMVSCCap, pAMVSCPrev, pAMASC;
	CComPtr<IAMCrossbar>            pAMXBar;
	CComPtr<IAMTVTuner>             pAMTuner;
	CComPtr<IAMDroppedFrames>       pAMDF;

	CComPtr<IVMRMixerControl9>      m_pVMRMC9;
	CComPtr<IMFVideoDisplayControl> m_pMFVDC;
	CComPtr<IMFVideoProcessor>      m_pMFVP;
	CComPtr<IAMLine21Decoder_2>     m_pLN21;
	CComPtr<IVMRWindowlessControl9> m_pVMRWC;

	CComPtr<ID3DFullscreenControl>	m_pD3DFS;

	CComPtr<IVMRMixerBitmap9>		m_pVMB;
	CComPtr<IMadVRTextOsd>			m_pMVTO;


	CComPtr<ISubPicAllocatorPresenter>  m_pCAP;
	CComPtr<ISubPicAllocatorPresenter2> m_pCAP2;

	CComPtr<IMadVRSubclassReplacement> m_pMVRSR;

	CComQIPtr<IBaseFilter>			m_pMainSourceFilter;
	CComQIPtr<IBaseFilter>			m_pSwitcherFilter;
	CComQIPtr<IFileSourceFilter>	m_pMainFSF;
	CComQIPtr<IKeyFrameInfo>		m_pKFI;

	CComQIPtr<IDirectVobSub>		m_pDVS;

	void EnableAutoVolumeControl(bool enable);
	void SetBalance(int balance);

	// subtitles

	CCritSec m_csSubLock;
	CInterfaceList<ISubStream> m_pSubStreams;
	int m_iSubtitleSel; // if (m_iSubtitleSel & (1 << 31)): disabled ...
	CComPtr<ISubStream> m_pCurrentSubStream;

	friend class CTextPassThruFilter;

	// windowing

	CRect m_lastWindowRect;
	CPoint m_lastMouseMove, m_lastMouseMoveFullScreen;

	CRect m_rcDesktop;

	void ShowControls(int nCS, bool fSave = true);
	void CalcControlsSize(CSize& cSize);

	bool m_bDelaySetOutputRect = false;
	void SetDefaultWindowRect(int iMonitor = 0);
	void SetDefaultFullscreenState();
	void RestoreDefaultWindowRect();
	void ZoomVideoWindow(bool snap = true, double scale = -1);
	double GetZoomAutoFitScale();

	void SetAlwaysOnTop(int i);

	// dynamic menus

	void SetupOpenCDSubMenu();
	void SetupFiltersSubMenu();
	void SetupAudioSwitcherSubMenu();
	void SetupAudioOptionSubMenu();
	void SetupSubtitlesSubMenu();
	void SetupNavMixAudioSubMenu();
	void SetupNavMixSubtitleSubMenu();
	void SetupVideoStreamsSubMenu();
	void SetupNavChaptersSubMenu();
	void SetupFavoritesSubMenu();
	void SetupShadersSubMenu();
	void SetupRecentFilesSubMenu();
	void SetupLanguageMenu();

	IBaseFilter* FindSwitcherFilter();
	void SetupNavStreamSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup);
	void OnNavStreamSelectSubMenu(UINT id, DWORD dwSelGroup);

	void SetupNavMixStreamSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup);
	void OnNavMixStreamSelectSubMenu(UINT id, DWORD dwSelGroup);

	void SetupNavMixStreamSubtitleSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup);
	void OnNavMixStreamSubtitleSelectSubMenu(UINT id, DWORD dwSelGroup);

	CMenu m_popupMainMenu, m_popupMenu;
	CMenu m_openCDsMenu;
	CMenu m_filtersMenu, m_subtitlesMenu, m_audiosMenu;
	CMenu m_languageMenu;
	CMenu m_videoStreamsMenu;
	CMenu m_chaptersMenu;
	CMenu m_favoritesMenu;
	CMenu m_shadersMenu;
	CMenu m_recentfilesMenu;

	CInterfaceArray<IUnknown, &IID_IUnknown> m_pparray;
	CInterfaceArray<IAMStreamSelect> m_ssarray;

	struct Stream {
		int  Filter		= 0;
		int  Index		= 0;
		int  Num		= -1;
		int  Sel		= -1;
		bool Ext		= false;
		bool forced		= false;
		bool def		= false;
		CString Name;
	};

	BOOL SelectMatchTrack(CAtlArray<Stream>& Tracks, CString pattern, BOOL bExtPrior, size_t& nIdx);

	CAtlArray<Stream> subarray;
	void SubFlags(CString strname, bool& forced, bool& def);
	size_t GetSubSelIdx();
	int cntintsub;
	int m_nSelSub2;

	// chapters (file mode)
	CComPtr<IDSMChapterBag> m_pCB;
	void SetupChapters();

	//

	void AddTextPassThruFilter();

	int m_nLoops;

	bool m_fCustomGraph;
	bool m_fShockwaveGraph;

	CComPtr<ISubClock> m_pSubClock;

	int m_fFrameSteppingActive;
	int m_nStepForwardCount;
	REFERENCE_TIME m_rtStepForwardStart;
	int m_nVolumeBeforeFrameStepping;

	bool m_bEndOfStream;

	LARGE_INTEGER m_LastSaveTime;

	bool m_fBuffering;

	bool m_fLiveWM;

	bool m_fUpdateInfoBar;

	void SendStatusMessage(CString msg, int nTimeOut);
	CString m_playingmsg, m_closingmsg;

	REFERENCE_TIME m_rtDurationOverride;

	CComPtr<IUnknown> m_pProv;

	void CleanGraph();

	CComPtr<IBaseFilter> pAudioDubSrc;

	void ShowOptions(int idPage = 0);

	bool GetDIB(BYTE** ppData, long& size, bool fSilent = false);
	void SaveDIB(LPCTSTR fn, BYTE* pData, long size);
	BOOL IsRendererCompatibleWithSaveImage();
	void SaveImage(LPCTSTR fn);
	void SaveThumbnails(LPCTSTR fn);

	//

	friend class CWebClientSocket;
	friend class CWebServer;
	CAutoPtr<CWebServer> m_pWebServer;
	PMODE m_ePlaybackMode;
	ULONG m_lCurrentChapter;
	ULONG m_lChapterStartTime;

public:
	void StartWebServer(int nPort);
	void StopWebServer();

	CString GetStatusMessage();
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

	HWND m_hWnd_toolbar;

public:
	CMainFrame();
	DECLARE_DYNAMIC(CMainFrame)

	// Attributes
public:
	bool m_bFullScreen;
	bool m_bFirstFSAfterLaunchOnFullScreen;
	bool m_bStartInD3DFullscreen;
	bool m_bHideCursor;

	CMenu m_navMixAudioMenu, m_navMixSubtitleMenu;

	CComPtr<IBaseFilter> m_pRefClock; // Adjustable reference clock. GothSync
	CComPtr<ISyncClock> m_pSyncClock;

	bool IsFrameLessWindow() const {
		return(m_bFullScreen || AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS);
	}
	bool IsCaptionHidden() const {//If no caption, there is no menu bar. But if is no menu bar, then the caption can be.
		return(!m_bFullScreen && AfxGetAppSettings().iCaptionMenuMode > MODE_HIDEMENU);//!=MODE_SHOWCAPTIONMENU && !=MODE_HIDEMENU
	}
	bool IsMenuHidden() const {
		return(!m_bFullScreen && AfxGetAppSettings().iCaptionMenuMode != MODE_SHOWCAPTIONMENU);
	}
	bool IsSomethingLoaded() const {
		return((m_eMediaLoadState == MLS_LOADING || m_eMediaLoadState == MLS_LOADED) && !IsD3DFullScreenMode());
	}
	bool IsPlaylistEmpty() {
		return(m_wndPlaylistBar.GetCount() == 0);
	}
	bool IsInteractiveVideo() const {
		return(m_fShockwaveGraph);
	}
	
	bool IsD3DFullScreenMode() const;
	bool CursorOnD3DFullScreenWindow() const;
	void DestroyD3DWindow();

	CControlBar* m_pLastBar;

protected:
	bool			m_bUseSmartSeek;
	MPC_LOADSTATE	m_eMediaLoadState;
	bool			m_bClosingState;
	bool			m_bAudioOnly;

	BOOL			m_bNextIsOpened = FALSE;

	dispmode		m_dmBeforeFullscreen;

	CString					m_LastOpenFile;
	CAutoPtr<OpenMediaData>	m_lastOMD;

	CString m_LastOpenBDPath, m_BDLabel;
	HMONITOR m_LastWindow_HM;

	DVD_DOMAIN m_iDVDDomain;
	DWORD m_iDVDTitle;

	double m_PlaybackRate;

	double m_ZoomX, m_ZoomY, m_PosX, m_PosY;
	int m_AngleX, m_AngleY, m_AngleZ;

	// Operations
	bool OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD);
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
	void OpenSetupWindowTitle(CString fn);
	void AutoChangeMonitorMode();
	double miFPS;

	bool GraphEventComplete();

	friend class CGraphThread;
	CGraphThread* m_pGraphThread;
	bool m_bOpenedThruThread;

	void LoadKeyFrames();
	std::vector<REFERENCE_TIME> m_kfs;

	bool m_fOpeningAborted;
	bool m_bWasSnapped;

	UINT	m_flastnID;
	bool	m_bfirstPlay;
	DWORD	m_dwLastRun;

public:
	BOOL OpenCurPlaylistItem(REFERENCE_TIME rtStart = INVALID_TIME, BOOL bAddRecent = TRUE);
	void OpenMedia(CAutoPtr<OpenMediaData> pOMD);
	void PlayFavoriteFile(CString fav);
	void PlayFavoriteDVD(CString fav);
	bool ResetDevice();
	bool DisplayChange();
	void CloseMedia(BOOL bNextIsOpened = FALSE);
	void StartTunerScan(CAutoPtr<TunerScanData> pTSD);
	void StopTunerScan();

	void AddCurDevToPlaylist();

	bool m_fTrayIcon;
	void ShowTrayIcon(bool fShow);
	void SetTrayTip(CString str);

	CSize GetVideoSize();
	
	void ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo);
	void ToggleD3DFullscreen(bool fSwitchScreenResWhenHasTo);

	void MoveVideoWindow(bool bShowStats = false, bool bForcedSetVideoRect = false);
	void RepaintVideo();
	void HideVideoWindow(bool fHide);

	OAFilterState GetMediaState();
	REFERENCE_TIME GetPos(), GetDur();
	bool GetNeighbouringKeyFrames(REFERENCE_TIME rtTarget, std::pair<REFERENCE_TIME, REFERENCE_TIME>& keyframes) const;
	REFERENCE_TIME GetClosestKeyFrame(REFERENCE_TIME rtTarget) const;
	void SeekTo(REFERENCE_TIME rt, bool bShowOSD = true);
	bool ValidateSeek(REFERENCE_TIME rtPos, REFERENCE_TIME rtStop);

	bool GetBufferingProgress(int* Progress);

	// subtitle streams order function
	bool LoadSubtitle(CSubtitleItem subItem, ISubStream **actualStream = NULL);

	void UpdateSubtitle(bool fDisplayMessage = false, bool fApplyDefStyle = false);
	void SetSubtitle(ISubStream* pSubStream, int iSubtitleSel = -1, bool fApplyDefStyle = false);
	void ReplaceSubtitle(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew);
	void InvalidateSubtitle(DWORD_PTR nSubtitleId = -1, REFERENCE_TIME rtInvalidate = -1);
	void ReloadSubtitle();

	void UpdateSubDefaultStyle();

	void SetAudioTrackIdx(int index);
	void SetSubtitleTrackIdx(int index);

	// shaders
	CAtlList<CString> m_shaderlabels;
	CAtlList<CString> m_shaderlabelsScreenSpace;
	void SetShaders();
	void UpdateShaders(CString label);

	// capturing
	bool m_fCapturing;
	HRESULT BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt); // pBF: 0 buff, 1 enc, 2 mux, pmt is for 1 enc
	bool BuildToCapturePreviewPin(
		IBaseFilter* pVidCap, IPin** pVidCapPin, IPin** pVidPrevPin,
		IBaseFilter* pAudCap, IPin** pAudCapPin, IPin** pAudPrevPin);
	bool BuildGraphVideoAudio(int fVPreview, bool fVCapture, int fAPreview, bool fACapture);
	bool DoCapture(), StartCapture(), StopCapture();

	bool DoAfterPlaybackEvent();
	void ParseDirs(CAtlList<CString>& sl);
	int SearchInDir(bool DirForward);

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void RecalcLayout(BOOL bNotify = TRUE);

	// Dvb capture
	void DisplayCurrentChannelOSD();
	void DisplayCurrentChannelInfo();

	// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members

	CChildView m_wndView;
	CPlayerSeekBar m_wndSeekBar;
	CPlayerInfoBar m_wndInfoBar;
	CPlayerInfoBar m_wndStatsBar;
	CPlayerStatusBar m_wndStatusBar;
	CList<CControlBar*> m_bars;

	CPlayerSubresyncBar m_wndSubresyncBar;
	CPlayerCaptureBar m_wndCaptureBar;
	CPlayerNavigationBar m_wndNavigationBar;
	CPlayerShaderEditorBar m_wndShaderEditorBar;
	CEditListEditor m_wndEditListEditor;
	CList<CSizingControlBar*> m_dockingbars;

	CFileDropTarget m_fileDropTarget;
	// TODO
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	DROPEFFECT OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	void OnDragLeave();
	DROPEFFECT OnDragScroll(DWORD dwKeyState, CPoint point);

	const UINT CF_URL = RegisterClipboardFormat(_T("UniformResourceLocator"));
	void DropFiles(CAtlList<CString>& slFiles);

	LPCTSTR GetRecentFile();

	friend class CPPagePlayback; // TODO
	friend class CPPageAudio; // TODO
	friend class CPPagePlayer;
	friend class CMPlayerCApp; // TODO

	void RestoreControlBars();
	void SaveControlBars();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
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
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnDisplayChange();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg LRESULT OnAppCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnRawInput(UINT nInputcode, HRAWINPUT hRawInput);

	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetDevice(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRepaintRenderLess(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnPostOpen(WPARAM wParam, LPARAM lParam);

	afx_msg void SaveAppSettings();

	BOOL OnButton(UINT id, UINT nFlags, CPoint point);

	bool templclick;

	afx_msg void OnEnterSizeMove();
	afx_msg void ClipRectToMonitor(LPRECT prc);
	int snap_Margin, snap_x, snap_y;
	BOOL isSnapClose( int a, int b );
	BOOL m_bWndZoomed;
	RECT rc_forceNP;
	RECT rc_NP;

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg LRESULT OnXButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXButtonUp(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXButtonDblClk(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
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
	afx_msg void OnUpdateMenuNavSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuNavAudio(CCmdUI* pCmdUI);

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
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnFileOpenDVD();
	afx_msg void OnFileOpenIso();
	afx_msg void OnFileOpenDevice();
	afx_msg void OnFileOpenCD(UINT nID);
	afx_msg void OnFileReOpen();
	afx_msg void OnFileSaveAs();
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveImage();
	afx_msg void OnFileSaveImageAuto();
	afx_msg void OnUpdateFileSaveImage(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveThumbnails();
	afx_msg void OnUpdateFileSaveThumbnails(CCmdUI* pCmdUI);
	afx_msg void OnFileLoadSubtitle();
	afx_msg void OnUpdateFileLoadSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveSubtitle();
	afx_msg void OnUpdateFileSaveSubtitle(CCmdUI* pCmdUI);
	afx_msg void OnFileLoadAudio();
	afx_msg void OnFileISDBSearch();
	afx_msg void OnUpdateFileISDBSearch(CCmdUI* pCmdUI);
	afx_msg void OnFileISDBUpload();
	afx_msg void OnUpdateFileISDBUpload(CCmdUI* pCmdUI);
	afx_msg void OnFileISDBDownload();
	afx_msg void OnUpdateFileISDBDownload(CCmdUI* pCmdUI);
	afx_msg void OnFileProperties();
	afx_msg void OnUpdateFileProperties(CCmdUI* pCmdUI);
	afx_msg void OnFileClosePlaylist();
	afx_msg void OnFileCloseMedia(); // no menu item
	afx_msg void OnUpdateFileClose(CCmdUI* pCmdUI);

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
	afx_msg void OnViewEditListEditor();
	afx_msg void OnEDLIn();
	afx_msg void OnUpdateEDLIn(CCmdUI* pCmdUI);
	afx_msg void OnEDLOut();
	afx_msg void OnUpdateEDLOut(CCmdUI* pCmdUI);
	afx_msg void OnEDLNewClip();
	afx_msg void OnUpdateEDLNewClip(CCmdUI* pCmdUI);
	afx_msg void OnEDLSave();
	afx_msg void OnUpdateEDLSave(CCmdUI* pCmdUI);
	afx_msg void OnViewCapture();
	afx_msg void OnUpdateViewCapture(CCmdUI* pCmdUI);
	afx_msg void OnViewShaderEditor();
	afx_msg void OnUpdateViewShaderEditor(CCmdUI* pCmdUI);
	afx_msg void OnViewMinimal();
	afx_msg void OnUpdateViewMinimal(CCmdUI* pCmdUI);
	afx_msg void OnViewCompact();
	afx_msg void OnUpdateViewCompact(CCmdUI* pCmdUI);
	afx_msg void OnViewNormal();
	afx_msg void OnUpdateViewNormal(CCmdUI* pCmdUI);
	afx_msg void OnViewFullscreen();
	afx_msg void OnViewFullscreenSecondary();
	afx_msg void OnMoveWindowToPrimaryScreen();
	afx_msg void OnUpdateViewFullscreen(CCmdUI* pCmdUI);
	afx_msg void OnViewZoom(UINT nID);
	afx_msg void OnUpdateViewZoom(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomAutoFit();
	afx_msg void OnViewDefaultVideoFrame(UINT nID);
	afx_msg void OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI);
	afx_msg void OnViewSwitchVideoFrame();
	afx_msg void OnUpdateViewSwitchVideoFrame(CCmdUI* pCmdUI);
	afx_msg void OnViewKeepaspectratio();
	afx_msg void OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI);
	afx_msg void OnViewCompMonDeskARDiff();
	afx_msg void OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScan(UINT nID);
	afx_msg void OnUpdateViewPanNScan(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScanPresets(UINT nID);
	afx_msg void OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI);
	afx_msg void OnViewRotate(UINT nID);
	afx_msg void OnUpdateViewRotate(CCmdUI* pCmdUI);
	afx_msg void OnViewAspectRatio(UINT nID);
	afx_msg void OnUpdateViewAspectRatio(CCmdUI* pCmdUI);
	afx_msg void OnViewAspectRatioNext();
	afx_msg void OnViewOntop(UINT nID);
	afx_msg void OnUpdateViewOntop(CCmdUI* pCmdUI);
	afx_msg void OnViewOptions();
	afx_msg void OnUpdateViewTearingTest(CCmdUI* pCmdUI);
	afx_msg void OnViewTearingTest();
	afx_msg void OnUpdateViewDisplayStats(CCmdUI* pCmdUI);
	afx_msg void OnViewResetStats();
	afx_msg void OnViewDisplayStatsSC();

	afx_msg void OnUpdateViewD3DFullscreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewDisableDesktopComposition(CCmdUI* pCmdUI);

	afx_msg void OnUpdateViewFullscreenGUISupport(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewEnableFrameTimeCorrection(CCmdUI* pCmdUI);
	afx_msg void OnViewVSync();
	afx_msg void OnViewVSyncAccurate();

	afx_msg void OnViewD3DFullScreen();
	afx_msg void OnViewDisableDesktopComposition();
	afx_msg void OnViewAlternativeVSync();
	afx_msg void OnViewResetDefault();
	afx_msg void OnUpdateViewReset(CCmdUI* pCmdUI);

	afx_msg void OnViewFullscreenGUISupport();
	afx_msg void OnViewEnableFrameTimeCorrection();
	afx_msg void OnViewVSyncOffsetIncrease();
	afx_msg void OnViewVSyncOffsetDecrease();
	afx_msg void OnUpdateShaderToggle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShaderToggleScreenSpace(CCmdUI* pCmdUI);
	afx_msg void OnShaderToggle();
	afx_msg void OnShaderToggleScreenSpace();
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
	afx_msg void OnPlayPauseI();
	afx_msg void OnPlayPlaypause();
	afx_msg void OnPlayStop();
	afx_msg void OnUpdatePlayPauseStop(CCmdUI* pCmdUI);
	afx_msg void OnPlayFrameStep(UINT nID);
	afx_msg void OnUpdatePlayFrameStep(CCmdUI* pCmdUI);
	afx_msg void OnPlaySeek(UINT nID);
	afx_msg void OnPlaySeekKey(UINT nID); // no menu item
	afx_msg void OnUpdatePlaySeek(CCmdUI* pCmdUI);
	afx_msg void OnPlayGoto();
	afx_msg void OnUpdateGoto(CCmdUI* pCmdUI);
	afx_msg void OnPlayChangeRate(UINT nID);
	afx_msg void OnUpdatePlayChangeRate(CCmdUI* pCmdUI);
	afx_msg void OnPlayResetRate();
	afx_msg void OnUpdatePlayResetRate(CCmdUI* pCmdUI);
	afx_msg void OnPlayChangeAudDelay(UINT nID);
	afx_msg void OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI);
	afx_msg void OnPlayFilters(UINT nID);
	afx_msg void OnUpdatePlayFilters(CCmdUI* pCmdUI);
	afx_msg void OnPlayShaders(UINT nID);
	afx_msg void OnPlayAudio(UINT nID);

	afx_msg void OnPlayAudioOption(UINT nID);

	afx_msg void OnUpdatePlayAudio(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlayAudioOption(CCmdUI* pCmdUI);
	afx_msg void OnPlaySubtitles(UINT nID);
	afx_msg void OnUpdatePlaySubtitles(CCmdUI* pCmdUI);

	afx_msg void OnUpdateNavMixSubtitles(CCmdUI* pCmdUI);

	afx_msg void OnSelectStream(UINT nID);
	afx_msg void OnPlayVolume(UINT nID);
	afx_msg void OnPlayVolumeGain(UINT nID);
	afx_msg void OnAutoVolumeControl();
	afx_msg void OnUpdateNormalizeVolume(CCmdUI* pCmdUI);
	afx_msg void OnPlayColor(UINT nID);
	afx_msg void OnAfterplayback(UINT nID);
	afx_msg void OnUpdateAfterplayback(CCmdUI* pCmdUI);

	afx_msg void OnNavigateSkip(UINT nID);
	afx_msg void OnUpdateNavigateSkip(CCmdUI* pCmdUI);
	afx_msg void OnNavigateSkipFile(UINT nID);
	afx_msg void OnUpdateNavigateSkipFile(CCmdUI* pCmdUI);
	afx_msg void OnNavigateMenu(UINT nID);
	afx_msg void OnUpdateNavigateMenu(CCmdUI* pCmdUI);
	afx_msg void OnNavigateAudioMix(UINT nID);
	afx_msg void OnNavigateSubpic(UINT nID);
	afx_msg void OnNavigateAngle(UINT nID);
	afx_msg void OnNavigateChapters(UINT nID);
	afx_msg void OnNavigateMenuItem(UINT nID);
	afx_msg void OnUpdateNavigateMenuItem(CCmdUI* pCmdUI);
	afx_msg void OnTunerScan();
	afx_msg void OnUpdateTunerScan(CCmdUI* pCmdUI);

	afx_msg void OnFavoritesAdd();
	afx_msg void OnUpdateFavoritesAdd(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesQuickAddFavorite();
	afx_msg void OnFavoritesOrganize();
	afx_msg void OnUpdateFavoritesOrganize(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesFile(UINT nID);
	afx_msg void OnUpdateFavoritesFile(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesDVD(UINT nID);
	afx_msg void OnUpdateFavoritesDVD(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesDevice(UINT nID);
	afx_msg void OnUpdateFavoritesDevice(CCmdUI* pCmdUI);
	afx_msg void OnRecentFileClear();
	afx_msg void OnUpdateRecentFileClear(CCmdUI* pCmdUI);
	afx_msg void OnRecentFile(UINT nID);
	afx_msg void OnUpdateRecentFile(CCmdUI* pCmdUI);

	afx_msg void OnHelpHomepage();
	afx_msg void OnHelpCheckForUpdate();
	//afx_msg void OnHelpDocumentation();
	afx_msg void OnHelpToolbarImages();
	//afx_msg void OnHelpDonate();

	// Subtitle position
	afx_msg void OnSubtitlePos(UINT nID);

	afx_msg void OnClose();

	afx_msg void OnLanguage(UINT nID);

	void OnFilePostOpenMedia(CAutoPtr<OpenMediaData> pOMD);
	void OnFilePostCloseMedia();

	CMPC_Lcd m_Lcd;

	// Main Window
	CWnd*			m_pVideoWnd;
	CWnd*			m_pOSDWnd;

	CPlayerToolBar		m_wndToolBar;
	CPlayerListCtrl		m_wndListCtrl;
	CPlayerPlaylistBar	m_wndPlaylistBar;
	CFlyBar				m_wndFlyBar;
	CPreView			m_wndPreView; // SmartSeek

	bool			IsMadVRExclusiveMode;

	void CreateFlyBar();
	bool FlyBarSetPos();
	void DestroyFlyBar();

	void CreateOSDBar();
	bool OSDBarSetPos();
	void DestroyOSDBar();

	HRESULT PreviewWindowHide();
	HRESULT PreviewWindowShow(REFERENCE_TIME rtCur2);
	bool CanPreviewUse();

	CFullscreenWnd*	m_pFullscreenWnd;
	COSD		m_OSD;

	bool		m_bRemainingTime;
	bool		m_bOSDLocalTime;
	bool		m_bOSDFileName;
	CString		GetSystemLocalTime();
	int			m_nCurSubtitle;
	long		m_lSubtitleShift;
	__int64		m_rtCurSubPos;
	CString		m_strTitle;
	CString		m_strCurPlaybackLabel;
	CString		m_strFnFull;
	CString		m_strUrl;

	YOUTUBE_FIELDS	m_youtubeFields;
	CString			GetStrForTitle();
	CString			GetAltFileName();

	bool		m_bToggleShader;
	bool		m_bToggleShaderScreenSpace;
	bool		m_bInOptions;
	bool		m_bStopTunerScan;

	void		SetLoadState(MPC_LOADSTATE iState);
	void		SetPlayState(MPC_PLAYSTATE iState);
	bool		CreateFullScreenWindow();
	void		SetupEVRColorControl();
	void		SetupVMR9ColorControl();
	void		SetColorControl(DWORD flags, int& brightness, int& contrast, int& hue, int& saturation);
	void		SetClosedCaptions(bool enable);
	LPCTSTR		GetDVDAudioFormatName(DVD_AudioAttributes& ATR) const;
	void		SetAudioDelay(REFERENCE_TIME rtShift);
	void		SetSubtitleDelay(int delay_ms);
	//void		AutoSelectTracks();
	bool		IsRealEngineCompatible(CString strFilename) const;
	void		SetTimersPlay();
	void		KillTimersStop();

	// MPC API functions
	void		ProcessAPICommand(COPYDATASTRUCT* pCDS);
	void		SendAPICommand (MPCAPI_COMMAND nCommand, LPCWSTR fmt, ...);
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

	HMODULE		m_hDWMAPI;
	HRESULT		(__stdcall * m_DwmSetWindowAttributeFnc)(HWND hwnd, DWORD dwAttribute, __in  LPCVOID pvAttribute, DWORD cbAttribute);
	HRESULT		(__stdcall * m_DwmSetIconicThumbnailFnc)( __in  HWND hwnd, __in  HBITMAP hbmp, __in  DWORD dwSITFlags);
	HRESULT		(__stdcall * m_DwmSetIconicLivePreviewBitmapFnc)(HWND hwnd, HBITMAP hbmp, __in_opt  POINT *pptClient, DWORD dwSITFlags);
	HRESULT		(__stdcall * m_DwmInvalidateIconicBitmapsFnc)( __in  HWND hwnd);

	HBITMAP		m_CaptureWndBitmap;
	bool		isWindowMinimized;
	HBITMAP		CreateCaptureDIB(int nWidth, int nHeight);
	void		CreateCaptureWindow();


	HRESULT		SetAudioPicture(BOOL show = TRUE);
	CMPCPngImage	m_InternalImage, m_InternalImageSmall;
	bool		m_bInternalImageRes;

	HBITMAP		m_ThumbCashedBitmap;
	CSize		m_ThumbCashedSize;

	void		AddFavorite(bool bDisplayMessage = false, bool bShowDialog = true);

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	void WTSRegisterSessionNotification();
	void WTSUnRegisterSessionNotification();

	CString m_OldMessage;
	void SetStatusMessage(CString m_msg);
	CString FillMessage();

	bool	m_fValidDVDOpen;

	CComPtr<IBaseFilter> m_pBFmadVR;

	HMODULE			m_hWtsLib;

	CDebugMonitor	m_DebugMonitor;

	static DWORD WINAPI		NotifyRenderThreadEntryPoint(LPVOID lpParameter);
	void					SetupNotifyRenderThread(CAtlArray<HANDLE>& handles);
	DWORD					NotifyRenderThread();

	CStringArray			m_ExtSubFiles;
	CAtlArray<CTime>		m_ExtSubFilesTime;
	CStringArray			m_ExtSubPaths;
	CAtlArray<HANDLE>		m_ExtSubPathsHandles;
	HANDLE					m_hNotifyRenderThread;
	HANDLE					m_hStopNotifyRenderThreadEvent;
	HANDLE					m_hRefreshNotifyRenderThreadEvent;

	::CEvent				m_hGraphThreadEventOpen;
	::CEvent				m_hGraphThreadEventClose;

public:
#if (_MSC_VER < 1800)
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, UINT nEventData);
#else
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);
#endif
	afx_msg void OnSessionChange(UINT nSessionState, UINT nId);

	void EnableShaders1(bool enable);
	void EnableShaders2(bool enable);

	CHdmvClipInfo::CPlaylist m_MPLSPlaylist;
	BOOL m_bIsBDPlay;
	BOOL OpenBD(CString Path, REFERENCE_TIME rtStart = INVALID_TIME, BOOL bAddRecent = TRUE);

	void CheckMenuRadioItem(UINT first, UINT last, UINT check);

	bool m_bUseReclock;

private:
	enum TH_STATE {
		TH_START,
		TH_WORK,
		TH_CLOSE,
		TH_ERROR
	};
	volatile TH_STATE	m_fYoutubeThreadWork;
	volatile QWORD		m_YoutubeCurrent;
	volatile QWORD		m_YoutubeTotal;
	CString				m_YoutubeFile;
	CWinThread*			m_YoutubeThread;

	HRESULT		QueryProgressYoutube(LONGLONG *pllTotal, LONGLONG *pllCurrent) {
		if (m_YoutubeTotal > 0 && m_YoutubeCurrent < m_YoutubeTotal) {
			if (pllTotal) {
				*pllTotal = m_YoutubeTotal;
			}
			if (pllCurrent) {
				*pllCurrent = m_YoutubeCurrent;
			}
			return S_OK;
		}

		return E_FAIL;
	}

	int			GetStreamCount(DWORD dwSelGroup);

	DWORD_PTR	m_nMainFilterId;

	BOOL		m_bLeftMouseDown			= FALSE;
	BOOL		m_bLeftMouseDownFullScreen	= FALSE;

public:
	UINT		YoutubeThreadProc();

	BOOL		CheckMainFilter(IBaseFilter* pBF);

	void		AddSubtitlePathsAddons(CString FileName);
	void		AddAudioPathsAddons(CString FileName);

	void		MakeBDLabel(CString path, CString& label, CString* pBDlabel = NULL);
	void		MakeDVDLabel(CString& label, CString* pDVDlabel = NULL);

	CString		GetCurFileName();
	CString		GetCurDVDPath(BOOL bSelectCurTitle = FALSE);

	GUID		GetTimeFormat();

	CColorControl	m_ColorCintrol;
protected:
	CDiskImage	m_DiskImage;
	BOOL		m_bNeedUnmountImage = TRUE;
	BOOL		OpenIso(CString pathName);

	void		AddRecent(CString pathName);

	CString		GetVidPos();
	CString		CreateSnapShotFileName();

	REFTIME		GetAvgTimePerFrame(BOOL bUsePCAP = TRUE) const;

	BOOL		OpenYoutubePlaylist(CString url);
};
