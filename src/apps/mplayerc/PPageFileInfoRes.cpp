/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "DSUtil/std_helper.h"
#include <HighDPI.h>
#include "PPageFileInfoRes.h"
#include "FileDialogs.h"
#include "Misc.h"
#include "WicUtils.h"
#include <wmsdkidl.h>
#include "DSUtil/ID3V2PictureType.h"

struct mime_info_t {
	const char* mime;
	const char* name;
	const char* ext;
};

static const mime_info_t s_image_mime_info[] = {
	{"image/jpeg", "JPEG",    "jpg" },
	{"image/jpg",  "JPEG",    "jpg" }, // non standart mime type
	{"image/png",  "PNG",     "png" },
	{"image/gif",  "GIF",     "gif" },
	{"image/bmp",  "BMP",     "bmp" },
	{"image/webp", "WebP",    "webp"},
	{"image/heic", "HEIC",    "heic"},
	{"image/avif", "AVIF",    "avif"},
	{"image/jxl",  "JPEG XL", "jxl" },
	{"image/jls",  "JPEG-LS", "jls" },
};

static const mime_info_t s_font_mime_info[] = {
	{"font/ttf",                    "TrueType", "ttf"},
	{"application/x-truetype-font", "TrueType", "ttf"},
	{"application/x-font-ttf",      "TrueType", "ttf"},
	{"font/otf",                    "OpenType", "otf"},
	{"application/vnd.ms-opentype", "OpenType", "otf"},
};

static const mime_info_t s_other_mime_info[] = {
	{"font/collection", "TrueType Collection", "ttc" },
};

// CPPageFileInfoRes dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoRes, CPPageBase)
CPPageFileInfoRes::CPPageFileInfoRes(const CStringW& fn, IFilterGraph* pFG, CDPI* pSheetDpi)
	: CPPageBase(CPPageFileInfoRes::IDD, CPPageFileInfoRes::IDD)
	, m_fn(fn)
	, m_fullfn(fn)
	, m_pSheetDpi(pSheetDpi)
{
	m_fn.TrimRight('/');
	int i = std::max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

	if (i >= 0 && i < m_fn.GetLength() - 1) {
		m_fn = m_fn.Mid(i + 1);
	}

	HRESULT hr = S_OK;

	BeginEnumFilters(pFG, pEF, pBF) {
		if (CComQIPtr<IDSMResourceBag> pRB = pBF.p) {
			for (DWORD i = 0; i < pRB->ResGetCount(); i++) {
				CComBSTR name, desc, mime;
				BYTE* pData = nullptr;
				DWORD len = 0;
				if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, nullptr))) {
					CDSMResource r;
					r.name = name;
					r.desc = desc;
					r.mime = mime;
					r.data.resize(len);
					memcpy(r.data.data(), pData, r.data.size());
					CoTaskMemFree(pData);
					m_resources.emplace_back(r);
				}
			}
		}
		else if (CComQIPtr<IWMHeaderInfo> pWMHI = pBF.p) {
			WORD streamNum = 0;
			WMT_ATTR_DATATYPE type;
			WORD length;

			hr = pWMHI->GetAttributeByName(&streamNum, L"WM/Picture", &type, nullptr, &length);
			if (SUCCEEDED(hr) && type == WMT_TYPE_BINARY && length > sizeof(WM_PICTURE)) {
				std::vector<BYTE> value(length);
				hr = pWMHI->GetAttributeByName(&streamNum, L"WM/Picture", &type, value.data(), &length);
				if (SUCCEEDED(hr)) {
					WM_PICTURE* wmpicture = (WM_PICTURE*)value.data();
					CDSMResource r;
					r.name = ID3v2PictureTypeToStr(wmpicture->bPictureType);
					r.mime = wmpicture->pwszMIMEType;
					r.desc = wmpicture->pwszDescription;
					r.data.resize(wmpicture->dwDataLen);
					memcpy(r.data.data(), wmpicture->pbData, wmpicture->dwDataLen);
					m_resources.emplace_back(r);
				}
			}
		}
	}
	EndEnumFilters;
}

CPPageFileInfoRes::~CPPageFileInfoRes()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

UINT CPPageFileInfoRes::GetResourceCount()
{
	return (UINT)m_resources.size();
}

void CPPageFileInfoRes::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_STATIC1, m_picPreview);
}

BEGIN_MESSAGE_MAP(CPPageFileInfoRes, CPPageBase)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList)
	ON_BN_CLICKED(IDC_BUTTON1, OnSaveAs)
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoRes message handlers

BOOL CPPageFileInfoRes::OnInitDialog()
{
	__super::OnInitDialog();

	m_hIcon = LoadIconW(m_fullfn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	m_list.InsertColumn(0, L"#", LVCFMT_RIGHT, m_pSheetDpi->ScaleX(20));
	m_list.InsertColumn(1, ResStr(IDS_PROPSHEET_NAME), LVCFMT_LEFT, m_pSheetDpi->ScaleX(150));
	m_list.InsertColumn(2, ResStr(IDS_PROPSHEET_MIME), LVCFMT_LEFT, m_pSheetDpi->ScaleX(150));
	m_list.InsertColumn(3, ResStr(IDS_PROPSHEET_DESC), LVCFMT_LEFT, m_pSheetDpi->ScaleX(60));
	m_list.InsertColumn(4, L"-", LVCFMT_LEFT, m_pSheetDpi->ScaleX(20)); // dummy column

	int n = 0;
	for (const auto& resource : m_resources) {
		int iItem = m_list.InsertItem(m_list.GetItemCount(), std::to_wstring(++n).c_str());
		m_list.SetItemText(iItem, 1, resource.name);
		m_list.SetItemText(iItem, 2, resource.mime);
		m_list.SetItemText(iItem, 3, resource.desc);
	}

	for (int iCol = 0; iCol < 4; iCol++) {
		m_list.SetColumnWidth(iCol, LVSCW_AUTOSIZE_USEHEADER);
	}
	m_list.DeleteColumn(4); // delete the dummy column

	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);

	UpdateData(FALSE);

	return TRUE;
}

const CDSMResource* CPPageFileInfoRes::GetResource(int idx)
{
	ASSERT(m_list.GetItemCount() == (int)m_resources.size());

	if (idx < 0 || idx >= (int)m_resources.size()) {
		return nullptr;
	}

	auto it = m_resources.cbegin();
	std::advance(it, idx);

	return &(*it);
}

void CPPageFileInfoRes::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & (LVIS_SELECTED | LVNI_FOCUSED)) {
			GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);

			HRESULT hr = E_NOT_SET;
			CComPtr<IWICBitmap> pBitmap;
			CComPtr<IWICBitmap> pBitmapScaled;

			auto resource = GetResource(pNMLV->iItem);
			if (resource && StartsWith(resource->mime, L"image/")) {
				UINT bm_w, bm_h;

				hr = WicLoadImage(&pBitmap, true, const_cast<BYTE*>(resource->data.data()), resource->data.size());
				if (SUCCEEDED(hr)) {
					hr = pBitmap->GetSize(&bm_w, &bm_h);
				}

				if (SUCCEEDED(hr)) {
					CRect rect = {};
					m_picPreview.GetClientRect(&rect);
					UINT w = rect.Width();
					UINT h = rect.Height();

					if (bm_w > w || bm_h > h) {
						UINT wy = w * bm_h;
						UINT hx = h * bm_w;
						if (wy > hx) {
							w = hx / bm_h;
						}
						else {
							h = wy / bm_w;
						}

						hr = WicCreateBitmapScaled(&pBitmapScaled, w, h, pBitmap);
						pBitmap.Release();
					}
				}
			}

			HBITMAP hBitmap = nullptr;
			if (SUCCEEDED(hr)) {
				hr = WicCreateDibSecton(hBitmap, pBitmapScaled ? pBitmapScaled : pBitmap);
			}
			hBitmap = m_picPreview.SetBitmap(hBitmap);
			DeleteObject(hBitmap);
		}
		else {
			GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);

			HBITMAP hBitmap = m_picPreview.SetBitmap(nullptr);
			DeleteObject(hBitmap);
		}
	}

	*pResult = 0;
}

void CPPageFileInfoRes::OnSaveAs()
{
	auto resource = GetResource(m_list.GetSelectionMark());
	if (!resource) {
		return;
	}

	CStringW fname(resource->name);
	CStringW ext = ::PathFindExtensionW(fname);

	CStringA mime(resource->mime);
	mime.MakeLower();
	CStringW ext_list;
	
	if (StartsWith(mime, "image/")) {
		for (const auto& mime_info : s_image_mime_info) {
			if (mime == mime_info.mime) {
				ext_list.Format(IDS_PROPSHEET_FILE_IMAGE, mime_info.name);
				ext_list.AppendFormat(L" (*.%hs)|*.%hs|", mime_info.ext, mime_info.ext);
				if (ext.IsEmpty()) {
					fname += L"." + CStringW(mime_info.ext);
				}
				break;
			}
		}
	}

	if (ext_list.IsEmpty()) {
		for (const auto& mime_info : s_font_mime_info) {
			if (mime == mime_info.mime) {
				ext_list.Format(IDS_PROPSHEET_FILE_FONT, mime_info.name);
				ext_list.AppendFormat(L" (*.%hs)|*.%hs|", mime_info.ext, mime_info.ext);
				if (ext.IsEmpty()) {
					fname += L"." + CStringW(mime_info.ext);
				}
				break;
			}
		}
	}

	if (ext_list.IsEmpty()) {
		for (const auto& mime_info : s_other_mime_info) {
			if (mime == mime_info.mime) {
				ext_list.Format(L"%hs (*.%hs)|*.%hs|", mime_info.name, mime_info.ext, mime_info.ext);
				if (ext.IsEmpty()) {
					fname += L"." + CStringW(mime_info.ext);
				}
				break;
			}
		}
	}

	if (ext_list.IsEmpty()) {
		ext_list = L"All files|*.*|";
	}

	CSaveFileDialog fd(nullptr, fname,
				   OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR,
				   ext_list, this);

	if (fd.DoModal() == IDOK) {
		FILE* f = nullptr;
		if (_wfopen_s(&f, fd.GetPathName(), L"wb") == 0) {
			fwrite(resource->data.data(), 1, resource->data.size(), f);
			fclose(f);
		}
	}
}

void CPPageFileInfoRes::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r;
	if (::IsWindow(m_list.GetSafeHwnd())) {
		m_list.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_list.SetWindowPos(nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}

	HDWP hDWP = ::BeginDeferWindowPos(1);
	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != nullptr; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild->SendMessageW(WM_GETDLGCODE) & DLGC_BUTTON || pChild == GetDlgItem(IDC_STATIC1)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, r.left, r.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
		} else if (pChild != GetDlgItem(IDC_LIST1) && pChild != GetDlgItem(IDC_DEFAULTICON)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	::EndDeferWindowPos(hDWP);
}

BOOL CPPageFileInfoRes::OnSetActive()
{
	BOOL ret = __super::OnSetActive();
	PostMessageW(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoRes::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	SendDlgItemMessageW(IDC_EDIT1, EM_SETSEL, 0, 0);

	return 0;
}
