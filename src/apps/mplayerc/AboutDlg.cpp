/*
 * (C) 2014-2023 see Authors.txt
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
#include "AboutDlg.h"
#include "DSUtil/FileHandle.h"

#include "Version.h"

extern "C" char *GetFFmpegCompiler();
extern "C" char *GetlibavcodecVersion();

CAboutDlg::CAboutDlg()
	: CDialog(CAboutDlg::IDD)
{
	m_hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
}

CAboutDlg::~CAboutDlg()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

BOOL CAboutDlg::OnInitDialog()
{
	UpdateData();

	__super::OnInitDialog();

#ifdef _WIN64
	m_appname += L" (64-bit)";
#endif

	m_strVersionNumber.Append(MPC_VERSION_WSTR);
#if (MPC_VERSION_STATUS == 0)
	m_strVersionNumber.Append(L" dev");
#endif
#ifdef _DEBUG
	m_strVersionNumber.Append(L", Debug");
#endif
#ifdef REV_HASH
	m_strGitInfo.AppendFormat(L"git %S - %S", REV_DATE, REV_HASH);
#endif

#if defined(_MSC_VER)
	m_MPCCompiler.Format(L"MSVC %.2d.%.2d.%.5d", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000);
	#if _MSC_BUILD
		m_MPCCompiler.AppendFormat(L".%.2d", _MSC_BUILD);
	#endif
#else
	m_MPCCompiler = L"unknown compiler";
#endif

// https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#if (__AVX2__)
	m_MPCCompiler += L" (AVX2)";
#elif (__AVX__)
	m_MPCCompiler += L" (AVX)";
#endif

	m_FFmpegCompiler	= GetFFmpegCompiler();
	m_libavcodecVersion	= GetlibavcodecVersion();

	m_AuthorsPath = GetProgramDir() + L"Authors.txt";

	if (::PathFileExistsW(m_AuthorsPath)) {
		m_Credits.Replace(L"Authors.txt", L"<a>Authors.txt</a>");
	}

	if (m_hIcon != nullptr) {
		static_cast<CStatic*>(GetDlgItem(IDC_MAINFRAME_ICON))->SetIcon(m_hIcon);
	}

	UpdateData(FALSE);

	GetDlgItem(IDOK)->SetFocus();

	return FALSE;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC1, m_appname);
	DDX_Text(pDX, IDC_VERSION_NUMBER, m_strVersionNumber);
	DDX_Text(pDX, IDC_EDIT2, m_strGitInfo);
	DDX_Text(pDX, IDC_MPC_COMPILER, m_MPCCompiler);
	DDX_Text(pDX, IDC_FFMPEG_COMPILER, m_FFmpegCompiler);
	DDX_Text(pDX, IDC_LIBAVCODEC_VERSION, m_libavcodecVersion);
	DDX_Text(pDX, IDC_AUTHORS_LINK, m_Credits);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_AUTHORS_LINK, OnAuthors)
	ON_NOTIFY(NM_CLICK, IDC_SOURCEFORGE_LINK, OnHomepage)
	ON_NOTIFY(NM_CLICK, IDC_GITHUB_LINK, OnGitHub)
END_MESSAGE_MAP()

void CAboutDlg::OnAuthors(NMHDR* pNMHDR, LRESULT* pResult)
{
	ShellExecuteW(m_hWnd, L"open", m_AuthorsPath, nullptr, nullptr, SW_SHOWDEFAULT);

	*pResult = 0;
}

void CAboutDlg::OnHomepage(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecuteW(m_hWnd, L"open", L"https://sourceforge.net/projects/mpcbe/", nullptr, nullptr, SW_SHOWDEFAULT);

	*pResult = 0;
}

void CAboutDlg::OnGitHub(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecuteW(m_hWnd, L"open", L"https://github.com/Aleksoid1978/MPC-BE", nullptr, nullptr, SW_SHOWDEFAULT);

	*pResult = 0;
}
