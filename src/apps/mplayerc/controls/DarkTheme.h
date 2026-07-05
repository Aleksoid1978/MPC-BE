/*
 * (C) 2026 see Authors.txt
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

// Helpers to apply a dark visual theme to standard Win32/MFC dialogs (the
// Options property sheet and its pages). Uses documented UXTHEME/DWM APIs plus
// the undocumented immersive dark-mode ordinals available on Windows 10 1809+.
//
// Every entry point is a no-op when the dark theme is disabled
// (AfxGetAppSettings().bUseDarkTheme) or unsupported by the OS, so the classic
// light appearance is preserved in those cases.

namespace DarkTheme
{
	// True when the dark theme should be applied (bUseDarkTheme && OS supports it).
	bool IsActive();

	// Enables per-process immersive dark mode for common controls. Idempotent.
	void AllowDarkModeForApp();

	// Enables dark mode and a dark title bar for a top-level window.
	void EnableForWindow(HWND hWnd);

	// Applies the matching dark visual style to every child control of hWndParent
	// (edits/combos -> DarkMode_CFD, buttons/tree/list/updown -> DarkMode_Explorer).
	void ApplyThemeToChildren(HWND hWndParent);

	// Our owner-drawn group boxes fill their whole rect with the dark face colour. When a group
	// box is defined after the controls it frames (so it sits above them in the z-order), that
	// fill paints over those controls and they vanish until individually invalidated (on hover,
	// an enable toggle, or Apply). Push every group box to the bottom of the sibling z-order and
	// give it WS_CLIPSIBLINGS so its fill is clipped out of the controls it contains. Call AFTER
	// ApplyThemeToChildren (never during it) so the child enumeration isn't disturbed.
	void FixGroupBoxes(HWND hWndParent);

	// Applies the same dark visual style to a single control. Use for controls created
	// on the fly (e.g. the in-place combo/edit of a list control) that never pass
	// through ApplyThemeToChildren.
	void ApplyThemeToControl(HWND hCtrl);

	// Fully owner-draws a trackbar (dark background, dark groove, celeste thumb) via a
	// subclass. Use for sliders whose NM_CUSTOMDRAW channel colour is not reliably applied
	// (e.g. the subtitle Default Style alpha sliders after the Reset button).
	// bThemeControl marks the R/G/B/Brightness sliders (which control the theme colour): they
	// paint from a committed snapshot instead of the live colour, so the one being dragged doesn't
	// recolour under the cursor. Call CommitThemeColors() when a drag ends to update the snapshot.
	void MakeTrackbarOwnerDrawn(HWND hTrackbar, bool bThemeControl = false);
	void CommitThemeColors();

	// Applies the dark theme to a whole auxiliary top-level dialog opened from the Options
	// pages (dark title bar, dark background + control colours, themed child controls).
	// Call once from the dialog's OnInitDialog.
	void ThemeDialog(HWND hDlg);

	// Re-applies or removes the dark theme across a whole window tree in response to the
	// "Use dark theme" toggle being changed at runtime (Interface page + Apply). Handles both
	// directions — applying when now active, stripping every subclass/override when now
	// inactive — and forces a full redraw. Pass the Options property sheet HWND.
	void RefreshTheme(HWND hRoot);

	// Re-tints the themed backgrounds live as the R/G/B/Brightness sliders move, so the Options
	// dialog tracks the player colour in real time (the text colour stays fixed). Pass the sheet.
	void RefreshColors(HWND hRoot);

	// Builds a dark themed checkbox STATE image list for a list-view using
	// LVS_EX_CHECKBOXES (index 0 = none, 1 = unchecked, 2 = checked). Assigning this
	// via LVSIL_STATE keeps the checkboxes dark even when the list is disabled (the
	// native auto-created checkboxes render with an ugly light border when disabled).
	// When bDisabled is true the greyed-out (disabled) checkbox glyph is used, to show
	// an inactive list. Returns false when theming is unavailable (keep the default).
	bool MakeCheckStateImageList(CImageList& il, int size, HWND hRef, bool bDisabled = false);

	// WM_CTLCOLOR* helper: sets dark text/background on pDC and returns a cached
	// dark brush, or nullptr when the dark theme is inactive (use default handling).
	HBRUSH OnCtlColor(CDC* pDC, UINT nCtlColor);

	// NM_CUSTOMDRAW helper for trackbars (sliders): paints the channel dark, which
	// Windows' native dark mode does not do. Returns true (with *pResult set) when
	// the notification came from a trackbar and was handled; false otherwise.
	bool TrackbarCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

	// NM_CUSTOMDRAW helper for checkboxes and radio buttons: draws the native theme
	// glyph plus a light caption. Native dark mode gives checkboxes light text but
	// leaves radio-button text black, so both are drawn here for consistency.
	// Returns true (with *pResult set) only for check/radio buttons; false otherwise.
	bool ButtonCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

	// Background palette follows the R/G/B/Brightness sliders (tints in real time to match the
	// player); TextColor() is fixed so text stays readable regardless of the slider values.
	COLORREF FaceColor();       // dialog / page / static background
	COLORREF TextColor();       // text
	COLORREF CtrlBackColor();   // sunken control interior (edit / listbox)
	COLORREF CtrlBorderColor(); // shared 1px border for every control (edits, spins, color wells, group boxes, tabs)
	COLORREF GridlineColor();   // list-view grid lines (darker than the control borders)
}
