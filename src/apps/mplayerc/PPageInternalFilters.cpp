/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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
#include "ComPropertySheet.h"
#include "../../filters/filters.h"


static filter_t s_source_filters[] = {
	{L"AC3",						SOURCE_FILTER, SRC_AC3,				0},
	{L"AMR",						SOURCE_FILTER, SRC_AMR,				0},
	{L"AVI",						SOURCE_FILTER, SRC_AVI,				IDS_SRC_AVI},
	{L"APE",						SOURCE_FILTER, SRC_APE,				0},
	{L"Bink Video",					SOURCE_FILTER, SRC_BINK,			0 },
	{L"CDDA (Audio CD)",			SOURCE_FILTER, SRC_CDDA,			IDS_SRC_CDDA},
	{L"CDXA (VCD/SVCD/XCD)",		SOURCE_FILTER, SRC_CDXA,			0},
	{L"DirectShow Media",			SOURCE_FILTER, SRC_DSM,				0},
	{L"DSD (DSF/DFF)",				SOURCE_FILTER, SRC_DSD,				0},
	{L"DTS/DTS-HD",					SOURCE_FILTER, SRC_DTS,				0},
	{L"DVD Video Title Set",		SOURCE_FILTER, SRC_VTS,				IDS_SRC_VTS},
	{L"FLAC",						SOURCE_FILTER, SRC_FLAC,			0},
	{L"FLIC animation",				SOURCE_FILTER, SRC_FLIC,			0},
	{L"FLV",						SOURCE_FILTER, SRC_FLV,				0},
	{L"Matroska/WebM",				SOURCE_FILTER, SRC_MATROSKA,		0},
	{L"MP4/MOV",					SOURCE_FILTER, SRC_MP4,				0},
	{L"MPEG Audio",					SOURCE_FILTER, SRC_MPA,				IDS_SRC_MPA},
	{L"MPEG PS/TS/PVA",				SOURCE_FILTER, SRC_MPEG,			0},
	{L"MusePack",					SOURCE_FILTER, SRC_MUSEPACK,		0},
	{L"Ogg/Opus/Speex",				SOURCE_FILTER, SRC_OGG,				0},
	{L"RAW Video",					SOURCE_FILTER, SRC_RAWVIDEO,		0},
	{L"RealMedia",					SOURCE_FILTER, SRC_REAL,			0},
	{L"RoQ",						SOURCE_FILTER, SRC_ROQ,				IDS_SRC_ROQ},
	{L"SHOUTcast",					SOURCE_FILTER, SRC_SHOUTCAST,		0},
	{L"Std input",					SOURCE_FILTER, SRC_STDINPUT,		0},
	{L"TAK",						SOURCE_FILTER, SRC_TAK,				0},
	{L"TTA",						SOURCE_FILTER, SRC_TTA,				0},
	{L"WAV/Wave64",					SOURCE_FILTER, SRC_WAV,				0},
	{L"WavPack",					SOURCE_FILTER, SRC_WAVPACK,			0},
	{L"UDP/HTTP",					SOURCE_FILTER, SRC_UDP,				0},
	{L"DVR Video",					SOURCE_FILTER, SRC_DVR,				0},
};

static filter_t s_video_decoders[] = {
	// DXVA2 decoders
	{L"DXVA2: H264/AVC",			DXVA_DECODER,  VDEC_DXVA_H264,		0},
	{L"DXVA2: HEVC",				DXVA_DECODER,  VDEC_DXVA_HEVC,		0},
	{L"DXVA2: MPEG-2 Video",		DXVA_DECODER,  VDEC_DXVA_MPEG2,		0},
	{L"DXVA2: VC-1",				DXVA_DECODER,  VDEC_DXVA_VC1,		0},
	{L"DXVA2: WMV3",				DXVA_DECODER,  VDEC_DXVA_WMV3,		0},
	{L"DXVA2: VP9",					DXVA_DECODER,  VDEC_DXVA_VP9,		0},
	// Software decoders
	{L"AMV Video",					VIDEO_DECODER, VDEC_AMV,			0},
	{L"Apple ProRes",				VIDEO_DECODER, VDEC_PRORES,			0},
	{L"AOMedia Video 1 (AV1)",		VIDEO_DECODER, VDEC_AV1,			0},
	{L"Avid DNxHD",					VIDEO_DECODER, VDEC_DNXHD,			0},
	{L"Bink Video",					VIDEO_DECODER, VDEC_BINK,			0},
	{L"Canopus Lossless/HQ/HQX",	VIDEO_DECODER, VDEC_CANOPUS,		0},
	{L"CineForm",					VIDEO_DECODER, VDEC_CINEFORM,		0},
	{L"Cinepak",					VIDEO_DECODER, VDEC_CINEPAK,		0},
	{L"Dirac",						VIDEO_DECODER, VDEC_DIRAC,			0},
	{L"DivX",						VIDEO_DECODER, VDEC_DIVX,			0},
	{L"DV Video",					VIDEO_DECODER, VDEC_DV,				0},
	{L"FLV1/4",						VIDEO_DECODER, VDEC_FLV,			0},
	{L"H263",						VIDEO_DECODER, VDEC_H263,			0},
	{L"H264/AVC",					VIDEO_DECODER, VDEC_H264,			0},
	{L"H264 (MVC 3D)",				VIDEO_DECODER, VDEC_H264_MVC,		IDS_TRA_INTEL_MSDK},
	{L"HEVC",						VIDEO_DECODER, VDEC_HEVC,			0},
	{L"Indeo 3/4/5",				VIDEO_DECODER, VDEC_INDEO,			0},
	{L"Lossless video (HuffYUV, FFV1, Lagarith, MagicYUV)", VIDEO_DECODER, VDEC_LOSSLESS, 0},
	{L"MJPEG",						VIDEO_DECODER, VDEC_MJPEG,			0},
	{L"MPEG-1 Video",				VIDEO_DECODER, VDEC_MPEG1,			IDS_TRA_FFMPEG},
	{L"MPEG-2 Video",				VIDEO_DECODER, VDEC_MPEG2,			IDS_TRA_FFMPEG},
	{L"DVD MPEG Video",				VIDEO_DECODER, VDEC_DVD_LIBMPEG2,	IDS_TRA_MPEG2},
	{L"MS MPEG-4",					VIDEO_DECODER, VDEC_MSMPEG4,		0},
	{L"PNG",						VIDEO_DECODER, VDEC_PNG,			0},
	{L"QuickTime video (8BPS, QTRle, rpza)", VIDEO_DECODER, VDEC_QT,		0},
	{L"Screen Recorder (CSCD, MS, TSCC, VMnc, G2M)", VIDEO_DECODER, VDEC_SCREEN, 0},
	{L"SVQ1/3",						VIDEO_DECODER, VDEC_SVQ,			0},
	{L"Theora",						VIDEO_DECODER, VDEC_THEORA,			0},
	{L"Ut Video",					VIDEO_DECODER, VDEC_UT,				0},
	{L"VC-1",						VIDEO_DECODER, VDEC_VC1,			0},
	{L"Vidvox Hap",					VIDEO_DECODER, VDEC_HAP,			0},
	{L"VP3/5/6",					VIDEO_DECODER, VDEC_VP356,			0},
	{L"VP7/8/9",					VIDEO_DECODER, VDEC_VP789,			0},
	{L"WMV1/2/3",					VIDEO_DECODER, VDEC_WMV,			0},
	{L"Xvid/MPEG-4",				VIDEO_DECODER, VDEC_XVID,			0},
	{L"RealVideo",					VIDEO_DECODER, VDEC_REAL,			IDS_TRA_RV},
	{L"Uncompressed video (v210, V410, Y8, I420, \x2026)", VIDEO_DECODER, VDEC_UNCOMPRESSED, 0},
};

static filter_t s_audio_decoders[] = {
	{L"AAC",						AUDIO_DECODER, ADEC_AAC,			0},
	{L"AC3/E-AC3/TrueHD/MLP",		AUDIO_DECODER, ADEC_AC3,			0},
	{L"ALAC",						AUDIO_DECODER, ADEC_ALAC,			0},
	{L"ALS",						AUDIO_DECODER, ADEC_ALS,			0},
	{L"AMR",						AUDIO_DECODER, ADEC_AMR,			0},
	{L"APE",						AUDIO_DECODER, ADEC_APE,			0},
	{L"Bink Audio",					AUDIO_DECODER, ADEC_BINK,			0},
	{L"DSP Group TrueSpeech",		AUDIO_DECODER, ADEC_TRUESPEECH,		0},
	{L"DTS",						AUDIO_DECODER, ADEC_DTS,			0},
	{L"DSD/DST",					AUDIO_DECODER, ADEC_DSD,			0},
	{L"FLAC",						AUDIO_DECODER, ADEC_FLAC,			0},
	{L"Indeo Audio",				AUDIO_DECODER, ADEC_INDEO,			0},
	{L"LPCM",						AUDIO_DECODER, ADEC_LPCM,			IDS_TRA_LPCM},
	{L"MPEG Audio",					AUDIO_DECODER, ADEC_MPA,			0},
	{L"MusePack SV7/SV8",			AUDIO_DECODER, ADEC_MUSEPACK,		0},
	{L"Nellymoser",					AUDIO_DECODER, ADEC_NELLY,			0},
	{L"Opus",						AUDIO_DECODER, ADEC_OPUS,			0},
	{L"PS2 Audio (PCM/ADPCM)",		AUDIO_DECODER, ADEC_PS2,			IDS_TRA_PS2AUD},
	{L"QDesign Music Codec",		AUDIO_DECODER, ADEC_QDMC,			0},
	{L"RealAudio",					AUDIO_DECODER, ADEC_REAL,			IDS_TRA_RA},
	{L"Shorten",					AUDIO_DECODER, ADEC_SHORTEN,		0},
	{L"Speex",						AUDIO_DECODER, ADEC_SPEEX,			0},
	{L"TAK",						AUDIO_DECODER, ADEC_TAK,			0},
	{L"TTA",						AUDIO_DECODER, ADEC_TTA,			0},
	{L"Vorbis",						AUDIO_DECODER, ADEC_VORBIS,			0},
	{L"Voxware MetaSound",			AUDIO_DECODER, ADEC_VOXWARE,		0},
	{L"WavPack lossless audio",		AUDIO_DECODER, ADEC_WAVPACK,		0},
	{L"WMA v.1/v.2",				AUDIO_DECODER, ADEC_WMA,			0},
	{L"WMA v.9 Professional",		AUDIO_DECODER, ADEC_WMA9,			0},
	{L"WMA Lossless",				AUDIO_DECODER, ADEC_WMALOSSLESS,	0},
	{L"WMA Voice",					AUDIO_DECODER, ADEC_WMAVOICE,		0},
	{L"Other PCM/ADPCM",			AUDIO_DECODER, ADEC_PCM_ADPCM,		0},
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

	::SendMessageW(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

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

	switch (m_n) {
	case SOURCE:
		for (size_t i = 0; i < std::size(s_source_filters); i++) {
			if (id == ENABLE_ALL) {
				SetCheck(i, TRUE);
			}
			else if (id == DISABLE_ALL) {
				SetCheck(i, FALSE);
			}
		}
		break;
	case VIDEO:
		for (size_t i = 0; i < std::size(s_video_decoders); i++) {
			switch (id) {
			case ENABLE_ALL:
				SetCheck(i, TRUE);
				break;
			case DISABLE_ALL:
				SetCheck(i, FALSE);
				break;
			case ENABLE_FFMPEG:
				if (s_video_decoders[i].type == VIDEO_DECODER) {
					SetCheck(i, TRUE);
				}
				break;
			case DISABLE_FFMPEG:
				if (s_video_decoders[i].type == VIDEO_DECODER) {
					SetCheck(i, FALSE);
				}
				break;
			case ENABLE_DXVA:
				if (s_video_decoders[i].type == DXVA_DECODER) {
					SetCheck(i, TRUE);
				}
				break;
			case DISABLE_DXVA:
				if (s_video_decoders[i].type == DXVA_DECODER) {
					SetCheck(i, FALSE);
				}
				break;
			}
		}
		break;
	case AUDIO:
		for (size_t i = 0; i < std::size(s_audio_decoders); i++) {
			if (id == ENABLE_ALL) {
				SetCheck(i, TRUE);
			}
			else if (id == DISABLE_ALL) {
				SetCheck(i, FALSE);
			}
		}
		break;
	}

	GetParent()->SendMessageW(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CLBN_CHKCHANGE), (LPARAM)m_hWnd);
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
	DDX_Control(pDX, IDC_BUTTON8, m_btnCDDACfg);
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
	ON_BN_CLICKED(IDC_BUTTON8, OnCDDAReaderConfig)
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

	for (auto& item : s_source_filters) {
		bool checked = s.SrcFilters[item.flag];
		m_listSrc.AddFilter(&item, checked);
	}

	for (auto& item : s_video_decoders) {
		bool checked = false;
		if (item.type == DXVA_DECODER) {
			checked = s.DXVAFilters[item.flag];
		}
		else if (item.type == VIDEO_DECODER) {
			checked = s.VideoFilters[item.flag];
		}
		else {
			ASSERT(0);
			continue;
		}
		m_listVideo.AddFilter(&item, checked);
	}

	for (auto& item : s_audio_decoders) {
		bool checked = s.AudioFilters[item.flag];
		m_listAudio.AddFilter(&item, checked);
	}

	m_listSrc.UpdateCheckState();
	m_listVideo.UpdateCheckState();
	m_listAudio.UpdateCheckState();

	m_edtBufferDuration = std::clamp(s.iBufferDuration, APP_BUFDURATION_MIN, APP_BUFDURATION_MAX) / 1000;
	m_edtBufferDuration.SetRange(APP_BUFDURATION_MIN / 1000, APP_BUFDURATION_MAX / 1000);
	m_spnBufferDuration.SetRange(APP_BUFDURATION_MIN / 1000, APP_BUFDURATION_MAX / 1000);

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

	SendMessageW(WM_NOTIFY, m_Tab.GetDlgCtrlID(), (LPARAM)&hdr);

	SetCursor(m_hWnd, IDC_BUTTON1, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON2, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON3, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON4, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON5, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON6, IDC_HAND);
	SetCursor(m_hWnd, IDC_BUTTON7, IDC_HAND);

	m_btnVTSCfg.ShowWindow(SW_HIDE);

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
	CUnknown* pObj = CreateInstance(nullptr, &hr);

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
			m_btnCDDACfg.ShowWindow(SW_SHOW);
			//m_btnVTSCfg.ShowWindow(SW_SHOW);
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
			m_btnCDDACfg.ShowWindow(SW_HIDE);
			//m_btnVTSCfg.ShowWindow(SW_HIDE);
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
			m_btnCDDACfg.ShowWindow(SW_HIDE);
			//m_btnVTSCfg.ShowWindow(SW_HIDE);
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

void CPPageInternalFilters::OnCDDAReaderConfig()
{
	ShowPPage(CreateInstance<CCDDAReader>);
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
