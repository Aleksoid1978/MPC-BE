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

interface __declspec(uuid("1D7FBEA1-D294-4350-B49B-078EACA282C3"))
IMatroskaSplitterFilter :
public IUnknown {
	STDMETHOD(Apply()) PURE;

	STDMETHOD(SetLoadEmbeddedFonts(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetLoadEmbeddedFonts()) PURE;
	STDMETHOD(SetCalcDuration(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetCalcDuration()) PURE;
};
