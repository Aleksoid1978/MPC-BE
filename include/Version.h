#ifndef ISPP_INVOKED
/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include "Version_rev.h"

#ifndef MPC_VERSION_REV
#define MPC_VERSION_REV         0
#endif

#define DO_MAKE_STR(x)          #x
#define MAKE_STR(x)             DO_MAKE_STR(x)

#define MPC_VERSION_MAJOR       1
#define MPC_VERSION_MINOR       5
#define MPC_VERSION_PATCH       1

#define MPC_VERSION_STATUS      0
// MPC_VERSION_STATUS: 0 - beta; 1 - stable

#define MPC_WND_CLASS_NAME      "MPC-BE"
#define MPC_YEAR_COMMENTS       "2002-2017"
#define MPC_VERSION_COMMENTS    "http://sourceforge.net/projects/mpcbe/"

#ifndef ISPP_INVOKED

#define MPC_COMP_NAME_STR       L"MPC-BE Team"
#define MPC_COPYRIGHT_STR       L"Copyright © 2002-2017 all contributors, see Authors.txt"

#define MPC_VERSION_NUM         MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_PATCH
#define MPC_VERSION_STR         MAKE_STR(MPC_VERSION_MAJOR) "." \
                                MAKE_STR(MPC_VERSION_MINOR) "." \
                                MAKE_STR(MPC_VERSION_PATCH)

#define MPC_VERSION_NUM_SVN     MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_PATCH,MPC_VERSION_REV
#define MPC_VERSION_STR_SVN     MPC_VERSION_STR "." MAKE_STR(MPC_VERSION_REV)

#endif

// MPC_VERSION_ARCH is currently used in VSFilter only.
#ifdef _WIN64
#define MPC_VERSION_ARCH        "x64"
#else
#define MPC_VERSION_ARCH        "x86"
#endif

#endif
