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

#pragma  once

#include <ObjBase.h>
#include <strmif.h>

DECLARE_INTERFACE_IID_(ISubtitleHelper, IUnknown, "10505DE9-0759-483C-8B4E-BFCEA450861A")
{
	STDMETHOD(Connect)(HWND hWnd) PURE; // should be called after MPC-VR and XYSubFilter have been added to the graph, but before the filters are connected.
	STDMETHOD(Disconnect)() PURE; // can be optionally called before the graph is torn down

	// for compatibility with players that use the IBasicVideo and IVideoWindow interfaces to 
	// send positioning information to the video renderer, and want the subtitles to follow
	// suit
	STDMETHOD_(void, SetPositionFromRenderer)() PURE;
};
