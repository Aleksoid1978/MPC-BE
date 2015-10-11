// CDPI class is based on an example from the article "Writing DPI-Aware Win32 Applications".
// http://download.microsoft.com/download/1/f/e/1fe476f5-2b7a-4af1-a0ed-768454a0b5b1/Writing%20DPI%20Aware%20Applications.pdf

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
    int _dpiX = 96;
    int _dpiY = 96;

private:
    void _Init()
    {
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            _dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            _dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }
    }

public:
    CDPI() { _Init(); }

    // Get screen DPI.
    int GetDPIX() { return _dpiX; }
    int GetDPIY() { return _dpiY; }

    // Convert between raw pixels and relative pixels.
    int ScaleX(int x) { return MulDiv(x, _dpiX, 96); }
    int ScaleY(int y) { return MulDiv(y, _dpiY, 96); }
    int UnscaleX(int x) { return MulDiv(x, 96, _dpiX); }
    int UnscaleY(int y) { return MulDiv(y, 96, _dpiY); }

    // Scale rectangle from raw pixels to relative pixels.
    void ScaleRect(__inout RECT *pRect)
    {
        pRect->left = ScaleX(pRect->left);
        pRect->right = ScaleX(pRect->right);
        pRect->top = ScaleY(pRect->top);
        pRect->bottom = ScaleY(pRect->bottom);
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    int PointsToPixels(int pt) { return MulDiv(pt, _dpiY, 72); }

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
				if (S_OK == pGetDpiForMonitor(MonitorFromWindow(hWindow, MONITOR_DEFAULTTONULL), MDT_EFFECTIVE_DPI, &dpix, &dpiy)) {
					_dpiX = dpix;
					_dpiY = dpiy;
				}
			}
        }
    }

    void OverrideDPI(int dpix, int dpiy)
    {
        _dpiX = dpix;
        _dpiY = dpiy;
    }
};
