/*
 * (C) 2014-2017 see Authors.txt
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
#include "../../DSUtil/FileHandle.h"

extern "C" char *GetFFmpegCompiler();
extern "C" char *GetlibavcodecVersion();

CAboutDlg::CAboutDlg()
	: CDialog(CAboutDlg::IDD)
{
	m_hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
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

	m_strVersionNumber.Format(L"%s (build %d)", _T(MPC_VERSION_STR), MPC_VERSION_REV);
#if (MPC_VERSION_STATUS == 0)
	m_strVersionNumber.Append(L" beta");
#endif

#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1210)
		m_MPCCompiler = L"ICL " MAKE_STR(__INTEL_COMPILER) L" Build ") MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#else
		#error Compiler is not supported!
#endif
#elif defined(_MSC_VER)
#if (_MSC_VER == 1910)
	m_MPCCompiler = L"MSVC 2017";
#elif (_MSC_VER == 1900)
	#if (_MSC_FULL_VER >= 190024210 && _MSC_FULL_VER <= 190024218)
		m_MPCCompiler = L"MSVC 2015.3";
	#elif (_MSC_FULL_VER == 190023918)
		m_MPCCompiler = L"MSVC 2015.2";
	#elif (_MSC_FULL_VER == 190023506)
		m_MPCCompiler = L"MSVC 2015.1";
	#else
		m_MPCCompiler = L"MSVC 2015";
	#endif
#elif (_MSC_VER <= 1800)
	#error Compiler is not supported!
#endif
#else
	#error Please add support for your compiler
#endif

#ifdef _DEBUG
	m_MPCCompiler += L" Debug";
#endif

#if (__AVX__)
	m_MPCCompiler += L" (AVX)";
#elif (__SSSE3__)
	m_MPCCompiler += L" (SSSE3)";
#elif (__SSE3__)
	m_MPCCompiler += L" (SSE3)";
#elif !defined(_M_X64) && defined(_M_IX86_FP)
	#if (_M_IX86_FP == 2)   // /arch:SSE2 was used
		m_MPCCompiler += L" (SSE2)";
	#elif (_M_IX86_FP == 1) // /arch:SSE was used
		m_MPCCompiler += L" (SSE)";
	#endif
#endif

	m_FFmpegCompiler	= GetFFmpegCompiler();
	m_libavcodecVersion	= GetlibavcodecVersion();

	m_AuthorsPath = GetProgramDir() + L"Authors.txt";

	if (::PathFileExists(m_AuthorsPath)) {
		m_Credits.Replace(L"Authors.txt", L"<a>Authors.txt</a>");
	}

	if (m_hIcon != NULL) {
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
	DDX_Text(pDX, IDC_MPC_COMPILER, m_MPCCompiler);
	DDX_Text(pDX, IDC_FFMPEG_COMPILER, m_FFmpegCompiler);
	DDX_Text(pDX, IDC_LIBAVCODEC_VERSION, m_libavcodecVersion);
	DDX_Text(pDX, IDC_AUTHORS_LINK, m_Credits);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_SOURCEFORGE_LINK, OnHomepage)
	ON_NOTIFY(NM_CLICK, IDC_AUTHORS_LINK, OnAuthors)
END_MESSAGE_MAP()

void CAboutDlg::OnHomepage(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecute(m_hWnd, L"open", _T(MPC_VERSION_COMMENTS), NULL, NULL, SW_SHOWDEFAULT);

	*pResult = 0;
}

void CAboutDlg::OnAuthors(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecute(m_hWnd, L"open", m_AuthorsPath, NULL, NULL, SW_SHOWDEFAULT);

	*pResult = 0;
}
