/*
 * (C) 2016-2018 see Authors.txt
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
	const double rgb_low_PC  = 0.0;
	const double rgb_high_PC = 255.0;

	const double rgb_low_TV  = 16.0;
	const double rgb_high_TV = 219.0;

	const double coeff_default = 1.0;
	const double coeff_TV_2_PC = rgb_high_PC / rgb_high_TV;
	const double coeff_PC_2_TV = rgb_high_TV / rgb_high_PC;

	const double Rec601_Kr = 0.299;
	const double Rec601_Kb = 0.114;
	const double Rec601_Kg = 0.587;

	const double Rec709_Kr = 0.2125;
	const double Rec709_Kb = 0.0721;
	const double Rec709_Kg = 0.7154;

	static void YCrCbToRGB(BYTE Y, BYTE Cr, BYTE Cb, const double Kr, const double Kb, const double Kg, const double coeff, const double yuv_low, const double rgb_low, const double rgb_high, double& r, double& g, double& b)
	{
		Y = (Y - yuv_low);

		r = Y * coeff + 2 * (Cr - 128) * (1.0 - Kr);
		g = Y * coeff - 2 * (Cb - 128) * (1.0 - Kb) * Kb / Kg - 2 * (Cr - 128) * (1.0 - Kr) * Kr / Kg;
		b = Y * coeff + 2 * (Cb - 128) * (1.0 - Kb);

		r = std::clamp(fabs(r), 0.0, rgb_high);
		g = std::clamp(fabs(g), 0.0, rgb_high);
		b = std::clamp(fabs(b), 0.0, rgb_high);

		r += rgb_low;
		g += rgb_low;
		b += rgb_low;
	}

	static DWORD YCrCbToRGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb, const double Kr, const double Kb, const double Kg, convertType type)
	{
		double r, g, b;
		double coeff, yuv_low, rgb_low, rgb_high;

		switch (type) {
			default:
			case convertType::TV_2_TV:
				coeff    = coeff_default;
				yuv_low  = rgb_low_TV;
				rgb_low  = rgb_low_TV;
				rgb_high = rgb_high_TV;
				break;
			case convertType::PC_2_PC:
				coeff    = coeff_default;
				yuv_low  = rgb_low_PC;
				rgb_low  = rgb_low_PC;
				rgb_high = rgb_high_PC;
				break;
			case convertType::TV_2_PC:
				coeff    = coeff_TV_2_PC;
				yuv_low  = rgb_low_TV;
				rgb_low  = rgb_low_PC;
				rgb_high = rgb_high_PC;
				break;
			case convertType::PC_2_TV:
				coeff    = coeff_PC_2_TV;
				yuv_low  = rgb_low_PC;
				rgb_low  = rgb_low_TV;
				rgb_high = rgb_high_TV;
				break;
		}

		YCrCbToRGB(Y, Cr, Cb, Kr, Kb, Kg, coeff, yuv_low, rgb_low, rgb_high, r, g, b);

		return D3DCOLOR_ARGB(A, (BYTE)(r), (BYTE)(g), (BYTE)(b));
	}

	DWORD YCrCbToRGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb, bool bRec709, convertType type/* = convertType::DEFAULT*/)
	{
		if (bRec709) {
			return YCrCbToRGB(A, Y, Cr, Cb, Rec709_Kr, Rec709_Kb, Rec709_Kg, type);
		} else {
			return YCrCbToRGB(A, Y, Cr, Cb, Rec601_Kr, Rec601_Kb, Rec601_Kg, type);
		}
	}
} // namespace ColorConvert
