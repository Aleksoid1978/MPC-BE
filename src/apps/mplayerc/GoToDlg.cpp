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
#include "GoToDlg.h"
#include <math.h>


// CGoToDlg dialog

IMPLEMENT_DYNAMIC(CGoToDlg, CDialog)
CGoToDlg::CGoToDlg(REFERENCE_TIME time, REFERENCE_TIME maxTime, double fps, CWnd* pParent /*=nullptr*/)
	: CDialog(CGoToDlg::IDD, pParent)
	, m_time(time)
	, m_maxTime(maxTime)
	, m_fps(fps)
{
	if (m_fps == 0) {
		CString str = L"0";
		AfxGetProfile().ReadString(IDS_R_SETTINGS, IDS_RS_GOTO_FPS, str);

		if (float fps; swscanf_s(str, L"%f", &fps) == 1) {
			m_fps = fps;
		}
	}
}

CGoToDlg::~CGoToDlg()
{
}

void CGoToDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_timestr);
	DDX_Text(pDX, IDC_EDIT2, m_framestr);
	DDX_Control(pDX, IDC_EDIT1, m_timeedit);
	DDX_Control(pDX, IDC_EDIT2, m_frameedit);
}

BOOL CGoToDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	bool showHours = (m_maxTime >= 3600*1000*10000i64);

	if (showHours) {
		m_timeedit.EnableMask(L"DD DD DD DDD", L"__:__:__.___", L'0', L"0123456789");
	} else {
		m_timeedit.EnableMask(L"DD DD DDD", L"__:__.___", L'0', L"0123456789");
	}

	m_timeedit.EnableGetMaskedCharsOnly(false);
	m_timeedit.EnableSelectByGroup(false);

	int time = (int) (m_time / 10000);

	if (time >= 0) {
		if (showHours) {
			m_timestr.Format(L"%02d:%02d:%02d.%03d",
							 (time/(1000*60*60))%60, (time/(1000*60))%60, (time/1000)%60, time%1000);
		} else {
			m_timestr.Format(L"%02d:%02d.%03d",
							 (time/(1000*60))%60, (time/1000)%60, time%1000);
		}

		if (m_fps > 0) {
			m_framestr.Format(L"%d, %.3f", (int)(m_fps*m_time/10000000), m_fps);
		}

		UpdateData(FALSE);

		int gtlu = TYPE_TIME;
		AfxGetProfile().ReadInt(IDS_R_SETTINGS, IDS_RS_GOTO_LAST_USED, gtlu);
		switch (gtlu) {
			default:
			case TYPE_TIME:
				m_timeedit.SetFocus();
				m_timeedit.SetSel(0, 0);
				break;
			case TYPE_FRAME:
				m_frameedit.SetFocus();
				m_frameedit.SetSel(0, m_framestr.Find(','));
				break;
		}
	}

	return FALSE;
}

BEGIN_MESSAGE_MAP(CGoToDlg, CDialog)
	ON_BN_CLICKED(IDC_OK1, OnBnClickedOk1)
	ON_BN_CLICKED(IDC_OK2, OnBnClickedOk2)
END_MESSAGE_MAP()

// CGoToDlg message handlers

void CGoToDlg::OnBnClickedOk1()
{
	UpdateData();

	AfxGetProfile().WriteInt(IDS_R_SETTINGS, IDS_RS_GOTO_LAST_USED, TYPE_TIME);

	unsigned int hh = 0;
	unsigned int mm = 0;
	float ss = 0.0;
	wchar_t c1 = L':'; // delimiter character
	wchar_t c2 = L':'; // delimiter character
	wchar_t c3[2]; // unnecessary character

	if (((swscanf_s(m_timestr, L"%f%1s", &ss, &c3, _countof(c3)) == 1 || // sss[.ms]
			swscanf_s(m_timestr, L"%u%c%f%1s", &mm, &c2, sizeof(wchar_t), &ss, &c3, _countof(c3)) == 3 && ss < 60 || // mmm:ss[.ms]
			(swscanf_s(m_timestr, L"%u%c%u%c%f%1s", &hh, &c1, sizeof(wchar_t), &mm, &c2, sizeof(wchar_t), &ss, &c3, _countof(c3)) == 5 && mm < 60  && ss < 60)) && // hhh:mm:ss[.ms]
			c1 == L':' && c2 == L':' && ss >= 0)) {

		int time = (int)(1000*((hh*60+mm)*60+ss)+0.5);
		m_time = time * 10000i64;

		OnOK();
	} else {
		AfxMessageBox(ResStr(IDS_GOTO_ERROR_PARSING_TIME), MB_ICONEXCLAMATION | MB_OK);
	}
}

void CGoToDlg::OnBnClickedOk2()
{
	UpdateData();

	AfxGetProfile().WriteInt(IDS_R_SETTINGS, IDS_RS_GOTO_LAST_USED, TYPE_FRAME);

	unsigned int frame;
	float fps;
	wchar_t c1[2]; // delimiter character
	wchar_t c2[2]; // unnecessary character

	int result = swscanf_s(m_framestr, L"%u%1s%f%1s", &frame, &c1, _countof(c1), &fps, &c2, _countof(c2));

	if (result == 1) {
		m_time = (REFERENCE_TIME)ceil(10000000.0*frame/m_fps);
		OnOK();
	} else if (result == 3 && c1[0] == L',') {
		m_time = (REFERENCE_TIME)ceil(10000000.0*frame/fps);
		OnOK();
	} else if (result == 0 || c1[0] != L',') {
		AfxMessageBox(ResStr(IDS_GOTO_ERROR_PARSING_TEXT), MB_ICONEXCLAMATION | MB_OK);
	} else {
		AfxMessageBox(ResStr(IDS_GOTO_ERROR_PARSING_FPS), MB_ICONEXCLAMATION | MB_OK);
	}
}

void CGoToDlg::OnOK()
{
	if (m_time > m_maxTime) {
		AfxMessageBox(ResStr(IDS_GOTO_ERROR_INVALID_TIME), MB_ICONEXCLAMATION | MB_OK);
	} else {
		__super::OnOK();
	}
}

BOOL CGoToDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		if (*GetFocus() == m_timeedit) {
			OnBnClickedOk1();
		} else if (*GetFocus() == m_frameedit) {
			OnBnClickedOk2();
		}

		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}
