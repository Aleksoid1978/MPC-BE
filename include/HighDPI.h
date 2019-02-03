/*
 * (C) 2015-2017 see Authors.txt
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
 * CDPI class is based on an example from the article "Writing DPI-Aware Win32 Applications".
 * http://download.microsoft.com/download/1/f/e/1fe476f5-2b7a-4af1-a0ed-768454a0b5b1/Writing%20DPI%20Aware%20Applications.pdf
 */

#pragma once

#include "../src/DSUtil/SysVersion.h"

namespace
{
    typedef enum MONITOR_DPI_TYPE {
        MDT_EFFECTIVE_DPI = 0,
        MDT_ANGULAR_DPI = 1,
        MDT_RAW_DPI = 2,
        MDT_DEFAULT = MDT_EFFECTIVE_DPI
    } MONITOR_DPI_TYPE;

    typedef HRESULT (WINAPI *tpGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT* dpiX, UINT* dpiY);
    typedef int (WINAPI *tpGetSystemMetricsForDpi)(int nIndex, UINT dpi);
}

// Definition: relative pixel = 1 pixel at 96 DPI and scaled based on actual DPI.
class CDPI
{
private:
    int m_dpiX  = 96;
    int m_dpiY  = 96;
    int m_sdpiX = 96;
    int m_sdpiY = 96;

private:
    void Init()
    {
        HDC hdc = GetDC(nullptr);
        if (hdc) {
            m_dpiX = m_sdpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            m_dpiY = m_sdpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(nullptr, hdc);
        }
    }

public:
    CDPI() { Init(); }

    // Get screen DPI.
    int GetDPIX() const { return m_dpiX; }
    int GetDPIY() const { return m_dpiY; }

    int GetDPIScalePercent() const { return 100 * m_dpiX / 96; }

    // Convert between raw pixels and relative pixels.
    inline int ScaleX(int x) const { return MulDiv(x, m_dpiX, 96); }
    inline int ScaleY(int y) const { return MulDiv(y, m_dpiY, 96); }
    inline int ScaleFloorX(int x) const { return x * m_dpiX / 96; }
    inline int ScaleFloorY(int y) const { return y * m_dpiY / 96; }

	inline int TransposeXtoY(int x) const { return MulDiv(x, m_dpiY, m_dpiX); }
	inline int TransposeYtoX(int y) const { return MulDiv(y, m_dpiX, m_dpiY); }

    inline int ScaleSystemToMonitorX(int x) const { return MulDiv(x, m_dpiX, m_sdpiX); }
    inline int ScaleSystemToMonitorY(int y) const { return MulDiv(y, m_dpiY, m_sdpiY); }

    // Scale rectangle from raw pixels to relative pixels.
    inline void ScaleRect(__inout RECT *pRect)
    {
        pRect->left = ScaleX(pRect->left);
        pRect->right = ScaleX(pRect->right);
        pRect->top = ScaleY(pRect->top);
        pRect->bottom = ScaleY(pRect->bottom);
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    inline int PointsToPixels(int pt) const { return MulDiv(pt, m_dpiY, 72); }

    int GetSystemMetricsDPI(int nIndex)
    {
        if (SysVersion::IsWin10orLater()) {
            static tpGetSystemMetricsForDpi pGetSystemMetricsForDpi = (tpGetSystemMetricsForDpi)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi");
            if (pGetSystemMetricsForDpi) {
                return pGetSystemMetricsForDpi(nIndex, GetDPIScalePercent());
            }
        }

        return ScaleSystemToMonitorX(::GetSystemMetrics(nIndex));
    }

protected:
    void UseCurentMonitorDPI(HWND hWindow)
    {
        if (SysVersion::IsWin8orLater()) {
            static HMODULE hShcore = LoadLibraryW(L"Shcore.dll");
            if (hShcore) {
                static tpGetDpiForMonitor pGetDpiForMonitor = (tpGetDpiForMonitor)GetProcAddress(hShcore, "GetDpiForMonitor");
                if (pGetDpiForMonitor) {
                    UINT dpix, dpiy;
                    if (S_OK == pGetDpiForMonitor(MonitorFromWindow(hWindow, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI, &dpix, &dpiy)) {
                        m_dpiX = dpix;
                        m_dpiY = dpiy;
                    }
                }
            }
        }
    }

    void OverrideDPI(int dpix, int dpiy)
    {
        m_dpiX = dpix;
        m_dpiY = dpiy;
    }
};
