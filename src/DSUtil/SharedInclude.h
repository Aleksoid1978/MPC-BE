/*
 * (C) 2009-2014 see Authors.txt
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

#pragma warning(disable:4244)
#pragma warning(disable:4995)
#ifdef _WIN64
	#pragma warning(disable:4267)
#endif

#ifdef _DEBUG
	// Remove this if you want to see all the "unsafe" functions used
	// For Release builds _CRT_SECURE_NO_WARNINGS is defined
	#pragma warning(disable:4996)

	#define _CRTDBG_MAP_ALLOC // include Microsoft memory leak detection procedures
	#include <crtdbg.h>
	#define DNew DEBUG_NEW
#else
	#define DNew new
#endif
