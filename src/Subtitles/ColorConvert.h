/*
* (C) 2016 see Authors.txt
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

namespace ColorConvert {
	enum convertType {
		DEFAULT,
		TV_2_TV = DEFAULT,
		PC_2_PC,
		TV_2_PC,
		PC_2_TV
	};

	DWORD YCrCbToRGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb, bool bRec709, convertType type = convertType::DEFAULT);
} // namespace ColorConvert
