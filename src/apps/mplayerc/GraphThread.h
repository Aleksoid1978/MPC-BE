/*
 * (C) 2023 see Authors.txt
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

class CMainFrame;

class CGraphThread : public CWinThread
{
	CMainFrame* m_pMainFrame;

	DECLARE_DYNCREATE(CGraphThread);

public:
	CGraphThread() : m_pMainFrame(nullptr) {}

	void SetMainFrame(CMainFrame* pMainFrame) {
		m_pMainFrame = pMainFrame;
	}

	BOOL InitInstance();
	int ExitInstance();

	enum {
		TM_EXIT = WM_APP,
		TM_OPEN,
		TM_CLOSE,
		TM_RESIZE,
		TM_RESET,
		TM_TUNER_SCAN,
		TM_DISPLAY_CHANGE
	};
	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnOpen(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose(WPARAM wParam, LPARAM lParam);
	afx_msg void OnResize(WPARAM wParam, LPARAM lParam);
	afx_msg void OnReset(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTunerScan(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDisplayChange(WPARAM wParam, LPARAM lParam);
};
