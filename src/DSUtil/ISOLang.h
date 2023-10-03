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

#pragma once

extern CString			ISO6391ToLanguage(LPCSTR code);
extern CString			ISO6392ToLanguage(LPCSTR code);

extern bool				IsISO639Language(LPCSTR code);
extern CString			ISO639XToLanguage(LPCSTR code, bool bCheckForFullLangName = false);
extern LCID				ISO6391ToLcid(LPCSTR code);
extern LCID				ISO6392ToLcid(LPCSTR code);
extern LPCSTR			ISO6391To6392(LPCSTR code);
extern LPCSTR			ISO6392To6391(LPCSTR code);
extern CString			LanguageToISO6392(LPCWSTR lang);
