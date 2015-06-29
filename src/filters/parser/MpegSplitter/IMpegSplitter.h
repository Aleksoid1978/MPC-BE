/*
 * (C) 2011-2014 see Authors.txt
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

interface __declspec(uuid("1DC9C085-04AC-4BB8-B2BD-C49A4D30B104"))
IMpegSplitterFilter :
public IUnknown {
	STDMETHOD(Apply()) PURE;

	STDMETHOD(SetForcedSub(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetForcedSub()) PURE;

	STDMETHOD(SetAudioLanguageOrder(WCHAR *nValue)) PURE;
	STDMETHOD_(WCHAR *, GetAudioLanguageOrder()) PURE;

	STDMETHOD(SetSubtitlesLanguageOrder(WCHAR *nValue)) PURE;
	STDMETHOD_(WCHAR *, GetSubtitlesLanguageOrder()) PURE;

	STDMETHOD(SetTrueHD(int nValue)) PURE;
	STDMETHOD_(int, GetTrueHD()) PURE;

	STDMETHOD(SetSubEmptyPin(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetSubEmptyPin()) PURE;

	STDMETHOD_(int, GetMPEGType()) PURE;
};
