/*
 * (C) 2011-2016 see Authors.txt
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
#include "vkCodes.h"

LPCWSTR GetKeyName(UINT vkCode)
{
	ASSERT(vkCode < 256);
	vkCode &= 0xff;

	static LPCWSTR s_pszKeys[256] = {
		L"Unused",
		L"Left mouse button",
		L"Right mouse button",
		L"Control-break",
		L"Middle mouse button",
		L"X1 mouse button",
		L"X2 mouse button",
		L"Undefined",
		L"Backspace",
		L"Tab",
		L"Unknown",
		L"Unknown",
		L"Clear",
		L"Enter",
		L"Unknown",
		L"Unknown",
		L"Shift",
		L"Control",
		L"Alt",
		L"Pause",
		L"Caps Lock",
		L"IME Kana mode",
		L"Unknown",
		L"IME Junja mode",
		L"IME final mode",
		L"IME Hanja mode",
		L"Unknown",
		L"Esc",
		L"IME convert",
		L"IME nonconvert",
		L"IME accept",
		L"IME mode change",
		L"Space",
		L"Page Up",
		L"Page Down",
		L"End",
		L"Home",
		L"Left Arrow",
		L"Up Arrow",
		L"Right Arrow",
		L"Down Arrow",
		L"Select",
		L"Print",
		L"Execute",
		L"Print Screen",
		L"Ins",
		L"Del",
		L"Help",
		L"0",
		L"1",
		L"2",
		L"3",
		L"4",
		L"5",
		L"6",
		L"7",
		L"8",
		L"9",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"A",
		L"B",
		L"C",
		L"D",
		L"E",
		L"F",
		L"G",
		L"H",
		L"I",
		L"J",
		L"K",
		L"L",
		L"M",
		L"N",
		L"O",
		L"P",
		L"Q",
		L"R",
		L"S",
		L"T",
		L"U",
		L"V",
		L"W",
		L"X",
		L"Y",
		L"Z",
		L"Left Win",
		L"Right Win",
		L"App",
		L"Unknown",
		L"Sleep",
		L"Num 0",
		L"Num 1",
		L"Num 2",
		L"Num 3",
		L"Num 4",
		L"Num 5",
		L"Num 6",
		L"Num 7",
		L"Num 8",
		L"Num 9",
		L"Multiply",
		L"Add",
		L"Separator",
		L"Subtract",
		L"Decimal",
		L"Divide",
		L"F1",
		L"F2",
		L"F3",
		L"F4",
		L"F5",
		L"F6",
		L"F7",
		L"F8",
		L"F9",
		L"F10",
		L"F11",
		L"F12",
		L"F13",
		L"F14",
		L"F15",
		L"F16",
		L"F17",
		L"F18",
		L"F19",
		L"F20",
		L"F21",
		L"F22",
		L"F23",
		L"F24",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Num Lock",
		L"Scroll Lock",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Left Shift",
		L"Right Shift",
		L"Left Control",
		L"Right Control",
		L"Left Alt",
		L"Right Alt",
		L"Browser Back",
		L"Browser Forward",
		L"Browser Refresh",
		L"Browser Stop",
		L"Browser Search",
		L"Browser Favorites",
		L"Browser Home",
		L"Volume Mute",
		L"Volume Down",
		L"Volume Up",
		L"Next Track",
		L"Previous Track",
		L"Stop Media",
		L"Play/Pause Media",
		L"Start Mail",
		L"Select Media",
		L"Start App 1",
		L"Start App 2",
		L"Unknown",
		L"Unknown",
		L";",
		L"=",
		L",",
		L"_",
		L".",
		L"/",
		L"`",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"Unknown",
		L"[",
		L"\\",
		L"]",
		L"'",
		L"OEM",
		L"Unknown",
		L"OEM",
		L"<> or \\|",
		L"OEM",
		L"OEM",
		L"IME Process key",
		L"OEM",
		L"VK_PACKET",
		L"Unknown",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"OEM",
		L"Attn",
		L"CrSel",
		L"ExSel",
		L"Erase EOF",
		L"Play",
		L"Zoom",
		L"Unknown",
		L"PA1",
		L"Clear",
		L"Unknown"
	};

	return(s_pszKeys[vkCode]);
}

BOOL HotkeyToString(UINT vkCode, UINT fModifiers, CString& s) {

	s.Empty();

	if (fModifiers & MOD_CONTROL) {
		s += L"Ctrl + ";
	}

	if (fModifiers & MOD_ALT) {
		s += L"Alt + ";
	}

	if (fModifiers & MOD_SHIFT) {
		s += L"Shift + ";
	}

	if (vkCode) {
		s += GetKeyName(vkCode);
	}

	return(!s.IsEmpty());
}

BOOL HotkeyModToString(UINT vkCode, BYTE fModifiers, CString& s)
{
	s.Empty();

	if (fModifiers & FCONTROL) {
		s += L"Ctrl + ";
	}

	if (fModifiers & FALT) {
		s += L"Alt + ";
	}

	if (fModifiers & FSHIFT) {
		s += L"Shift + ";
	}

	if (vkCode) {
		s += GetKeyName(vkCode);
	}

	return(!s.IsEmpty());
}

BOOL HotkeyToString(DWORD dwHk, CString& s)
{
	return(HotkeyToString(LOBYTE(LOWORD(dwHk)), HIBYTE(LOWORD(dwHk)), s));
}
