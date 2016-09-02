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

#include "stdafx.h"
#include "ColorConvert.h"

namespace ColorConvert {
	const double coeff_TV_PC = 255.0 / 219.0;

	const double Rec601_Kr = 0.299;
	const double Rec601_Kb = 0.114;
	const double Rec601_Kg = 0.587;

	static void YCrCbToRGB_Rec601_TV(const BYTE Y, const BYTE Cr, const BYTE Cb, double& r, double& g, double& b)
	{
		r = Y + 2 * (Cr - 128) * (1.0 - Rec601_Kr);
		g = Y - 2 * (Cb - 128) * (1.0 - Rec601_Kb) * Rec601_Kb / Rec601_Kg - 2 * (Cr - 128) * (1.0 - Rec601_Kr) * Rec601_Kr / Rec601_Kg;
		b = Y + 2 * (Cb - 128) * (1.0 - Rec601_Kb);

		r = clamp(fabs(r), 0.0, 255.0);
		g = clamp(fabs(g), 0.0, 255.0);
		b = clamp(fabs(b), 0.0, 255.0);
	}

	static void YCrCbToRGB_Rec601_PC(const BYTE Y, const BYTE Cr, const BYTE Cb, double& r, double& g, double& b)
	{
		r = coeff_TV_PC * (Y - 16) + 2 * (Cr - 128) * (1.0 - Rec601_Kr);
		g = coeff_TV_PC * (Y - 16) - 2 * (Cb - 128) * (1.0 - Rec601_Kb) * Rec601_Kb / Rec601_Kg - 2 * (Cr - 128) * (1.0 - Rec601_Kr) * Rec601_Kr / Rec601_Kg;
		b = coeff_TV_PC * (Y - 16) + 2 * (Cb - 128) * (1.0 - Rec601_Kb);

		r = clamp(fabs(r), 0.0, 255.0);
		g = clamp(fabs(g), 0.0, 255.0);
		b = clamp(fabs(b), 0.0, 255.0);
	}

	COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb, bool TV_RANGE/* = true*/)
	{
		double r, g, b;
		if (TV_RANGE) {
			YCrCbToRGB_Rec601_TV(Y, Cr, Cb, r, g, b);
		} else {
			YCrCbToRGB_Rec601_PC(Y, Cr, Cb, r, g, b);
		}

		return RGB(r, g, b);
	}

	DWORD YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb, bool TV_RANGE/* = true*/)
	{
		double r, g, b;
		if (TV_RANGE) {
			YCrCbToRGB_Rec601_TV(Y, Cr, Cb, r, g, b);
		} else {
			YCrCbToRGB_Rec601_PC(Y, Cr, Cb, r, g, b);
		}

		return D3DCOLOR_ARGB(A, (BYTE)(r), (BYTE)(g), (BYTE)(b));
	}

	const double Rec709_Kr = 0.2125;
	const double Rec709_Kb = 0.0721;
	const double Rec709_Kg = 0.7154;

	static void YCrCbToRGB_Rec709_TV(const BYTE Y, const BYTE Cr, const BYTE Cb, double& r, double& g, double& b)
	{
		r = Y + 2 * (Cr - 128) * (1.0 - Rec709_Kr);
		g = Y - 2 * (Cb - 128) * (1.0 - Rec709_Kb) * Rec709_Kb / Rec709_Kg - 2 * (Cr - 128) * (1.0 - Rec709_Kr) * Rec709_Kr / Rec709_Kg;
		b = Y + 2 * (Cb - 128) * (1.0 - Rec709_Kb);

		r = clamp(fabs(r), 0.0, 255.0);
		g = clamp(fabs(g), 0.0, 255.0);
		b = clamp(fabs(b), 0.0, 255.0);
	}

	static void YCrCbToRGB_Rec709_PC(const BYTE Y, const BYTE Cr, const BYTE Cb, double& r, double& g, double& b)
	{
		r = coeff_TV_PC * (Y - 16) + 2 * (Cr - 128) * (1.0 - Rec709_Kr);
		g = coeff_TV_PC * (Y - 16) - 2 * (Cb - 128) * (1.0 - Rec709_Kb) * Rec709_Kb / Rec709_Kg - 2 * (Cr - 128) * (1.0 - Rec709_Kr) * Rec709_Kr / Rec709_Kg;
		b = coeff_TV_PC * (Y - 16) + 2 * (Cb - 128) * (1.0 - Rec709_Kb);

		r = clamp(fabs(r), 0.0, 255.0);
		g = clamp(fabs(g), 0.0, 255.0);
		b = clamp(fabs(b), 0.0, 255.0);
	}

	COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb, bool TV_RANGE/* = true*/)
	{
		double r, g, b;
		if (TV_RANGE) {
			YCrCbToRGB_Rec709_TV(Y, Cr, Cb, r, g, b);
		} else {
			YCrCbToRGB_Rec709_PC(Y, Cr, Cb, r, g, b);
		}

		return RGB(r, g, b);
	}

	DWORD YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb, bool TV_RANGE/* = true*/)
	{
		double r, g, b;
		if (TV_RANGE) {
			YCrCbToRGB_Rec709_TV(Y, Cr, Cb, r, g, b);
		} else {
			YCrCbToRGB_Rec709_PC(Y, Cr, Cb, r, g, b);
		}

		return D3DCOLOR_ARGB(A, (BYTE)(r), (BYTE)(g), (BYTE)(b));
	}
} // namespace ColorConvert
