/*
 * (C) 2003-2006 Gabest
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

#include "stdafx.h"
#include "MainFrm.h"
#include "PPageLogo.h"
#include "WicUtils.h"

// CPPageLogo dialog

IMPLEMENT_DYNAMIC(CPPageLogo, CPPageBase)
CPPageLogo::CPPageLogo()
	: CPPageBase(CPPageLogo::IDD, CPPageLogo::IDD)
{
	m_logoids.emplace_back(IDF_LOGO0);
	m_logoids.emplace_back(IDF_LOGO1);
}

CPPageLogo::~CPPageLogo()
{
}

void CPPageLogo::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_intext);
	DDX_Text(pDX, IDC_LOGOFILENAME, m_logofn);
	DDX_Control(pDX, IDC_LOGOPREVIEW, m_logopreview);
	DDX_Text(pDX, IDC_AUTHOR, m_author);
}

BOOL CPPageLogo::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_intext = s.bLogoExternal ? 1 : 0;
	m_logofn = s.strLogoFileName;

	UpdateData(FALSE);

	for (unsigned i = 0; i < m_logoids.size(); ++i) {
		if (m_logoids[i] == s.nLogoId) {
			m_logoidpos = i;
			break;
		}
	}

	if (!m_intext) {
		OnBnClickedRadio1();
	} else {
		OnBnClickedRadio2();
	}

	return TRUE;
}

BOOL CPPageLogo::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.bLogoExternal = !!m_intext;
	s.strLogoFileName = m_logofn;
	s.nLogoId = m_logoids[m_logoidpos];

	AfxGetMainFrame()->m_wndView.LoadLogo();

	return __super::OnApply();
}

void CPPageLogo::GetDataFromRes()
{
	m_author.Empty();
	HBITMAP hBitmap = nullptr;

	UINT id = m_logoids[m_logoidpos];

	if (IDF_LOGO0 != id) {
		BYTE* data;
		UINT size;
		HRESULT hr = LoadResourceFile(id, &data, size);
		if (SUCCEEDED(hr)) {
			CComPtr<IWICBitmap> pBitmap;
			hr = WicLoadImage(&pBitmap, true, data, size);
			if (SUCCEEDED(hr)) {
				hr = WicCreateHBitmap(hBitmap, pBitmap);
			}
		}
		if (!m_author.LoadString(id)) {
			m_author = ResStr(IDS_LOGO_AUTHOR);
		}
	}

	hBitmap = m_logopreview.SetBitmap(hBitmap);
	DeleteObject(hBitmap);
}

BEGIN_MESSAGE_MAP(CPPageLogo, CPPageBase)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedRadio2)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpin1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
END_MESSAGE_MAP()

// CPPageLogo message handlers

void CPPageLogo::OnBnClickedRadio1()
{
	GetDataFromRes();
	Invalidate();

	m_intext = 0;
	UpdateData(FALSE);

	SetModified();
}

void CPPageLogo::OnBnClickedRadio2()
{
	UpdateData();

	m_author.Empty();
	HBITMAP hBitmap = nullptr;

	CComPtr<IWICBitmap> pBitmap;
	CComPtr<IWICBitmap> pBitmapScaled;
	UINT bm_w, bm_h;

	HRESULT hr = WicLoadImage(&pBitmap, true, m_logofn.GetString());
	if (SUCCEEDED(hr)) {
		hr = pBitmap->GetSize(&bm_w, &bm_h);
	}
	if (SUCCEEDED(hr)) {
		CRect rect = {};
		m_logopreview.GetClientRect(&rect);
		UINT w = rect.Width();
		UINT h = rect.Height();

		if (bm_w > w || bm_h > w) {
			UINT wy = w * bm_h;
			UINT hx = h * bm_w;
			if (wy > hx) {
				w = hx / bm_h;
			} else {
				h = wy / bm_w;
			}

			hr = WicCreateBitmapScaled(&pBitmapScaled, w, h, pBitmap);
			pBitmap.Release();
		}
	}
	if (SUCCEEDED(hr)) {
		hr = WicCreateDibSecton(hBitmap, pBitmapScaled ? pBitmapScaled : pBitmap);
	}

	hBitmap = m_logopreview.SetBitmap(hBitmap);
	DeleteObject(hBitmap);

	Invalidate();

	m_intext = 1;
	UpdateData(FALSE);

	SetModified();
}

void CPPageLogo::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	if (pNMUpDown->iDelta < 0) {
		if (++m_logoidpos >= (int)m_logoids.size()) {
			m_logoidpos = 0;
		}
	} else {
		if (--m_logoidpos < 0) {
			m_logoidpos = m_logoids.size() - 1;
		}
	}

	GetDataFromRes();

	UpdateData(FALSE);

	SetModified();

	*pResult = 0;
}

void CPPageLogo::OnBnClickedButton2()
{
	CString formats(L"*.bmp;*.jpg;*.jpeg;*.png;*.gif");

	if (S_OK == WicCheckComponent(CLSID_WICHeifDecoder)) {
		formats.Append(L";*.heif;*.heic");
	}
	if (S_OK == WicCheckComponent(CLSID_WICWebpDecoder)) {
		formats.Append(L";*.webp");
	}

	CFileDialog dlg(TRUE, nullptr, m_logofn,
					OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOCHANGEDIR,
					L"Images (" + formats + L")|" + formats + L"||",
					this, 0);

	if (dlg.DoModal() == IDOK) {
		m_logofn = dlg.GetPathName();
		UpdateData(FALSE);
		OnBnClickedRadio2();
	}
}
