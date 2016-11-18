/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

#include "../../../DSUtil/SharedInclude.h"

#define VC_EXTRALEAN	// Exclude rarely-used stuff from Windows headers

#include <afxdisp.h>
#include <afxole.h>
#include <crtdefs.h>
#include <BaseClasses/streams.h>
#include <dvdmedia.h>
#include <mpconfig.h>
#include <memory>

#include "../../../DSUtil/DSUtil.h"

#define CHECK_HR(exp) { if (FAILED(hr = exp)) return hr; }
