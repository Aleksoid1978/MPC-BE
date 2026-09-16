/*
* (C) 2016-2026 see Authors.txt
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

#include <array>

namespace ColorConvert {
	enum class ConvertType {
		TV_2_TV,
		PC_2_PC,
		TV_2_PC,
		PC_2_TV
	};

	enum class ColorSpace {
		Unknown,
		REC601,
		REC709,
		BT2020
	};

	class Converter {
	public:
		void Set(ColorSpace cs, ConvertType type);

		DWORD YCrCbToRGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb) const;

	private:
		std::array<double, 256> m_ry, m_gy, m_by;
		std::array<double, 256> m_rv, m_gu, m_gv, m_bu;
		double m_rgb_low = 0.0;
		double m_rgb_high = 255.0;

		ColorSpace m_cs = ColorSpace::Unknown;
		ConvertType m_type = ConvertType::TV_2_TV;
	};
} // namespace ColorConvert
