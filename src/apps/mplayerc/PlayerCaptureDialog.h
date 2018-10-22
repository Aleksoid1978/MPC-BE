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

#pragma once

#include "../../filters/transform/BufferFilter/BufferFilter.h"
#include "controls/FloatEdit.h"
#include <ResizableLib/ResizableDialog.h>
#include "PlayerBar.h"


class CMainFrame;

template<class T>
struct CFormatElem
{
	CMediaType mt;
	T caps;
};

template<class T>
class CFormat : public CAutoPtrArray<CFormatElem<T> >
{
public:
	CString name;
	CFormat(CString name = L"") : name(name) {}
	virtual ~CFormat() {}
};

template<class T>
class CFormatArray : public CAutoPtrArray<CFormat<T> >
{
public:
	virtual ~CFormatArray() {}

	CFormat<T>* Find(CString name, bool fCreate = false) {
		for (size_t i = 0; i < GetCount(); ++i) {
			if (GetAt(i)->name == name) {
				return GetAt(i);
			}
		}

		if (fCreate) {
			CAutoPtr<CFormat<T>> pf(DNew CFormat<T>(name));
			CFormat<T>* tmp = pf;
			Add(pf);
			return tmp;
		}

		return nullptr;
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, CFormat<T>** ppf) {
		if (!pmt) {
			return false;
		}

		for (size_t i = 0; i < GetCount(); ++i) {
			CFormat<T>* pf = GetAt(i);
			for (size_t j = 0; j < pf->GetCount(); ++j) {
				CFormatElem<T>* pfe = pf->GetAt(j);
				if (!pmt || (pfe->mt.majortype == pmt->majortype && pfe->mt.subtype == pmt->subtype)) {
					if (ppf) {
						*ppf = pf;
					}
					return true;
				}
			}
		}

		return false;
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, T* pcaps, CFormat<T>** ppf, CFormatElem<T>** ppfe) {
		if (!pmt && !pcaps) {
			return false;
		}

		for (size_t i = 0; i < GetCount(); ++i) {
			CFormat<T>* pf = GetAt(i);
			for (size_t j = 0; j < pf->GetCount(); ++j) {
				CFormatElem<T>* pfe = pf->GetAt(j);
				if ((!pmt || pfe->mt == *pmt) && (!pcaps || !memcmp(pcaps, &pfe->caps, sizeof(T)))) {
					if (ppf) {
						*ppf = pf;
					}
					if (ppfe) {
						*ppfe = pfe;
					}
					return true;
				}
			}
		}

		return false;
	}

	bool AddFormat(AM_MEDIA_TYPE* pmt, T caps) {
		if (!pmt) {
			return false;
		}

		if (FindFormat(pmt, nullptr, nullptr, nullptr)) {
			DeleteMediaType(pmt);
			return false;
		}
		//if (pmt->formattype == FORMAT_VideoInfo2) {DeleteMediaType(pmt); return false;} // TODO

		CFormat<T>* pf = Find(MakeFormatName(pmt), true);
		if (!pf) {
			DeleteMediaType(pmt);
			return false;
		}

		CAutoPtr<CFormatElem<T> > pfe(DNew CFormatElem<T>());
		pfe->mt = *pmt;
		pfe->caps = caps;
		pf->Add(pfe);

		return true;
	}

	bool AddFormat(AM_MEDIA_TYPE* pmt, void* pcaps, int size) {
		if (!pcaps) {
			return false;
		}
		ASSERT(size == sizeof(T));
		return AddFormat(pmt, *(T*)pcaps);
	}

	virtual CString MakeFormatName(AM_MEDIA_TYPE* pmt) PURE;
	virtual CString MakeDimensionName(CFormatElem<T>* pfe) PURE;
};

typedef CFormatElem<VIDEO_STREAM_CONFIG_CAPS> CVidFormatElem;
typedef CFormat<VIDEO_STREAM_CONFIG_CAPS> CVidFormat;

class CVidFormatArray : public CFormatArray<VIDEO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt) {
		CString str(L"Default");

		if (!pmt) {
			return str;
		}

		BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
								? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
								: (pmt->formattype == FORMAT_VideoInfo2)
								? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
								: nullptr;

		if (!bih) {
			// it may have a fourcc in the mediasubtype, let's check that

			WCHAR guid[100];
			ZeroMemory(guid, 100 * sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if (CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0) {
				str.Format(L"%c%c%c%c",
						   (WCHAR)((pmt->subtype.Data1>>0)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>8)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>16)&0xff),
						   (WCHAR)((pmt->subtype.Data1>>24)&0xff));
			}

			return str;
		}

		switch (bih->biCompression) {
			case BI_RGB:
				str.Format(L"RGB%u", bih->biBitCount);
				break;
			case BI_RLE8:
				str = L"RLE8";
				break;
			case BI_RLE4:
				str = L"RLE4";
				break;
			case BI_BITFIELDS:
				str.Format(L"BITF%u", bih->biBitCount);
				break;
			case BI_JPEG:
				str = L"JPEG";
				break;
			case BI_PNG:
				str = L"PNG";
				break;
			default:
				str.Format(L"%c%c%c%c",
						   (WCHAR)((bih->biCompression>>0)&0xff),
						   (WCHAR)((bih->biCompression>>8)&0xff),
						   (WCHAR)((bih->biCompression>>16)&0xff),
						   (WCHAR)((bih->biCompression>>24)&0xff));
				break;
		}

		return str;
	}

	CString MakeDimensionName(CVidFormatElem* pfe) {
		CString str(L"Default");

		if (!pfe) {
			return str;
		}

		BITMAPINFOHEADER* bih = (pfe->mt.formattype == FORMAT_VideoInfo)
								? &((VIDEOINFOHEADER*)pfe->mt.pbFormat)->bmiHeader
								: (pfe->mt.formattype == FORMAT_VideoInfo2)
								? &((VIDEOINFOHEADER2*)pfe->mt.pbFormat)->bmiHeader
								: nullptr;

		if (bih == nullptr) {
			return str;
		}

		str.Format(L"%ldx%ld %.2f", bih->biWidth, bih->biHeight, 10000000.0f / ((VIDEOINFOHEADER*)pfe->mt.pbFormat)->AvgTimePerFrame);

		if (pfe->mt.formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pfe->mt.pbFormat;
			str.AppendFormat(L" i%02x %u:%u", vih2->dwInterlaceFlags, vih2->dwPictAspectRatioX, vih2->dwPictAspectRatioY);
		}

		return str;
	}
};

typedef CFormatElem<AUDIO_STREAM_CONFIG_CAPS> CAudFormatElem;
typedef CFormat<AUDIO_STREAM_CONFIG_CAPS> CAudFormat;

class CAudFormatArray : public CFormatArray<AUDIO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt) {
		CString str(L"Unknown");

		if (!pmt) {
			return str;
		}

		WAVEFORMATEX* wfe = (pmt->formattype == FORMAT_WaveFormatEx)
							? (WAVEFORMATEX*)pmt->pbFormat
							: nullptr;

		if (!wfe) {
			WCHAR guid[100];
			ZeroMemory(guid, 100 * sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if (CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0) {
				str.Format(L"0x%04x", pmt->subtype.Data1);
			}

			return str;
		}

		switch (wfe->wFormatTag) {
			case 1:
				str = L"PCM";
				break;
			default:
				str.Format(L"0x%03x", wfe->wFormatTag);
				break;
		}

		return str;
	}

	CString MakeDimensionName(CAudFormatElem* pfe) {
		CString str(L"Unknown");

		if (!pfe) {
			return str;
		}

		WAVEFORMATEX* wfe = (pfe->mt.formattype == FORMAT_WaveFormatEx)
							? (WAVEFORMATEX*)pfe->mt.pbFormat
							: nullptr;

		if (!wfe) {
			return str;
		}

		str.Format(L"%6uKHz %ubps", wfe->nSamplesPerSec, wfe->wBitsPerSample);

		switch (wfe->nChannels) {
			case 1:
				str += L" mono";
				break;
			case 2:
				str += L" stereo";
				break;
			default:
				str.AppendFormat(L" %u channels", wfe->nChannels);
				break;
		}

		str.AppendFormat(L" %3ukbps", wfe->nAvgBytesPerSec*8/1000);

		return str;
	}
};

//

struct Codec {
	CComPtr<IMoniker> pMoniker;
	CComPtr<IBaseFilter> pBF;
	CString friendlyName;
	CComBSTR displayName;
};

// CPlayerCaptureDialog dialog

class CPlayerCaptureDialog : public CResizableDialog
{
	//DECLARE_DYNAMIC(CPlayerCaptureDialog)

private:
	CMainFrame* m_pMainFrame;
	bool m_bInitialized;

	CComboBox m_vidinput;
	CComboBox m_vidtype;
	CComboBox m_viddimension;
	CSpinButtonCtrl m_vidhor;
	CSpinButtonCtrl m_vidver;
	CEdit m_vidhoredit;
	CEdit m_vidveredit;
	CFloatEdit m_vidfpsedit;
	float m_vidfps;
	CButton m_vidsetres;
	CComboBox m_audinput;
	CComboBox m_audtype;
	CComboBox m_auddimension;
	CComboBox m_vidcodec;
	CComboBox m_vidcodectype;
	CComboBox m_vidcodecdimension;
	CButton m_vidoutput;
	CButton m_vidpreview;
	CComboBox m_audcodec;
	CComboBox m_audcodectype;
	CComboBox m_audcodecdimension;
	CButton m_audoutput;
	CButton m_audpreview;
	int m_nVidBuffers;
	int m_nAudBuffers;
	CButton m_recordbtn;
	UINT_PTR m_nRecordTimerID;
	BOOL m_fSepAudio;
	int m_muxtype;
	CComboBox m_muxctrl;

	// video input
	CStringW m_vidDisplayName;
	CComPtr<IAMStreamConfig> m_pAMVSC;
	CComPtr<IAMCrossbar> m_pAMXB;
	CComPtr<IAMTVTuner> m_pAMTuner;
	CComPtr<IAMVfwCaptureDialogs> m_pAMVfwCD;
	CVidFormatArray m_vfa;

	// audio input
	CStringW m_audDisplayName;
	CComPtr<IAMStreamConfig> m_pAMASC;
	CInterfaceArray<IAMAudioInputMixer> m_pAMAIM;
	CAudFormatArray m_afa;

	// video codec
	std::vector<Codec> m_pVidEncArray;
	CVidFormatArray m_vcfa;

	// audio codec
	std::vector<Codec> m_pAudEncArray;
	CAudFormatArray m_acfa;

	CComPtr<IMoniker> m_pVidEncMoniker, m_pAudEncMoniker;

	void EmptyVideo();
	void EmptyAudio();

	void UpdateMediaTypes();
	void UpdateUserDefinableControls();
	void UpdateVideoCodec();
	void UpdateAudioCodec();
	void UpdateMuxer();
	void UpdateOutputControls();

	void UpdateGraph();

	std::map<HWND, BOOL> m_wndenabledmap;
	void EnableControls(CWnd* pWnd, bool fEnable);

public:
	CString m_file;
	BOOL m_fVidOutput;
	int m_fVidPreview;
	BOOL m_fAudOutput;
	int m_fAudPreview;

	CMediaType m_mtv, m_mta, m_mtcv, m_mtca;
	CComPtr<IBaseFilter> m_pVidEnc, m_pAudEnc, m_pMux, m_pDst, m_pAudMux, m_pAudDst;
	CComPtr<IBaseFilter> m_pVidBuffer, m_pAudBuffer;

	CPlayerCaptureDialog(CMainFrame* pMainFrame);
	virtual ~CPlayerCaptureDialog();

	BOOL Create(CWnd* pParent = nullptr);

	// Dialog Data
	enum { IDD = IDD_CAPTURE_DLG };

	void InitControls();

	void SetupVideoControls(CStringW displayName, IAMStreamConfig* pAMSC, IAMCrossbar* pAMXB, IAMTVTuner* pAMTuner);
	void SetupVideoControls(CStringW displayName, IAMStreamConfig* pAMSC, IAMVfwCaptureDialogs* pAMVfwCD);
	void SetupAudioControls(CStringW displayName, IAMStreamConfig* pAMSC, const CInterfaceArray<IAMAudioInputMixer>& pAMAIM);

	void UpdateVideoControls();
	void UpdateAudioControls();

	bool IsTunerActive();

	bool SetVideoInput(int input);
	bool SetVideoChannel(int channel);
	bool SetAudioInput(int input);

	int GetVideoInput() const;
	int GetVideoChannel(); // can't be const because of the IAMTVTuner interface
	int GetAudioInput() const;

	bool IsInitialized() const {
		return m_bInitialized;
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

	afx_msg void OnDestroy();
	afx_msg void OnVideoInput();
	afx_msg void OnVideoType();
	afx_msg void OnVideoDimension();
	afx_msg void OnOverrideVideoDimension();
	afx_msg void OnAudioInput();
	afx_msg void OnAudioType();
	afx_msg void OnAudioDimension();
	afx_msg void OnRecordVideo();
	afx_msg void OnVideoCodec();
	afx_msg void OnVideoCodecType();
	afx_msg void OnVideoCodecDimension();
	afx_msg void OnRecordAudio();
	afx_msg void OnAudioCodec();
	afx_msg void OnAudioCodecType();
	afx_msg void OnAudioCodecDimension();
	afx_msg void OnOpenFile();
	afx_msg void OnRecord();
	afx_msg void OnChangeVideoBuffers();
	afx_msg void OnChangeAudioBuffers();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedVidAudPreview();
	afx_msg void OnBnClickedAudioToWav();
	afx_msg void OnChangeFileType();
};
