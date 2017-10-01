/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include <d3d9.h>
#include <dxva.h>
#include <dxva2api.h>
#include <moreuuids.h>

#include "IPinHook.h"
#include "AllocatorCommon.h"
#include "../../../DSUtil/SysVersion.h"

#define DXVA_LOGFILE_A 0 // set to 1 for logging DXVA data to a file
#define LOG_BITSTREAM  0 // set to 1 for logging DXVA bistream data to a file
#define LOG_MATRIX     0 // set to 1 for logging DXVA matrix data to a file

#if defined(_DEBUG) && DXVA_LOGFILE_A
#define LOG_FILE_DXVA       L"dxva_ipinhook.log"
#define LOG_FILE_PICTURE    L"picture.log"
#define LOG_FILE_BITFIELDS  L"picture_BitFields.log"
#define LOG_FILE_SLICELONG  L"slicelong.log"
#define LOG_FILE_SLICESHORT L"sliceshort.log"
#define LOG_FILE_BITSTREAM  L"bitstream.log"
#endif

IPinCVtbl*         g_pPinCVtbl         = nullptr;
IMemInputPinCVtbl* g_pMemInputPinCVtbl = nullptr;
IPinC*             g_pPinC             = nullptr;

REFERENCE_TIME g_tSegmentStart    = 0;
FRAME_TYPE     g_nFrameType       = PICT_NONE;
HANDLE         g_hNewSegmentEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

namespace DXVAState {
	BOOL m_bDXVActive      = FALSE;
	GUID m_guidDXVADecoder = GUID_NULL;

	CString m_sDXVADecoderDescription      = L"Not using DXVA";
	CString m_sDXVADecoderShortDescription = L"DXVA";

	void ClearState()
	{
		m_bDXVActive      = FALSE;
		m_guidDXVADecoder = GUID_NULL;

		m_sDXVADecoderDescription      = L"Not using DXVA";
		m_sDXVADecoderShortDescription = L"DXVA";
	}

	void SetActiveState(const GUID& guidDXVADecoder, const CString& customDescription/* = L""*/)
	{
		m_bDXVActive = TRUE;
		m_guidDXVADecoder = guidDXVADecoder;

		m_sDXVADecoderDescription = !customDescription.IsEmpty() ? customDescription : GetDXVAMode(&m_guidDXVADecoder);
		m_sDXVADecoderShortDescription = (m_guidDXVADecoder != GUID_NULL) ? L"DXVA2" : L"H/W";
	}

	const BOOL GetState()
	{
		return m_bDXVActive;
	}

	const CString GetDescription()
	{
		return m_sDXVADecoderDescription;
	}

	const CString GetShortDescription()
	{
		return m_sDXVADecoderShortDescription;
	}
}

// DirectShow hooks
static HRESULT (STDMETHODCALLTYPE* NewSegmentOrg)(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate) PURE;
static HRESULT STDMETHODCALLTYPE NewSegmentMine(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate)
{
	if (g_pPinC == This) {
		g_tSegmentStart = tStart;
		g_nFrameType    = PICT_NONE;
		SetEvent(g_hNewSegmentEvent);
	}

	return NewSegmentOrg(This, tStart, tStop, dRate);
}

static HRESULT (STDMETHODCALLTYPE* ReceiveConnectionOrg)(IPinC* This, /* [in] */ IPinC* pConnector, /* [in] */ const AM_MEDIA_TYPE* pmt) PURE;
static HRESULT STDMETHODCALLTYPE ReceiveConnectionMine(IPinC* This, /* [in] */ IPinC* pConnector, /* [in] */ const AM_MEDIA_TYPE* pmt)
{
	if (pmt) {
		// Force the renderer to always reject the P010(except the DXVA) and P016 pixel format
		if (pmt->subtype == MEDIASUBTYPE_P010) {
			if (SysVersion::IsWin10orLater() || GetCLSID((IPin*)pConnector) == GUID_LAVVideoDecoder) {
				// 1) Windows 10 support software P010 input
				// 2) LAV Video Decoder used biCompression equal 'P010' for DXVA :(
				return ReceiveConnectionOrg(This, pConnector, pmt);
			}

			if (pmt->pbFormat) {
				VIDEOINFOHEADER2& vih2 = *(VIDEOINFOHEADER2*)pmt->pbFormat;
				BITMAPINFOHEADER* bih = &vih2.bmiHeader;
				switch (bih->biCompression) {
					case FCC('dxva'):
					case FCC('DXVA'):
					case FCC('DxVA'):
					case FCC('DXvA'):
						return ReceiveConnectionOrg(This, pConnector, pmt);
				}
			}

			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		if (pmt->subtype == MEDIASUBTYPE_P016) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return ReceiveConnectionOrg(This, pConnector, pmt);
}

static HRESULT (STDMETHODCALLTYPE* ReceiveOrg)(IMemInputPinC * This, IMediaSample *pSample) PURE;
static HRESULT STDMETHODCALLTYPE ReceiveMineI(IMemInputPinC * This, IMediaSample *pSample)
{
	if (pSample) {
		// Get frame type
		if (CComQIPtr<IMediaSample2> pMS2 = pSample) {
			AM_SAMPLE2_PROPERTIES props;
			if (SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))) {
				g_nFrameType = PICT_BOTTOM_FIELD;
				if (props.dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE) {
					g_nFrameType = PICT_FRAME;
				} else if (props.dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST) {
					g_nFrameType = PICT_TOP_FIELD;
				}
			}
		}
	}

	return ReceiveOrg(This, pSample);
}

static HRESULT STDMETHODCALLTYPE ReceiveMine(IMemInputPinC * This, IMediaSample *pSample)
{
	return ReceiveMineI(This, pSample);
}

void UnhookNewSegmentAndReceive()
{
	BOOL res;
	DWORD flOldProtect = 0;

	// Casimir666 : unhook previous VTables
	if (g_pPinCVtbl && g_pMemInputPinCVtbl) {
		res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
		if (g_pPinCVtbl->NewSegment == NewSegmentMine) {
			g_pPinCVtbl->NewSegment = NewSegmentOrg;
		}

		if (g_pPinCVtbl->ReceiveConnection == ReceiveConnectionMine) {
			g_pPinCVtbl->ReceiveConnection = ReceiveConnectionOrg;
		}

		res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

		res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
		if (g_pMemInputPinCVtbl->Receive == ReceiveMine) {
			g_pMemInputPinCVtbl->Receive = ReceiveOrg;
		}
		res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

		g_pPinCVtbl          = nullptr;
		g_pPinC              = nullptr;
		g_pMemInputPinCVtbl  = nullptr;
		NewSegmentOrg        = nullptr;
		ReceiveConnectionOrg = nullptr;
		ReceiveOrg           = nullptr;
	}
}

bool HookNewSegmentAndReceive(IPin* pPin)
{
	IPinC* pPinC = (IPinC*)pPin;
	CComQIPtr<IMemInputPin> pMemInputPin = pPin;
	IMemInputPinC* pMemInputPinC = (IMemInputPinC*)(IMemInputPin*)pMemInputPin;

	if (!pPinC || !pMemInputPinC) {
		return false;
	}

	g_tSegmentStart = 0;

	BOOL res;
	DWORD flOldProtect = 0;

	UnhookNewSegmentAndReceive();

	// Casimir666 : change sizeof(IPinC) to sizeof(IPinCVtbl) to fix crash with EVR hack on Vista!
	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
	if (NewSegmentOrg == nullptr) {
		NewSegmentOrg = pPinC->lpVtbl->NewSegment;
	}
	pPinC->lpVtbl->NewSegment = NewSegmentMine;

    if (ReceiveConnectionOrg == nullptr) {
        ReceiveConnectionOrg = pPinC->lpVtbl->ReceiveConnection;
    }
    pPinC->lpVtbl->ReceiveConnection = ReceiveConnectionMine;

	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

	// Casimir666 : change sizeof(IMemInputPinC) to sizeof(IMemInputPinCVtbl) to fix crash with EVR hack on Vista!
	res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
	if (ReceiveOrg == nullptr) {
		ReceiveOrg = pMemInputPinC->lpVtbl->Receive;
	}
	pMemInputPinC->lpVtbl->Receive = ReceiveMine;
	res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

	g_pPinCVtbl         = pPinC->lpVtbl;
	g_pPinC             = pPinC;
	g_pMemInputPinCVtbl = pMemInputPinC->lpVtbl;

	return true;
}

#if defined(_DEBUG) && DXVA_LOGFILE_A
static void LOG_TOFILE(LPCTSTR FileName, LPCTSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int nCount = _vsctprintf(fmt, args) + 1;
	if (WCHAR* buff = DNew WCHAR[nCount]) {
		FILE* f;
		vswprintf_s(buff, nCount, fmt, args);
		if (_wfopen_s(&f, FileName, L"at") == 0) {
			fseek(f, 0, 2);
			fwprintf_s(f, L"%s\n", buff);
			fclose(f);
		}
		delete [] buff;
	}
	va_end(args);
}

static void LOG(LPCTSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	//int   nCount = _vsctprintf(fmt, args) + 1;
	WCHAR buff[3000];
	FILE* f;
	vswprintf_s(buff, _countof(buff), fmt, args);
	if (_wfopen_s(&f, LOG_FILE_DXVA, L"at") == 0) {
		fseek(f, 0, 2);
		fwprintf_s(f, L"%s\n", buff);
		fclose(f);
	}

	va_end(args);
}

static void LogDXVA_PicParams_H264(DXVA_PicParams_H264* pPic)
{
	CString			strRes;
	int				i;
	static bool		bFirstParam	= true;

	if (bFirstParam) {
		LOG_TOFILE (LOG_FILE_PICTURE, L"RefPicFlag,wFrameWidthInMbsMinus1,wFrameHeightInMbsMinus1,CurrPic.Index7Bits,num_ref_frames,wBitFields,bit_depth_luma_minus8,bit_depth_chroma_minus8,Reserved16Bits,StatusReportFeedbackNumber,RFL.Index7Bits[0]," \
					L"RFL.Index7Bits[1],RFL.Index7Bits[2],RFL.Index7Bits[3],RFL.Index7Bits[4],RFL.Index7Bits[5]," \
					L"RFL.Index7Bits[6],RFL.Index7Bits[7],RFL.Index7Bits[8],RFL.Index7Bits[9],RFL.Index7Bits[10]," \
					L"RFL.Index7Bits[11],RFL.Index7Bits[12],RFL.Index7Bits[13],RFL.Index7Bits[14],RFL.Index7Bits[15]," \
					L"CurrFieldOrderCnt[0], CurrFieldOrderCnt[1],FieldOrderCntList[0][0], FieldOrderCntList[0][1],FieldOrderCntList[1][0], FieldOrderCntList[1][1],FieldOrderCntList[2][0], FieldOrderCntList[2][1],FieldOrderCntList[3][0], FieldOrderCntList[3][1],FieldOrderCntList[4][0], FieldOrderCntList[4][1],FieldOrderCntList[5][0]," \
					L"FieldOrderCntList[5][1],FieldOrderCntList[6][0], FieldOrderCntList[6][1],FieldOrderCntList[7][0], FieldOrderCntList[7][1],FieldOrderCntList[8][0], FieldOrderCntList[8][1],FieldOrderCntList[9][0], FieldOrderCntList[9][1],FieldOrderCntList[10][0], FieldOrderCntList[10][1],FieldOrderCntList[11][0],"\
					L"FieldOrderCntList[11][1],FieldOrderCntList[12][0], FieldOrderCntList[12][1],FieldOrderCntList[13][0], FieldOrderCntList[13][1],FieldOrderCntList[14][0], FieldOrderCntList[14][1],FieldOrderCntList[15][0], FieldOrderCntList[15][1],pic_init_qs_minus26,chroma_qp_index_offset,second_chroma_qp_index_offset,"\
					L"ContinuationFlag,pic_init_qp_minus26,num_ref_idx_l0_active_minus1,num_ref_idx_l1_active_minus1,Reserved8BitsA,FrameNumList[0],FrameNumList[1],FrameNumList[2],FrameNumList[3],FrameNumList[4],FrameNumList[5],FrameNumList[6],FrameNumList[7],FrameNumList[8],FrameNumList[9],FrameNumList[10],FrameNumList[11],"\
					L"FrameNumList[12],FrameNumList[13],FrameNumList[14],FrameNumList[15],UsedForReferenceFlags,NonExistingFrameFlags,frame_num,log2_max_frame_num_minus4,pic_order_cnt_type,log2_max_pic_order_cnt_lsb_minus4,delta_pic_order_always_zero_flag,direct_8x8_inference_flag,entropy_coding_mode_flag,pic_order_present_flag,"\
					L"num_slice_groups_minus1,slice_group_map_type,deblocking_filter_control_present_flag,redundant_pic_cnt_present_flag,Reserved8BitsB,slice_group_change_rate_minus1");

	}

	strRes.AppendFormat(L"%d,", pPic->RefPicFlag);
	strRes.AppendFormat(L"%d,", pPic->wFrameWidthInMbsMinus1);
	strRes.AppendFormat(L"%d,", pPic->wFrameHeightInMbsMinus1);

	//			DXVA_PicEntry_H264  CurrPic)); /* flag is bot field flag */
	//	strRes.AppendFormat(L"%d,", pPic->CurrPic.AssociatedFlag);
	//	strRes.AppendFormat(L"%d,", pPic->CurrPic.bPicEntry);
	strRes.AppendFormat(L"%d,", pPic->CurrPic.Index7Bits);


	strRes.AppendFormat(L"%d,", pPic->num_ref_frames);
	strRes.AppendFormat(L"%d,", pPic->wBitFields);
	strRes.AppendFormat(L"%d,", pPic->bit_depth_luma_minus8);
	strRes.AppendFormat(L"%d,", pPic->bit_depth_chroma_minus8);

	strRes.AppendFormat(L"%d,", pPic->Reserved16Bits);
	strRes.AppendFormat(L"%d,", pPic->StatusReportFeedbackNumber);

	for (i = 0; i < 16; i++) {
		//strRes.AppendFormat(L"%d,", pPic->RefFrameList[i].AssociatedFlag);
		//strRes.AppendFormat(L"%d,", pPic->RefFrameList[i].bPicEntry);
		strRes.AppendFormat(L"%d,", pPic->RefFrameList[i].Index7Bits);
	}

	strRes.AppendFormat(L"%d, %d,", pPic->CurrFieldOrderCnt[0], pPic->CurrFieldOrderCnt[1]);

	for (int i = 0; i < 16; i++) {
		strRes.AppendFormat(L"%d, %d,", pPic->FieldOrderCntList[i][0], pPic->FieldOrderCntList[i][1]);
	}
	//strRes.AppendFormat(L"%d,", pPic->FieldOrderCntList[16][2]);

	strRes.AppendFormat(L"%d,", pPic->pic_init_qs_minus26);
	strRes.AppendFormat(L"%d,", pPic->chroma_qp_index_offset);   /* also used for QScb */
	strRes.AppendFormat(L"%d,", pPic->second_chroma_qp_index_offset); /* also for QScr */
	strRes.AppendFormat(L"%d,", pPic->ContinuationFlag);

	/* remainder for parsing */
	strRes.AppendFormat(L"%d,", pPic->pic_init_qp_minus26);
	strRes.AppendFormat(L"%d,", pPic->num_ref_idx_l0_active_minus1);
	strRes.AppendFormat(L"%d,", pPic->num_ref_idx_l1_active_minus1);
	strRes.AppendFormat(L"%d,", pPic->Reserved8BitsA);

	for (int i=0; i<16; i++) {
		strRes.AppendFormat(L"%d,", pPic->FrameNumList[i]);
	}

	//strRes.AppendFormat(L"%d,", pPic->FrameNumList[16]);
	strRes.AppendFormat(L"%d,", pPic->UsedForReferenceFlags);
	strRes.AppendFormat(L"%d,", pPic->NonExistingFrameFlags);
	strRes.AppendFormat(L"%d,", pPic->frame_num);

	strRes.AppendFormat(L"%d,", pPic->log2_max_frame_num_minus4);
	strRes.AppendFormat(L"%d,", pPic->pic_order_cnt_type);
	strRes.AppendFormat(L"%d,", pPic->log2_max_pic_order_cnt_lsb_minus4);
	strRes.AppendFormat(L"%d,", pPic->delta_pic_order_always_zero_flag);

	strRes.AppendFormat(L"%d,", pPic->direct_8x8_inference_flag);
	strRes.AppendFormat(L"%d,", pPic->entropy_coding_mode_flag);
	strRes.AppendFormat(L"%d,", pPic->pic_order_present_flag);
	strRes.AppendFormat(L"%d,", pPic->num_slice_groups_minus1);

	strRes.AppendFormat(L"%d,", pPic->slice_group_map_type);
	strRes.AppendFormat(L"%d,", pPic->deblocking_filter_control_present_flag);
	strRes.AppendFormat(L"%d,", pPic->redundant_pic_cnt_present_flag);
	strRes.AppendFormat(L"%d,", pPic->Reserved8BitsB);

	strRes.AppendFormat(L"%d,", pPic->slice_group_change_rate_minus1);

	//for (int i=0; i<810; i++)
	//	strRes.AppendFormat(L"%d,", pPic->SliceGroupMap[i]);
	//			strRes.AppendFormat(L"%d,", pPic->SliceGroupMap[810]);

	// SABOTAGE !!!
	//for (int i=0; i<16; i++)
	//{
	//	pPic->FieldOrderCntList[i][0] =  pPic->FieldOrderCntList[i][1] = 0;
	//	pPic->RefFrameList[i].AssociatedFlag = 1;
	//	pPic->RefFrameList[i].bPicEntry = 255;
	//	pPic->RefFrameList[i].Index7Bits = 127;
	//}

	// === Dump PicParams!
	//static FILE* hPict = nullptr;
	//if (!hPict) hPict = fopen ("PicParam.bin", "wb");
	//if (hPict)
	//{
	//	fwrite (pPic, sizeof (DXVA_PicParams_H264), 1, hPict);
	//}

	LOG_TOFILE(LOG_FILE_PICTURE, strRes);

	if (bFirstParam) {
		LOG_TOFILE (LOG_FILE_BITFIELDS, L"field_pic_flag,"\
					L"MbaffFrameFlag, residual_colour_transform_flag, sp_for_switch_flag," \
					L"chroma_format_idc, RefPicFlag, constrained_intra_pred_flag," \
					L"weighted_pred_flag, weighted_bipred_idc, MbsConsecutiveFlag," \
					L"frame_mbs_only_flag, transform_8x8_mode_flag, MinLumaBipredSize8x8Flag, IntraPicFlag");
	}

	strRes.Empty();
	strRes.AppendFormat(L"%d,", pPic->field_pic_flag);
	strRes.AppendFormat(L"%d,", pPic->MbaffFrameFlag);
	strRes.AppendFormat(L"%d,", pPic->residual_colour_transform_flag);
	strRes.AppendFormat(L"%d,", pPic->sp_for_switch_flag);
	strRes.AppendFormat(L"%d,", pPic->chroma_format_idc);
	strRes.AppendFormat(L"%d,", pPic->RefPicFlag);
	strRes.AppendFormat(L"%d,", pPic->constrained_intra_pred_flag);
	strRes.AppendFormat(L"%d,", pPic->weighted_pred_flag);
	strRes.AppendFormat(L"%d,", pPic->weighted_bipred_idc);
	strRes.AppendFormat(L"%d,", pPic->MbsConsecutiveFlag);
	strRes.AppendFormat(L"%d,", pPic->frame_mbs_only_flag);
	strRes.AppendFormat(L"%d,", pPic->transform_8x8_mode_flag);
	strRes.AppendFormat(L"%d,", pPic->MinLumaBipredSize8x8Flag);
	strRes.AppendFormat(L"%d,", pPic->IntraPicFlag);

	LOG_TOFILE(LOG_FILE_BITFIELDS, strRes);

	bFirstParam = false;

}

static void LogH264SliceShort(DXVA_Slice_H264_Short* pSlice, int nCount)
{
	CString		strRes;
	static bool	bFirstSlice = true;

	if (bFirstSlice) {
		strRes = L"nCnt, BSNALunitDataLocation, SliceBytesInBuffer, wBadSliceChopping";
		LOG_TOFILE (LOG_FILE_SLICESHORT, strRes);
		strRes = "";
		bFirstSlice = false;
	}

	for (int i = 0; i < nCount; i++) {
		strRes.AppendFormat(L"%d,", i);
		strRes.AppendFormat(L"%d,", pSlice[i].BSNALunitDataLocation);
		strRes.AppendFormat(L"%d,", pSlice[i].SliceBytesInBuffer);
		strRes.AppendFormat(L"%d", pSlice[i].wBadSliceChopping);

		LOG_TOFILE (LOG_FILE_SLICESHORT, strRes);
		strRes = "";
	}
}

static void LogSliceInfo(DXVA_SliceInfo* pSlice, int nCount)
{
	CString		strRes;
	static bool	bFirstSlice = true;

	if (bFirstSlice) {
		strRes = L"nCnt, wHorizontalPosition, wVerticalPosition, dwSliceBitsInBuffer,dwSliceDataLocation, bStartCodeBitOffset, bReservedBits, wMBbitOffset, wNumberMBsInSlice, wQuantizerScaleCode, wBadSliceChopping";

		LOG_TOFILE (LOG_FILE_SLICESHORT, strRes);
		strRes = "";
		bFirstSlice = false;
	}

	for (int i = 0; i < nCount; i++) {
		strRes.AppendFormat(L"%d,", i);
		strRes.AppendFormat(L"%d,", pSlice[i].wHorizontalPosition);
		strRes.AppendFormat(L"%d,", pSlice[i].wVerticalPosition);
		strRes.AppendFormat(L"%d,", pSlice[i].dwSliceBitsInBuffer);
		strRes.AppendFormat(L"%d,", pSlice[i].dwSliceDataLocation);
		strRes.AppendFormat(L"%d,", pSlice[i].bStartCodeBitOffset);
		strRes.AppendFormat(L"%d,", pSlice[i].bReservedBits);
		strRes.AppendFormat(L"%d,", pSlice[i].wMBbitOffset);
		strRes.AppendFormat(L"%d,", pSlice[i].wNumberMBsInSlice);
		strRes.AppendFormat(L"%d,", pSlice[i].wQuantizerScaleCode);
		strRes.AppendFormat(L"%d",  pSlice[i].wBadSliceChopping);

		LOG_TOFILE (LOG_FILE_SLICESHORT, strRes);
		strRes = "";
	}
}

static void LogH264SliceLong(DXVA_Slice_H264_Long* pSlice, int nCount)
{
	static bool	bFirstSlice = true;
	CString		strRes;

	if (bFirstSlice) {
		strRes = L"nCnt, BSNALunitDataLocation, SliceBytesInBuffer, wBadSliceChopping," \
				 L"first_mb_in_slice, NumMbsForSlice, BitOffsetToSliceData, slice_type,luma_log2_weight_denom,chroma_log2_weight_denom," \
				 L"num_ref_idx_l0_active_minus1,num_ref_idx_l1_active_minus1,slice_alpha_c0_offset_div2,slice_beta_offset_div2," \
				 L"Reserved8Bits,slice_qs_delta,slice_qp_delta,redundant_pic_cnt,direct_spatial_mv_pred_flag,cabac_init_idc," \
				 L"disable_deblocking_filter_idc,slice_id,";

		for (int i=0; i<2; i++) {	/* L0 & L1 */
			for (int j=0; j<32; j++) {
				strRes.AppendFormat(L"R[%d][%d].AssociatedFlag,", i, j);
				strRes.AppendFormat(L"R[%d][%d].bPicEntry,",		 i, j);
				strRes.AppendFormat(L"R[%d][%d].Index7Bits,",	 i, j);
			}
		}

		for (int a=0; a<2; a++) {	/* L0 & L1; Y, Cb, Cr */
			for (int b=0; b<32; b++) {
				for (int c=0; c<3; c++) {
					for (int d=0; d<2; d++) {
						strRes.AppendFormat(L"W[%d][%d][%d][%d],", a,b,c,d);
					}
				}
			}
		}


		LOG_TOFILE (LOG_FILE_SLICELONG, strRes);
		strRes = "";
	}
	bFirstSlice = false;

	for (int i = 0; i < nCount; i++) {
		strRes.AppendFormat(L"%d,", i);
		strRes.AppendFormat(L"%d,", pSlice[i].BSNALunitDataLocation);
		strRes.AppendFormat(L"%d,", pSlice[i].SliceBytesInBuffer);
		strRes.AppendFormat(L"%d,", pSlice[i].wBadSliceChopping);

		strRes.AppendFormat(L"%d,", pSlice[i].first_mb_in_slice);
		strRes.AppendFormat(L"%d,", pSlice[i].NumMbsForSlice);

		strRes.AppendFormat(L"%d,", pSlice[i].BitOffsetToSliceData);

		strRes.AppendFormat(L"%d,", pSlice[i].slice_type);
		strRes.AppendFormat(L"%d,", pSlice[i].luma_log2_weight_denom);
		strRes.AppendFormat(L"%d,", pSlice[i].chroma_log2_weight_denom);
		strRes.AppendFormat(L"%d,", pSlice[i].num_ref_idx_l0_active_minus1);
		strRes.AppendFormat(L"%d,", pSlice[i].num_ref_idx_l1_active_minus1);
		strRes.AppendFormat(L"%d,", pSlice[i].slice_alpha_c0_offset_div2);
		strRes.AppendFormat(L"%d,", pSlice[i].slice_beta_offset_div2);
		strRes.AppendFormat(L"%d,", pSlice[i].Reserved8Bits);

		strRes.AppendFormat(L"%d,", pSlice[i].slice_qs_delta);

		strRes.AppendFormat(L"%d,", pSlice[i].slice_qp_delta);
		strRes.AppendFormat(L"%d,", pSlice[i].redundant_pic_cnt);
		strRes.AppendFormat(L"%d,", pSlice[i].direct_spatial_mv_pred_flag);
		strRes.AppendFormat(L"%d,", pSlice[i].cabac_init_idc);
		strRes.AppendFormat(L"%d,", pSlice[i].disable_deblocking_filter_idc);
		strRes.AppendFormat(L"%d,", pSlice[i].slice_id);

		for (int a=0; a<2; a++) {	/* L0 & L1 */
			for (int b=0; b<32; b++) {
				strRes.AppendFormat(L"%d,", pSlice[i].RefPicList[a][b].AssociatedFlag);
				strRes.AppendFormat(L"%d,", pSlice[i].RefPicList[a][b].bPicEntry);
				strRes.AppendFormat(L"%d,", pSlice[i].RefPicList[a][b].Index7Bits);
			}
		}

		for (int a=0; a<2; a++) {	/* L0 & L1; Y, Cb, Cr */
			for (int b=0; b<32; b++) {
				for (int c=0; c<3; c++) {
					for (int d=0; d<2; d++) {
						strRes.AppendFormat(L"%d,", pSlice[i].Weights[a][b][c][d]);
					}
				}
			}
		}

		LOG_TOFILE (LOG_FILE_SLICELONG, strRes);
		strRes = "";
	}
}

static void LogDXVA_PictureParameters(DXVA_PictureParameters* pPic)
{
	static bool	bFirstPictureParam = true;
	CString		strRes;

	if (bFirstPictureParam) {
		LOG_TOFILE (LOG_FILE_PICTURE, L"wDecodedPictureIndex,wDeblockedPictureIndex,wForwardRefPictureIndex,wBackwardRefPictureIndex,wPicWidthInMBminus1,wPicHeightInMBminus1,bMacroblockWidthMinus1,bMacroblockHeightMinus1,bBlockWidthMinus1,bBlockHeightMinus1,bBPPminus1,bPicStructure,bSecondField,bPicIntra,bPicBackwardPrediction,bBidirectionalAveragingMode,bMVprecisionAndChromaRelation,bChromaFormat,bPicScanFixed,bPicScanMethod,bPicReadbackRequests,bRcontrol,bPicSpatialResid8,bPicOverflowBlocks,bPicExtrapolation,bPicDeblocked,bPicDeblockConfined,bPic4MVallowed,bPicOBMC,bPicBinPB,bMV_RPS,bReservedBits,wBitstreamFcodes,wBitstreamPCEelements,bBitstreamConcealmentNeed,bBitstreamConcealmentMethod");
	}
	bFirstPictureParam = false;

	strRes.Format (L"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				   pPic->wDecodedPictureIndex,
				   pPic->wDeblockedPictureIndex,
				   pPic->wForwardRefPictureIndex,
				   pPic->wBackwardRefPictureIndex,
				   pPic->wPicWidthInMBminus1,
				   pPic->wPicHeightInMBminus1,
				   pPic->bMacroblockWidthMinus1,
				   pPic->bMacroblockHeightMinus1,
				   pPic->bBlockWidthMinus1,
				   pPic->bBlockHeightMinus1,
				   pPic->bBPPminus1,
				   pPic->bPicStructure,
				   pPic->bSecondField,
				   pPic->bPicIntra,
				   pPic->bPicBackwardPrediction,
				   pPic->bBidirectionalAveragingMode,
				   pPic->bMVprecisionAndChromaRelation,
				   pPic->bChromaFormat,
				   pPic->bPicScanFixed,
				   pPic->bPicScanMethod,
				   pPic->bPicReadbackRequests,
				   pPic->bRcontrol,
				   pPic->bPicSpatialResid8,
				   pPic->bPicOverflowBlocks,
				   pPic->bPicExtrapolation,
				   pPic->bPicDeblocked,
				   pPic->bPicDeblockConfined,
				   pPic->bPic4MVallowed,
				   pPic->bPicOBMC,
				   pPic->bPicBinPB,
				   pPic->bMV_RPS,
				   pPic->bReservedBits,
				   pPic->wBitstreamFcodes,
				   pPic->wBitstreamPCEelements,
				   pPic->bBitstreamConcealmentNeed,
				   pPic->bBitstreamConcealmentMethod);

	LOG_TOFILE (LOG_FILE_PICTURE, strRes);
}

void LogDXVA_Bitstream(BYTE* pBuffer, int nSize)
{
	CString		strRes;
	static bool	bFirstBitstream = true;

	if (bFirstBitstream) {
		LOG_TOFILE (LOG_FILE_BITSTREAM, L"Size,Start, Stop");
	}
	bFirstBitstream = false;

	strRes.Format (L"%d, -", nSize);

	for (int i=0; i<20; i++) {
		if (i < nSize) {
			strRes.AppendFormat (L" %02x", pBuffer[i]);
		} else {
			strRes.Append(L" --");
		}
	}

	strRes.Append (L", -", nSize);
	for (int i=0; i<20; i++) {
		if (nSize-i >= 0) {
			strRes.AppendFormat (L" %02x", pBuffer[i]);
		} else {
			strRes.Append(L" --");
		}
	}

	LOG_TOFILE (LOG_FILE_BITSTREAM, strRes);

}

#else
inline static void LOG(...) {}
inline static void LogDXVA_PicParams_H264(DXVA_PicParams_H264* pPic) {}
inline static void LogDXVA_PictureParameters(DXVA_PictureParameters* pPic) {}
inline static void LogDXVA_Bitstream(BYTE* pBuffer, int nSize) {}
#endif

// DXVA2 hooks

#ifdef _DEBUG

#define MAX_BUFFER_TYPE 15

static void LogDecodeBufferDesc(DXVA2_DecodeBufferDesc* pDecodeBuff)
{
	LOG(L"DecodeBufferDesc type : %d   Size=%d   NumMBsInBuffer=%d", pDecodeBuff->CompressedBufferType, pDecodeBuff->DataSize, pDecodeBuff->NumMBsInBuffer);
}

class CFakeDirectXVideoDecoder : public CUnknown, public IDirectXVideoDecoder
{
private :
	CComPtr<IDirectXVideoDecoder> m_pDec;
	BYTE*                         m_ppBuffer[MAX_BUFFER_TYPE];
	UINT                          m_ppBufferLen[MAX_BUFFER_TYPE];

public :
	CFakeDirectXVideoDecoder(LPUNKNOWN pUnk, IDirectXVideoDecoder* pDec)
		: CUnknown(L"Fake DXVA2 Dec", pUnk)
	{
		m_pDec.Attach(pDec);
		memset(m_ppBuffer, 0, sizeof(m_ppBuffer));
	}

	~CFakeDirectXVideoDecoder() {
		LOG(L"CFakeDirectXVideoDecoder destroyed !\n");
	}

	DECLARE_IUNKNOWN;

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
		if (riid == __uuidof(IDirectXVideoDecoder)) {
			return GetInterface((IDirectXVideoDecoder*)this, ppv);
		} else {
			return __super::NonDelegatingQueryInterface(riid, ppv);
		}
	}

	virtual HRESULT STDMETHODCALLTYPE GetVideoDecoderService(IDirectXVideoDecoderService **ppService) {
		HRESULT hr = m_pDec->GetVideoDecoderService(ppService);
		LOG(L"IDirectXVideoDecoder::GetVideoDecoderService  hr = %08x\n", hr);
		return hr;
	}

	virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets, UINT *pNumSurfaces) {
		HRESULT hr = m_pDec->GetCreationParameters(pDeviceGuid, pVideoDesc, pConfig, pDecoderRenderTargets, pNumSurfaces);
		LOG(L"IDirectXVideoDecoder::GetCreationParameters hr = %08x\n", hr);
		return hr;
	}


	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT BufferType, void **ppBuffer, UINT *pBufferSize) {
		HRESULT hr = m_pDec->GetBuffer(BufferType, ppBuffer, pBufferSize);

		if (BufferType < MAX_BUFFER_TYPE) {
			m_ppBuffer[BufferType]    = (BYTE*)*ppBuffer;
			m_ppBufferLen[BufferType] = *pBufferSize;
		}
		//			LOG(L"IDirectXVideoDecoder::GetBuffer Type = %d,  hr = %08x", BufferType, hr);

		return hr;
	}

	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT BufferType) {
		HRESULT hr = m_pDec->ReleaseBuffer(BufferType);
		//			LOG(L"IDirectXVideoDecoder::ReleaseBuffer Type = %d,  hr = %08x", BufferType, hr);
		return hr;
	}

	virtual HRESULT STDMETHODCALLTYPE BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData) {
		HRESULT hr = m_pDec->BeginFrame(pRenderTarget, pvPVPData);
		LOG(L"IDirectXVideoDecoder::BeginFrame pRenderTarget = %08x,  hr = %08x", pRenderTarget, hr);
		return hr;
	}

	virtual HRESULT STDMETHODCALLTYPE EndFrame(HANDLE *pHandleComplete) {
		HRESULT hr = m_pDec->EndFrame(pHandleComplete);
		LOG(L"IDirectXVideoDecoder::EndFrame  Handle=0x%08x  hr = %08x\n", pHandleComplete, hr);
		return hr;
	}

	virtual HRESULT STDMETHODCALLTYPE Execute(const DXVA2_DecodeExecuteParams *pExecuteParams) {

#if defined(_DEBUG) && DXVA_LOGFILE_A
		for (DWORD i = 0; i < pExecuteParams->NumCompBuffers; i++) {
			CString		strBuffer;

			LogDecodeBufferDesc (&pExecuteParams->pCompressedBuffers[i]);
			/*
			for (int j=0; j<4000 && j<pExecuteParams->pCompressedBuffers[i].DataSize; j++)
				strBuffer.AppendFormat (L"%02x ", m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType][j]);

			LOG (L" - Buffer type=%d,  offset=%d, size=%d",
				pExecuteParams->pCompressedBuffers[i].CompressedBufferType,
				pExecuteParams->pCompressedBuffers[i].DataOffset,
				pExecuteParams->pCompressedBuffers[i].DataSize);

			LOG (strBuffer);*/

			if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_PictureParametersBufferType) {
				if (DXVAState::m_guidDXVADecoder == DXVA2_ModeH264_E || DXVAState::m_guidDXVADecoder == DXVA_Intel_H264_ClearVideo) {
					LogDXVA_PicParams_H264((DXVA_PicParams_H264*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType]);
				} else if (DXVAState::m_guidDXVADecoder == DXVA2_ModeVC1_D || DXVAState::m_guidDXVADecoder == DXVA2_ModeMPEG2_VLD || DXVAState::m_guidDXVADecoder == DXVA_Intel_VC1_ClearVideo || DXVAState::m_guidDXVADecoder == DXVA2_ModeVC1_D2010) {
					LogDXVA_PictureParameters((DXVA_PictureParameters*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType]);
				}
			}

			if (DXVAState::m_guidDXVADecoder == DXVA2_ModeH264_E || DXVAState::m_guidDXVADecoder == DXVA_Intel_H264_ClearVideo) {
				if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_SliceControlBufferType) {
					if (pExecuteParams->pCompressedBuffers[i].DataSize % sizeof(DXVA_Slice_H264_Long) == 0) {
						DXVA_Slice_H264_Long*	pSlice = (DXVA_Slice_H264_Long*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType];
						LogH264SliceLong(pSlice, pExecuteParams->pCompressedBuffers[i].DataSize / sizeof(DXVA_Slice_H264_Long));
					} else if (pExecuteParams->pCompressedBuffers[i].DataSize % sizeof(DXVA_Slice_H264_Short) == 0) {
						DXVA_Slice_H264_Short*	pSlice = (DXVA_Slice_H264_Short*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType];
						LogH264SliceShort(pSlice, pExecuteParams->pCompressedBuffers[i].DataSize / sizeof(DXVA_Slice_H264_Short));
					}
				}
			} else if (DXVAState::m_guidDXVADecoder == DXVA2_ModeVC1_D || DXVAState::m_guidDXVADecoder == DXVA2_ModeMPEG2_VLD || DXVAState::m_guidDXVADecoder == DXVA_Intel_VC1_ClearVideo || DXVAState::m_guidDXVADecoder == DXVA2_ModeVC1_D2010) {
				if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_SliceControlBufferType) {
					DXVA_SliceInfo*	pSlice = (DXVA_SliceInfo*)m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType];
					LogSliceInfo(pSlice, pExecuteParams->pCompressedBuffers[i].DataSize / sizeof(DXVA_SliceInfo));
				}
			}

#if LOG_MATRIX
			if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_InverseQuantizationMatrixBufferType) {
				char strFile[MAX_PATH];
				static int nNb = 1;
				sprintf_s(strFile, "Matrix%d.bin", nNb++);
				FILE* hFile = fopen(strFile, "wb");
				if (hFile) {
					fwrite(m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType],
						   1,
						   pExecuteParams->pCompressedBuffers[i].DataSize,
						   hFile);
					fclose(hFile);
				}
			}
#endif

			if (pExecuteParams->pCompressedBuffers[i].CompressedBufferType == DXVA2_BitStreamDateBufferType) {
				LogDXVA_Bitstream(m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType], pExecuteParams->pCompressedBuffers[i].DataSize);

#if LOG_BITSTREAM
				char strFile[MAX_PATH];
				static int nNb = 1;
				sprintf_s(strFile, "BitStream%d.bin", nNb++);
				FILE* hFile = fopen(strFile, "wb");
				if (hFile) {
					fwrite(m_ppBuffer[pExecuteParams->pCompressedBuffers[i].CompressedBufferType],
						   1,
						   pExecuteParams->pCompressedBuffers[i].DataSize,
						   hFile);
					fclose(hFile);
				}
#endif
			}
		}
#endif

		HRESULT hr = m_pDec->Execute (pExecuteParams);

#ifdef _DEBUG
		if (pExecuteParams->pExtensionData) {
			LOG(L"IDirectXVideoDecoder::Execute  %d buffer, fct = %d  (in=%d, out=%d),  hr = %08x",
				pExecuteParams->NumCompBuffers,
				pExecuteParams->pExtensionData->Function,
				pExecuteParams->pExtensionData->PrivateInputDataSize,
				pExecuteParams->pExtensionData->PrivateOutputDataSize,
				hr);
		} else {
			LOG(L"IDirectXVideoDecoder::Execute  %d buffer, hr = %08x", pExecuteParams->NumCompBuffers, hr);
		}
#endif
		return hr;
	}
};
#endif


interface IDirectXVideoDecoderServiceC;
struct IDirectXVideoDecoderServiceCVtbl {
	BEGIN_INTERFACE
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IDirectXVideoDecoderServiceC* pThis, /* [in] */ REFIID riid, /* [iid_is][out] */ void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )(IDirectXVideoDecoderServiceC* pThis);
	ULONG ( STDMETHODCALLTYPE *Release )(IDirectXVideoDecoderServiceC*	pThis);
	HRESULT (STDMETHODCALLTYPE* CreateSurface)(IDirectXVideoDecoderServiceC* pThis, __in  UINT Width, __in  UINT Height, __in  UINT BackBuffers, __in  D3DFORMAT Format, __in  D3DPOOL Pool, __in  DWORD Usage, __in  DWORD DxvaType, __out_ecount(BackBuffers+1)  IDirect3DSurface9 **ppSurface, __inout_opt  HANDLE *pSharedHandle);

	HRESULT (STDMETHODCALLTYPE* GetDecoderDeviceGuids)(
		IDirectXVideoDecoderServiceC*					pThis,
		__out  UINT*									pCount,
		__deref_out_ecount_opt(*pCount)  GUID**			pGuids);

	HRESULT (STDMETHODCALLTYPE* GetDecoderRenderTargets)(
		IDirectXVideoDecoderServiceC*					pThis,
		__in  REFGUID									Guid,
		__out  UINT*									pCount,
		__deref_out_ecount_opt(*pCount)  D3DFORMAT**	pFormats);

	HRESULT (STDMETHODCALLTYPE* GetDecoderConfigurations)(
		IDirectXVideoDecoderServiceC*					pThis,
		__in  REFGUID									Guid,
		__in  const DXVA2_VideoDesc*					pVideoDesc,
		__reserved  void*								pReserved,
		__out  UINT*									pCount,
		__deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs);

	HRESULT (STDMETHODCALLTYPE* CreateVideoDecoder)(
		IDirectXVideoDecoderServiceC*					pThis,
		__in  REFGUID									Guid,
		__in  const DXVA2_VideoDesc*					pVideoDesc,
		__in  const DXVA2_ConfigPictureDecode*			pConfig,
		__in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
		__in  UINT										NumRenderTargets,
		__deref_out  IDirectXVideoDecoder**				ppDecode);

	END_INTERFACE
};

interface IDirectXVideoDecoderServiceC {
	CONST_VTBL struct IDirectXVideoDecoderServiceCVtbl *lpVtbl;
};

IDirectXVideoDecoderServiceCVtbl* g_pIDirectXVideoDecoderServiceCVtbl = nullptr;
static HRESULT (STDMETHODCALLTYPE* CreateVideoDecoderOrg )(IDirectXVideoDecoderServiceC* pThis, __in  REFGUID Guid, __in  const DXVA2_VideoDesc* pVideoDesc, __in  const DXVA2_ConfigPictureDecode* pConfig, __in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets, __in  UINT NumRenderTargets, __deref_out  IDirectXVideoDecoder** ppDecode) PURE;
#ifdef _DEBUG
static HRESULT (STDMETHODCALLTYPE* GetDecoderDeviceGuidsOrg)(IDirectXVideoDecoderServiceC* pThis, __out  UINT* pCount, __deref_out_ecount_opt(*pCount)  GUID** pGuids) PURE;
static HRESULT (STDMETHODCALLTYPE* GetDecoderConfigurationsOrg)(IDirectXVideoDecoderServiceC* pThis, __in  REFGUID Guid, __in const DXVA2_VideoDesc* pVideoDesc, __reserved void* pReserved, __out UINT* pCount, __deref_out_ecount_opt(*pCount)  DXVA2_ConfigPictureDecode **ppConfigs) PURE;
#endif

#ifdef _DEBUG
static void LogDXVA2Config(const DXVA2_ConfigPictureDecode* pConfig)
{
	LOG(L"Config");
	LOG(L"    - Config4GroupedCoefs               %d", pConfig->Config4GroupedCoefs);
	LOG(L"    - ConfigBitstreamRaw                %d", pConfig->ConfigBitstreamRaw);
	LOG(L"    - ConfigDecoderSpecific             %d", pConfig->ConfigDecoderSpecific);
	LOG(L"    - ConfigHostInverseScan             %d", pConfig->ConfigHostInverseScan);
	LOG(L"    - ConfigIntraResidUnsigned          %d", pConfig->ConfigIntraResidUnsigned);
	LOG(L"    - ConfigMBcontrolRasterOrder        %d", pConfig->ConfigMBcontrolRasterOrder);
	LOG(L"    - ConfigMinRenderTargetBuffCount    %d", pConfig->ConfigMinRenderTargetBuffCount);
	LOG(L"    - ConfigResid8Subtraction           %d", pConfig->ConfigResid8Subtraction);
	LOG(L"    - ConfigResidDiffAccelerator        %d", pConfig->ConfigResidDiffAccelerator);
	LOG(L"    - ConfigResidDiffHost               %d", pConfig->ConfigResidDiffHost);
	LOG(L"    - ConfigSpatialHost8or9Clipping     %d", pConfig->ConfigSpatialHost8or9Clipping);
	LOG(L"    - ConfigSpatialResid8               %d", pConfig->ConfigSpatialResid8);
	LOG(L"    - ConfigSpatialResidInterleaved     %d", pConfig->ConfigSpatialResidInterleaved);
	LOG(L"    - ConfigSpecificIDCT                %d", pConfig->ConfigSpecificIDCT);
	LOG(L"    - guidConfigBitstreamEncryption     %s", CStringFromGUID(pConfig->guidConfigBitstreamEncryption));
	LOG(L"    - guidConfigMBcontrolEncryption     %s", CStringFromGUID(pConfig->guidConfigMBcontrolEncryption));
	LOG(L"    - guidConfigResidDiffEncryption     %s", CStringFromGUID(pConfig->guidConfigResidDiffEncryption));
}

static void LogDXVA2VideoDesc(const DXVA2_VideoDesc* pVideoDesc)
{
	LOG(L"VideoDesc");
	LOG(L"    - Format                            %s  (0x%08x)", D3DFormatToString(pVideoDesc->Format), pVideoDesc->Format);
	LOG(L"    - InputSampleFreq                   %d/%d", pVideoDesc->InputSampleFreq.Numerator, pVideoDesc->InputSampleFreq.Denominator);
	LOG(L"    - OutputFrameFreq                   %d/%d", pVideoDesc->OutputFrameFreq.Numerator, pVideoDesc->OutputFrameFreq.Denominator);
	LOG(L"    - SampleFormat                      %d", pVideoDesc->SampleFormat.value);
	LOG(L"    - SampleHeight                      %d", pVideoDesc->SampleHeight);
	LOG(L"    - SampleWidth                       %d", pVideoDesc->SampleWidth);
	LOG(L"    - UABProtectionLevel                %d", pVideoDesc->UABProtectionLevel);
}
#endif

static HRESULT STDMETHODCALLTYPE CreateVideoDecoderMine(IDirectXVideoDecoderServiceC*					pThis,
														__in  REFGUID									Guid,
														__in  const DXVA2_VideoDesc*					pVideoDesc,
														__in  const DXVA2_ConfigPictureDecode*			pConfig,
														__in_ecount(NumRenderTargets)  IDirect3DSurface9 **ppDecoderRenderTargets,
														__in  UINT										NumRenderTargets,
														__deref_out  IDirectXVideoDecoder**				ppDecode)
{
	//	DebugBreak();
	//	((DXVA2_VideoDesc*)pVideoDesc)->Format = (D3DFORMAT)0x3231564E;
	DXVAState::SetActiveState(Guid);

#ifdef _DEBUG
	LOG(L"\n\n");
	LogDXVA2VideoDesc(pVideoDesc);
	LogDXVA2Config(pConfig);
#endif

	HRESULT hr = CreateVideoDecoderOrg(pThis, Guid, pVideoDesc, pConfig, ppDecoderRenderTargets, NumRenderTargets, ppDecode);

	if (FAILED(hr)) {
		DXVAState::ClearState();
	}
#ifdef _DEBUG
	else {
		if ((Guid == DXVA2_ModeH264_E) ||
				(Guid == DXVA2_ModeVC1_D)  ||
				(Guid == DXVA2_Intel_H264_ClearVideo) ||
				(Guid == DXVA2_Intel_VC1_ClearVideo) ||
				(Guid == DXVA2_ModeVC1_D2010) ||
				(Guid == DXVA2_ModeMPEG2_VLD)) {
			*ppDecode = DNew CFakeDirectXVideoDecoder(nullptr, *ppDecode);
			(*ppDecode)->AddRef();
		}

		for (UINT i = 0; i < NumRenderTargets; i++) {
			LOG(L" - Surf %d : %08x", i, ppDecoderRenderTargets[i]);
		}
	}
#endif

	DLog(L"DXVA Decoder : %s", DXVAState::GetDescription());
#ifdef _DEBUG
	LOG(L"IDirectXVideoDecoderService::CreateVideoDecoder  %s  (%d render targets) hr = %08x", DXVAState::GetDescription(), NumRenderTargets, hr);
#endif
	return hr;
}

#ifdef _DEBUG
static HRESULT STDMETHODCALLTYPE GetDecoderDeviceGuidsMine(IDirectXVideoDecoderServiceC*			pThis,
															__out  UINT*							pCount,
															__deref_out_ecount_opt(*pCount) GUID**	pGuids)
{
	HRESULT hr = GetDecoderDeviceGuidsOrg(pThis, pCount, pGuids);
	LOG(L"IDirectXVideoDecoderService::GetDecoderDeviceGuids  hr = %08x\n", hr);

	return hr;
}

static HRESULT STDMETHODCALLTYPE GetDecoderConfigurationsMine(IDirectXVideoDecoderServiceC*		pThis,
															  __in  REFGUID						Guid,
															  __in  const DXVA2_VideoDesc*		pVideoDesc,
															  __reserved  void*					pReserved,
															  __out  UINT*						pCount,
															  __deref_out_ecount_opt(*pCount)	DXVA2_ConfigPictureDecode **ppConfigs)
{
	HRESULT hr = GetDecoderConfigurationsOrg(pThis, Guid, pVideoDesc, pReserved, pCount, ppConfigs);

	// Force LongSlice decoding !
	//	memcpy (&(*ppConfigs)[1], &(*ppConfigs)[0], sizeof(DXVA2_ConfigPictureDecode));

	return hr;
}
#endif

void HookDirectXVideoDecoderService(void* pIDirectXVideoDecoderService)
{
	IDirectXVideoDecoderServiceC* pIDirectXVideoDecoderServiceC = (IDirectXVideoDecoderServiceC*) pIDirectXVideoDecoderService;

	BOOL res;
	DWORD flOldProtect = 0;

	// Casimir666 : unhook previous VTables
	if (g_pIDirectXVideoDecoderServiceCVtbl) {
		res = VirtualProtect(g_pIDirectXVideoDecoderServiceCVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), PAGE_WRITECOPY, &flOldProtect);
		if (g_pIDirectXVideoDecoderServiceCVtbl->CreateVideoDecoder == CreateVideoDecoderMine) {
			g_pIDirectXVideoDecoderServiceCVtbl->CreateVideoDecoder = CreateVideoDecoderOrg;
		}

#ifdef _DEBUG
		if (g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderConfigurations == GetDecoderConfigurationsMine) {
			g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderConfigurations = GetDecoderConfigurationsOrg;
		}

		//if (g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderDeviceGuids == GetDecoderDeviceGuidsMine)
		//	g_pIDirectXVideoDecoderServiceCVtbl->GetDecoderDeviceGuids = GetDecoderDeviceGuidsOrg;
#endif

		res = VirtualProtect(g_pIDirectXVideoDecoderServiceCVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), flOldProtect, &flOldProtect);

		g_pIDirectXVideoDecoderServiceCVtbl = nullptr;
		CreateVideoDecoderOrg               = nullptr;
#ifdef _DEBUG
		GetDecoderConfigurationsOrg         = nullptr;
#endif
		DXVAState::ClearState();
	}

#if defined(_DEBUG) && DXVA_LOGFILE_A
	::DeleteFile (LOG_FILE_DXVA);
	::DeleteFile (LOG_FILE_PICTURE);
	::DeleteFile (LOG_FILE_SLICELONG);
#endif

	if (!g_pIDirectXVideoDecoderServiceCVtbl && pIDirectXVideoDecoderService) {
		res = VirtualProtect(pIDirectXVideoDecoderServiceC->lpVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), PAGE_WRITECOPY, &flOldProtect);

		CreateVideoDecoderOrg = pIDirectXVideoDecoderServiceC->lpVtbl->CreateVideoDecoder;
		pIDirectXVideoDecoderServiceC->lpVtbl->CreateVideoDecoder = CreateVideoDecoderMine;

#ifdef _DEBUG
		GetDecoderConfigurationsOrg = pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderConfigurations;
		pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderConfigurations = GetDecoderConfigurationsMine;

		//GetDecoderDeviceGuidsOrg = pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderDeviceGuids;
		//pIDirectXVideoDecoderServiceC->lpVtbl->GetDecoderDeviceGuids = GetDecoderDeviceGuidsMine;
#endif

		res = VirtualProtect(pIDirectXVideoDecoderServiceC->lpVtbl, sizeof(IDirectXVideoDecoderServiceCVtbl), flOldProtect, &flOldProtect);

		g_pIDirectXVideoDecoderServiceCVtbl = pIDirectXVideoDecoderServiceC->lpVtbl;
	}
}
