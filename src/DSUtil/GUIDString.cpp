/*
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru).
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

#include "stdafx.h"
#include "GUIDString.h"

// modified code from wxdebug.cpp

GUID_STRING_ENTRY MPC_g_GuidNames[] = {
#define MPC_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
{ #name, { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } },
	#include <moreuuids.h>
};

C_MPCGuidNameList m_GuidNames;
int MPC_g_cGuidNames = sizeof(MPC_g_GuidNames) / sizeof(MPC_g_GuidNames[0]);

char *C_MPCGuidNameList::operator [] (const GUID &guid)
{
	for (int i = 0; i < MPC_g_cGuidNames; i++) {
		if (MPC_g_GuidNames[i].guid == guid) {
			return MPC_g_GuidNames[i].szName;
		}
	}

	if (guid == GUID_NULL) {
		return "GUID_NULL";
	}

	return "Unknown GUID Name";
}
