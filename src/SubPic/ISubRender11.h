// EXPERIMENTAL!

/*
 * (C) 2022 see Authors.txt
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

#include <ObjBase.h>
#include <strmif.h>

interface ID3D11DeviceContext1;

DECLARE_INTERFACE_IID_(ISubRender11Callback, IUnknown, "1B430F17-4CB2-4C6B-A850-1847F9677C75")
{
	// NULL means release current device, textures and other resources
	STDMETHOD(SetDevice)(ID3D11Device1* dev) PURE;

	// destination video rectangle, will be inside (0, 0)-(width, height)
	// width,height is the size of the entire output window
	STDMETHOD(Render11)(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
						REFERENCE_TIME avgTimePerFrame, RECT croppedVideoRect,
						RECT originalVideoRect, RECT viewportRect,
						const double videoStretchFactor = 1.0,
						int xOffsetInPixels = 0, DWORD flags = 0) PURE;
};

DECLARE_INTERFACE_IID_(ISubRender11, IUnknown, "524FA4AC-35CF-402B-9015-300FBC432563")
{
	STDMETHOD(SetCallback11)(ISubRender11Callback* cb) PURE;
};
