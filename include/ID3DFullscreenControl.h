/*
 * (C) 2021 see Authors.txt
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

// Set and query D3DFullscreen mode.
interface __declspec(uuid("8EA1E899-B77D-4777-9F0E-66421BEA50F8"))
ID3DFullscreenControl :
public IUnknown {
	STDMETHOD(SetD3DFullscreen)(bool fEnabled) PURE;
	STDMETHOD(GetD3DFullscreen)(bool * pfEnabled) PURE;
};
