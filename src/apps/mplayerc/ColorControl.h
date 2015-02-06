/*
 * (C) 2014-2015 see Authors.txt
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

#include <d3d9.h>
#include <vmr9.h>
#include <dxva2api.h> //#include <evr9.h>

enum ControlType {
	ProcAmp_Brightness	= 0x1,
	ProcAmp_Contrast	= 0x2,
	ProcAmp_Hue			= 0x4,
	ProcAmp_Saturation	= 0x8,
	ProcAmp_All = ProcAmp_Brightness | ProcAmp_Contrast | ProcAmp_Hue | ProcAmp_Saturation,
};

struct COLORPROPERTY_RANGE {
	DWORD	dwProperty;
	int		MinValue;
	int		MaxValue;
	int		DefaultValue;
	int		StepSize;
};

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

class CColorControl
{
	COLORPROPERTY_RANGE		m_ColorBri;
	COLORPROPERTY_RANGE		m_ColorCon;
	COLORPROPERTY_RANGE		m_ColorHue;
	COLORPROPERTY_RANGE		m_ColorSat;

	VMR9ProcAmpControlRange	m_VMR9ColorBri;
	VMR9ProcAmpControlRange	m_VMR9ColorCon;
	VMR9ProcAmpControlRange	m_VMR9ColorHue;
	VMR9ProcAmpControlRange	m_VMR9ColorSat;

	DXVA2_ValueRange		m_EVRColorBri;
	DXVA2_ValueRange		m_EVRColorCon;
	DXVA2_ValueRange		m_EVRColorHue;
	DXVA2_ValueRange		m_EVRColorSat;

public:
	CColorControl();

	COLORPROPERTY_RANGE*		GetColorControl(ControlType nFlag);
	void						ResetColorControlRange();
	void						UpdateColorControlRange(bool isEVR);
	VMR9ProcAmpControlRange*	GetVMR9ColorControl(ControlType nFlag);
	DXVA2_ValueRange*			GetEVRColorControl(ControlType nFlag);
};
