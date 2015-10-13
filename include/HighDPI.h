/*
 * (C) 2015 see Authors.txt
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

namespace
{
    typedef enum MONITOR_DPI_TYPE {
        MDT_EFFECTIVE_DPI = 0,
        MDT_ANGULAR_DPI = 1,
        MDT_RAW_DPI = 2,
        MDT_DEFAULT = MDT_EFFECTIVE_DPI
    } MONITOR_DPI_TYPE;

    typedef HRESULT (WINAPI *tpGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT* dpiX, UINT* dpiY);
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
        HDC hdc = GetDC(NULL);
        if (hdc) {
            m_dpiX = m_sdpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            m_dpiY = m_sdpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }
    }

public:
    CDPI() { Init(); }

    // Get screen DPI.
    int GetDPIX() const { return m_dpiX; }
    int GetDPIY() const { return m_dpiY; }

    // Convert between raw pixels and relative pixels.
    inline int ScaleX(int x) const { return MulDiv(x, m_dpiX, 96); }
    inline int ScaleY(int y) const { return MulDiv(y, m_dpiY, 96); }
    inline int UnscaleX(int x) const { return MulDiv(x, 96, m_dpiX); }
    inline int UnscaleY(int y) const { return MulDiv(y, 96, m_dpiY); }

    inline int ScaleSystemToOverrideX(int x) const { return MulDiv(x, m_dpiX, m_sdpiX); }
    inline int ScaleSystemToOverrideY(int y) const { return MulDiv(y, m_dpiY, m_sdpiY); }

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

protected:
    void UseCurentMonitorDPI(HWND hWindow)
    {
        static OSVERSIONINFO osvi = { sizeof(osvi) };
        if (osvi.dwMajorVersion == 0) {
            GetVersionEx(&osvi);
        }

        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 3 || osvi.dwMajorVersion > 6) {
            static HMODULE m_hShellScalingAPI = NULL;
            static tpGetDpiForMonitor pGetDpiForMonitor = NULL;

            if (!m_hShellScalingAPI) {
                m_hShellScalingAPI = LoadLibrary(L"Shcore.dll");
            }

            if (m_hShellScalingAPI && !pGetDpiForMonitor) {
                pGetDpiForMonitor = (tpGetDpiForMonitor)GetProcAddress(m_hShellScalingAPI, "GetDpiForMonitor");
            }

            if (pGetDpiForMonitor) {
                UINT dpix, dpiy;
                if (S_OK == pGetDpiForMonitor(MonitorFromWindow(hWindow, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI, &dpix, &dpiy)) {
                    m_dpiX = dpix;
                    m_dpiY = dpiy;
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
