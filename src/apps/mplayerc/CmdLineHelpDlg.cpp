/*
 * (C) 2014-2025 see Authors.txt
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
#include "CmdLineHelpDlg.h"
#include "Misc.h"

CmdLineHelpDlg::CmdLineHelpDlg(const CString& cmdLine)
	: CResizableDialog(CmdLineHelpDlg::IDD)
	, m_cmdLine(cmdLine)
{
	m_cmdLine.Replace(L"\n", L"\r\n");
}

CmdLineHelpDlg::~CmdLineHelpDlg()
{
}

void CmdLineHelpDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC1, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_text);
}


BEGIN_MESSAGE_MAP(CmdLineHelpDlg, CResizableDialog)
END_MESSAGE_MAP()

BOOL CmdLineHelpDlg::OnInitDialog()
{
	__super::OnInitDialog();

	LOGFONTW lf = {};
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
	lf.lfHeight = -ScaleY(12);

	UINT i = 0;
	bool success;
	do {
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, MonospaceFonts[i]);
		success = IsFontInstalled(MonospaceFonts[i]) && m_font.CreateFontIndirectW(&lf);
		i++;
	} while (!success && i < std::size(MonospaceFonts));

	GetDlgItem(IDC_EDIT1)->SetFont(&m_font);

	m_icon.SetIcon(LoadIconW(nullptr, IDI_INFORMATION));

	m_text = m_cmdLine;
	m_text.Append(ResStr(IDS_CMD_USAGE));
	m_text.Append(L"\r\n\r\n");

	std::vector<std::pair<CStringW, CStringW>> commands;

	commands.reserve(IDS_CMD_RESET - IDS_CMD_PATHNAME + 1);
	int maxcmdlen = 0;

	for (int i = IDS_CMD_PATHNAME; i <= IDS_CMD_RESET; i++) {
		CString s = ResStr(i);
		const int cmdlen = s.Find('\t');
		if (cmdlen > 0) {
			commands.emplace_back(s.Left(cmdlen), s.Mid(cmdlen + 1));
			if (cmdlen > maxcmdlen) {
				maxcmdlen = cmdlen;
			}
		}
	}
	maxcmdlen++;

	std::vector<wchar_t> spaces(maxcmdlen, L' ');

	for (const auto& [name, desc] : commands) {
		m_text.Append(name);
		m_text.Append(spaces.data(), maxcmdlen - name.GetLength());

		auto delims = L"\n\r";
		int k = 0;
		CStringW token = desc.Tokenize(delims, k);
		m_text.Append(token);
		m_text.Append(L"\r\n");

		token = desc.Tokenize(delims, k);
		while (token.GetLength()) {
			m_text.Append(spaces.data(), maxcmdlen);
			m_text.Append(token);
			m_text.Append(L"\r\n");
			token = desc.Tokenize(delims, k);
		}
	}

	UpdateData(FALSE);

	GetDlgItem(IDOK)->SetFocus(); // Force the focus on the OK button

	AddAnchor(IDC_STATIC1, TOP_LEFT);
	AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	EnableSaveRestore(IDS_R_DLG_CMD_LINE_HELP);

	return FALSE;
}
