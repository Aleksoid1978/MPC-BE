/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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

const std::vector<CShaderAutoCompleteDlg::hlslfunc_t> CShaderAutoCompleteDlg::m_HLSLFuncs = {
	hlslfunc_t{L"abs",         L"abs(x)",                    L"Absolute value (per component). "},
	hlslfunc_t{L"acos",        L"acos(x)",                   L"Returns the arccosine of each component of x. Each component should be in the range [-1, 1]. "},
	hlslfunc_t{L"all",         L"all(x)",                    L"Test if all components of x are nonzero. "},
	hlslfunc_t{L"any",         L"any(x)",                    L"Test is any component of x is nonzero. "},
	hlslfunc_t{L"asin",        L"asin(x)",                   L"Returns the arcsine of each component of x. Each component should be in the range [-pi/2, pi/2]. "},
	hlslfunc_t{L"atan",        L"atan(x)",                   L"Returns the arctangent of x. The return values are in the range [-pi/2, pi/2]. "},
	hlslfunc_t{L"atan2",       L"atan2(y, x)",               L"Returns the arctangent of y/x. The signs of y and x are used to determine the quadrant of the return values in the range [-pi, pi]. atan2 is well-defined for every point other than the origin, even if x equals 0 and y does not equal 0. "},
	hlslfunc_t{L"ceil",        L"ceil(x)",                   L"Returns the smallest integer which is greater than or equal to x. "},
	hlslfunc_t{L"clamp",       L"clamp(x, min, max)",        L"Clamps x to the range [min, max]. "},
	hlslfunc_t{L"clip",        L"clip(x)",                   L"Discards the current pixel, if any component of x is less than zero. This can be used to simulate clip planes, if each component of x represents the distance from a plane. "},
	hlslfunc_t{L"cos",         L"cos(x)",                    L"Returns the cosine of x. "},
	hlslfunc_t{L"cosh",        L"cosh(x)",                   L"Returns the hyperbolic cosine of x. "},
	hlslfunc_t{L"cross",       L"cross(a, b)",               L"Returns the cross product of two 3-D vectors a and b. "},
	hlslfunc_t{L"d3dcolortoubyte4", L"D3DCOLORtoUBYTE4(x)",  L"Swizzles and scales components of the 4-D vector x to compensate for the lack of UBYTE4 support in some hardware. "},
	hlslfunc_t{L"ddx",         L"ddx(x)",                    L"Returns the partial derivative of x with respect to the screen-space x-coordinate. "},
	hlslfunc_t{L"ddy",         L"ddy(x)",                    L"Returns the partial derivative of x with respect to the screen-space y-coordinate. "},
	hlslfunc_t{L"degrees",     L"degrees(x)",                L"Converts x from radians to degrees. "},
	hlslfunc_t{L"determinant", L"determinant(m)",            L"Returns the determinant of the square matrix m. "},
	hlslfunc_t{L"distance",    L"distance(a, b)",            L"Returns the distance between two points a and b. "},
	hlslfunc_t{L"dot",         L"dot(a, b)",                 L"Returns the dot product of two vectors a and b. "},
	hlslfunc_t{L"exp",         L"exp(x)",                    L"Returns the base-e exponent ex. "},
	hlslfunc_t{L"exp2",        L"exp2(value a)",             L"Base 2 Exp (per component). "},
	hlslfunc_t{L"faceforward", L"faceforward(n, i, ng)",     L"Returns -n * sign(dot(i, ng)). "},
	hlslfunc_t{L"floor",       L"floor(x)",                  L"Returns the greatest integer which is less than or equal to x. "},
	hlslfunc_t{L"fmod",        L"fmod(a, b)",                L"Returns the floating point remainder f of a / b such that a = i * b + f, where i is an integer, f has the same sign as x, and the absolute value of f is less than the absolute value of b. "},
	hlslfunc_t{L"frac",        L"frac(x)",                   L"Returns the fractional part f of x, such that f is a value greater than or equal to 0, and less than 1. "},
	hlslfunc_t{L"frc",         L"frc(value a)",              L"Fractional part (per component). "},
	hlslfunc_t{L"frexp",       L"frexp(x, out exp)",         L"Returns the mantissa and exponent of x. frexp returns the mantissa, and the exponent is stored in the output parameter exp. If x is 0, the function returns 0 for both the mantissa and the exponent. "},
	hlslfunc_t{L"fwidth",      L"fwidth(x)",                 L"Returns abs(ddx(x))+abs(ddy(x)). "},
	hlslfunc_t{L"isfinite",    L"isfinite(x)",               L"Returns true if x is finite, false otherwise. "},
	hlslfunc_t{L"isinf",       L"isinf(x)",                  L"Returns true if x is +INF or -INF, false otherwise. "},
	hlslfunc_t{L"isnan",       L"isnan(x)",                  L"Returns true if x is NAN or QNAN, false otherwise. "},
	hlslfunc_t{L"ldexp",       L"ldexp(x, exp)",             L"Returns x * 2exp. "},
	hlslfunc_t{L"len",         L"len(value a)",              L"Vector length. "},
	hlslfunc_t{L"length",      L"length(v)",                 L"Returns the length of the vector v. "},
	hlslfunc_t{L"lerp",        L"lerp(a, b, s)",             L"Returns a + s(b - a). This linearly interpolates between a and b, such that the return value is a when s is 0, and b when s is 1. "},
	hlslfunc_t{L"lit",         L"lit(ndotl, ndoth, m)",      L"Returns a lighting vector (ambient, diffuse, specular, 1): ambient = 1; diffuse = (ndotl < 0) ? 0 : ndotl; specular = (ndotl < 0) || (ndoth < 0) ? 0 : (ndoth * m); "},
	hlslfunc_t{L"log",         L"log(x)",                    L"Returns the base-e logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	hlslfunc_t{L"log10",       L"log10(x)",                  L"Returns the base-10 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	hlslfunc_t{L"log2",        L"log2(x)",                   L"Returns the base-2 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	hlslfunc_t{L"max",         L"max(a, b)",                 L"Selects the greater of a and b. "},
	hlslfunc_t{L"min",         L"min(a, b)",                 L"Selects the lesser of a and b. "},
	hlslfunc_t{L"modf",        L"modf(x, out ip)",           L"Splits the value x into fractional and integer parts, each of which has the same sign and x. The signed fractional portion of x is returned. The integer portion is stored in the output parameter ip. "},
	hlslfunc_t{L"mul",         L"mul(a, b)",                 L"Performs matrix multiplication between a and b. If a is a vector, it treated as a row vector. If b is a vector, it is treated as a column vector. The inner dimension acolumns and brows must be equal. The result has the dimension arows x bcolumns. "},
	hlslfunc_t{L"noise",       L"noise(x)",                  L"Not yet implemented. "},
	hlslfunc_t{L"normalize",   L"normalize(v)",              L"Returns the normalized vector v / length(v). If the length of v is 0, the result is indefinite. "},
	hlslfunc_t{L"pow",         L"pow(x, y)",                 L"Returns x^y. "},
	hlslfunc_t{L"radians",     L"radians(x)",                L"Converts x from degrees to radians. "},
	hlslfunc_t{L"reflect",     L"reflect(i, n)",             L"Returns the reflection vector v, given the entering ray direction i, and the surface normal n. Such that v = i - 2 * dot(i, n) * n "},
	hlslfunc_t{L"refract",     L"refract(i, n, eta)",        L"Returns the refraction vector v, given the entering ray direction i, the surface normal n, and the relative index of refraction eta. If the angle between i and n is too great for a given eta, refract returns (0,0,0). "},
	hlslfunc_t{L"round",       L"round(x)",                  L"Rounds x to the nearest integer. "},
	hlslfunc_t{L"rsqrt",       L"rsqrt(x)",                  L"Returns 1 / sqrt(x). "},
	hlslfunc_t{L"saturate",    L"saturate(x)",               L"Clamps x to the range [0, 1]. "},
	hlslfunc_t{L"sign",        L"sign(x)",                   L"Computes the sign of x. Returns -1 if x is less than 0, 0 if x equals 0, and 1 if x is greater than zero. "},
	hlslfunc_t{L"sin",         L"sin(x)",                    L"Returns the sine of x. "},
	hlslfunc_t{L"sincos",      L"sincos(x, out s, out c)",   L"Returns the sine and cosine of x. sin(x) is stored in the output parameter s. cos(x) is stored in the output parameter c. "},
	hlslfunc_t{L"sinh",        L"sinh(x)",                   L"Returns the hyperbolic sine of x. "},
	hlslfunc_t{L"smoothstep",  L"smoothstep(min, max, x)",   L"Returns 0 if x < min. Returns 1 if x > max. Returns a smooth Hermite interpolation between 0 and 1, if x is in the range [min, max]. "},
	hlslfunc_t{L"sqrt",        L"sqrt(value a)",             L"Square root (per component). "},
	hlslfunc_t{L"step",        L"step(a, x)",                L"Returns (x >= a) ? 1 : 0. "},
	hlslfunc_t{L"tan",         L"tan(x)",                    L"Returns the tangent of x. "},
	hlslfunc_t{L"tanh",        L"tanh(x)",                   L"Returns the hyperbolic tangent of x. "},
	hlslfunc_t{L"tex1d",       L"tex1D(s, t)",               L"1-D texture lookup. s is a sampler or a sampler1D object. t is a scalar. "},
	hlslfunc_t{L"tex1d(",      L"tex1D(s, t, ddx, ddy)",     L"1-D texture lookup, with derivatives. s is a sampler or sampler1D object. t, ddx, and ddy are scalars. "},
	hlslfunc_t{L"tex1dbias",   L"tex1Dbias(s, t)",           L"1-D biased texture lookup. s is a sampler or sampler1D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. "},
	hlslfunc_t{L"tex1dgrad",   L"tex1Dgrad(s, t, ddx, ddy)", L" "},
	hlslfunc_t{L"tex1dlod",    L"tex1Dlod(s, t)",            L" "},
	hlslfunc_t{L"tex1dproj",   L"tex1Dproj(s, t)",           L"1-D projective texture lookup. s is a sampler or sampler1D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. "},
	hlslfunc_t{L"tex2d",       L"tex2D(s, t)",               L"2-D texture lookup. s is a sampler or a sampler2D object. t is a 2-D texture coordinate. "},
	hlslfunc_t{L"tex2d(",      L"tex2D(s, t, ddx, ddy)",     L"2-D texture lookup, with derivatives. s is a sampler or sampler2D object. t, ddx, and ddy are 2-D vectors. "},
	hlslfunc_t{L"tex2dbias",   L"tex2Dbias(s, t)",           L"2-D biased texture lookup. s is a sampler or sampler2D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. "},
	hlslfunc_t{L"tex2dgrad",   L"tex2Dgrad(s, t, ddx, ddy)", L" "},
	hlslfunc_t{L"tex2dlod",    L"tex2Dlod(s, t)",            L" "},
	hlslfunc_t{L"tex2dproj",   L"tex2Dproj(s, t)",           L"2-D projective texture lookup. s is a sampler or sampler2D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. "},
	hlslfunc_t{L"tex3d",       L"tex3D(s, t)",               L"3-D volume texture lookup. s is a sampler or a sampler3D object. t is a 3-D texture coordinate. "},
	hlslfunc_t{L"tex3d(",      L"tex3D(s, t, ddx, ddy)",     L"3-D volume texture lookup, with derivatives. s is a sampler or sampler3D object. t, ddx, and ddy are 3-D vectors. "},
	hlslfunc_t{L"tex3dbias",   L"tex3Dbias(s, t)",           L"3-D biased texture lookup. s is a sampler or sampler3D object. t is a 4-D vector. The mip level is biased by t.w before the lookup takes place. "},
	hlslfunc_t{L"tex3dgrad",   L"tex3Dgrad(s, t, ddx, ddy)", L" "},
	hlslfunc_t{L"tex3dlod",    L"tex3Dlod(s, t)",            L" "},
	hlslfunc_t{L"tex3dproj",   L"tex3Dproj(s, t)",           L"3-D projective volume texture lookup. s is a sampler or sampler3D object. t is a 4-D vector. t is divided by its last component before the lookup takes place. "},
	hlslfunc_t{L"texcube",     L"texCUBE(s, t)",             L"3-D cube texture lookup. s is a sampler or a samplerCUBE object. t is a 3-D texture coordinate. "},
	hlslfunc_t{L"texcube(",    L"texCUBE(s, t, ddx, ddy)",   L"3-D cube texture lookup, with derivatives. s is a sampler or samplerCUBE object. t, ddx, and ddy are 3-D vectors. "},
	hlslfunc_t{L"texcubebias", L"texCUBEbias(s, t)",         L"3-D biased cube texture lookup. s is a sampler or samplerCUBE object. t is a 4-dimensional vector. The mip level is biased by t.w before the lookup takes place.  "},
	hlslfunc_t{L"texcubegrad", L"texCUBEgrad(s, t, ddx, ddy)", L" "},
	hlslfunc_t{L"texcubelod",  L"texCUBElod(s, t)",          L" "},
	hlslfunc_t{L"texcubeproj", L"texCUBEproj(s, t)",         L"3-D projective cube texture lookup. s is a sampler or samplerCUBE object. t is a 4-D vector. t is divided by its last component before the lookup takes place. "},
	hlslfunc_t{L"transpose",   L"transpose(m)",              L"Returns the transpose of the matrix m. If the source is dimension mrows x mcolumns, the result is dimension mcolumns x mrows. "},
	hlslfunc_t{L"trunc",       L"trunc(x)",                  L" "}
};

//const std::vector<CShaderAutoCompleteDlg::hlslfunc_t>* CShaderAutoCompleteDlg::m_pHLSLFuncs = s_HLSLFuncs;

CShaderAutoCompleteDlg::CShaderAutoCompleteDlg(CWnd* pParent)
	: CResizableDialog(CShaderAutoCompleteDlg::IDD, pParent)
{
	m_text[0] = 0;
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
						WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						nullptr, nullptr, nullptr, nullptr);

	memset(&m_ti, 0, sizeof(m_ti));
	m_ti.cbSize = sizeof(TOOLINFO);
	m_ti.uFlags = TTF_ABSOLUTE|TTF_TRACK;
	m_ti.hwnd = m_hWnd;
	m_ti.lpszText = m_text;

	::SendMessageW(m_hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&m_ti);
	::SendMessageW(m_hToolTipWnd, TTM_SETMAXTIPWIDTH, 0, (LPARAM)400);

	return TRUE;
}

void CShaderAutoCompleteDlg::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	GetParent()->SetFocus();
}

void CShaderAutoCompleteDlg::OnLbnSelchangeList1()
{
	::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);

	int i = m_list.GetCurSel();

	if (i < 0) {
		return;
	}

	if (auto hlslfunc = (hlslfunc_t*)m_list.GetItemData(i)) {

		wcscpy_s(m_ti.lpszText, _countof(m_text), hlslfunc->desc);
		CRect r;
		GetWindowRect(r);
		::SendMessageW(m_hToolTipWnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&m_ti);
		::SendMessageW(m_hToolTipWnd, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(r.left, r.bottom+1));
		::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
	}
}

void CShaderAutoCompleteDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CResizableDialog::OnShowWindow(bShow, nStatus);

	if (!bShow) {
		::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
	}
}
