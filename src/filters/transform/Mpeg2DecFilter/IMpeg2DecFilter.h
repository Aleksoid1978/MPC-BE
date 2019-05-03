/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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

enum ditype : int {
	DIAuto = 0,
	DIWeave,
	DIBlend,
	DIBob,
};

interface __declspec(uuid("0ABEAA65-0317-47B9-AE1D-D9EA905AFD25"))
IMpeg2DecFilter :
public IUnknown {
	STDMETHOD(SetDeinterlaceMethod(ditype di)) PURE;
	STDMETHOD_(ditype, GetDeinterlaceMethod()) PURE;
	// Brightness: -255.0 to 255.0, default 0.0
	// Contrast: 0.0 to 10.0, default 1.0
	// Hue: -180.0 to +180.0, default 0.0
	// Saturation: 0.0 to 10.0, default 1.0

	STDMETHOD(SetBrightness(int brightness)) PURE;
	STDMETHOD(SetContrast(int contrast)) PURE;
	STDMETHOD(SetHue(int hue)) PURE;
	STDMETHOD(SetSaturation(int saturation)) PURE;
	STDMETHOD_(int, GetBrightness()) PURE;
	STDMETHOD_(int, GetContrast()) PURE;
	STDMETHOD_(int, GetHue()) PURE;
	STDMETHOD_(int, GetSaturation()) PURE;

	STDMETHOD(EnableForcedSubtitles(bool fEnable)) PURE;
	STDMETHOD_(bool, IsForcedSubtitlesEnabled()) PURE;

	STDMETHOD(EnableInterlaced(bool fEnable)) PURE;
	STDMETHOD_(bool, IsInterlacedEnabled()) PURE;

	STDMETHOD(EnableReadARFromStream(bool fEnable)) PURE;
	STDMETHOD_(bool, IsReadARFromStreamEnabled()) PURE;

	STDMETHOD(Apply()) PURE;
};
