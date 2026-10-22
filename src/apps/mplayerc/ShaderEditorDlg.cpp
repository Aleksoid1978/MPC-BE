/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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
#include "MainFrm.h"
#include "ShaderNewDlg.h"
#include "ShaderEditorDlg.h"
#include "controls/DarkTheme.h"

static const CShaderAutoCompleteDlg::hlslfunc_t s_HLSLFuncs[] = {
	// ps_3_0
	{L"abs",         L"abs(x)",                    L"Absolute value (per component). "},
	{L"acos",        L"acos(x)",                   L"Returns the arccosine of each component of x. Each component should be in the range [-1, 1]. "},
	{L"all",         L"all(x)",                    L"Test if all components of x are nonzero. "},
	{L"any",         L"any(x)",                    L"Test is any component of x is nonzero. "},
	{L"asin",        L"asin(x)",                   L"Returns the arcsine of each component of x. Each component should be in the range [-pi/2, pi/2]. "},
	{L"atan",        L"atan(x)",                   L"Returns the arctangent of x. The return values are in the range [-pi/2, pi/2]. "},
	{L"atan2",       L"atan2(y, x)",               L"Returns the arctangent of y/x. The signs of y and x are used to determine the quadrant of the return values in the range [-pi, pi]. atan2 is well-defined for every point other than the origin, even if x equals 0 and y does not equal 0. "},
	{L"ceil",        L"ceil(x)",                   L"Returns the smallest integer which is greater than or equal to x. "},
	{L"clamp",       L"clamp(x, min, max)",        L"Clamps x to the range [min, max]. "},
	{L"clip",        L"clip(x)",                   L"Discards the current pixel, if any component of x is less than zero. This can be used to simulate clip planes, if each component of x represents the distance from a plane. "},
	{L"cos",         L"cos(x)",                    L"Returns the cosine of x. "},
	{L"cosh",        L"cosh(x)",                   L"Returns the hyperbolic cosine of x. "},
	{L"cross",       L"cross(a, b)",               L"Returns the cross product of two 3D vectors a and b. "},
	{L"d3dcolortoubyte4", L"D3DCOLORtoUBYTE4(x)",  L"Swizzles and scales components of the 4D vector x to compensate for the lack of UBYTE4 support in some hardware. "},
	{L"ddx",         L"ddx(x)",                    L"Returns the partial derivative of x with respect to the screen-space x-coordinate. "},
	{L"ddy",         L"ddy(x)",                    L"Returns the partial derivative of x with respect to the screen-space y-coordinate. "},
	{L"degrees",     L"degrees(x)",                L"Converts x from radians to degrees. "},
	{L"determinant", L"determinant(m)",            L"Returns the determinant of the square matrix m. "},
	{L"distance",    L"distance(a, b)",            L"Returns the distance between two points a and b. "},
	{L"dot",         L"dot(a, b)",                 L"Returns the dot product of two vectors a and b. "},
	{L"exp",         L"exp(x)",                    L"Returns the base-e exponent ex. "},
	{L"exp2",        L"exp2(value a)",             L"Base 2 Exp (per component). "},
	{L"faceforward", L"faceforward(n, i, ng)",     L"Returns -n * sign(dot(i, ng)). "},
	{L"floor",       L"floor(x)",                  L"Returns the greatest integer which is less than or equal to x. "},
	{L"fmod",        L"fmod(a, b)",                L"Returns the floating point remainder f of a / b such that a = i * b + f, where i is an integer, f has the same sign as x, and the absolute value of f is less than the absolute value of b. "},
	{L"frac",        L"frac(x)",                   L"Returns the fractional part f of x, such that f is a value greater than or equal to 0, and less than 1. "},
	{L"frc",         L"frc(value a)",              L"Fractional part (per component). "},
	{L"frexp",       L"frexp(x, out exp)",         L"Returns the mantissa and exponent of x. frexp returns the mantissa, and the exponent is stored in the output parameter exp. If x is 0, the function returns 0 for both the mantissa and the exponent. "},
	{L"fwidth",      L"fwidth(x)",                 L"Returns abs(ddx(x))+abs(ddy(x)). "},
	{L"isfinite",    L"isfinite(x)",               L"Returns true if x is finite, false otherwise. "},
	{L"isinf",       L"isinf(x)",                  L"Returns true if x is +INF or -INF, false otherwise. "},
	{L"isnan",       L"isnan(x)",                  L"Returns true if x is NAN or QNAN, false otherwise. "},
	{L"ldexp",       L"ldexp(x, exp)",             L"Returns x * 2exp. "},
	{L"len",         L"len(value a)",              L"Vector length. "},
	{L"length",      L"length(v)",                 L"Returns the length of the vector v. "},
	{L"lerp",        L"lerp(a, b, s)",             L"Returns a + s(b - a). This linearly interpolates between a and b, such that the return value is a when s is 0, and b when s is 1. "},
	{L"lit",         L"lit(ndotl, ndoth, m)",      L"Returns a lighting vector (ambient, diffuse, specular, 1): ambient = 1; diffuse = (ndotl < 0) ? 0 : ndotl; specular = (ndotl < 0) || (ndoth < 0) ? 0 : (ndoth * m); "},
	{L"log",         L"log(x)",                    L"Returns the base-e logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	{L"log10",       L"log10(x)",                  L"Returns the base-10 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	{L"log2",        L"log2(x)",                   L"Returns the base-2 logarithm of x. If x is negative, the function returns indefinite. If x is 0, the function returns +INF. "},
	{L"max",         L"max(a, b)",                 L"Selects the greater of a and b. "},
	{L"min",         L"min(a, b)",                 L"Selects the lesser of a and b. "},
	{L"modf",        L"modf(x, out ip)",           L"Splits the value x into fractional and integer parts, each of which has the same sign and x. The signed fractional portion of x is returned. The integer portion is stored in the output parameter ip. "},
	{L"mul",         L"mul(a, b)",                 L"Performs matrix multiplication between a and b. If a is a vector, it treated as a row vector. If b is a vector, it is treated as a column vector. The inner dimension acolumns and brows must be equal. The result has the dimension arows x bcolumns. "},
	{L"noise",       L"noise(x)",                  L"Not yet implemented. "},
	{L"normalize",   L"normalize(v)",              L"Returns the normalized vector v / length(v). If the length of v is 0, the result is indefinite. "},
	{L"pow",         L"pow(x, y)",                 L"Returns x^y. "},
	{L"radians",     L"radians(x)",                L"Converts x from degrees to radians. "},
	{L"reflect",     L"reflect(i, n)",             L"Returns the reflection vector v, given the entering ray direction i, and the surface normal n. Such that v = i - 2 * dot(i, n) * n "},
	{L"refract",     L"refract(i, n, eta)",        L"Returns the refraction vector v, given the entering ray direction i, the surface normal n, and the relative index of refraction eta. If the angle between i and n is too great for a given eta, refract returns (0,0,0). "},
	{L"round",       L"round(x)",                  L"Rounds x to the nearest integer. "},
	{L"rsqrt",       L"rsqrt(x)",                  L"Returns 1 / sqrt(x). "},
	{L"saturate",    L"saturate(x)",               L"Clamps x to the range [0, 1]. "},
	{L"sign",        L"sign(x)",                   L"Computes the sign of x. Returns -1 if x is less than 0, 0 if x equals 0, and 1 if x is greater than zero. "},
	{L"sin",         L"sin(x)",                    L"Returns the sine of x. "},
	{L"sincos",      L"sincos(x, out s, out c)",   L"Returns the sine and cosine of x. sin(x) is stored in the output parameter s. cos(x) is stored in the output parameter c. "},
	{L"sinh",        L"sinh(x)",                   L"Returns the hyperbolic sine of x. "},
	{L"smoothstep",  L"smoothstep(min, max, x)",   L"Returns 0 if x < min. Returns 1 if x > max. Returns a smooth Hermite interpolation between 0 and 1, if x is in the range [min, max]. "},
	{L"sqrt",        L"sqrt(value a)",             L"Square root (per component). "},
	{L"step",        L"step(a, x)",                L"Returns (x >= a) ? 1 : 0. "},
	{L"tan",         L"tan(x)",                    L"Returns the tangent of x. "},
	{L"tanh",        L"tanh(x)",                   L"Returns the hyperbolic tangent of x. "},
	{L"tex1d",       L"tex1D(s, t)",               L"1D texture lookup. s is a sampler or a sampler1D object. t is a scalar. "},
	{L"tex1d(",      L"tex1D(s, t, ddx, ddy)",     L"1D texture lookup, with derivatives. s is a sampler or sampler1D object. t, ddx, and ddy are scalars. "},
	{L"tex1dbias",   L"tex1Dbias(s, t)",           L"1D biased texture lookup. s is a sampler or sampler1D object. t is a 4D vector. The mip level is biased by t.w before the lookup takes place. "},
	{L"tex1dgrad",   L"tex1Dgrad(s, t, ddx, ddy)", L"1D texture lookup with a gradient. "},
	{L"tex1dlod",    L"tex1Dlod(s, t)",            L"1D texture lookup with LOD. "},
	{L"tex1dproj",   L"tex1Dproj(s, t)",           L"1D projective texture lookup. s is a sampler or sampler1D object. t is a 4D vector. t is divided by its last component before the lookup takes place. "},
	{L"tex2d",       L"tex2D(s, t)",               L"2D texture lookup. s is a sampler or a sampler2D object. t is a 2D texture coordinate. "},
	{L"tex2d(",      L"tex2D(s, t, ddx, ddy)",     L"2D texture lookup, with derivatives. s is a sampler or sampler2D object. t, ddx, and ddy are 2D vectors. "},
	{L"tex2dbias",   L"tex2Dbias(s, t)",           L"2D biased texture lookup. s is a sampler or sampler2D object. t is a 4D vector. The mip level is biased by t.w before the lookup takes place. "},
	{L"tex2dgrad",   L"tex2Dgrad(s, t, ddx, ddy)", L"2D texture lookup with a gradient. "},
	{L"tex2dlod",    L"tex2Dlod(s, t)",            L"2D texture lookup with LOD. "},
	{L"tex2dproj",   L"tex2Dproj(s, t)",           L"2D projective texture lookup. s is a sampler or sampler2D object. t is a 4D vector. t is divided by its last component before the lookup takes place. "},
	{L"tex3d",       L"tex3D(s, t)",               L"3D volume texture lookup. s is a sampler or a sampler3D object. t is a 3D texture coordinate. "},
	{L"tex3d(",      L"tex3D(s, t, ddx, ddy)",     L"3D volume texture lookup, with derivatives. s is a sampler or sampler3D object. t, ddx, and ddy are 3D vectors. "},
	{L"tex3dbias",   L"tex3Dbias(s, t)",           L"3D biased texture lookup. s is a sampler or sampler3D object. t is a 4D vector. The mip level is biased by t.w before the lookup takes place. "},
	{L"tex3dgrad",   L"tex3Dgrad(s, t, ddx, ddy)", L"3D texture lookup with a gradient. "},
	{L"tex3dlod",    L"tex3Dlod(s, t)",            L"3D texture lookup with LOD. "},
	{L"tex3dproj",   L"tex3Dproj(s, t)",           L"3D projective volume texture lookup. s is a sampler or sampler3D object. t is a 4D vector. t is divided by its last component before the lookup takes place. "},
	{L"texcube",     L"texCUBE(s, t)",             L"3D cube texture lookup. s is a sampler or a samplerCUBE object. t is a 3D texture coordinate. "},
	{L"texcube(",    L"texCUBE(s, t, ddx, ddy)",   L"3D cube texture lookup, with derivatives. s is a sampler or samplerCUBE object. t, ddx, and ddy are 3D vectors. "},
	{L"texcubebias", L"texCUBEbias(s, t)",         L"3D biased cube texture lookup. s is a sampler or samplerCUBE object. t is a 4-dimensional vector. The mip level is biased by t.w before the lookup takes place.  "},
	{L"texcubegrad", L"texCUBEgrad(s, t, ddx, ddy)", L"Cube texture lookup with a gradient. "},
	{L"texcubelod",  L"texCUBElod(s, t)",          L"Cube texture lookup with LOD. "},
	{L"texcubeproj", L"texCUBEproj(s, t)",         L"3D projective cube texture lookup. s is a sampler or samplerCUBE object. t is a 4D vector. t is divided by its last component before the lookup takes place. "},
	{L"transpose",   L"transpose(m)",              L"Returns the transpose of the matrix m. If the source is dimension mrows x mcolumns, the result is dimension mcolumns x mrows. "},
	{L"trunc",       L"trunc(x)",                  L"Truncates floating-point value(s) to integer value(s) "},
	// ps_4_0
	{L"abort",   L"abort()",     L"Terminates the current draw or dispatch call being executed. "},
	{L"asfloat", L"asfloat(x)",  L"Convert the input type to a float. "},
	{L"asint",   L"asint(x)",    L"Convert the input type to an integer. "},
	{L"asuint",  L"asuint(x)",   L"Convert the input type to an unsigned integer. "},
};

//
// CShaderEdit
//

CShaderEdit::CShaderEdit()
{
	m_acdlg.Create(CShaderAutoCompleteDlg::IDD, nullptr);

	m_nEndChar = -1;
	m_nIDEvent = (UINT_PTR)-1;
}

CShaderEdit::~CShaderEdit()
{
	m_acdlg.DestroyWindow();
}

BOOL CShaderEdit::PreTranslateMessage(MSG* pMsg)
{
	if (m_acdlg.IsWindowVisible()
			&& pMsg->message == WM_KEYDOWN
			&& (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN
				|| pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT
				|| pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)) {
		int i = m_acdlg.m_list.GetCurSel();

		if (pMsg->wParam == VK_RETURN && i >= 0) {
			CString str;
			m_acdlg.m_list.GetText(i, str);
			i = str.Find('(')+1;
			if (i > 0) {
				str = str.Left(i);
			}

			int nStartChar = 0, nEndChar = -1;
			GetSel(nStartChar, nEndChar);

			CString text;
			GetWindowTextW(text);
			while (nStartChar > 0 && _istalnum(text.GetAt(nStartChar-1))) {
				nStartChar--;
			}

			SetSel(nStartChar, nEndChar);
			ReplaceSel(str, TRUE);
		}
		else if (pMsg->wParam == VK_ESCAPE) {
			m_acdlg.ShowWindow(SW_HIDE);
			return GetParent()->PreTranslateMessage(pMsg);
		}
		else {
			m_acdlg.m_list.SendMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
		}

		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == 'A' || pMsg->wParam == 'a') && GetKeyState(VK_CONTROL) < 0) {
		SetSel(0, -1, TRUE);
		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CShaderEdit, CLineNumberEdit)
	ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
	ON_WM_KILLFOCUS()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CShaderEdit::OnUpdate()
{
	if (m_nIDEvent == (UINT_PTR)-1) {
		m_nIDEvent = SetTimer(1, 100, nullptr);
	}

	CString text;
	int nStartChar = 0, nEndChar = -1;
	GetSel(nStartChar, nEndChar);

	if (nStartChar == nEndChar) {
		GetWindowTextW(text);
		while (nStartChar > 0 && _istalnum(text.GetAt(nStartChar-1))) {
			nStartChar--;
		}
	}

	if (nStartChar < nEndChar) {
		text = text.Mid(nStartChar, nEndChar - nStartChar);
		text.TrimRight('(');
		text.MakeLower();

		m_acdlg.m_list.ResetContent();

		for (auto& hlslfunc : s_HLSLFuncs) {
			if (wcsncmp(hlslfunc.str, text, text.GetLength()) == 0) {
				int i = m_acdlg.m_list.AddString(hlslfunc.name);
				m_acdlg.m_list.SetItemDataPtr(i, (void*)&hlslfunc);
			}
		}

		if (m_acdlg.m_list.GetCount() > 0) {
			int lineheight = GetLineHeight();

			CPoint p = PosFromChar(nStartChar);
			p.y += lineheight;
			ClientToScreen(&p);
			CRect r(p, CSize(100, 100));

			m_acdlg.MoveWindow(r);
			m_acdlg.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			m_acdlg.ShowWindow(SW_SHOWNOACTIVATE);

			m_nEndChar = nEndChar;

			return;
		}
	}

	m_acdlg.ShowWindow(SW_HIDE);
}

void CShaderEdit::OnKillFocus(CWnd* pNewWnd)
{
	CString text;
	GetWindowTextW(text);
	__super::OnKillFocus(pNewWnd);
	GetWindowTextW(text);

	m_acdlg.ShowWindow(SW_HIDE);
}

void CShaderEdit::OnTimer(UINT_PTR nIDEvent)
{
	if (m_nIDEvent == nIDEvent) {
		int nStartChar = 0, nEndChar = -1;
		GetSel(nStartChar, nEndChar);

		if (nStartChar != nEndChar || m_nEndChar != nEndChar) {
			m_acdlg.ShowWindow(SW_HIDE);
		}
	}

	__super::OnTimer(nIDEvent);
}

// CShaderEditorDlg dialog

CShaderEditorDlg::CShaderEditorDlg()
	: CResizableDialog(CShaderEditorDlg::IDD, nullptr)
{
}

CShaderEditorDlg::~CShaderEditorDlg()
{
	m_Font.DeleteObject();
	delete m_pPSC;
}

BOOL CShaderEditorDlg::Create(CWnd* pParent)
{
	if (!__super::Create(IDD, pParent)) {
		return FALSE;
	}

	LOGFONTW lf = {};
	lf.lfHeight = -AfxGetMainFrame()->ScaleY(12);
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

	for (const auto &fontname : MonospaceFonts) {
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, fontname);
		if (IsFontInstalled(fontname) && m_Font.CreateFontIndirectW(&lf)) {
			m_edSrcdata.SetFont(&m_Font);
			break;
		}
	}

	AddAnchor(IDC_COMBO3, TOP_LEFT);
	AddAnchor(IDC_COMBO1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBO2, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT2, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, TOP_RIGHT);
	AddAnchor(IDC_BUTTON2, TOP_RIGHT);
	AddAnchor(IDC_BUTTON4, BOTTOM_RIGHT);

	m_edSrcdata.SetTabStops(16);

	SetMinTrackSize(CSize(250, 40));

	m_cbDXNum.AddString(L"DX9");
	m_cbDXNum.AddString(L"DX11");

	m_cbProfile.AddString(L"ps_3_0");
	m_cbProfile.AddString(L"ps_4_0");

	m_bD3D11 = (AfxGetAppSettings().m_VRSettings.iVideoRenderer == VIDRNDT_MPCVR && IsWindows8OrGreater());

	RefreshDarkTheme();

	return TRUE;
}

void CShaderEditorDlg::RefreshDarkTheme()
{
	DarkTheme::RefreshDialog(GetSafeHwnd());
	// The source editor's line-number margin is a self-drawn CStatic that paints its own light
	// background (RGB(200,200,200)); recolour it so the left gutter isn't a bright strip, and put
	// the CLineNumberEdit defaults back when the option is turned off again.
	if (DarkTheme::IsActive()) {
		m_edSrcdata.SetMarginBackgroundColor(DarkTheme::FaceColor(), TRUE);
		m_edSrcdata.SetMarginForegroundColor(RGB(120, 125, 130), TRUE);
	} else {
		m_edSrcdata.SetMarginBackgroundColor(RGB(200, 200, 200), TRUE);
		m_edSrcdata.SetMarginForegroundColor(RGB(0, 0, 0), TRUE);
	}
}

void CShaderEditorDlg::UpdateShaderList()
{
	m_cbProfile.EnableWindow(FALSE);
	if (m_bD3D11) {
		m_cbDXNum.SetCurSel(1);
		m_cbProfile.SetCurSel(1);
	} else {
		m_cbDXNum.SetCurSel(0);
		m_cbProfile.SetCurSel(0);
	}

	m_cbLabels.ResetContent();
	m_edSrcdata.SetWindowTextW(L"");
	m_edOutput.SetWindowTextW(L"");

	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		if (m_bD3D11) {
			path += L"Shaders11\\";
		} else {
			path += L"Shaders\\";
		}
		if (::PathFileExistsW(path)) {
			WIN32_FIND_DATAW wfd;
			HANDLE hFile = FindFirstFileW(path + L"*.hlsl", &wfd);
			if (hFile != INVALID_HANDLE_VALUE) {
				do {
					CString filename(wfd.cFileName);
					filename.Truncate(filename.GetLength() - 5);
					m_cbLabels.AddString(filename);
				} while (FindNextFileW(hFile, &wfd));
				FindClose(hFile);
			}
		}
	}
	CorrectComboListWidth(m_cbLabels);
}

void CShaderEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO3, m_cbDXNum);
	DDX_Control(pDX, IDC_COMBO1, m_cbLabels);
	DDX_Control(pDX, IDC_COMBO2, m_cbProfile);
	DDX_Control(pDX, IDC_EDIT1, m_edSrcdata);
	DDX_Control(pDX, IDC_EDIT2, m_edOutput);
}

bool CShaderEditorDlg::HitTestSplitter(CPoint p)
{
	CRect r, rs, ro;
	m_edSrcdata.GetWindowRect(&rs);
	m_edOutput.GetWindowRect(&ro);
	ScreenToClient(&rs);
	ScreenToClient(&ro);
	GetClientRect(&r);
	r.left = ro.left;
	r.right = ro.right;
	r.top = rs.bottom;
	r.bottom = ro.top;
	return !!r.PtInRect(p);
}


void CShaderEditorDlg::NewShader()
{
	CShaderNewDlg dlg;
	if (IDOK != dlg.DoModal()) {
		return;
	}

	// if shader already exists, then select it
	int i = m_cbLabels.SelectString(0, dlg.m_Name);
	if (i > 0) {
		return;
	}

	CStringA srcdata;
	if (!LoadResource(m_bD3D11 ? IDF_SHADER11_EMPTY : IDF_SHADER_EMPTY, srcdata, L"FILE")) {
		return;
	}

	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		if (m_bD3D11) {
			path.AppendFormat(L"Shaders11\\%s.hlsl", dlg.m_Name);
		} else {
			path.AppendFormat(L"Shaders\\%s.hlsl", dlg.m_Name);
		}

		CStdioFile file;
		if (file.Open(path, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary)) {
			file.Write(srcdata.GetBuffer(), srcdata.GetLength());
			file.Close();

			m_cbLabels.AddString(dlg.m_Name);
			m_cbLabels.SelectString(0, dlg.m_Name);
			OnCbnSelchangeCombo1();
		}
	}
}

void CShaderEditorDlg::DeleteShader()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		if (IDYES != AfxMessageBox(ResStr(IDS_SHADEREDITORDLG_0), MB_YESNO)) {
			return;
		}

		CString label;
		m_cbLabels.GetLBText(i, label);

		auto pFrame = AfxGetMainFrame();

		if (pFrame->DeleteShaderFile(label, m_bD3D11)) {
			m_cbLabels.DeleteString(i);
			m_cbLabels.SetCurSel(-1);

			m_edSrcdata.SetWindowTextW(L"");
			m_edOutput.SetWindowTextW(L"");

			pFrame->TidyShaderCashe();
			pFrame->SetShaders(); // reset shaders
		}
	}
}

BEGIN_MESSAGE_MAP(CShaderEditorDlg, CResizableDialog)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedSave)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedMenu)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedApply)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CShaderEditorDlg message handlers

BOOL CShaderEditorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB
			   && pMsg->hwnd == m_edSrcdata.GetSafeHwnd()) {
		int nStartChar, nEndChar;
		m_edSrcdata.GetSel(nStartChar, nEndChar);
		if (nStartChar == nEndChar) {
			m_edSrcdata.ReplaceSel(L"\t");
		}
		return TRUE;
	} else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
		return GetParent()->PreTranslateMessage(pMsg);
	}

	return __super::PreTranslateMessage(pMsg);
}

void CShaderEditorDlg::OnCbnSelchangeCombo3()
{
	bool bD3D11 = (m_cbDXNum.GetCurSel() == 1);

	if (m_bD3D11 != bD3D11) {
		m_bD3D11 = bD3D11;
		UpdateShaderList();
	}
}

void CShaderEditorDlg::OnCbnSelchangeCombo1()
{
	auto pFrame = AfxGetMainFrame();
	pFrame->SetShaders(); // reset shaders

	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		CString label;
		m_cbLabels.GetLBText(i, label);

		ShaderC* pShader = pFrame->GetShader(label, m_bD3D11);

		m_cbProfile.SelectString(0, pShader->profile);

		CString srcdata(pShader->srcdata);
		srcdata.Replace(L"\n", L"\r\n");
		m_edSrcdata.SetWindowTextW(srcdata);

		m_edOutput.SetWindowTextW(L"");
	}
}

void CShaderEditorDlg::OnBnClickedSave()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		ShaderC shader;

		m_cbLabels.GetLBText(i, shader.label);
		m_cbProfile.GetLBText(m_cbProfile.GetCurSel(), shader.profile);

		m_edSrcdata.GetWindowTextW(shader.srcdata);
		shader.srcdata.Remove('\r');

		bool ret = AfxGetMainFrame()->SaveShaderFile(&shader, m_bD3D11);
	}
}

void CShaderEditorDlg::OnBnClickedMenu()
{
	int i = m_cbLabels.GetCurSel();

	enum {
		M_SAVE = 1,
		M_NEW,
		M_DELETE,
		M_FOLDER
	};

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenuW(MF_STRING | (i >= 0 ? MF_ENABLED : MF_GRAYED), M_SAVE, ResStr(IDS_SHADER_SAVE));
	menu.AppendMenuW(MF_SEPARATOR);
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_NEW, ResStr(IDS_SHADER_NEW));
	menu.AppendMenuW(MF_STRING | (i >= 0 ? MF_ENABLED : MF_GRAYED), M_DELETE, ResStr(IDS_SHADER_DELETE));
	menu.AppendMenuW(MF_SEPARATOR);
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_FOLDER, ResStr(IDS_SHADER_FOLDER));

	CRect wrect;
	GetDlgItem(IDC_BUTTON1)->GetWindowRect(&wrect);

	int id = menu.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, wrect.left, wrect.bottom, this);
	switch (id) {
	case M_SAVE:
		OnBnClickedSave();
		break;
	case M_NEW:
		NewShader();
		break;
	case M_DELETE:
		DeleteShader();
		break;
	case M_FOLDER:
		{
			CString path;
			if (AfxGetMyApp()->GetAppSavePath(path)) {
				if (m_bD3D11) {
					path.Append(L"Shaders11");
				} else {
					path.Append(L"Shaders");
				}
				ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_RESTORE);
			}
		}
		break;
	}
}

void CShaderEditorDlg::OnBnClickedApply()
{
	auto pFrame = AfxGetMainFrame();
	pFrame->SetShaders(); // reset shaders

	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		CString label;
		m_cbLabels.GetLBText(i, label);
		CString str;
		m_cbProfile.GetLBText(m_cbProfile.GetCurSel(), str);
		CStringA profile(str);

		m_edSrcdata.GetWindowTextW(str);
		str.Remove('\r');
		CStringA srcdata(str);

		if (srcdata.GetLength() && profile.GetLength()) {
			if (!m_pPSC) {
				m_pPSC = DNew CPixelShaderCompiler(nullptr);
			}

			CString disasm, errmsg;
			HRESULT hr = m_pPSC->CompileShader(srcdata, "main", profile, D3DCOMPILE_DEBUG, nullptr, nullptr, &errmsg, &disasm);

			if (SUCCEEDED(hr)) {
				errmsg = L"D3DXCompileShader succeeded\n";
				errmsg += L"\n";
				errmsg += disasm;

				if (pFrame->m_pCAP) {
					hr = pFrame->m_pCAP->AddPixelShader(TARGET_SCREEN, label, profile, srcdata); // and add shader to the end of list
				}
			}

			errmsg.Replace(L"\n", L"\r\n");
			m_edOutput.SetWindowTextW(errmsg);
		}
	}
}

void CShaderEditorDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (HitTestSplitter(point)) {
		m_fSplitterGrabbed = true;
		SetCapture();
	}

	__super::OnLButtonDown(nFlags, point);
}

void CShaderEditorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_fSplitterGrabbed) {
		ReleaseCapture();
		m_fSplitterGrabbed = false;
	}

	__super::OnLButtonUp(nFlags, point);
}

void CShaderEditorDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_fSplitterGrabbed) {
		CRect r, rs, ro;
		GetClientRect(&r);
		m_edSrcdata.GetWindowRect(&rs);
		m_edOutput.GetWindowRect(&ro);
		ScreenToClient(&rs);
		ScreenToClient(&ro);

		int dist = ro.top - rs.bottom;
		int avgdist = dist / 2;

		auto pFrame = AfxGetMainFrame();

		rs.bottom = std::min(std::max(point.y, rs.top + pFrame->ScaleY(40)), ro.bottom - pFrame->ScaleY(48)) - avgdist;
		ro.top = rs.bottom + dist;
		m_edSrcdata.MoveWindow(&rs);
		m_edOutput.MoveWindow(&ro);

		int div = 100 * ((rs.bottom + ro.top) / 2) / (ro.bottom - rs.top);

		RemoveAnchor(IDC_EDIT1);
		RemoveAnchor(IDC_EDIT2);
		AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
		AddAnchor(IDC_EDIT2, BOTTOM_LEFT, BOTTOM_RIGHT);
	}

	__super::OnMouseMove(nFlags, point);
}

BOOL CShaderEditorDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);

	if (HitTestSplitter(p)) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return TRUE;
	}

	return __super::OnSetCursor(pWnd, nHitTest, message);
}
