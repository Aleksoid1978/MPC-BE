/*
 * (C) 2016-2018 see Authors.txt
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

extern CString ResStr(UINT nID);

extern void SetCursor(HWND m_hWnd, LPCWSTR lpCursorName);
extern void SetCursor(HWND m_hWnd, UINT nID, LPCWSTR lpCursorName);

extern void CorrectComboListWidth(CComboBox& ComboBox);
extern void CorrectCWndWidth(CWnd* pWnd);

extern void SetMenuRadioCheck(CCmdUI* pCmdUI, bool bCheck);

extern inline void AddStringData(CComboBox& ComboBox, LPCWSTR str, DWORD_PTR data);
extern inline void AddStringData(CListBox& ListBox, LPCWSTR str, DWORD_PTR data);
extern inline DWORD_PTR GetCurItemData(CComboBox& ComboBox);
extern inline DWORD_PTR GetCurItemData(CListBox& ListBox);
extern void SelectByItemData(CComboBox& ComboBox, DWORD_PTR data);
extern void SelectByItemData(CListBox& ListBox, DWORD_PTR data);
