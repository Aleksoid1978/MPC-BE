/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "ShaderAutoCompleteDlg.h"

// CShaderAutoCompleteDlg dialog

CShaderAutoCompleteDlg::CShaderAutoCompleteDlg(CWnd* pParent)
	: CResizableDialog(CShaderAutoCompleteDlg::IDD, pParent)
{
	m_text[0] = 0;

	m_inst[L"abs"] = L"abs(value a)|Absolute value (per component). ";
	m_inst[L"acos"] = L"acos(x)|Returns the arccosine of each component of x. Each component should be in the range [-1, 1]. ";
	m_inst[L"all"] = L"all(x)|Test if all components of x are nonzero. ";
	m_inst[L"any"] = L"any(x)|Test is any component of x is nonzero. ";
	m_inst[L"asin"] = L"asin(x)|Returns the arcsine of each component of x. Each component should be in the range [-pi/2, pi/2]. ";
	m_inst[L"atan"] = L"atan(x)|Returns the arctangent of x. The return values are in the range [-pi/2, pi/2]. ";
	m_inst[L"atan2"] = L"atan2(y, x)|Returns the arctangent of y/x. The signs of y and x are used to determine the quadrant of the return values in the range [-pi, pi]. atan2 is well-defined for every point other than the origin, even if x equals 0 and y does not equal 0. ";
	m_inst[L"ceil"] = L"ceil(x)|Returns the smallest integer which is greater than or equal to x. ";
	m_inst[L"clamp"] = L"clamp(x, min, max)|Clamps x to the range [min, max]. ";
	m_inst[L"clip"] = L"clip(x)|Discards the current pixel, if any component of x is less than zero. This can be used to simulate clip planes, if each component of x represents the distance from a plane. ";
	m_inst[L"cos"] = L"cos(x)|Returns the cosine of x. ";
	m_inst[L"cosh"] = L"cosh(x)|Returns the hyperbolic cosine of x. ";
	m_inst[L"cross"] = L"cross(a, b)|Returns the cross product of two 3-D vectors a and b. ";
	m_inst[L"d3dcolortoubyte4"] = L"D3DCOLORtoUBYTE4(x)|Swizzles and scales components of the 4-D vector x to compensate for the lack of UBYTE4 support in some hardware. ";
	m_inst[L"ddx"] = L"ddx(x)|Returns the partial derivative of x with respect to the screen-space x-coordinate. ";
	m_inst[L"ddy"] = L"ddy(x)|Returns the partial derivative of x with respect to the screen-space y-coordinate. ";
	m_inst[L"degrees"] = L"degrees(x)|Converts x from radians to degrees. ";
	m_inst[L"determinant"] = L"determinant(m)|Returns the determinant of the square matrix m. ";
	m_inst[L"distance"] = L"distance(a, b)|Returns the distance between two points a and b. ";
	m_inst[L"dot"] = L"dot(a, b)|Returns the dot product of two vectors a and b. ";
	m_inst[L"exp"] = L"exp(x)|Returns the base-e exponent ex. ";
	m_inst[L"exp2"] = L"exp2(value a)|Base 2 Exp (per component). ";
	m_inst[L"faceforward"] = L"faceforward(n, i, ng)|Returns -n * sign(dot(i, ng)). ";
	m_inst[L"floor"] = L"floor(x)|Returns the greatest integer which is less than or equal to x. ";
	m_inst[L"fmod"] = L"fmod(a, b)|Returns the floating point remainder f of a / b such that a = i * b + f, where i is an integer, f has the same sign as x, and the absolute value of f is less than the absolute value of b. ";
	m_inst[L"frac"] = L"frac(x)|Returns the fractional part f of x, such that f is a value greater than or equal to 0, and less than 1. ";
	m_inst[L"frc"] = L"frc(value a)|Fractional part (per component). ";
	m_inst[L"frexp"] = L"frexp(x, out exp)|Returns the mantissa and exponent of x. frexp returns the mantissa, and the exponent is stored in the output parameter exp. If x is 0, the function returns 0 for both the mantissa and the exponent. ";
	m_inst[L"fwidth"] = L"fwidth(x)|Returns abs(ddx(x))+abs(ddy(x)). ";
	m_inst[L"isfinite"] = L"isfinite(x)|Returns true if x is finite, false otherwise. ";
	m_inst[L"isinf"] = L"isinf(x)|Returns true if x is +INF or -INF, false otherwise. ";
	m_inst[L"isnan"] = L"isnan(x)|Returns true if x is NAN or QNAN, false otherwise. ";
	m_inst[L"ldexp"] = L"ldexp(x, exp)|Returns x * 2exp. ";
	m_inst[L"len"] = L"len(value a)|Vector length. ";
	m_inst[L"length"] = L"length(v)|Returns the length of the vector v. ";
	m_inst[L"lerp"] = L"lerp(a, b, s)|Returns a + s(b - a). This linearly interpolates between a and b, such that the return value is a when s is 0, and b when s is 1. ";
	m_inst[L"lit"] = L"lit(ndotl, ndoth, m)|Returns a lighting vector (ambient, diffuse, specular, 1): ambient = 1; diffuse = (ndotl < 0) ? 0 : ndotl; specular = (ndotl < 0) || (ndoth < 0) ? 0 : (ndoth * m); ";
	m_inst[L"log"] = L"log(x)|Returns the base-e logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. ";
	m_inst[L"log10"] = L"log10(x)|Returns the base-10 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. ";
	m_inst[L"log2"] = L"log2(x)|Returns the base-2 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. ";
	m_inst[L"max"] = L"max(a, b)|Selects the greater of a and b. ";
	m_inst[L"min"] = L"min(a, b)|Selects the lesser of a and b. ";
	m_inst[L"modf"] = L"modf(x, out ip)|Splits the value x into fractional and integer parts, each of which has the same sign and x. The signed fractional portion of x is returned. The integer portion is stored in the output parameter ip. ";
	m_inst[L"mul"] = L"mul(a, b)|Performs matrix multiplication between a and b. If a is a vector, it treated as a row vector. If b is a vector, it is treated as a column vector. The inner dimension acolumns and brows must be equal. The result has the dimension arows x bcolumns. ";
	m_inst[L"noise"] = L"noise(x)|Not yet implemented. ";
	m_inst[L"normalize"] = L"normalize(v)|Returns the normalized vector v / length(v). If the length of v is 0, the result is indefinite. ";
	m_inst[L"pow"] = L"pow(x, y)|Returns xy. ";
	m_inst[L"radians"] = L"radians(x)|Converts x from degrees to radians. ";
	m_inst[L"reflect"] = L"reflect(i, n)|Returns the reflection vector v, given the entering ray direction i, and the surface normal n. Such that v = i - 2 * dot(i, n) * n ";
	m_inst[L"refract"] = L"refract(i, n, eta)|Returns the refraction vector v, given the entering ray direction i, the surface normal n, and the relative index of refraction eta. If the angle between i and n is too great for a given eta, refract returns (0,0,0). ";
	m_inst[L"round"] = L"round(x)|Rounds x to the nearest integer. ";
	m_inst[L"rsqrt"] = L"rsqrt(x)|Returns 1 / sqrt(x). ";
	m_inst[L"saturate"] = L"saturate(x)|Clamps x to the range [0, 1]. ";
	m_inst[L"sign"] = L"sign(x)|Computes the sign of x. Returns -1 if x is less than 0, 0 if x equals 0, and 1 if x is greater than zero. ";
	m_inst[L"sin"] = L"sin(x)|Returns the sine of x. ";
	m_inst[L"sincos"] = L"sincos(x, out s, out c)|Returns the sine and cosine of x. sin(x) is stored in the output parameter s. cos(x) is stored in the output parameter c. ";
	m_inst[L"sinh"] = L"sinh(x)|Returns the hyperbolic sine of x. ";
	m_inst[L"smoothstep"] = L"smoothstep(min, max, x)|Returns 0 if x < min. Returns 1 if x > max. Returns a smooth Hermite interpolation between 0 and 1, if x is in the range [min, max]. ";
	m_inst[L"sqrt"] = L"sqrt(value a)|Square root (per component). ";
	m_inst[L"step"] = L"step(a, x)|Returns (x >= a) ? 1 : 0. ";
	m_inst[L"tan"] = L"tan(x)|Returns the tangent of x. ";
	m_inst[L"tanh"] = L"tanh(x)|Returns the hyperbolic tangent of x. ";
	m_inst[L"tex1d"] = L"tex1D(s, t)|1-D texture lookup. s is a sampler or a sampler1D object. t is a scalar. ";
	m_inst[L"tex1d("] = L"tex1D(s, t, ddx, ddy)|1-D texture lookup, with derivatives. s is a sampler or sampler1D object. t, ddx, and ddy are scalars. ";
	m_inst[L"tex1dproj"] = L"tex1Dproj(s, t)|1-D projective texture lookup. s is a sampler or sampler1D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. ";
	m_inst[L"tex1dbias"] = L"tex1Dbias(s, t)|1-D biased texture lookup. s is a sampler or sampler1D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. ";
	m_inst[L"tex2d"] = L"tex2D(s, t)|2-D texture lookup. s is a sampler or a sampler2D object. t is a 2-D texture coordinate. ";
	m_inst[L"tex2d("] = L"tex2D(s, t, ddx, ddy)|2-D texture lookup, with derivatives. s is a sampler or sampler2D object. t, ddx, and ddy are 2-D vectors. ";
	m_inst[L"tex2dproj"] = L"tex2Dproj(s, t)|2-D projective texture lookup. s is a sampler or sampler2D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. ";
	m_inst[L"tex2dbias"] = L"tex2Dbias(s, t)|2-D biased texture lookup. s is a sampler or sampler2D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. ";
	m_inst[L"tex3d"] = L"tex3D(s, t)|3-D volume texture lookup. s is a sampler or a sampler3D object. t is a 3-D texture coordinate. ";
	m_inst[L"tex3d("] = L"tex3D(s, t, ddx, ddy)|3-D volume texture lookup, with derivatives. s is a sampler or sampler3D object. t, ddx, and ddy are 3-D vectors. ";
	m_inst[L"tex3dproj"] = L"tex3Dproj(s, t)|3-D projective volume texture lookup. s is a sampler or sampler3D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. ";
	m_inst[L"tex3dbias"] = L"tex3Dbias(s, t)|3-D biased texture lookup. s is a sampler or sampler3D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. ";
	m_inst[L"texcube"] = L"texCUBE(s, t)|3-D cube texture lookup. s is a sampler or a samplerCUBE object. t is a 3-D texture coordinate. ";
	m_inst[L"texcube("] = L"texCUBE(s, t, ddx, ddy)|3-D cube texture lookup, with derivatives. s is a sampler or samplerCUBE object. t, ddx, and ddy are 3-D vectors. ";
	m_inst[L"texcubeproj"] = L"texCUBEproj(s, t)|3-D projective cube texture lookup. s is a sampler or samplerCUBE object. t is a 4-D vector. t is divided by its last component before the lookup takes place. ";
	m_inst[L"texcubebias"] = L"texCUBEbias(s, t)|3-D biased cube texture lookup. s is a sampler or samplerCUBE object. t is a 4-dimensional vector. The mip level is biased by t.w before the lookup takes place.  ";
	m_inst[L"transpose"] = L"transpose(m)|Returns the transpose of the matrix m. If the source is dimension mrows x mcolumns, the result is dimension mcolumns x mrows. ";
}

CShaderAutoCompleteDlg::~CShaderAutoCompleteDlg()
{
}

void CShaderAutoCompleteDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CShaderAutoCompleteDlg, CResizableDialog)
	ON_WM_SETFOCUS()
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CShaderAutoCompleteDlg message handlers

BOOL CShaderAutoCompleteDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);

	m_hToolTipWnd = CreateWindowEx(
						WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						NULL, NULL, NULL, NULL);

	memset(&m_ti, 0, sizeof(m_ti));
	m_ti.cbSize = sizeof(TOOLINFO);
	m_ti.uFlags = TTF_ABSOLUTE|TTF_TRACK;
	m_ti.hwnd = m_hWnd;
	m_ti.lpszText = m_text;

	::SendMessage(m_hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&m_ti);
	::SendMessage(m_hToolTipWnd, TTM_SETMAXTIPWIDTH, 0, (LPARAM)400);

	return TRUE;
}

void CShaderAutoCompleteDlg::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	GetParent()->SetFocus();
}

void CShaderAutoCompleteDlg::OnLbnSelchangeList1()
{
	::SendMessage(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);

	int i = m_list.GetCurSel();

	if (i < 0) {
		return;
	}

	if (POSITION pos = (POSITION)m_list.GetItemData(i)) {
		CString str, desc;
		m_inst.GetNextAssoc(pos, str, desc);
		CAtlList<CString> sl;
		Explode(desc, sl, L'|', 2);

		if (sl.GetCount() != 2) {
			return;
		}

		wcscpy_s(m_ti.lpszText, _countof(m_text), sl.RemoveTail());
		CRect r;
		GetWindowRect(r);
		::SendMessage(m_hToolTipWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&m_ti);
		::SendMessage(m_hToolTipWnd, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(r.left, r.bottom+1));
		::SendMessage(m_hToolTipWnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
	}
}

void CShaderAutoCompleteDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CResizableDialog::OnShowWindow(bShow, nStatus);

	if (!bShow) {
		::SendMessage(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
	}
}
