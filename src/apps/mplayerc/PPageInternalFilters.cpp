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
#include "PPageInternalFilters.h"
#include "ComPropertyPage.h"
#include "../../filters/filters.h"


static filter_t s_filters[] = {

	// Source filters
	{_T("AMR"),						SOURCE_FILTER, SRC_AMR,				0},
	{_T("AVI"),						SOURCE_FILTER, SRC_AVI,				IDS_SRC_AVI},
	{_T("APE"),						SOURCE_FILTER, SRC_APE,				0},
	{_T("CDDA (Audio CD)"),			SOURCE_FILTER, SRC_CDDA,			IDS_SRC_CDDA},
	{_T("CDXA (VCD/SVCD/XCD)"),		SOURCE_FILTER, SRC_CDXA,			0},
	{_T("DirectShow Media"),		SOURCE_FILTER, SRC_DSM,				0},
	{_T("DSD (DSF/DFF)"),			SOURCE_FILTER, SRC_DSD,				0},
	{_T("DTS/AC3"),					SOURCE_FILTER, SRC_DTSAC3,			0},
	{_T("DVD Video Title Set"),		SOURCE_FILTER, SRC_VTS,				IDS_SRC_VTS},
	{_T("FLAC"),					SOURCE_FILTER, SRC_FLAC,			0},
	{_T("FLIC animation"),			SOURCE_FILTER, SRC_FLIC,			0},
	{_T("FLV"),						SOURCE_FILTER, SRC_FLV,				0},
	{_T("Matroska/WebM"),			SOURCE_FILTER, SRC_MATROSKA,		0},
	{_T("MP4/MOV"),					SOURCE_FILTER, SRC_MP4,				0},
	{_T("MPEG Audio"),				SOURCE_FILTER, SRC_MPA,				IDS_SRC_MPA},
	{_T("MPEG PS/TS/PVA"),			SOURCE_FILTER, SRC_MPEG,			0},
	{_T("MusePack"),				SOURCE_FILTER, SRC_MUSEPACK,		0},
	{_T("Ogg/Opus/Speex"),			SOURCE_FILTER, SRC_OGG,				0},
	{_T("RAW Video"),				SOURCE_FILTER, SRC_RAWVIDEO,		0},
	{_T("RealMedia"),				SOURCE_FILTER, SRC_REAL,			0},
	{_T("RoQ"),						SOURCE_FILTER, SRC_ROQ,				IDS_SRC_ROQ},
	{_T("SHOUTcast"),				SOURCE_FILTER, SRC_SHOUTCAST,		0},
	{_T("Std input"),				SOURCE_FILTER, SRC_STDINPUT,		0},
	{_T("TAK"),						SOURCE_FILTER, SRC_TAK,				0},
	{_T("TTA"),						SOURCE_FILTER, SRC_TTA,				0},
	{_T("WAV/Wave64"),				SOURCE_FILTER, SRC_WAV,				0},
	{_T("WavPack"),					SOURCE_FILTER, SRC_WAVPACK,			0},
	{_T("UDP/HTTP"),				SOURCE_FILTER, SRC_UDP,				0},

	// DXVA decoder
	{_T("H264/AVC (DXVA)"),			DXVA_DECODER,  VDEC_DXVA_H264,		0},
	{_T("HEVC (DXVA)"),				DXVA_DECODER,  VDEC_DXVA_HEVC,		0},
	{_T("MPEG-2 Video (DXVA)"),		DXVA_DECODER,  VDEC_DXVA_MPEG2,		0},
	{_T("VC-1 (DXVA)"),				DXVA_DECODER,  VDEC_DXVA_VC1,		0},
	{_T("WMV3 (DXVA)"),				DXVA_DECODER,  VDEC_DXVA_WMV3,		0},
	{_T("VP9 (DXVA)"),				DXVA_DECODER,  VDEC_DXVA_VP9,		0},

		// Video Decoder
	{_T("AMV Video"),				VIDEO_DECODER, VDEC_AMV,			0},
	{_T("Apple ProRes"),			VIDEO_DECODER, VDEC_PRORES,			0},
	{_T("Avid DNxHD"),				VIDEO_DECODER, VDEC_DNXHD,			0},
	{_T("Bink Video"),				VIDEO_DECODER, VDEC_BINK,			0},
	{_T("Canopus Lossless/HQ"),		VIDEO_DECODER, VDEC_CANOPUS,		0},
	{_T("Cinepak"),					VIDEO_DECODER, VDEC_CINEPAK,		0},
	{_T("Dirac"),					VIDEO_DECODER, VDEC_DIRAC,			0},
	{_T("DivX"),					VIDEO_DECODER, VDEC_DIVX,			0},
	{_T("DV Video"),				VIDEO_DECODER, VDEC_DV,				0},
	{_T("FLV1/4"),					VIDEO_DECODER, VDEC_FLV,			0},
	{_T("H263"),					VIDEO_DECODER, VDEC_H263,			0},
	{_T("H264/AVC (FFmpeg)"),		VIDEO_DECODER, VDEC_H264,			0},
	{_T("HEVC"),					VIDEO_DECODER, VDEC_HEVC,			0},
	{_T("Indeo 3/4/5"),				VIDEO_DECODER, VDEC_INDEO,			0},
	{_T("Lossless video (huffyuv, Lagarith, FFV1)"), VIDEO_DECODER, VDEC_LOSSLESS, 0},
	{_T("MJPEG"),					VIDEO_DECODER, VDEC_MJPEG,			0},
	{_T("MPEG-1 Video (FFmpeg)"),	VIDEO_DECODER, VDEC_MPEG1,			IDS_TRA_FFMPEG},
	{_T("MPEG-2 Video (FFmpeg)"),	VIDEO_DECODER, VDEC_MPEG2,			IDS_TRA_FFMPEG},
	{_T("MPEG-1 Video (libmpeg2)"),	VIDEO_DECODER, VDEC_LIBMPEG2_MPEG1,	IDS_TRA_MPEG2},
	{_T("MPEG-2/DVD Video (libmpeg2)"), VIDEO_DECODER, VDEC_LIBMPEG2_MPEG2, IDS_TRA_MPEG2},
	{_T("MS MPEG-4"),				VIDEO_DECODER, VDEC_MSMPEG4,		0},
	{_T("PNG"),						VIDEO_DECODER, VDEC_PNG,			0},
	{_T("QuickTime video (8BPS, QTRle, rpza)"), VIDEO_DECODER, VDEC_QT,		0},
	{_T("Screen Recorder (CSCD, MS, TSCC, VMnc)"), VIDEO_DECODER, VDEC_SCREEN, 0},
	{_T("SVQ1/3"),					VIDEO_DECODER, VDEC_SVQ,			0},
	{_T("Theora"),					VIDEO_DECODER, VDEC_THEORA,			0},
	{_T("Ut Video"),				VIDEO_DECODER, VDEC_UT,				0},
	{_T("VC-1 (FFmpeg)"),			VIDEO_DECODER, VDEC_VC1,			0},
	{_T("VP3/5/6"),					VIDEO_DECODER, VDEC_VP356,			0},
	{_T("VP7/8/9"),					VIDEO_DECODER, VDEC_VP789,			0},
	{_T("WMV1/2/3"),				VIDEO_DECODER, VDEC_WMV,			0},
	{_T("Xvid/MPEG-4"),				VIDEO_DECODER, VDEC_XVID,			0},
	{_T("RealVideo"),				VIDEO_DECODER, VDEC_REAL,			IDS_TRA_RV},
	{_T("Uncompressed video (v210, V410, Y800, I420, ...)"), VIDEO_DECODER, VDEC_UNCOMPRESSED, 0},

	// Audio decoder
	{_T("AAC"),						AUDIO_DECODER, ADEC_AAC,			0},
	{_T("AC3/E-AC3/TrueHD/MLP"),	AUDIO_DECODER, ADEC_AC3,			0},
	{_T("ALAC"),					AUDIO_DECODER, ADEC_ALAC,			0},
	{_T("ALS"),						AUDIO_DECODER, ADEC_ALS,			0},
	{_T("AMR"),						AUDIO_DECODER, ADEC_AMR,			0},
	{_T("APE"),						AUDIO_DECODER, ADEC_APE,			0},
	{_T("Bink Audio"),				AUDIO_DECODER, ADEC_BINK,			0},
	{_T("DSP Group TrueSpeech"),	AUDIO_DECODER, ADEC_TRUESPEECH,		0},
	{_T("DTS"),						AUDIO_DECODER, ADEC_DTS,			0},
	{_T("DSD (experimental)"),		AUDIO_DECODER, ADEC_DSD,			0},
	{_T("FLAC"),					AUDIO_DECODER, ADEC_FLAC,			0},
	{_T("Indeo Audio"),				AUDIO_DECODER, ADEC_INDEO,			0},
	{_T("LPCM"),					AUDIO_DECODER, ADEC_LPCM,			IDS_TRA_LPCM},
	{_T("MPEG Audio"),				AUDIO_DECODER, ADEC_MPA,			0},
	{_T("MusePack SV7/SV8"),		AUDIO_DECODER, ADEC_MUSEPACK,		0},
	{_T("Nellymoser"),				AUDIO_DECODER, ADEC_NELLY,			0},
	{_T("Opus"),					AUDIO_DECODER, ADEC_OPUS,			0},
	{_T("PS2 Audio (PCM/ADPCM)"),	AUDIO_DECODER, ADEC_PS2,			IDS_TRA_PS2AUD},
	{_T("QDesign Music Codec 2"),	AUDIO_DECODER, ADEC_QDM2,			0},
	{_T("RealAudio"),				AUDIO_DECODER, ADEC_REAL,			IDS_TRA_RA},
	{_T("Shorten"),					AUDIO_DECODER, ADEC_SHORTEN,		0},
	{_T("Speex"),					AUDIO_DECODER, ADEC_SPEEX,			0},
	{_T("TAK"),						AUDIO_DECODER, ADEC_TAK,			0},
	{_T("TTA"),						AUDIO_DECODER, ADEC_TTA,			0},
	{_T("Vorbis"),					AUDIO_DECODER, ADEC_VORBIS,			0},
	{_T("Voxware MetaSound"),		AUDIO_DECODER, ADEC_VOXWARE,		0},
	{_T("WavPack lossless audio"),	AUDIO_DECODER, ADEC_WAVPACK,		0},
	{_T("WMA v.1/v.2"),				AUDIO_DECODER, ADEC_WMA,			0},
	{_T("WMA v.9 Professional"),	AUDIO_DECODER, ADEC_WMA9,			0},
	{_T("WMA Lossless"),			AUDIO_DECODER, ADEC_WMALOSSLESS,	0},
	{_T("WMA Voice"),				AUDIO_DECODER, ADEC_WMAVOICE,		0},
	{_T("Other PCM/ADPCM"),			AUDIO_DECODER, ADEC_PCM_ADPCM,		0},
};

IMPLEMENT_DYNAMIC(CPPageInternalFiltersListBox, CCheckListBox)
CPPageInternalFiltersListBox::CPPageInternalFiltersListBox(int n)
	: CCheckListBox()
	, m_n(n)
{
	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		m_nbFiltersPerType[i] = m_nbChecked[i] = 0;
	}
}

void CPPageInternalFiltersListBox::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	EnableToolTips(TRUE);
}

INT_PTR CPPageInternalFiltersListBox::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	BOOL b = FALSE;
	int row = ItemFromPoint(point, b);

	if (row < 0) {
		return -1;
	}

	CRect r;
	GetItemRect(row, r);
	pTI->rect = r;
	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)row;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->uFlags |= TTF_ALWAYSTIP;

	return pTI->uId;
}

BEGIN_MESSAGE_MAP(CPPageInternalFiltersListBox, CCheckListBox)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CPPageInternalFiltersListBox::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	filter_t* f = (filter_t*)GetItemDataPtr(static_cast<int>(pNMHDR->idFrom));

	if (f->nHintID == 0) {
		return FALSE;
	}

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	static CString m_strTipText;

	m_strTipText = ResStr(f->nHintID);

	pTTT->lpszText = m_strTipText.GetBuffer();

	*pResult = 0;

	return TRUE;
}

int CPPageInternalFiltersListBox::AddFilter(filter_t* filter, bool checked)
{
	int index = AddString(filter->label);
	// SetItemDataPtr must be called before SetCheck
	SetItemDataPtr(index, filter);
	SetCheck(index, checked);

	return index;
}

void CPPageInternalFiltersListBox::UpdateCheckState()
{
	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		m_nbFiltersPerType[i] = m_nbChecked[i] = 0;
	}

	for (int i = 0; i < GetCount(); i++) {
		filter_t* filter = (filter_t*) GetItemDataPtr(i);

		m_nbFiltersPerType[filter->type]++;

		if (GetCheck(i)) {
			m_nbChecked[filter->type]++;
		}
	}
}

void CPPageInternalFiltersListBox::OnRButtonDown(UINT nFlags, CPoint point)
{
	CCheckListBox::OnRButtonDown(nFlags, point);

	CMenu m;
	m.CreatePopupMenu();

	enum {
		ENABLE_ALL = 1,
		DISABLE_ALL,
		ENABLE_FFMPEG,
		DISABLE_FFMPEG,
		ENABLE_DXVA,
		DISABLE_DXVA,
	};

	int totalFilters = 0, totalChecked = 0;

	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		totalFilters += m_nbFiltersPerType[i];
		totalChecked += m_nbChecked[i];
	}

	UINT state = (totalChecked != totalFilters) ? MF_ENABLED : MF_GRAYED;
	m.AppendMenu(MF_STRING | state, ENABLE_ALL, ResStr(IDS_ENABLE_ALL_FILTERS));
	state = (totalChecked != 0) ? MF_ENABLED : MF_GRAYED;
	m.AppendMenu(MF_STRING | state, DISABLE_ALL, ResStr(IDS_DISABLE_ALL_FILTERS));

	if (m_n == VIDEO) {
		m.AppendMenu(MF_SEPARATOR);
		state = (m_nbChecked[VIDEO_DECODER] != m_nbFiltersPerType[VIDEO_DECODER]) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, ENABLE_FFMPEG, ResStr(IDS_ENABLE_FFMPEG_FILTERS));
		state = (m_nbChecked[VIDEO_DECODER] != 0) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, DISABLE_FFMPEG, ResStr(IDS_DISABLE_FFMPEG_FILTERS));

		m.AppendMenu(MF_SEPARATOR);
		state = (m_nbChecked[DXVA_DECODER] != m_nbFiltersPerType[DXVA_DECODER]) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, ENABLE_DXVA, ResStr(IDS_ENABLE_DXVA_FILTERS));
		state = (m_nbChecked[DXVA_DECODER] != 0) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, DISABLE_DXVA, ResStr(IDS_DISABLE_DXVA_FILTERS));
	}

	CPoint p = point;
	::MapWindowPoints(m_hWnd, HWND_DESKTOP, &p, 1);

	UINT id = m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this);

	if (id == 0) {
		return;
	}

	int index = 0;

	for (int i = 0; i < _countof(s_filters); i++) {
		switch (s_filters[i].type) {
		case SOURCE_FILTER:
			if (m_n != SOURCE) {
				continue;
			}
			break;
		case DXVA_DECODER:
		case VIDEO_DECODER:
			if (m_n != VIDEO) {
				continue;
			}
			break;
		case AUDIO_DECODER:
			if (m_n != AUDIO) {
				continue;
			}
			break;
		default:
			continue;
		}

		switch (id) {
		case ENABLE_ALL:
			SetCheck(index, TRUE);
			break;
		case DISABLE_ALL:
			SetCheck(index, FALSE);
			break;
		case ENABLE_FFMPEG:
			if (s_filters[i].type == VIDEO_DECODER) {
				SetCheck(index, TRUE);
			}
			break;
		case DISABLE_FFMPEG:
			if (s_filters[i].type == VIDEO_DECODER) {
				SetCheck(index, FALSE);
			}
			break;
		case ENABLE_DXVA:
			if (s_filters[i].type == DXVA_DECODER) {
				SetCheck(index, TRUE);
			}
			break;
		case DISABLE_DXVA:
			if (s_filters[i].type == DXVA_DECODER) {
				SetCheck(index, FALSE);
			}
			break;
		}

		index++;
	}

	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CLBN_CHKCHANGE), (LPARAM)m_hWnd);
}

// CPPageInternalFilters dialog

IMPLEMENT_DYNAMIC(CPPageInternalFilters, CPPageBase)
CPPageInternalFilters::CPPageInternalFilters()
	: CPPageBase(CPPageInternalFilters::IDD, CPPageInternalFilters::IDD)
	, m_listSrc(SOURCE)
	, m_listVideo(VIDEO)
	, m_listAudio(AUDIO)
{
}

CPPageInternalFilters::~CPPageInternalFilters()
{
}

void CPPageInternalFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TAB1, m_Tab);
	DDX_Control(pDX, IDC_LIST1, m_listSrc);
	DDX_Control(pDX, IDC_LIST2, m_listVideo);
	DDX_Control(pDX, IDC_LIST3, m_listAudio);
	DDX_Control(pDX, IDC_BUTTON5, m_btnAviCfg);
	DDX_Control(pDX, IDC_BUTTON1, m_btnMpegCfg);
	DDX_Control(pDX, IDC_BUTTON6, m_btnMatroskaCfg);
	DDX_Control(pDX, IDC_BUTTON7, m_btnVTSCfg);
	DDX_Control(pDX, IDC_BUTTON2, m_btnVideoCfg);
	DDX_Control(pDX, IDC_BUTTON3, m_btnMPEG2Cfg);
	DDX_Control(pDX, IDC_BUTTON4, m_btnAudioCfg);
	DDX_Control(pDX, IDC_EDIT1, m_edtBufferDuration);
	DDX_Control(pDX, IDC_SPIN1, m_spnBufferDuration);
}

BEGIN_MESSAGE_MAP(CPPageInternalFilters, CPPageBase)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST3, OnSelChange)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST2, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST3, OnCheckBoxChange)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CPPageInternalFilters::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON5, OnAviSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON1, OnMpegSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON6, OnMatroskaSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON7, OnVTSReaderConfig)
	ON_BN_CLICKED(IDC_BUTTON2, OnVideoDecConfig)
	ON_BN_CLICKED(IDC_BUTTON3, OnMPEG2DecConfig)
	ON_BN_CLICKED(IDC_BUTTON4, OnAudioDecConfig)
END_MESSAGE_MAP()

// CPPageInternalFilters message handlers

BOOL CPPageInternalFilters::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_listSrc.SetItemHeight(0, ScaleY(13)+3);   // 16 without scale
	m_listVideo.SetItemHeight(0, ScaleY(13)+3); // 16 without scale
	m_listAudio.SetItemHeight(0, ScaleY(13)+3); // 16 without scale

	for (int i = 0; i < _countof(s_filters)-1; i++) {
		CPPageInternalFiltersListBox* l;
		bool checked = false;

		switch (s_filters[i].type) {
			case SOURCE_FILTER:
				l		= &m_listSrc;
				checked	= s.SrcFilters[s_filters[i].flag];
				break;
			case DXVA_DECODER:
				l		= &m_listVideo;
				checked	= s.DXVAFilters[s_filters[i].flag];
				break;
			case VIDEO_DECODER:
				l		= &m_listVideo;
				checked	= s.VideoFilters[s_filters[i].flag];
				break;
			case AUDIO_DECODER:
				l		= &m_listAudio;
				checked	= s.AudioFilters[s_filters[i].flag];
				break;
			default:
				l		= NULL;
				checked	= false;
		}

		if (l) {
			l->AddFilter(&s_filters[i], checked);
		}
	}

	m_listSrc.UpdateCheckState();
	m_listVideo.UpdateCheckState();
	m_listAudio.UpdateCheckState();

	m_edtBufferDuration = CLAMP(s.iBufferDuration / 1000, 1, 10);
	m_edtBufferDuration.SetRange(1, 10);
	m_spnBufferDuration.SetRange(1, 10);

	TC_ITEM tci;
	memset(&tci, 0, sizeof(tci));
	tci.mask = TCIF_TEXT;

	CString TabName	= ResStr(IDS_FILTERS_SOURCE);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(0, &tci);

	TabName			= ResStr(IDS_FILTERS_VIDEO);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(1, &tci);

	TabName			= ResStr(IDS_FILTERS_AUDIO);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(2, &tci);

	NMHDR hdr;
	hdr.code		= TCN_SELCHANGE;
	hdr.hwndFrom	= m_Tab.m_hWnd;

	SendMessage (WM_NOTIFY, m_Tab.GetDlgCtrlID(), (LPARAM)&hdr);

	SetCursor(m_hWnd, IDC_BUTTON1, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON2, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON3, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON4, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON5, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON6, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON7, IDC_HAND);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageInternalFilters::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	CPPageInternalFiltersListBox* list = &m_listSrc;
	for (int l=0; l<3; l++) {
		for (int i = 0; i < list->GetCount(); i++) {
			filter_t* f = (filter_t*) list->GetItemDataPtr(i);

			switch (f->type) {
				case SOURCE_FILTER:
					s.SrcFilters[f->flag] = !!list->GetCheck(i);
					break;
				case DXVA_DECODER:
					s.DXVAFilters[f->flag] = !!list->GetCheck(i);
					break;
				case VIDEO_DECODER:
					s.VideoFilters[f->flag] = !!list->GetCheck(i);
					break;
				case AUDIO_DECODER:
					s.AudioFilters[f->flag] = !!list->GetCheck(i);
					break;
			}
		}

		list = (l == 1) ? &m_listVideo : &m_listAudio;
	}

	s.iBufferDuration = m_edtBufferDuration * 1000;

	return __super::OnApply();
}

void CPPageInternalFilters::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
{
	if (!CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = CreateInstance(NULL, &hr);

	if (!pObj) {
		return;
	}

	CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pObj;

	if (SUCCEEDED(hr)) {
		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk) {
			CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
			ps.AddPages(pSPP);
			ps.DoModal();
		}
	}
}

void CPPageInternalFilters::OnSelChange()
{
}

void CPPageInternalFilters::OnCheckBoxChange()
{
	m_listSrc.UpdateCheckState();
	m_listVideo.UpdateCheckState();
	m_listAudio.UpdateCheckState();

	SetModified();
}


void CPPageInternalFilters::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	switch (m_Tab.GetCurSel()) {
		case SOURCE :
			m_listSrc.ShowWindow(SW_SHOW);
			m_listVideo.ShowWindow(SW_HIDE);
			m_listAudio.ShowWindow(SW_HIDE);

			m_btnAviCfg.ShowWindow(SW_SHOW);
			m_btnMpegCfg.ShowWindow(SW_SHOW);
			m_btnMatroskaCfg.ShowWindow(SW_SHOW);
			m_btnVTSCfg.ShowWindow(SW_SHOW);
			m_btnVideoCfg.ShowWindow(SW_HIDE);
			m_btnMPEG2Cfg.ShowWindow(SW_HIDE);
			m_btnAudioCfg.ShowWindow(SW_HIDE);

			GetDlgItem(IDC_STATIC1)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT1)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_SPIN1)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC2)->ShowWindow(SW_SHOW);
			break;
		case VIDEO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_SHOW);
			m_listAudio.ShowWindow(SW_HIDE);

			m_btnAviCfg.ShowWindow(SW_HIDE);
			m_btnMpegCfg.ShowWindow(SW_HIDE);
			m_btnMatroskaCfg.ShowWindow(SW_HIDE);
			m_btnVTSCfg.ShowWindow(SW_HIDE);
			m_btnVideoCfg.ShowWindow(SW_SHOW);
			m_btnMPEG2Cfg.ShowWindow(SW_SHOW);
			m_btnAudioCfg.ShowWindow(SW_HIDE);

			GetDlgItem(IDC_STATIC1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_SPIN1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC2)->ShowWindow(SW_HIDE);
			break;
		case AUDIO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_HIDE);
			m_listAudio.ShowWindow(SW_SHOW);

			m_btnAviCfg.ShowWindow(SW_HIDE);
			m_btnMpegCfg.ShowWindow(SW_HIDE);
			m_btnMatroskaCfg.ShowWindow(SW_HIDE);
			m_btnVTSCfg.ShowWindow(SW_HIDE);
			m_btnVideoCfg.ShowWindow(SW_HIDE);
			m_btnMPEG2Cfg.ShowWindow(SW_HIDE);
			m_btnAudioCfg.ShowWindow(SW_SHOW);

			GetDlgItem(IDC_STATIC1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_SPIN1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC2)->ShowWindow(SW_HIDE);
			break;
		default:
			break;
	}

	*pResult = 0;
}

void CPPageInternalFilters::OnAviSplitterConfig()
{
	ShowPPage(CreateInstance<CAviSplitterFilter>);
}

void CPPageInternalFilters::OnMpegSplitterConfig()
{
	ShowPPage(CreateInstance<CMpegSplitterFilter>);
}

void CPPageInternalFilters::OnMatroskaSplitterConfig()
{
	ShowPPage(CreateInstance<CMatroskaSplitterFilter>);
}

void CPPageInternalFilters::OnVTSReaderConfig()
{
	ShowPPage(CreateInstance<CVTSReader>);
}

void CPPageInternalFilters::OnVideoDecConfig()
{
	ShowPPage(CreateInstance<CMPCVideoDecFilter>);
}

void CPPageInternalFilters::OnMPEG2DecConfig()
{
	ShowPPage(CreateInstance<CMpeg2DecFilter>);
}

void CPPageInternalFilters::OnAudioDecConfig()
{
	ShowPPage(CreateInstance<CMpaDecFilter>);
}
