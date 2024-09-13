/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#define VC_EXTRALEAN                        // Exclude rarely-used stuff from Windows headers

#ifndef STRICT_TYPED_ITEMIDS
#define STRICT_TYPED_ITEMIDS
#endif

#include <afxwin.h>                         // MFC core and standard components
#include <afxext.h>                         // MFC extensions
#include <afxdisp.h>                        // MFC Automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>

// disable conflicting defines
#undef DrawText

#include "stdafx_common_cfg.h"
#include "stdafx_common_dshow.h"
#include <qnetwork.h>

#include <cmath>
#include <algorithm>
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <regex>
#include <atomic>

#include "resource.h"
#include "DSUtil/DSUtil.h"
#include "mpc_messages.h"
#include "mplayerc.h"
