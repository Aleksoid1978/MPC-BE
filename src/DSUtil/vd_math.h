/*
 * (C) 2020 see Authors.txt
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

__int64 FractionScale64(__int64 a, UINT32 b, UINT32 c);	// faster that Int64x32Div32 from wxutil.h
UINT64  UMulDiv64x32(UINT64 a, UINT32 b, UINT32 c);		// speed equal to FractionScale64
__int64 MulDiv64(__int64 a, __int64 b, __int64 c);		// in x86 mode slower than llMulDiv from wxutil.h. rounding to the nearest integer
