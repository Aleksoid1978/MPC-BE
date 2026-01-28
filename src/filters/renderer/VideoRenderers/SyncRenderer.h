/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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
#include <ID3DFullscreenControl.h>
#include "SubPic/ISubPic.h"
#include "AllocatorPresenterImpl.h"
#include "AllocatorCommon.h"
#include <dxva2api.h>
#include "D3DUtil/D3D9Font.h"
#include "../SyncClock/ISyncClock.h"

#define NB_JITTER 126
#define MAX_FIFO_SIZE 1024

#undef DrawText // disable conflicting define

enum {
	shader_bspline_x,
	shader_bspline_y,
	shader_mitchell_x,
	shader_mitchell_y,
	shader_catmull_x,
	shader_catmull_y,
	shader_bicubic06_x,
	shader_bicubic06_y,
	shader_bicubic08_x,
	shader_bicubic08_y,
	shader_bicubic10_x,
	shader_bicubic10_y,
	shader_lanczos2_x,
	shader_lanczos2_y,
	shader_lanczos3_x,
	shader_lanczos3_y,
	shader_downscaling_x,
	shader_downscaling_y,
	shader_count
};

class CFocusThread;


namespace GothSync
{
#pragma pack(push, 1)

	template<unsigned texcoords>
	struct MYD3DVERTEX {
		float x, y, z, rhw;
		struct {
			float u, v;
		} t[texcoords];
	};

	template<>
	struct MYD3DVERTEX<0> {
		float x, y, z, rhw;
		DWORD Diffuse;
	};

#pragma pack(pop)

	class CGenlock;
	class CSyncRenderer;

	// Base allocator-presenter

	class CBaseAP:
		public CAllocatorPresenterImpl
	{
	protected:
		static const int MAX_PICTURE_SLOTS = RS_EVRBUFFERS_MAX + 2; // Last 2 for pixels shader!

		HMODULE m_hD3D9 = nullptr;
		HRESULT (__stdcall * m_pfDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex**) = nullptr;

		CCritSec m_allocatorLock;
		CComPtr<IDirect3D9Ex>		m_pD3D9Ex;
		CComPtr<IDirect3DDevice9Ex>	m_pDevice9Ex;

		bool m_bEnableSubPic = true;

		ExtraRendererSettings m_ExtraSets;

		bool m_bDeviceResetRequested = false;
		bool m_bPendingResetDevice   = false;
		bool m_bNeedResetDevice      = false;

		UINT						m_CurrentAdapter = 0;
		D3DCAPS9					m_Caps;
		D3DFORMAT					m_SurfaceFmt;
		D3DFORMAT					m_BackbufferFmt;
		D3DFORMAT					m_DisplayFmt;
		D3DTEXTUREFILTERTYPE		m_filter;
		D3DPRESENT_PARAMETERS		m_pp;

		CComPtr<IDirect3DTexture9>	m_pVideoTextures[MAX_PICTURE_SLOTS];
		CComPtr<IDirect3DSurface9>	m_pVideoSurfaces[MAX_PICTURE_SLOTS];
		CComPtr<IDirect3DTexture9>	m_pScreenSizeTextures[2];
		CComPtr<IDirect3DTexture9>	m_pResizeTexture;
		CComPtr<ID3DXLine>			m_pLine;

		CD3D9Font m_Font3D;

		// Shaders
		std::unique_ptr<CPixelShaderCompiler> m_pPSC;
		std::list<CExternalPixelShader>	m_pPixelShaders;
		std::list<CExternalPixelShader>	m_pPixelShadersScreenSpace;
		CComPtr<IDirect3DPixelShader9>	m_pResizerPixelShaders[shader_count];
		CComPtr<IDirect3DPixelShader9>	m_pPSCorrectionYCgCo;

		void SendResetRequest();
		virtual HRESULT CreateDXDevice(CString &_Error);
		virtual HRESULT AllocSurfaces(D3DFORMAT Format = D3DFMT_A8R8G8B8);
		virtual void DeleteSurfaces();

		LONGLONG m_LastAdapterCheck;

		HRESULT InitShaderResizer();

		// Functions to trace timing performance
		void SyncStats(LONGLONG syncTime);
		void SyncOffsetStats(LONGLONG syncOffset);
		void DrawStats();

		template<unsigned texcoords>
		HRESULT TextureBlt(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter);
		MFOffset GetOffset(float v);
		MFVideoArea GetArea(float x, float y, DWORD width, DWORD height);
		bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d);

		HRESULT DrawRectBase(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<0> v[4]);
		HRESULT DrawRect(const D3DCOLOR _Color, const CRect &_Rect);

		HRESULT TextureCopy(IDirect3DTexture9* pTexture);
		HRESULT TextureCopyRect(
			IDirect3DTexture9* pTexture,
			const CRect& srcRect, const CRect& dstRect,
			const D3DTEXTUREFILTERTYPE filter,
			const int iRotation, const bool bFlip);

		HRESULT TextureResizeShader(
			IDirect3DTexture9* pTexture,
			const CRect& srcRect, const CRect& dstRect,
			IDirect3DPixelShader9* pShader,
			const int iRotation, const bool bFlip);

		HRESULT ResizeShaderPass(
			IDirect3DTexture9* pTexture, IDirect3DSurface9* pRenderTarget,
			const CRect& srcRect, const CRect& dstRect,
			const int iShaderX);

		HRESULT AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture);

		virtual void OnResetDevice() {};

		HRESULT (__stdcall *m_pfD3DXCreateLine)(
			_In_  LPDIRECT3DDEVICE9 pDevice,
			_Out_ LPD3DXLINE        *ppLine
		) = nullptr;

		long m_nTearingPos = 0;

		bool                       m_bAlphaBitmapEnable = false;
		CComPtr<IDirect3DTexture9> m_pAlphaBitmapTexture;
		MFVideoAlphaBitmapParams   m_AlphaBitmapParams = {};

		unsigned m_nSurfaces = 4; // Total number of DX Surfaces
		UINT32 m_iCurSurface = 0; // Surface currently displayed
		long m_nUsedBuffer = 0;

		CSize m_ScreenSize;
		int m_iRotation = 0; // total rotation angle clockwise of frame (0, 90, 180 or 270 deg.)
		bool m_bFlip    = false; // horizontal flip. for vertical flip use together with a rotation of 180 deg.
		DXVA2_ExtendedFormat m_inputExtFormat = {};
		const wchar_t* m_wsResizer = L""; // empty string, not nullptr

		long m_lNextSampleWait; // Waiting time for next sample in EVR
		bool m_bSnapToVSync = false; // True if framerate is low enough so that snap to vsync makes sense

		UINT m_uScanLineEnteringPaint; // The active scan line when entering Paint()
		REFERENCE_TIME m_llEstVBlankTime = 0.0; // Next vblank start time in reference clock "coordinates"

		double m_fAvrFps       = 0.0; // Estimate the true FPS as given by the distance between vsyncs when a frame has been presented
		double m_fJitterStdDev = 0.0; // VSync estimate std dev
		double m_fJitterMean; // Mean time between two syncpulses when a frame has been presented (i.e. when Paint() has been called

		double m_fSyncOffsetAvr    = 0.0; // Mean time between the call of Paint() and vsync. To avoid tearing this should be several ms at least
		double m_fSyncOffsetStdDev = 0.0; // The std dev of the above

		bool m_b10BitOutput = false;
		bool m_bIsFullscreen;
		BOOL m_bCompositionEnabled; // DWM composition before creating a D3D9 device

		// Display and frame rates and cycles
		double m_dDetectedScanlineTime = 0.0; // Time for one (horizontal) scan line. Extracted at stream start and used to calculate vsync time
		double m_dD3DRefreshCycle = 0.0; // Display refresh cycle ms
		double m_dEstRefreshCycle = 0.0; // As estimated from scan lines
		double m_dFrameCycle = 0.0; // Average sample time, extracted from the samples themselves
		// double m_fps is defined in ISubPic.h
		double m_dOptimumDisplayCycle = 0.0; // The display cycle that is closest to the frame rate. A multiple of the actual display cycle
		double m_dCycleDifference = 0.0; // Difference in video and display cycle time relative to the video cycle time

		unsigned m_pcFramesDropped;
		unsigned m_pcFramesDrawn;

		LONGLONG m_llJitters[NB_JITTER] = {};     // Vertical sync time stats
		LONGLONG m_llSyncOffsets[NB_JITTER] = {}; // Sync offset time stats
		int m_nNextJitter = 0;
		int m_nNextSyncOffset = 0;

		LONGLONG m_llLastSyncTime = 0;

		LONGLONG m_MaxJitter;
		LONGLONG m_MinJitter;
		LONGLONG m_MaxSyncOffset;
		LONGLONG m_MinSyncOffset;
		unsigned m_uSyncGlitches = 0;

		LONGLONG m_llSampleTime, m_llLastSampleTime; // Present time for the current sample
		LONGLONG m_llHysteresis = 0;
		long m_lShiftToNearest = -1; // Illegal value to start with
		long m_lShiftToNearestPrev;
		bool m_bVideoSlowerThanDisplay;

		double m_TextScale = 1.0;
		CString m_strMixerOutputFmt;
		CString m_strMsgError;

		CGenlock *m_pGenlock = nullptr; // The video - display synchronizer class
		CComPtr<IReferenceClock> m_pRefClock; // The reference clock. Used in Paint()
		CComPtr<IAMAudioRendererStats> m_pAudioStats; // Audio statistics from audio renderer. To check so that audio is in sync
		long m_lAudioLag = 0; // Time difference between audio and video when the audio renderer is matching rate to the external reference clock
		long m_lAudioLagMin = 10000, m_lAudioLagMax = -10000; // The accumulated difference between the audio renderer and the master clock
		DWORD m_lAudioSlaveMode; // To check whether the audio renderer matches rate with SyncClock (returns the value 4 if it does)

		double GetRefreshRate(); // Get the best estimate of the display refresh rate in Hz
		double GetDisplayCycle(); // Get the best estimate of the display cycle time in milliseconds
		double GetCycleDifference(); // Get the difference in video and display cycle times.
		void EstimateRefreshTimings(); // Estimate the times for one scan line and one frame respectively from the actual refresh data

		CFocusThread* m_FocusThread = nullptr;
	public:
		CBaseAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error);
		~CBaseAP();

		// IAllocatorPresenter
		STDMETHODIMP DisableSubPicInitialization() override;
		STDMETHODIMP_(SIZE) GetVideoSize();
		STDMETHODIMP_(SIZE) GetVideoSizeAR();
		STDMETHODIMP SetRotation(int rotation) override;
		STDMETHODIMP_(int) GetRotation() override;
		STDMETHODIMP SetFlip(bool flip) override;
		STDMETHODIMP_(bool) GetFlip() override;
		STDMETHODIMP_(bool) Paint(bool fAll);
		STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
		STDMETHODIMP GetDisplayedImage(LPVOID* dibImage) override;
		STDMETHODIMP_(int) GetPixelShaderMode() override { return 9; }
		STDMETHODIMP ClearPixelShaders(int target);
		STDMETHODIMP AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode);
		STDMETHODIMP_(bool) ResetDevice();
		STDMETHODIMP_(bool) DisplayChange();
		STDMETHODIMP_(void) ResetStats() override;
		STDMETHODIMP_(void) SetExtraSettings(ExtraRendererSettings* pExtraSets) override;

		// ISubRenderOptions
		STDMETHODIMP GetInt(LPCSTR field, int* value);
		STDMETHODIMP GetString(LPCSTR field, LPWSTR* value, int* chars);
	};

	// Sync allocator-presenter

	class CSyncAP:
		public CBaseAP,
		public IMFGetService,
		public IMFTopologyServiceLookupClient,
		public IMFVideoDeviceID,
		public IMFVideoPresenter,
		public IDirect3DDeviceManager9,
		public IMFAsyncCallback,
		public IQualProp,
		public IMFRateSupport,
		public IMFVideoDisplayControl,
		public IMFVideoMixerBitmap,
		public IEVRTrustedVideoPlugin,
		public ISyncClockAdviser,
		public ID3DFullscreenControl,
		public IPlaybackNotify
	{
	private:
		HMODULE m_hDxva2Lib = nullptr;
		HMODULE m_hEvrLib = nullptr;
		HMODULE m_hAvrtLib = nullptr;

	public:
		CSyncAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error);
		~CSyncAP();

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		STDMETHODIMP_(bool) Paint(bool fAll);
		STDMETHODIMP InitializeDevice(AM_MEDIA_TYPE* pMediaType);

		// IAllocatorPresenter
		STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) override;
		STDMETHODIMP_(CLSID) GetAPCLSID() override;
		STDMETHODIMP_(bool) ResetDevice() override;
		STDMETHODIMP_(bool) IsRendering() override { return (m_nRenderState == Started); }

		// IMFClockStateSink
		STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
		STDMETHODIMP OnClockStop(MFTIME hnsSystemTime);
		STDMETHODIMP OnClockPause(MFTIME hnsSystemTime);
		STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime);
		STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate);

		// IBaseFilter delegate
		bool GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);

		// IQualProp (EVR statistics window). These are incompletely implemented currently
		STDMETHODIMP get_FramesDroppedInRenderer(int *pcFrames);
		STDMETHODIMP get_FramesDrawn(int *pcFramesDrawn);
		STDMETHODIMP get_AvgFrameRate(int *piAvgFrameRate);
		STDMETHODIMP get_Jitter(int *iJitter);
		STDMETHODIMP get_AvgSyncOffset(int *piAvg);
		STDMETHODIMP get_DevSyncOffset(int *piDev);

		// IMFRateSupport
		STDMETHODIMP GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
		STDMETHODIMP GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
		STDMETHODIMP IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate);
		float GetMaxRate(BOOL bThin);

		// IMFVideoPresenter
		STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
		STDMETHODIMP GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

		// IMFTopologyServiceLookupClient
		STDMETHODIMP InitServicePointers(__in  IMFTopologyServiceLookup *pLookup);
		STDMETHODIMP ReleaseServicePointers();

		// IMFVideoDeviceID
		STDMETHODIMP GetDeviceID(__out  IID *pDeviceID);

		// IMFGetService
		STDMETHODIMP GetService (__RPC__in REFGUID guidService, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID *ppvObject);

		// IMFAsyncCallback
		STDMETHODIMP GetParameters(__RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue);
		STDMETHODIMP Invoke(__RPC__in_opt IMFAsyncResult *pAsyncResult);

		// IMFVideoDisplayControl
		STDMETHODIMP GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo);
		STDMETHODIMP GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax);
		STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest);
		STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest);
		STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode);
		STDMETHODIMP GetAspectRatioMode(DWORD *pdwAspectRatioMode);
		STDMETHODIMP SetVideoWindow(HWND hwndVideo);
		STDMETHODIMP GetVideoWindow(HWND *phwndVideo);
		STDMETHODIMP RepaintVideo();
		STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp);
		STDMETHODIMP SetBorderColor(COLORREF Clr);
		STDMETHODIMP GetBorderColor(COLORREF *pClr);
		STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags);
		STDMETHODIMP GetRenderingPrefs(DWORD *pdwRenderFlags);
		STDMETHODIMP SetFullscreen(BOOL fFullscreen);
		STDMETHODIMP GetFullscreen(BOOL *pfFullscreen);

		// IMFVideoMixerBitmap
		STDMETHODIMP ClearAlphaBitmap();
		STDMETHODIMP GetAlphaBitmapParameters(MFVideoAlphaBitmapParams *pBmpParms);
		STDMETHODIMP SetAlphaBitmap(const MFVideoAlphaBitmap *pBmpParms);
		STDMETHODIMP UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams *pBmpParms);

		// IEVRTrustedVideoPlugin
		STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes);
		STDMETHODIMP CanConstrict(BOOL *pYes);
		STDMETHODIMP SetConstriction(DWORD dwKPix);
		STDMETHODIMP DisableImageExport(BOOL bDisable);

		// IDirect3DDeviceManager9
		STDMETHODIMP ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken);
		STDMETHODIMP OpenDeviceHandle(HANDLE *phDevice);
		STDMETHODIMP CloseDeviceHandle(HANDLE hDevice);
		STDMETHODIMP TestDevice(HANDLE hDevice);
		STDMETHODIMP LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock);
		STDMETHODIMP UnlockDevice(HANDLE hDevice, BOOL fSaveState);
		STDMETHODIMP GetVideoService(HANDLE hDevice, REFIID riid, void **ppService);

		// ID3DFullscreenControl
		STDMETHODIMP SetD3DFullscreen(bool fEnabled);
		STDMETHODIMP GetD3DFullscreen(bool* pfEnabled);

		// IPlaybackNotify
		STDMETHODIMP Stop();

	protected:
		void OnResetDevice();

	private:
		// dxva.dll
		typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);
		// evr.dll
		typedef HRESULT (__stdcall *PTR_MFCreateVideoSampleFromSurface)(IUnknown* pUnkSurface, IMFSample** ppSample);
		typedef HRESULT (__stdcall *PTR_MFCreateVideoMediaType)(const MFVIDEOFORMAT* pVideoFormat, IMFVideoMediaType** ppIVideoMediaType);
		// avrt.dll
		typedef HANDLE (__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
		typedef BOOL (__stdcall *PTR_AvSetMmThreadPriority)(HANDLE AvrtHandle, AVRT_PRIORITY Priority);
		typedef BOOL (__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

		enum RENDER_STATE {
			Stopped  = State_Stopped,
			Paused   = State_Paused,
			Started  = State_Running,
			Shutdown = State_Running + 1
		} ;

		CSyncRenderer* m_pOuterEVR = nullptr;

		CComPtr<IMFClock> m_pClock;
		CComPtr<IDirect3DDeviceManager9> m_pD3DManager;
		CComPtr<IMFTransform> m_pMixer;
		CComPtr<IMediaEventSink> m_pSink;
		CComPtr<IMFVideoMediaType> m_pMediaType;
		MFVideoAspectRatioMode m_dwVideoAspectRatioMode = MFVideoARMode_PreservePicture;
		MFVideoRenderPrefs     m_dwVideoRenderPrefs     = (MFVideoRenderPrefs)0;
		COLORREF m_BorderColor = RGB(0, 0, 0);

		HANDLE m_hEvtQuit  = nullptr; // Stop rendering thread event
		bool   m_bEvtQuit  = false;
		HANDLE m_hEvtFlush = nullptr; // Discard all buffers
		bool   m_bEvtFlush = false;
		HANDLE m_hEvtSkip  = nullptr; // Skip frame
		bool   m_bEvtSkip  = false;

		bool m_bUseInternalTimer   = false;
		int  m_LastSetOutputRange  = -1;
		std::atomic_bool m_bPendingRenegotiate = false;
		bool m_bPendingMediaFinished = false;
		bool m_bPrerolled = false; // true if first sample has been displayed.

		HANDLE m_hRenderThread = nullptr;
		HANDLE m_hMixerThread  = nullptr;
		RENDER_STATE m_nRenderState = Shutdown;
		bool m_bStepping = false;

		CCritSec m_SampleQueueLock;
		CCritSec m_ImageProcessingLock;

		std::list<CComQIPtr<IMFSample, &IID_IMFSample>> m_FreeSamples;
		std::list<CComQIPtr<IMFSample, &IID_IMFSample>> m_ScheduledSamples;
		UINT m_nResetToken = 0;
		int  m_nStepCount  = 0;

		bool GetSampleFromMixer();
		void MixerThread();
		static DWORD WINAPI MixerThreadStatic(LPVOID lpParam);
		void RenderThread();
		static DWORD WINAPI RenderThreadStatic(LPVOID lpParam);

		void StartWorkerThreads();
		void StopWorkerThreads();
		HRESULT CheckShutdown() const;
		void CompleteFrameStep(bool bCancel);

		void RemoveAllSamples();
		STDMETHODIMP AdviseSyncClock(ISyncClock* sC);
		HRESULT BeginStreaming();
		HRESULT GetFreeSample(IMFSample** ppSample);
		HRESULT GetScheduledSample(IMFSample** ppSample, int &_Count);
		void MoveToFreeList(IMFSample* pSample, bool bTail);
		void MoveToScheduledList(IMFSample* pSample, bool _bSorted);
		void FlushSamples();
		void FlushSamplesInternal();

		int GetOutputMediaTypeMerit(IMFMediaType *pMediaType);
		HRESULT RenegotiateMediaType();
		HRESULT IsMediaTypeSupported(IMFMediaType* pMixerType);
		HRESULT CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType);
		HRESULT SetMediaType(IMFMediaType* pType);

		// Functions pointers for Vista/.NET3 specific library
		PTR_DXVA2CreateDirect3DDeviceManager9 pfDXVA2CreateDirect3DDeviceManager9 = nullptr;
		PTR_MFCreateVideoSampleFromSurface pfMFCreateVideoSampleFromSurface = nullptr;
		PTR_MFCreateVideoMediaType pfMFCreateVideoMediaType = nullptr;

		PTR_AvSetMmThreadCharacteristicsW pfAvSetMmThreadCharacteristicsW = nullptr;
		PTR_AvSetMmThreadPriority pfAvSetMmThreadPriority = nullptr;
		PTR_AvRevertMmThreadCharacteristics pfAvRevertMmThreadCharacteristics = nullptr;
	};

	// Sync renderer

	class CSyncRenderer:
		public CUnknown,
		public IBaseFilter
	{
		CComPtr<IUnknown> m_pEVR;
		IBaseFilter* m_pEVRBase;
		CSyncAP *m_pAllocatorPresenter;

	public:
		CSyncRenderer(LPCWSTR pName, LPUNKNOWN pUnk, HRESULT& hr, CSyncAP *pAllocatorPresenter);
		~CSyncRenderer();

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppvObject);

		// IBaseFilter
		STDMETHODIMP EnumPins(__out IEnumPins **ppEnum);
		STDMETHODIMP FindPin(LPCWSTR Id, __out IPin **ppPin);
		STDMETHODIMP QueryFilterInfo(__out FILTER_INFO *pInfo);
		STDMETHODIMP JoinFilterGraph(__in_opt IFilterGraph *pGraph, __in_opt LPCWSTR pName);
		STDMETHODIMP QueryVendorInfo(__out LPWSTR *pVendorInfo);
		STDMETHODIMP Stop();
		STDMETHODIMP Pause();
		STDMETHODIMP Run(REFERENCE_TIME tStart);
		STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, __out FILTER_STATE *State);
		STDMETHODIMP SetSyncSource(__in_opt  IReferenceClock *pClock);
		STDMETHODIMP GetSyncSource(__deref_out_opt  IReferenceClock **pClock);
		STDMETHODIMP GetClassID(__RPC__out CLSID *pClassID);
	};

	// CGenlock

	class CGenlock
	{
		class MovingAverage
		{
		private:
			std::vector<double> fifo;
			unsigned oldestSample = 0;
			double   sum          = 0;

		public:
			MovingAverage(int size)
			{
				size = std::clamp(size, 1, MAX_FIFO_SIZE);
				fifo.resize(size);
			}

			double Average(double sample)
			{
				sum = sum + sample - fifo[oldestSample];
				fifo[oldestSample] = sample;
				oldestSample++;
				if (oldestSample == fifo.size()) {
					oldestSample = 0;
				}
				return sum / fifo.size();
			}
		};

	public:
		CGenlock(double target, double limit, double clockD, UINT mon);
		~CGenlock();

		HRESULT ResetClock(); // Reset reference clock speed to nominal
		HRESULT SetTargetSyncOffset(double targetD);
		HRESULT GetTargetSyncOffset(double *targetD);
		HRESULT SetControlLimit(double cL);
		HRESULT GetControlLimit(double *cL);
		HRESULT SetDisplayResolution(UINT columns, UINT lines);
		HRESULT AdviseSyncClock(ISyncClock* sC);
		HRESULT SetMonitor(UINT mon); // Set the number of the monitor to synchronize
		HRESULT ResetStats(); // Reset timing statistics

		HRESULT ControlClock(double syncOffset, double frameCycle); // Adjust the frequency of the clock if needed
		HRESULT UpdateStats(double syncOffset, double frameCycle); // Don't adjust anything, just update the syncOffset stats

	public:
		int    adjDelta               = 0; // -1 for display slower in relation to video, 0 for keep, 1 for faster
		UINT   clockAdjustmentsMade   = 0; // The number of adjustments made to clock frequency
		double minSyncOffset = 0.0;
		double maxSyncOffset = 0.0;
		double syncOffsetAvg = 0.0; // Average of the above
		double minFrameCycle = 0.0;
		double maxFrameCycle = 0.0;
		double frameCycleAvg = 0.0;

	private:
		BOOL liveSource = FALSE; // TRUE if live source -> display sync is the only option
		double cycleDelta; // Adjustment factor for cycle time as fraction of nominal value

		UINT visibleLines = 0, visibleColumns = 0; // The nominal resolution
		MovingAverage syncOffsetFifo{ 64 };
		MovingAverage frameCycleFifo{ 4 };

		double controlLimit; // How much the sync offset is allowed to drift from target sync offset
		UINT monitor; // The monitor to be controlled. 0-based.
		CComPtr<ISyncClock> syncClock; // Interface to an adjustable reference clock

		double lowSyncOffset; // The closest we want to let the scheduled render time to get to the next vsync. In % of the frame time
		double targetSyncOffset; // Where we want the scheduled render time to be in relation to the next vsync
		double highSyncOffset; // The furthers we want to let the scheduled render time to get to the next vsync
		CCritSec csGenlockLock;
	};
}
