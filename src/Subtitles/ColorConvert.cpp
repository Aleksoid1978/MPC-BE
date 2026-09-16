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

#include "stdafx.h"
#include "ColorConvert.h"

namespace ColorConvert {
	namespace {
		constexpr double rgb_low_TV = 16.0;
		constexpr double rgb_high_TV = 219.0;

		struct CS { double Kr, Kb, Kg; };
		constexpr CS k601{ 0.299,  0.114,  0.587 };
		constexpr CS k709{ 0.2125, 0.0721, 0.7154 };
		constexpr CS k2020{ 0.2627, 0.0593, 0.6780 };
	}

	void Converter::Set(ColorSpace cs, ConvertType type)
	{
		if (cs == m_cs && type == m_type) {
			return;
		}

		m_cs = cs;
		m_type = type;

		const CS c = cs == ColorSpace::BT2020 ? k2020
			: cs == ColorSpace::REC709 ? k709
			: k601;

		const bool srcTV = (type == ConvertType::TV_2_TV || type == ConvertType::TV_2_PC);
		const bool dstPC = (type == ConvertType::PC_2_PC || type == ConvertType::TV_2_PC);

		const double src_low = srcTV ? rgb_low_TV : 0.0;
		m_rgb_low = dstPC ? 0.0 : rgb_low_TV;
		m_rgb_high = dstPC ? 255.0 : rgb_high_TV;
		const double coeff = (dstPC ? 255.0 : rgb_high_TV) / (srcTV ? rgb_high_TV : 255.0);

		for (int i = 0; i < 256; i++) {
			const double y = (i - src_low) * coeff;
			m_ry[i] = y; m_gy[i] = y; m_by[i] = y;

			const double cr = i - 128.0;
			const double cb = i - 128.0;
			m_rv[i] = 2.0 * cr * (1.0 - c.Kr);
			m_gv[i] = -2.0 * cr * (1.0 - c.Kr) * c.Kr / c.Kg;
			m_gu[i] = -2.0 * cb * (1.0 - c.Kb) * c.Kb / c.Kg;
			m_bu[i] = 2.0 * cb * (1.0 - c.Kb);
		}
	}

	DWORD Converter::YCrCbToRGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb) const
	{
		const double r = std::clamp(m_ry[Y] + m_rv[Cr], 0.0, m_rgb_high) + m_rgb_low;
		const double g = std::clamp(m_gy[Y] + m_gu[Cb] + m_gv[Cr], 0.0, m_rgb_high) + m_rgb_low;
		const double b = std::clamp(m_by[Y] + m_bu[Cb], 0.0, m_rgb_high) + m_rgb_low;

		return D3DCOLOR_ARGB(A, (BYTE)r, (BYTE)g, (BYTE)b);
	}
} // namespace ColorConvert
