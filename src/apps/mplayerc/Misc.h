/*
 * (C) 2016-2024 see Authors.txt
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

bool    SetPrivilege(LPCWSTR privilege, bool bEnable = true);
bool    ExploreToFile(CString path);
BOOL    IsUserAdmin();
CString GetLastErrorMsg(LPWSTR lpszFunction, DWORD dw = GetLastError());

HICON LoadIcon(const CString& fn, bool fSmall);
bool  LoadType(const CString& fn, CString& type);
bool  LoadResource(UINT resid, CStringA& str, LPCWSTR restype);
HRESULT LoadResourceFile(UINT resid, BYTE** ppData, UINT& size);

CStringW GetDragQueryFileName(HDROP hDrop, UINT iFile);

WORD AssignedKeyToCmd(UINT keyValue);

enum :UINT {
	MOUSE_CLICK_LEFT = 1,
	MOUSE_CLICK_LEFT_DBL,
	MOUSE_CLICK_MIDLE,
	MOUSE_CLICK_RIGHT,
	MOUSE_CLICK_X1,
	MOUSE_CLICK_X2,
	MOUSE_WHEEL_UP,
	MOUSE_WHEEL_DOWN,
	MOUSE_WHEEL_LEFT,
	MOUSE_WHEEL_RIGHT,
};

WORD AssignedMouseToCmd(UINT mouseValue, UINT nFlags);

void SetAudioRenderer(int AudioDevNo);
void ThemeRGB(int iR, int iG, int iB, int& iRed, int& iGreen, int& iBlue);
COLORREF ThemeRGB(const int iR, const int iG, const int iB);

const LPCWSTR MonospaceFonts[] = {L"Consolas", L"Lucida Console", L"Courier New", L"" };
bool IsFontInstalled(LPCWSTR lpszFont);

//
// CMPCGradient
//

class CMPCGradient
{
	std::unique_ptr<BYTE[]> m_pData;
	UINT m_size = 0;
	UINT m_width = 0;
	UINT m_height = 0;

public:
	UINT Size();
	void Clear();
	HRESULT Create(IWICBitmapSource* pBitmapSource);
	bool Paint(CDC* dc, CRect r, int ptop, int br = -1, int rc = -1, int gc = -1, int bc = -1);
};
