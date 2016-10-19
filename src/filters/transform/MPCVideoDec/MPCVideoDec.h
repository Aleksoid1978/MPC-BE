/*
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

#include "../BaseVideoFilter/BaseVideoFilter.h"
#include "IMPCVideoDec.h"
#include "MPCVideoDecSettingsWnd.h"
#include "./DXVADecoder/DXVA2Decoder.h"
#include "FormatConverter.h"
#include "../../../apps/mplayerc/FilterEnum.h"

#include <IMediaSideData.h>

#include "./MSDKDecoder/MSDKDecoder.h"

#define MPCVideoDecName L"MPC Video Decoder"
#define MPCVideoConvName L"MPC Video Converter"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
struct AVFrame;

class __declspec(uuid("008BAC12-FBAF-497b-9670-BC6F6FBAE2C4"))
	CMPCVideoDecFilter
	: public CBaseVideoFilter
	, public IMPCVideoDecFilter
	, public ISpecifyPropertyPages2
{
protected:
	// === Persistants parameters (registry)
	int										m_nThreadNumber;
	int										m_nDiscardMode;
	MPC_DEINTERLACING_FLAGS					m_nDeinterlacing;
	int										m_nARMode;
	int										m_nDXVACheckCompatibility;
	int										m_nDXVA_SD;
	bool									m_fPixFmts[PixFmt_count];
	int										m_nSwRGBLevels;
	//

	CCritSec								m_csProps;

	bool									m_DXVAFilters[VDEC_DXVA_LAST];
	bool									m_VideoFilters[VDEC_LAST];

	bool									m_bDXVACompatible;
	unsigned __int64						m_nActiveCodecs;
	enum AVPixelFormat						m_PixelFormat;

	BOOL									m_bInterlaced;

	// === FFMpeg variables
	AVCodec*								m_pAVCodec;
	AVCodecContext*							m_pAVCtx;
	AVCodecParserContext*					m_pParser;
	AVFrame*								m_pFrame;
	int										m_nCodecNb;
	enum AVCodecID							m_nCodecId;
	int										m_nWorkaroundBug;
	int										m_nErrorConcealment;
	REFERENCE_TIME							m_rtAvrTimePerFrame;
	bool									m_bCalculateStopTime;

	bool									m_bWaitKeyFrame;

	int										m_nARX, m_nARY;

	// Buffer management for truncated stream (store stream chunks & reference time sent by splitter)
	BYTE*									m_pFFBuffer;
	int										m_nFFBufferSize;
	BYTE*									m_pFFBuffer2;
	int										m_nFFBufferSize2;

	REFERENCE_TIME							m_rtLastStart;			// rtStart for last delivered frame
	REFERENCE_TIME							m_rtLastStop;			// rtStop  for last delivered frame
	double									m_dRate;

	bool									m_bUseDXVA;
	bool									m_bUseFFmpeg;
	CFormatConverter						m_FormatConverter;
	CSize									m_pOutSize;				// Picture size on output pin

	// === common variables
	VIDEO_OUTPUT_FORMATS*					m_pVideoOutputFormat;
	int										m_nVideoOutputCount;
	CDXVA2Decoder*							m_pDXVADecoder;
	GUID									m_DXVADecoderGUID;

	DWORD									m_nPCIVendor;
	DWORD									m_nPCIDevice;
	UINT64									m_VideoDriverVersion;
	CString									m_strDeviceDescription;

	// === DXVA1 variables
	DDPIXELFORMAT							m_DDPixelFormat;

	// === DXVA2 variables
	CComPtr<IDirect3DDeviceManager9>		m_pDeviceManager;
	CComPtr<IDirectXVideoDecoderService>	m_pDecoderService;
	DXVA2_ConfigPictureDecode				m_DXVA2Config;
	HANDLE									m_hDevice;
	DXVA2_VideoDesc							m_VideoDesc;

	BOOL									m_bWaitingForKeyFrame;
	BOOL									m_bRVDropBFrameTimings;

	REFERENCE_TIME							m_rtStart;
	REFERENCE_TIME							m_rtStartCache;
	REFERENCE_TIME							m_rtStopCache;

	BOOL									m_bUsePTS;
	BOOL									m_bCorrectPTS;

	DWORD									m_fSYNC;

	CMediaType								m_pCurrentMediaType;

	BOOL									m_bDecodingStart;

	BOOL									m_bHEVC10bit;

	CMSDKDecoder*							m_pMSDKDecoder;
	int										m_iMvcOutputMode;
	bool									m_bMvcSwapLR;

	BOOL									m_MVC_Base_View_R_flag;

	// === Private functions
	void			Cleanup();
	void			ffmpegCleanup();
	int				FindCodec(const CMediaType* mtIn, BOOL bForced = FALSE);
	void			AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* mt);
	void			GetOutputFormats (int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats);
	void			DetectVideoCard(HWND hWnd);
	void			BuildOutputFormat();

	HRESULT			Decode(IMediaSample* pIn, BYTE* pDataIn, int nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	HRESULT			ChangeOutputMediaFormat(int nType);

	HRESULT			ReopenVideo();
	void			SetThreadCount();
	HRESULT			FindDecoderConfiguration();

	HRESULT			InitDecoder(const CMediaType *pmt);

	int				m_nAlign = 16;
	int				m_nSurfaceWidth  = 0;
	int				m_nSurfaceHeight = 0;

	static int					av_get_buffer(struct AVCodecContext *c, AVFrame *pic, int flags);
	static enum AVPixelFormat	av_get_format(struct AVCodecContext *s, const enum AVPixelFormat * pix_fmts);

public:
	CMPCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMPCVideoDecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	CTransformOutputPin* GetOutputPin() { return m_pOutput; };

	REFERENCE_TIME	GetFrameDuration();
	void			UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	void			GetFrameTimeStamp(AVFrame* pFrame, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	bool			IsAVI();
	bool			AddFrameSideData(IMediaSample* pSample, AVFrame* pFrame);

	// === Overriden DirectShow functions
	HRESULT			SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	HRESULT			CheckInputType(const CMediaType* mtIn);
	HRESULT			CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT			Transform(IMediaSample* pIn);
	HRESULT			CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);
	HRESULT			DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT			BeginFlush();
	HRESULT			EndFlush();
	HRESULT			NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);
	HRESULT			EndOfStream();

	HRESULT			BreakConnect(PIN_DIRECTION dir);

	// === ISpecifyPropertyPages2

	STDMETHODIMP	GetPages(CAUUID* pPages);
	STDMETHODIMP	CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// === IMPCVideoDecFilter
	STDMETHODIMP SetThreadNumber(int nValue);
	STDMETHODIMP_(int) GetThreadNumber();
	STDMETHODIMP SetDiscardMode(int nValue);
	STDMETHODIMP_(int) GetDiscardMode();
	STDMETHODIMP SetDeinterlacing(MPC_DEINTERLACING_FLAGS nValue);
	STDMETHODIMP_(MPC_DEINTERLACING_FLAGS) GetDeinterlacing();
	STDMETHODIMP SetARMode(int nValue);
	STDMETHODIMP_(int) GetARMode();

	STDMETHODIMP SetDXVACheckCompatibility(int nValue);
	STDMETHODIMP_(int) GetDXVACheckCompatibility();
	STDMETHODIMP SetDXVA_SD(int nValue);
	STDMETHODIMP_(int) GetDXVA_SD();

	STDMETHODIMP SetSwRefresh(int nValue);
	STDMETHODIMP SetSwPixelFormat(MPCPixelFormat pf, bool enable);
	STDMETHODIMP_(bool) GetSwPixelFormat(MPCPixelFormat pf);
	STDMETHODIMP SetSwRGBLevels(int nValue);
	STDMETHODIMP_(int) GetSwRGBLevels();

	STDMETHODIMP SetActiveCodecs(ULONGLONG nValue);
	STDMETHODIMP_(ULONGLONG) GetActiveCodecs();

	STDMETHODIMP SaveSettings();

	STDMETHODIMP_(CString) GetInformation(MPCInfo index);

	STDMETHODIMP_(GUID*) GetDXVADecoderGuid();
	STDMETHODIMP_(int) GetColorSpaceConversion();

	STDMETHODIMP SetMvcOutputMode(int nMode, bool bSwapLR);
	STDMETHODIMP_(int) GetMvcActive();

	// === common functions
	BOOL						IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered);
	BOOL						IsSupportedDecoderMode(const GUID* mode);
	int							GetPicEntryNumber();
	int							PictWidth();
	int							PictHeight();
	int							PictWidthAligned();
	int							PictHeightAligned();

	DXVA2_ExtendedFormat		GetDXVA2ExtendedFormat(AVCodecContext *ctx, AVFrame *frame);

	inline bool					UseDXVA2()			const { return (m_nDecoderMode == MODE_DXVA2); };
	inline AVCodecContext*		GetAVCtx()			const { return m_pAVCtx; };
	inline AVFrame*				GetFrame()			const { return m_pFrame; };
	inline enum AVCodecID		GetCodec()			const { return m_nCodecId; };
	inline DWORD				GetPCIVendor()		const { return m_nPCIVendor; };
	inline DWORD				GetPCIDevice()		const { return m_nPCIDevice; };

	bool						IsDXVASupported();
	void						UpdateAspectRatio();
	void						FlushDXVADecoder();
	void						SetTypeSpecificFlags(IMediaSample* pMS);

	// === DXVA2 functions
	void						FillInVideoDescription(DXVA2_VideoDesc *pDesc, D3DFORMAT Format = D3DFMT_A8R8G8B8);
	HRESULT						ConfigureDXVA2(IPin *pPin);
	HRESULT						SetEVRForDXVA2(IPin *pPin);
	HRESULT						FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
															  const GUID& guidDecoder,
															  DXVA2_ConfigPictureDecode *pSelectedConfig,
															  BOOL *pbFoundDXVA2Configuration);
	HRESULT						CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets);
	HRESULT						InitAllocator(IMemAllocator **ppAlloc);

	// === EVR functions
	HRESULT						DetectVideoCard_EVR(IPin *pPin);

	// === Codec functions
	HRESULT						SetFFMpegCodec(int nCodec, bool bEnabled);
	HRESULT						SetDXVACodec(int nCodec, bool bEnabled);

private:
	friend class CVideoDecDXVAAllocator;
	friend class CMSDKDecoder;
	CVideoDecDXVAAllocator*		m_pDXVA2Allocator;

	// *** from LAV
	// *** Re-Commit the allocator (creates surfaces and new decoder)
	HRESULT						RecommitAllocator();
};

class CVideoDecOutputPin : public CBaseVideoOutputPin
{
	CMPCVideoDecFilter* m_pVideoDecFilter;
public:
	CVideoDecOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	~CVideoDecOutputPin();

	HRESULT InitAllocator(IMemAllocator **ppAlloc);

	DECLARE_IUNKNOWN
};

namespace MPCVideoDec {
	struct FORMAT {
		const CLSID* clsMajorType;
		const CLSID* clsMinorType;

		const int    FFMPEGCode;
		const int    DXVACode;
	};
	typedef CAtlList<FORMAT> FORMATS;

	void GetSupportedFormatList(FORMATS& fmts);
} // namespace MPCVideoDec
