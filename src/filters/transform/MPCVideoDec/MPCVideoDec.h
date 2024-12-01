/*
 * (C) 2006-2024 see Authors.txt
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
#include "filters/filters/FilterInterfacesImpl.h"
#include "IMPCVideoDec.h"
#include "MPCVideoDecSettingsWnd.h"
#include "./DXVADecoder/DXVA2Decoder.h"
#include "FormatConverter.h"
#include "apps/mplayerc/FilterEnum.h"

#include <IMediaSideData.h>
#include <basestruct.h>
#include <mpc_defines.h>
#include <d3d11.h>

#include "./MSDKDecoder/MSDKDecoder.h"

#define MPCVideoDecName L"MPC Video Decoder"
#define MPCVideoConvName L"MPC Video Converter"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
struct AVFrame;
struct AVPacket;
struct AVBufferRef;
class CD3D11Decoder;

class __declspec(uuid("008BAC12-FBAF-497b-9670-BC6F6FBAE2C4"))
	CMPCVideoDecFilter
	: public CBaseVideoFilter
	, public IMPCVideoDecFilter
	, public CExFilterConfigImpl
	, public ISpecifyPropertyPages2
{
private:
	enum class HwType {
		None,
		DXVA2,
		D3D11,
		D3D11CopyBack,
		D3D12CopyBack,
		NVDEC
	};

	CCritSec								m_csInitDec;
	CCritSec								m_csProps;
	// === Persistants parameters (registry)
	int										m_nThreadNumber = 0;
	MPC_SCAN_TYPE							m_nScanType = SCAN_AUTO;
	int										m_nARMode = 2;
	int										m_nDiscardMode;
	MPCHwDecoder							m_nHwDecoder;
	MPC_ADAPTER_ID							m_HwAdapter = {};
	bool									m_bHwCodecs[HWCodec_count];
	int										m_nDXVACheckCompatibility = 1;
	int										m_nDXVA_SD = 0;
	bool									m_fPixFmts[PixFmt_count];
	int										m_nSwRGBLevels = 0;
	//
	bool									m_VideoFilters[VDEC_COUNT] = {};

	bool									m_bEnableHwDecoding  = true; // internal (not saved)
	bool									m_bDXVACompatible = true;
	unsigned __int64						m_nActiveCodecs = CODECS_ALL & ~CODEC_H264_MVC;

	// === FFMpeg variables
	const AVCodec*							m_pAVCodec = nullptr;
	AVCodecContext*							m_pAVCtx   = nullptr;
	AVCodecParserContext*					m_pParser  = nullptr;
	AVFrame*								m_pFrame   = nullptr;
	AVFrame*								m_pHWFrame = nullptr;
	enum AVCodecID							m_CodecId;
	REFERENCE_TIME							m_rtAvrTimePerFrame = 0;
	bool									m_bCalculateStopTime = false;

	BYTE*									m_pFFBuffer = nullptr;
	unsigned int							m_nFFBufferSize = 0;

	bool									m_bReorderBFrame = false;
	struct Timings {
		REFERENCE_TIME rtStart;
		REFERENCE_TIME rtStop;
	} m_tBFrameDelay[2];
	int										m_nBFramePos = 0;

	bool									m_bWaitKeyFrame = false;

	int										m_nARX = 0, m_nARY = 0;

	REFERENCE_TIME							m_rtLastStart = INVALID_TIME; // rtStart for last delivered frame
	REFERENCE_TIME							m_rtLastStop  = 0;            // rtStop for last delivered frame
	double									m_dRate = 1.0;

	bool									m_bUseFFmpeg = true;
	CFormatConverter						m_FormatConverter;
	CSize									m_pOutSize; // Picture size on output pin

	HwType									m_hwType = {};

	AVPixelFormat							m_HWPixFmt;
	AVBufferRef*							m_HWDeviceCtx = nullptr;
	CComPtr<ID3D11Texture2D>				m_pStagingD3D11Texture2D;

	// === common variables
	std::vector<VIDEO_OUTPUT_FORMATS>		m_VideoOutputFormats;
	std::unique_ptr<CDXVA2Decoder>			m_pDXVADecoder;
	GUID									m_DXVADecoderGUID = GUID_NULL;
	D3DFORMAT								m_DXVASurfaceFormat = D3DFMT_UNKNOWN;

	UINT									m_nPCIVendor;
	UINT									m_nPCIDevice;
	UINT64									m_VideoDriverVersion;
	CString									m_strDeviceDescription;

	// === DXVA2 variables
	CComPtr<IDirect3DDeviceManager9>		m_pDeviceManager;
	CComPtr<IDirectXVideoDecoderService>	m_pDecoderService;
	DXVA2_ConfigPictureDecode				m_DXVA2Config;
	HANDLE									m_hDevice = INVALID_HANDLE_VALUE;
	DXVA2_VideoDesc							m_VideoDesc;

	BOOL									m_bFailDXVA2Decode = FALSE;
	BOOL									m_bFailD3D11Decode = FALSE;

	bool									m_bReinit = false;

	BOOL									m_bWaitingForKeyFrame  = TRUE;
	BOOL									m_bRVDropBFrameTimings = FALSE;

	REFERENCE_TIME							m_rtStartCache = INVALID_TIME;

	DWORD									m_dwSYNC  = 0;
	DWORD									m_dwSYNC2 = 0;

	CMediaType								m_pCurrentMediaType;
	DXVA2_ExtendedFormat					m_inputDxvaExtFormat = {};

	BOOL									m_bDecodingStart = FALSE;
	BOOL									m_bDecoderAcceptFormat = FALSE;

	bool									m_bHighBitdepth = false;

	std::unique_ptr<CMSDKDecoder>			m_pMSDKDecoder;
	int										m_iMvcOutputMode = MVC_OUTPUT_Auto;
	bool									m_bMvcSwapLR     = false;

	BOOL									m_MVC_Base_View_R_flag = FALSE;

	CLSID									m_OutputFilterClsid = GUID_NULL;

	struct {
		MediaSideDataHDR*                  masterDataHDR        = nullptr;
		MediaSideDataHDRContentLightLevel* HDRContentLightLevel = nullptr;
		int                                profile              = -1;
		int                                pix_fmt              = -1;

		int                                interlaced           = -1; // 0 - Progressive, 1 - Interlaced TFF, 2 - Interlaced BFF

		int                                color_primaries        = -1;
		int                                colorspace             = -1;
		int                                color_trc              = -1;
		int                                chroma_sample_location = -1;
		int                                color_range            = -1;

		void Clear() {
			SAFE_DELETE(masterDataHDR);
			SAFE_DELETE(HDRContentLightLevel)
			profile = -1;
			pix_fmt = -1;

			interlaced = -1;

			color_primaries        = -1;
			colorspace             = -1;
			color_trc              = -1;
			chroma_sample_location = -1;
			color_range            = -1;
		}

	} m_FilterInfo;

	bool m_bHasPalette = false;
	uint32_t m_Palette[256] = {};

	std::unique_ptr<CD3D11Decoder> m_pD3D11Decoder;

	// === Private functions
	void			Cleanup();
	void			CleanupD3DResources();
	void			CleanupDXVAVariables();
	void			CleanupFFmpeg();
	int				FindCodec(const CMediaType* mtIn, BOOL bForced = FALSE);
	void			AllocExtradata(const CMediaType* mt);
	void			GetOutputFormats (int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats);
	void			DetectVideoCard(HWND hWnd);
	void			BuildOutputFormat();

	HRESULT			FillAVPacket(AVPacket *avpkt, const BYTE *buffer, int buflen);
	HRESULT			DecodeInternal(AVPacket *avpkt, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bPreroll = FALSE);
	HRESULT			ParseInternal(const BYTE *buffer, int buflen, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bPreroll);
	HRESULT			Decode(const BYTE *buffer, int buflen, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bSyncPoint = FALSE, BOOL bPreroll = FALSE);
	HRESULT			ChangeOutputMediaFormat(int nType);

	void			SetThreadCount();
	HRESULT			FindDecoderConfiguration();

	HRESULT			InitDecoder(const CMediaType *pmt);

	int				m_nAlign = 16;
	int				m_nSurfaceWidth  = 0;
	int				m_nSurfaceHeight = 0;

	AVPixelFormat	m_dxva_pix_fmt;

	HRESULT						CheckDXVA2Decoder(AVCodecContext *c);

	static int					av_get_buffer(struct AVCodecContext *c, AVFrame *pic, int flags);
	static enum AVPixelFormat	av_get_format(struct AVCodecContext *c, const enum AVPixelFormat* pix_fmts);

	bool						CheckDXVACompatible(const enum AVCodecID codec, const enum AVPixelFormat pix_fmt, const int profile);

public:
	CMPCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMPCVideoDecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	CTransformOutputPin* GetOutputPin() { return m_pOutput; }

	REFERENCE_TIME	GetFrameDuration();
	void			UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	void			GetFrameTimeStamp(AVFrame* pFrame, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
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
	STDMETHODIMP SetScanType(MPC_SCAN_TYPE nValue);
	STDMETHODIMP_(MPC_SCAN_TYPE) GetScanType();
	STDMETHODIMP SetARMode(int nValue);
	STDMETHODIMP_(int) GetARMode();

	STDMETHODIMP SetHwCodec(MPCHwCodec hwcodec, bool enable);
	STDMETHODIMP_(bool) GetHwCodec(MPCHwCodec hwcodec);
	STDMETHODIMP SetHwDecoder(int value);
	STDMETHODIMP_(int) GetHwDecoder();
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

	STDMETHODIMP GetD3D11Adapter(MPC_ADAPTER_ID* pAdapterId);
	STDMETHODIMP SetD3D11Adapter(UINT VendorId, UINT DeviceId);

	// IExFilterConfig
	STDMETHODIMP Flt_GetInt(LPCSTR field, int* value) override;
	STDMETHODIMP Flt_GetInt64(LPCSTR field, __int64* value) override;
	STDMETHODIMP Flt_GetString(LPCSTR field, LPWSTR* value, unsigned* chars);
	STDMETHODIMP Flt_SetBool(LPCSTR field, bool value) override;
	STDMETHODIMP Flt_SetInt(LPCSTR field, int value) override;

	// === common functions
	BOOL						IsSupportedDecoderConfig(const D3DFORMAT& nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered);
	BOOL						IsSupportedDecoderMode(const GUID& decoderGUID);
	void						GetPictSize(int& width, int& height);

	DXVA2_ExtendedFormat		GetDXVA2ExtendedFormat(const AVCodecContext *ctx, const AVFrame *frame);

	inline bool					UseDXVA2() const { return m_nDecoderMode == MODE_DXVA2; }
	inline bool					UseD3D11() const { return m_nDecoderMode == MODE_D3D11; }

	bool						IsDXVASupported(const bool bMode);
	void						UpdateAspectRatio();
	void						FlushDXVADecoder();
	void						SetTypeSpecificFlags(IMediaSample* pMS);

	// === DXVA2 functions
	void						FillInVideoDescription(DXVA2_VideoDesc& videoDesc, D3DFORMAT Format = D3DFMT_A8R8G8B8);
	HRESULT						ConfigureDXVA2(IPin *pPin);
	HRESULT						SetEVRForDXVA2(IPin *pPin);
	HRESULT						FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
															  const GUID& guidDecoder,
															  DXVA2_ConfigPictureDecode& selectedConfig,
															  D3DFORMAT& surfaceFormat,
															  BOOL& bFoundDXVA2Configuration);
	HRESULT						CreateDXVA2Decoder(LPDIRECT3DSURFACE9* ppDecoderRenderTargets, UINT nNumRenderTargets);
	HRESULT						ReinitDXVA2Decoder();

	HRESULT						InitAllocator(IMemAllocator **ppAlloc);

	// === EVR functions
	HRESULT						DetectVideoCard_EVR(IPin *pPin);

	// === Codec functions
	HRESULT						SetFFMpegCodec(int nCodec, bool bEnabled);

private:
	friend class CVideoDecDXVAAllocator;
	friend class CDXVA2Decoder;
	friend class CMSDKDecoder;
	friend class CD3D11Decoder;

	BOOL m_bInInit = FALSE;

	CVideoDecDXVAAllocator* m_pDXVA2Allocator = nullptr;

	// *** from LAV
	// *** Re-Commit the allocator (creates surfaces and new decoder)
	HRESULT						RecommitAllocator();
};

//
// CVideoDecOutputPin
//

class CVideoDecOutputPin : public CBaseVideoOutputPin
{
	CMPCVideoDecFilter* m_pVideoDecFilter;
public:
	CVideoDecOutputPin(LPCWSTR pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	~CVideoDecOutputPin();

	HRESULT InitAllocator(IMemAllocator **ppAlloc);

	DECLARE_IUNKNOWN
};

namespace MPCVideoDec {
	struct FORMAT {
		const CLSID* clsMajorType;
		const CLSID* clsMinorType;

		const int    FFMPEGCode;
	};
	typedef std::list<FORMAT> FORMATS;

	void GetSupportedFormatList(FORMATS& fmts);
} // namespace MPCVideoDec
