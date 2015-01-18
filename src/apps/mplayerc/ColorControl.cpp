/*
 * (C) 2014 see Authors.txt
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
{
	memset(&m_ColorControl, 0, sizeof(m_ColorControl));
	ResetColorControlRange();

	memset(&m_VMR9ColorControl, 0, sizeof(m_VMR9ColorControl));
	m_VMR9ColorControl[0].dwSize		= sizeof (VMR9ProcAmpControlRange);
	m_VMR9ColorControl[0].dwProperty	= ProcAmpControl9_Brightness;
	m_VMR9ColorControl[1].dwSize		= sizeof (VMR9ProcAmpControlRange);
	m_VMR9ColorControl[1].dwProperty	= ProcAmpControl9_Contrast;
	m_VMR9ColorControl[2].dwSize		= sizeof (VMR9ProcAmpControlRange);
	m_VMR9ColorControl[2].dwProperty	= ProcAmpControl9_Hue;
	m_VMR9ColorControl[3].dwSize		= sizeof (VMR9ProcAmpControlRange);
	m_VMR9ColorControl[3].dwProperty	= ProcAmpControl9_Saturation;

	memset(&m_EVRColorControl, 0, sizeof(m_EVRColorControl));
}

COLORPROPERTY_RANGE* CColorControl::GetColorControl(ControlType nFlag)
{
	switch (nFlag) {
		case ProcAmp_Brightness :
			return &m_ColorControl[0];
		case ProcAmp_Contrast :
			return &m_ColorControl[1];
		case ProcAmp_Hue :
			return &m_ColorControl[2];
		case ProcAmp_Saturation :
			return &m_ColorControl[3];
	}

	return NULL;
}

void CColorControl::ResetColorControlRange()
{
	m_ColorControl[0].dwProperty	= ProcAmp_Brightness;
	m_ColorControl[0].MinValue		= -100;
	m_ColorControl[0].MaxValue		= 100;
	m_ColorControl[0].DefaultValue	= 0;
	m_ColorControl[0].StepSize		= 1;
	m_ColorControl[1].dwProperty	= ProcAmp_Contrast;
	m_ColorControl[1].MinValue		= -100;
	m_ColorControl[1].MaxValue		= 100;
	m_ColorControl[1].DefaultValue	= 0;
	m_ColorControl[1].StepSize		= 1;
	m_ColorControl[2].dwProperty	= ProcAmp_Hue;
	m_ColorControl[2].MinValue		= -180;
	m_ColorControl[2].MaxValue		= 180;
	m_ColorControl[2].DefaultValue	= 0;
	m_ColorControl[2].StepSize		= 1;
	m_ColorControl[3].dwProperty	= ProcAmp_Saturation;
	m_ColorControl[3].MinValue		= -100;
	m_ColorControl[3].MaxValue		= 100;
	m_ColorControl[3].DefaultValue	= 0;
	m_ColorControl[3].StepSize		= 1;
}

void CColorControl::UpdateColorControlRange(bool isEVR)
{
	if (isEVR) {
		// Brightness
		m_ColorControl[0].MinValue		= FixedToInt(m_EVRColorControl[0].MinValue);
		m_ColorControl[0].MaxValue		= FixedToInt(m_EVRColorControl[0].MaxValue);
		m_ColorControl[0].DefaultValue	= FixedToInt(m_EVRColorControl[0].DefaultValue);
		m_ColorControl[0].StepSize		= max(1, FixedToInt(m_EVRColorControl[0].StepSize));
		// Contrast
		m_ColorControl[1].MinValue		= FixedToInt(m_EVRColorControl[1].MinValue, 100) - 100;
		m_ColorControl[1].MaxValue		= FixedToInt(m_EVRColorControl[1].MaxValue, 100) - 100;
		m_ColorControl[1].DefaultValue	= FixedToInt(m_EVRColorControl[1].DefaultValue, 100) - 100;
		m_ColorControl[1].StepSize		= max(1, FixedToInt(m_EVRColorControl[1].StepSize, 100));
		// Hue
		m_ColorControl[2].MinValue		= FixedToInt(m_EVRColorControl[2].MinValue);
		m_ColorControl[2].MaxValue		= FixedToInt(m_EVRColorControl[2].MaxValue);
		m_ColorControl[2].DefaultValue	= FixedToInt(m_EVRColorControl[2].DefaultValue);
		m_ColorControl[2].StepSize		= max(1, FixedToInt(m_EVRColorControl[2].StepSize));
		// Saturation
		m_ColorControl[3].MinValue		= FixedToInt(m_EVRColorControl[3].MinValue, 100) - 100;
		m_ColorControl[3].MaxValue		= FixedToInt(m_EVRColorControl[3].MaxValue, 100) - 100;
		m_ColorControl[3].DefaultValue	= FixedToInt(m_EVRColorControl[3].DefaultValue, 100) - 100;
		m_ColorControl[3].StepSize		= max(1, FixedToInt(m_EVRColorControl[3].StepSize, 100));
	} else {
		// Brightness
		m_ColorControl[0].MinValue		= (int)floor(m_VMR9ColorControl[0].MinValue + 0.5);
		m_ColorControl[0].MaxValue		= (int)floor(m_VMR9ColorControl[0].MaxValue + 0.5);
		m_ColorControl[0].DefaultValue	= (int)floor(m_VMR9ColorControl[0].DefaultValue + 0.5);
		m_ColorControl[0].StepSize		= max(1,(int)(m_VMR9ColorControl[0].StepSize + 0.5));
		// Contrast
		//if(m_VMR9ColorControl[1].MinValue == 0.0999908447265625) m_VMR9ColorControl[1].MinValue = 0.11; //fix nvidia bug
		if (*(int*)&m_VMR9ColorControl[1].MinValue == 1036830720) {
			m_VMR9ColorControl[1].MinValue = 0.11f; //fix nvidia bug
		}
		m_ColorControl[1].MinValue		= (int)floor(m_VMR9ColorControl[1].MinValue * 100 + 0.5) - 100;
		m_ColorControl[1].MaxValue		= (int)floor(m_VMR9ColorControl[1].MaxValue * 100 + 0.5) - 100;
		m_ColorControl[1].DefaultValue	= (int)floor(m_VMR9ColorControl[1].DefaultValue * 100 + 0.5) - 100;
		m_ColorControl[1].StepSize		= max(1, (int)(m_VMR9ColorControl[1].StepSize * 100 + 0.5));
		// Hue
		m_ColorControl[2].MinValue		= (int)floor(m_VMR9ColorControl[2].MinValue + 0.5);
		m_ColorControl[2].MaxValue		= (int)floor(m_VMR9ColorControl[2].MaxValue + 0.5);
		m_ColorControl[2].DefaultValue	= (int)floor(m_VMR9ColorControl[2].DefaultValue + 0.5);
		m_ColorControl[2].StepSize		= max(1,(int)(m_VMR9ColorControl[2].StepSize + 0.5));
		// Saturation
		m_ColorControl[3].MinValue		= (int)floor(m_VMR9ColorControl[3].MinValue * 100 + 0.5) - 100;
		m_ColorControl[3].MaxValue		= (int)floor(m_VMR9ColorControl[3].MaxValue * 100 + 0.5) - 100;
		m_ColorControl[3].DefaultValue	= (int)floor(m_VMR9ColorControl[3].DefaultValue * 100 + 0.5) - 100;
		m_ColorControl[3].StepSize		= max(1, (int)(m_VMR9ColorControl[3].StepSize * 100 + 0.5));
	}

	// limit the minimum and maximum values
	// Brightness
	if (m_ColorControl[0].MinValue < -100) m_ColorControl[0].MinValue = -100;
	if (m_ColorControl[0].MaxValue > 100) m_ColorControl[0].MaxValue = 100;
	// Contrast
	if (m_ColorControl[1].MinValue < -100) m_ColorControl[1].MinValue = -100;
	if (m_ColorControl[1].MaxValue > 100) m_ColorControl[1].MaxValue = 100;
	// Hue
	if (m_ColorControl[2].MinValue < -180) m_ColorControl[2].MinValue = -180;
	if (m_ColorControl[2].MaxValue > 180) m_ColorControl[2].MaxValue = 180;
	// Saturation
	if (m_ColorControl[3].MinValue < -100) m_ColorControl[3].MinValue = -100;
	if (m_ColorControl[3].MaxValue > 100) m_ColorControl[3].MaxValue = 100;
}

VMR9ProcAmpControlRange* CColorControl::GetVMR9ColorControl(ControlType nFlag)
{
	switch (nFlag) {
		case ProcAmp_Brightness :
			return &m_VMR9ColorControl[0];
		case ProcAmp_Contrast :
			return &m_VMR9ColorControl[1];
		case ProcAmp_Hue :
			return &m_VMR9ColorControl[2];
		case ProcAmp_Saturation :
			return &m_VMR9ColorControl[3];
	}
	return NULL;
}

DXVA2_ValueRange* CColorControl::GetEVRColorControl(ControlType nFlag)
{
	switch (nFlag) {
		case ProcAmp_Brightness :
			return &m_EVRColorControl[0];
		case ProcAmp_Contrast :
			return &m_EVRColorControl[1];
		case ProcAmp_Hue :
			return &m_EVRColorControl[2];
		case ProcAmp_Saturation :
			return &m_EVRColorControl[3];
	}
	return NULL;
}
