#ifndef ISPP_INVOKED
/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#endif

#ifndef MPC_VERSION_H
#define MPC_VERSION_H

#include "../revision.h"

#ifndef REV_DATE
#define REV_DATE       0
#endif

#ifndef REV_NUM
#define REV_NUM        0
#endif

#ifndef REV_BRANCH
#define REV_BRANCH     LOCAL
#endif

#define MPC_VERSION_REV         REV_NUM

#define DO_MAKE_STR(x)          #x
#define MAKE_STR(x)             DO_MAKE_STR(x)

#define MPC_VERSION_MAJOR       1
#define MPC_VERSION_MINOR       8
#define MPC_VERSION_PATCH       2

#define MPC_VERSION_STATUS      0
// MPC_VERSION_STATUS: 0 - dev; 1 - stable

#define MPC_YEAR_COMMENTS       "2002-2025"
#define MPC_VERSION_COMMENTS    "https://sourceforge.net/projects/mpcbe/"

#ifndef ISPP_INVOKED

#define MPC_COMP_NAME_STR       L"MPC-BE Team"
#define MPC_COPYRIGHT_STR       L"Copyright © 2002-2025 all contributors, see Authors.txt"

#define MPC_VERSION_FULL_NUM    MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_PATCH,MPC_VERSION_REV
#define MPC_VERSION_FULL_STR    MAKE_STR(MPC_VERSION_MAJOR) "." MAKE_STR(MPC_VERSION_MINOR) "." MAKE_STR(MPC_VERSION_PATCH) "." MAKE_STR(MPC_VERSION_REV)
#define MPC_VERSION_FULL_WSTR   _CRT_WIDE(MPC_VERSION_FULL_STR)

#if MPC_VERSION_STATUS == 1 && MPC_VERSION_REV == 0
#define MPC_VERSION_NUM         MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_PATCH
#define MPC_VERSION_STR         MAKE_STR(MPC_VERSION_MAJOR) "." MAKE_STR(MPC_VERSION_MINOR) "." MAKE_STR(MPC_VERSION_PATCH)
#define MPC_VERSION_WSTR        _CRT_WIDE(MPC_VERSION_STR)
#else
#define MPC_VERSION_NUM         MPC_VERSION_FULL_NUM
#define MPC_VERSION_STR         MPC_VERSION_FULL_STR
#define MPC_VERSION_WSTR        MPC_VERSION_FULL_WSTR
#endif

#endif

#endif
