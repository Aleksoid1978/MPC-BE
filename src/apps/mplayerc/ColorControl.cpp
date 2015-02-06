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

#include "stdafx.h"
#include "ColorControl.h"

// CColorControl

CColorControl::CColorControl()
	: m_VMR9ColorBri({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Brightness, 0, 0, 0, 0})
	, m_VMR9ColorCon({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Contrast, 0, 0, 0, 0})
	, m_VMR9ColorHue({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Hue, 0, 0, 0, 0})
	, m_VMR9ColorSat({sizeof(VMR9ProcAmpControlRange), ProcAmpControl9_Saturation, 0, 0, 0, 0})
{
	ResetColorControlRange();

	memset(&m_EVRColorBri, 0, sizeof(DXVA2_ValueRange));
	memset(&m_EVRColorCon, 0, sizeof(DXVA2_ValueRange));
	memset(&m_EVRColorHue, 0, sizeof(DXVA2_ValueRange));
	memset(&m_EVRColorSat, 0, sizeof(DXVA2_ValueRange));
}

COLORPROPERTY_RANGE* CColorControl::GetColorControl(ControlType nFlag)
{
	switch (nFlag) {
	case ProcAmp_Brightness:
		return &m_ColorBri;
	case ProcAmp_Contrast:
		return &m_ColorCon;
	case ProcAmp_Hue:
		return &m_ColorHue;
	case ProcAmp_Saturation:
		return &m_ColorSat;
	}

	return NULL;
}

void CColorControl::ResetColorControlRange()
{
	m_ColorBri.dwProperty	= ProcAmp_Brightness;
	m_ColorBri.MinValue		= -100;
	m_ColorBri.MaxValue		= 100;
	m_ColorBri.DefaultValue	= 0;
	m_ColorBri.StepSize		= 1;

	m_ColorCon.dwProperty	= ProcAmp_Contrast;
	m_ColorCon.MinValue		= -100;
	m_ColorCon.MaxValue		= 100;
	m_ColorCon.DefaultValue	= 0;
	m_ColorCon.StepSize		= 1;

	m_ColorHue.dwProperty	= ProcAmp_Hue;
	m_ColorHue.MinValue		= -180;
	m_ColorHue.MaxValue		= 180;
	m_ColorHue.DefaultValue	= 0;
	m_ColorHue.StepSize		= 1;

	m_ColorSat.dwProperty	= ProcAmp_Saturation;
	m_ColorSat.MinValue		= -100;
	m_ColorSat.MaxValue		= 100;
	m_ColorSat.DefaultValue	= 0;
	m_ColorSat.StepSize		= 1;
}

void CColorControl::UpdateColorControlRange(bool isEVR)
{
	if (isEVR) {
		// Brightness
		m_ColorBri.MinValue		= FixedToInt(m_EVRColorBri.MinValue);
		m_ColorBri.MaxValue		= FixedToInt(m_EVRColorBri.MaxValue);
		m_ColorBri.DefaultValue	= FixedToInt(m_EVRColorBri.DefaultValue);
		m_ColorBri.StepSize		= max(1, FixedToInt(m_EVRColorBri.StepSize));
		// Contrast
		m_ColorCon.MinValue		= FixedToInt(m_EVRColorCon.MinValue, 100) - 100;
		m_ColorCon.MaxValue		= FixedToInt(m_EVRColorCon.MaxValue, 100) - 100;
		m_ColorCon.DefaultValue	= FixedToInt(m_EVRColorCon.DefaultValue, 100) - 100;
		m_ColorCon.StepSize		= max(1, FixedToInt(m_EVRColorCon.StepSize, 100));
		// Hue
		m_ColorHue.MinValue		= FixedToInt(m_EVRColorHue.MinValue);
		m_ColorHue.MaxValue		= FixedToInt(m_EVRColorHue.MaxValue);
		m_ColorHue.DefaultValue	= FixedToInt(m_EVRColorHue.DefaultValue);
		m_ColorHue.StepSize		= max(1, FixedToInt(m_EVRColorHue.StepSize));
		// Saturation
		m_ColorSat.MinValue		= FixedToInt(m_EVRColorSat.MinValue, 100) - 100;
		m_ColorSat.MaxValue		= FixedToInt(m_EVRColorSat.MaxValue, 100) - 100;
		m_ColorSat.DefaultValue	= FixedToInt(m_EVRColorSat.DefaultValue, 100) - 100;
		m_ColorSat.StepSize		= max(1, FixedToInt(m_EVRColorSat.StepSize, 100));
	} else {
		// Brightness
		m_ColorBri.MinValue		= (int)floor(m_VMR9ColorBri.MinValue + 0.5);
		m_ColorBri.MaxValue		= (int)floor(m_VMR9ColorBri.MaxValue + 0.5);
		m_ColorBri.DefaultValue	= (int)floor(m_VMR9ColorBri.DefaultValue + 0.5);
		m_ColorBri.StepSize		= max(1,(int)(m_VMR9ColorBri.StepSize + 0.5));
		// Contrast
		//if(m_VMR9ColorCon.MinValue == 0.0999908447265625) m_VMR9ColorCon.MinValue = 0.11; //fix nvidia bug
		if (*(int*)&m_VMR9ColorCon.MinValue == 1036830720) {
			m_VMR9ColorCon.MinValue = 0.11f; //fix nvidia bug
		}
		m_ColorCon.MinValue		= (int)floor(m_VMR9ColorCon.MinValue * 100 + 0.5) - 100;
		m_ColorCon.MaxValue		= (int)floor(m_VMR9ColorCon.MaxValue * 100 + 0.5) - 100;
		m_ColorCon.DefaultValue	= (int)floor(m_VMR9ColorCon.DefaultValue * 100 + 0.5) - 100;
		m_ColorCon.StepSize		= max(1, (int)(m_VMR9ColorCon.StepSize * 100 + 0.5));
		// Hue
		m_ColorHue.MinValue		= (int)floor(m_VMR9ColorHue.MinValue + 0.5);
		m_ColorHue.MaxValue		= (int)floor(m_VMR9ColorHue.MaxValue + 0.5);
		m_ColorHue.DefaultValue	= (int)floor(m_VMR9ColorHue.DefaultValue + 0.5);
		m_ColorHue.StepSize		= max(1,(int)(m_VMR9ColorHue.StepSize + 0.5));
		// Saturation
		m_ColorSat.MinValue		= (int)floor(m_VMR9ColorSat.MinValue * 100 + 0.5) - 100;
		m_ColorSat.MaxValue		= (int)floor(m_VMR9ColorSat.MaxValue * 100 + 0.5) - 100;
		m_ColorSat.DefaultValue	= (int)floor(m_VMR9ColorSat.DefaultValue * 100 + 0.5) - 100;
		m_ColorSat.StepSize		= max(1, (int)(m_VMR9ColorSat.StepSize * 100 + 0.5));
	}

	// limit the minimum and maximum values
	// Brightness
	if (m_ColorBri.MinValue < -100) m_ColorBri.MinValue = -100;
	if (m_ColorBri.MaxValue > 100) m_ColorBri.MaxValue = 100;
	// Contrast
	if (m_ColorCon.MinValue < -100) m_ColorCon.MinValue = -100;
	if (m_ColorCon.MaxValue > 100) m_ColorCon.MaxValue = 100;
	// Hue
	if (m_ColorHue.MinValue < -180) m_ColorHue.MinValue = -180;
	if (m_ColorHue.MaxValue > 180) m_ColorHue.MaxValue = 180;
	// Saturation
	if (m_ColorSat.MinValue < -100) m_ColorSat.MinValue = -100;
	if (m_ColorSat.MaxValue > 100) m_ColorSat.MaxValue = 100;
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
	return NULL;
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
	return NULL;
}
