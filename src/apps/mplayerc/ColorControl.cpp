/*
 * (C) 2014-2017 see Authors.txt
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
#include "ColorControl.h"

__inline DXVA2_Fixed32 IntToFixed(__in const int _int_, __in const SHORT divisor = 1)
{
	DXVA2_Fixed32 _fixed_;
	_fixed_.Value = _int_ / divisor;
	_fixed_.Fraction = (_int_ % divisor * 0x10000 + divisor/2) / divisor;
	return _fixed_;
}

__inline int FixedToInt(__in const DXVA2_Fixed32 _fixed_, __in const SHORT factor = 1)
{
	return (int)_fixed_.Value * factor + ((int)_fixed_.Fraction * factor + 0x8000) / 0x10000;
}

// CColorControl

CColorControl::CColorControl()
	: m_VMR9ColorBri({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Brightness, -100, 100, 0, 1    })
	, m_VMR9ColorCon({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Contrast,      0,   2, 1, 0.01f})
	, m_VMR9ColorHue({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Hue,        -180, 180, 0, 1    })
	, m_VMR9ColorSat({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Saturation,    0,   2, 1, 0.01f})
	, m_EVRColorBri({DXVA2FloatToFixed(-100), DXVA2FloatToFixed(100), DXVA2FloatToFixed(0), DXVA2FloatToFixed(1    )})
	, m_EVRColorCon({DXVA2FloatToFixed(   0), DXVA2FloatToFixed(  2), DXVA2FloatToFixed(1), DXVA2FloatToFixed(0.01f)})
	, m_EVRColorHue({DXVA2FloatToFixed(-180), DXVA2FloatToFixed(180), DXVA2FloatToFixed(0), DXVA2FloatToFixed(1    )})
	, m_EVRColorSat({DXVA2FloatToFixed(   0), DXVA2FloatToFixed(  2), DXVA2FloatToFixed(1), DXVA2FloatToFixed(0.01f)})
	, m_VMR9Used(false)
{
}

VMR9ProcAmpControlRange* CColorControl::GetVMR9ColorControl(ControlType nFlag)
{
	switch (nFlag) {
	case ProcAmp_Brightness:
		return &m_VMR9ColorBri;
	case ProcAmp_Contrast:
		return &m_VMR9ColorCon;
	case ProcAmp_Hue:
		return &m_VMR9ColorHue;
	case ProcAmp_Saturation:
		return &m_VMR9ColorSat;
	}
	return nullptr;
}

DXVA2_ValueRange* CColorControl::GetEVRColorControl(ControlType nFlag)
{
	switch (nFlag) {
	case ProcAmp_Brightness:
		return &m_EVRColorBri;
	case ProcAmp_Contrast:
		return &m_EVRColorCon;
	case ProcAmp_Hue:
		return &m_EVRColorHue;
	case ProcAmp_Saturation:
		return &m_EVRColorSat;
	}
	return nullptr;
}

void CColorControl::EnableVMR9ColorControl()
{
	// fix nvidia min contrast bug
	if (*(int*)&m_VMR9ColorCon.MinValue == 1036830720) {
		m_VMR9ColorCon.MinValue = 0.11f;
	}

	m_VMR9Used = true;
}

void CColorControl::EnableEVRColorControl()
{
	m_VMR9Used = false;
}

VMR9ProcAmpControl CColorControl::GetVMR9ProcAmpControl(DWORD flags, int brightness, int contrast, int hue, int saturation)
{
	VMR9ProcAmpControl procAmpControl;
	procAmpControl.dwSize     = sizeof(VMR9ProcAmpControl);
	procAmpControl.dwFlags    = flags;
	procAmpControl.Brightness = clamp((float)brightness,               m_VMR9ColorBri.MinValue, m_VMR9ColorBri.MaxValue);
	procAmpControl.Contrast   = clamp((float)(contrast + 100) / 100,   m_VMR9ColorCon.MinValue, m_VMR9ColorCon.MaxValue);
	procAmpControl.Hue        = clamp((float)hue,                      m_VMR9ColorHue.MinValue, m_VMR9ColorHue.MaxValue);
	procAmpControl.Saturation = clamp((float)(saturation + 100) / 100, m_VMR9ColorSat.MinValue, m_VMR9ColorSat.MaxValue);

	return procAmpControl;
}

DXVA2_ProcAmpValues CColorControl::GetEVRProcAmpValues(int brightness, int contrast, int hue, int saturation)
{
	DXVA2_ProcAmpValues procAmpValues;
	procAmpValues.Brightness.ll = clamp(IntToFixed(brightness).ll,            m_EVRColorBri.MinValue.ll, m_EVRColorBri.MaxValue.ll);
	procAmpValues.Contrast.ll   = clamp(IntToFixed(contrast + 100, 100).ll,   m_EVRColorCon.MinValue.ll, m_EVRColorCon.MaxValue.ll);
	procAmpValues.Hue.ll        = clamp(IntToFixed(hue).ll,                   m_EVRColorHue.MinValue.ll, m_EVRColorHue.MaxValue.ll);
	procAmpValues.Saturation.ll = clamp(IntToFixed(saturation + 100, 100).ll, m_EVRColorSat.MinValue.ll, m_EVRColorSat.MaxValue.ll);

	return procAmpValues;
}

void CColorControl::GetDefaultValues(int& brightness, int& contrast, int& hue, int& saturation)
{
	if (m_VMR9Used) {
		brightness = (int)floor(m_VMR9ColorBri.DefaultValue + 0.5);
		contrast   = (int)floor(m_VMR9ColorCon.DefaultValue * 100 + 0.5) - 100;
		hue		   = (int)floor(m_VMR9ColorHue.DefaultValue + 0.5);
		saturation = (int)floor(m_VMR9ColorSat.DefaultValue * 100 + 0.5) - 100;

	}
	else {
		brightness = FixedToInt(m_EVRColorBri.DefaultValue);
		contrast   = FixedToInt(m_EVRColorCon.DefaultValue, 100) - 100;
		hue		   = FixedToInt(m_EVRColorHue.DefaultValue);
		saturation = FixedToInt(m_EVRColorSat.DefaultValue, 100) - 100;
	}
}
