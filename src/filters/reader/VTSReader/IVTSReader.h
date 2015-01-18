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

#include <basestruct.h>

interface __declspec(uuid("CFCFBA29-5E0D-4031-BC58-407291F56C11"))
IVTSReader :
public IUnknown {
	STDMETHOD(Apply()) PURE;

	STDMETHOD(SetReadAllProgramChains(BOOL nValue)) PURE;
	STDMETHOD_(BOOL, GetReadAllProgramChains()) PURE;

	STDMETHOD_(REFERENCE_TIME, GetDuration()) PURE;
	STDMETHOD_(AV_Rational, GetAspect()) PURE;
};
