/*
 * (C) 2007-2014 see Authors.txt
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
#include "../../../../include/stdafx_common.h"
#include <afxwin.h>                         // MFC core and standard components
#include "../../../../include/stdafx_common_dshow.h"

#include <dx/d3dx9.h>
#include <evr.h>
#include <mfapi.h>
#include <Mferror.h>
#include <vector>
#include "memcpy_sse.h"

#pragma warning(disable:4005)
